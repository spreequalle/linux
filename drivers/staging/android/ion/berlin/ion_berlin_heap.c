/*
 * Copyright 2012 Marvell International Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/spinlock.h>

#include <linux/err.h>
#include <linux/genalloc.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include "../ion.h"
#include "../ion_priv.h"

struct ion_berlin_heap {
	struct ion_heap heap;
	struct gen_pool *pool;
	ion_phys_addr_t base;
};

ion_phys_addr_t ion_berlin_allocate(struct ion_heap *heap,
				      unsigned long size,
				      unsigned long align)
{
	struct ion_berlin_heap *berlin_heap =
		container_of(heap, struct ion_berlin_heap, heap);
	unsigned long offset = gen_pool_alloc(berlin_heap->pool, size);

	if (!offset)
		return ION_CARVEOUT_ALLOCATE_FAIL;

	return offset;
}

void ion_berlin_free(struct ion_heap *heap, ion_phys_addr_t addr,
		       unsigned long size)
{
	struct ion_berlin_heap *berlin_heap =
		container_of(heap, struct ion_berlin_heap, heap);

	if (addr == ION_CARVEOUT_ALLOCATE_FAIL)
		return;
	gen_pool_free(berlin_heap->pool, addr, size);
}

static int ion_berlin_heap_phys(struct ion_heap *heap,
				  struct ion_buffer *buffer,
				  ion_phys_addr_t *addr, size_t *len)
{
	*addr = buffer->priv_phys;
	*len = buffer->size;
	return 0;
}

static int ion_berlin_heap_allocate(struct ion_heap *heap,
				      struct ion_buffer *buffer,
				      unsigned long size, unsigned long align,
				      unsigned long flags)
{
	if (align > PAGE_SIZE)
		return -EINVAL;

	buffer->priv_phys = ion_berlin_allocate(heap, size, align);
	return buffer->priv_phys == ION_CARVEOUT_ALLOCATE_FAIL ? -ENOMEM : 0;
}

static void ion_berlin_heap_free(struct ion_buffer *buffer)
{
	struct ion_heap *heap = buffer->heap;

	ion_berlin_free(heap, buffer->priv_phys, buffer->size);
	buffer->priv_phys = ION_CARVEOUT_ALLOCATE_FAIL;
}

static struct sg_table *ion_berlin_heap_map_dma(struct ion_heap *heap,
						  struct ion_buffer *buffer)
{
	struct sg_table *table;
	int ret;

	table = kzalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!table)
		return ERR_PTR(-ENOMEM);
	ret = sg_alloc_table(table, 1, GFP_KERNEL);
	if (ret) {
		kfree(table);
		return ERR_PTR(ret);
	}
	sg_set_page(table->sgl, pfn_to_page(PFN_DOWN(buffer->priv_phys)),
			buffer->size, 0);
	return table;
}

static void ion_berlin_heap_unmap_dma(struct ion_heap *heap,
					struct ion_buffer *buffer)
{
	sg_free_table(buffer->sg_table);
	kfree(buffer->sg_table);
}

static struct ion_heap_ops berlin_heap_ops = {
	.allocate = ion_berlin_heap_allocate,
	.free = ion_berlin_heap_free,
	.phys = ion_berlin_heap_phys,
	.map_dma = ion_berlin_heap_map_dma,
	.unmap_dma = ion_berlin_heap_unmap_dma,
	.map_user = ion_heap_map_user,
	.map_kernel = ion_heap_map_kernel,
	.unmap_kernel = ion_heap_unmap_kernel,
};

struct ion_heap *ion_berlin_heap_create(struct ion_platform_heap *heap_data)
{
	struct ion_berlin_heap *berlin_heap;

	berlin_heap = kzalloc(sizeof(struct ion_berlin_heap), GFP_KERNEL);
	if (!berlin_heap)
		return ERR_PTR(-ENOMEM);

	berlin_heap->pool = gen_pool_create(12, -1);
	if (!berlin_heap->pool) {
		kfree(berlin_heap);
		return ERR_PTR(-ENOMEM);
	}
	if (heap_data->is_best_fit)
		gen_pool_set_algo(berlin_heap->pool, gen_pool_best_fit, NULL);
	berlin_heap->base = heap_data->base;
	gen_pool_add(berlin_heap->pool, berlin_heap->base, heap_data->size,
		     -1);
	berlin_heap->heap.ops = &berlin_heap_ops;
	berlin_heap->heap.type = ION_HEAP_TYPE_BERLIN;
	if (heap_data->is_defer_free)
		berlin_heap->heap.flags = ION_HEAP_FLAG_DEFER_FREE;

	return &berlin_heap->heap;
}

void ion_berlin_heap_destroy(struct ion_heap *heap)
{
	struct ion_berlin_heap *berlin_heap =
	     container_of(heap, struct  ion_berlin_heap, heap);

	gen_pool_destroy(berlin_heap->pool);
	kfree(berlin_heap);
	berlin_heap = NULL;
}
