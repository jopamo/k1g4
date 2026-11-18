/* SPDX-License-Identifier: GPL-2.0 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM thp

#if !defined(_TRACE_THP_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_THP_H

#include <linux/types.h>
#include <linux/tracepoint.h>


DECLARE_EVENT_CLASS(migration_pmd,

		TP_PROTO(unsigned long addr, unsigned long pmd),

		TP_ARGS(addr, pmd),

		TP_STRUCT__entry(
			__field(unsigned long, addr)
			__field(unsigned long, pmd)
		),

		TP_fast_assign(
			__entry->addr = addr;
			__entry->pmd = pmd;
		),
		TP_printk("addr=%lx, pmd=%lx", __entry->addr, __entry->pmd)
);

DEFINE_EVENT(migration_pmd, set_migration_pmd,
	TP_PROTO(unsigned long addr, unsigned long pmd),
	TP_ARGS(addr, pmd)
);

DEFINE_EVENT(migration_pmd, remove_migration_pmd,
	TP_PROTO(unsigned long addr, unsigned long pmd),
	TP_ARGS(addr, pmd)
);
#endif /* _TRACE_THP_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
