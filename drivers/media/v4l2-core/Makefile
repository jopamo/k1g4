# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the V4L2 core
#


# removed stale make refs from tuner-objs

videodev-objs	:=	v4l2-dev.o v4l2-ioctl.o v4l2-device.o v4l2-fh.o \
			v4l2-event.o v4l2-subdev.o v4l2-common.o \
			v4l2-ctrls-core.o v4l2-ctrls-api.o \
			v4l2-ctrls-request.o v4l2-ctrls-defs.o

# Please keep it alphabetically sorted by Kconfig name
# (e. g. LC_ALL=C sort Makefile)
# removed stale make refs from videodev-$(CONFIG_COMPAT)
videodev-$(CONFIG_MEDIA_CONTROLLER) += v4l2-mc.o
videodev-$(CONFIG_SPI) += v4l2-spi.o
videodev-$(CONFIG_TRACEPOINTS) += v4l2-trace.o
videodev-$(CONFIG_VIDEO_V4L2_I2C) += v4l2-i2c.o

# Please keep it alphabetically sorted by Kconfig name
# (e. g. LC_ALL=C sort Makefile)

obj-$(CONFIG_V4L2_ASYNC) += v4l2-async.o
obj-$(CONFIG_V4L2_CCI) += v4l2-cci.o
obj-$(CONFIG_V4L2_FLASH_LED_CLASS) += v4l2-flash-led-class.o
obj-$(CONFIG_V4L2_FWNODE) += v4l2-fwnode.o
obj-$(CONFIG_V4L2_H264) += v4l2-h264.o
obj-$(CONFIG_V4L2_ISP) += v4l2-isp.o
obj-$(CONFIG_V4L2_JPEG_HELPER) += v4l2-jpeg.o
obj-$(CONFIG_V4L2_MEM2MEM_DEV) += v4l2-mem2mem.o
obj-$(CONFIG_V4L2_VP9) += v4l2-vp9.o

# removed stale make refs from obj-$(CONFIG_VIDEO_TUNER)
obj-$(CONFIG_VIDEO_DEV) += v4l2-dv-timings.o videodev.o
