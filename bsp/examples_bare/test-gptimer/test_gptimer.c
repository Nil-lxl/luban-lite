/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <console.h>
#include "aic_getopt.h"
#include "aic_common.h"
#include "drv_gptimer.h"
#include "boot_param.h"

static bool g_gpt_init_flag = 0;

#ifndef USEC_PER_SEC
#define USEC_PER_SEC            1000000
#endif

#define GPTIMER_MAX_ELAPSE      (60 * USEC_PER_SEC) // 60 sec

#define TIMER_NUM     AIC_GPTIMER_CH_NUM

static bool g_debug[TIMER_NUM] = {0};
static u32 g_loop_max[TIMER_NUM] = {0};
static u32 g_loop_cnt[TIMER_NUM] = {0};
static ulong g_start_us[TIMER_NUM] = {0};
static u32 g_cb_para[TIMER_NUM] = {0};

static const char sopts[] = "m:c:s:u:g:a:f:dh";
static const struct option lopts[] = {
    {"mode",        required_argument, NULL, 'm'},
    {"channel",     required_argument, NULL, 'c'},
    {"sec",         required_argument, NULL, 's'},
    {"microsecond", required_argument, NULL, 'u'},
    {"gptmode",     required_argument, NULL, 'g'},
    {"trgmode",     required_argument, NULL, 'a'},
    {"frequency",   required_argument, NULL, 'f'},
    {"debug",             no_argument, NULL, 'v'},
    {"usage",             no_argument, NULL, 'h'},
    {0, 0, 0, 0}
};

static void usage(char *program)
{
    printf("Usage: %s [options]: \n", program);
    printf("\t -m, --mode\t\tmode of timer, oneshot/period\n");
    printf("\t -c, --channel\t\tthe number of gptimer [0, %d]\n", TIMER_NUM);
    printf("\t -s, --second\t\tthe second of timer (must > 0)\n");
    printf("\t -u, --microsecond\tthe microsecond of timer (must > 0)\n");
    printf("\t -g, --gptmode\t\tthe mode of gptimer, count/match\n");
    printf("\t -a, --triggermode\tthe trigger mode of gptimer, auto/rsi/fall/bil\n");
    printf("\t -f, --frequency\tthe frequncy of the gptimer (must > 0)\n");
    printf("\t -d, --debug\t\tshow the timeout log\n");
    printf("\t -h, --usage \n");
    printf("\n");
    printf("Example: %s -m period -c 0 -s 1 -u 3 -g count -a auto -f 1000000 -d \n", program);
}

struct gptimer_match_out g_outval[GPT_OUT_NUMS] = {
    {1, OUT_INIT_LOW, CMP_OUT_HIGH, CMP_OUT_LOW},
    {1, OUT_INIT_HIGH, CMP_OUT_LOW, CMP_OUT_HIGH},
    {0, OUT_INIT_LOW, CMP_OUT_HIGH, CMP_OUT_LOW},
    {0, OUT_INIT_LOW, CMP_OUT_HIGH, CMP_OUT_LOW},
};

enum gpt_cmp_act g_cmpa_act = GPTIMER_CNT_CONTINUE;
enum gpt_cmp_act g_cmpb_act = GPTIMER_CNT_CONTINUE;

/* Timer timeout callback function */
void gptimer_cb(void *arg)
{
    u32 ch = *((u32 *)arg);

    if (g_debug[ch])
        printf("%d/%d gptimer%d timeout callback! Elapsed %ld us\n",
               g_loop_cnt[ch], g_loop_max[ch],
               ch, aic_timer_get_us() - g_start_us[ch]);

    g_start_us[ch] = aic_timer_get_us();
    g_loop_cnt[ch]++;
    if ((g_loop_max[ch] > 1) && (g_loop_cnt[ch] > g_loop_max[ch]))
        drv_gptimer_stop(ch);

}

