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

#define FIR_SIZE 1280
#define FIR_TAPS 160
#define FIR_ITERATIONS 10

static float32_t fir_input[FIR_SIZE] __attribute__((aligned(32)));
static float32_t fir_output_dsp[FIR_SIZE] __attribute__((aligned(32)));
static float32_t fir_output_sw[FIR_SIZE] __attribute__((aligned(32)));
static float32_t fir_coeffs[FIR_TAPS] __attribute__((aligned(32)));
static float32_t fir_state[FIR_SIZE + FIR_TAPS - 1] __attribute__((aligned(32)));

static void software_fir(float32_t *input, float32_t *output, float32_t *coeffs,
                         int input_size, int num_taps)
{
    int n, k;
    for (n = 0; n < input_size; n++) {
        float32_t sum = 0.0f;
        for (k = 0; k < num_taps; k++) {
            if (n - k >= 0) {
                sum += input[n - k] * coeffs[k];
            }
        }
        output[n] = sum;
    }
}

int test_csi_fir(void)
{
    csi_fir_instance_f32 fir_instance;
    u64 start_us, end_us;
    u64 dsp_time, sw_time;
    float total_error = 0.0f;
    float avg_error;
    int i;
    float max_error = 0.0f;

    printf("\n========== FIR Test ==========\n");

    /* Generate input signal with high precision PI */
    for (i = 0; i < FIR_SIZE; i++) {
        fir_input[i] = sinf(2.0f * PI * (float32_t)i * 5.0f / (float32_t)FIR_SIZE) +
                       0.3f * sinf(2.0f * PI * (float32_t)i * 20.0f / (float32_t)FIR_SIZE);
    }

    for (i = 0; i < FIR_TAPS; i++) {
        fir_coeffs[i] = 1.0f / FIR_TAPS;
    }

    printf("FIR Size: %d, Taps: %d, Iterations: %d\n", FIR_SIZE, FIR_TAPS, FIR_ITERATIONS);

    csi_fir_init_f32(&fir_instance, FIR_TAPS, fir_coeffs, fir_state, FIR_SIZE);

    /* Run DSP FIR multiple times for accurate timing */
    memset(fir_output_dsp, 0, sizeof(fir_output_dsp));
    start_us = aic_get_time_us();
    for (int iter = 0; iter < FIR_ITERATIONS; iter++) {
        csi_fir_f32(&fir_instance, fir_input, fir_output_dsp, FIR_SIZE);
    }
    end_us = aic_get_time_us();
    dsp_time = (end_us - start_us) / FIR_ITERATIONS;
    printf("DSP FIR: %lu us\n", (unsigned long)dsp_time);

    /* Run software FIR multiple times for accurate timing */
    memset(fir_output_sw, 0, sizeof(fir_output_sw));
    start_us = aic_get_time_us();
    for (int iter = 0; iter < FIR_ITERATIONS; iter++) {
        software_fir(fir_input, fir_output_sw, fir_coeffs, FIR_SIZE, FIR_TAPS);
    }
    end_us = aic_get_time_us();
    sw_time = (end_us - start_us) / FIR_ITERATIONS;
    printf("SW FIR: %lu us\n", (unsigned long)sw_time);

    if (dsp_time > 0 && sw_time > 0) {
        printf("Speedup: %.2fx\n", (float)sw_time / dsp_time);
    }

    printf("%6s %12s %12s %12s %12s\n", "Index", "Input", "DSP Output", "SW Output", "Diff");
    for (i = 0; i < 10; i++) {
        float diff = fabsf(fir_output_dsp[i] - fir_output_sw[i]);
        printf("%6d %12.4f %12.4f %12.4f %12.4f\n",
               i, fir_input[i], fir_output_dsp[i], fir_output_sw[i], diff);
    }

    /* Calculate average and max error */
    for (i = 0; i < FIR_SIZE; i++) {
        float diff = fabsf(fir_output_dsp[i] - fir_output_sw[i]);
        total_error += diff;
        if (diff > max_error) {
            max_error = diff;
        }
    }
    avg_error = total_error / FIR_SIZE;

    printf("Avg error: %.4f\n", avg_error);
    printf("Max error: %.4f\n", max_error);

    return 0;
}