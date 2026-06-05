/*
 * Copyright (c) 2010-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aic_core.h>
#include <zephyr/kernel.h>
#include <zephyr/k_work_q.h>

/* Global system work queue */
struct k_work_q k_sys_work_q = {0};

/* Spinlock to protect work queue state */
static struct k_spinlock work_lock;

/* Helper macros for flag manipulation */
#define FLAG_SET(flagp, bit)    (*(flagp) |= BIT(bit))
#define FLAG_CLEAR(flagp, bit)  (*(flagp) &= ~BIT(bit))
#define FLAG_TEST(flagp, bit)   ((*(flagp) & BIT(bit)) != 0U)
#define FLAG_TEST_AND_CLEAR(flagp, bit) \
    ({ bool ret = FLAG_TEST(flagp, bit); FLAG_CLEAR(flagp, bit); ret; })


/* Basic work operations */
void k_work_init(struct k_work *work, k_work_handler_t handler)
{
	__ASSERT_NO_MSG(work != NULL);
	__ASSERT_NO_MSG(handler != NULL);

	work->handler = handler;
	work->queue = NULL;
	work->flags = 0;
	/* Initialize the node's next pointer to NULL */
	work->node.next = NULL;
}

int k_work_submit_to_queue(struct k_work_q *queue, struct k_work *work)
{
	int ret = 0;

	__ASSERT_NO_MSG(work != NULL);
	__ASSERT_NO_MSG(queue != NULL);

	k_spinlock_key_t key = k_spin_lock(&work_lock);

	/* Check if already queued */
	if (!FLAG_TEST(&work->flags, K_WORK_QUEUED_BIT)) {
		/* If no queue specified, use last known queue */
		if (queue == NULL) {
			queue = work->queue;
		}

		/* If work is currently running, must use same queue */
		if (FLAG_TEST(&work->flags, K_WORK_RUNNING_BIT)) {
			__ASSERT_NO_MSG(work->queue != NULL);
			queue = work->queue;
		}

		/* Check if queue accepts new work */
		if (queue == NULL || !FLAG_TEST(&queue->flags, K_WORK_QUEUE_STARTED_BIT)) {
			ret = -ENODEV;
		} else if (FLAG_TEST(&queue->flags, K_WORK_QUEUE_DRAIN_BIT)) {
			ret = -EBUSY;
		} else {
			/* Add to pending list */
			sys_slist_append(&queue->pending, &work->node);
			FLAG_SET(&work->flags, K_WORK_QUEUED_BIT);
			work->queue = queue;
			FLAG_SET(&queue->flags, K_WORK_QUEUE_BUSY_BIT);
			ret = 0;
		}
	} else {
		/* Work already queued */
	}

	k_spin_unlock(&work_lock, key);
	return ret;
}

int k_work_submit(struct k_work *work)
{	
	return k_work_submit_to_queue(&k_sys_work_q, work);
}

bool k_work_flush(struct k_work *work, struct k_work_sync *sync)
{
	/* Simplified implementation - just wait for work to complete */
	__ASSERT_NO_MSG(work != NULL);
	
	while (FLAG_TEST(&work->flags, K_WORK_QUEUED_BIT | K_WORK_RUNNING_BIT)) {
		k_msleep(1);
	}
	
	return false;
}

int k_work_cancel(struct k_work *work)
{
	k_spinlock_key_t key;
	int ret = 0;

	__ASSERT_NO_MSG(work != NULL);

	key = k_spin_lock(&work_lock);

	/* Remove from queue if pending */
	if (FLAG_TEST(&work->flags, K_WORK_QUEUED_BIT)) {
		(void)sys_slist_find_and_remove(&work->queue->pending, &work->node);
		FLAG_CLEAR(&work->flags, K_WORK_QUEUED_BIT);
	}

	/* If still running, mark as canceling */
	if (FLAG_TEST(&work->flags, K_WORK_RUNNING_BIT)) {
		FLAG_SET(&work->flags, K_WORK_CANCELING_BIT);
		ret = -EBUSY;
	}

	k_spin_unlock(&work_lock, key);
	return ret;
}

bool k_work_cancel_sync(struct k_work *work, struct k_work_sync *sync)
{
	__ASSERT_NO_MSG(work != NULL);
	
	k_work_cancel(work);
	
	/* Wait for completion */
	while (FLAG_TEST(&work->flags, K_WORK_RUNNING_BIT)) {
		k_msleep(1);
	}
	
	return true;
}

