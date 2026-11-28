// SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause
/*
 * Kernel wrapper for upstream lib/lz4/lz4.c.
 *
 * Keep the kernel ABI with wrkmem-based entry points while importing
 * upstream compression logic.
 */

#include <linux/module.h>
#include <linux/string.h>

#define LZ4_FREESTANDING 1
#define LZ4_STATIC_LINKING_ONLY_DISABLE_MEMORY_ALLOCATION 1
#define LZ4_DISABLE_DEPRECATE_WARNINGS 1
#define LZ4_memcpy(dst, src, size) __builtin_memcpy(dst, src, size)
#define LZ4_memmove(dst, src, size) __builtin_memmove(dst, src, size)
#define LZ4_memset(dst, val, size) __builtin_memset(dst, val, size)

/*
 * Kernel signatures differ from upstream for these entry points.
 * Rename upstream symbols, then provide kernel wrappers below.
 */
#define LZ4_compress_default lz4_upstream_compress_default
#define LZ4_compress_fast lz4_upstream_compress_fast
#define LZ4_compress_destSize lz4_upstream_compress_destSize

/* Keep decompressor symbols out of this object to avoid duplicates. */
#define LZ4_decompress_safe_forceExtDict lz4_upstream_decompress_safe_forceExtDict
#define LZ4_decompress_safe_partial_forceExtDict lz4_upstream_decompress_safe_partial_forceExtDict
#define LZ4_decompress_safe lz4_upstream_decompress_safe
#define LZ4_decompress_safe_partial lz4_upstream_decompress_safe_partial
#define LZ4_decompress_fast lz4_upstream_decompress_fast
#define LZ4_decompress_safe_withPrefix64k lz4_upstream_decompress_safe_withPrefix64k
#define LZ4_decompress_fast_withPrefix64k lz4_upstream_decompress_fast_withPrefix64k
#define LZ4_decompress_safe_doubleDict lz4_upstream_decompress_safe_doubleDict
#define LZ4_createStreamDecode lz4_upstream_createStreamDecode
#define LZ4_freeStreamDecode lz4_upstream_freeStreamDecode
#define LZ4_setStreamDecode lz4_upstream_setStreamDecode
#define LZ4_decoderRingBufferSize lz4_upstream_decoderRingBufferSize
#define LZ4_decompress_safe_continue lz4_upstream_decompress_safe_continue
#define LZ4_decompress_fast_continue lz4_upstream_decompress_fast_continue
#define LZ4_decompress_safe_usingDict lz4_upstream_decompress_safe_usingDict
#define LZ4_decompress_safe_partial_usingDict lz4_upstream_decompress_safe_partial_usingDict
#define LZ4_decompress_fast_usingDict lz4_upstream_decompress_fast_usingDict
#define LZ4_uncompress lz4_upstream_uncompress
#define LZ4_uncompress_unknownOutputSize lz4_upstream_uncompress_unknownOutputSize

#ifdef current
#undef current
#endif

#include "lz4.c"

#undef LZ4_compress_default
#undef LZ4_compress_fast
#undef LZ4_compress_destSize

#undef LZ4_decompress_safe_forceExtDict
#undef LZ4_decompress_safe_partial_forceExtDict
#undef LZ4_decompress_safe
#undef LZ4_decompress_safe_partial
#undef LZ4_decompress_fast
#undef LZ4_decompress_safe_withPrefix64k
#undef LZ4_decompress_fast_withPrefix64k
#undef LZ4_decompress_safe_doubleDict
#undef LZ4_createStreamDecode
#undef LZ4_freeStreamDecode
#undef LZ4_setStreamDecode
#undef LZ4_decoderRingBufferSize
#undef LZ4_decompress_safe_continue
#undef LZ4_decompress_fast_continue
#undef LZ4_decompress_safe_usingDict
#undef LZ4_decompress_safe_partial_usingDict
#undef LZ4_decompress_fast_usingDict
#undef LZ4_uncompress
#undef LZ4_uncompress_unknownOutputSize

int LZ4_compress_default(const char *source, char *dest, int inputSize,
	int maxOutputSize, void *wrkmem);
int LZ4_compress_fast(const char *source, char *dest, int inputSize,
	int maxOutputSize, int acceleration, void *wrkmem);
int LZ4_compress_destSize(const char *source, char *dest, int *sourceSizePtr,
	int targetDestSize, void *wrkmem);

int LZ4_compress_default(const char *source, char *dest, int inputSize,
	int maxOutputSize, void *wrkmem)
{
	return LZ4_compress_fast_extState(wrkmem, source, dest,
				      inputSize, maxOutputSize, 1);
}
EXPORT_SYMBOL(LZ4_compress_default);

int LZ4_compress_fast(const char *source, char *dest, int inputSize,
	int maxOutputSize, int acceleration, void *wrkmem)
{
	return LZ4_compress_fast_extState(wrkmem, source, dest,
				      inputSize, maxOutputSize, acceleration);
}
EXPORT_SYMBOL(LZ4_compress_fast);

int LZ4_compress_destSize(const char *source, char *dest, int *sourceSizePtr,
	int targetDestSize, void *wrkmem)
{
	return LZ4_compress_destSize_extState(wrkmem, source, dest,
				      sourceSizePtr, targetDestSize, 1);
}
EXPORT_SYMBOL(LZ4_compress_destSize);

EXPORT_SYMBOL(LZ4_loadDict);
EXPORT_SYMBOL(LZ4_saveDict);
EXPORT_SYMBOL(LZ4_compress_fast_continue);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("LZ4 compressor");
