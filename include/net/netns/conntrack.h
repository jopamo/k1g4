/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __NETNS_CONNTRACK_H
#define __NETNS_CONNTRACK_H

#include <linux/list.h>
#include <linux/list_nulls.h>
#include <linux/atomic.h>
#include <linux/workqueue.h>
#include <linux/netfilter/nf_conntrack_tcp.h>
#include <linux/seqlock.h>

struct ctl_table_header;
struct nf_conntrack_ecache;

struct nf_generic_net {
	unsigned int timeout;
};

struct nf_tcp_net {
	unsigned int timeouts[TCP_CONNTRACK_TIMEOUT_MAX];
	u8 tcp_loose;
	u8 tcp_be_liberal;
	u8 tcp_max_retrans;
	u8 tcp_ignore_invalid_rst;
#if IS_ENABLED(CONFIG_NF_FLOW_TABLE)
	unsigned int offload_timeout;
#endif
};

enum udp_conntrack {
	UDP_CT_UNREPLIED,
	UDP_CT_REPLIED,
	UDP_CT_MAX
};

struct nf_udp_net {
	unsigned int timeouts[UDP_CT_MAX];
#if IS_ENABLED(CONFIG_NF_FLOW_TABLE)
	unsigned int offload_timeout;
#endif
};

struct nf_icmp_net {
	unsigned int timeout;
};



struct nf_ip_net {
	struct nf_generic_net   generic;
	struct nf_tcp_net	tcp;
	struct nf_udp_net	udp;
	struct nf_icmp_net	icmp;
	struct nf_icmp_net	icmpv6;
};

struct netns_ct {
#ifdef CONFIG_NF_CONNTRACK_EVENTS
	bool ecache_dwork_pending;
#endif
	u8			sysctl_log_invalid; /* Log invalid packets */
	u8			sysctl_events;
	u8			sysctl_acct;
	u8			sysctl_tstamp;
	u8			sysctl_checksum;

	struct ip_conntrack_stat __percpu *stat;
	struct nf_ct_event_notifier __rcu *nf_conntrack_event_cb;
	struct nf_ip_net	nf_ct_proto;
#if defined(CONFIG_NF_CONNTRACK_LABELS)
	atomic_t		labels_used;
#endif
};
#endif
