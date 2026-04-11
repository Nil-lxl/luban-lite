/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc: mpp thread interface
 */

#ifndef MPP_THREAD_H
#define MPP_THREAD_H

#include <stdint.h>
#include <inttypes.h>
#include "mpp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MPP_WAIT_FOREVER  (-1)

typedef void *mpp_sem_t;


mpp_sem_t mpp_sem_create();
void mpp_sem_delete(mpp_sem_t sem);
int mpp_sem_signal(mpp_sem_t sem);
int mpp_sem_wait(mpp_sem_t sem, int64_t us);


#ifdef __cplusplus
}
#endif

#endif /* MPP_THREAD_H */
