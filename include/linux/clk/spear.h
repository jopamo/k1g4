/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2020 STMicroelectronics - All Rights Reserved
 *
 * Author: Lee Jones <lee.jones@linaro.org>
 */

#ifndef __LINUX_CLK_SPEAR_H
#define __LINUX_CLK_SPEAR_H

static inline void __init spear3xx_clk_init(void __iomem *misc_base,
					    void __iomem *soc_config_base) {}

static inline void __init spear6xx_clk_init(void __iomem *misc_base) {}

static inline void spear1310_clk_init(void __iomem *misc_base, void __iomem *ras_base) {}

static inline void spear1340_clk_init(void __iomem *misc_base) {}

#endif
