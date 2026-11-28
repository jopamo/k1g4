// SPDX-License-Identifier: GPL-2.0-or-later

#include <kunit/device.h>
#include <kunit/static_stub.h>
#include <kunit/test.h>

/* GSO */

#include <linux/skbuff.h>
#include <net/gso.h>

static const char hdr[] = "abcdefgh";
#define GSO_TEST_SIZE 1000

static void __init_skb(struct sk_buff *skb)
{
	skb_reset_mac_header(skb);
	memcpy(skb_mac_header(skb), hdr, sizeof(hdr));

	/* skb_segment expects skb->data at start of payload */
	skb_pull(skb, sizeof(hdr));
	skb_reset_network_header(skb);
	skb_reset_transport_header(skb);

	/* proto is arbitrary, as long as not ETH_P_TEB or vlan */
	skb->protocol = htons(ETH_P_ATALK);
	skb_shinfo(skb)->gso_size = GSO_TEST_SIZE;
}

enum gso_test_nr {
	GSO_TEST_LINEAR,
	GSO_TEST_NO_GSO,
	GSO_TEST_FRAGS,
	GSO_TEST_FRAGS_PURE,
	GSO_TEST_GSO_PARTIAL,
	GSO_TEST_FRAG_LIST,
	GSO_TEST_FRAG_LIST_PURE,
	GSO_TEST_FRAG_LIST_NON_UNIFORM,
	GSO_TEST_FRAG_LIST_HEAD_FRAG,
	GSO_TEST_FRAG_LIST_NO_HEAD_FRAG,
	GSO_TEST_GSO_BY_FRAGS,
	GSO_TEST_RESEGMENT,
	GSO_TEST_RESEGMENT_MULTI,
	GSO_TEST_RESEGMENT_TAIL,
	GSO_TEST_RESEGMENT_ONE_MSS,
};

struct gso_test_case {
	enum gso_test_nr id;
	const char *name;

	/* input */
	unsigned int linear_len;
	unsigned int nr_frags;
	const unsigned int *frags;
	unsigned int nr_frag_skbs;
	const unsigned int *frag_skbs;
	unsigned int max_segs;
	const unsigned int *frag_skb_frags;
	unsigned int gso_type;
	/* skbs from alloc_skb() instead of build_skb(), head_frag clear */
	bool no_head_frag;

	/* output as expected */
	unsigned int nr_segs;
	const unsigned int *segs;
	const unsigned int *gso_segs;
};

static struct gso_test_case cases[] = {
	{
		.id = GSO_TEST_NO_GSO,
		.name = "no_gso",
		.linear_len = GSO_TEST_SIZE,
		.nr_segs = 1,
		.segs = (const unsigned int[]) { GSO_TEST_SIZE },
	},
	{
		.id = GSO_TEST_LINEAR,
		.name = "linear",
		.linear_len = GSO_TEST_SIZE + GSO_TEST_SIZE + 1,
		.nr_segs = 3,
		.segs = (const unsigned int[]) { GSO_TEST_SIZE, GSO_TEST_SIZE, 1 },
	},
	{
		.id = GSO_TEST_FRAGS,
		.name = "frags",
		.linear_len = GSO_TEST_SIZE,
		.nr_frags = 2,
		.frags = (const unsigned int[]) { GSO_TEST_SIZE, 1 },
		.nr_segs = 3,
		.segs = (const unsigned int[]) { GSO_TEST_SIZE, GSO_TEST_SIZE, 1 },
	},
	{
		.id = GSO_TEST_FRAGS_PURE,
		.name = "frags_pure",
		.nr_frags = 3,
		.frags = (const unsigned int[]) { GSO_TEST_SIZE, GSO_TEST_SIZE, 2 },
		.nr_segs = 3,
		.segs = (const unsigned int[]) { GSO_TEST_SIZE, GSO_TEST_SIZE, 2 },
	},
	{
		.id = GSO_TEST_GSO_PARTIAL,
		.name = "gso_partial",
		.linear_len = GSO_TEST_SIZE,
		.nr_frags = 2,
		.frags = (const unsigned int[]) { GSO_TEST_SIZE, 3 },
		.nr_segs = 2,
		.segs = (const unsigned int[]) { 2 * GSO_TEST_SIZE, 3 },
	},
	{
		/* commit 89319d3801d1: frag_list on mss boundaries */
		.id = GSO_TEST_FRAG_LIST,
		.name = "frag_list",
		.linear_len = GSO_TEST_SIZE,
		.nr_frag_skbs = 2,
		.frag_skbs = (const unsigned int[]) { GSO_TEST_SIZE, GSO_TEST_SIZE },
		.nr_segs = 3,
		.segs = (const unsigned int[]) { GSO_TEST_SIZE, GSO_TEST_SIZE, GSO_TEST_SIZE },
	},
	{
		.id = GSO_TEST_FRAG_LIST_PURE,
		.name = "frag_list_pure",
		.nr_frag_skbs = 2,
		.frag_skbs = (const unsigned int[]) { GSO_TEST_SIZE, GSO_TEST_SIZE },
		.nr_segs = 2,
		.segs = (const unsigned int[]) { GSO_TEST_SIZE, GSO_TEST_SIZE },
	},
	{
		/* commit 43170c4e0ba7: GRO of frag_list trains */
		.id = GSO_TEST_FRAG_LIST_NON_UNIFORM,
		.name = "frag_list_non_uniform",
		.linear_len = GSO_TEST_SIZE,
		.nr_frag_skbs = 4,
		.frag_skbs = (const unsigned int[]) { GSO_TEST_SIZE, 1, GSO_TEST_SIZE, 2 },
		.nr_segs = 4,
		.segs = (const unsigned int[]) { GSO_TEST_SIZE, GSO_TEST_SIZE, GSO_TEST_SIZE, 3 },
	},
	{
		/* commit 13acc94eff12: frag_list members with linear data in a
		 * page fragment, shared as a page frag of the segment
		 */
		.id = GSO_TEST_FRAG_LIST_HEAD_FRAG,
		.name = "frag_list_head_frag",
		.linear_len = 8,
		.nr_frags = 1,
		.frags = (const unsigned int[]) { 64 },
		.nr_frag_skbs = 1,
		.frag_skbs = (const unsigned int[]) { 8 },
		.frag_skb_frags = (const unsigned int[]) { 64 },
		.gso_type = SKB_GSO_DODGY,
		.nr_segs = 1,
		.segs = (const unsigned int[]) { 144 },
	},
	{
		/* commit 3dcbdb134f32: gso_size smaller than the linear data,
		 * as after bpf_skb_change_proto() grew the header, with
		 * frag_list members whose linear data is not in a page fragment
		 */
		.id = GSO_TEST_FRAG_LIST_NO_HEAD_FRAG,
		.name = "frag_list_no_head_frag",
		.linear_len = GSO_TEST_SIZE + 20,
		.nr_frag_skbs = 1,
		.frag_skbs = (const unsigned int[]) { GSO_TEST_SIZE + 20 },
		.gso_type = SKB_GSO_DODGY,
		.no_head_frag = true,
		.nr_segs = 3,
		.segs = (const unsigned int[]) { GSO_TEST_SIZE, GSO_TEST_SIZE, 40 },
	},
	{
		/* commit 3953c46c3ac7 ("sk_buff: allow segmenting based on frag sizes") and
		 * commit 90017accff61 ("sctp: Add GSO support")
		 *
		 * "there will be a cover skb with protocol headers and
		 *  children ones containing the actual segments"
		 */
		.id = GSO_TEST_GSO_BY_FRAGS,
		.name = "gso_by_frags",
		.nr_frag_skbs = 4,
		.frag_skbs = (const unsigned int[]) { 100, 200, 300, 400 },
		.nr_segs = 4,
		.segs = (const unsigned int[]) { 100, 200, 300, 400 },
	},
	{
		.id = GSO_TEST_RESEGMENT,
		.name = "resegment",
		.linear_len = GSO_TEST_SIZE,
		.nr_frags = 3,
		.frags = (const unsigned int[]) {
			GSO_TEST_SIZE, GSO_TEST_SIZE, 3,
		},
		.max_segs = 2,
		.nr_segs = 2,
		.segs = (const unsigned int[]) {
			2 * GSO_TEST_SIZE, GSO_TEST_SIZE + 3,
		},
		.gso_segs = (const unsigned int[]) { 2, 2 },
	},
	{
		.id = GSO_TEST_RESEGMENT_MULTI,
		.name = "resegment_multi",
		.linear_len = 2 * GSO_TEST_SIZE,
		.nr_frags = 4,
		.frags = (const unsigned int[]) {
			GSO_TEST_SIZE, GSO_TEST_SIZE, GSO_TEST_SIZE, 3,
		},
		.max_segs = 2,
		.nr_segs = 3,
		.segs = (const unsigned int[]) {
			2 * GSO_TEST_SIZE, 2 * GSO_TEST_SIZE, GSO_TEST_SIZE + 3,
		},
		.gso_segs = (const unsigned int[]) { 2, 2, 2 },
	},
	{
		/* The last output is not a GSO skb. */
		.id = GSO_TEST_RESEGMENT_TAIL,
		.name = "resegment_tail",
		.linear_len = GSO_TEST_SIZE,
		.nr_frags = 2,
		.frags = (const unsigned int[]) {
			GSO_TEST_SIZE, 400,
		},
		.max_segs = 2,
		.nr_segs = 2,
		.segs = (const unsigned int[]) {
			2 * GSO_TEST_SIZE, 400,
		},
		.gso_segs = (const unsigned int[]) { 2, 0 },
	},
	{
		/* max_segs of 1 is the same as no limit. */
		.id = GSO_TEST_RESEGMENT_ONE_MSS,
		.name = "resegment_one_mss",
		.linear_len = GSO_TEST_SIZE,
		.nr_frags = 3,
		.frags = (const unsigned int[]) {
			GSO_TEST_SIZE, GSO_TEST_SIZE, 3,
		},
		.max_segs = 1,
		.nr_segs = 4,
		.segs = (const unsigned int[]) {
			GSO_TEST_SIZE, GSO_TEST_SIZE, GSO_TEST_SIZE, 3,
		},
		.gso_segs = (const unsigned int[]) { 0, 0, 0, 0 },
	},
};

