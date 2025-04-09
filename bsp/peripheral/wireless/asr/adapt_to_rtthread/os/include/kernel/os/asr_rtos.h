/**
 ******************************************************************************
 * @file    asr_rtos.h
 * @author  asr WIFI team
 * @version V1.0.0
 * @date    05-Sep-2017
 * @brief   This file provides all the headers of RTOS operation provided by Lega.
 ******************************************************************************
 *
 *  Copyright (c) 2017 ASR Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is furnished
 *  to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 *  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 */

#ifndef __ASRRTOS_H__
#define __ASRRTOS_H__

//#include "co_int.h"
#include "stddef.h"
#include <stdio.h>
#include <stdbool.h>
#include "asr_rtos_api.h"

/** @addtogroup ASR_Core_APIs
  * @{
  */

/** @defgroup ASR_RTOS ASR RTOS Operations
  * @brief Provide management APIs for Thread, Mutex, Timer, Semaphore and FIFO
  * @{
  */
#define     RTOS_HIGHEST_PRIORITY       (configMAX_PRIORITIES)

#define FreeRTOS_VERSION_MAJOR 9



#define ASR_HARDWARE_IO_WORKER_THREAD     ( (asr_worker_thread_t*)&asr_hardware_io_worker_thread)
#define ASR_NETWORKING_WORKER_THREAD      ( (asr_worker_thread_t*)&asr_worker_thread )
#define ASR_WORKER_THREAD                 ( (asr_worker_thread_t*)&asr_worker_thread )

#define ASR_NETWORK_WORKER_PRIORITY      (3)
#define ASR_DEFAULT_WORKER_PRIORITY      (5)
#define ASR_DEFAULT_LIBRARY_PRIORITY     (5)
#define ASR_APPLICATION_PRIORITY         (7)

#define kNanosecondsPerSecond   1000000000UUL
#define kMicrosecondsPerSecond  1000000UL
#define kMillisecondsPerSecond  1000

typedef enum
{
    WAIT_FOR_ANY_EVENT,
    WAIT_FOR_ALL_EVENTS,
} asr_event_flags_wait_option_t;

typedef struct
{
    asr_thread_t thread;
    asr_queue_t  event_queue;
} asr_worker_thread_t;

typedef struct
{
    event_handler_t        function;
    void*                  arg;
    asr_timer_t           timer;
    asr_worker_thread_t*  thread;
} asr_timed_event_t;

extern asr_worker_thread_t asr_hardware_io_worker_thread;
extern asr_worker_thread_t asr_worker_thread;

/* Legacy definitions */
#define asr_thread_sleep                 asr_rtos_thread_sleep
#define asr_thread_msleep                asr_rtos_thread_msleep



/** @defgroup ASR_RTOS_Thread ASR RTOS Thread Management Functions
 *  @brief Provide thread creation, delete, suspend, resume, and other RTOS management API
 *  @verbatim
 *   ASR thread priority table
 *
 * +----------+-----------------+
 * | Priority |      Thread     |
 * |----------|-----------------|
 * |     0    |      ASR       |   Highest priority
 * |     1    |     Network     |
 * |     2    |                 |
 * |     3    | Network worker  |
 * |     4    |                 |
 * |     5    | Default Library |
 * |          | Default worker  |
 * |     6    |                 |
 * |     7    |   Application   |
 * |     8    |                 |
 * |     9    |      Idle       |   Lowest priority
 * +----------+-----------------+
 *  @endverbatim
 * @{
 */

/** @brief   Creates a worker thread
 *
 * Creates a worker thread
 * A worker thread is a thread in whose context timed and asynchronous events
 * execute.
 *
 * @param worker_thread    : a pointer to the worker thread to be created
 * @param priority         : thread priority
 * @param stack_size       : thread's stack size in number of bytes
 * @param event_queue_size : number of events can be pushed into the queue
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred
 */
OSStatus asr_rtos_create_worker_thread( asr_worker_thread_t* worker_thread, uint8_t priority, uint32_t stack_size, uint32_t event_queue_size );


/** @brief   Deletes a worker thread
 *
 * @param worker_thread : a pointer to the worker thread to be created
 *
 * @return    kNoErr : on success.
 * @return    kGeneralErr   : if an error occurred
 */
