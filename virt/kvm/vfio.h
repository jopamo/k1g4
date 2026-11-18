/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __KVM_VFIO_H
#define __KVM_VFIO_H

static inline int kvm_vfio_ops_init(void)
{
	return 0;
}
static inline void kvm_vfio_ops_exit(void)
{
}

#endif
