# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_TTY)		+= tty_io.o n_tty.o tty_ioctl.o tty_ldisc.o \
				   tty_buffer.o tty_port.o tty_mutex.o \
				   tty_ldsem.o tty_baudrate.o tty_jobctrl.o \
				   n_null.o
obj-$(CONFIG_LEGACY_PTYS)	+= pty.o
obj-$(CONFIG_UNIX98_PTYS)	+= pty.o
obj-$(CONFIG_AUDIT)		+= tty_audit.o
obj-$(CONFIG_MAGIC_SYSRQ)	+= sysrq.o
obj-$(CONFIG_N_HDLC)		+= n_hdlc.o
obj-$(CONFIG_N_GSM)		+= n_gsm.o

obj-y				+= vt/
obj-$(CONFIG_HVC_DRIVER)	+= hvc/
obj-y				+= serial/
obj-$(CONFIG_SERIAL_DEV_BUS)	+= serdev/

# tty drivers
# removed stale make refs from obj-$(CONFIG_AMIGA_BUILTIN_SERIAL)
obj-$(CONFIG_MOXA_SMARTIO)	+= mxser.o
obj-$(CONFIG_NOZOMI)		+= nozomi.o
obj-$(CONFIG_NULL_TTY)	        += ttynull.o
obj-$(CONFIG_PPC_EPAPR_HV_BYTECHAN) += ehv_bytechan.o
# removed stale make refs from obj-$(CONFIG_GOLDFISH_TTY)
obj-$(CONFIG_MIPS_EJTAG_FDC_TTY) += mips_ejtag_fdc.o
obj-$(CONFIG_VCC)		+= vcc.o
obj-$(CONFIG_RPMSG_TTY)		+= rpmsg_tty.o
