/*
 * Copyright (c) 2016, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/dlist.h>
#include <zephyr/sys/sflist.h>
#include <zephyr/k_spinlock.h>

#ifndef K_SEM_H_
#define K_SEM_H_

/**
 * @cond INTERNAL_HIDDEN
 */

//struct k_sem {
//	_wait_q_t wait_q;
//    rt_sem_t sem;
//	unsigned int count;
//	unsigned int limit;
//
//	//Z_DECL_POLL_EVENT
//    sys_dlist_t poll_events;
//
//	SYS_PORT_TRACING_TRACKING_FIELD(k_sem)
//
//#ifdef OBJ_CORE_SEM
//	struct k_obj_core  obj_core;
//#endif
//};

struct k_sem {
    //struct rt_semaphore sem;
    rt_sem_t sem;
	_wait_q_t wait_q;
	unsigned int count;
	unsigned int limit;

	//Z_DECL_POLL_EVENT
    sys_dlist_t poll_events;

	//SYS_PORT_TRACING_TRACKING_FIELD(k_sem)

#ifdef OBJ_CORE_SEM
	struct k_obj_core  obj_core;
#endif
};

#define Z_SEM_INITIALIZER(obj, initial_count, count_limit)                                        \
    {                                                                                             \
        .wait_q = Z_WAIT_Q_INIT(&(obj).wait_q),                                                   \
        .count = (initial_count),                                                                 \
        .limit = (count_limit),                                                                   \
        Z_POLL_EVENT_OBJ_INIT(obj)                                                                \
    }

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @defgroup semaphore_apis Semaphore APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Maximum limit value allowed for a semaphore.
 *
 * This is intended for use when a semaphore does not have
 * an explicit maximum limit, and instead is just used for
 * counting purposes.
 *
 */
#define K_SEM_MAX_LIMIT UINT_MAX

/**
 * @brief Initialize a semaphore.
 *
 * This routine initializes a semaphore object, prior to its first use.
 *
 * @param sem Address of the semaphore.
 * @param initial_count Initial semaphore count.
 * @param limit Maximum permitted semaphore count.
 *
 * @see K_SEM_MAX_LIMIT
 *
 * @retval 0 Semaphore created successfully
 * @retval -EINVAL Invalid values
 *
 */
int k_sem_init(struct k_sem *sem, unsigned int initial_count,
			  unsigned int limit);

/**
 * @brief Take a semaphore.
 *
 * This routine takes @a sem.
 *
 * @note @a timeout must be set to K_NO_WAIT if called from ISR.
 *
 * @funcprops \isr_ok
 *
 * @param sem Address of the semaphore.
 * @param timeout Waiting period to take the semaphore,
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 Semaphore taken.
 * @retval -EBUSY Returned without waiting.
 * @retval -EAGAIN Waiting period timed out,
 *			or the semaphore was reset during the waiting period.
 */
int k_sem_take(struct k_sem *sem, k_timeout_t timeout);

/**
 * @brief Give a semaphore.
 *
 * This routine gives @a sem, unless the semaphore is already at its maximum
 * permitted count.
 *
 * @funcprops \isr_ok
 *
 * @param sem Address of the semaphore.
 */
void k_sem_give(struct k_sem *sem);

/**
 * @brief Resets a semaphore's count to zero.
 *
 * This routine sets the count of @a sem to zero.
 * Any outstanding semaphore takes will be aborted
 * with -EAGAIN.
 *
 * @param sem Address of the semaphore.
 */
void k_sem_reset(struct k_sem *sem);

/**
 * @brief Get a semaphore's count.
 *
 * This routine returns the current count of @a sem.
 *
 * @param sem Address of the semaphore.
 *
 * @return Current semaphore count.
 */
unsigned int k_sem_count_get(struct k_sem *sem);

/**
 * @internal
 */
static inline unsigned int z_impl_k_sem_count_get(struct k_sem *sem)
{
	return sem->count;
}

/**
 * @brief Statically define and initialize a semaphore.
 *
 * The semaphore can be accessed outside the module where it is defined using:
 *
 * @code extern struct k_sem <name>; @endcode
 *
 * @param name Name of the semaphore.
 * @param initial_count Initial semaphore count.
 * @param count_limit Maximum permitted semaphore count.
 */
#define K_SEM_DEFINE(name, initial_count, count_limit)                                             \
	STRUCT_SECTION_ITERABLE(k_sem, name) =                                                     \
		Z_SEM_INITIALIZER(name, initial_count, count_limit);                               \
	BUILD_ASSERT(((count_limit) != 0) &&                                                       \
		     (((initial_count) < (count_limit)) || ((initial_count) == (count_limit))) &&  \
		     ((count_limit) <= K_SEM_MAX_LIMIT));

/** @} */

#endif /* K_SEM_H_ */