static void gso_test_case_to_desc(struct gso_test_case *t, char *desc)
{
	sprintf(desc, "%s", t->name);
}

KUNIT_ARRAY_PARAM(gso_test, cases, gso_test_case_to_desc);

static void gso_test_func(struct kunit *test)
{
	const int shinfo_size = SKB_DATA_ALIGN(sizeof(struct skb_shared_info));
	struct sk_buff *skb, *segs, *cur, *next, *last;
	const struct gso_test_case *tcase;
	netdev_features_t features;
	struct page *page;
	int i;

	tcase = test->param_value;

	if (tcase->no_head_frag) {
		skb = alloc_skb(sizeof(hdr) + tcase->linear_len, GFP_KERNEL);
	} else {
		page = alloc_page(GFP_KERNEL);
		KUNIT_ASSERT_NOT_NULL(test, page);
		skb = build_skb(page_address(page), sizeof(hdr) + tcase->linear_len + shinfo_size);
	}
	KUNIT_ASSERT_NOT_NULL(test, skb);
	__skb_put(skb, sizeof(hdr) + tcase->linear_len);

	__init_skb(skb);
	skb_shinfo(skb)->gso_type = tcase->gso_type;

	if (tcase->nr_frags) {
		unsigned int pg_off = 0;

		page = alloc_page(GFP_KERNEL);
		KUNIT_ASSERT_NOT_NULL(test, page);
		page_ref_add(page, tcase->nr_frags - 1);

		for (i = 0; i < tcase->nr_frags; i++) {
			skb_fill_page_desc(skb, i, page, pg_off, tcase->frags[i]);
			pg_off += tcase->frags[i];
		}

		KUNIT_ASSERT_LE(test, pg_off, PAGE_SIZE);

		skb->data_len = pg_off;
		skb->len += skb->data_len;
		skb->truesize += skb->data_len;
	}

	if (tcase->frag_skbs) {
		unsigned int total_size = 0, total_true_size = 0;
		struct sk_buff *frag_skb, *prev = NULL;

		for (i = 0; i < tcase->nr_frag_skbs; i++) {
			unsigned int frag_size, frag_len;

			frag_size = tcase->frag_skbs[i];
			if (tcase->no_head_frag) {
				frag_skb = alloc_skb(frag_size, GFP_KERNEL);
			} else {
				page = alloc_page(GFP_KERNEL);
				KUNIT_ASSERT_NOT_NULL(test, page);
				frag_skb = build_skb(page_address(page),
						     frag_size + shinfo_size);
			}
			KUNIT_ASSERT_NOT_NULL(test, frag_skb);
			__skb_put(frag_skb, frag_size);

			frag_len = tcase->frag_skb_frags ? tcase->frag_skb_frags[i] : 0;
			if (frag_len) {
				page = alloc_page(GFP_KERNEL);
				KUNIT_ASSERT_NOT_NULL(test, page);
				skb_fill_page_desc(frag_skb, 0, page, 0, frag_len);

				frag_skb->data_len = frag_len;
				frag_skb->len += frag_len;
				frag_skb->truesize += frag_len;
			}

			if (prev)
				prev->next = frag_skb;
			else
				skb_shinfo(skb)->frag_list = frag_skb;
			prev = frag_skb;

			total_size += frag_skb->len;
			total_true_size += frag_skb->truesize;
		}

		skb->len += total_size;
		skb->data_len += total_size;
		skb->truesize += total_true_size;

		if (tcase->id == GSO_TEST_GSO_BY_FRAGS)
			skb_shinfo(skb)->gso_size = GSO_BY_FRAGS;
	}

	features = NETIF_F_SG | NETIF_F_HW_CSUM;
	if (tcase->id == GSO_TEST_GSO_PARTIAL)
		features |= NETIF_F_GSO_PARTIAL;

	/* TODO: this should also work with SG,
	 * rather than hit BUG_ON(i >= nfrags)
	 */
	if (tcase->id == GSO_TEST_FRAG_LIST_NON_UNIFORM)
		features &= ~NETIF_F_SG;

	SKB_GSO_CB(skb)->max_segs = tcase->max_segs;
	segs = skb_segment(skb, features);
	if (IS_ERR(segs)) {
		KUNIT_FAIL(test, "segs error %pe", segs);
		goto free_gso_skb;
	} else if (!segs) {
		KUNIT_FAIL(test, "no segments");
		goto free_gso_skb;
	}

	last = segs->prev;
	for (cur = segs, i = 0; cur; cur = next, i++) {
		next = cur->next;

		if (i >= tcase->nr_segs)
			goto consume;

		KUNIT_ASSERT_EQ(test, cur->len, sizeof(hdr) + tcase->segs[i]);

		/* segs have skb->data pointing to the mac header */
		KUNIT_ASSERT_PTR_EQ(test, skb_mac_header(cur), cur->data);
		KUNIT_ASSERT_PTR_EQ(test, skb_network_header(cur), cur->data + sizeof(hdr));

		/* header was copied to all segs */
		KUNIT_ASSERT_EQ(test, memcmp(skb_mac_header(cur), hdr, sizeof(hdr)), 0);
		if (tcase->gso_segs) {
			unsigned int gso_segs = tcase->gso_segs[i];

			KUNIT_EXPECT_EQ(test, skb_shinfo(cur)->gso_segs,
					gso_segs);
			if (!gso_segs) {
				KUNIT_EXPECT_FALSE(test, skb_is_gso(cur));
			} else {
				KUNIT_EXPECT_TRUE(test, skb_is_gso(cur));
				KUNIT_EXPECT_EQ(test, skb_shinfo(cur)->gso_size,
						GSO_TEST_SIZE);
			}
		}

		/* last seg can be found through segs->prev pointer */
		if (!next)
			KUNIT_ASSERT_PTR_EQ(test, cur, last);

consume:
		consume_skb(cur);
	}

	KUNIT_ASSERT_EQ(test, i, tcase->nr_segs);

free_gso_skb:
	consume_skb(skb);
}

