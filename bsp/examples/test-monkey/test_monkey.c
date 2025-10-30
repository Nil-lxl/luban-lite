/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Siyao.Li <siyao.li@artinchip.com>
 */
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include <rtthread.h>
#include <fcntl.h>
#include <unistd.h>

#include "touch.h"
#include "lv_port_indev.h"
#include "mpp_fb.h"

#define THREAD_PRIORITY             25
#define THREAD_STACK_SIZE           4096
#define THREAD_TIMESLICE            5
#define COMB_LINE_NUM               10
#define TIME_INTERVAL_DEFAULT_VAL   100

static const char sopts[] = "DF:R::P::WT:U:N:A:ax:y:r:e:u:d:h";
static const struct option lopts[] = {
    {"mode_draw",                     no_argument, NULL, 'D'},
    {"mode_free_point",         required_argument, NULL, 'F'},
    {"mode_record",             optional_argument, NULL, 'R'},
    {"mode_play",               optional_argument, NULL, 'P'},
    {"mode_combination",              no_argument, NULL, 'W'},
    {"time_interval_line",      required_argument, NULL, 'T'},
    {"time_interval_point",     required_argument, NULL, 'U'},
    {"total_lines_drawn",       required_argument, NULL, 'N'},
    {"total_points_drawn",      required_argument, NULL, 'A'},
    {"comb_start_test",               no_argument, NULL, 'a'},
    {"comb_line_x",             required_argument, NULL, 'x'},
    {"comb_line_y",             required_argument, NULL, 'y'},
    {"comb_line_swipe_right",   required_argument, NULL, 'r'},
    {"comb_line_swipe_left",    required_argument, NULL, 'e'},
    {"comb_line_swipe_up",      required_argument, NULL, 'u'},
    {"comb_line_swipe_down",    required_argument, NULL, 'd'},
    {"help",                          no_argument, NULL, 'h'},
    {0, 0, 0, 0}
    };

static rt_thread_t  gt911_thread = RT_NULL;
static rt_sem_t     gt911_sem = RT_NULL;
static rt_device_t  dev = RT_NULL;
static struct rt_touch_data *read_data;
static struct rt_touch_info info;
static struct aicfb_screeninfo g_fb_info = {0};
static struct mpp_fb *g_fb = NULL;
static int g_xres = 0;
static int g_yres = 0;

static int g_mode_draw_gt911 = 0;
static int g_mode_free_point = 0;
static int g_mode_record_log = 0;
static int g_mode_play_log = 0;

static int g_total_line_drawn = 0;
static int g_total_point_drawn = 1;
static int g_time_interval_line = TIME_INTERVAL_DEFAULT_VAL;
static int g_time_interval_points = TIME_INTERVAL_DEFAULT_VAL;

static int g_comb_index = 0;
static int g_comb_start = 0;
static int g_comb_x1[COMB_LINE_NUM] = {0};
static int g_comb_y1[COMB_LINE_NUM] = {0};
static int g_comb_direct[COMB_LINE_NUM] = {0};
static int g_comb_length[COMB_LINE_NUM] = {0};
static int g_free_points_num = 0;
static char *g_record_path;
static char *g_play_path;
static int g_fd_record;

enum g_comb_direct_type {
    COMB_DIRECT_RIGHT = 0,
    COMB_DIRECT_LEFT  = 1,
    COMB_DIRECT_UP    = 2,
    COMB_DIRECT_DOWN  = 3,
};

static void cmd_monkey_usage(char *program)
{
    printf("Usage: %s [options]\n", program);
    printf("\t -D,                 --Use mode: draw gt911\n");
    printf("\t -F <points number>, --Use mode: free_points\n");
    printf("\t -R<file path>,      --Use mode: record_trajectory\n");
    printf("\t -P<file path>,      --Use mode: play_trajectory\n");
    printf("\t -W,                 --Use mode: painting combination\n");
    printf("\t -T <time interval>, --Time interval for drawing lines(ms)\n");
    printf("\t -U <time interval>, --Time interval for drawing points(ms)\n");
    printf("\t -N <lines number>,  --Total number of lines drawn\n");
    printf("\t -A <points number>, --Total number of points drawn\n");
    printf("\t -a,                 --COMB MODE: Start test\n");
    printf("\t -x <coordinate>,    --COMB MODE: Set the x-coordinate\n");
    printf("\t -y <coordinate>,    --COMB MODE: Set the y-coordinate\n");
    printf("\t -r <length>, --COMB MODE: Draw line in the right-direction\n");
    printf("\t -e <width>,  --COMB MODE: Draw line in the left-direction\n");
    printf("\t -u <length>, --COMB MODE: Draw line in the up-direction\n");
    printf("\t -d <length>, --COMB MODE: Draw line in the down-direction\n");
    printf("\t -h, --help \n");
    printf("\n");
    printf("Example:\n");
    printf("COMB_MODE:%s -x 10 -y 10 -r 300 -W\n", program);
    printf("\t%s -T 10 -N 10 -a \n", program);
}