/* Delayable work operations */
void k_work_init_delayable(struct k_work_delayable *dwork, k_work_handler_t handler)
{
	__ASSERT_NO_MSG(dwork != NULL);
	k_work_init(&dwork->work, handler);
	dwork->timeout.handle = RT_NULL;
	dwork->timeout.expiry = 0;
	dwork->queue = NULL;
}

/* Timer callback for delayable work */
static void dwork_timer_entry(void *parameter)
{
	struct k_work_delayable *dwork = (struct k_work_delayable *)parameter;
	struct k_work_q *queue = dwork->queue ? dwork->queue : &k_sys_work_q;
	
	/* Submit the work to the queue */
	k_work_submit_to_queue(queue, &dwork->work);
}

int k_work_schedule_for_queue(struct k_work_q *queue, struct k_work_delayable *dwork, k_timeout_t delay)
{
	__ASSERT_NO_MSG(dwork != NULL);
	
	if (delay <= 0) {
		return k_work_submit_to_queue(queue, &dwork->work);
	}

	/* Create or reset timer */
	if (dwork->timeout.handle == RT_NULL) {
		dwork->timeout.handle = rt_timer_create("dwork", dwork_timer_entry, dwork, 
		                                        delay, RT_TIMER_FLAG_ONE_SHOT | RT_TIMER_FLAG_SOFT_TIMER);
		if (dwork->timeout.handle == RT_NULL) {
			return -ENOMEM;
		}
	} else {
		rt_timer_stop(dwork->timeout.handle);
		rt_timer_control(dwork->timeout.handle, RT_TIMER_CTRL_SET_TIME, &delay);
	}

	dwork->queue = queue;
	rt_timer_start(dwork->timeout.handle);
	
	return 0;
}

int k_work_schedule(struct k_work_delayable *dwork, k_timeout_t delay)
{
	return k_work_schedule_for_queue(&k_sys_work_q, dwork, delay);
}

int k_work_reschedule_for_queue(struct k_work_q *queue, struct k_work_delayable *dwork, k_timeout_t delay)
{
	__ASSERT_NO_MSG(dwork != NULL);
	
	/* Cancel any pending work first */
	k_work_cancel_delayable(dwork);
	
	/* Then schedule new work */
	return k_work_schedule_for_queue(queue, dwork, delay);
}

int k_work_reschedule(struct k_work_delayable *dwork, k_timeout_t delay)
{
	return k_work_reschedule_for_queue(&k_sys_work_q, dwork, delay);
}

int k_work_cancel_delayable(struct k_work_delayable *dwork)
{
	__ASSERT_NO_MSG(dwork != NULL);
	
	if (dwork->timeout.handle != RT_NULL) {
		rt_timer_stop(dwork->timeout.handle);
		/* Don't delete timer, just stop it. It will be reused on next schedule */
	}
	
	return k_work_cancel(&dwork->work);
}

bool k_work_cancel_delayable_sync(struct k_work_delayable *dwork, struct k_work_sync *sync)
{
	__ASSERT_NO_MSG(dwork != NULL);
	
	if (dwork->timeout.handle != RT_NULL) {
		rt_timer_stop(dwork->timeout.handle);
		/* Don't delete timer, just stop it. It will be reused on next schedule */
	}
	
	return k_work_cancel_sync(&dwork->work, sync);
}

int k_work_delayable_busy_get(const struct k_work_delayable *dwork)
{
	__ASSERT_NO_MSG(dwork != NULL);
	
	/* Check if timer is active or work is queued/running */
	if (dwork->timeout.handle != RT_NULL) {
		/* In RT-Thread, we can check timer status if needed, 
		 * for now assume if handle exists it might be busy */
		return 1; 
	}
	
	/* Check the underlying work flags */
	if (FLAG_TEST(&dwork->work.flags, K_WORK_QUEUED_BIT | K_WORK_RUNNING_BIT)) {
		return 1;
	}
	
	return 0;
}