OSStatus asr_rtos_delete_worker_thread( asr_worker_thread_t* worker_thread );


/** @brief    Suspend all other thread
  *
  * @param    none
  *
  * @return   none
  */
void asr_rtos_suspend_all_thread(void);


/** @brief    Rresume all other thread
  *
  * @param    none
  *
  * @return   none
  */
long asr_rtos_resume_all_thread(void);


/** @brief    Sleeps until another thread has terminated
  *
  * @Details  Causes the current thread to sleep until the specified other thread
  *           has terminated. If the processor is heavily loaded with higher priority
  *           tasks, this thread may not wake until significantly after the thread termination.
  *
  * @param    thread : the handle of the other thread which will terminate
  *
  * @return   kNoErr        : on success.
  * @return   kGeneralErr   : if an error occurred
  */
OSStatus asr_rtos_thread_join( asr_thread_t* thread );


/** @brief    Forcibly wakes another thread
  *
  * @Details  Causes the specified thread to wake from suspension. This will usually
  *           cause an error or timeout in that thread, since the task it was waiting on
  *           is not complete.
  *
  * @param    thread : the handle of the other thread which will be woken
  *
  * @return   kNoErr        : on success.
  * @return   kGeneralErr   : if an error occurred
  */
OSStatus asr_rtos_thread_force_awake( asr_thread_t* thread );



/** @brief    Suspend current thread for a specific time
  *
  * @param    seconds : A time interval (Unit: seconds)
  *
  * @return   None.
  */
void asr_rtos_thread_sleep(uint32_t seconds);

/** @brief    Suspend current thread for a specific time
 *
 * @param     milliseconds : A time interval (Unit: millisecond)
 *
 * @return    None.
 */
void asr_rtos_thread_msleep(uint32_t milliseconds);







/** @defgroup ASR_RTOS_EVENT ASR RTOS Event Functions
  * @{
  */

/**
  * @brief    Sends an asynchronous event to the associated worker thread
  *
  * @param worker_thread :the worker thread in which context the callback should execute from
  * @param function      : the callback function to be called from the worker thread
  * @param arg           : the argument to be passed to the callback function
  *
  * @return    kNoErr        : on success.
  * @return    kGeneralErr   : if an error occurred
  */
OSStatus asr_rtos_send_asynchronous_event( asr_worker_thread_t* worker_thread, event_handler_t function, void* arg );

/** Requests a function be called at a regular interval
 *
 * This function registers a function that will be called at a regular
 * interval. Since this is based on the RTOS time-slice scheduling, the
 * accuracy is not high, and is affected by processor load.
 *
 * @param event_object  : pointer to a event handle which will be initialised
 * @param worker_thread : pointer to the worker thread in whose context the
 *                        callback function runs on
 * @param function      : the callback function that is to be called regularly
 * @param time_ms       : the time period between function calls in milliseconds
 * @param arg           : an argument that will be supplied to the function when
 *                        it is called
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred
 */
OSStatus asr_rtos_register_timed_event( asr_timed_event_t* event_object, asr_worker_thread_t* worker_thread, event_handler_t function, uint32_t time_ms, void* arg );


/** Removes a request for a regular function execution
 *
 * This function de-registers a function that has previously been set-up
 * with @ref asr_rtos_register_timed_event.
 *
 * @param event_object : the event handle used with @ref asr_rtos_register_timed_event
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred
 */
OSStatus asr_rtos_deregister_timed_event( asr_timed_event_t* event_object );



/** @brief    Initialize an endpoint for a RTOS event, a file descriptor
  *           will be created, can be used for select
  *
  * @param    event_handle : asr_semaphore_t, asr_mutex_t or asr_queue_t
  *
  * @retval   On success, a file descriptor for RTOS event is returned.
  *           On error, -1 is returned.
  */
int asr_rtos_init_event_fd(asr_event_t event_handle);

/** @brief    De-initialise an endpoint created from a RTOS event
  *
  * @param    fd : file descriptor for RTOS event
  *
  * @retval   0 for success. On error, -1 is returned.
  */
int asr_rtos_deinit_event_fd(int fd);


/**
  * @}
  */

#endif

/**
  * @}
  */

/**
  * @}
  */