static void tp_callback(int x, int y)
{
    aic_touch_inputevent_cb(x, y, RT_TOUCH_MONKEY_TEST);
}

static void comb_swipe_right(int comb_cnt)
{
    int i = comb_cnt;

    rt_kprintf("Type%d: x-right: Point(%d,%d) Length(%d)\n", i, g_comb_x1[i],
               g_comb_y1[i], g_comb_length[i]);
    for (int x_pos = g_comb_x1[i]; x_pos < g_comb_length[i]; ++x_pos) {
        tp_callback(x_pos, g_comb_y1[i]);
        usleep(g_time_interval_points);
    }
}

static void comb_swipe_left(int comb_cnt)
{
    int i = comb_cnt;

    rt_kprintf("Type%d: x-left Point(%d,%d) Length(%d)\n", i, g_comb_x1[i],
               g_comb_y1[i], g_comb_length[i]);
    for (int x_pos = g_comb_x1[i]; g_comb_x1[i] - x_pos < g_comb_length[i]; --x_pos) {
        tp_callback(x_pos, g_comb_y1[i]);
        usleep(g_time_interval_points);
    }
}

static void comb_swipe_up(int comb_cnt)
{
    int i = comb_cnt;

    rt_kprintf("Type%d: y-up: Point(%d,%d) Length(%d)\n", i, g_comb_x1[i],
               g_comb_y1[i], g_comb_length[i]);
    for (int y_pos = g_comb_y1[i]; g_comb_y1[i] - y_pos > g_comb_length[i]; --y_pos) {
        tp_callback(g_comb_x1[i], y_pos);
        usleep(g_time_interval_points);
    }
}

static void comb_swipe_down(int comb_cnt)
{
    int i = comb_cnt;

    rt_kprintf("Type%d: y-down: Point(%d,%d) Length(%d)\n", i, g_comb_x1[i],
               g_comb_y1[i], g_comb_length[i]);
    for (int y_pos = g_comb_y1[i]; y_pos < g_comb_length[i]; ++y_pos) {
        tp_callback(g_comb_x1[i], y_pos);
        usleep(g_time_interval_points);
    }
}

static void draw_mode_combination(void)
{
    int comb_cnt = 0;

    while(g_total_line_drawn--) {
        rt_kprintf("[%d]\n", comb_cnt);
        for (int i = 0; i < g_comb_index; i++) {
            aicos_msleep(g_time_interval_line);
            switch (g_comb_direct[i]) {
                case COMB_DIRECT_RIGHT:
                    comb_swipe_right(i);
                    break;
                case COMB_DIRECT_LEFT:
                    comb_swipe_left(i);
                    break;
                case COMB_DIRECT_UP:
                    comb_swipe_up(i);
                    break;
                case COMB_DIRECT_DOWN:
                    comb_swipe_down(i);
                    break;
                default:
                    break;
            }
        }
        comb_cnt++;
    }
}

static void draw_mode_free_point(void)
{
    int random_x;
    int random_y;
    int free_point_cnt = 0;

    srand(time(NULL));
    while (g_free_points_num--) {
        random_x = rand() % g_xres;
        random_y = rand() % g_yres;
        rt_kprintf("[%d] x %d y %d\n", free_point_cnt, random_x, random_y);
        tp_callback(random_x, random_y);
        free_point_cnt++;
    }
}

static int monkey_get_fb_info(void)
{
    int ret = 0;

    g_fb = mpp_fb_open();
    if (!g_fb) {
        pr_err("mpp_fb_open error!\n");
        return -1;
    }

    ret = mpp_fb_ioctl(g_fb, AICFB_GET_SCREENINFO, &g_fb_info);
    if (ret < 0) {
        pr_err("ioctl() failed! errno: -%d\n", -ret);
        return -1;
    }

    pr_info("Screen width: %d, height: %d\n",
            g_fb_info.width, g_fb_info.height);

    g_xres = g_fb_info.width;
    g_yres = g_fb_info.height;

    return ret;
}