/* IP tunnel flags */

#include <net/ip_tunnels.h>

struct ip_tunnel_flags_test {
	const char	*name;

	const u16	*src_bits;
	const u16	*exp_bits;
	u8		src_num;
	u8		exp_num;

	__be16		exp_val;
	bool		exp_comp;
};

#define IP_TUNNEL_FLAGS_TEST(n, src, comp, eval, exp) {	\
	.name		= (n),				\
	.src_bits	= (src),			\
	.src_num	= ARRAY_SIZE(src),		\
	.exp_comp	= (comp),			\
	.exp_val	= (eval),			\
	.exp_bits	= (exp),			\
	.exp_num	= ARRAY_SIZE(exp),		\
}

/* These are __be16-compatible and can be compared as is */
static const u16 ip_tunnel_flags_1[] = {
	IP_TUNNEL_KEY_BIT,
	IP_TUNNEL_STRICT_BIT,
	IP_TUNNEL_ERSPAN_OPT_BIT,
};

/* Due to the previous flags design limitation, setting either
 * ``IP_TUNNEL_CSUM_BIT`` (on Big Endian) or ``IP_TUNNEL_DONT_FRAGMENT_BIT``
 * (on Little) also sets VTI/ISATAP bit. In the bitmap implementation, they
 * correspond to ``BIT(16)``, which is bigger than ``U16_MAX``, but still is
 * backward-compatible.
 */
#ifdef __LITTLE_ENDIAN
#define IP_TUNNEL_CONFLICT_BIT	IP_TUNNEL_DONT_FRAGMENT_BIT
#else
#define IP_TUNNEL_CONFLICT_BIT	IP_TUNNEL_CSUM_BIT
#endif

static const u16 ip_tunnel_flags_2_src[] = {
	IP_TUNNEL_CONFLICT_BIT,
};

static const u16 ip_tunnel_flags_2_exp[] = {
	IP_TUNNEL_CONFLICT_BIT,
	IP_TUNNEL_SIT_ISATAP_BIT,
};

/* Bits 17 and higher are not compatible with __be16 flags */
static const u16 ip_tunnel_flags_3_src[] = {
	IP_TUNNEL_VXLAN_OPT_BIT,
	17,
	18,
	20,
};

static const u16 ip_tunnel_flags_3_exp[] = {
	IP_TUNNEL_VXLAN_OPT_BIT,
};

static const struct ip_tunnel_flags_test ip_tunnel_flags_test[] = {
	IP_TUNNEL_FLAGS_TEST("compat", ip_tunnel_flags_1, true,
			     cpu_to_be16(BIT(IP_TUNNEL_KEY_BIT) |
					 BIT(IP_TUNNEL_STRICT_BIT) |
					 BIT(IP_TUNNEL_ERSPAN_OPT_BIT)),
			     ip_tunnel_flags_1),
	IP_TUNNEL_FLAGS_TEST("conflict", ip_tunnel_flags_2_src, true,
			     VTI_ISVTI, ip_tunnel_flags_2_exp),
	IP_TUNNEL_FLAGS_TEST("new", ip_tunnel_flags_3_src, false,
			     cpu_to_be16(BIT(IP_TUNNEL_VXLAN_OPT_BIT)),
			     ip_tunnel_flags_3_exp),
};

static void
ip_tunnel_flags_test_case_to_desc(const struct ip_tunnel_flags_test *t,
				  char *desc)
{
	strscpy(desc, t->name, KUNIT_PARAM_DESC_SIZE);
}
KUNIT_ARRAY_PARAM(ip_tunnel_flags_test, ip_tunnel_flags_test,
		  ip_tunnel_flags_test_case_to_desc);

static void ip_tunnel_flags_test_run(struct kunit *test)
{
	const struct ip_tunnel_flags_test *t = test->param_value;
	IP_TUNNEL_DECLARE_FLAGS(src) = { };
	IP_TUNNEL_DECLARE_FLAGS(exp) = { };
	IP_TUNNEL_DECLARE_FLAGS(out);

	for (u32 j = 0; j < t->src_num; j++)
		__set_bit(t->src_bits[j], src);
	for (u32 j = 0; j < t->exp_num; j++)
		__set_bit(t->exp_bits[j], exp);

	KUNIT_ASSERT_EQ(test, t->exp_comp,
			ip_tunnel_flags_is_be16_compat(src));
	KUNIT_ASSERT_EQ(test, (__force u16)t->exp_val,
			(__force u16)ip_tunnel_flags_to_be16(src));

	ip_tunnel_flags_from_be16(out, t->exp_val);
	KUNIT_ASSERT_TRUE(test, __ipt_flag_op(bitmap_equal, exp, out));
}

/* Netmem & SKB fragment refcounting */

#include <linux/dma-map-ops.h>
#include <linux/skbuff_ref.h>
#include <net/netdev_rx_queue.h>
#include <net/page_pool/helpers.h>
#include <net/tcp.h>

struct frag_completion {
	struct ubuf_info uarg;
	unsigned int calls;
};

static void frag_complete(struct sk_buff *skb, struct ubuf_info *uarg, bool success)
{
	container_of(uarg, struct frag_completion, uarg)->calls++;
}

static void skb_managed_frag_test(struct kunit *test)
{
	static const struct ubuf_info_ops ops = { .complete = frag_complete };
	struct frag_completion done = { .uarg = { .ops = &ops,
		.flags = SKBFL_ZEROCOPY_ENABLE | SKBFL_MANAGED_FRAG_REFS } };
	struct page *page = alloc_page(GFP_KERNEL);
	int i;

	KUNIT_ASSERT_NOT_NULL(test, page);
	/* Stand in for the registered buffer. Keep a separate observation ref. */
	get_page(page);
	for (i = 0; i < 2; i++) {
		struct sk_buff *skb = alloc_skb(0, GFP_KERNEL);

		if (!skb) {
			KUNIT_FAIL(test, "allocating managed skb");
			break;
		}
		skb_zcopy_init(skb, &done.uarg);
		skb_fill_page_desc_noacc(skb, 0, page, 32, 64);
		KUNIT_EXPECT_EQ(test, page_ref_count(page), 2);
		if (i) {
			skb_zcopy_downgrade_managed(skb);
			KUNIT_EXPECT_FALSE(test, skb_zcopy_managed(skb));
			KUNIT_EXPECT_EQ(test, page_ref_count(page), 3);
		}
		consume_skb(skb);
		KUNIT_EXPECT_EQ(test, done.calls, i + 1);
		KUNIT_EXPECT_EQ(test, page_ref_count(page), 2);
		if (page_ref_count(page) != 2)
			break;
	}
	/* The observation reference also lets us diagnose an erroneous put. */
	if (page_ref_count(page) > 1)
		put_page(page);
	put_page(page);
}

