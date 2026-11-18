// SPDX-License-Identifier: GPL-2.0
#include <linux/clk-provider.h>
#include <linux/clk/at91_pmc.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/slab.h>

#include "pmc.h"

#define MASTER_SOURCE_MAX	4

#define PERIPHERAL_AT91RM9200	0
#define PERIPHERAL_AT91SAM9X5	1

#define PERIPHERAL_MAX		64

#define PERIPHERAL_ID_MIN	2

#define PROG_SOURCE_MAX		5
#define PROG_ID_MAX		7

#define SYSTEM_MAX_ID		31

#define GCK_INDEX_DT_AUDIO_PLL	5

static DEFINE_SPINLOCK(mck_lock);


static const struct clk_pcr_layout dt_pcr_layout = {
	.offset = 0x10c,
	.cmd = BIT(12),
	.pid_mask = GENMASK(5, 0),
	.div_mask = GENMASK(17, 16),
	.gckcss_mask = GENMASK(10, 8),
};




static void __init of_at91rm9200_clk_main_osc_setup(struct device_node *np)
{
	struct clk_hw *hw;
	const char *name = np->name;
	const char *parent_name;
	struct regmap *regmap;
	bool bypass;
	struct device_node *parent_np;

	of_property_read_string(np, "clock-output-names", &name);
	bypass = of_property_read_bool(np, "atmel,osc-bypass");
	parent_name = of_clk_get_parent_name(np, 0);

	parent_np = of_get_parent(np);
	regmap = syscon_node_to_regmap(parent_np);
	of_node_put(parent_np);
	if (IS_ERR(regmap))
		return;

	hw = at91_clk_register_main_osc(regmap, name, parent_name, NULL, bypass);
	if (IS_ERR(hw))
		return;

	of_clk_add_hw_provider(np, of_clk_hw_simple_get, hw);
}
CLK_OF_DECLARE(at91rm9200_clk_main_osc, "atmel,at91rm9200-clk-main-osc",
	       of_at91rm9200_clk_main_osc_setup);

static void __init of_at91sam9x5_clk_main_rc_osc_setup(struct device_node *np)
{
	struct clk_hw *hw;
	u32 frequency = 0;
	u32 accuracy = 0;
	const char *name = np->name;
	struct regmap *regmap;
	struct device_node *parent_np;

	of_property_read_string(np, "clock-output-names", &name);
	of_property_read_u32(np, "clock-frequency", &frequency);
	of_property_read_u32(np, "clock-accuracy", &accuracy);

	parent_np = of_get_parent(np);
	regmap = syscon_node_to_regmap(parent_np);
	of_node_put(parent_np);
	if (IS_ERR(regmap))
		return;

	hw = at91_clk_register_main_rc_osc(regmap, name, frequency, accuracy);
	if (IS_ERR(hw))
		return;

	of_clk_add_hw_provider(np, of_clk_hw_simple_get, hw);
}
CLK_OF_DECLARE(at91sam9x5_clk_main_rc_osc, "atmel,at91sam9x5-clk-main-rc-osc",
	       of_at91sam9x5_clk_main_rc_osc_setup);

static void __init of_at91rm9200_clk_main_setup(struct device_node *np)
{
	struct clk_hw *hw;
	const char *parent_name;
	const char *name = np->name;
	struct regmap *regmap;
	struct device_node *parent_np;

	parent_name = of_clk_get_parent_name(np, 0);
	of_property_read_string(np, "clock-output-names", &name);

	parent_np = of_get_parent(np);
	regmap = syscon_node_to_regmap(parent_np);
	of_node_put(parent_np);
	if (IS_ERR(regmap))
		return;

	hw = at91_clk_register_rm9200_main(regmap, name, parent_name, NULL);
	if (IS_ERR(hw))
		return;

	of_clk_add_hw_provider(np, of_clk_hw_simple_get, hw);
}
CLK_OF_DECLARE(at91rm9200_clk_main, "atmel,at91rm9200-clk-main",
	       of_at91rm9200_clk_main_setup);

