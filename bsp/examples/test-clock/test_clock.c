/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0*
 *
 * Authors: Senye.Liang <senye.liang@artinchip.com>
 */
#include <string.h>
#include <ctype.h>
#include "aic_core.h"
#include "aic_clk_id.h"
#include "aic_hal_clk.h"

enum cmu_cmd{
    GET_INFO,
    GET_PARENT,
    SET_FREQ,
    SET_DISABLE,
    SET_ENABLE,
};

static int __get_info_byid(int id, char floors)
{
    const char *name = NULL;
    unsigned long rate;
    int status = 0;

    name = hal_clk_get_name(id);
    if (name == NULL)
        return -1;

    rate = hal_clk_get_freq(id);
    status = hal_clk_is_enabled(id);

    if (floors == 0) {
        printf("    %-6ld      %-14s      %-10lu      %10d\n",
               (long int)id, name, rate, status);
    } else {
        printf("    %-6ld    %*s%-*s      %-10lu      %8d\n",
               (long int)id, floors * 2, "", (19 - floors * 2), name, rate, status);
    }
    return 0;
}


static void get_all_info(int id, int *clk_status, char *floors)
{
    int i;

    (*floors)++;
    for (i = id; i < AIC_CLK_NUM; i++) {
        if (clk_status[i] == 1) {
            clk_status[i] = 2;
            __get_info_byid(i, *floors);
        }
    }

    for (i = id; i <= AIC_CLK_NUM; i++) {
        if (id == hal_clk_get_parent(i) && (clk_status[i] != 2)) {
            clk_status[i] = 1;
            get_all_info(i, clk_status, &(*floors));
            (*floors)--;
        }
     }
}

static int clk_dump(char *argv)
{
    int temp, id;
    char floors = 0;
    int clk_status[AIC_CLK_NUM] = {0};

    id = strtoul(argv, NULL, 10);
    if (id < 0 || id > AIC_CLK_NUM)
        id = 0;

    for (id = 0; id < AIC_CLK_NUM; id++) {
        temp = hal_clk_get_parent(id);
        if (temp < 0) {
            clk_status[id] = 1;
            get_all_info(id, clk_status, &floors);
            floors = 0;
        }
    }
    return 0;
}

/* RT-THREAD */
#if defined(RT_USING_FINSH)
#include <finsh.h>
#include <getopt.h>
static int __get_parent_byid(int id)
{
    int parent_id;

    parent_id = hal_clk_get_parent(id);
    if (parent_id < 0) {
        __get_info_byid(id, 0);
        return -1;
    }

    __get_parent_byid(parent_id);
    __get_info_byid(id, 0);

    return 0;
}

static int __name_to_id(char *name, int *id)
{
    const char *clk_name;

    for (unsigned long i = 0; i < AIC_CLK_NUM; i++) {
        clk_name = hal_clk_get_name(i);
        if (clk_name == NULL)
            continue;

        if (strcmp(clk_name, name) == 0) {
            *id = i;
            return 0;
        }
    }
    return -1;
}

static int __get_byname(char *name, int cmu_cmd)
{
    int find_flag = 0;
    const char *clk_name;

    for (int i = 0; i < AIC_CLK_NUM; i++) {
        clk_name = hal_clk_get_name(i);
        if (clk_name == NULL)
            continue;

        if (!strncmp(clk_name, name, strlen(name))) {
            switch (cmu_cmd) {
                case GET_INFO:
                    __get_info_byid(i, 0);
                    break;
                case GET_PARENT:
                    __get_parent_byid(i);
                    printf("\n");
                    break;
                default:
                    break;
            }
            find_flag = 1;
        }
    }

    if (!find_flag) {
        printf("clk name is wrong.\n");
        return -1;
    }
    return 0;
}

