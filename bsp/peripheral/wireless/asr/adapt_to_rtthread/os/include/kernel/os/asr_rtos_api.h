/**
 ******************************************************************************
 * @file    asr_rtos_api.h
 * @author  asr WIFI team
 * @version V1.0.0
 * @date    28-Dec-2018
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

 #ifndef _ASR_RTOS_API_H_
 #define _ASR_RTOS_API_H_
#include <stdint.h>
#include <stdio.h>
#ifdef ALIOS_SUPPORT
#include "list.h"
//#else
//#include "linux/list.h"
#endif
//#include "linux/container_of.h"
//#include "uwifi_wlan_list.h"
#include <aic_list.h>
#include <aic_common.h>
#include <rtthread.h>
#define ASR_DEBUG_MALLOC_TRACE     0 //customer should not change
#define ASR_NEVER_TIMEOUT   RT_WAITING_FOREVER
#define ASR_WAIT_FOREVER    RT_WAITING_FOREVER
#define ASR_NO_WAIT         RT_WAITING_NO
#define ASR_API_TIMEOUT  10000   // max cmd last time(wpa3 connect)
#ifndef NULL
#define NULL (void*)0
#endif
#define ASR_SEMAPHORE_COUNT_MAX (0xFFFF)

#define ASR_TASK_CONFIG_MAX                16
#define ASR_TASK_CONFIG_UWIFI              0
#define ASR_TASK_CONFIG_UWIFI_SDIO         1
#define ASR_TASK_CONFIG_UWIFI_RX_TO_OS     2
#define ASR_TASK_CONFIG_UWIFI_AT           3
#define ASR_TASK_CONFIG_UWIFI_AT_UART_RCV  4
#define ASR_TASK_CONFIG_IPERF              5
#define ASR_TASK_CONFIG_PING               6
#define ASR_TASK_CONFIG_LWIP               7
#define ASR_TASK_CONFIG_DOUB_85_SC         8

typedef enum
{
    kNoErr=0,
    kGeneralErr,
    kTimeoutErr,
    kNoMemErr,
}OSStatus;
#define FALSE 0
#define TRUE 1
typedef enum
{
    kFALSE=0,
    kTRUE = 1,
}OSBool;
typedef struct
{
    /* the number of bytes of free heap.*/
    uint32_t free_heap_size;
} asr_rtos_mem_info_t;

typedef enum {
    ASR_RTOS_MEM_INFO_REQ_TYPE_PRINTF, //
    ASR_RTOS_MEM_INFO_REQ_TYPE_DATA, //
} asr_rtos_mem_info_req_type_t;

typedef int    asr_atomic_t;
typedef uint32_t  asr_event_flags_t;
typedef void * asr_semaphore_t;
typedef void * asr_mutex_t;
typedef void * asr_thread_t ;
typedef struct  _asr_thread_hand_t
{
    asr_thread_t thread;
    void* stack_ptr;
}asr_thread_hand_t;

//typedef void * asr_event_t;

typedef void  (*native_thread_t) (void *) ;

typedef void * asr_queue_t;

typedef struct
{
    asr_queue_t queue;
    uint32_t max_size;
    uint32_t cur_size;
    asr_mutex_t mutex;
    uint32_t msg_size;
} asr_queue_stuct_t;

typedef void * asr_event_t;// ASR OS event: asr_semaphore_t, asr_mutex_t or asr_queue_t

typedef void (*timer_handler_t)( void* arg );

typedef OSStatus (*event_handler_t)( void* arg );

typedef void* xTimerHandle;
typedef uint32_t asr_time_t;
typedef void* xTaskHandle;

typedef void * asr_hisr_t;
typedef void  (*hisr_func) (void) ;

typedef struct
{
    void *          handle;
    timer_handler_t function;
    timer_handler_t timer_func;
    void *          timer_arg;
    void *          arg;
    uint32_t        inittime;
    uint32_t        rescheduletime;
    uint8_t            repeat;
    struct list_head item;
    uint32_t        cnt;
}asr_timer_t;

typedef struct
{
    uint8_t index;
    uint8_t task_priority;
    uint32_t stack_size;
} asr_task_config_t;

typedef void * asr_thread_arg_t;
typedef void (*asr_thread_function_t)( asr_thread_arg_t arg );

int asr_arch_irq_save(void);
int asr_arch_irq_restore(int flags);

/** @defgroup ASR_RTOS ASR RTOS Operations
  * @{
  */
