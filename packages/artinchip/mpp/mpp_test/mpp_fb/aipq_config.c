/*
 * Copyright (C) 2024-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: ArtInChip
 */

#include <rtconfig.h>
#ifdef RT_USING_FINSH
#include <rthw.h>
#include <rtthread.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <aic_core.h>
#include <aic_hal_dsi.h>

#include "mpp_fb.h"

#define MIPI_DSI_DISABLE_COMMAND     0
#define MIPI_DSI_ADB_UPDATE_COMMAND  1
#define MIPI_DSI_SEND_COMMAND        2
#define MIPI_DSI_UART_UPDATE_COMMAND  3
#define MIPI_DSI_UART_COPY_COMMAND   4

#define MIPI_DBI_UPDATE_COMMAND  0
#define MIPI_DBI_SEND_COMMAND    1

#define RGB_SPI_COMMAND_UPDATE  0
#define RGB_SPI_COMMAND_DISABLE 1
#define RGB_SPI_COMMAND_CLEAR   2
#define RGB_SPI_COMMAND_SEND    3

enum AIC_COM_TYPE {
    AIC_DE_COM   = 0x00,  /* display engine component */
    AIC_RGB_COM  = 0x01,  /* rgb component */
    AIC_LVDS_COM = 0x02,  /* lvds component */
    AIC_MIPI_COM = 0x03,  /* mipi dsi component */
    AIC_DBI_COM  = 0x04,  /* mipi dbi component */
};

struct panel_dbi_commands {
    const u8 *buf;
    size_t len;
    unsigned int command_flag;
};

struct spi_cfg {
    unsigned int qspi_mode;
    unsigned int vbp_num;
    unsigned int code1_cfg;
    unsigned int code[3];
};

struct panel_dbi {
    unsigned int type;
    unsigned int format;
    unsigned int first_line;
    unsigned int other_line;
    struct panel_dbi_commands commands;
    struct spi_cfg *spi;
};

struct rgb_spi_command {
    unsigned int len;
    unsigned int command_on;
    unsigned char *buf;
};

struct panel_rgb {
    unsigned int mode;
    unsigned int format;
    unsigned int clock_phase;
    unsigned int data_order;
    unsigned int data_mirror;
    struct rgb_spi_command spi_command;
};

struct panel_lvds {
    unsigned int mode;
    unsigned int link_mode;
    unsigned int link_swap;
    unsigned int pols[2];
    unsigned int lanes[2];
};

struct dsi_command
{
    unsigned int len;
    unsigned int command_on;
    unsigned char *buf;
};

struct panel_dsi {
    enum dsi_mode mode;
    enum dsi_format format;
    unsigned int lane_num;
    unsigned int vc_num;
    unsigned int dc_inv;
    unsigned int ln_polrs;
    unsigned int ln_assign;
    struct dsi_command command;
};

static inline int char2int(char ch)
{
    return ch - '0';
}

static long long int str2int(char *_str)
{
    if (_str == NULL) {
        pr_err("The string is empty!\n");
        return -1;
    }

    if (strncmp(_str, "0x", 2))
        return atoi(_str);
    else
        return strtoll(_str, NULL, 16);
}

static unsigned char hex_char_to_u8(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');
    else if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    else
        return 0;
}

static unsigned char hex_pair_to_u8(char c1, char c2) {
    return (hex_char_to_u8(c1) << 4) | hex_char_to_u8(c2);
}

static void usage(char *app)
{
    printf("Usage: %s [Options], built on %s %s\n", app, __DATE__, __TIME__);
    printf("\t-u, --usage \n");
    printf("\t-t, --timing: config display timing \n");
    printf("\t-c, --connector: config display interface \n");
    printf("\t-g, --get: get aipq tools config \n");
    printf("\t-p, --pin: set or clr gpio \n");
    printf("\t-i, --input: input dsi command file \n");
    printf("\t-m, --merge the dsi command \n");
    printf("\t-a, --analysis for dbi command \n");
    printf("\t-s, --spi init command for rgb panel \n");
    printf("\n");
    printf("Example: %s \n", app);
}

static int get_file_size(const char *path)
{
    struct stat st;

    stat(path, &st);

    return st.st_size;
}

