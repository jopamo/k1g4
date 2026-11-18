/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2012, NVIDIA Corporation. All rights reserved.
 */

#ifndef __SOC_TEGRA_IRQ_H
#define __SOC_TEGRA_IRQ_H

#include <linux/types.h>

static inline bool tegra_pending_sgi(void)
{
	return false;
}

#endif /* __SOC_TEGRA_IRQ_H */
