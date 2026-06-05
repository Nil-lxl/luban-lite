/*
 * Copyright (c) 2016, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>

#ifndef K_SPINLOCK_H_
#define K_SPINLOCK_H_

typedef unsigned int k_spinlock_key_t;

struct k_spinlock {
    uint32_t data;
};

static unsigned int irq_lock(void)
{
    return rt_hw_interrupt_disable();
}

static void irq_unlock(unsigned int key)
{
    rt_hw_interrupt_enable(key);
}

static k_spinlock_key_t k_spin_lock(struct k_spinlock *l)
{
    return irq_lock();
}

static void k_spin_unlock(struct k_spinlock *l, k_spinlock_key_t key)
{
    irq_unlock(key);
}

#endif /* K_SPINLOCK_H_ */
