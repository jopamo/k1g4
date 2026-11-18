// SPDX-License-Identifier: GPL-2.0
/*
 * KUnit tests for in-kernel LZ4 APIs.
 */

#include <kunit/test.h>
#include <linux/lz4.h>
#include <linux/slab.h>

static void fill_test_pattern(char *buf, int len)
{
	int i;

	for (i = 0; i < len; i++)
		buf[i] = (char)((i * 37 + i / 7 + 13) & 0xff);
}

static void lz4_state_layout(struct kunit *test)
{
	KUNIT_EXPECT_EQ(test, (size_t)LZ4_MEM_COMPRESS, sizeof(LZ4_stream_t));
	KUNIT_EXPECT_EQ(test, (size_t)LZ4_STREAMDECODESIZE,
			sizeof(LZ4_streamDecode_t));
}

static void lz4_default_roundtrip(struct kunit *test)
{
	enum { SRC_LEN = 4096 };
	int max_csize = LZ4_compressBound(SRC_LEN);
	char *src = kunit_kmalloc(test, SRC_LEN, GFP_KERNEL);
	char *cbuf = kunit_kmalloc(test, max_csize, GFP_KERNEL);
	char *dbuf = kunit_kmalloc(test, SRC_LEN, GFP_KERNEL);
	void *wrkmem = kunit_kzalloc(test, LZ4_MEM_COMPRESS, GFP_KERNEL);
	int csize, dsize;

	KUNIT_ASSERT_NOT_NULL(test, src);
	KUNIT_ASSERT_NOT_NULL(test, cbuf);
	KUNIT_ASSERT_NOT_NULL(test, dbuf);
	KUNIT_ASSERT_NOT_NULL(test, wrkmem);

	fill_test_pattern(src, SRC_LEN);

	csize = LZ4_compress_default(src, cbuf, SRC_LEN, max_csize, wrkmem);
	KUNIT_ASSERT_GT(test, csize, 0);

	dsize = LZ4_decompress_safe(cbuf, dbuf, csize, SRC_LEN);
	KUNIT_EXPECT_EQ(test, dsize, SRC_LEN);
	KUNIT_EXPECT_MEMEQ(test, dbuf, src, SRC_LEN);
}

static void lz4_fast_roundtrip(struct kunit *test)
{
	enum { SRC_LEN = 4096 };
	int max_csize = LZ4_compressBound(SRC_LEN);
	char *src = kunit_kmalloc(test, SRC_LEN, GFP_KERNEL);
	char *cbuf = kunit_kmalloc(test, max_csize, GFP_KERNEL);
	char *dbuf = kunit_kmalloc(test, SRC_LEN, GFP_KERNEL);
	void *wrkmem = kunit_kzalloc(test, LZ4_MEM_COMPRESS, GFP_KERNEL);
	int csize, dsize, consumed;

	KUNIT_ASSERT_NOT_NULL(test, src);
	KUNIT_ASSERT_NOT_NULL(test, cbuf);
	KUNIT_ASSERT_NOT_NULL(test, dbuf);
	KUNIT_ASSERT_NOT_NULL(test, wrkmem);

	fill_test_pattern(src, SRC_LEN);

	csize = LZ4_compress_fast(src, cbuf, SRC_LEN, max_csize, 8, wrkmem);
	KUNIT_ASSERT_GT(test, csize, 0);

	dsize = LZ4_decompress_safe(cbuf, dbuf, csize, SRC_LEN);
	KUNIT_EXPECT_EQ(test, dsize, SRC_LEN);
	KUNIT_EXPECT_MEMEQ(test, dbuf, src, SRC_LEN);

	memset(dbuf, 0, SRC_LEN);
	consumed = LZ4_decompress_fast(cbuf, dbuf, SRC_LEN);
	KUNIT_EXPECT_EQ(test, consumed, csize);
	KUNIT_EXPECT_MEMEQ(test, dbuf, src, SRC_LEN);
}

static void lz4_destsize_partial_roundtrip(struct kunit *test)
{
	enum { SRC_LEN = 8192, TARGET_DST_SIZE = 64 };
	char *src = kunit_kzalloc(test, SRC_LEN, GFP_KERNEL);
	char *cbuf = kunit_kmalloc(test, TARGET_DST_SIZE, GFP_KERNEL);
	char *dbuf;
	void *wrkmem = kunit_kzalloc(test, LZ4_MEM_COMPRESS, GFP_KERNEL);
	int consumed_src = SRC_LEN;
	int csize, dsize;

	KUNIT_ASSERT_NOT_NULL(test, src);
	KUNIT_ASSERT_NOT_NULL(test, cbuf);
	KUNIT_ASSERT_NOT_NULL(test, wrkmem);

	dbuf = kunit_kmalloc(test, SRC_LEN, GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, dbuf);

	csize = LZ4_compress_destSize(src, cbuf, &consumed_src,
				      TARGET_DST_SIZE, wrkmem);
	KUNIT_ASSERT_GT(test, csize, 0);
	KUNIT_EXPECT_LE(test, csize, TARGET_DST_SIZE);
	KUNIT_ASSERT_GT(test, consumed_src, 0);
	KUNIT_EXPECT_LE(test, consumed_src, SRC_LEN);

	dsize = LZ4_decompress_safe(cbuf, dbuf, csize, consumed_src);
	KUNIT_EXPECT_EQ(test, dsize, consumed_src);
	KUNIT_EXPECT_MEMEQ(test, dbuf, src, consumed_src);
}

static struct kunit_case lz4_kunit_cases[] = {
	KUNIT_CASE(lz4_state_layout),
	KUNIT_CASE(lz4_default_roundtrip),
	KUNIT_CASE(lz4_fast_roundtrip),
	KUNIT_CASE(lz4_destsize_partial_roundtrip),
	{}
};

static struct kunit_suite lz4_kunit_suite = {
	.name = "lz4-kernel-api",
	.test_cases = lz4_kunit_cases,
};
kunit_test_suite(lz4_kunit_suite);

MODULE_DESCRIPTION("KUnit tests for kernel LZ4 APIs");
MODULE_LICENSE("GPL");
