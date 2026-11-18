#undef TRACE_SYSTEM
#define TRACE_SYSTEM hyperv

#if !defined(_TRACE_HYPERV_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_HYPERV_H

#include <linux/tracepoint.h>


#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH asm/trace/
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE hyperv
#endif /* _TRACE_HYPERV_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