static int cmd_test_gptimer(int argc, char *argv[])
{
    u32 c, ch = 0;
    struct gptimer_work_para para = {0};
    enum gpt_work_mode mode = MODE_ONESHOT;
    enum gptimer_mode gpt_mode = GPTIMER_MODE_COUNT;
    enum gpt_trg_mode trg_mode = GPT_TRG_MODE_AUTO;
    u32 freq= 1000000;
    bool debug_flag = 0;

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'm':
            if (strncasecmp("period", optarg, strlen(optarg)) == 0)
                mode = MODE_PERIOD;
            continue;

        case 'c':
            ch = atoi(optarg);
            if (ch > TIMER_NUM) {
                printf("Channel number %s is invalid\n", optarg);
                return 0;
            }
            g_cb_para[ch] = ch;
            continue;

        case 's':
            para.tm.sec = atoi(optarg);
            continue;

        case 'u':
            para.tm.usec = atoi(optarg);
            continue;

        case 'd':
            debug_flag = 1;
            continue;

        case 'g':
            if (strncasecmp("count", optarg, strlen(optarg)) == 0)
                gpt_mode = GPTIMER_MODE_COUNT;
            else if (strncasecmp("match", optarg, strlen(optarg)) == 0)
                gpt_mode = GPTIMER_MODE_MATCH;
            continue;

        case 'a':
            if (strncasecmp("auto", optarg, strlen(optarg)) == 0)
                trg_mode = GPT_TRG_MODE_AUTO;
            else if (strncasecmp("rsi", optarg, strlen(optarg)) == 0)
                trg_mode = GPT_TRG_MODE_RSI;
            else if (strncasecmp("fall", optarg, strlen(optarg)) == 0)
                trg_mode = GPT_TRG_MODE_FALL;
            else if (strncasecmp("bil", optarg, strlen(optarg)) == 0)
                trg_mode = GPT_TRG_MODE_BILATERAL;
            continue;

        case 'f':
            freq = atoi(optarg);
            continue;

        case 'h':
            usage(argv[0]);
            return 0;

        default:
            printf("Invalid argument\n");
            usage(argv[0]);
            return 0;
        }
    }

    if (!g_gpt_init_flag) {
        drv_gptimer_init();
        g_gpt_init_flag = 1;
    }

    if (debug_flag)
        g_debug[ch] = 1;
    else
        g_debug[ch] = 0;

    if ((para.tm.sec == 0) && (para.tm.usec == 0)) {
        printf("Invalid time value.\n");
        usage(argv[0]);
        return 0;
    }

    para.gpt_para.gptimer_mode = gpt_mode;
    para.gpt_para.gptimer_trgmode = trg_mode;

    if (gpt_mode == GPTIMER_MODE_MATCH) {
        para.gpt_para.matchval.cmpa_act = g_cmpa_act;
        para.gpt_para.matchval.cmpb_act = g_cmpb_act;
        memcpy(&para.gpt_para.matchval.outval[0], g_outval, sizeof(struct gptimer_match_out) * 4);
    }

    /* stop the timer first */
    drv_gptimer_stop(ch);

    /* set the timer frequency to freqHz */
    drv_gptimer_freq_set(ch, freq);

    printf("gptimer%d: Create a timer of %d.%06d sec, %s mode\n",
           ch, (u32)para.tm.sec, (u32)para.tm.usec,
           mode == MODE_ONESHOT ? "Oneshot" : "Period");

    g_loop_max[ch] = 0;

    if (mode != MODE_ONESHOT) {
        g_loop_max[ch] = GPTIMER_MAX_ELAPSE / (para.tm.sec * USEC_PER_SEC + para.tm.usec);
        printf("\tWill loop %d times\n", g_loop_max[ch]);
    }

    g_loop_cnt[ch] = 0;

    para.gpt_para.gptimer_cb.func = gptimer_cb;
    para.gpt_para.gptimer_cb.para = (void *)&g_cb_para[ch];

    g_start_us[ch] = aic_timer_get_us();

    drv_gptimer_start(ch, mode, &para);

    return 0;

}

CONSOLE_CMD(test_gptimer, cmd_test_gptimer,  "GPTimer test example");

