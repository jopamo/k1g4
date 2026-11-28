// SPDX-License-Identifier: GPL-2.0

#include "lkdtm.h"
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <asm/mmu.h>


static __always_inline void tlbiel_va(unsigned long va,
				      unsigned long pid,
				      unsigned long ap,
				      unsigned long ric)
{
	unsigned long rb, rs, prs, r;

	rb = va & ~(PPC_BITMASK(52, 63));
	rb |= ap << PPC_BITLSHIFT(58);
	rs = pid << PPC_BITLSHIFT(31);

	prs = 1; /* process scoped */
	r = 1;   /* radix format */

	/*
	 * Trigger an MCE by issuing radix tlbiel with an invalid operand combination.
	 * The combination of RIC = 2 with IS = 0 (Invalidation selector specified
	 * in the RB register) is invalid.
	 * This invalid combination causes hardware to raise a machine check.
	 */
	asm volatile(PPC_TLBIEL(%0, %4, %3, %2, %1)
			: : "r"(rb), "i"(r), "i"(prs), "i"(ric), "r"(rs) : "memory");
}

static void lkdtm_PPC_SLB_MULTIHIT(void)
{
	pr_err("XFAIL: This test requires CONFIG_PPC_64S_HASH_MMU\n");
}

static void lkdtm_PPC_RADIX_TLBIEL(void)
{
	unsigned long addr = PAGE_OFFSET;

	if (radix_enabled()) {
		pr_info("Injecting Radix TLB invalidation MCE\n");
		tlbiel_va(addr, 0, 0, RIC_FLUSH_ALL);
		pr_info("Recovered from radix tlbiel attempt\n");
	} else {
		pr_err("XFAIL: This test is for ppc64 and with radix mode MMU only\n");
	}
}

static struct crashtype crashtypes[] = {
	CRASHTYPE(PPC_SLB_MULTIHIT),
	CRASHTYPE(PPC_RADIX_TLBIEL),
};

struct crashtype_category powerpc_crashtypes = {
	.crashtypes = crashtypes,
	.len	    = ARRAY_SIZE(crashtypes),
};
