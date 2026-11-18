/* SPDX-License-Identifier: GPL-2.0-only */
/******************************************************************************

(c) 2008 NetApp.  All Rights Reserved.


******************************************************************************/

/*
 * Functions to create and manage the backchannel
 */

#ifndef _LINUX_SUNRPC_BC_XPRT_H
#define _LINUX_SUNRPC_BC_XPRT_H

#include <linux/sunrpc/svcsock.h>
#include <linux/sunrpc/xprt.h>
#include <linux/sunrpc/sched.h>

static inline int xprt_setup_backchannel(struct rpc_xprt *xprt,
					 unsigned int min_reqs)
{
	return 0;
}

static inline void xprt_destroy_backchannel(struct rpc_xprt *xprt,
					    unsigned int max_reqs)
{
}

static inline bool svc_is_backchannel(const struct svc_rqst *rqstp)
{
	return false;
}

static inline void set_bc_enabled(struct svc_serv *serv)
{
}

static inline void xprt_free_bc_request(struct rpc_rqst *req)
{
}

static inline void xprt_svc_shutdown_bc(struct rpc_xprt *xprt)
{
}

static inline void xprt_svc_destroy_nullify_bc(struct rpc_xprt *xprt, struct svc_serv **serv)
{
	svc_destroy(serv);
}
#endif /* _LINUX_SUNRPC_BC_XPRT_H */
