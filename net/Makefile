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
obj-$(CONFIG_INET_PSP)		+= psp/
obj-y				+= ipv6/
obj-$(CONFIG_PACKET)		+= packet/
# removed stale make refs from obj-$(CONFIG_NET_KEY)
obj-$(CONFIG_BRIDGE)		+= bridge/
obj-$(CONFIG_NET_DEVLINK)	+= devlink/
obj-y				+= dsa/
obj-$(CONFIG_X25)		+= x25/
obj-$(CONFIG_LAPB)		+= lapb/
obj-$(CONFIG_CAN)		+= can/
obj-$(CONFIG_BT)		+= bluetooth/
# removed stale make refs from obj-$(CONFIG_SUNRPC)
# removed stale make refs from obj-$(CONFIG_AF_RXRPC)
obj-$(CONFIG_AF_KCM)		+= kcm/
obj-$(CONFIG_STREAM_PARSER)	+= strparser/
obj-$(CONFIG_ATM)		+= atm/
obj-$(CONFIG_L2TP)		+= l2tp/
obj-$(CONFIG_PHONET)		+= phonet/
ifneq ($(CONFIG_VLAN_8021Q),)
obj-y				+= 8021q/
endif
# removed stale make refs from obj-$(CONFIG_IP_SCTP)
# removed stale make refs from obj-$(CONFIG_RDS)
obj-$(CONFIG_WIRELESS)		+= wireless/
obj-$(CONFIG_MAC80211)		+= mac80211/
# removed stale make refs from obj-$(CONFIG_TIPC)
obj-$(CONFIG_NETLABEL)		+= netlabel/
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
obj-$(CONFIG_CEPH_LIB)		+= ceph/
obj-$(CONFIG_BATMAN_ADV)	+= batman-adv/
obj-$(CONFIG_NFC)		+= nfc/
obj-$(CONFIG_PSAMPLE)		+= psample/
obj-$(CONFIG_NET_IFE)		+= ife/
obj-$(CONFIG_OPENVSWITCH)	+= openvswitch/
obj-$(CONFIG_VSOCKETS)	+= vmw_vsock/
obj-$(CONFIG_MPLS)		+= mpls/
obj-$(CONFIG_NET_NSH)		+= nsh/
obj-$(CONFIG_HSR)		+= hsr/
obj-$(CONFIG_NET_SWITCHDEV)	+= switchdev/
obj-$(CONFIG_NET_L3_MASTER_DEV)	+= l3mdev/
obj-$(CONFIG_QRTR)		+= qrtr/
obj-$(CONFIG_NET_NCSI)		+= ncsi/
obj-$(CONFIG_XDP_SOCKETS)	+= xdp/
obj-$(CONFIG_MPTCP)		+= mptcp/
obj-$(CONFIG_MCTP)		+= mctp/
# removed stale make refs from obj-$(CONFIG_NET_HANDSHAKE)
# removed stale make refs from obj-$(CONFIG_NET_SHAPER)
