// SPDX-License-Identifier: GPL-2.0

#include <linux/sched.h>
#include <linux/elf.h>
#include <linux/regset.h>
#include <asm/user32.h>
#include <asm/sigcontext.h>


static int genregs_get(struct task_struct *target,
		       const struct user_regset *regset,
		       struct membuf to)
{
	int reg;

	for (reg = 0; to.left; reg++)
		membuf_store(&to, getreg(target, reg * sizeof(unsigned long)));
	return 0;
}

static int genregs_set(struct task_struct *target,
		       const struct user_regset *regset,
		       unsigned int pos, unsigned int count,
		       const void *kbuf, const void __user *ubuf)
{
	int ret = 0;

	if (kbuf) {
		const unsigned long *k = kbuf;

		while (count >= sizeof(*k) && !ret) {
			ret = putreg(target, pos, *k++);
			count -= sizeof(*k);
			pos += sizeof(*k);
		}
	} else {
		const unsigned long  __user *u = ubuf;

		while (count >= sizeof(*u) && !ret) {
			unsigned long word;

			ret = __get_user(word, u++);
			if (ret)
				break;
			ret = putreg(target, pos, word);
			count -= sizeof(*u);
			pos += sizeof(*u);
		}
	}
	return ret;
}

static int generic_fpregs_active(struct task_struct *target, const struct user_regset *regset)
{
	return regset->n;
}

static int generic_fpregs_get(struct task_struct *target,
			      const struct user_regset *regset,
			      struct membuf to)
{
	void *fpregs = task_pt_regs(target)->regs.fp;

	membuf_write(&to, fpregs, regset->size * regset->n);
	return 0;
}

static int generic_fpregs_set(struct task_struct *target,
			      const struct user_regset *regset,
			      unsigned int pos, unsigned int count,
			      const void *kbuf, const void __user *ubuf)
{
	void *fpregs = task_pt_regs(target)->regs.fp;

	return user_regset_copyin(&pos, &count, &kbuf, &ubuf,
				  fpregs, 0, regset->size * regset->n);
}

static struct user_regset uml_regsets[] __ro_after_init = {
	[REGSET_GENERAL] = {
		USER_REGSET_NOTE_TYPE(PRSTATUS),
		.n		= sizeof(struct user_regs_struct) / sizeof(long),
		.size		= sizeof(long),
		.align		= sizeof(long),
		.regset_get	= genregs_get,
		.set		= genregs_set
	},
	[REGSET_FP] = {
		USER_REGSET_NOTE_TYPE(PRFPREG),
		.n		= sizeof(struct user_i387_struct) / sizeof(long),
		.size		= sizeof(long),
		.align		= sizeof(long),
		.active		= generic_fpregs_active,
		.regset_get	= generic_fpregs_get,
		.set		= generic_fpregs_set,
	},
	[REGSET_XSTATE] = {
		USER_REGSET_NOTE_TYPE(X86_XSTATE),
		.size		= sizeof(long),
		.align		= sizeof(long),
		.active		= generic_fpregs_active,
		.regset_get	= generic_fpregs_get,
		.set		= generic_fpregs_set,
	},
	/* TODO: Add TLS regset for 32bit */
};

static const struct user_regset_view user_uml_view = {
	.name = "x86_64", .e_machine = EM_X86_64,
	.regsets = uml_regsets, .n = ARRAY_SIZE(uml_regsets)
};

const struct user_regset_view *
task_user_regset_view(struct task_struct *tsk)
{
	return &user_uml_view;
}

static int __init init_regset_xstate_info(void)
{
	uml_regsets[REGSET_XSTATE].n =
		host_fp_size / uml_regsets[REGSET_XSTATE].size;

	return 0;
}
arch_initcall(init_regset_xstate_info);
