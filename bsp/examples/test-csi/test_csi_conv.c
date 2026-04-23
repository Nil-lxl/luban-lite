/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xiaodong.zhao@artinchip.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <rtthread.h>
#include <finsh.h>
#include <aic_common.h>
#include "csi_math.h"

#define CONV_SIZE_A 1024
#define CONV_SIZE_B 128
#define CONV_SIZE_OUT (CONV_SIZE_A + CONV_SIZE_B - 1)
#define CONV_ITERATIONS 10

static float32_t conv_input_a[CONV_SIZE_A] __attribute__((aligned(32)));
static float32_t conv_input_b[CONV_SIZE_B] __attribute__((aligned(32)));
static float32_t conv_output_dsp[CONV_SIZE_OUT] __attribute__((aligned(32)));
static float32_t conv_output_sw[CONV_SIZE_OUT] __attribute__((aligned(32)));

static void software_conv(float32_t *pSrcA, uint32_t srcALen,
                          float32_t *pSrcB, uint32_t srcBLen,
                          float32_t *pDst)
{
    uint32_t i, j;
    for (i = 0; i < srcALen + srcBLen - 1; i++) {
        pDst[i] = 0.0f;
    }
    for (i = 0; i < srcALen; i++) {
        for (j = 0; j < srcBLen; j++) {
            pDst[i + j] += pSrcA[i] * pSrcB[j];
        }
    }
}

int test_csi_conv(void)
{
    u64 start_us, end_us;
    u64 dsp_time, sw_time;
    float total_error = 0.0f;
    float avg_error;
    uint32_t i;
    float max_error = 0.0f;

    printf("\n========== Convolution Test ==========\n");

    /* Generate input signals with high precision PI */
    for (i = 0; i < CONV_SIZE_A; i++) {
        conv_input_a[i] = sinf(2.0f * PI * (float32_t)i * 5.0f / (float32_t)CONV_SIZE_A);
    }
    for (i = 0; i < CONV_SIZE_B; i++) {
        conv_input_b[i] = (float32_t)(i % 10) / 10.0f;
    }

    printf("Conv Size A: %d, B: %d, Out: %d, Iterations: %d\n", CONV_SIZE_A, CONV_SIZE_B, CONV_SIZE_OUT, CONV_ITERATIONS);

    /* Run DSP convolution multiple times for accurate timing */
    memset(conv_output_dsp, 0, sizeof(conv_output_dsp));
    start_us = aic_get_time_us();
    for (int iter = 0; iter < CONV_ITERATIONS; iter++) {
        csi_conv_f32(conv_input_a, CONV_SIZE_A, conv_input_b, CONV_SIZE_B, conv_output_dsp);
    }
    end_us = aic_get_time_us();
    dsp_time = (end_us - start_us) / CONV_ITERATIONS;
    printf("DSP Conv: %lu us\n", (unsigned long)dsp_time);

    /* Run software convolution multiple times for accurate timing */
    memset(conv_output_sw, 0, sizeof(conv_output_sw));
    start_us = aic_get_time_us();
    for (int iter = 0; iter < CONV_ITERATIONS; iter++) {
        software_conv(conv_input_a, CONV_SIZE_A, conv_input_b, CONV_SIZE_B, conv_output_sw);
    }
    end_us = aic_get_time_us();
    sw_time = (end_us - start_us) / CONV_ITERATIONS;
    printf("SW Conv: %lu us\n", (unsigned long)sw_time);

    if (dsp_time > 0 && sw_time > 0) {
        printf("Speedup: %.2fx\n", (float)sw_time / dsp_time);
    }

    printf("%6s %12s %12s %12s\n", "Index", "DSP Out", "SW Out", "Diff");
    for (i = 0; i < 10; i++) {
        float diff = fabsf(conv_output_dsp[i] - conv_output_sw[i]);
        printf("%6lu %12.4f %12.4f %12.4f\n",
               (unsigned long)i, conv_output_dsp[i], conv_output_sw[i], diff);
    }

    /* Calculate average and max error */
    for (i = 0; i < CONV_SIZE_OUT; i++) {
        float diff = fabsf(conv_output_dsp[i] - conv_output_sw[i]);
        total_error += diff;
        if (diff > max_error) {
            max_error = diff;
        }
    }
    avg_error = total_error / CONV_SIZE_OUT;

    printf("Avg error: %.4f\n", avg_error);
    printf("Max error: %.4f\n", max_error);

    return 0;
}