/* Work queue thread entry function */
static void work_queue_thread_entry(void *p1, void *p2, void *p3)
{
	struct k_work_q *queue = (struct k_work_q *)p1;

	while (true) {
		sys_snode_t *node;
		struct k_work *work = NULL;
		k_work_handler_t handler = NULL;
		bool yield;

		/* Take lock and check for work */
		k_spinlock_key_t key = k_spin_lock(&work_lock);

		/* Get next work item from pending list */
		node = sys_slist_get(&queue->pending);
		if (node != NULL) {
			work = CONTAINER_OF(node, struct k_work, node);
			FLAG_SET(&work->flags, K_WORK_RUNNING_BIT);
			FLAG_CLEAR(&work->flags, K_WORK_QUEUED_BIT);
			handler = work->handler;
		} else if (FLAG_TEST_AND_CLEAR(&queue->flags, K_WORK_QUEUE_DRAIN_BIT)) {
			/* Wake up threads waiting for drain */
			/* TODO: Implement wake-up mechanism using wait_q */
		} else if (FLAG_TEST(&queue->flags, K_WORK_QUEUE_STOP_BIT)) {
			/* Stop requested */
			FLAG_CLEAR(&queue->flags, K_WORK_QUEUE_STARTED_BIT);
			k_spin_unlock(&work_lock, key);
			return;
		}

		k_spin_unlock(&work_lock, key);

		if (work == NULL) {
			/* No work available, sleep to avoid busy-waiting */
			k_msleep(10);  /* Sleep for 10ms */
			continue;
		}

		/* Execute the work handler */
		if (handler) {
			handler(work);
		}

		/* Mark work as no longer running */
		key = k_spin_lock(&work_lock);
		FLAG_CLEAR(&work->flags, K_WORK_RUNNING_BIT);
		
		/* Handle cancel/flush completion */
		if (FLAG_TEST(&work->flags, K_WORK_FLUSHING_BIT)) {
			/* TODO: Signal flush completion */
			FLAG_CLEAR(&work->flags, K_WORK_FLUSHING_BIT);
		}
		if (FLAG_TEST(&work->flags, K_WORK_CANCELING_BIT)) {
			/* TODO: Signal cancel completion */
			FLAG_CLEAR(&work->flags, K_WORK_CANCELING_BIT);
		}

		FLAG_CLEAR(&queue->flags, K_WORK_QUEUE_BUSY_BIT);
		yield = !FLAG_TEST(&queue->flags, K_WORK_QUEUE_NO_YIELD_BIT);
		k_spin_unlock(&work_lock, key);

		if (yield) {
			k_yield();
		}
	}
}

/* Work queue management */
void k_work_queue_init(struct k_work_q *queue)
{
	__ASSERT_NO_MSG(queue != NULL);

	sys_slist_init(&queue->pending);
	queue->flags = 0;
}

void k_work_queue_start(struct k_work_q *queue,
                        k_thread_stack_t *stack, size_t stack_size,
                        int prio, const struct k_work_queue_config *cfg)
{
	__ASSERT_NO_MSG(queue != NULL);
	__ASSERT_NO_MSG(stack != NULL);

	if (FLAG_TEST(&queue->flags, K_WORK_QUEUE_STARTED_BIT)) {
		return;
	}

	/* Initialize queue state */
	sys_slist_init(&queue->pending);
	FLAG_SET(&queue->flags, K_WORK_QUEUE_STARTED_BIT);

	/* Create and start work queue thread */
	const char *name = (cfg && cfg->name) ? cfg->name : "zephyr_wq";
	
	if (!k_thread_create(&queue->thread, stack, stack_size,
	                work_queue_thread_entry, queue, NULL, NULL,
	                prio, 0, K_FOREVER)) {
		FLAG_CLEAR(&queue->flags, K_WORK_QUEUE_STARTED_BIT);
		return;
	}

	if (cfg && cfg->name) {
		k_thread_name_set(&queue->thread, cfg->name);
	}
}

/* System work queue initialization */
static int k_sys_work_workqueue_init(void)
{
	/* Allocate stack as a static array (8KB for RT-Thread thread creation) */
	static char sys_wq_stack[8192] __attribute__((aligned(8)));
	static bool initialized = false;

	if (initialized) {
		return 0;
	}

	if (FLAG_TEST(&k_sys_work_q.flags, K_WORK_QUEUE_STARTED_BIT)) {
		initialized = true;
		return 0;
	}

	k_work_queue_init(&k_sys_work_q);
	k_work_queue_start(&k_sys_work_q,
	                   sys_wq_stack,
	                   sizeof(sys_wq_stack),
	                   21,  /* Priority */
	                   NULL);
	
	initialized = true;

	return 0;
}
INIT_DEVICE_EXPORT(k_sys_work_workqueue_init);