/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_MSHYPER_H
#define _ASM_X86_MSHYPER_H

#include <linux/types.h>
#include <linux/nmi.h>
#include <linux/msi.h>
#include <linux/io.h>
#include <linux/static_call.h>
#include <asm/nospec-branch.h>
#include <asm/msr.h>
#include <hyperv/hvhdk.h>
#include <asm/fpu/types.h>

/*
 * Hyper-V always provides a single IO-APIC at this MMIO address.
 * Ideally, the value should be looked up in ACPI tables, but it
 * is needed for mapping the IO-APIC early in boot on Confidential
 * VMs, before ACPI functions can be used.
 */
#define HV_IOAPIC_BASE_ADDRESS 0xfec00000

#define HV_VTL_NORMAL 0x0
#define HV_VTL_SECURE 0x1
#define HV_VTL_MGMT   0x2

union hv_ghcb;

DECLARE_STATIC_KEY_FALSE(isolation_type_snp);
DECLARE_STATIC_KEY_FALSE(isolation_type_tdx);

typedef int (*hyperv_fill_flush_list_func)(
		struct hv_guest_mapping_flush_list *flush,
		void *data);

void hyperv_vector_handler(struct pt_regs *regs);

static inline unsigned char hv_get_nmi_reason(void)
{
	return 0;
}

extern u64 hv_tdx_hypercall(u64 control, u64 param1, u64 param2);
extern u64 hv_snp_hypercall(u64 control, u64 param1, u64 param2);
extern u64 hv_std_hypercall(u64 control, u64 param1, u64 param2);

static inline void hyperv_init(void) {}
static inline void hyperv_setup_mmu_ops(void) {}
static inline void set_hv_tscchange_cb(void (*cb)(void)) {}
static inline void clear_hv_tscchange_cb(void) {}
static inline void hyperv_stop_tsc_emulation(void) {};
static inline struct hv_vp_assist_page *hv_get_vp_assist_page(unsigned int cpu)
{
	return NULL;
}
static inline int hyperv_flush_guest_mapping(u64 as) { return -1; }
static inline int hyperv_flush_guest_mapping_range(u64 as,
		hyperv_fill_flush_list_func fill_func, void *data)
{
	return -1;
}
static inline void hv_set_msr(unsigned int reg, u64 value) { }
static inline u64 hv_get_msr(unsigned int reg) { return 0; }
static inline void hv_set_non_nested_msr(unsigned int reg, u64 value) { }
static inline u64 hv_get_non_nested_msr(unsigned int reg) { return 0; }
static inline int hv_apicid_to_vp_index(u32 apic_id) { return -EINVAL; }

struct mshv_vtl_cpu_context {
	union {
		struct {
			u64 rax;
			u64 rcx;
			u64 rdx;
			u64 rbx;
			u64 cr2;
			u64 rbp;
			u64 rsi;
			u64 rdi;
			u64 r8;
			u64 r9;
			u64 r10;
			u64 r11;
			u64 r12;
			u64 r13;
			u64 r14;
			u64 r15;
		};
		u64 gp_regs[16];
	};

	struct fxregs_state fx_state;
};

static inline void __init hv_vtl_init_platform(void) {}
static inline int __init hv_vtl_early_init(void) { return 0; }
static inline void mshv_vtl_return_call(struct mshv_vtl_cpu_context *vtl0) {}
static inline void mshv_vtl_return_call_init(u64 vtl_return_offset) {}
static inline void mshv_vtl_return_hypercall(void) {}
static inline void __mshv_vtl_return_call(struct mshv_vtl_cpu_context *vtl0) {}


#include <asm-generic/mshyperv.h>

#endif