#define asr_rtos_declare_critical() int cpu_sr

/** @brief Enter a critical session, all interrupts are disabled
  *
  * @return    none
  */
#define asr_rtos_enter_critical() cpu_sr = asr_arch_irq_save()

/** @brief Exit a critical session, all interrupts are enabled
  *
  * @return    none
  */
#define asr_rtos_exit_critical() asr_arch_irq_restore(cpu_sr)
/**
  * @}
  */


/** @defgroup ASR_RTOS_Thread ASR RTOS Thread Management Functions
 *  @brief Provide thread creation, delete, suspend, resume, and other RTOS management API
  * @{
 */

/** @brief Creates and starts a new thread
  *
  * @param thread     : Pointer to variable that will receive the thread handle (can be null)
  * @param priority   : A priority number.
  * @param name       : a text name for the thread (can be null)
  * @param function   : the main thread function
  * @param stack_size : stack size for this thread
  * @param arg        : argument which will be passed to thread function
  *
  * @return    kNoErr          : on success.
  * @return    kGeneralErr     : if an error occurred
  */
OSStatus asr_rtos_create_thread( asr_thread_hand_t* thread, uint8_t priority,const char* name, asr_thread_function_t function, uint32_t stack_size, asr_thread_arg_t arg );

/** @brief   Deletes a terminated thread
  *
  * @param   thread     : the handle of the thread to delete, , NULL is the current thread
  *
  * @return  kNoErr        : on success.
  * @return  kGeneralErr   : if an error occurred
  */
OSStatus asr_rtos_delete_thread( asr_thread_hand_t* thread );

/** @brief    Suspend a thread
  *
  * @param    thread     : the handle of the thread to suspend, NULL is the current thread
  *
  * @return   kNoErr        : on success.
  * @return   kGeneralErr   : if an error occurred
  */
void asr_rtos_suspend_thread(asr_thread_t* thread);

/** @brief    Checks if a thread is the current thread
  *
  * @Details  Checks if a specified thread is the currently running thread
  *
  * @param    thread : the handle of the other thread against which the current thread
  *                    will be compared
  *
  * @return   true   : specified thread is the current thread
  * @return   false  : specified thread is not currently running
  */
OSBool asr_rtos_is_current_thread( asr_thread_t* thread );

/** @brief    Get current thread handler
  *
  * @return   Current ASR RTOS thread handler
  */
asr_thread_t* asr_rtos_get_current_thread( void );

OSBool asr_rtos_is_thread_valid(asr_thread_hand_t* thread);

/** @brief    Suspend current thread for a specific time, at lease delay 1 tick
 *
 * @param     num_ms : A time interval (Unit: millisecond)
 *
 * @return    kNoErr.
 */
OSStatus asr_rtos_delay_milliseconds( uint32_t num_ms );

/** @brief    Print Thread status into buffer
  *
  * @param    buffer, point to buffer to store thread status
  * @param    length, length of the buffer
  *
  * @return   none
  */
OSStatus asr_rtos_print_thread_status( char* buffer, int length );

asr_thread_t asr_rtos_get_thread_by_name(char* name);

OSStatus asr_rtos_task_cfg_get(uint32_t index, asr_task_config_t *cfg );

/**
  * @}
  */

  /** @defgroup ASR_RTOS_SEM ASR RTOS Semaphore Functions
  * @brief Provide management APIs for semaphore such as init,set,get and dinit.
  * @{
  */

/** @brief    Initialises a counting semaphore and set count to 0
  *
  * @param    semaphore : a pointer to the semaphore handle to be initialised
  * @param    count     : the init value of this semaphore
  *
  * @return   kNoErr        : on success.
  * @return   kGeneralErr   : if an error occurred
  */
OSStatus asr_rtos_init_semaphore( asr_semaphore_t* semaphore, int value );

/** @brief    Set (post/put/increment) a semaphore
  *
  * @param    semaphore : a pointer to the semaphore handle to be set
  *
  * @return   kNoErr        : on success.
  * @return   kGeneralErr   : if an error occurred
  */
OSStatus asr_rtos_set_semaphore( asr_semaphore_t* semaphore );