static void __init of_at91sam9x5_clk_main_setup(struct device_node *np)
{
	struct clk_hw *hw;
	const char *parent_names[2];
	unsigned int num_parents;
	const char *name = np->name;
	struct regmap *regmap;
	struct device_node *parent_np;

	num_parents = of_clk_get_parent_count(np);
	if (num_parents == 0 || num_parents > 2)
		return;

	of_clk_parent_fill(np, parent_names, num_parents);
	parent_np = of_get_parent(np);
	regmap = syscon_node_to_regmap(parent_np);
	of_node_put(parent_np);
	if (IS_ERR(regmap))
		return;

	of_property_read_string(np, "clock-output-names", &name);

	hw = at91_clk_register_sam9x5_main(regmap, name, parent_names, NULL,
					   num_parents);
	if (IS_ERR(hw))
		return;

	of_clk_add_hw_provider(np, of_clk_hw_simple_get, hw);
}
CLK_OF_DECLARE(at91sam9x5_clk_main, "atmel,at91sam9x5-clk-main",
	       of_at91sam9x5_clk_main_setup);

static struct clk_master_characteristics * __init
of_at91_clk_master_get_characteristics(struct device_node *np)
{
	struct clk_master_characteristics *characteristics;

	characteristics = kzalloc_obj(*characteristics);
	if (!characteristics)
		return NULL;

	if (of_at91_get_clk_range(np, "atmel,clk-output-range", &characteristics->output))
		goto out_free_characteristics;

	of_property_read_u32_array(np, "atmel,clk-divisors",
				   characteristics->divisors, 4);

	characteristics->have_div3_pres =
		of_property_read_bool(np, "atmel,master-clk-have-div3-pres");

	return characteristics;

out_free_characteristics:
	kfree(characteristics);
	return NULL;
}

static void __init
of_at91_clk_master_setup(struct device_node *np,
			 const struct clk_master_layout *layout)
{
	struct clk_hw *hw;
	unsigned int num_parents;
	const char *parent_names[MASTER_SOURCE_MAX];
	const char *name = np->name;
	struct clk_master_characteristics *characteristics;
	struct regmap *regmap;
	struct device_node *parent_np;

	num_parents = of_clk_get_parent_count(np);
	if (num_parents == 0 || num_parents > MASTER_SOURCE_MAX)
		return;

	of_clk_parent_fill(np, parent_names, num_parents);

	of_property_read_string(np, "clock-output-names", &name);

	characteristics = of_at91_clk_master_get_characteristics(np);
	if (!characteristics)
		return;

	parent_np = of_get_parent(np);
	regmap = syscon_node_to_regmap(parent_np);
	of_node_put(parent_np);
	if (IS_ERR(regmap))
		return;

	hw = at91_clk_register_master_pres(regmap, "masterck_pres", num_parents,
					   parent_names, NULL, layout,
					   characteristics, &mck_lock);
	if (IS_ERR(hw))
		goto out_free_characteristics;

	hw = at91_clk_register_master_div(regmap, name, "masterck_pres", NULL,
					  layout, characteristics,
					  &mck_lock, CLK_SET_RATE_GATE, 0);
	if (IS_ERR(hw))
		goto out_free_characteristics;

	of_clk_add_hw_provider(np, of_clk_hw_simple_get, hw);
	return;

out_free_characteristics:
	kfree(characteristics);
}

static void __init of_at91rm9200_clk_master_setup(struct device_node *np)
{
	of_at91_clk_master_setup(np, &at91rm9200_master_layout);
}
CLK_OF_DECLARE(at91rm9200_clk_master, "atmel,at91rm9200-clk-master",
	       of_at91rm9200_clk_master_setup);

static void __init of_at91sam9x5_clk_master_setup(struct device_node *np)
{
	of_at91_clk_master_setup(np, &at91sam9x5_master_layout);
}
CLK_OF_DECLARE(at91sam9x5_clk_master, "atmel,at91sam9x5-clk-master",
	       of_at91sam9x5_clk_master_setup);

