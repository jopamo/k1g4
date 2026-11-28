/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _XEN_XEN_H
#define _XEN_XEN_H

#include <linux/types.h>

enum xen_domain_type {
	XEN_NATIVE,		/* running on bare hardware    */
	XEN_PV_DOMAIN,		/* running in a PV domain      */
	XEN_HVM_DOMAIN,		/* running in a Xen hvm domain */
};

#define xen_domain_type		XEN_NATIVE

#define xen_pvh			0

#ifdef CONFIG_X86
#include <asm/cpufeature.h>

#define xen_pv_domain()		(cpu_feature_enabled(X86_FEATURE_XENPV))
#else
#define xen_pv_domain()		0
#endif

#define xen_domain()		(xen_domain_type != XEN_NATIVE)
#define xen_hvm_domain()	(xen_domain_type == XEN_HVM_DOMAIN)
#define xen_pvh_domain()	(xen_pvh)

extern uint32_t xen_start_flags;

#define xen_pv_pci_possible	0

#include <xen/interface/hvm/start_info.h>
extern struct hvm_start_info pvh_start_info;
void xen_prepare_pvh(void);
struct pt_regs;
void xen_pv_evtchn_do_upcall(struct pt_regs *regs);

#define xen_initial_domain()	(0)

struct bio_vec;
struct page;

bool xen_biovec_phys_mergeable(const struct bio_vec *vec1,
		const struct page *page);


#define xen_unpopulated_pages 0UL
#include <xen/balloon.h>
static inline int xen_alloc_unpopulated_pages(unsigned int nr_pages,
		struct page **pages)
{
	return xen_alloc_ballooned_pages(nr_pages, pages);
}
static inline void xen_free_unpopulated_pages(unsigned int nr_pages,
		struct page **pages)
{
	xen_free_ballooned_pages(nr_pages, pages);
}

#include <linux/bug.h>
static inline bool xen_processor_present(uint32_t acpi_id)
{
	BUG();
	return false;
}

#endif	/* _XEN_XEN_H */