static void gt911_entry(void *parameter)
{
    int point_cnt = 0;
    int point_data_size;
    rt_uint16_t *cal_buf;

    point_data_size = 2 * g_total_point_drawn * 3;
    cal_buf = (rt_uint16_t *)malloc(sizeof(rt_uint16_t) * point_data_size);
    memset(cal_buf, 0, point_data_size * sizeof(rt_uint16_t));

    rt_device_control(dev, RT_TOUCH_CTRL_GET_INFO, &info);

    read_data = (struct rt_touch_data *)rt_malloc(sizeof(struct rt_touch_data) * info.point_num);
    memset(read_data, 0, sizeof(*read_data));
    rt_kprintf("Starting record %d points\n",g_total_point_drawn);

    while (point_cnt < g_total_point_drawn) {
        rt_sem_take(gt911_sem, RT_WAITING_FOREVER);
        int num = 0;
#ifdef AIC_TOUCH_PANEL_GT911
        num = rt_device_read(dev, 0, read_data, info.point_num);
#endif
        if (num != info.point_num) {
            rt_device_control(dev, RT_TOUCH_CTRL_ENABLE_INT, RT_NULL);
            continue;
        }
        for (rt_uint8_t i = 0; i < info.point_num; i++) {
            if (read_data[i].event != RT_TOUCH_EVENT_DOWN
                && read_data[i].event != RT_TOUCH_EVENT_UP
                && read_data[i].event != RT_TOUCH_EVENT_MOVE)
                continue;

            rt_uint16_t  u16X, u16Y;

            u16X = read_data[i].x_coordinate;
            u16Y = read_data[i].y_coordinate;

            rt_kprintf("[%d] %d %d type-%d\n", point_cnt, u16X, u16Y,
                       read_data[i].event);
            if (g_mode_record_log <= 0) {
                point_cnt++;
                aic_touch_inputevent_cb(u16X, u16Y, read_data[i].event);
                continue;
            }
            if (point_cnt < g_total_point_drawn) {
                memcpy(cal_buf + point_cnt * 3 * sizeof(rt_uint16_t),
                       &read_data[i].event, sizeof(rt_uint16_t));
                memcpy(cal_buf + (1 + point_cnt * 3) * sizeof(rt_uint16_t),
                       &u16X, sizeof(rt_uint16_t));
                memcpy(cal_buf + (2 + point_cnt * 3) *sizeof(rt_uint16_t),
                       &u16Y, sizeof(rt_uint16_t));
            }
            if (point_cnt == g_total_point_drawn - 1) {
                rt_kprintf("Finish record!\n");
                write(g_fd_record, cal_buf,
                      point_data_size * sizeof(rt_uint16_t));
                close(g_fd_record);
                g_mode_record_log = 0;
            }
            point_cnt++;
            aic_touch_inputevent_cb(u16X, u16Y, read_data[i].event);
        }
        rt_device_control(dev, RT_TOUCH_CTRL_ENABLE_INT, RT_NULL);
    }
    free(cal_buf);
    rt_sem_delete(gt911_sem);
    rt_device_close(dev);
    g_total_point_drawn = 0;
    rt_thread_delete(gt911_thread);
}

static rt_err_t rx_callback(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(gt911_sem);
    rt_device_control(dev, RT_TOUCH_CTRL_DISABLE_INT, RT_NULL);
#ifdef AIC_PM_DEMO
    extern struct rt_event pm_event;
    rt_event_send(&pm_event, 2);
#endif
    return 0;
}

int monkey_tpc_run(const char *name, rt_uint16_t x, rt_uint16_t y)
{
    void *id;

    dev = rt_device_find(name);
    if (dev == RT_NULL)
    {
        rt_kprintf("can't find device:%s\n", name);
        return -1;
    }

    if (rt_device_open(dev, RT_DEVICE_FLAG_INT_RX) != RT_EOK)
    {
        rt_kprintf("open device failed!");
        return -1;
    }

    id = rt_malloc(sizeof(rt_uint8_t) * 8);
    rt_device_control(dev, RT_TOUCH_CTRL_GET_ID, id);
    rt_uint8_t * read_id = (rt_uint8_t *)id;
    rt_kprintf("id = GT%d%d%d \n", read_id[0] - '0', read_id[1] - '0',
               read_id[2] - '0');

    rt_device_control(dev, RT_TOUCH_CTRL_GET_INFO, id);
    rt_kprintf("range_x = %d \n", (*(struct rt_touch_info*)id).range_x);
    rt_kprintf("range_y = %d \n", (*(struct rt_touch_info*)id).range_y);
    rt_kprintf("point_num = %d \n", (*(struct rt_touch_info*)id).point_num);
    rt_free(id);
    rt_device_set_rx_indicate(dev, rx_callback);
    gt911_sem = rt_sem_create("dsem", 1, RT_IPC_FLAG_FIFO);

    if (gt911_sem == RT_NULL)
    {
        rt_kprintf("create dynamic semaphore failed.\n");
        return -1;
    }

    gt911_thread = rt_thread_create("gt911", gt911_entry, RT_NULL,
                                    THREAD_STACK_SIZE, THREAD_PRIORITY,
                                    THREAD_TIMESLICE);

    if (gt911_thread != RT_NULL)
        rt_thread_startup(gt911_thread);

    return 0;
}

