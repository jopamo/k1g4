/* SPDX-License-Identifier: GPL-2.0 */
/*
 * linux/include/linux/sunrpc/debug.h
 *
 * Debugging support for sunrpc module
 *
 * Copyright (C) 1996, Olaf Kirch <okir@monad.swb.de>
 */
#ifndef _LINUX_SUNRPC_DEBUG_H_
#define _LINUX_SUNRPC_DEBUG_H_

#include <uapi/linux/sunrpc/debug.h>

/*
 * Debugging macros etc
 */
extern unsigned int		rpc_debug;
extern unsigned int		nfs_debug;
extern unsigned int		nfsd_debug;
extern unsigned int		nlm_debug;

#define dprintk(fmt, ...)						\
	dfprintk(FACILITY, fmt, ##__VA_ARGS__)
#define dprintk_rcu(fmt, ...)						\
	dfprintk_rcu(FACILITY, fmt, ##__VA_ARGS__)

#undef ifdebug
# define ifdebug(fac)		if (0)
# define dfprintk(fac, fmt, ...)	no_printk(fmt, ##__VA_ARGS__)
# define dfprintk_rcu(fac, fmt, ...)	no_printk(fmt, ##__VA_ARGS__)

/*
 * Sysctl interface for RPC debugging
 */

struct rpc_clnt;
struct rpc_xprt;

static inline void
sunrpc_debugfs_init(void)
{
	return;
}

static inline void
sunrpc_debugfs_exit(void)
{
	return;
}

static inline void
rpc_clnt_debugfs_register(struct rpc_clnt *clnt)
{
	return;
}

static inline void
rpc_clnt_debugfs_unregister(struct rpc_clnt *clnt)
{
	return;
}

static inline void
rpc_xprt_debugfs_register(struct rpc_xprt *xprt)
{
	return;
}

static inline void
rpc_xprt_debugfs_unregister(struct rpc_xprt *xprt)
{
	return;
}

#endif /* _LINUX_SUNRPC_DEBUG_H_ */