static void __init
of_at91_clk_periph_setup(struct device_node *np, u8 type)
{
	int num;
	u32 id;
	struct clk_hw *hw;
	const char *parent_name;
	const char *name;
	struct device_node *periphclknp;
	struct regmap *regmap;
	struct device_node *parent_np;

	parent_name = of_clk_get_parent_name(np, 0);
	if (!parent_name)
		return;

	num = of_get_child_count(np);
	if (!num || num > PERIPHERAL_MAX)
		return;

	parent_np = of_get_parent(np);
	regmap = syscon_node_to_regmap(parent_np);
	of_node_put(parent_np);
	if (IS_ERR(regmap))
		return;

	for_each_child_of_node(np, periphclknp) {
		if (of_property_read_u32(periphclknp, "reg", &id))
			continue;

		if (id >= PERIPHERAL_MAX)
			continue;

		if (of_property_read_string(np, "clock-output-names", &name))
			name = periphclknp->name;

		if (type == PERIPHERAL_AT91RM9200) {
			hw = at91_clk_register_peripheral(regmap, name,
							  parent_name, NULL, id);
		} else {
			struct clk_range range = CLK_RANGE(0, 0);
			unsigned long flags = 0;

			of_at91_get_clk_range(periphclknp,
					      "atmel,clk-output-range",
					      &range);

			/*
			 * mpddr_clk feed DDR controller and is enabled by
			 * bootloader thus we need to keep it enabled in case
			 * there is no Linux consumer for it.
			 */
			if (!strcmp(periphclknp->name, "mpddr_clk"))
				flags = CLK_IS_CRITICAL;

			hw = at91_clk_register_sam9x5_peripheral(regmap,
								 &pmc_pcr_lock,
								 &dt_pcr_layout,
								 name,
								 parent_name,
								 NULL,
								 id, &range,
								 INT_MIN,
								 flags);
		}

		if (IS_ERR(hw))
			continue;

		of_clk_add_hw_provider(periphclknp, of_clk_hw_simple_get, hw);
	}
}

static void __init of_at91rm9200_clk_periph_setup(struct device_node *np)
{
	of_at91_clk_periph_setup(np, PERIPHERAL_AT91RM9200);
}
CLK_OF_DECLARE(at91rm9200_clk_periph, "atmel,at91rm9200-clk-peripheral",
	       of_at91rm9200_clk_periph_setup);

static void __init of_at91sam9x5_clk_periph_setup(struct device_node *np)
{
	of_at91_clk_periph_setup(np, PERIPHERAL_AT91SAM9X5);
}
CLK_OF_DECLARE(at91sam9x5_clk_periph, "atmel,at91sam9x5-clk-peripheral",
	       of_at91sam9x5_clk_periph_setup);

static struct clk_pll_characteristics * __init
of_at91_clk_pll_get_characteristics(struct device_node *np)
{
	int i;
	int offset;
	u32 tmp;
	int num_output;
	u32 num_cells;
	struct clk_range input;
	struct clk_range *output;
	u8 *out = NULL;
	u16 *icpll = NULL;
	struct clk_pll_characteristics *characteristics;

	if (of_at91_get_clk_range(np, "atmel,clk-input-range", &input))
		return NULL;

	if (of_property_read_u32(np, "#atmel,pll-clk-output-range-cells",
				 &num_cells))
		return NULL;

	if (num_cells < 2 || num_cells > 4)
		return NULL;

	num_output = of_property_count_u32_elems(np, "atmel,pll-clk-output-ranges");
	if (num_output <= 0)
		return NULL;
	num_output /= num_cells;

	characteristics = kzalloc_obj(*characteristics);
	if (!characteristics)
		return NULL;

	output = kzalloc_objs(*output, num_output);
	if (!output)
		goto out_free_characteristics;

	if (num_cells > 2) {
		out = kcalloc(num_output, sizeof(*out), GFP_KERNEL);
		if (!out)
			goto out_free_output;
	}

	if (num_cells > 3) {
		icpll = kzalloc_objs(*icpll, num_output);
		if (!icpll)
			goto out_free_output;
	}

	for (i = 0; i < num_output; i++) {
		offset = i * num_cells;
		if (of_property_read_u32_index(np,
					       "atmel,pll-clk-output-ranges",
					       offset, &tmp))
			goto out_free_output;
		output[i].min = tmp;
		if (of_property_read_u32_index(np,
					       "atmel,pll-clk-output-ranges",
					       offset + 1, &tmp))
			goto out_free_output;
		output[i].max = tmp;

		if (num_cells == 2)
			continue;

		if (of_property_read_u32_index(np,
					       "atmel,pll-clk-output-ranges",
					       offset + 2, &tmp))
			goto out_free_output;
		out[i] = tmp;

		if (num_cells == 3)
			continue;

		if (of_property_read_u32_index(np,
					       "atmel,pll-clk-output-ranges",
					       offset + 3, &tmp))
			goto out_free_output;
		icpll[i] = tmp;
	}

	characteristics->input = input;
	characteristics->num_output = num_output;
	characteristics->output = output;
	characteristics->out = out;
	characteristics->icpll = icpll;
	return characteristics;

out_free_output:
	kfree(icpll);
	kfree(out);
	kfree(output);
out_free_characteristics:
	kfree(characteristics);
	return NULL;
}