static void cmd_test_monkey(int argc, char **argv)
{
    int c;
    if (argc < 2) {
        cmd_monkey_usage(argv[0]);
        return;
    }
    if (g_xres == 0 && g_yres == 0)
            monkey_get_fb_info();

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'D':
            g_mode_draw_gt911 = 1;
            continue;
        case 'F':
            g_mode_free_point = 1;
            g_free_points_num = atoi(optarg);
            continue;
        case 'R':
            g_mode_record_log = 1;
            g_record_path = optarg;
            rt_kprintf("Set record path: %s\n", g_record_path);
            continue;
        case 'P':
            g_mode_play_log = 1;
            g_play_path = optarg;
            rt_kprintf("Select play path: %s\n", g_play_path);
            continue;
        case 'W':
            g_comb_index++;
            rt_kprintf("Set the combination line %d\n", g_comb_index);
            continue;
        case 'T':
            g_time_interval_line = atoi(optarg);
            continue;
        case 'U':
            g_time_interval_points = atoi(optarg);
            continue;
        case 'N':
            g_total_line_drawn = atoi(optarg);
            continue;
        case 'A':
            g_total_point_drawn = atoi(optarg);
            continue;
        case 'a':
            g_comb_start = 1;
            continue;
        case 'x':
            g_comb_x1[g_comb_index] = atoi(optarg);
            continue;
        case 'y':
            g_comb_y1[g_comb_index] = atoi(optarg);
            continue;
        case 'r':
            g_comb_direct[g_comb_index] = COMB_DIRECT_RIGHT;
            g_comb_length[g_comb_index] = atoi(optarg);
            continue;
        case 'e':
            g_comb_direct[g_comb_index] = COMB_DIRECT_LEFT;
            g_comb_length[g_comb_index] = atoi(optarg);
            continue;
        case 'u':
            g_comb_direct[g_comb_index] = COMB_DIRECT_UP;
            g_comb_length[g_comb_index] = atoi(optarg);
            continue;
        case 'd':
            g_comb_direct[g_comb_index] = COMB_DIRECT_DOWN;
            g_comb_length[g_comb_index] = atoi(optarg);
            continue;
        case 'h':
        default:
            cmd_monkey_usage(argv[0]);
            continue;
        }
    }

    if (g_comb_start) {
        draw_mode_combination();
        g_comb_start = 0;
    }

    if (g_mode_free_point && g_free_points_num) {
        draw_mode_free_point();
        g_mode_free_point = 0;
    }

    if (g_mode_draw_gt911) {
        monkey_tpc_run("gt911", g_xres, g_yres);
        g_mode_draw_gt911 = 0;
    }

    if (g_mode_record_log) {
        g_fd_record = open(g_record_path, O_WRONLY | O_CREAT);

        monkey_tpc_run("gt911", g_xres, g_yres);
    }

    if (g_mode_play_log) {
        int point_cnt;
        int line_cnt  = 0;
        int fd_play;
        rt_uint16_t *buf;
        int x_play, y_play;
        int point_data_size;

        point_data_size = 2 * g_total_point_drawn * 3;
        buf = (rt_uint16_t *)malloc(sizeof(rt_uint16_t) * point_data_size);

        memset(buf, 0, point_data_size * sizeof(rt_uint16_t));

        fd_play = open(g_play_path, O_RDONLY);
        if (fd_play < 0)
            return;

        read(fd_play, buf, point_data_size  * sizeof(rt_uint16_t));
        g_total_line_drawn = g_total_point_drawn;

        while(line_cnt < g_total_line_drawn) {
            rt_kprintf("Number of drawing lines [%d]\n", line_cnt);
            line_cnt++;
            for (point_cnt = 0; point_cnt < g_total_point_drawn; point_cnt++) {
                x_play = *(rt_uint16_t *)(buf + (1 + 3 * point_cnt)
                         * sizeof(rt_uint16_t));
                y_play = *(rt_uint16_t *)(buf + (2 + 3 * point_cnt)
                         * sizeof(rt_uint16_t));
                rt_kprintf("[%d] %d %d\n", point_cnt, x_play, y_play);
                tp_callback(x_play, y_play);
                usleep(g_time_interval_points);
                }

        }
        close(fd_play);
        rt_kprintf("Finish play!\n");

        g_mode_play_log = 0;
    }

    return;
}

MSH_CMD_EXPORT_ALIAS(cmd_test_monkey, test_monkey, monkey sample);
