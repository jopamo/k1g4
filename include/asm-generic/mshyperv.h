/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Linux-specific definitions for managing interactions with Microsoft's
 * Hyper-V hypervisor. The definitions in this file are architecture
 * independent. See arch/<arch>/include/asm/mshyperv.h for definitions
 * that are specific to architecture <arch>.
 *
 * Definitions that are derived from Hyper-V code or headers should not go in
 * this file, but should instead go in the relevant files in include/hyperv.
 *
 * Copyright (C) 2019, Microsoft, Inc.
 *
 * Author : Michael Kelley <mikelley@microsoft.com>
 */

#ifndef _ASM_GENERIC_MSHYPERV_H
#define _ASM_GENERIC_MSHYPERV_H

#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/bitops.h>
#include <acpi/acpi_numa.h>
#include <linux/cpumask.h>
#include <linux/nmi.h>
#include <asm/ptrace.h>
#include <hyperv/hvhdk.h>

#define VTPM_BASE_ADDRESS 0xfed40000

enum hv_partition_type {
	HV_PARTITION_TYPE_GUEST,
	HV_PARTITION_TYPE_ROOT,
	HV_PARTITION_TYPE_L1VH,
};

struct ms_hyperv_info {
	u32 features;
	u32 priv_high;
	u32 ext_features;
	u32 misc_features;
	u32 hints;
	u32 nested_features;
	u32 max_vp_index;
	u32 max_lp_index;
	u8 vtl;
	union {
		u32 isolation_config_a;
		struct {
			u32 paravisor_present : 1;
			u32 reserved_a1 : 31;
		};
	};
	union {
		u32 isolation_config_b;
		struct {
			u32 cvm_type : 4;
			u32 reserved_b1 : 1;
			u32 shared_gpa_boundary_active : 1;
			u32 shared_gpa_boundary_bits : 6;
			u32 reserved_b2 : 20;
		};
	};
	u64 shared_gpa_boundary;
	bool msi_ext_dest_id;
	bool confidential_vmbus_available;
};
extern struct ms_hyperv_info ms_hyperv;
extern bool hv_nested;
extern u64 hv_current_partition_id;
extern enum hv_partition_type hv_curr_partition_type;

extern void * __percpu *hyperv_pcpu_input_arg;
extern void * __percpu *hyperv_pcpu_output_arg;

int hv_pci_register_dev_id(int pci_domain_nr, u32 logical_dev_id_prefix);
void hv_pci_unregister_dev_id(int pci_domain_nr);
int hv_pci_lookup_dev_id(int pci_domain_nr, u32 *prefix);

u64 hv_do_hypercall(u64 control, void *inputaddr, void *outputaddr);
u64 hv_do_fast_hypercall8(u16 control, u64 input8);
u64 hv_do_fast_hypercall16(u16 control, u64 input1, u64 input2);

bool hv_isolation_type_snp(void);
bool hv_isolation_type_tdx(void);

/*
 * On architectures where Hyper-V doesn't support AEOI (e.g., ARM64),
 * it doesn't provide a recommendation flag and AEOI must be disabled.
 */
static inline bool hv_recommend_using_aeoi(void)
{
#ifdef HV_DEPRECATING_AEOI_RECOMMENDED
	return !(ms_hyperv.hints & HV_DEPRECATING_AEOI_RECOMMENDED);
#else
	return false;
#endif
}

static inline struct hv_proximity_domain_info hv_numa_node_to_pxm_info(int node)
{
	struct hv_proximity_domain_info pxm_info = {};

	if (node != NUMA_NO_NODE) {
		pxm_info.domain_id = node_to_pxm(node);
		pxm_info.flags.proximity_info_valid = 1;
		pxm_info.flags.proximity_preferred = 1;
	}

	return pxm_info;
}

/* Helper functions that provide a consistent pattern for checking Hyper-V hypercall status. */
static inline int hv_result(u64 status)
{
	return status & HV_HYPERCALL_RESULT_MASK;
}

static inline bool hv_result_success(u64 status)
{
	return hv_result(status) == HV_STATUS_SUCCESS;
}

