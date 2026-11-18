/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ARCH_X86_KVM_VMX_ONHYPERV_H__
#define __ARCH_X86_KVM_VMX_ONHYPERV_H__

#include <hyperv/hvhdk.h>
#include <asm/mshyperv.h>

#include <linux/jump_label.h>

#include "capabilities.h"
#include "hyperv_evmcs.h"
#include "vmcs12.h"

#define current_evmcs ((struct hv_enlightened_vmcs *)this_cpu_read(current_vmcs))

static __always_inline bool kvm_is_using_evmcs(void) { return false; }
static __always_inline void evmcs_write64(unsigned long field, u64 value) {}
static __always_inline void evmcs_write32(unsigned long field, u32 value) {}
static __always_inline void evmcs_write16(unsigned long field, u16 value) {}
static __always_inline u64 evmcs_read64(unsigned long field) { return 0; }
static __always_inline u32 evmcs_read32(unsigned long field) { return 0; }
static __always_inline u16 evmcs_read16(unsigned long field) { return 0; }
static inline void evmcs_load(u64 phys_addr) {}

#endif /* __ARCH_X86_KVM_VMX_ONHYPERV_H__ */