/** @brief    Get (wait/decrement) a semaphore
  *
  * @Details  Attempts to get (wait/decrement) a semaphore. If semaphore is at zero already,
  *           then the calling thread will be suspended until another thread sets the
  *           semaphore with @ref asr_rtos_set_semaphore
  *
  * @param    semaphore : a pointer to the semaphore handle
  * @param    timeout_ms: the number of milliseconds to wait before returning
  *
  * @return   kNoErr        : on success.
  * @return   kGeneralErr   : if an error occurred
  */
OSStatus asr_rtos_get_semaphore( asr_semaphore_t* semaphore, uint32_t timeout_ms );

/** @brief    De-initialise a semaphore
  *
  * @Details  Deletes a semaphore created with @ref asr_rtos_init_semaphore
  *
  * @param    semaphore : a pointer to the semaphore handle
  *
  * @return   kNoErr        : on success.
  * @return   kGeneralErr   : if an error occurred
  */
OSStatus asr_rtos_deinit_semaphore( asr_semaphore_t* semaphore );
/**
  * @}
  */


/** @defgroup ASR_RTOS_MUTEX ASR RTOS Mutex Functions
  * @brief Provide management APIs for Mutex such as init,lock,unlock and dinit.
  * @{
  */

/** @brief    Initialises a mutex
  *
  * @Details  A mutex is different to a semaphore in that a thread that already holds
  *           the lock on the mutex can request the lock again (nested) without causing
  *           it to be suspended.
  *
  * @param    mutex : a pointer to the mutex handle to be initialised
  *
  * @return   kNoErr        : on success.
  * @return   kGeneralErr   : if an error occurred
  */
OSStatus asr_rtos_init_mutex( asr_mutex_t* mutex );

/** @brief    Obtains the lock on a mutex
  *
  * @Details  Attempts to obtain the lock on a mutex. If the lock is already held
  *           by another thead, the calling thread will be suspended until the mutex
  *           lock is released by the other thread.
  *
  * @param    mutex : a pointer to the mutex handle to be locked
  *
  * @return   kNoErr        : on success.
  * @return   kGeneralErr   : if an error occurred
  */
OSStatus asr_rtos_lock_mutex( asr_mutex_t* mutex );

/** @brief    Releases the lock on a mutex
  *
  * @Details  Releases a currently held lock on a mutex. If another thread
  *           is waiting on the mutex lock, then it will be resumed.
  *
  * @param    mutex : a pointer to the mutex handle to be unlocked
  *
  * @return   kNoErr        : on success.
  * @return   kGeneralErr   : if an error occurred
  */
OSStatus asr_rtos_unlock_mutex( asr_mutex_t* mutex );

/** @brief    De-initialise a mutex
  *
  * @Details  Deletes a mutex created with @ref asr_rtos_init_mutex
  *
  * @param    mutex : a pointer to the mutex handle
  *
  * @return   kNoErr        : on success.
  * @return   kGeneralErr   : if an error occurred
  */
OSStatus asr_rtos_deinit_mutex( asr_mutex_t* mutex );

/**
  * @}
  */


/** @defgroup ASR_RTOS_QUEUE ASR RTOS FIFO Queue Functions
  * @brief Provide management APIs for FIFO such as init,push,pop and dinit.
  * @{
  */

/** @brief    Initialises a FIFO queue
  *
  * @param    queue : a pointer to the queue handle to be initialised
  * @param    name  : a text string name for the queue (NULL is allowed)
  * @param    message_size : size in bytes of objects that will be held in the queue
  * @param    number_of_messages : depth of the queue - i.e. max number of objects in the queue
  *
  * @return   kNoErr        : on success.
  * @return   kGeneralErr   : if an error occurred
  */
OSStatus asr_rtos_init_queue( asr_queue_stuct_t* queue_struct, const char* name, uint32_t message_size, uint32_t number_of_messages );

/** @brief    Pushes an object onto a queue's front
  *
  * @param    queue : a pointer to the queue handle
  * @param    message : the object to be added to the queue. Size is assumed to be
  *                  the size specified in @ref asr_rtos_init_queue
  * @param    timeout_ms: the number of milliseconds to wait before returning
  *
  * @return   kNoErr        : on success.
  * @return   kGeneralErr   : if an error or timeout occurred
  */
OSStatus asr_rtos_push_to_queue_front( asr_queue_stuct_t* queue_struct, void* message, uint32_t timeout_ms );

/** @brief    Pushes an object onto a queue
  *
  * @param    queue : a pointer to the queue handle
  * @param    message : the object to be added to the queue. Size is assumed to be
  *                  the size specified in @ref asr_rtos_init_queue
  * @param    timeout_ms: the number of milliseconds to wait before returning
  *
  * @return   kNoErr        : on success.
  * @return   kGeneralErr   : if an error or timeout occurred
  */
