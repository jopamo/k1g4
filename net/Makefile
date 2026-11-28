# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the linux networking.
#
# 2 Sep 2000, Christoph Hellwig <hch@infradead.org>
# Rewritten to use lists instead of if-statements.
#

obj-y				:= devres.o socket.o core/

# removed stale make refs from obj-$(CONFIG_COMPAT)

# LLC has to be linked before the files in net/802/
obj-$(CONFIG_LLC)		+= llc/
obj-y				+= ethernet/ 802/ sched/ netlink/ bpf/ ethtool/
obj-$(CONFIG_NETFILTER)		+= netfilter/
obj-$(CONFIG_INET)		+= ipv4/
# removed stale make refs from obj-$(CONFIG_TLS)
# removed stale make refs from obj-$(CONFIG_XFRM)
obj-$(CONFIG_UNIX)		+= unix/
# removed stale make refs from obj-$(CONFIG_INET_PSP)
obj-y				+= ipv6/
obj-$(CONFIG_PACKET)		+= packet/
# removed stale make refs from obj-$(CONFIG_NET_KEY)
obj-$(CONFIG_BRIDGE)		+= bridge/
obj-$(CONFIG_NET_DEVLINK)	+= devlink/
obj-y				+= dsa/
# removed stale make refs from obj-$(CONFIG_X25)
# removed stale make refs from obj-$(CONFIG_LAPB)
obj-$(CONFIG_CAN)		+= can/
obj-$(CONFIG_BT)		+= bluetooth/
# removed stale make refs from obj-$(CONFIG_SUNRPC)
# removed stale make refs from obj-$(CONFIG_AF_RXRPC)
obj-$(CONFIG_AF_KCM)		+= kcm/
obj-$(CONFIG_STREAM_PARSER)	+= strparser/
# removed stale make refs from obj-$(CONFIG_ATM)
# removed stale make refs from obj-$(CONFIG_L2TP)
# removed stale make refs from obj-$(CONFIG_PHONET)
ifneq ($(CONFIG_VLAN_8021Q),)
obj-y				+= 8021q/
endif
# removed stale make refs from obj-$(CONFIG_IP_SCTP)
# removed stale make refs from obj-$(CONFIG_RDS)
obj-$(CONFIG_WIRELESS)		+= wireless/
obj-$(CONFIG_MAC80211)		+= mac80211/
# removed stale make refs from obj-$(CONFIG_TIPC)
# removed stale make refs from obj-$(CONFIG_NETLABEL)
# removed stale make refs from obj-$(CONFIG_IUCV)
# removed stale make refs from obj-$(CONFIG_SMC)
obj-$(CONFIG_RFKILL)		+= rfkill/
obj-$(CONFIG_NET_9P)		+= 9p/
obj-$(CONFIG_DCB)		+= dcb/
obj-$(CONFIG_6LOWPAN)		+= 6lowpan/
obj-$(CONFIG_IEEE802154)	+= ieee802154/
obj-$(CONFIG_MAC802154)		+= mac802154/

obj-$(CONFIG_SYSCTL)		+= sysctl_net.o
obj-$(CONFIG_DNS_RESOLVER)	+= dns_resolver/
# removed stale make refs from obj-$(CONFIG_CEPH_LIB)
obj-$(CONFIG_BATMAN_ADV)	+= batman-adv/
obj-$(CONFIG_NFC)		+= nfc/
obj-$(CONFIG_PSAMPLE)		+= psample/
obj-$(CONFIG_NET_IFE)		+= ife/
# removed stale make refs from obj-$(CONFIG_OPENVSWITCH)
obj-$(CONFIG_VSOCKETS)	+= vmw_vsock/
# removed stale make refs from obj-$(CONFIG_MPLS)
# removed stale make refs from obj-$(CONFIG_NET_NSH)
obj-$(CONFIG_HSR)		+= hsr/
obj-$(CONFIG_NET_SWITCHDEV)	+= switchdev/
obj-$(CONFIG_NET_L3_MASTER_DEV)	+= l3mdev/
obj-$(CONFIG_QRTR)		+= qrtr/
obj-$(CONFIG_NET_NCSI)		+= ncsi/
obj-$(CONFIG_XDP_SOCKETS)	+= xdp/
# removed stale make refs from obj-$(CONFIG_MPTCP)
obj-$(CONFIG_MCTP)		+= mctp/
# removed stale make refs from obj-$(CONFIG_NET_HANDSHAKE)
# removed stale make refs from obj-$(CONFIG_NET_SHAPER)