static void aicpq_print_config(struct aicfb_pq_config *config, int connector_type)
{
        printf("display timing:\n\tpixelclock: %d\n"
                "\thactive: %d\n"
                "\thfront_porch: %d\n"
                "\thback_porch: %d\n"
                "\thsync_len: %d\n"
                "\tvactive: %d\n"
                "\tvfront_porch: %d\n"
                "\tvback_porch: %d\n"
                "\tvsync_len: %d\n",
                config->timing->pixelclock,
                config->timing->hactive,
                config->timing->hfront_porch,
                config->timing->hback_porch,
                config->timing->hsync_len,
                config->timing->vactive,
                config->timing->vfront_porch,
                config->timing->vback_porch,
                config->timing->vsync_len);

        switch (connector_type)
        {
            case AIC_RGB_COM:
            {
                struct panel_rgb *rgb = config->data;

                printf("RGB info:\n"
                        "\tmode:%d\n"
                        "\tformat:%d\n"
                        "\tclock_phase:%d\n"
                        "\tdata_order:%08x\n"
                        "\tdata_mirror:%d\n",
                        rgb->mode,
                        rgb->format,
                        rgb->clock_phase,
                        rgb->data_order,
                        rgb->data_mirror);
                break;
            }
            case AIC_LVDS_COM:
            {
                struct panel_lvds *lvds = config->data;

                printf("lvds info:\n"
                        "\tmode:%d\n"
                        "\tlink_mode:%d\n"
                        "\tlink_swap:%d\n"
                        "\tlink_0_pols:%x\n"
                        "\tlink_1_pols:%x\n"
                        "\tlink_0_lanes:%x\n"
                        "\tlink_1_lanes:%x\n",
                        lvds->mode,
                        lvds->link_mode,
                        lvds->link_swap,
                        lvds->pols[0],
                        lvds->pols[1],
                        lvds->lanes[0],
                        lvds->lanes[1]
                        );
                break;
            }
            case AIC_MIPI_COM:
            {
                struct panel_dsi *dsi = config->data;

                printf("dsi info:\n"
                        "\tmode:%d\n"
                        "\tformat:%d\n"
                        "\tlane_num:%d\n"
                        "\tvc_num:%d\n"
                        "\tdc_inv:%d\n"
                        "\tln_polrs:%d\n"
                        "\tln_assign:%04x\n",
                        dsi->mode,
                        dsi->format,
                        dsi->lane_num,
                        dsi->vc_num,
                        dsi->dc_inv,
                        dsi->ln_polrs,
                        dsi->ln_assign);
                break;
            }
            case AIC_DBI_COM:
            {
                struct panel_dbi *dbi = config->data;
                unsigned int dbi_code = dbi->spi->code[0] << 16 |
                                        dbi->spi->code[1] << 8  |
                                        dbi->spi->code[2];

                printf("dbi info:\n"
                        "\ttype:%d\n"
                        "\tformat:%d\n"
                        "\tfirst_line:%x\n"
                        "\tother_line:%x\n"
                        "\tqspi_mode:%d\n"
                        "\tvbp_num:%d\n"
                        "\tcode1_cfg:%x\n"
                        "\tdbi_code:%x\n",
                        dbi->type,
                        dbi->format,
                        dbi->first_line,
                        dbi->other_line,
                        dbi->spi->qspi_mode,
                        dbi->spi->vbp_num,
                        dbi->spi->code1_cfg,
                        dbi_code
                        );
                break;
            }
            default:
                printf("Unknown Connector type: %d !\n", connector_type);
                break;
        }
}

#define AIPQ_TIMING_SRTING_LEN  29

static void aipq_parse_timing(struct display_timing *timing)
{
    char str_3[4] = {0};
    char str_4[5] = {0};
    int len = 0;

    len = strlen(optarg);
    if (len != AIPQ_TIMING_SRTING_LEN) {
        pr_err("Invaild timing table, table len %d\n", len);
        return;
    }

    strncpy(str_3, optarg + 0, 3);
    timing->pixelclock = str2int(str_3) * 1000 * 1000;

    strncpy(str_4, optarg + 3, 4);
    timing->hactive = str2int(str_4);

    strncpy(str_3, optarg + 7, 3);
    timing->hfront_porch = str2int(str_3);

    strncpy(str_3, optarg + 10, 3);
    timing->hback_porch = str2int(str_3);

    strncpy(str_3, optarg + 13, 3);
    timing->hsync_len = str2int(str_3);

    strncpy(str_4, optarg + 16, 4);
    timing->vactive = str2int(str_4);

    strncpy(str_3, optarg + 20, 3);
    timing->vfront_porch = str2int(str_3);

    strncpy(str_3, optarg + 23, 3);
    timing->vback_porch = str2int(str_3);

    strncpy(str_3, optarg + 26, 3);
    timing->vsync_len = str2int(str_3);
}