static void skb_frag_ref_test_page(struct kunit *test)
{
	struct sk_buff *skb;
	struct page *page;
	int base_ref;

	page = alloc_page(GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, page);
	base_ref = page_ref_count(page);

	/* Layer 1: get_page / put_page use page non-pp refcount */
	get_page(page);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref + 1);
	put_page(page);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref);

#ifdef CONFIG_PAGE_POOL
	/* Layer 2: napi_pp_get/put_page return false on non-PP page */
	KUNIT_EXPECT_FALSE(test, napi_pp_get_page(page));
	KUNIT_EXPECT_FALSE(test, napi_pp_put_page(page));
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref);
#endif

	/* Layer 3: skb_page_ref / unref fall back to non-pp on non-PP page */
	skb_page_ref(page, false);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref + 1);
	skb_page_unref(page, false);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref);

	skb_page_ref(page, true);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref + 1);
	skb_page_unref(page, true);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref);

	/* Layer 4: skb_frag_ref / skb_frag_unref on non-PP page */
	skb = alloc_skb(64, GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, skb);
	skb_fill_page_desc(skb, 0, page, 0, 64);

	skb->pp_recycle = 1;
	skb_frag_ref(skb, 0);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref + 1);
	skb_frag_unref(skb, 0);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref);

	skb_shinfo(skb)->nr_frags = 0;
	consume_skb(skb);
	__free_page(page);
}

#ifdef CONFIG_PAGE_POOL
static void page_pool_alloc_test(struct kunit *test)
{
	struct page_pool_params params = { .nid = NUMA_NO_NODE };
	struct page *pages[4] = {};
	unsigned int offset, size;
	struct page_pool *pool;
	int i;

	pool = page_pool_create(&params);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pool);

	size = PAGE_SIZE / 2 + 1;
	pages[0] = page_pool_alloc(pool, &offset, &size, GFP_KERNEL);
	KUNIT_EXPECT_NOT_NULL(test, pages[0]);
	KUNIT_EXPECT_EQ(test, offset, 0);
	KUNIT_EXPECT_EQ(test, size, PAGE_SIZE);
	KUNIT_EXPECT_NULL(test, pool->frag_page);

	size = PAGE_SIZE / 2;
	pages[1] = page_pool_alloc(pool, &offset, &size, GFP_KERNEL);
	KUNIT_EXPECT_NOT_NULL(test, pages[1]);
	if (!pages[1])
		goto out;
	KUNIT_EXPECT_EQ(test, offset, 0);
	KUNIT_EXPECT_EQ(test, size, PAGE_SIZE / 2);

	/* The last fragment accounts for the whole tail, including padding. */
	size--;
	pages[2] = page_pool_alloc(pool, &offset, &size, GFP_KERNEL);
	KUNIT_EXPECT_PTR_EQ(test, pages[2], pages[1]);
	KUNIT_EXPECT_EQ(test, offset, PAGE_SIZE / 2);
	KUNIT_EXPECT_EQ(test, size, PAGE_SIZE / 2);

	/* The old fragments are still held, so rollover needs another page. */
	pages[3] = page_pool_alloc_frag(pool, &offset, PAGE_SIZE, GFP_KERNEL);
	KUNIT_EXPECT_NOT_NULL(test, pages[3]);
	if (pages[3]) {
		struct page *backing = pages[3];

		KUNIT_EXPECT_PTR_NE(test, backing, pages[1]);
		KUNIT_EXPECT_EQ(test, offset, 0);
		page_pool_put_full_page(pool, backing, false);

		/* Now there are no outstanding fragments on the current page. */
		pages[3] = page_pool_alloc_frag(pool, &offset, PAGE_SIZE, GFP_KERNEL);
		KUNIT_EXPECT_PTR_EQ(test, pages[3], backing);
		KUNIT_EXPECT_EQ(test, offset, 0);
	}
out:
	for (i = 0; i < ARRAY_SIZE(pages); i++)
		if (pages[i])
			page_pool_put_full_page(pool, pages[i], false);
	page_pool_destroy(pool);
}

#include "page_pool_priv.h"

struct pp_test_mapping {
	struct device *dev;
	dma_addr_t addr;
	size_t size;
	enum dma_data_direction dir;
	unsigned long attrs;
};

struct page_pool_dma_test_state {
	struct pp_test_mapping mapped, unmapped;
	struct page *observed;
	unsigned int attempts;
	unsigned int maps, unmaps, syncs;
	struct pp_test_mapping synced;
	bool fail_map;
	struct page *return_on_unmap;
	dma_addr_t last_unmap;
};

static dma_addr_t pp_test_map_phys(struct device *dev, phys_addr_t phys,
				   size_t size, enum dma_data_direction dir,
				   unsigned long attrs)
{
	struct page_pool_dma_test_state *state = dev_get_drvdata(dev);

	state->mapped = (struct pp_test_mapping){ dev, phys, size, dir, attrs };
	state->maps++;
	return state->fail_map ? DMA_MAPPING_ERROR : phys;
}

static void pp_test_unmap_phys(struct device *dev, dma_addr_t addr, size_t size,
			       enum dma_data_direction dir, unsigned long attrs)
{
	struct page_pool_dma_test_state *state = dev_get_drvdata(dev);
	struct page *page = state->return_on_unmap;

	state->unmapped = (struct pp_test_mapping){ dev, addr, size, dir, attrs };
	state->unmaps++;
	state->last_unmap = addr;
	if (page) {
		state->return_on_unmap = NULL;
		page_pool_put_full_page_to_owner(page, false);
	}
}

static void pp_test_sync(struct device *dev, dma_addr_t addr, size_t size,
			 enum dma_data_direction dir)
{
	struct page_pool_dma_test_state *state = dev_get_drvdata(dev);

	state->synced = (struct pp_test_mapping){ dev, addr, size, dir };
	state->syncs++;
}

static const struct dma_map_ops pp_test_dma_ops = {
	.map_phys = pp_test_map_phys,
	.unmap_phys = pp_test_unmap_phys,
	.sync_single_for_device = pp_test_sync,
};

