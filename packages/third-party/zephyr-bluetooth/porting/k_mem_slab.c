/*
 * Copyright (c) 2010-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aic_core.h>
#include <zephyr/kernel.h>

#include <zephyr/k_mem_slab.h>

static int create_free_list(struct k_mem_slab *slab)
{
	uint32_t j;
	char *p;

	/* blocks must be word aligned */
	if(((slab->block_size | (uintptr_t)slab->buffer) &
				(sizeof(void *) - 1)) != 0U) {
		return -EINVAL;
	}

	slab->free_list = NULL;
	p = slab->buffer;

	for (j = 0U; j < slab->num_blocks; j++) {
		*(char **)p = slab->free_list;
		slab->free_list = p;
		p += slab->block_size;
	}
	return 0;
}

int k_mem_slab_init(struct k_mem_slab *slab, void *buffer,
		    size_t block_size, uint32_t num_blocks)
{
	int rc = 0;

	slab->num_blocks = num_blocks;
	slab->block_size = block_size;
	slab->buffer = buffer;
	slab->num_used = 0U;

	rc = create_free_list(slab);
	if (rc < 0) {
		goto out;
	}

out:

	return rc;
}

int k_mem_slab_alloc(struct k_mem_slab *slab, void **mem,
			    k_timeout_t timeout)
{
	k_spinlock_key_t key = k_spin_lock(&slab->lock);
	int result;

	if (slab->free_list != NULL) {
		/* take a free block */
		*mem = slab->free_list;
		slab->free_list = *(char **)(slab->free_list);
		slab->num_used++;

		result = 0;
	} else {
        *mem = NULL;
        result = -ENOMEM;
    }

	k_spin_unlock(&slab->lock, key);

	return result;

}

void k_mem_slab_free(struct k_mem_slab *slab, void *mem)
{
	k_spinlock_key_t key = k_spin_lock(&slab->lock);

	**(char ***) mem = slab->free_list;
	slab->free_list = *(char **) mem;
	slab->num_used--;

	k_spin_unlock(&slab->lock, key);
}
