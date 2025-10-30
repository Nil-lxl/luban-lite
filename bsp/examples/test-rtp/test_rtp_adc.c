/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: ArtInChip
 */
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include <rtthread.h>
#include <time.h>
#include "rtdevice.h"
#include "aic_core.h"
#include "aic_log.h"
#include "hal_rtp.h"
#include "touch.h"
#include "hal_adcim.h"
#include "aic_drv_gpio.h"
#include "float.h"

/*
 * The voltage range of the RTP ADC is based on VCCIO_3V3.
 * The voltage range of the GPAI ADC is based on LDO.
 */
#define AIC_RTP_ADC_DEFAULT_VOLTAGE    3.3
#define AIC_RTP_ADC_VOLTAGE_ACCURACY   10000
#define AIC_RTP_ADC_CHAN_MAX        4

#define AIC_RTP_ADC_XP  "PA.8"
#define AIC_RTP_ADC_YP  "PA.9"
#define AIC_RTP_ADC_XN  "PA.10"
#define AIC_RTP_ADC_YN  "PA.11"

static const char sopts[] = "r:t:n:g:h";
static const struct option lopts[] = {
    {"read",         required_argument, NULL, 'r'},
    {"def_voltage",  required_argument, NULL, 't'},
    {"number",       required_argument, NULL, 'n'},
    {"get_resistance", required_argument, NULL, 'g'},
    {"help",         no_argument, NULL, 'h'},
    {0, 0, 0, 0}
    };

static rt_device_t g_rtp_dev = RT_NULL;
static int g_sample_num = 10;

static void cmd_rtp_usage(char *program)
{
    printf("Usage: %s [options]\n", program);
    printf("\t -r, --read <chan>\t\n");
    printf("\t -t, --voltage\t\tModify default voltage\n");
    printf("\t -n, --number\t\tSet the number of samples\n");
    printf("\t -g, --get\t\tget the panel resistance\n");
    printf("\t -h, --help \n");
    printf("\n");
    printf("Example1: %s -r 1 -n 10 -t 3.3\n", program);
    printf("Example2: %s -g 1\n", program);
    printf("CHAN-ID:\n");
    printf("\t[0] Y-\n");
    printf("\t[1] X-\n");
    printf("\t[2] Y+\n");
    printf("\t[3] X+\n");
}

static unsigned int g_xp, p_xp;
static unsigned int g_yp, p_yp;
static unsigned int g_xn, p_xn;
static unsigned int g_yn, p_yn;

static void rtp_gpio_set_status(int driver_set, int io_mux, int pin_fun)
{
    unsigned int gpio_drive_set = driver_set;
    unsigned int gpio_io_mux = io_mux;

    const char* pins[] = {AIC_RTP_ADC_XP, AIC_RTP_ADC_YP, AIC_RTP_ADC_XN, AIC_RTP_ADC_YN};
    unsigned int *groups[] = { &g_xp, &g_yp, &g_xn, &g_yn };
    unsigned int *pins_num[] = { &p_xp, &p_yp, &p_xn, &p_yn };

    for (int i = 0; i < 4; i++) {
        unsigned int pin = hal_gpio_name2pin(pins[i]);
        drv_pin_mux_set(pin, gpio_io_mux);
        drv_pin_drive_set(pin, gpio_drive_set);
        *groups[i] = GPIO_GROUP(pin);
        *pins_num[i] = GPIO_GROUP_PIN(pin);
    }

    if (pin_fun) {
        hal_gpio_direction_output(g_xp, p_xp);
        hal_gpio_direction_output(g_yp, p_yp);
        hal_gpio_direction_output(g_xn, p_xn);
        hal_gpio_direction_output(g_yn, p_yn);
    }
}

