// SPDX-License-Identifier: GPL-2.0-only
/*
 *	stacktrace.c : stacktracing APIs needed by rest of kernel
 *			(wrappers over ARC dwarf based unwinder)
 *
 * Copyright (C) 2004, 2007-2010, 2011-2012 Synopsys, Inc. (www.synopsys.com)
 *
 *  vineetg: aug 2009
 *  -Implemented CONFIG_STACKTRACE APIs, primarily save_stack_trace_tsk( )
 *   for displaying task's kernel mode call stack in /proc/<pid>/stack
 *  -Iterator based approach to have single copy of unwinding core and APIs
 *   needing unwinding, implement the logic in iterator regarding:
 *      = which frame onwards to start capture
 *      = which frame to stop capturing (wchan)
 *      = specifics of data structs where trace is saved(CONFIG_STACKTRACE etc)
 *
 *  vineetg: March 2009
 *  -Implemented correct versions of thread_saved_pc() and __get_wchan()
 *
 *  rajeshwarr: 2008
 *  -Initial implementation
 */

#include <linux/ptrace.h>
#include <linux/export.h>
#include <linux/stacktrace.h>
#include <linux/kallsyms.h>
#include <linux/sched/debug.h>

#include <asm/arcregs.h>
#include <asm/unwind.h>
#include <asm/stacktrace.h>
#include <asm/switch_to.h>

/*-------------------------------------------------------------------------
 *              Unwinder Iterator
 *-------------------------------------------------------------------------
 */


notrace noinline unsigned int
arc_unwind_core(struct task_struct *tsk, struct pt_regs *regs,
		int (*consumer_fn) (unsigned int, void *), void *arg)
{
	/* On ARC, only Dward based unwinder works. fp based backtracing is
	 * not possible (-fno-omit-frame-pointer) because of the way function
	 * prologue is setup (callee regs saved and then fp set and not other
	 * way around
	 */
	pr_warn_once("CONFIG_ARC_DW2_UNWIND needs to be enabled\n");
	return 0;

}

/*-------------------------------------------------------------------------
 * callbacks called by unwinder iterator to implement kernel APIs
 *
 * The callback can return -1 to force the iterator to stop, which by default
 * keeps going till the bottom-most frame.
 *-------------------------------------------------------------------------
 */

/* Call-back which plugs into unwinding core to dump the stack in
 * case of panic/OOPs/BUG etc
 */
static int __print_sym(unsigned int address, void *arg)
{
	const char *loglvl = arg;

	printk("%s  %pS\n", loglvl, (void *)address);
	return 0;
}

#ifdef CONFIG_STACKTRACE

/* Call-back which plugs into unwinding core to capture the
 * traces needed by kernel on /proc/<pid>/stack
 */
static int __collect_all(unsigned int address, void *arg)
{
	struct stack_trace *trace = arg;

	if (trace->skip > 0)
		trace->skip--;
	else
		trace->entries[trace->nr_entries++] = address;

	if (trace->nr_entries >= trace->max_entries)
		return -1;

	return 0;
}

static int __collect_all_but_sched(unsigned int address, void *arg)
{
	struct stack_trace *trace = arg;

	if (in_sched_functions(address))
		return 0;

	if (trace->skip > 0)
		trace->skip--;
	else
		trace->entries[trace->nr_entries++] = address;

	if (trace->nr_entries >= trace->max_entries)
		return -1;

	return 0;
}

#endif

static int __get_first_nonsched(unsigned int address, void *unused)
{
	if (in_sched_functions(address))
		return 0;

	return -1;
}

/*-------------------------------------------------------------------------
 *              APIs expected by various kernel sub-systems
 *-------------------------------------------------------------------------
 */

noinline void show_stacktrace(struct task_struct *tsk, struct pt_regs *regs,
			      const char *loglvl)
{
	printk("%s\nStack Trace:\n", loglvl);
	arc_unwind_core(tsk, regs, __print_sym, (void *)loglvl);
}
EXPORT_SYMBOL(show_stacktrace);

/* Expected by sched Code */
void show_stack(struct task_struct *tsk, unsigned long *sp, const char *loglvl)
{
	show_stacktrace(tsk, NULL, loglvl);
}

/* Another API expected by schedular, shows up in "ps" as Wait Channel
 * Of course just returning schedule( ) would be pointless so unwind until
 * the function is not in schedular code
 */
unsigned int __get_wchan(struct task_struct *tsk)
{
	return arc_unwind_core(tsk, NULL, __get_first_nonsched, NULL);
}

#ifdef CONFIG_STACKTRACE

/*
 * API required by CONFIG_STACKTRACE, CONFIG_LATENCYTOP.
 * A typical use is when /proc/<pid>/stack is queried by userland
 */
void save_stack_trace_tsk(struct task_struct *tsk, struct stack_trace *trace)
{
	/* Assumes @tsk is sleeping so unwinds from __switch_to */
	arc_unwind_core(tsk, NULL, __collect_all_but_sched, trace);
}

void save_stack_trace(struct stack_trace *trace)
{
	/* Pass NULL for task so it unwinds the current call frame */
	arc_unwind_core(NULL, NULL, __collect_all, trace);
}
EXPORT_SYMBOL_GPL(save_stack_trace);
#endif
