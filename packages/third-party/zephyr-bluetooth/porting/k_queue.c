/*
 * Copyright (c) 2010-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aic_core.h>
#include <zephyr/kernel.h>

#include <zephyr/k_queue.h>

void k_queue_init(struct k_queue *queue)
{
	sys_sflist_init(&queue->data_q);
	queue->lock = (struct k_spinlock) {};
	z_waitq_init(&queue->wait_q);
#if defined(CONFIG_POLL)
	sys_dlist_init(&queue->poll_events);
#endif
}

int k_queue_is_empty(struct k_queue *queue)
{
	return sys_sflist_is_empty(&queue->data_q) ? 1 : 0;
}

static int32_t queue_insert(struct k_queue *queue, void *prev, void *data,
			    bool alloc, bool is_append)
{
	k_spinlock_key_t key = k_spin_lock(&queue->lock);
	int32_t result = 0;
	bool resched = false;

	if (is_append) {
		prev = sys_sflist_peek_tail(&queue->data_q);
	}

	/* Only need to actually allocate if no threads are pending */
	//if (alloc) {
	//	struct alloc_node *anode;

	//	anode = z_thread_malloc(sizeof(*anode));
	//	if (anode == NULL) {
	//		result = -ENOMEM;
	//		goto out;
	//	}
	//	anode->data = data;
	//	sys_sfnode_init(&anode->node, 0x1);
	//	data = anode;
	//} else {
	//	sys_sfnode_init(data, 0x0);
	//}

	sys_sfnode_init(data, 0x0);

	sys_sflist_insert(&queue->data_q, prev, data);
	//resched = handle_poll_events(queue, K_POLL_STATE_DATA_AVAILABLE);

out:
	//if (resched) {
	//	z_reschedule(&queue->lock, key);
	//} else {
	//	k_spin_unlock(&queue->lock, key);
	//}
	k_spin_unlock(&queue->lock, key);

	return result;
}

void k_queue_insert(struct k_queue *queue, void *prev, void *data)
{
	(void)queue_insert(queue, prev, data, false, false);
}

void k_queue_append(struct k_queue *queue, void *data)
{
	(void)queue_insert(queue, NULL, data, false, true);
}

void k_queue_prepend(struct k_queue *queue, void *data)
{
	(void)queue_insert(queue, NULL, data, false, false);
}

void *queue_node_peek(sys_sfnode_t *node, bool needs_free)
{
	//void *ret;

	//if ((node != NULL) && (sys_sfnode_flags_get(node) != (uint8_t)0)) {
	//	/* If the flag is set, then the enqueue operation for this item
	//	 * did a behind-the scenes memory allocation of an alloc_node
	//	 * struct, which is what got put in the queue. Free it and pass
	//	 * back the data pointer.
	//	 */
	//	struct alloc_node *anode;

	//	anode = CONTAINER_OF(node, struct alloc_node, node);
	//	ret = anode->data;
	//	if (needs_free) {
	//		k_free(anode);
	//	}
	//} else {
	//	/* Data was directly placed in the queue, the first word
	//	 * reserved for the linked list. User mode isn't allowed to
	//	 * do this, although it can get data sent this way.
	//	 */
	//	ret = (void *)node;
	//}

	//return ret;
    return (void *)node;
}

void *k_queue_get(struct k_queue *queue, k_timeout_t timeout)
{
	k_spinlock_key_t key = k_spin_lock(&queue->lock);
	void *data;

	if (likely(!sys_sflist_is_empty(&queue->data_q))) {
		sys_sfnode_t *node;

		node = sys_sflist_get_not_empty(&queue->data_q);
		data = queue_node_peek(node, true);
		k_spin_unlock(&queue->lock, key);

		return data;
	}

	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		k_spin_unlock(&queue->lock, key);

		return NULL;
	}

	k_spin_unlock(&queue->lock, key);
    u64 start_us = aic_get_time_us();
    do {
	    key = k_spin_lock(&queue->lock);
        if (likely(!sys_sflist_is_empty(&queue->data_q))) {
		    sys_sfnode_t *node;

		    node = sys_sflist_get_not_empty(&queue->data_q);
		    data = queue_node_peek(node, true);
		    k_spin_unlock(&queue->lock, key);

		    return data;
        }
	    k_spin_unlock(&queue->lock, key);
    } while ((aic_get_time_us() - start_us) < timeout);

    return NULL;
	//int ret = z_pend_curr(&queue->lock, key, &queue->wait_q, timeout);

	//return (ret != 0) ? NULL : _current->base.swap_data;
}

void *k_queue_peek_head(struct k_queue *queue)
{
	return queue_node_peek(sys_sflist_peek_head(&queue->data_q), false);
}
