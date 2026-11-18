/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_IA32_H
#define _ASM_X86_IA32_H


static __always_inline bool ia32_enabled(void)
{
	return IS_ENABLED(CONFIG_X86_32);
}

static inline void ia32_disable(void) {}


static inline bool ia32_enabled_verbose(void)
{
	bool enabled = ia32_enabled();

	if (IS_ENABLED(CONFIG_IA32_EMULATION) && !enabled)
		pr_notice_once("32-bit emulation disabled. You can reenable with ia32_emulation=on\n");

	return enabled;
}

#endif /* _ASM_X86_IA32_H */
