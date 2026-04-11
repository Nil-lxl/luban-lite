/*
 * Copyright (C) 2023-2024 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Huahui Mai <huahui.mai@artinchip.com>
 *
 */

#include <rtconfig.h>
#ifdef RT_USING_FINSH
#include <rthw.h>
#include <rtthread.h>
#include <string.h>
#include <stdlib.h>
#include <finsh.h>
#include "lvgl.h"

#include "elevator_ui.h"
#include "elevator_uart.h"
#include "aic_core.h"

#define UART_DEVICE     "uart1"
#define UART_MAX_COUNT  64
#define DATA_MAX_COUNT  12

rt_device_t elevator_uart;
static struct rt_semaphore rx_sem;
static rt_mutex_t dynamic_mutex = RT_NULL;

static LIST_HEAD(task);

#define UART_IOCTL_DEF(ioctl, _ui_func)	            \
	[UART_IOCTL_NR(ioctl)] = {		                \
		.cmd = ioctl,			                    \
		.ui_func = _ui_func,			            \
		.name = #ioctl			                    \
	}

/* Ioctl table */
static const struct uart_ioctl_desc uart_ioctls[] = {
    UART_IOCTL_DEF(UART_IOCTL_UP_DOWN, inside_up_down_ioctl),
    UART_IOCTL_DEF(UART_IOCTL_LEVEL, inside_level_ioctl),
    UART_IOCTL_DEF(UART_IOCTL_REPLAY, replayer_ioctl),
};

#define UART_IOCTL_COUNT	ARRAY_SIZE(uart_ioctls)

/**
 * DOC: uart specific ioctls
 *
 * When UART_DEVICE receives a command, elevator ui will executes a specific
 * ioctls.
 *
 * Command format: AA ioctl_id data...
 *
 * Command table:
 * AA 00 00/01 : elevator up/down
 * AA 01 level : elevator level
 * AA 02 00 : elevator player video
 */

rt_err_t elevator_uart_input(rt_device_t dev, rt_size_t size)
{
    if (size > 0)
        rt_sem_release(&rx_sem);

    return RT_EOK;
}

static inline bool check_uart_receive_over(int *count, int *code)
{
    if (*count == UART_MAX_COUNT || (*code == 1 && *count > DATA_MAX_COUNT))
    {
        *count = 0;
        *code = 0;

        return true;
    }
    return false;
}

static inline bool check_uart_receive_done(int *code, char ch)
{
    if (*code == 1 && ch == '\n')
    {
        *code = 0;

        return true;
    }
    return false;
}

int uart_add_disp_task(char *str_rx, int data_len)
{
    struct disp_task *disp_task = NULL;
    int id = atoi(str_rx);

    if (id >= UART_IOCTL_COUNT)
    {
        rt_kprintf("invalid id: %d, uart ioctl count %d\n", id, UART_IOCTL_COUNT);
        return -1;
    }

    disp_task = aicos_malloc(MEM_CMA, sizeof(struct disp_task));
    if (!disp_task)
    {
        rt_kprintf("alloc task failed\n");
        return -1;
    }

    disp_task->ioctl = &uart_ioctls[id];
    disp_task->data_len = data_len - 4;
    memcpy(disp_task->data, str_rx + 4, data_len - 4);

    list_add_tail(&disp_task->next, &task);
    return 0;
}

void elevator_uart_rx_thread_entry(void *parameter)
{
    char ch;
    char str_rx[UART_MAX_COUNT] = {0};
    int cnt = 0, code = 0;

    while (1)
    {
        while (rt_device_read(elevator_uart, -1, &ch, 1) != 1)
        {
            str_rx[cnt] = ch;
            cnt++;

            if (check_uart_receive_over(&cnt, &code))
                goto next;

            if (check_uart_receive_done(&code, ch))
            {
                    rt_mutex_take(dynamic_mutex, RT_WAITING_FOREVER);

                    uart_add_disp_task(str_rx, cnt);
                    cnt = 0;
                    rt_mutex_release(dynamic_mutex);
            }

            /* Got AA code */
            if (cnt >= 2 && (str_rx[cnt -2] == 'A' && str_rx[cnt - 1] == 'A'))
            {
                memset(str_rx, 0, cnt);
                cnt = 0;
                code = 1;
            }
next:
            rt_sem_take(&rx_sem, RT_WAITING_FOREVER);
        }
    }
}

static void command_callback(lv_timer_t *tmr)
{
    const struct uart_ioctl_desc *ioctl = NULL;
    struct disp_task *t, *node;
	uart_ui_ioctl_t *func;

    rt_mutex_take(dynamic_mutex, RT_WAITING_FOREVER);

    if (list_empty(&task))
    {
        rt_mutex_release(dynamic_mutex);
        return;
    }

    list_for_each_entry_safe(t, node, &task, next) {
        ioctl = t->ioctl;
        func = ioctl->ui_func;

        if (func)
            func(t->data, t->data_len);

        pr_debug("ioctl cmd: %d, name: %s\n", ioctl->cmd, ioctl->name);

        list_del(&t->next);
        aicos_free(MEM_CMA, t);
    }

    rt_mutex_release(dynamic_mutex);
}

int evevator_uart_main(void)
{
    int ret;

    elevator_uart = rt_device_find(UART_DEVICE);
    if (!elevator_uart)
    {
        rt_kprintf("find %s failed!\n", UART_DEVICE);
        return -RT_ERROR;
    }

    ret = rt_device_open(elevator_uart,
                            RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    if (ret != RT_EOK)
    {
        rt_kprintf("open %s failed : %d !\n", UART_DEVICE, ret);
        return -RT_ERROR;
    }

    rt_device_set_rx_indicate(elevator_uart, elevator_uart_input);
    rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);

    rt_thread_t thread_rx = rt_thread_create("uart_rx",
                                                elevator_uart_rx_thread_entry,
                                                RT_NULL,
                                                1024 * 8,
                                                15,
                                                15);
    if (thread_rx != RT_NULL)
    {
        rt_thread_startup(thread_rx);
    }
    else
    {
        rt_device_close(elevator_uart);
        return -RT_ERROR;
    }

    dynamic_mutex = rt_mutex_create("dmutex", RT_IPC_FLAG_PRIO);
    if (dynamic_mutex == RT_NULL)
    {
        rt_kprintf("create dynamic mutex failed.\n");
        return -1;
    }

    lv_timer_create(command_callback, 150, 0);

    return 0;
}

#endif /* RT_USING_FINSH */
