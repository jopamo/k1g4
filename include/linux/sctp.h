/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SCTP kernel reference Implementation
 * (C) Copyright IBM Corp. 2001, 2004
 * Copyright (c) 1999-2000 Cisco, Inc.
 * Copyright (c) 1999-2001 Motorola, Inc.
 * Copyright (c) 2001 Intel Corp.
 * Copyright (c) 2001 Nokia, Inc.
 * Copyright (c) 2001 La Monte H.P. Yarroll
 *
 * This file is part of the SCTP kernel reference Implementation
 *
 * Various protocol defined structures.
 *
 * Please send any bug reports or fixes you make to the
 * email address(es):
 *    lksctp developers <linux-sctp@vger.kernel.org>
 *
 * Or submit a bug report through the following website:
 *    http://www.sf.net/projects/lksctp
 *
 * Written or modified by:
 *    La Monte H.P. Yarroll <piggy@acm.org>
 *    Karl Knutson <karl@athena.chicago.il.us>
 *    Jon Grimm <jgrimm@us.ibm.com>
 *    Xingang Guo <xingang.guo@intel.com>
 *    randall@sctp.chicago.il.us
 *    kmorneau@cisco.com
 *    qxie1@email.mot.com
 *    Sridhar Samudrala <sri@us.ibm.com>
 *    Kevin Gao <kevin.gao@intel.com>
 *
 * Any bugs reported given to us we will try to fix... any fixes shared will
 * be incorporated into the next SCTP release.
 */
#ifndef __LINUX_SCTP_H__
#define __LINUX_SCTP_H__

#include <linux/types.h>
#include <linux/skbuff.h>

/* Section 3.1.  SCTP Common Header Format */
struct sctphdr {
	__be16 source;
	__be16 dest;
	__be32 vtag;
	__le32 checksum;
};

static inline struct sctphdr *sctp_hdr(const struct sk_buff *skb)
{
	return (struct sctphdr *)skb_transport_header(skb);
}

#endif /* __LINUX_SCTP_H__ */