OSStatus asr_rtos_push_to_queue( asr_queue_stuct_t* queue_struct, void* message, uint32_t timeout_ms );

/** @brief    Pops an object off a queue
  *
  * @param    queue : a pointer to the queue handle
  * @param    message : pointer to a buffer that will receive the object being
  *                     popped off the queue. Size is assumed to be
  *                     the size specified in @ref asr_rtos_init_queue , hence
  *                     you must ensure the buffer is long enough or memory
  *                     corruption will result
  * @param    timeout_ms: the number of milliseconds to wait before returning
  *
  * @return   kNoErr        : on success.
  * @return   kGeneralErr   : if an error or timeout occurred
  */
OSStatus asr_rtos_pop_from_queue( asr_queue_stuct_t* queue_struct, void* message, uint32_t timeout_ms );

/** @brief    De-initialise a queue created with @ref asr_rtos_init_queue
  *
  * @param    queue : a pointer to the queue handle
  *
  * @return   kNoErr        : on success.
  * @return   kGeneralErr   : if an error occurred
  */
OSStatus asr_rtos_deinit_queue( asr_queue_stuct_t* queue_struct );

/** @brief    Check if a queue is empty, only used in_interrupt_context
  *
  * @param    queue : a pointer to the queue handle
  *
  * @return   true  : queue is empty.
  * @return   false : queue is not empty.
  */
OSBool asr_rtos_is_queue_empty( asr_queue_stuct_t* queue_struct );

/** @brief    Check if a queue is full, only used in_interrupt_context
  *
  * @param    queue : a pointer to the queue handle
  *
  * @return   true  : queue is empty.
  * @return   false : queue is not empty.
  */
OSBool asr_rtos_is_queue_full( asr_queue_stuct_t* queue_struct );

OSBool asr_rtos_is_queue_valid( asr_queue_stuct_t* queue_struct );

/**
  * @}
  */
#define UWIFI_SDIO_EVENT_RX     (0x1 << 0)
#define UWIFI_SDIO_EVENT_TX     (0x1 << 1)
#define UWIFI_SDIO_EVENT_MSG     (0x1 << 2)
OSStatus asr_rtos_create_event(asr_event_t *evt);
OSStatus asr_rtos_deinit_event(asr_event_t *evt);
OSStatus asr_rtos_wait_for_event(asr_event_t *evt, uint32_t mask, uint32_t *rcv_flags, uint32_t timeout);
OSStatus asr_rtos_set_event(asr_event_t *evt, uint32_t mask);
OSStatus asr_rtos_clear_event(asr_event_t *evt, uint32_t mask);
/** @defgroup ASR_RTOS_TIMER ASR RTOS Timer Functions
  * @brief Provide management APIs for timer such as init,start,stop,reload and dinit.
  * @{
  */

/**
  * @brief    Gets time in miiliseconds since RTOS start
  *
  * @note:    Since this is only 32 bits, it will roll over every 49 days, 17 hours.
  *
  * @returns  Time in milliseconds since RTOS started.
  */
uint32_t asr_rtos_get_time(void);
uint32_t get_rt_tick_per_second(void);

uint32_t asr_rtos_get_ticks(void);

/**
  * @brief     Initialize a RTOS timer
  *
  * @note      Timer does not start running until @ref asr_start_timer is called
  *
  * @param     timer    : a pointer to the timer handle to be initialised
  * @param     time_ms  : Timer period in milliseconds
  * @param     function : the callback handler function that is called each time the
  *                       timer expires
  * @param     arg      : an argument that will be passed to the callback function
  *
  * @return    kNoErr        : on success.
  * @return    kGeneralErr   : if an error occurred
  */
OSStatus asr_rtos_init_timer( asr_timer_t* timer, uint32_t time_ms, timer_handler_t function, void* arg );

/** @brief    Starts a RTOS timer running
  *
  * @note     Timer must have been previously initialised with @ref asr_rtos_init_timer
  *
  * @param    timer    : a pointer to the timer handle to start
  *
  * @return   kNoErr        : on success.
  * @return   kGeneralErr   : if an error occurred
  */
OSStatus asr_rtos_start_timer( asr_timer_t* timer );