static void page_pool_dma_test(struct kunit *test)
{
	struct page_pool_params params = {
		.nid = NUMA_NO_NODE,
		.order = 1,
		.flags = PP_FLAG_DMA_MAP,
		.dma_dir = DMA_FROM_DEVICE,
	};
	struct page_pool_dma_test_state state = { .fail_map = true };
	struct page_pool *pool;
	struct device *dev;
	struct page *page;
	unsigned long id;
	int i;

	if (!IS_ENABLED(CONFIG_ARCH_HAS_DMA_OPS) ||
	    IS_ENABLED(CONFIG_ARCH_HAS_DMA_MAP_DIRECT) || !PP_DMA_INDEX_BITS)
		kunit_skip(test, "requires DMA map ops and page-pool DMA indices");

	dev = kunit_device_register(test, "pp-dma");
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, dev);
	dev_set_drvdata(dev, &state);
	dev->coherent_dma_mask = DMA_BIT_MASK(64);
	dev->dma_mask = &dev->coherent_dma_mask;
	set_dma_ops(dev, &pp_test_dma_ops);
	params.dev = dev;
	pool = page_pool_create(&params);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pool);

	page = page_pool_alloc_pages(pool, GFP_KERNEL);
	KUNIT_EXPECT_NULL(test, page);
	KUNIT_EXPECT_EQ(test, state.maps, 1);
	KUNIT_EXPECT_EQ(test, state.unmaps, 0);
	KUNIT_EXPECT_EQ(test, pool->pages_state_hold_cnt, 0);
	KUNIT_EXPECT_TRUE(test, xa_empty(&pool->dma_mapped));
	if (page) {
		page_pool_put_full_page(pool, page, false);
		goto destroy;
	}

	state.fail_map = false;
	for (i = 0; i < 2; i++) {
		page = page_pool_alloc_pages(pool, GFP_KERNEL);
		if (!page) {
			KUNIT_FAIL(test, "allocating tracked DMA page");
			goto destroy;
		}
		id = (page->pp_magic & PP_DMA_INDEX_MASK) >> PP_DMA_INDEX_SHIFT;
		KUNIT_EXPECT_NE(test, id, 0UL);
		KUNIT_EXPECT_PTR_EQ(test, xa_load(&pool->dma_mapped, id), page);
		KUNIT_EXPECT_TRUE(test, page_pool_page_is_pp(page));
		KUNIT_EXPECT_EQ(test, page_pool_get_dma_addr(page), page_to_phys(page));
		get_page(page); /* Force detach; inspect metadata before ordinary free. */
		if (!i) {
			page_pool_put_full_page_to_owner(page, false);
			KUNIT_EXPECT_TRUE(test, xa_empty(&pool->dma_mapped));
		} else {
			/* Return after scrub claims the entry but before unmap finishes. */
			state.return_on_unmap = page;
			page_pool_destroy(pool);
			if (state.return_on_unmap) {
				KUNIT_FAIL(test, "scrub did not unmap the page");
				state.return_on_unmap = NULL;
				page_pool_put_full_page(pool, page, false);
			}
		}
		KUNIT_EXPECT_EQ(test, state.unmaps, i + 1);
		KUNIT_EXPECT_EQ(test, state.last_unmap, page_to_phys(page));
		KUNIT_EXPECT_FALSE(test, page_pool_page_is_pp(page));
		KUNIT_EXPECT_NULL(test, page->pp);
		KUNIT_EXPECT_EQ(test, page->pp_magic, 0UL);
		KUNIT_EXPECT_EQ(test, page_pool_get_dma_addr(page), 0);
		put_page(page);
	}
	return;

destroy:
	page_pool_destroy(pool);
}

static void page_pool_skb_return_sync_test(struct kunit *test)
{
	struct page_pool_params params = {
		.nid = NUMA_NO_NODE, .order = 1,
		.flags = PP_FLAG_DMA_MAP | PP_FLAG_DMA_SYNC_DEV,
		.dma_dir = DMA_FROM_DEVICE, .offset = 128, .max_len = 1536,
	};
	struct page_pool_dma_test_state state = {};
	struct page_pool *pool;
	struct sk_buff *skb;
	struct device *dev;
	struct page *page;
	unsigned int before;

	if (!IS_ENABLED(CONFIG_ARCH_HAS_DMA_OPS) ||
	    IS_ENABLED(CONFIG_ARCH_HAS_DMA_MAP_DIRECT) || !IS_ENABLED(CONFIG_DMA_NEED_SYNC))
		kunit_skip(test, "requires observable DMA sync ops");
	dev = kunit_device_register(test, "pp-skb-sync");
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, dev);
	dev_set_drvdata(dev, &state);
	dev->coherent_dma_mask = DMA_BIT_MASK(64);
	dev->dma_mask = &dev->coherent_dma_mask;
	set_dma_ops(dev, &pp_test_dma_ops);
	params.dev = dev;
	pool = page_pool_create(&params);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pool);
	page = page_pool_alloc_pages(pool, GFP_KERNEL);
	KUNIT_EXPECT_NOT_NULL(test, page);
	if (!page)
		goto destroy;
	before = state.syncs;
	KUNIT_EXPECT_EQ(test, before, 1);
	skb = build_skb(page_address(page), PAGE_SIZE << params.order);
	KUNIT_EXPECT_NOT_NULL(test, skb);
	if (!skb) {
		page_pool_put_full_page(pool, page, false);
		goto destroy;
	}
	skb_reserve(skb, params.offset);
	skb_put(skb, 64);
	skb_mark_for_recycle(skb);
	kfree_skb(skb);
	KUNIT_EXPECT_EQ(test, state.syncs, before + 1);
	KUNIT_EXPECT_EQ(test, state.synced.addr, state.mapped.addr + params.offset);
	KUNIT_EXPECT_EQ(test, state.synced.size, params.max_len);
	KUNIT_EXPECT_EQ(test, state.synced.dir, params.dma_dir);
destroy:
	page_pool_destroy(pool);
	KUNIT_EXPECT_EQ(test, state.maps, state.unmaps);
}

static int pp_test_fail_index(struct page_pool *pool, struct page *page, gfp_t gfp)
{
	struct kunit *test = kunit_get_current_test();
	struct page_pool_dma_test_state *state = dev_get_drvdata(pool->p.dev);

	state->attempts++;
	KUNIT_EXPECT_EQ(test, state->maps, 1);
	KUNIT_EXPECT_EQ(test, page_pool_get_dma_addr(page), state->mapped.addr);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), 1);
	get_page(page); /* Keep the failed allocation observable after allocator put. */
	state->observed = page;
	kunit_deactivate_static_stub(test, page_pool_register_dma_index);
	return -ENOMEM;
}

static void page_pool_dma_index_failure_test(struct kunit *test)
{
	struct page_pool_params params = {
		.nid = NUMA_NO_NODE, .order = 1, /* one page, not a bulk refill */
		.flags = PP_FLAG_DMA_MAP, .dma_dir = DMA_FROM_DEVICE,
	};
	struct page_pool_dma_test_state state = {};
	struct page_pool *pool;
	struct page *page;
	struct device *dev;

	if (!IS_ENABLED(CONFIG_ARCH_HAS_DMA_OPS) ||
	    IS_ENABLED(CONFIG_ARCH_HAS_DMA_MAP_DIRECT) || !PP_DMA_INDEX_BITS)
		kunit_skip(test, "requires DMA map ops and page-pool DMA indices");
	dev = kunit_device_register(test, "pp-index");
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, dev);
	dev_set_drvdata(dev, &state);
	dev->coherent_dma_mask = DMA_BIT_MASK(64);
	dev->dma_mask = &dev->coherent_dma_mask;
	set_dma_ops(dev, &pp_test_dma_ops);
	params.dev = dev;
	pool = page_pool_create(&params);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pool);
	kunit_activate_static_stub(test, page_pool_register_dma_index, pp_test_fail_index);
	page = page_pool_alloc_pages(pool, GFP_KERNEL);
	KUNIT_EXPECT_NULL(test, page);
	if (page)
		page_pool_put_full_page(pool, page, false);
	KUNIT_EXPECT_EQ(test, state.attempts, 1);
	KUNIT_EXPECT_EQ(test, state.maps, 1);
	KUNIT_EXPECT_EQ(test, state.unmaps, 1);
	KUNIT_EXPECT_PTR_EQ(test, state.mapped.dev, dev);
	KUNIT_EXPECT_PTR_EQ(test, state.unmapped.dev, dev);
	KUNIT_EXPECT_EQ(test, state.unmapped.addr, state.mapped.addr);
	KUNIT_EXPECT_EQ(test, state.mapped.size, PAGE_SIZE << params.order);
	KUNIT_EXPECT_EQ(test, state.unmapped.size, state.mapped.size);
	KUNIT_EXPECT_EQ(test, state.mapped.dir, DMA_FROM_DEVICE);
	KUNIT_EXPECT_EQ(test, state.unmapped.dir, state.mapped.dir);
	KUNIT_EXPECT_EQ(test, state.mapped.attrs, DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_WEAK_ORDERING);
	KUNIT_EXPECT_EQ(test, state.unmapped.attrs, state.mapped.attrs);
	KUNIT_EXPECT_TRUE(test, xa_empty(&pool->dma_mapped));
	KUNIT_EXPECT_EQ(test, pool->pages_state_hold_cnt, 0);
	KUNIT_EXPECT_EQ(test, atomic_read(&pool->pages_state_release_cnt), 0);
	KUNIT_EXPECT_EQ(test, pool->alloc.count, 0);
	KUNIT_EXPECT_TRUE(test, ptr_ring_empty(&pool->ring));
	KUNIT_EXPECT_NULL(test, pool->frag_page);
	if (state.observed) {
		KUNIT_EXPECT_EQ(test, page_pool_get_dma_addr(state.observed), 0);
		KUNIT_EXPECT_EQ(test, state.observed->pp_magic & PP_DMA_INDEX_MASK, 0UL);
		KUNIT_EXPECT_EQ(test, page_ref_count(state.observed), 1);
	}

	page = page_pool_alloc_pages(pool, GFP_KERNEL);
	KUNIT_EXPECT_NOT_NULL(test, page);
	if (page) {
		unsigned long index = (page->pp_magic & PP_DMA_INDEX_MASK) >> PP_DMA_INDEX_SHIFT;

		KUNIT_EXPECT_PTR_EQ(test, xa_load(&pool->dma_mapped, index), page);
		page_pool_put_full_page(pool, page, false);
	}
	page_pool_destroy(pool);
	KUNIT_EXPECT_EQ(test, state.maps, 2);
	KUNIT_EXPECT_EQ(test, state.unmaps, 2);
	if (state.observed) {
		KUNIT_EXPECT_EQ(test, page_ref_count(state.observed), 1);
		put_page(state.observed);
	}
}

