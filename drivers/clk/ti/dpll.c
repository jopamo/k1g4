// SPDX-License-Identifier: GPL-2.0-only
/*
 * OMAP DPLL clock support
 *
 * Copyright (C) 2013 Texas Instruments, Inc.
 *
 * Tero Kristo <t-kristo@ti.com>
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/clk/ti.h>
#include "clock.h"

#undef pr_fmt
#define pr_fmt(fmt) "%s: " fmt, __func__


static const struct clk_ops dpll_core_ck_ops = {};
static const struct clk_ops dpll_ck_ops = {};
static const struct clk_ops dpll_no_gate_ck_ops = {};
const struct clk_hw_omap_ops clkhwops_omap3_dpll = {};

static const struct clk_ops omap2_dpll_core_ck_ops = {};



/**
 * _register_dpll - low level registration of a DPLL clock
 * @user: pointer to the hardware clock definition for the clock
 * @node: device node for the clock
 *
 * Finalizes DPLL registration process. In case a failure (clk-ref or
 * clk-bypass is missing), the clock is added to retry list and
 * the initialization is retried on later stage.
 */
static void __init _register_dpll(void *user,
				  struct device_node *node)
{
	struct clk_hw *hw = user;
	struct clk_hw_omap *clk_hw = to_clk_hw_omap(hw);
	struct dpll_data *dd = clk_hw->dpll_data;
	const char *name;
	struct clk *clk;
	const struct clk_init_data *init = hw->init;

	clk = of_clk_get(node, 0);
	if (IS_ERR(clk)) {
		pr_debug("clk-ref missing for %pOFn, retry later\n",
			 node);
		if (!ti_clk_retry_init(node, hw, _register_dpll))
			return;

		goto cleanup;
	}

	dd->clk_ref = __clk_get_hw(clk);

	clk = of_clk_get(node, 1);

	if (IS_ERR(clk)) {
		pr_debug("clk-bypass missing for %pOFn, retry later\n",
			 node);
		if (!ti_clk_retry_init(node, hw, _register_dpll))
			return;

		goto cleanup;
	}

	dd->clk_bypass = __clk_get_hw(clk);

	/* register the clock */
	name = ti_dt_clk_name(node);
	clk = of_ti_clk_register_omap_hw(node, &clk_hw->hw, name);

	if (!IS_ERR(clk)) {
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
		kfree(init->parent_names);
		kfree(init);
		return;
	}

cleanup:
	kfree(clk_hw->dpll_data);
	kfree(init->parent_names);
	kfree(init);
	kfree(clk_hw);
}


/**
 * of_ti_dpll_setup - Setup function for OMAP DPLL clocks
 * @node: device node containing the DPLL info
 * @ops: ops for the DPLL
 * @ddt: DPLL data template to use
 *
 * Initializes a DPLL clock from device tree data.
 */
static void __init of_ti_dpll_setup(struct device_node *node,
				    const struct clk_ops *ops,
				    const struct dpll_data *ddt)
{
	struct clk_hw_omap *clk_hw = NULL;
	struct clk_init_data *init = NULL;
	const char **parent_names = NULL;
	struct dpll_data *dd = NULL;
	int ssc_clk_index;
	u8 dpll_mode = 0;
	u32 min_div;

	dd = kmemdup(ddt, sizeof(*dd), GFP_KERNEL);
	clk_hw = kzalloc_obj(*clk_hw);
	init = kzalloc_obj(*init);
	if (!dd || !clk_hw || !init)
		goto cleanup;

	clk_hw->dpll_data = dd;
	clk_hw->ops = &clkhwops_omap3_dpll;
	clk_hw->hw.init = init;

	init->name = ti_dt_clk_name(node);
	init->ops = ops;

	init->num_parents = of_clk_get_parent_count(node);
	if (!init->num_parents) {
		pr_err("%pOFn must have parent(s)\n", node);
		goto cleanup;
	}

	parent_names = kcalloc(init->num_parents, sizeof(char *), GFP_KERNEL);
	if (!parent_names)
		goto cleanup;

	of_clk_parent_fill(node, parent_names, init->num_parents);

	init->parent_names = parent_names;

	if (ti_clk_get_reg_addr(node, 0, &dd->control_reg))
		goto cleanup;

	/*
	 * Special case for OMAP2 DPLL, register order is different due to
	 * missing idlest_reg, also clkhwops is different. Detected from
	 * missing idlest_mask.
	 */
	if (!dd->idlest_mask) {
		if (ti_clk_get_reg_addr(node, 1, &dd->mult_div1_reg))
			goto cleanup;
	} else {
		if (ti_clk_get_reg_addr(node, 1, &dd->idlest_reg))
			goto cleanup;

		if (ti_clk_get_reg_addr(node, 2, &dd->mult_div1_reg))
			goto cleanup;
	}

	if (dd->autoidle_mask) {
		if (ti_clk_get_reg_addr(node, 3, &dd->autoidle_reg))
			goto cleanup;

		ssc_clk_index = 4;
	} else {
		ssc_clk_index = 3;
	}

	if (dd->ssc_deltam_int_mask && dd->ssc_deltam_frac_mask &&
	    dd->ssc_modfreq_mant_mask && dd->ssc_modfreq_exp_mask) {
		if (ti_clk_get_reg_addr(node, ssc_clk_index++,
					&dd->ssc_deltam_reg))
			goto cleanup;

		if (ti_clk_get_reg_addr(node, ssc_clk_index++,
					&dd->ssc_modfreq_reg))
			goto cleanup;

		of_property_read_u32(node, "ti,ssc-modfreq-hz",
				     &dd->ssc_modfreq);
		of_property_read_u32(node, "ti,ssc-deltam", &dd->ssc_deltam);
		dd->ssc_downspread =
			of_property_read_bool(node, "ti,ssc-downspread");
	}

	if (of_property_read_bool(node, "ti,low-power-stop"))
		dpll_mode |= 1 << DPLL_LOW_POWER_STOP;

	if (of_property_read_bool(node, "ti,low-power-bypass"))
		dpll_mode |= 1 << DPLL_LOW_POWER_BYPASS;

	if (of_property_read_bool(node, "ti,lock"))
		dpll_mode |= 1 << DPLL_LOCKED;

	if (!of_property_read_u32(node, "ti,min-div", &min_div) &&
	    min_div > dd->min_divider)
		dd->min_divider = min_div;

	if (dpll_mode)
		dd->modes = dpll_mode;

	_register_dpll(&clk_hw->hw, node);
	return;

cleanup:
	kfree(dd);
	kfree(parent_names);
	kfree(init);
	kfree(clk_hw);
}




