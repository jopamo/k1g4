# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_VSOCKETS) += vsock.o
obj-$(CONFIG_VSOCKETS_DIAG) += vsock_diag.o
# removed stale make refs from obj-$(CONFIG_VMWARE_VMCI_VSOCKETS)
obj-$(CONFIG_VIRTIO_VSOCKETS) += vmw_vsock_virtio_transport.o
obj-$(CONFIG_VIRTIO_VSOCKETS_COMMON) += vmw_vsock_virtio_transport_common.o
# removed stale make refs from obj-$(CONFIG_HYPERV_VSOCKETS)
obj-$(CONFIG_VSOCKETS_LOOPBACK) += vsock_loopback.o

vsock-y += af_vsock.o af_vsock_tap.o vsock_addr.o
vsock-$(CONFIG_BPF_SYSCALL) += vsock_bpf.o

vsock_diag-y += diag.o

# removed stale make refs from vmw_vsock_vmci_transport-y

vmw_vsock_virtio_transport-y += virtio_transport.o

vmw_vsock_virtio_transport_common-y += virtio_transport_common.o

# removed stale make refs from hv_sock-y