static void page_pool_return_test(struct kunit *test)
{
	/* Order 1 bypasses bulk allocation, so no unobserved cached pages. */
	struct page_pool_params params = { .nid = NUMA_NO_NODE, .order = 1,
					   .pool_size = 1 };
	unsigned int n = PP_ALLOC_CACHE_SIZE + 2;
	struct page **pages, *page;
	struct page_pool *pool;
	unsigned int i;

	pages = kunit_kcalloc(test, n, sizeof(*pages), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, pages);
	pool = page_pool_create(&params);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pool);
	for (i = 0; i < n; i++) {
		pages[i] = page_pool_alloc_pages(pool, GFP_KERNEL);
		if (!pages[i]) {
			KUNIT_FAIL(test, "allocating page %u", i);
			while (i)
				page_pool_put_full_page(pool, pages[--i], false);
			goto out;
		}
	}

	page = pages[0];
	page_pool_fragment_page(page, 3);
	page_pool_put_full_page(pool, page, false);
	page_pool_put_full_page(pool, page, false);
	KUNIT_EXPECT_EQ(test, atomic_long_read(&page->pp_ref_count), 1L);
	KUNIT_EXPECT_EQ(test, pool->alloc.count, 0);
	KUNIT_EXPECT_TRUE(test, __ptr_ring_empty(&pool->ring));

	/* Only this test accesses the pool; allow direct returns into its cache. */
	for (i = 0; i < PP_ALLOC_CACHE_SIZE; i++)
		page_pool_put_full_page(pool, pages[i], true);
	KUNIT_EXPECT_EQ(test, pool->alloc.count, PP_ALLOC_CACHE_SIZE);
	KUNIT_EXPECT_PTR_EQ(test, pool->alloc.cache[0], page);
	KUNIT_EXPECT_EQ(test, atomic_long_read(&page->pp_ref_count), 1L);

	/* No NAPI or pool CPU is bound to this process-context caller. */
	page_pool_put_full_page(pool, pages[i], false);
	KUNIT_EXPECT_PTR_EQ(test, __ptr_ring_peek(&pool->ring), pages[i]);
	page_pool_put_full_page(pool, pages[i + 1], true);
	KUNIT_EXPECT_EQ(test, atomic_read(&pool->pages_state_release_cnt), 1);

	/* Flush the cache and make the ring page eligible even on NUMA. */
	page_pool_update_nid(pool, page_to_nid(pages[i]));
	page = page_pool_alloc_pages(pool, GFP_KERNEL);
	KUNIT_EXPECT_PTR_EQ(test, page, pages[i]);
	if (!page)
		goto out;
	get_page(page);
	page_pool_put_full_page(pool, page, false);
	KUNIT_EXPECT_FALSE(test, page_pool_page_is_pp(page));
	KUNIT_EXPECT_NULL(test, page->pp);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), 1);
	KUNIT_EXPECT_EQ(test, atomic_read(&pool->pages_state_release_cnt), n);
	KUNIT_EXPECT_EQ(test, pool->pages_state_hold_cnt, n);
	put_page(page);
out:
	page_pool_destroy(pool);
}

static void page_pool_coalesce_test(struct kunit *test)
{
	struct page_pool_params params = { .nid = NUMA_NO_NODE };
	struct sk_buff *from, *to, *clone = NULL;
	struct page *pooled, *ordinary;
	struct page_pool *pool;
	bool stolen;
	int delta;

	pool = page_pool_create(&params);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pool);
	pooled = page_pool_alloc_pages(pool, GFP_KERNEL);
	ordinary = alloc_page(GFP_KERNEL);
	from = alloc_skb(0, GFP_KERNEL);
	to = alloc_skb(0, GFP_KERNEL);
	if (!pooled || !ordinary || !from || !to) {
		KUNIT_FAIL(test, "allocating coalesce inputs");
		goto out;
	}
	skb_mark_for_recycle(from);
	skb_mark_for_recycle(to);
	skb_put(to, skb_tailroom(to)); /* force the shared-fragment path */
	page_pool_ref_page(pooled);
	get_page(ordinary);
	skb_add_rx_frag(from, 0, pooled, 0, 64, PAGE_SIZE);
	skb_add_rx_frag(from, 1, ordinary, 0, 64, PAGE_SIZE);
	clone = skb_clone(from, GFP_KERNEL);
	if (!clone) {
		KUNIT_FAIL(test, "cloning source skb");
		goto out;
	}
	KUNIT_EXPECT_TRUE(test, skb_try_coalesce(to, from, &stolen, &delta));
	KUNIT_EXPECT_FALSE(test, stolen);
	KUNIT_EXPECT_EQ(test, skb_shinfo(to)->nr_frags, 2);
	KUNIT_EXPECT_EQ(test, atomic_long_read(&pooled->pp_ref_count), 3L);
	KUNIT_EXPECT_EQ(test, page_ref_count(pooled), 1);
	KUNIT_EXPECT_EQ(test, page_ref_count(ordinary), 3);
out:
	consume_skb(to);
	consume_skb(clone);
	consume_skb(from);
	if (ordinary) {
		KUNIT_EXPECT_EQ(test, page_ref_count(ordinary), 1);
		put_page(ordinary);
	}
	if (pooled) {
		KUNIT_EXPECT_EQ(test, atomic_long_read(&pooled->pp_ref_count), 1L);
		page_pool_put_full_page(pool, pooled, false);
	}
	page_pool_destroy(pool);
}