static void clk_set_opera(char *argv, char cmu_cmd)
{
    int ret, id = 0;
    unsigned long rate;

    if (!isalpha((unsigned char)*argv)) {
        id = strtoul(argv, NULL, 10);
    } else {
        if (__name_to_id(argv, &id) < 0) {
            printf("\n[clk_set_opera] name_to_id fail.\n");
            return;
        }
    }
    rate = strtoul((argv + strlen(argv) + 1), NULL, 10);

    switch (cmu_cmd) {
        case SET_FREQ:
            ret = hal_clk_set_freq(id, rate);
            if (ret < 0) {
                printf("\nset clk frequency fail\n");
                return;
            }
            break;
        case SET_DISABLE:
            ret = hal_clk_disable(id);
            if (ret < 0) {
                printf("\nset clk disable fail\n");
                return;
            }
            break;
        case SET_ENABLE:
            ret = hal_clk_enable(id);
            if (ret < 0) {
                printf("\nset clk enable fail\n");
                return;
            }
            break;
        default:
            printf("\ninput status parameter error\n");
    }

    __get_info_byid(id, 0);
}

static int clk_get_info(char *argv, int cmu_cmd)
{
    int ret = 0;
    unsigned int id = 0;

    id = strtoul(argv, NULL, 10);
    ret = isalpha((unsigned char)*argv);

    switch (cmu_cmd) {
        case GET_INFO:
            (ret == 0) ? __get_info_byid(id, 0):__get_byname(argv, GET_INFO);
            break;
        case GET_PARENT:
            (ret == 0) ? __get_parent_byid(id):__get_byname(argv, GET_PARENT);
            break;
        default:
            break;
    }

	return 0;
}

static void cmd_cmu_usage(char *program)
{
    printf("Usage: %s [options]\n", program);
    printf("\t -a, \tall clk info \n");
    printf("\t -f [id] or [name],       \tget clk info by id or name\n");
    printf("\t -p [id] or [name],       \tget clk parent by id or name\n");
    printf("\t -s [id] or [name] [rate],\tset clk rate by id or name\n");
    printf("\t -e [id] or [name],       \tset enable by id or name\n");
    printf("\t -d [id] or [name],       \tset disable by id or name\n");
    printf("\t -h ,\tusage\n");
}

static char sopts[] = "af:p:s:e:d:h";
static struct option lopts[] = {
    {"-a all clk info ",    no_argument, NULL, 'a'},
    {"-f clk info ",        required_argument, NULL, 'f'},
    {"-p clk parent",       required_argument, NULL, 'p'},
    {"-s set rate",         required_argument, NULL, 's'},
    {"-e set enable",       required_argument, NULL, 'e'},
    {"-d set disable",      required_argument, NULL, 'd'},
    {"-h help",             no_argument, NULL, 'h'},
    {0, 0, 0, 0}
    };

static int cmd_test_cmu(int argc, char **argv)
{
    int opt;

    printf("--------------------------------------------------------------------------\n");
	printf("Clk-ID    |        NAME        |        Hz        |        enable        |\n");

    if (argc < 2) {
        clk_dump("0");
        goto __out;
    }

    optind = 0;
    while ((opt = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (opt) {
        case 'a':
            clk_dump("0");
            break;
        case 'f':
            clk_get_info(optarg, GET_INFO);
            break;
        case 'p':
            clk_get_info(optarg, GET_PARENT);
            break;
        case 's':
            clk_set_opera(optarg, SET_FREQ);
            break;
        case 'e':
            clk_set_opera(optarg, SET_ENABLE);
            break;
        case 'd':
            clk_set_opera(optarg, SET_DISABLE);
            break;
        case'h':
        default:
            cmd_cmu_usage(argv[0]);
            break;
        }
    }

__out:
    printf("\n---------------------------------------------------------------------------\n");

    return 0;
}

MSH_CMD_EXPORT_ALIAS(cmd_test_cmu, test_clock, Test CMU CLK);
/* BAREMETAL */
#elif defined(AIC_CONSOLE_BARE_DRV)
#include <console.h>
static int cmd_test_cmu_bare(int argc, char *argv[])
{
    clk_dump("0");
    return 0;
}
CONSOLE_CMD(test_clock, cmd_test_cmu_bare, "Test CMU CLK.");
#endif