/** @brief    Stops a running RTOS timer
  *
  * @note     Timer must have been previously started with @ref asr_rtos_init_timer
  *
  * @param    timer    : a pointer to the timer handle to stop
  *
  * @return   kNoErr        : on success.
  * @return   kGeneralErr   : if an error occurred
  */
OSStatus asr_rtos_stop_timer( asr_timer_t* timer );

/** @brief    Reloads a RTOS timer that has expired
  *
  * @note     This is usually called in the timer callback handler, to
  *           reschedule the timer for the next period.
  *
  * @param    timer    : a pointer to the timer handle to reload
  *
  * @return   kNoErr        : on success.
  * @return   kGeneralErr   : if an error occurred
  */
OSStatus asr_rtos_reload_timer( asr_timer_t* timer );

/** @brief    De-initialise a RTOS timer
  *
  * @note     Deletes a RTOS timer created with @ref asr_rtos_init_timer
  *
  * @param    timer : a pointer to the RTOS timer handle
  *
  * @return   kNoErr        : on success.
  * @return   kGeneralErr   : if an error occurred
  */
OSStatus asr_rtos_deinit_timer( asr_timer_t* timer );

/** @brief    Check if an RTOS timer is running
  *
  * @param    timer : a pointer to the RTOS timer handle
  *
  * @return   true        : if running.
  * @return   false       : if not running
  */
OSBool asr_rtos_is_timer_running( asr_timer_t* timer );

/** @brief    Update an RTOS timer's init time
  *
  * @param    timer : a pointer to the RTOS timer handle
  *
  * @return   true        : if update it success.
  * @return   false       : if update it fail.
  */

OSStatus asr_rtos_update_timer( asr_timer_t* timer, uint32_t time_ms );

/**
  * @}
  */
void asr_msleep(uint32_t time_ms);

  /** @brief    judge whether in interrupt context or not
  *
  * @param
  *
  * @retval   true: in interrupt context. false: not in interrupt context.
  */
OSBool asr_rtos_is_in_interrupt_context(void);

/** @brief Creates a high level isr
  *
  */
void asr_rtos_create_hisr(asr_hisr_t* hisr, char* name, void (*hisr_entry)(void));

/** @brief start a high level isr
  *
  */
int asr_rtos_active_hisr(asr_hisr_t* hisr);

/** @brief delete a high level isr
  *
  */
OSStatus asr_rtos_delete_hisr(asr_hisr_t* hisr);

int asr_rtos_GetError(void);
void asr_rtos_SetError(int err);

#if !ASR_DEBUG_MALLOC_TRACE
/**
 * @ malloc and free
 */
void *asr_rtos_malloc( uint32_t xWantedSize );
void asr_rtos_free( void *pv );
void *asr_rtos_calloc(size_t nmemb, size_t size);
void *asr_rtos_realloc(void *ptr, uint32_t size);

#else
/**
 * @ used to debug memory leak
 */
#undef asr_rtos_malloc
#define asr_rtos_malloc(s) _asr_rtos_malloc((s),__FUNCTION__,__LINE__)

#undef asr_rtos_free
#define asr_rtos_free(v) _asr_rtos_free((v),__FUNCTION__,__LINE__)

#undef asr_rtos_calloc
#define asr_rtos_calloc(s,y) _asr_rtos_calloc((s),(y),__FUNCTION__,__LINE__)

#define asr_rtos_dump_malloc _asr_rtos_dump_malloc
void _asr_rtos_dump_malloc();
void _asr_rtos_free(void *pv,const char* function, int line);
void *_asr_rtos_malloc( uint32_t xWantedSize,const char* function, int line);
void *_asr_rtos_calloc(size_t nmemb, size_t size,const char* function, int line);
#endif


int asr_test_bit(int nr, asr_atomic_t* addr);
int asr_clear_bit(int nr, asr_atomic_t* addr);
int asr_set_bit(int nr, asr_atomic_t* addr);
int asr_test_and_set_bit(int nr, asr_atomic_t* addr);

static inline unsigned long asr_os_msec_to_ticks(unsigned long msecs)
{
	return ((msecs) * (RT_TICK_PER_SECOND)) / 1000;
}
static inline unsigned long asr_os_ticks_to_msec(unsigned long ticks)
{
	return ((ticks) * 1000) / (RT_TICK_PER_SECOND);
}

void asr_srand(unsigned seed);
int asr_rand(void);

#define srand asr_srand
#define rand  asr_rand

#endif
