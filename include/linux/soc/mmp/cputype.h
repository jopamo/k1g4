/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_MACH_CPUTYPE_H
#define __ASM_MACH_CPUTYPE_H

#if defined(CONFIG_ARM64)
#include <asm/cputype.h>
#endif

/*
 *  CPU   Stepping   CPU_ID      CHIP_ID
 *
 * PXA168    S0    0x56158400   0x0000C910
 * PXA168    A0    0x56158400   0x00A0A168
 * PXA910    Y1    0x56158400   0x00F2C920
 * PXA910    A0    0x56158400   0x00F2C910
 * PXA910    A1    0x56158400   0x00A0C910
 * PXA920    Y0    0x56158400   0x00F2C920
 * PXA920    A0    0x56158400   0x00A0C920
 * PXA920    A1    0x56158400   0x00A1C920
 * MMP2	     Z0	   0x560f5811   0x00F00410
 * MMP2      Z1    0x560f5811   0x00E00410
 * MMP2      A0    0x560f5811   0x00A0A610
 * MMP3      A0    0x562f5842   0x00A02128
 * MMP3      B0    0x562f5842   0x00B02128
 */

extern unsigned int mmp_chip_id;

#define cpu_is_mmp2()	(0)

#define cpu_is_mmp3()		(0)
#define cpu_is_mmp3_a0()	(0)
#define cpu_is_mmp3_b0()	(0)

#endif /* __ASM_MACH_CPUTYPE_H */