static int handle_dbi_command(struct panel_dbi *dbi, char *optarg,
                               void *fb, struct aicfb_pq_config *config)
{
    char dbi_src_num[64] = { 0 };
    u8 dbi_data[64] = { 0 };

    if (!dbi || !optarg || !fb || !config) {
        fprintf(stderr, "Invalid input parameters\n");
        return -1;
    }

    strncpy(dbi_src_num, optarg, sizeof(dbi_src_num) - 1);
    dbi_src_num[sizeof(dbi_src_num) - 1] = '\0';

    size_t input_len = strlen(dbi_src_num);
    if (input_len < 3) {
        fprintf(stderr, "Error: Input string too short\n");
        return -1;
    }

    unsigned char dbi_data_len = hex_char_to_u8(dbi_src_num[2]);
    size_t expected_len = 3 + 2 * dbi_data_len;

    if (input_len != expected_len) {
        fprintf(stderr, "Error: Expected %zu chars, got %zu\n", expected_len, input_len);
        return -1;
    }

    size_t dbi_data_size = 2 + dbi_data_len;
    if (dbi_data_size > sizeof(dbi_data)) {
        fprintf(stderr, "Error: Data exceeds buffer size\n");
        return -1;
    }

    dbi_data[0] = hex_pair_to_u8(dbi_src_num[0], dbi_src_num[1]);
    dbi_data[1] = dbi_data_len;

    for (int i = 0; i < dbi_data_len; ++i) {
        int pos = 3 + 2 * i;
        if (pos + 1 >= input_len) {
            fprintf(stderr, "Error: Data part incomplete\n");
            break;
        }
        dbi_data[2 + i] = hex_pair_to_u8(dbi_src_num[pos], dbi_src_num[pos + 1]);
    }

    dbi->commands.len = dbi_data_size;
    dbi->commands.buf = aicos_malloc(MEM_CMA, dbi->commands.len);
    memcpy((u8*)dbi->commands.buf, dbi_data, dbi->commands.len);

    dbi->commands.command_flag = MIPI_DBI_UPDATE_COMMAND;

    config->data = dbi;
    int ret = mpp_fb_ioctl(fb, AICFB_PQ_SET_CONFIG, config);

    if (dbi->commands.buf) {
        aicos_free(MEM_CMA, (u8*)dbi->commands.buf);
    }
    dbi->commands.len = 0;

    return ret;
}

static int handle_dsi_command(struct panel_dsi *dsi, char *optarg,
                             void *fb, struct aicfb_pq_config *config)
{
    char dsi_src_num[64] = { 0 };
    unsigned char dsi_data[64] = { 0 };
    size_t check_dsi_command = 0;
    int ret = 0;

    if (!dsi || !optarg || !fb || !config) {
        fprintf(stderr, "Invalid DSI command parameters\n");
        return -EINVAL;
    }

    strncpy(dsi_src_num, optarg, sizeof(dsi_src_num) - 1);
    dsi_src_num[sizeof(dsi_src_num) - 1] = '\0';

    size_t input_dsi_len = strlen(dsi_src_num);

    if (input_dsi_len < 2) {
        printf("Enter command update mode!\n");
        check_dsi_command = str2int(optarg);
    } else {
        for (int i = 0; i < input_dsi_len; ++i) {
            hex_char_to_u8(dsi_src_num[i]);
        }

        size_t dsi_data_size = input_dsi_len / 2;
        for (int i = 0; i < dsi_data_size; ++i) {
            int pos = 2 * i;
            if (pos + 1 >= input_dsi_len) {
                fprintf(stderr, "Data incomplete at position %d\n", pos);
                return -EINVAL;
            }
            dsi_data[i] = hex_pair_to_u8(dsi_src_num[pos], dsi_src_num[pos + 1]);
        }

        dsi->command.len = dsi_data_size;
        dsi->command.buf = aicos_malloc(MEM_CMA, dsi->command.len);
        if (!dsi->command.buf) {
            fprintf(stderr, "DSI command buffer allocation failed\n");
            return -ENOMEM;
        }
        memcpy(dsi->command.buf, dsi_data, dsi->command.len);
        dsi->command.command_on = MIPI_DSI_UART_COPY_COMMAND;
    }

    if (check_dsi_command)
        dsi->command.command_on = MIPI_DSI_UART_UPDATE_COMMAND;

    config->data = dsi;

    ret = mpp_fb_ioctl(fb, AICFB_PQ_SET_CONFIG, config);

    if (dsi->command.buf) {
        aicos_free(MEM_CMA, dsi->command.buf);
        dsi->command.buf = NULL;
    }
    dsi->command.len = 0;

    return ret;
}

