/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Multipath TCP
 *
 * Copyright (c) 2017 - 2019, Intel Corporation.
 */

#ifndef __NET_MPTCP_H
#define __NET_MPTCP_H

#include <linux/skbuff.h>
#include <linux/tcp.h>
#include <linux/types.h>

struct mptcp_info;
struct mptcp_sock;
struct mptcp_pm_addr_entry;
struct seq_file;

/* MPTCP sk_buff extension data */
struct mptcp_ext {
	u64		data_seq;
	u32		subflow_seq;
	u16		data_len;
	__sum16		csum;

	struct_group(flags,
		u8	use_map:1,
			dsn64:1,
			data_fin:1,
			use_ack:1,
			ack64:1,
			mpc_map:1,
			frozen:1,
			reset_transient:1;
		u8	reset_reason:4,
			csum_reqd:1,
			infinite_map:1;
	); /* end of flags group */
};

#define MPTCPOPT_HMAC_LEN	20
#define MPTCP_RM_IDS_MAX	8

struct mptcp_rm_list {
	u8 ids[MPTCP_RM_IDS_MAX];
	u8 nr;
};

struct mptcp_addr_info {
	u8			id;
	sa_family_t		family;
	__be16			port;
	union {
		struct in_addr	addr;
	};
};

struct mptcp_out_options {
};

#define MPTCP_SCHED_NAME_MAX	16
#define MPTCP_SCHED_MAX		128
#define MPTCP_SCHED_BUF_MAX	(MPTCP_SCHED_NAME_MAX * MPTCP_SCHED_MAX)

struct mptcp_sched_ops {
	int (*get_send)(struct mptcp_sock *msk);
	int (*get_retrans)(struct mptcp_sock *msk);

	char			name[MPTCP_SCHED_NAME_MAX];
	struct module		*owner;
	struct list_head	list;

	void (*init)(struct mptcp_sock *msk);
	void (*release)(struct mptcp_sock *msk);
} ____cacheline_aligned_in_smp;

#define MPTCP_PM_NAME_MAX	16
#define MPTCP_PM_MAX		128
#define MPTCP_PM_BUF_MAX	(MPTCP_PM_NAME_MAX * MPTCP_PM_MAX)

struct mptcp_pm_ops {
	char			name[MPTCP_PM_NAME_MAX];
	struct module		*owner;
	struct list_head	list;

	void (*init)(struct mptcp_sock *msk);
	void (*release)(struct mptcp_sock *msk);
} ____cacheline_aligned_in_smp;


static inline void mptcp_init(void)
{
}

static inline bool sk_is_mptcp(const struct sock *sk)
{
	return false;
}

static inline bool rsk_is_mptcp(const struct request_sock *req)
{
	return false;
}

static inline bool rsk_drop_req(const struct request_sock *req)
{
	return false;
}

static inline bool mptcp_syn_options(struct sock *sk, const struct sk_buff *skb,
				     unsigned int *size,
				     struct mptcp_out_options *opts)
{
	return false;
}

static inline bool mptcp_synack_options(const struct request_sock *req,
					unsigned int *size,
					struct mptcp_out_options *opts)
{
	return false;
}

static inline bool mptcp_incoming_options(struct sock *sk,
					  struct sk_buff *skb)
{
	return true;
}

static inline void mptcp_skb_ext_move(struct sk_buff *to,
				      const struct sk_buff *from)
{
}

static inline void mptcp_skb_ext_copy(struct sk_buff *to,
				      struct sk_buff *from)
{
}

static inline bool mptcp_skb_can_collapse(const struct sk_buff *to,
					  const struct sk_buff *from)
{
	return true;
}

static inline void mptcp_space(const struct sock *ssk, int *s, int *fs) { }
static inline void mptcp_seq_show(struct seq_file *seq) { }

static inline int mptcp_subflow_init_cookie_req(struct request_sock *req,
						const struct sock *sk_listener,
						struct sk_buff *skb)
{
	return 0; /* TCP fallback */
}

static inline struct request_sock *mptcp_subflow_reqsk_alloc(const struct request_sock_ops *ops,
							     struct sock *sk_listener,
							     bool attach_listener)
{
	return NULL;
}

/* Caller must hold an RCU read-side lock. */
static inline void mptcp_subflow_reqsk_clone(struct request_sock *req,
					     struct request_sock *new_req)
{
}

static inline __be32 mptcp_reset_option(const struct sk_buff *skb)  { return htonl(0u); }

static inline void mptcp_active_detect_blackhole(struct sock *sk, bool expired) { }

#if 0
int mptcpv6_init(void);
void mptcpv6_handle_mapped(struct sock *sk, bool mapped);
#elif IS_ENABLED(CONFIG_IPV6)
static inline int mptcpv6_init(void) { return 0; }
static inline void mptcpv6_handle_mapped(struct sock *sk, bool mapped) { }
#endif

static inline struct mptcp_sock *bpf_mptcp_sock_from_subflow(struct sock *sk) { return NULL; }

struct mptcp_sock { };

#endif /* __NET_MPTCP_H */