static void page_pool_xdp_bulk_test(struct kunit *test)
{
	struct page_pool_params params = { .nid = NUMA_NO_NODE, .order = 1 };
	struct page_pool *pool_a, *pool_b;
	struct xdp_frame *frame, *other;
	struct skb_shared_info *sinfo;
	struct xdp_frame_bulk bq;
	struct page *a, *b;
	int i, remaining;

	pool_a = page_pool_create(&params);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pool_a);
	pool_b = page_pool_create(&params);
	if (IS_ERR(pool_b)) {
		KUNIT_FAIL(test, "creating second page pool");
		goto destroy_a;
	}
	a = page_pool_alloc_pages(pool_a, GFP_KERNEL);
	b = page_pool_alloc_pages(pool_b, GFP_KERNEL);
	if (!a || !b) {
		KUNIT_FAIL(test, "allocating compound pages");
		if (a)
			page_pool_put_full_page(pool_a, a, false);
		if (b)
			page_pool_put_full_page(pool_b, b, false);
		goto destroy_b;
	}

	frame = page_address(a);
	*frame = (struct xdp_frame) { .data = frame + 1, .frame_sz = PAGE_SIZE,
		.mem_type = MEM_TYPE_PAGE_POOL, .flags = XDP_FLAGS_HAS_FRAGS };
	sinfo = xdp_get_shared_info_from_frame(frame);
	sinfo->nr_frags = 3;
	skb_frag_fill_page_desc(&sinfo->frags[0], b + 1, 0, 64);
	skb_frag_fill_page_desc(&sinfo->frags[1], a + 1, 0, 64);
	skb_frag_fill_page_desc(&sinfo->frags[2], b + 1, 64, 64);
	other = page_address(b);
	*other = (struct xdp_frame) { .data = other + 1, .frame_sz = PAGE_SIZE,
		.mem_type = MEM_TYPE_PAGE_POOL };
	page_pool_fragment_page(a, 2);
	page_pool_fragment_page(b, XDP_BULK_QUEUE_SIZE + 1);

	xdp_frame_bulk_init(&bq);
	rcu_read_lock();
	for (i = 0; i < XDP_BULK_QUEUE_SIZE - 1; i++)
		xdp_return_frame_bulk(other, &bq);
	/* The first tail fills the queue. Both pages still have references. */
	xdp_return_frame_bulk(frame, &bq);
	remaining = bq.count;
	xdp_flush_frame_bulk(&bq);
	rcu_read_unlock();
	KUNIT_EXPECT_EQ(test, remaining, 3);
	KUNIT_EXPECT_EQ(test, bq.count, 0);
	KUNIT_EXPECT_PTR_EQ(test, __ptr_ring_peek(&pool_a->ring), a);
	KUNIT_EXPECT_PTR_EQ(test, __ptr_ring_peek(&pool_b->ring), b);
	KUNIT_EXPECT_EQ(test, atomic_long_read(&a->pp_ref_count), 1L);
	KUNIT_EXPECT_EQ(test, atomic_long_read(&b->pp_ref_count), 1L);
destroy_b:
	page_pool_destroy(pool_b);
destroy_a:
	page_pool_destroy(pool_a);
}

static void skb_frag_ref_test_pp_page(struct kunit *test)
{
	struct page_pool_params pp_params = {
		.order = 0,
		.pool_size = 4,
		.nid = NUMA_NO_NODE,
	};
	struct page_pool *pool;
	struct sk_buff *skb, *copy, *clone, *split;
	struct page *page;
	long base_pp_ref;
	int base_ref;

	pool = page_pool_create(&pp_params);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pool);

	page = page_pool_dev_alloc_pages(pool);
	KUNIT_ASSERT_NOT_NULL(test, page);
	base_ref = page_ref_count(page);
	base_pp_ref = atomic_long_read(&page->pp_ref_count);

	/* Layer 1: get_page/put_page always use non-pp refcount */
	get_page(page);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref + 1);
	KUNIT_EXPECT_EQ(test, atomic_long_read(&page->pp_ref_count),
			base_pp_ref);
	put_page(page);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref);

	/* Layer 2: napi_pp_get/put_page use pp_ref_count */
	KUNIT_EXPECT_TRUE(test, napi_pp_get_page(page));
	KUNIT_EXPECT_EQ(test, atomic_long_read(&page->pp_ref_count),
			base_pp_ref + 1);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref);
	KUNIT_EXPECT_TRUE(test, napi_pp_put_page(page));
	KUNIT_EXPECT_EQ(test, atomic_long_read(&page->pp_ref_count),
			base_pp_ref);

	/* Layer 3: skb_page_ref / unref obey recycle flag */
	skb_page_ref(page, false);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref + 1);
	KUNIT_EXPECT_EQ(test, atomic_long_read(&page->pp_ref_count),
			base_pp_ref);
	skb_page_unref(page, false);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref);

	skb_page_ref(page, true);
	KUNIT_EXPECT_EQ(test, atomic_long_read(&page->pp_ref_count),
			base_pp_ref + 1);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref);
	skb_page_unref(page, true);
	KUNIT_EXPECT_EQ(test, atomic_long_read(&page->pp_ref_count),
			base_pp_ref);

	/* Layer 4: skb_frag_ref / skb_frag_unref match on pp_recycle=1 & 0 */
	skb = alloc_skb(64, GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, skb);
	skb_fill_page_desc(skb, 0, page, 0, 64);
	skb->len = 64;
	skb->data_len = 64;

	skb->pp_recycle = 1;
	skb_frag_ref(skb, 0);
	KUNIT_EXPECT_EQ(test, atomic_long_read(&page->pp_ref_count),
			base_pp_ref + 1);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref);
	skb_frag_unref(skb, 0);
	KUNIT_EXPECT_EQ(test, atomic_long_read(&page->pp_ref_count),
			base_pp_ref);

	/* pskb_copy on pp_recycle=1 skb propagates pp_recycle & pp_ref_count */
	copy = pskb_copy(skb, GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, copy);
	KUNIT_EXPECT_TRUE(test, copy->pp_recycle);
	KUNIT_EXPECT_EQ(test, atomic_long_read(&page->pp_ref_count),
			base_pp_ref + 1);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref);
	consume_skb(copy);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref);
	KUNIT_EXPECT_EQ(test, atomic_long_read(&page->pp_ref_count),
			base_pp_ref);


	/* skb_clone + pskb_expand_head preserves pp_recycle and pp_ref_count */
	clone = skb_clone(skb, GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, clone);
	KUNIT_ASSERT_EQ(test, pskb_expand_head(clone, 32, 0, GFP_KERNEL), 0);
	KUNIT_EXPECT_TRUE(test, clone->pp_recycle);
	KUNIT_EXPECT_EQ(test, atomic_long_read(&page->pp_ref_count),
			base_pp_ref + 1);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref);
	consume_skb(clone);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref);
	KUNIT_EXPECT_EQ(test, atomic_long_read(&page->pp_ref_count),
			base_pp_ref);

	/* skb_split inside linear header & inside nonlinear page_pool frags */
	skb_shinfo(skb)->nr_frags = 0;
	skb->len = 0;
	skb->data_len = 0;
	skb_reset_tail_pointer(skb);
	skb_put(skb, 32);
	skb_fill_page_desc(skb, 0, page, 0, 64);
	skb->len += 64;
	skb->data_len = 64;
	split = alloc_skb(32, GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, split);
	skb_split(skb, split, 16);
	KUNIT_EXPECT_TRUE(test, split->pp_recycle);
	KUNIT_EXPECT_EQ(test, atomic_long_read(&page->pp_ref_count),
			base_pp_ref);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref);
	skb_shinfo(split)->nr_frags = 0;
	consume_skb(split);
	skb->len = 0;
	skb->data_len = 0;
	skb_reset_tail_pointer(skb);
	skb_fill_page_desc(skb, 0, page, 0, 64);
	skb->len = 64;
	skb->data_len = 64;

	split = alloc_skb(0, GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, split);
	skb_split(skb, split, 32);
	KUNIT_EXPECT_TRUE(test, split->pp_recycle);
	KUNIT_EXPECT_EQ(test, atomic_long_read(&page->pp_ref_count),
			base_pp_ref + 1);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref);
	consume_skb(split);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref);
	KUNIT_EXPECT_EQ(test, atomic_long_read(&page->pp_ref_count),
			base_pp_ref);

	/* skb_segment on pp_recycle=1 skb propagates pp_recycle to segments */
	skb->protocol = htons(ETH_P_IP);
	skb_reset_mac_header(skb);
	skb_reset_network_header(skb);
	skb_shinfo(skb)->gso_size = 16;
	split = skb_segment(skb, NETIF_F_SG | NETIF_F_HW_CSUM);
	KUNIT_ASSERT_FALSE(test, IS_ERR_OR_NULL(split));
	KUNIT_EXPECT_TRUE(test, split->pp_recycle);
	KUNIT_EXPECT_EQ(test, atomic_long_read(&page->pp_ref_count),
			base_pp_ref + 2);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref);
	kfree_skb_list(split);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref);
	KUNIT_EXPECT_EQ(test, atomic_long_read(&page->pp_ref_count),
			base_pp_ref);

	/* skb_shift balances pp_ref_count across split, partial coalesce,
	 * multi-frag coalesce + partial next-frag shift, and full merge.
	 */
	skb_fill_page_desc(skb, 0, page, 0, 32);
	skb_fill_page_desc(skb, 1, page, 64, 32);
	page_pool_ref_page(page);
	skb->len = 64;
	skb->data_len = 64;
	split = alloc_skb(0, GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, split);
	split->pp_recycle = 1;
	KUNIT_EXPECT_EQ(test, skb_shift(split, skb, 16), 16);
	KUNIT_EXPECT_EQ(test, atomic_long_read(&page->pp_ref_count),
			base_pp_ref + 2);
	KUNIT_EXPECT_EQ(test, skb_shift(split, skb, 8), 8);
	KUNIT_EXPECT_EQ(test, atomic_long_read(&page->pp_ref_count),
			base_pp_ref + 2);
	KUNIT_EXPECT_EQ(test, skb_shift(split, skb, 24), 24);
	KUNIT_EXPECT_EQ(test, atomic_long_read(&page->pp_ref_count),
			base_pp_ref + 2);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref);
	KUNIT_EXPECT_EQ(test, skb_shift(split, skb, 16), 16);
	KUNIT_EXPECT_EQ(test, atomic_long_read(&page->pp_ref_count),
			base_pp_ref + 1);
	skb_shinfo(split)->nr_frags = 0;
	consume_skb(split);
	page_pool_put_full_page(pool, page, false);
	skb_fill_page_desc(skb, 0, page, 0, 64);
	skb->len = 64;
	skb->data_len = 64;

	skb->pp_recycle = 0;
	skb_frag_ref(skb, 0);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref + 1);
	KUNIT_EXPECT_EQ(test, atomic_long_read(&page->pp_ref_count),
			base_pp_ref);
	skb_frag_unref(skb, 0);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref);

	skb_shinfo(skb)->nr_frags = 0;
	consume_skb(skb);
	page_pool_put_full_page(pool, page, false);
	page_pool_destroy(pool);

	/* Order-1 compound page: tail page resolves to the pool head. */
	pp_params.order = 1;
	pool = page_pool_create(&pp_params);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pool);
	page = page_pool_dev_alloc_pages(pool);
	KUNIT_ASSERT_NOT_NULL(test, page);
	base_ref = page_ref_count(page);
	base_pp_ref = atomic_long_read(&page->pp_ref_count);

	KUNIT_EXPECT_TRUE(test, napi_pp_get_page(page + 1));
	KUNIT_EXPECT_EQ(test, atomic_long_read(&page->pp_ref_count),
			base_pp_ref + 1);
	KUNIT_EXPECT_EQ(test, page_ref_count(page), base_ref);
	KUNIT_EXPECT_TRUE(test, napi_pp_put_page(page + 1));
	KUNIT_EXPECT_EQ(test, atomic_long_read(&page->pp_ref_count),
			base_pp_ref);

	page_pool_put_full_page(pool, page, false);
	page_pool_destroy(pool);
}
#endif