static inline unsigned int hv_repcomp(u64 status)
{
	/* Bits [43:32] of status have 'Reps completed' data. */
	return (status & HV_HYPERCALL_REP_COMP_MASK) >>
			 HV_HYPERCALL_REP_COMP_OFFSET;
}

/*
 * Rep hypercalls. Callers of this functions are supposed to ensure that
 * rep_count, varhead_size, and rep_start comply with Hyper-V hypercall
 * definition.
 */
static inline u64 hv_do_rep_hypercall_ex(u16 code, u16 rep_count,
					 u16 varhead_size, u16 rep_start,
					 void *input, void *output)
{
	u64 control = code;
	u64 status;
	u16 rep_comp;

	control |= (u64)varhead_size << HV_HYPERCALL_VARHEAD_OFFSET;
	control |= (u64)rep_count << HV_HYPERCALL_REP_COMP_OFFSET;
	control |= (u64)rep_start << HV_HYPERCALL_REP_START_OFFSET;

	do {
		status = hv_do_hypercall(control, input, output);
		if (!hv_result_success(status))
			return status;

		rep_comp = hv_repcomp(status);

		control &= ~HV_HYPERCALL_REP_START_MASK;
		control |= (u64)rep_comp << HV_HYPERCALL_REP_START_OFFSET;

		touch_nmi_watchdog();
	} while (rep_comp < rep_count);

	return status;
}

/* For the typical case where rep_start is 0 */
static inline u64 hv_do_rep_hypercall(u16 code, u16 rep_count, u16 varhead_size,
				      void *input, void *output)
{
	return hv_do_rep_hypercall_ex(code, rep_count, varhead_size, 0,
				      input, output);
}

/* Generate the guest OS identifier as described in the Hyper-V TLFS */
static inline u64 hv_generate_guest_id(u64 kernel_version)
{
	u64 guest_id;

	guest_id = (((u64)HV_LINUX_VENDOR_ID) << 48);
	guest_id |= (kernel_version << 16);

	return guest_id;
}

int hv_get_hypervisor_version(union hv_hypervisor_version_info *info);

void hv_setup_vmbus_handler(void (*handler)(void));
void hv_remove_vmbus_handler(void);
void hv_setup_stimer0_handler(void (*handler)(void));
void hv_remove_stimer0_handler(void);

void hv_setup_crash_handler(void (*handler)(struct pt_regs *regs));
void hv_remove_crash_handler(void);
void hv_setup_mshv_handler(void (*handler)(void));

static inline void hv_identify_partition_type(void) {}
static inline bool hv_is_hyperv_initialized(void) { return false; }
static inline bool hv_is_hibernation_supported(void) { return false; }
static inline void hyperv_cleanup(void) {}
static inline void ms_hyperv_late_init(void) {}
static inline bool hv_is_isolation_supported(void) { return false; }
static inline enum hv_isolation_type hv_get_isolation_type(void)
{
	return HV_ISOLATION_TYPE_NONE;
}

static inline bool hv_root_partition(void) { return false; }
static inline bool hv_l1vh_partition(void) { return false; }
static inline bool hv_parent_partition(void) { return false; }
static inline bool hv_result_needs_memory(u64 status) { return false; }
static inline int hv_deposit_memory_node(int node, u64 partition_id, u64 status)
{
	return -EOPNOTSUPP;
}
static inline int hv_call_add_logical_proc(int node, u32 lp_index, u32 acpi_id)
{
	return -EOPNOTSUPP;
}
static inline int hv_call_notify_all_processors_started(void)
{
	return -EOPNOTSUPP;
}
static inline bool hv_lp_exists(u32 lp_index)
{
	return false;
}
static inline int hv_call_create_vp(int node, u64 partition_id, u32 vp_index, u32 flags)
{
	return -EOPNOTSUPP;
}

static inline int hv_deposit_memory(u64 partition_id, u64 status)
{
	return hv_deposit_memory_node(NUMA_NO_NODE, partition_id, status);
}

static inline u8 get_vtl(void) { return 0; }

#endif
