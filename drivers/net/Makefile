# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the Linux network device drivers.
#

#
# Networking Core Drivers
#
obj-$(CONFIG_BONDING) += bonding/
obj-$(CONFIG_IPVLAN) += ipvlan/
obj-$(CONFIG_IPVTAP) += ipvlan/
obj-$(CONFIG_DUMMY) += dummy.o
obj-$(CONFIG_WIREGUARD) += wireguard/
# removed stale make refs from obj-$(CONFIG_OVPN)
# removed stale make refs from obj-$(CONFIG_EQUALIZER)
obj-$(CONFIG_IFB) += ifb.o
obj-$(CONFIG_MACSEC) += macsec.o
# removed stale make refs from obj-$(CONFIG_AMT)
obj-$(CONFIG_MACVLAN) += macvlan.o
obj-$(CONFIG_MACVTAP) += macvtap.o
obj-$(CONFIG_MII) += mii.o
obj-$(CONFIG_MDIO) += mdio.o
obj-$(CONFIG_NET) += loopback.o
obj-$(CONFIG_NETCONSOLE) += netconsole.o
obj-$(CONFIG_NETKIT) += netkit.o
obj-y += phy/
obj-y += pse-pd/
obj-y += mdio/
obj-y += pcs/
obj-$(CONFIG_RIONET) += rionet.o
# removed stale make refs from obj-$(CONFIG_NET_TEAM)
obj-$(CONFIG_TUN) += tun.o
obj-$(CONFIG_TAP) += tap.o
obj-$(CONFIG_VETH) += veth.o
obj-$(CONFIG_VIRTIO_NET) += virtio_net.o
# removed stale make refs from obj-$(CONFIG_VXLAN)
# removed stale make refs from obj-$(CONFIG_GENEVE)
# removed stale make refs from obj-$(CONFIG_BAREUDP)
# removed stale make refs from obj-$(CONFIG_GTP)
obj-$(CONFIG_NLMON) += nlmon.o
# removed stale make refs from obj-$(CONFIG_PFCP)
obj-$(CONFIG_NET_VRF) += vrf.o
obj-$(CONFIG_VSOCKMON) += vsockmon.o
obj-$(CONFIG_MHI_NET) += mhi_net.o

#
# Networking Drivers
#
obj-$(CONFIG_ARCNET) += arcnet/
obj-$(CONFIG_CAN) += can/
ifdef CONFIG_NET_DSA
obj-y += dsa/
endif
obj-$(CONFIG_ETHERNET) += ethernet/
obj-$(CONFIG_FDDI) += fddi/
# removed stale make refs from obj-$(CONFIG_QCOM_IPA)
# removed stale make refs from obj-$(CONFIG_PLIP)
# removed stale make refs from obj-$(CONFIG_PPP)
# removed stale make refs from obj-$(CONFIG_PPP_ASYNC)
# removed stale make refs from obj-$(CONFIG_PPP_BSDCOMP)
# removed stale make refs from obj-$(CONFIG_PPP_DEFLATE)
# removed stale make refs from obj-$(CONFIG_PPP_MPPE)
# removed stale make refs from obj-$(CONFIG_PPP_SYNC_TTY)
# removed stale make refs from obj-$(CONFIG_PPPOE)
# removed stale make refs from obj-$(CONFIG_PPPOL2TP)
# removed stale make refs from obj-$(CONFIG_PPTP)
# removed stale make refs from obj-$(CONFIG_SLIP)
# removed stale make refs from obj-$(CONFIG_SLHC)
obj-$(CONFIG_SUNGEM_PHY) += sungem_phy.o
# removed stale make refs from obj-$(CONFIG_WAN)
obj-$(CONFIG_WLAN) += wireless/
obj-$(CONFIG_IEEE802154) += ieee802154/
obj-$(CONFIG_WWAN) += wwan/
obj-$(CONFIG_MCTP) += mctp/

# removed stale make refs from obj-$(CONFIG_VMXNET3)
# removed stale make refs from obj-$(CONFIG_XEN_NETDEV_FRONTEND)
# removed stale make refs from obj-$(CONFIG_XEN_NETDEV_BACKEND)

obj-$(CONFIG_USB_NET_DRIVERS) += usb/

# removed stale make refs from obj-$(CONFIG_HYPERV_NET)
obj-$(CONFIG_NTB_NETDEV) += ntb_netdev.o

obj-$(CONFIG_FUJITSU_ES) += fjes/
obj-$(CONFIG_USB4_NET) += thunderbolt/
obj-$(CONFIG_NETDEVSIM) += netdevsim/
obj-$(CONFIG_NET_FAILOVER) += net_failover.o
