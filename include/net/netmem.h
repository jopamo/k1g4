/* SPDX-License-Identifier: GPL-2.0
 *
 *	Network memory
 *
 *	Author:	Mina Almasry <almasrymina@google.com>
 */

#ifndef _NET_NETMEM_H
#define _NET_NETMEM_H

#include <linux/dma-mapping.h>
#include <linux/mm.h>
#include <net/net_debug.h>

/* These fields in struct page are used by the page_pool and net stack:
 *
 *        struct {
 *                unsigned long pp_magic;
 *                struct page_pool *pp;
 *                unsigned long _pp_mapping_pad;
 *                unsigned long dma_addr;
 *                atomic_long_t pp_ref_count;
 *        };
 *
 * This descriptor overlays the page_pool fields in struct page.
 */
struct netmem_desc {
	unsigned long _flags;
	unsigned long pp_magic;
	struct page_pool *pp;
	unsigned long _pp_mapping_pad;
	unsigned long dma_addr;
	atomic_long_t pp_ref_count;
};

#define NETMEM_DESC_ASSERT_OFFSET(pg, desc)        \
	static_assert(offsetof(struct page, pg) == \
		      offsetof(struct netmem_desc, desc))
NETMEM_DESC_ASSERT_OFFSET(flags, _flags);
NETMEM_DESC_ASSERT_OFFSET(pp_magic, pp_magic);
NETMEM_DESC_ASSERT_OFFSET(pp, pp);
NETMEM_DESC_ASSERT_OFFSET(_pp_mapping_pad, _pp_mapping_pad);
NETMEM_DESC_ASSERT_OFFSET(dma_addr, dma_addr);
NETMEM_DESC_ASSERT_OFFSET(pp_ref_count, pp_ref_count);
#undef NETMEM_DESC_ASSERT_OFFSET

/*
 * Since struct netmem_desc uses the space in struct page, the size
 * should be checked, until struct netmem_desc has its own instance from
 * slab, to avoid conflicting with other members within struct page.
 */
static_assert(sizeof(struct netmem_desc) <= offsetof(struct page, _refcount));

/* netmem_ref always refers to a struct page. */
typedef unsigned long __bitwise netmem_ref;

static inline struct page *__netmem_to_page(netmem_ref netmem)
{
	return (__force struct page *)netmem;
}

static inline struct page *netmem_to_page(netmem_ref netmem)
{
	return __netmem_to_page(netmem);
}

#define page_to_netmem(p)	(_Generic((p),			\
	const struct page * :	(__force const netmem_ref)(p),	\
	struct page * :		(__force netmem_ref)(p)))

/**
 * virt_to_netmem - convert virtual memory pointer to a netmem reference
 * @data: host memory pointer to convert
 *
 * Return: netmem reference to the &page backing this virtual address.
 */
static inline netmem_ref virt_to_netmem(const void *data)
{
	return page_to_netmem(virt_to_page(data));
}

static inline int netmem_ref_count(netmem_ref netmem)
{
	return page_ref_count(netmem_to_page(netmem));
}

static inline unsigned long netmem_pfn_trace(netmem_ref netmem)
{
	return page_to_pfn(netmem_to_page(netmem));
}

/* XXX: How to extract netmem_desc from page must be changed, once
 * netmem_desc no longer overlays on page and will be allocated through
 * slab.
 */
#define __pp_page_to_nmdesc(p)	(_Generic((p),				\
	const struct page * :	(const struct netmem_desc *)(p),	\
	struct page * :		(struct netmem_desc *)(p)))

/* CAUTION: Check if the page is a pp page before calling this helper or
 * know it's a pp page.
 */
#define pp_page_to_nmdesc(p)						\
({									\
	DEBUG_NET_WARN_ON_ONCE(!page_pool_page_is_pp(p));		\
	__pp_page_to_nmdesc(p);						\
})

/* Access the page-pool descriptor overlaid on struct page. */
static inline struct netmem_desc *__netmem_to_nmdesc(netmem_ref netmem)
{
	return (__force struct netmem_desc *)netmem;
}

static inline struct netmem_desc *netmem_to_nmdesc(netmem_ref netmem)
{
	return __pp_page_to_nmdesc(netmem_to_page(netmem));
}

static inline struct page_pool *__netmem_get_pp(netmem_ref netmem)
{
	return __netmem_to_nmdesc(netmem)->pp;
}

static inline struct page_pool *netmem_get_pp(netmem_ref netmem)
{
	return netmem_to_nmdesc(netmem)->pp;
}

static inline atomic_long_t *netmem_get_pp_ref_count_ref(netmem_ref netmem)
{
	return &netmem_to_nmdesc(netmem)->pp_ref_count;
}

static inline bool netmem_is_pref_nid(netmem_ref netmem, int pref_nid)
{
	return page_to_nid(netmem_to_page(netmem)) == pref_nid;
}

static inline netmem_ref netmem_compound_head(netmem_ref netmem)
{
	return page_to_netmem(compound_head(netmem_to_page(netmem)));
}

static inline void *__netmem_address(netmem_ref netmem)
{
	return page_address(__netmem_to_page(netmem));
}

static inline void *netmem_address(netmem_ref netmem)
{
	return __netmem_address(netmem);
}

/**
 * netmem_is_pfmemalloc - check if @netmem was allocated under memory pressure
 * @netmem: netmem reference to check
 *
 * Return: true if @netmem is page-backed and the page was allocated under
 * memory pressure, false otherwise.
 */
static inline bool netmem_is_pfmemalloc(netmem_ref netmem)
{
	return page_is_pfmemalloc(netmem_to_page(netmem));
}

#define NETMEM_32BIT_ARCH_WITH_64BIT_DMA	\
	(sizeof(dma_addr_t) > sizeof(unsigned long))

static inline unsigned long netmem_dma_addr_encode(dma_addr_t addr)
{
	if (NETMEM_32BIT_ARCH_WITH_64BIT_DMA)
		addr >>= PAGE_SHIFT;

	return addr;
}

static inline dma_addr_t netmem_dma_addr_decode(unsigned long addr)
{
	if (NETMEM_32BIT_ARCH_WITH_64BIT_DMA)
		return (dma_addr_t)addr << PAGE_SHIFT;

	return addr;
}

static inline bool netmem_dma_addr_fits(dma_addr_t addr)
{
	/* We assume page alignment to shave off bottom bits,
	 * if this "compression" doesn't work we need to drop.
	 */
	return addr == netmem_dma_addr_decode(netmem_dma_addr_encode(addr));
}

static inline unsigned long netmem_get_dma_addr(netmem_ref netmem)
{
	return netmem_to_nmdesc(netmem)->dma_addr;
}

/* Acquire a page reference, independently of page-pool fragment references. */
static __always_inline void netmem_inc_ref(netmem_ref netmem)
{
	get_page(netmem_to_page(netmem));
}

/* Release the reference acquired by netmem_inc_ref(). */
static __always_inline void netmem_dec_ref(netmem_ref netmem)
{
	put_page(netmem_to_page(netmem));
}

#define netmem_dma_unmap_addr_set(NETMEM, PTR, ADDR_NAME, VAL)	\
	do {							\
		(void)(NETMEM);					\
		dma_unmap_addr_set(PTR, ADDR_NAME, VAL);		\
	} while (0)

static inline void netmem_dma_unmap_page_attrs(struct device *dev,
					       dma_addr_t addr, size_t size,
					       enum dma_data_direction dir,
					       unsigned long attrs)
{
	if (!addr)
		return;

	dma_unmap_page_attrs(dev, addr, size, dir, attrs);
}

#endif /* _NET_NETMEM_H */
