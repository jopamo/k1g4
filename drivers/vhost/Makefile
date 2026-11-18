# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_VHOST_NET) += vhost_net.o
vhost_net-y := net.o

obj-$(CONFIG_VHOST_SCSI) += vhost_scsi.o
vhost_scsi-y := scsi.o

obj-$(CONFIG_VHOST_VSOCK) += vhost_vsock.o
vhost_vsock-y := vsock.o

obj-$(CONFIG_VHOST_RING) += vringh.o

# removed stale make refs from obj-$(CONFIG_VHOST_VDPA)
# removed stale make refs from vhost_vdpa-y

obj-$(CONFIG_VHOST)	+= vhost.o

obj-$(CONFIG_VHOST_IOTLB) += vhost_iotlb.o
vhost_iotlb-y := iotlb.o