void rtp_gpio_set_output(int xp_state, int yp_state, int xn_state, int yn_state)
{
    if (xp_state)
        hal_gpio_set_output(g_xp, p_xp);
    else
        hal_gpio_clr_output(g_xp, p_xp);

    if (yp_state)
        hal_gpio_set_output(g_yp, p_yp);
    else
        hal_gpio_clr_output(g_yp, p_yp);

    if (xn_state)
        hal_gpio_set_output(g_xn, p_xn);
    else
        hal_gpio_clr_output(g_xn, p_xn);

    if (yn_state)
        hal_gpio_set_output(g_yn, p_yn);
    else
        hal_gpio_clr_output(g_yn, p_yn);
}

float calculate_filtered_average(float *values, int size) {
    if (size <= 2) {
        float sum = 0.0f;
        for (int i = 0; i < size; i++) sum += values[i];
        return sum / size;
    }

    float min_val = values[0], max_val = values[0], sum = values[0];
    for (int i = 1; i < size; i++) {
        if (values[i] < min_val) min_val = values[i];
        if (values[i] > max_val) max_val = values[i];
        sum += values[i];
    }
    return (sum - min_val - max_val) / (size - 2);
}

void perform_measurement(float min_vals[], float max_vals[],
                         int is_first_measurement,
                         int xp_state, int yp_state, int xn_state, int yn_state)
{
    struct aic_rtp_adc_info adc_info = {0};
    int scale = AIC_RTP_ADC_VOLTAGE_ACCURACY;
    int voltage_int = 0, cnt = 0, cal_param = 0;
    int sample_num = 5;
    float def_voltage = AIC_RTP_ADC_DEFAULT_VOLTAGE;

    rtp_gpio_set_output(xp_state, yp_state, xn_state, yn_state);

    for (int i = 0; i < 4; i++) {
        adc_info.ch = i;
        cal_param = hal_adcim_auto_calibration();
        rt_device_control(g_rtp_dev, RT_TOUCH_CTRL_ENABLE_INT, RT_NULL);

        float measurements[5] = {0.0f};
        int valid_count = 0;

        while (cnt < sample_num) {
            rt_device_control(g_rtp_dev, RT_TOUCH_CTRL_GET_ADC, (void *)&adc_info);
            aicos_msleep(1);
            if (adc_info.data) {
                voltage_int = hal_adcim_adc2voltage(&adc_info.data, cal_param,
                                                   AIC_RTP_ADC_VOLTAGE_ACCURACY, def_voltage);
                float voltage = (float)voltage_int / scale;

                printf("[%d] ch%d: %d, voltage: %.4f v\n",
                      cnt, i, adc_info.data, voltage);

                measurements[valid_count++] = voltage;
            }
            cnt++;
        }
        cnt = 0;

        if (valid_count > 0) {
            float avg_voltage = calculate_filtered_average(measurements, valid_count);
            printf("Ch%d filtered: %.4f v\n", i, avg_voltage);

            if (is_first_measurement) {
                if (i < 2 && avg_voltage < min_vals[i]) min_vals[i] = avg_voltage;
                if (i >= 2 && avg_voltage > max_vals[i-2]) max_vals[i-2] = avg_voltage;
            } else {
                if (i < 2 && avg_voltage > max_vals[i]) max_vals[i] = avg_voltage;
                if (i >= 2 && avg_voltage < min_vals[i-2]) min_vals[i-2] = avg_voltage;
            }
        }
    }
}

