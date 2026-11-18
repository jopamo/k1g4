# SPDX-License-Identifier: GPL-2.0

# removed stale make refs from obj-$(CONFIG_GOOGLE_SMI)
obj-$(CONFIG_COREBOOT_TABLE)               += coreboot_table.o
obj-$(CONFIG_COREBOOT_CFR)		+= coreboot-cfr.o
obj-$(CONFIG_COREBOOT_FRAMEBUFFER)         += framebuffer-coreboot.o
obj-$(CONFIG_COREBOOT_MEMCONSOLE_COMMON)   += memconsole.o
obj-$(CONFIG_COREBOOT_MEMCONSOLE)          += memconsole-coreboot.o
# removed stale make refs from obj-$(CONFIG_GOOGLE_MEMCONSOLE_X86_LEGACY)

# Must come after coreboot_table.o, as this driver depends on that bus type.
obj-$(CONFIG_COREBOOT_CBMEM)		+= cbmem.o

# removed stale make refs from vpd-sysfs-y
# removed stale make refs from obj-$(CONFIG_GOOGLE_VPD)