static int handle_rgb_spi_command(struct panel_rgb *rgb, char *optarg,
                                 void *fb, struct aicfb_pq_config *config)
{
    char rgb_spi_src_num[64] = { 0 };
    unsigned char rgb_spi_data[64] = { 0 };
    size_t check_rgb_spi_command = -1;

    if (!rgb || !optarg || !fb || !config) {
        fprintf(stderr, "Invalid input parameters\n");
        return -1;
    }

    strncpy(rgb_spi_src_num, optarg, sizeof(rgb_spi_src_num) - 1);
    rgb_spi_src_num[sizeof(rgb_spi_src_num) - 1] = '\0';

    size_t input_len = strlen(rgb_spi_src_num);

    if (input_len < 2) {
        printf("Enter command mode!\n");
        check_rgb_spi_command = str2int(optarg);
    } else {
        for (int i = 0; i < input_len; ++i) {
            hex_char_to_u8(rgb_spi_src_num[i]);
        }

        size_t rgb_spi_data_size = input_len / 2;
        for (int i = 0; i < rgb_spi_data_size; ++i) {
            int pos = 2 * i;
            if (pos + 1 >= input_len) {
                fprintf(stderr, "Error: Data part incomplete\n");
                return -1;
            }
            rgb_spi_data[i] = hex_pair_to_u8(rgb_spi_src_num[pos], rgb_spi_src_num[pos + 1]);
        }

        rgb->spi_command.len = rgb_spi_data_size;
        rgb->spi_command.buf = aicos_malloc(MEM_CMA, rgb->spi_command.len);
        if (!rgb->spi_command.buf) {
            fprintf(stderr, "Memory allocation failed\n");
            return -1;
        }
        memcpy(rgb->spi_command.buf, rgb_spi_data, rgb->spi_command.len);

        rgb->spi_command.command_on = RGB_SPI_COMMAND_UPDATE;
    }

    if (check_rgb_spi_command == RGB_SPI_COMMAND_DISABLE)
        rgb->spi_command.command_on = RGB_SPI_COMMAND_DISABLE;
    if (check_rgb_spi_command == RGB_SPI_COMMAND_CLEAR)
        rgb->spi_command.command_on = RGB_SPI_COMMAND_CLEAR;

    config->data = rgb;
    int ret = mpp_fb_ioctl(fb, AICFB_PQ_SET_CONFIG, config);

    if (rgb->spi_command.buf) {
        aicos_free(MEM_CMA, rgb->spi_command.buf);
        rgb->spi_command.buf = NULL;
    }
    rgb->spi_command.len = 0;

    return ret;
}

