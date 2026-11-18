# SPDX-License-Identifier: GPL-2.0
# Makefile for the Linux sound card driver
#

obj-$(CONFIG_SOUND) += soundcore.o
# removed stale make refs from obj-$(CONFIG_DMASOUND)
obj-$(CONFIG_SND) += core/ i2c/ drivers/ isa/ pci/ arm/ synth/ usb/ \
	firewire/ spi/ pcmcia/ soc/ atmel/ hda/ x86/ xen/ \
	virtio/
# removed stale make refs from obj-$(CONFIG_SND_AOA)

# This one must be compilable even if sound is configured out
obj-$(CONFIG_AC97_BUS) += ac97_bus.o
obj-$(CONFIG_AC97_BUS_NEW) += ac97/

ifeq ($(CONFIG_SND),y)
  obj-y += last.o
endif

soundcore-y  := sound_core.o
