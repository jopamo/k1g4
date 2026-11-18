# SPDX-License-Identifier: GPL-2.0
#
# Building vDSO images for x86.
#

# Regular kernel objects
obj-y				:= vma.o extable.o
# removed stale make refs from obj-$(CONFIG_COMPAT_32)

# vDSO directories
obj-$(CONFIG_X86_64)		+= vdso64/
# removed stale make refs from obj-$(CONFIG_COMPAT_32)
