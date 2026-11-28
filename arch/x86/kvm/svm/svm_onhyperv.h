/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * KVM L1 hypervisor optimizations on Hyper-V for SVM.
 */

#ifndef __ARCH_X86_KVM_SVM_ONHYPERV_H__
#define __ARCH_X86_KVM_SVM_ONHYPERV_H__

#include <asm/mshyperv.h>


static inline bool svm_hv_is_enlightened_tlb_enabled(struct kvm_vcpu *vcpu)
{
	return false;
}

static inline void svm_hv_init_vmcb(struct vmcb *vmcb)
{
}

static inline __init void svm_hv_hardware_setup(void)
{
}

static inline void svm_hv_vmcb_dirty_nested_enlightenments(
		struct kvm_vcpu *vcpu)
{
}

static inline void svm_hv_update_vp_id(struct vmcb *vmcb,
		struct kvm_vcpu *vcpu)
{
}

#endif /* __ARCH_X86_KVM_SVM_ONHYPERV_H__ */