static void __init of_ti_omap4_dpll_setup(struct device_node *node)
{
	const struct dpll_data dd = {
		.idlest_mask = 0x1,
		.enable_mask = 0x7,
		.autoidle_mask = 0x7,
		.mult_mask = 0x7ff << 8,
		.div1_mask = 0x7f,
		.max_multiplier = 2047,
		.max_divider = 128,
		.min_divider = 1,
		.modes = (1 << DPLL_LOW_POWER_BYPASS) | (1 << DPLL_LOCKED),
	};

	of_ti_dpll_setup(node, &dpll_ck_ops, &dd);
}
CLK_OF_DECLARE(ti_omap4_dpll_clock, "ti,omap4-dpll-clock",
	       of_ti_omap4_dpll_setup);

static void __init of_ti_omap5_mpu_dpll_setup(struct device_node *node)
{
	const struct dpll_data dd = {
		.idlest_mask = 0x1,
		.enable_mask = 0x7,
		.autoidle_mask = 0x7,
		.mult_mask = 0x7ff << 8,
		.div1_mask = 0x7f,
		.max_multiplier = 2047,
		.max_divider = 128,
		.dcc_mask = BIT(22),
		.dcc_rate = 1400000000, /* DCC beyond 1.4GHz */
		.min_divider = 1,
		.modes = (1 << DPLL_LOW_POWER_BYPASS) | (1 << DPLL_LOCKED),
	};

	of_ti_dpll_setup(node, &dpll_ck_ops, &dd);
}
CLK_OF_DECLARE(of_ti_omap5_mpu_dpll_clock, "ti,omap5-mpu-dpll-clock",
	       of_ti_omap5_mpu_dpll_setup);

static void __init of_ti_omap4_core_dpll_setup(struct device_node *node)
{
	const struct dpll_data dd = {
		.idlest_mask = 0x1,
		.enable_mask = 0x7,
		.autoidle_mask = 0x7,
		.mult_mask = 0x7ff << 8,
		.div1_mask = 0x7f,
		.max_multiplier = 2047,
		.max_divider = 128,
		.min_divider = 1,
		.modes = (1 << DPLL_LOW_POWER_BYPASS) | (1 << DPLL_LOCKED),
	};

	of_ti_dpll_setup(node, &dpll_core_ck_ops, &dd);
}
CLK_OF_DECLARE(ti_omap4_core_dpll_clock, "ti,omap4-dpll-core-clock",
	       of_ti_omap4_core_dpll_setup);


