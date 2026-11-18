// SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause
/*
 * Kernel wrapper for upstream lib/lz4/lz4.c.
 */

#include <linux/module.h>
#include <linux/string.h>

#define LZ4_FREESTANDING 1
#define LZ4_STATIC_LINKING_ONLY_DISABLE_MEMORY_ALLOCATION 1
#define LZ4_DISABLE_DEPRECATE_WARNINGS 1
#define LZ4_memcpy(dst, src, size) __builtin_memcpy(dst, src, size)
#define LZ4_memmove(dst, src, size) __builtin_memmove(dst, src, size)
#define LZ4_memset(dst, val, size) __builtin_memset(dst, val, size)

/* Keep compressor symbols out of this object to avoid duplicates. */
#define LZ4_versionNumber lz4_upstream_versionNumber_dec
#define LZ4_versionString lz4_upstream_versionString_dec
#define LZ4_compressBound lz4_upstream_compressBound_dec
#define LZ4_sizeofState lz4_upstream_sizeofState_dec
#define LZ4_compress_forceExtDict lz4_upstream_compress_forceExtDict_dec
#define LZ4_compress_fast_extState lz4_upstream_compress_fast_extState_dec
#define LZ4_compress_fast_extState_fastReset lz4_upstream_compress_fast_extState_fastReset_dec
#define LZ4_compress_fast lz4_upstream_compress_fast_dec
#define LZ4_compress_default lz4_upstream_compress_default_dec
#define LZ4_compress_destSize_extState lz4_upstream_compress_destSize_extState_dec
#define LZ4_compress_destSize lz4_upstream_compress_destSize_dec
#define LZ4_createStream lz4_upstream_createStream_dec
#define LZ4_initStream lz4_upstream_initStream_dec
#define LZ4_resetStream lz4_upstream_resetStream_dec
#define LZ4_resetStream_fast lz4_upstream_resetStream_fast_dec
#define LZ4_freeStream lz4_upstream_freeStream_dec
#define LZ4_loadDict lz4_upstream_loadDict_dec
#define LZ4_loadDictSlow lz4_upstream_loadDictSlow_dec
#define LZ4_attach_dictionary lz4_upstream_attach_dictionary_dec
#define LZ4_compress_fast_continue lz4_upstream_compress_fast_continue_dec
#define LZ4_saveDict lz4_upstream_saveDict_dec
#define LZ4_compress_limitedOutput lz4_upstream_compress_limitedOutput_dec
#define LZ4_compress lz4_upstream_compress_dec
#define LZ4_compress_limitedOutput_withState lz4_upstream_compress_limitedOutput_withState_dec
#define LZ4_compress_withState lz4_upstream_compress_withState_dec
#define LZ4_compress_limitedOutput_continue lz4_upstream_compress_limitedOutput_continue_dec
#define LZ4_compress_continue lz4_upstream_compress_continue_dec
#define LZ4_sizeofStreamState lz4_upstream_sizeofStreamState_dec
#define LZ4_resetStreamState lz4_upstream_resetStreamState_dec
#define LZ4_slideInputBuffer lz4_upstream_slideInputBuffer_dec

#ifdef current
#undef current
#endif

#include "lz4.c"

#undef LZ4_versionNumber
#undef LZ4_versionString
#undef LZ4_compressBound
#undef LZ4_sizeofState
#undef LZ4_compress_forceExtDict
#undef LZ4_compress_fast_extState
#undef LZ4_compress_fast_extState_fastReset
#undef LZ4_compress_fast
#undef LZ4_compress_default
#undef LZ4_compress_destSize_extState
#undef LZ4_compress_destSize
#undef LZ4_createStream
#undef LZ4_initStream
#undef LZ4_resetStream
#undef LZ4_resetStream_fast
#undef LZ4_freeStream
#undef LZ4_loadDict
#undef LZ4_loadDictSlow
#undef LZ4_attach_dictionary
#undef LZ4_compress_fast_continue
#undef LZ4_saveDict
#undef LZ4_compress_limitedOutput
#undef LZ4_compress
#undef LZ4_compress_limitedOutput_withState
#undef LZ4_compress_withState
#undef LZ4_compress_limitedOutput_continue
#undef LZ4_compress_continue
#undef LZ4_sizeofStreamState
#undef LZ4_resetStreamState
#undef LZ4_slideInputBuffer

EXPORT_SYMBOL(LZ4_decompress_safe);
EXPORT_SYMBOL(LZ4_decompress_safe_partial);
EXPORT_SYMBOL(LZ4_decompress_fast);
EXPORT_SYMBOL(LZ4_setStreamDecode);
EXPORT_SYMBOL(LZ4_decompress_safe_continue);
EXPORT_SYMBOL(LZ4_decompress_fast_continue);
EXPORT_SYMBOL(LZ4_decompress_safe_usingDict);
EXPORT_SYMBOL(LZ4_decompress_fast_usingDict);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("LZ4 decompressor");