/* Can @nr pages of @order be allocated and held at the same time? */
static bool alloc_skb_with_frags_test_have_pages(unsigned int nr,
						 unsigned int order)
{
	struct page *pages[MAX_SKB_FRAGS];
	unsigned int i, n;

	for (n = 0; n < nr; n++) {
		pages[n] = alloc_pages(GFP_KERNEL | __GFP_NOWARN |
				       __GFP_NORETRY, order);
		if (!pages[n])
			break;
	}
	for (i = 0; i < n; i++)
		__free_pages(pages[i], order);

	return n == nr;
}

static void alloc_skb_with_frags_test_tail(struct kunit *test)
{
	/* MAX_SKB_FRAGS - 2 full chunks plus a 7 page tail: rounding the
	 * tail down to 4 + 2 + 1 pages would need one frag too many.
	 */
	unsigned long len = (MAX_SKB_FRAGS - 2) *
			    (PAGE_SIZE << PAGE_ALLOC_COSTLY_ORDER) +
			    7 * PAGE_SIZE;
	unsigned long frags_len = 0;
	struct sk_buff *skb;
	int err, i;

	skb = alloc_skb_with_frags(0, len, PAGE_ALLOC_COSTLY_ORDER, &err,
				   GFP_KERNEL);
	/* The full chunks can't fall back to smaller pages here, so the
	 * call needs up to MAX_SKB_FRAGS - 1 order-3 pages at once. Don't
	 * report a lack of them as a failure.
	 */
	if (!skb && !alloc_skb_with_frags_test_have_pages(MAX_SKB_FRAGS - 1,
							  PAGE_ALLOC_COSTLY_ORDER))
		kunit_skip(test, "not enough order-%d pages",
			   PAGE_ALLOC_COSTLY_ORDER);
	KUNIT_ASSERT_NOT_NULL_MSG(test, skb, "err %d", err);

	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++)
		frags_len += skb_frag_size(&skb_shinfo(skb)->frags[i]);
	KUNIT_EXPECT_EQ(test, frags_len, len);

	kfree_skb(skb);
}

static struct kunit_case net_test_cases[] = {
	KUNIT_CASE(alloc_skb_with_frags_test_tail),
	KUNIT_CASE_PARAM(gso_test_func, gso_test_gen_params),
	KUNIT_CASE_PARAM(ip_tunnel_flags_test_run,
			 ip_tunnel_flags_test_gen_params),
	KUNIT_CASE(skb_managed_frag_test),
	KUNIT_CASE(skb_frag_ref_test_page),
#ifdef CONFIG_PAGE_POOL
	KUNIT_CASE(page_pool_alloc_test),
	KUNIT_CASE(page_pool_return_test),
	KUNIT_CASE(page_pool_dma_test),
	KUNIT_CASE(page_pool_dma_index_failure_test),
	KUNIT_CASE(page_pool_skb_return_sync_test),
	KUNIT_CASE(page_pool_coalesce_test),
	KUNIT_CASE(page_pool_xdp_bulk_test),
	KUNIT_CASE(skb_frag_ref_test_pp_page),
#endif
	{ },
};

static struct kunit_suite net_test_suite = {
	.name		= "net_core",
	.test_cases	= net_test_cases,
};
kunit_test_suite(net_test_suite);

MODULE_DESCRIPTION("KUnit tests for networking core");
MODULE_IMPORT_NS("EXPORTED_FOR_KUNIT_TESTING");
MODULE_LICENSE("GPL");