static void __init of_ti_am3_no_gate_dpll_setup(struct device_node *node)
{
	const struct dpll_data dd = {
		.idlest_mask = 0x1,
		.enable_mask = 0x7,
		.ssc_enable_mask = 0x1 << 12,
		.ssc_downspread_mask = 0x1 << 14,
		.mult_mask = 0x7ff << 8,
		.div1_mask = 0x7f,
		.ssc_deltam_int_mask = 0x3 << 18,
		.ssc_deltam_frac_mask = 0x3ffff,
		.ssc_modfreq_mant_mask = 0x7f,
		.ssc_modfreq_exp_mask = 0x7 << 8,
		.max_multiplier = 2047,
		.max_divider = 128,
		.min_divider = 1,
		.max_rate = 1000000000,
		.modes = (1 << DPLL_LOW_POWER_BYPASS) | (1 << DPLL_LOCKED),
	};

	of_ti_dpll_setup(node, &dpll_no_gate_ck_ops, &dd);
}
CLK_OF_DECLARE(ti_am3_no_gate_dpll_clock, "ti,am3-dpll-no-gate-clock",
	       of_ti_am3_no_gate_dpll_setup);

static void __init of_ti_am3_jtype_dpll_setup(struct device_node *node)
{
	const struct dpll_data dd = {
		.idlest_mask = 0x1,
		.enable_mask = 0x7,
		.mult_mask = 0x7ff << 8,
		.div1_mask = 0x7f,
		.max_multiplier = 4095,
		.max_divider = 256,
		.min_divider = 2,
		.flags = DPLL_J_TYPE,
		.max_rate = 2000000000,
		.modes = (1 << DPLL_LOW_POWER_BYPASS) | (1 << DPLL_LOCKED),
	};

	of_ti_dpll_setup(node, &dpll_ck_ops, &dd);
}
CLK_OF_DECLARE(ti_am3_jtype_dpll_clock, "ti,am3-dpll-j-type-clock",
	       of_ti_am3_jtype_dpll_setup);

static void __init of_ti_am3_no_gate_jtype_dpll_setup(struct device_node *node)
{
	const struct dpll_data dd = {
		.idlest_mask = 0x1,
		.enable_mask = 0x7,
		.mult_mask = 0x7ff << 8,
		.div1_mask = 0x7f,
		.max_multiplier = 2047,
		.max_divider = 128,
		.min_divider = 1,
		.max_rate = 2000000000,
		.flags = DPLL_J_TYPE,
		.modes = (1 << DPLL_LOW_POWER_BYPASS) | (1 << DPLL_LOCKED),
	};

	of_ti_dpll_setup(node, &dpll_no_gate_ck_ops, &dd);
}
CLK_OF_DECLARE(ti_am3_no_gate_jtype_dpll_clock,
	       "ti,am3-dpll-no-gate-j-type-clock",
	       of_ti_am3_no_gate_jtype_dpll_setup);

static void __init of_ti_am3_dpll_setup(struct device_node *node)
{
	const struct dpll_data dd = {
		.idlest_mask = 0x1,
		.enable_mask = 0x7,
		.ssc_enable_mask = 0x1 << 12,
		.ssc_downspread_mask = 0x1 << 14,
		.mult_mask = 0x7ff << 8,
		.div1_mask = 0x7f,
		.ssc_deltam_int_mask = 0x3 << 18,
		.ssc_deltam_frac_mask = 0x3ffff,
		.ssc_modfreq_mant_mask = 0x7f,
		.ssc_modfreq_exp_mask = 0x7 << 8,
		.max_multiplier = 2047,
		.max_divider = 128,
		.min_divider = 1,
		.max_rate = 1000000000,
		.modes = (1 << DPLL_LOW_POWER_BYPASS) | (1 << DPLL_LOCKED),
	};

	of_ti_dpll_setup(node, &dpll_ck_ops, &dd);
}
CLK_OF_DECLARE(ti_am3_dpll_clock, "ti,am3-dpll-clock", of_ti_am3_dpll_setup);

static void __init of_ti_am3_core_dpll_setup(struct device_node *node)
{
	const struct dpll_data dd = {
		.idlest_mask = 0x1,
		.enable_mask = 0x7,
		.mult_mask = 0x7ff << 8,
		.div1_mask = 0x7f,
		.max_multiplier = 2047,
		.max_divider = 128,
		.min_divider = 1,
		.max_rate = 1000000000,
		.modes = (1 << DPLL_LOW_POWER_BYPASS) | (1 << DPLL_LOCKED),
	};

	of_ti_dpll_setup(node, &dpll_core_ck_ops, &dd);
}
CLK_OF_DECLARE(ti_am3_core_dpll_clock, "ti,am3-dpll-core-clock",
	       of_ti_am3_core_dpll_setup);

static void __init of_ti_omap2_core_dpll_setup(struct device_node *node)
{
	const struct dpll_data dd = {
		.enable_mask = 0x3,
		.mult_mask = 0x3ff << 12,
		.div1_mask = 0xf << 8,
		.max_divider = 16,
		.min_divider = 1,
	};

	of_ti_dpll_setup(node, &omap2_dpll_core_ck_ops, &dd);
}
CLK_OF_DECLARE(ti_omap2_core_dpll_clock, "ti,omap2-dpll-core-clock",
	       of_ti_omap2_core_dpll_setup);
