# SPDX-License-Identifier: GPL-2.0-only
ccflags-y += -O3
CFLAGS_lz4_compress.o += -Wno-frame-larger-than
CFLAGS_lz4_decompress.o += -Wno-frame-larger-than

obj-$(CONFIG_LZ4_COMPRESS) += lz4_compress.o
obj-$(CONFIG_LZ4HC_COMPRESS) += lz4hc_compress.o
obj-$(CONFIG_LZ4_DECOMPRESS) += lz4_decompress.o