static void __init
of_at91_clk_pll_setup(struct device_node *np,
		      const struct clk_pll_layout *layout)
{
	u32 id;
	struct clk_hw *hw;
	struct regmap *regmap;
	const char *parent_name;
	const char *name = np->name;
	struct device_node *parent_np;
	struct clk_pll_characteristics *characteristics;

	if (of_property_read_u32(np, "reg", &id))
		return;

	parent_name = of_clk_get_parent_name(np, 0);

	of_property_read_string(np, "clock-output-names", &name);

	parent_np = of_get_parent(np);
	regmap = syscon_node_to_regmap(parent_np);
	of_node_put(parent_np);
	if (IS_ERR(regmap))
		return;

	characteristics = of_at91_clk_pll_get_characteristics(np);
	if (!characteristics)
		return;

	hw = at91_clk_register_pll(regmap, name, parent_name, id, layout,
				   characteristics);
	if (IS_ERR(hw))
		goto out_free_characteristics;

	of_clk_add_hw_provider(np, of_clk_hw_simple_get, hw);
	return;

out_free_characteristics:
	kfree(characteristics);
}

static void __init of_at91rm9200_clk_pll_setup(struct device_node *np)
{
	of_at91_clk_pll_setup(np, &at91rm9200_pll_layout);
}
CLK_OF_DECLARE(at91rm9200_clk_pll, "atmel,at91rm9200-clk-pll",
	       of_at91rm9200_clk_pll_setup);

static void __init of_at91sam9g45_clk_pll_setup(struct device_node *np)
{
	of_at91_clk_pll_setup(np, &at91sam9g45_pll_layout);
}
CLK_OF_DECLARE(at91sam9g45_clk_pll, "atmel,at91sam9g45-clk-pll",
	       of_at91sam9g45_clk_pll_setup);

static void __init of_at91sam9g20_clk_pllb_setup(struct device_node *np)
{
	of_at91_clk_pll_setup(np, &at91sam9g20_pllb_layout);
}
CLK_OF_DECLARE(at91sam9g20_clk_pllb, "atmel,at91sam9g20-clk-pllb",
	       of_at91sam9g20_clk_pllb_setup);

static void __init of_sama5d3_clk_pll_setup(struct device_node *np)
{
	of_at91_clk_pll_setup(np, &sama5d3_pll_layout);
}
CLK_OF_DECLARE(sama5d3_clk_pll, "atmel,sama5d3-clk-pll",
	       of_sama5d3_clk_pll_setup);

static void __init
of_at91sam9x5_clk_plldiv_setup(struct device_node *np)
{
	struct clk_hw *hw;
	const char *parent_name;
	const char *name = np->name;
	struct regmap *regmap;
	struct device_node *parent_np;

	parent_name = of_clk_get_parent_name(np, 0);

	of_property_read_string(np, "clock-output-names", &name);

	parent_np = of_get_parent(np);
	regmap = syscon_node_to_regmap(parent_np);
	of_node_put(parent_np);
	if (IS_ERR(regmap))
		return;

	hw = at91_clk_register_plldiv(regmap, name, parent_name);
	if (IS_ERR(hw))
		return;

	of_clk_add_hw_provider(np, of_clk_hw_simple_get, hw);
}
CLK_OF_DECLARE(at91sam9x5_clk_plldiv, "atmel,at91sam9x5-clk-plldiv",
	       of_at91sam9x5_clk_plldiv_setup);

static void __init
of_at91_clk_prog_setup(struct device_node *np,
		       const struct clk_programmable_layout *layout,
		       u32 *mux_table)
{
	int num;
	u32 id;
	struct clk_hw *hw;
	unsigned int num_parents;
	const char *parent_names[PROG_SOURCE_MAX];
	const char *name;
	struct device_node *progclknp, *parent_np;
	struct regmap *regmap;

	num_parents = of_clk_get_parent_count(np);
	if (num_parents == 0 || num_parents > PROG_SOURCE_MAX)
		return;

	of_clk_parent_fill(np, parent_names, num_parents);

	num = of_get_child_count(np);
	if (!num || num > (PROG_ID_MAX + 1))
		return;

	parent_np = of_get_parent(np);
	regmap = syscon_node_to_regmap(parent_np);
	of_node_put(parent_np);
	if (IS_ERR(regmap))
		return;

	for_each_child_of_node(np, progclknp) {
		if (of_property_read_u32(progclknp, "reg", &id))
			continue;

		if (of_property_read_string(np, "clock-output-names", &name))
			name = progclknp->name;

		hw = at91_clk_register_programmable(regmap, name,
						    parent_names, NULL, num_parents,
						    id, layout, mux_table);
		if (IS_ERR(hw))
			continue;

		of_clk_add_hw_provider(progclknp, of_clk_hw_simple_get, hw);
	}
}