static void rtp_get_panel_resistance(void)
{
    float first_min_vals[2] = {FLT_MAX, FLT_MAX};
    float first_max_vals[2] = {0.0f, 0.0f};

    float second_min_vals[2] = {FLT_MAX, FLT_MAX};
    float second_max_vals[2] = {0.0f, 0.0f};

    float R[4] = {0.0f};

    g_rtp_dev = rt_device_find(AIC_RTP_NAME);
    if (!g_rtp_dev) {
        rt_kprintf("Failed to find %s device\n", AIC_RTP_NAME);
        return;
    }

    rtp_gpio_set_status(0, 1, 1);

    printf("=== First Measurement ===\n");
    perform_measurement(first_min_vals, first_max_vals, 1, 1, 1, 0, 0);

    printf("Ch0 min: %.4f v, Ch1 min: %.4f v\n", first_min_vals[0], first_min_vals[1]);
    printf("Ch2 max: %.4f v, Ch3 max: %.4f v\n", first_max_vals[0], first_max_vals[1]);

    for (int i = 0; i < 2; i++) {
        if (first_min_vals[i] > 0.0001f) {
            R[i] = (first_max_vals[i] - first_min_vals[i]) * 180.0f / first_min_vals[i];
        } else {
            R[i] = -1.0f;
        }
    }
    printf("R1 = %.2f Ω, R2 = %.2f Ω\n", R[0], R[1]);

    printf("\n=== Second Measurement ===\n");
    perform_measurement(second_min_vals, second_max_vals, 0, 0, 0, 1, 1);

    printf("Ch0 max: %.4f v, Ch1 max: %.4f v\n", second_max_vals[0], second_max_vals[1]);
    printf("Ch2 min: %.4f v, Ch3 min: %.4f v\n", second_min_vals[0], second_min_vals[1]);

    for (int i = 0; i < 2; i++) {
        if (second_min_vals[i] > 0.0001f) {
            R[i+2] = (second_max_vals[i] - second_min_vals[i]) * 180.0f / second_min_vals[i];
        } else {
            R[i+2] = -1.0f;
        }
    }
    printf("R3 = %.2f Ω, R4 = %.2f Ω\n", R[2], R[3]);

    int Rx = (R[1] + R[3]) / 2;
    int Ry = (R[0] + R[2]) / 2;
    printf("\nRx = %d Ω, Ry = %d Ω\n", Rx, Ry);

    rtp_gpio_set_status(3, 2, 0);
}

static void rtp_get_data(int ch, float def_voltage)
{
    struct aic_rtp_adc_info adc_info = {0};
    int scale = AIC_RTP_ADC_VOLTAGE_ACCURACY;
    int voltage = 0, cnt = 0, cal_param = 0;

    g_rtp_dev = rt_device_find(AIC_RTP_NAME);
    if (g_rtp_dev == RT_NULL) {
        rt_kprintf("Failed to find %s device\n", AIC_RTP_NAME);
        return;
    }

    adc_info.ch = ch;
    cal_param = hal_adcim_auto_calibration();
    rt_device_control(g_rtp_dev, RT_TOUCH_CTRL_ENABLE_INT, RT_NULL);

    while (cnt < g_sample_num) {
        rt_device_control(g_rtp_dev, RT_TOUCH_CTRL_GET_ADC, (void *)&adc_info);
        aicos_msleep(10);
        if (adc_info.data) {
            voltage = hal_adcim_adc2voltage(&adc_info.data, cal_param, AIC_RTP_ADC_VOLTAGE_ACCURACY, def_voltage);
            rt_kprintf("[%d] ch%d: %d\n", cnt, ch, adc_info.data);
            rt_kprintf("voltage:%d.%04d v\n", voltage / scale, voltage % scale);
        }
        cnt++;
    }

    return;
}

static void cmd_test_rtp_adc(int argc, char **argv)
{
    int c, ch = 0;
    float def_voltage = AIC_RTP_ADC_DEFAULT_VOLTAGE;
    int get_resistance = 0;

    if (argc < 2) {
        cmd_rtp_usage(argv[0]);
        return;
    }

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'r':
            ch = atoi(optarg);
            break;
        case 't':
            def_voltage = atof(optarg);
            break;
        case 'g':
            get_resistance = atoi(optarg);
            break;
        case 'n':
            g_sample_num = atoi(optarg);
            break;
        case 'h':
        default:
            cmd_rtp_usage(argv[0]);
            return;
        }
    }

    if (get_resistance) {
        rtp_get_panel_resistance();
        return;
    }

    if (ch < 0 || ch >= AIC_RTP_ADC_CHAN_MAX) {
        pr_err("Invalid channel No.%s\n", optarg);
        return;
    }

    rtp_get_data(ch, def_voltage);

    return;
}

MSH_CMD_EXPORT_ALIAS(cmd_test_rtp_adc, test_rtp_adc, rtp device sample);