static int aipq_config_test(int argc, char **argv)
{
    struct mpp_fb *fb = NULL;
    struct aicfb_pq_config config = { 0 };
    struct display_timing timing = { 0 };
    struct panel_rgb rgb = { 0 };
    struct panel_lvds lvds = { 0 };
    struct panel_dsi dsi = { 0 };
    struct panel_dbi dbi = { 0 };
    int connector_type = 1;
    int c, ret = 0;

    const char sopts[] = "g:p:i:t:m:a:s:c:u";
    const struct option lopts[] = {
        {"timing",          required_argument, NULL, 't'},
        {"connector",       required_argument, NULL, 'c'},
        {"get",             required_argument, NULL, 'g'},
        {"pin",             required_argument, NULL, 'p'},
        {"input",           required_argument, NULL, 'i'},
        {"merge",           required_argument, NULL, 'm'},
        {"analysis",        required_argument, NULL, 'a'},
        {"spi-init",        required_argument, NULL, 's'},
        {"usage",                 no_argument, NULL, 'u'},
        {0, 0, 0, 0}
    };

    fb = mpp_fb_open();
    if(!fb) {
        pr_err("mpp fb open failed\n");
        return -1;
    }

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'u':
            usage(argv[0]);
            return 0;
        case 'p':
        {
            u32 base = str2int(argv[2]);
            u32 id = str2int(argv[3]);
            u32 set = str2int(argv[4]);
            u32 mask, value;
            void *addr;

            mask = (0x1 << id);

            addr = (void*)(uintptr_t)base;
            value = readl(addr);

            if (set)
                value = value | mask;
            else
                value = value & ~(mask);

            writel(value, addr);

            break;
        }
        case 'i':
        {
            int fd = -1, ret = -1;
            int fsize = 0;

            if (strlen(optarg) < 3) {
                dsi.command.command_on = MIPI_DSI_DISABLE_COMMAND;
                config.data  = &dsi;
                mpp_fb_ioctl(fb, AICFB_PQ_SET_CONFIG, &config);
                break;
            }

            fd = open(optarg, O_RDONLY);
            if (fd < 0) {
                printf("Failed to open %s.\n", optarg);
                return -1;
            }
            fsize = get_file_size(optarg);

            if (fsize > 4096) {
                printf("file size: %d, over max size 4096\n", fsize);
                return -1;
            }
            dsi.command.len = fsize;

            dsi.command.buf = aicos_malloc(MEM_CMA, fsize);
            if (!dsi.command.buf) {
                printf("malloc dsi command buf failed\n");
                return -1;
            }

            ret = read(fd, (void *)dsi.command.buf, fsize);
            if (ret != fsize) {
                printf("read(%d) return %d.\n", fsize, ret);
                return -1;
            }

            dsi.command.command_on = MIPI_DSI_ADB_UPDATE_COMMAND;
            config.data  = &dsi;
            mpp_fb_ioctl(fb, AICFB_PQ_SET_CONFIG, &config);

            aicos_free(MEM_CMA, dsi.command.buf);
            break;
        }
        case 'm':
        {
            int ret = handle_dsi_command(&dsi, optarg, fb, &config);
            if (ret != 0) {
                fprintf(stderr, "DSI command handling failed (err:%d)\n", ret);
            }

            break;
        }
        case 'a':
        {
            int ret = handle_dbi_command(&dbi, optarg, fb, &config);
            if (ret != 0) {
                fprintf(stderr, "Failed to handle DBI command\n");
                }

            break;
        }
        case 's':
        {
            int ret = handle_rgb_spi_command(&rgb, optarg, fb, &config);
            if (ret != 0) {
                fprintf(stderr, "Failed to handle RGB SPI command\n");
            }

            break;
        }
        case 't':
        {
            aipq_parse_timing(&timing);

            config.timing = &timing;
            continue;
        }
        case 'c':
        {
            char con_info[32] = { 0 };

            memcpy(con_info, optarg, strlen(optarg));
            connector_type = con_info[0] - '0';

            switch (connector_type)
            {
                case AIC_RGB_COM:
                {
                    unsigned int data_order_id = 0;
                    unsigned int data_order_val[] = {
                        0x02100210, 0x02010201, 0x00120012,
                        0x00210021, 0x01200120, 0x01020102
                    };

                    rgb.mode        = char2int(con_info[1]);
                    rgb.format      = char2int(con_info[2]);
                    rgb.clock_phase = char2int(con_info[3]);
                    data_order_id   = char2int(con_info[4]);
                    rgb.data_mirror = char2int(con_info[5]);
                    rgb.data_order  = data_order_val[data_order_id];
                    rgb.spi_command.command_on = RGB_SPI_COMMAND_SEND;
                    config.data     = &rgb;
                    break;
                }
                case AIC_LVDS_COM:
                {
                    char polr[3] = {0};
                    char lanes[6] = {0};
                    lvds.mode      = char2int(con_info[1]);
                    lvds.link_mode = char2int(con_info[2]);
                    lvds.link_swap = char2int(con_info[3]);

                    memcpy(polr, con_info + 4, 2);
                    memcpy(lanes, con_info + 6, 5);
                    lvds.pols[0]     = strtoll(polr, NULL, 16);
                    lvds.lanes[0]    = strtoll(lanes, NULL, 16);

                    if (strlen(optarg) > 12) {
                        memcpy(polr, con_info + 11, 2);
                        memcpy(lanes, con_info + 13, 5);
                        lvds.pols[1]     = strtoll(polr, NULL, 16);
                        lvds.lanes[1]    = strtoll(lanes, NULL, 16);
                    }
                    config.data    = &lvds;
                    break;
                }
                case AIC_MIPI_COM:
                {
                    char str[3] = {0};

                    memcpy(str, con_info + 1, 2);
                    dsi.mode     = atoi(str);

                    dsi.format   = char2int(con_info[3]);
                    dsi.lane_num = char2int(con_info[4]);
                    dsi.vc_num   = char2int(con_info[5]);
                    dsi.dc_inv   = char2int(con_info[6]);

                    memcpy(str, con_info + 7, 2);

                    dsi.ln_polrs           = atoi(str);
                    dsi.ln_assign          = strtoll(con_info + 9, NULL, 16);
                    dsi.command.command_on = MIPI_DSI_SEND_COMMAND;
                    config.data            = &dsi;
                    break;
                }
                case AIC_DBI_COM:
                {

                    dbi.spi = aicos_malloc(MEM_CMA, sizeof(struct spi_cfg));
                    if(!dbi.spi) {
                        pr_err("Malloc spi_cfg buf failed!\n");
                        return -1;
                    }

                    char str[3] = {0};

                    dbi.type            = char2int(con_info[1]);
                    dbi.format          = char2int(con_info[2]);

                    memcpy(str, con_info + 3, 2);
                    dbi.first_line      = strtoll(str, NULL, 16);

                    memcpy(str, con_info + 5, 2);
                    dbi.other_line      = strtoll(str, NULL, 16);

                    dbi.spi->qspi_mode  = char2int(con_info[7]);
                    dbi.spi->vbp_num    = char2int(con_info[8]);

                    memcpy(str, con_info + 9, 2);
                    dbi.spi->code1_cfg  = strtoll(str, NULL, 16);

                    memcpy(str, con_info + 11, 2);
                    dbi.spi->code[0]    = strtoll(str, NULL, 16);
                    memcpy(str, con_info + 13, 2);
                    dbi.spi->code[1]    = strtoll(str, NULL, 16);
                    memcpy(str, con_info + 15, 2);
                    dbi.spi->code[2]    = strtoll(str, NULL, 16);

                    dbi.commands.command_flag = MIPI_DBI_SEND_COMMAND;
                    config.data         = &dbi;
                    break;
                }
                default:
                    break;
            }

            mpp_fb_ioctl(fb, AICFB_PQ_SET_CONFIG, &config);
            aicos_free(MEM_CMA, dbi.spi);
            break;
        }
        case 'g':
        {

            connector_type = str2int(optarg);

            config.timing = &timing;
            config.connector_type = connector_type;

            switch (connector_type)
            {
                case AIC_RGB_COM:
                    config.data = &rgb;
                    break;
                case AIC_LVDS_COM:
                    config.data = &lvds;
                    break;
                case AIC_MIPI_COM:
                    config.data = &dsi;
                    break;
                case AIC_DBI_COM:
                    dbi.spi = aicos_malloc(MEM_CMA, sizeof(struct spi_cfg));
                    if (!dbi.spi) {
                        pr_err("Malloc spi_cfg buf failed!\n");
                        return -1;
                    }
                    config.data = &dbi;
                    break;
                default:
                    break;
            }

            mpp_fb_ioctl(fb, AICFB_PQ_GET_CONFIG, &config);
            aicpq_print_config(&config, connector_type);
            aicos_free(MEM_CMA, dbi.spi);
            break;
        }
        default:
            pr_err("Invalid parameter: %#x\n", ret);
            usage(argv[0]);
            return 0;
        }
    }
    return 0;
}

MSH_CMD_EXPORT_ALIAS(aipq_config_test, aipq_cfg, set aipq config);
#endif /* RT_USING_FINSH */