static void __init of_at91rm9200_clk_prog_setup(struct device_node *np)
{
	of_at91_clk_prog_setup(np, &at91rm9200_programmable_layout, NULL);
}
CLK_OF_DECLARE(at91rm9200_clk_prog, "atmel,at91rm9200-clk-programmable",
	       of_at91rm9200_clk_prog_setup);

static void __init of_at91sam9g45_clk_prog_setup(struct device_node *np)
{
	of_at91_clk_prog_setup(np, &at91sam9g45_programmable_layout, NULL);
}
CLK_OF_DECLARE(at91sam9g45_clk_prog, "atmel,at91sam9g45-clk-programmable",
	       of_at91sam9g45_clk_prog_setup);

static void __init of_at91sam9x5_clk_prog_setup(struct device_node *np)
{
	of_at91_clk_prog_setup(np, &at91sam9x5_programmable_layout, NULL);
}
CLK_OF_DECLARE(at91sam9x5_clk_prog, "atmel,at91sam9x5-clk-programmable",
	       of_at91sam9x5_clk_prog_setup);

static void __init of_at91sam9260_clk_slow_setup(struct device_node *np)
{
	struct clk_hw *hw;
	const char *parent_names[2];
	unsigned int num_parents;
	const char *name = np->name;
	struct regmap *regmap;
	struct device_node *parent_np;

	num_parents = of_clk_get_parent_count(np);
	if (num_parents != 2)
		return;

	of_clk_parent_fill(np, parent_names, num_parents);
	parent_np = of_get_parent(np);
	regmap = syscon_node_to_regmap(parent_np);
	of_node_put(parent_np);
	if (IS_ERR(regmap))
		return;

	of_property_read_string(np, "clock-output-names", &name);

	hw = at91_clk_register_sam9260_slow(regmap, name, parent_names,
					    num_parents);
	if (IS_ERR(hw))
		return;

	of_clk_add_hw_provider(np, of_clk_hw_simple_get, hw);
}
CLK_OF_DECLARE(at91sam9260_clk_slow, "atmel,at91sam9260-clk-slow",
	       of_at91sam9260_clk_slow_setup);


static void __init of_at91rm9200_clk_sys_setup(struct device_node *np)
{
	int num;
	u32 id;
	struct clk_hw *hw;
	const char *name;
	struct device_node *sysclknp, *parent_np;
	const char *parent_name;
	struct regmap *regmap;

	num = of_get_child_count(np);
	if (num > (SYSTEM_MAX_ID + 1))
		return;

	parent_np = of_get_parent(np);
	regmap = syscon_node_to_regmap(parent_np);
	of_node_put(parent_np);
	if (IS_ERR(regmap))
		return;

	for_each_child_of_node(np, sysclknp) {
		unsigned long flags = 0;

		if (of_property_read_u32(sysclknp, "reg", &id))
			continue;

		if (of_property_read_string(np, "clock-output-names", &name))
			name = sysclknp->name;

		parent_name = of_clk_get_parent_name(sysclknp, 0);

		/*
		 * ddrck feeds DDR controller and is enabled by bootloader thus
		 * we need to keep it enabled in case there is no Linux consumer
		 * for it.
		 */
		if (!strcmp(sysclknp->name, "ddrck"))
			flags = CLK_IS_CRITICAL;

		hw = at91_clk_register_system(regmap, name, parent_name, NULL,
					      id, flags);
		if (IS_ERR(hw))
			continue;

		of_clk_add_hw_provider(sysclknp, of_clk_hw_simple_get, hw);
	}
}
CLK_OF_DECLARE(at91rm9200_clk_sys, "atmel,at91rm9200-clk-system",
	       of_at91rm9200_clk_sys_setup);


