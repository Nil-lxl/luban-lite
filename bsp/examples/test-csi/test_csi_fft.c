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
#include "csi_const_structs.h"

#define FFT_SIZE 4096
#define FFT_ITERATIONS 1

typedef struct {
    float re;
    float im;
} complex_float;

static float32_t input_re[FFT_SIZE] __attribute__((aligned(32)));
static float32_t input_im[FFT_SIZE] __attribute__((aligned(32)));
static complex_float sw_input[FFT_SIZE] __attribute__((aligned(32)));
static float32_t dsp_output_re[FFT_SIZE * 2] __attribute__((aligned(32)));

static void software_fft(complex_float *data, int n)
{
    int i, j, k, m;
    int step, step2;
    float temp_real, temp_imag;
    float w_real, w_imag, wp_real, wp_imag;
    float angle;

    j = 0;
    for (i = 0; i < n; i++) {
        if (i > j) {
            temp_real = data[i].re;
            temp_imag = data[i].im;
            data[i].re = data[j].re;
            data[i].im = data[j].im;
            data[j].re = temp_real;
            data[j].im = temp_imag;
        }
        k = n >> 1;
        while (k && (k <= j)) {
            j -= k;
            k >>= 1;
        }
        j += k;
    }

    for (step = 2; step <= n; step <<= 1) {
        step2 = step >> 1;
        angle = -2.0f * PI / step;
        w_real = cosf(angle);
        w_imag = sinf(angle);
        wp_real = 1.0f;
        wp_imag = 0.0f;

        for (m = 0; m < step2; m++) {
            for (i = m; i < n; i += step) {
                j = i + step2;
                temp_real = wp_real * data[j].re - wp_imag * data[j].im;
                temp_imag = wp_real * data[j].im + wp_imag * data[j].re;
                data[j].re = data[i].re - temp_real;
                data[j].im = data[i].im - temp_imag;
                data[i].re += temp_real;
                data[i].im += temp_imag;
            }
            temp_real = wp_real * w_real - wp_imag * w_imag;
            temp_imag = wp_real * w_imag + wp_imag * w_real;
            wp_real = temp_real;
            wp_imag = temp_imag;
        }
    }
}

int test_csi_fft(void)
{
    const csi_cfft_instance_f32 *cfft_instance = &csi_cfft_sR_f32_len1024;
    u64 start_us, end_us;
    u64 dsp_time, sw_time;
    float total_error_real = 0.0f;
    float total_error_imag = 0.0f;
    float max_error_real = 0.0f;
    float max_error_imag = 0.0f;
    float avg_error_real, avg_error_imag;
    int i;
        
    printf("\n========== FFT Test ==========\n");
        
    /* Generate input signal: mix of two sine waves with high precision PI */
    for (i = 0; i < FFT_SIZE; i++) {
        input_re[i] = sinf(2.0f * PI * (float32_t)i * 10.0f / (float32_t)FFT_SIZE) +
                      0.5f * sinf(2.0f * PI * (float32_t)i * 25.0f / (float32_t)FFT_SIZE);
        input_im[i] = 0.0f;
    }
        
    printf("FFT Size: %d, Iterations: %d\n", FFT_SIZE, FFT_ITERATIONS);

    /* DSP FFT timing */
    for (i = 0; i < FFT_SIZE; i++) {
        dsp_output_re[i * 2] = input_re[i];
        dsp_output_re[i * 2 + 1] = input_im[i];
    }
    start_us = aic_get_time_us();
    csi_cfft_f32(cfft_instance, dsp_output_re, 0, 1);
    end_us = aic_get_time_us();
    dsp_time = end_us - start_us;
    printf("DSP FFT: %lu us\n", (unsigned long)dsp_time);
        
    /* SW FFT timing */
    for (i = 0; i < FFT_SIZE; i++) {
        sw_input[i].re = input_re[i];
        sw_input[i].im = input_im[i];
    }
    start_us = aic_get_time_us();
    software_fft(sw_input, FFT_SIZE);
    end_us = aic_get_time_us();
    sw_time = end_us - start_us;
    printf("SW FFT: %lu us\n", (unsigned long)sw_time);

    if (sw_time > 0 && dsp_time > 0) {
        printf("Speedup: %dx\n", (int)((float)sw_time / dsp_time));
    }

    /* Prepare data for result comparison */
    for (i = 0; i < FFT_SIZE; i++) {
        dsp_output_re[i * 2] = input_re[i];
        dsp_output_re[i * 2 + 1] = input_im[i];
        sw_input[i].re = input_re[i];
        sw_input[i].im = input_im[i];
    }
    csi_cfft_f32(cfft_instance, dsp_output_re, 0, 1);
    software_fft(sw_input, FFT_SIZE);

    printf("%6s %12s %12s %12s %12s %12s %12s\n", "Index", "DSP Real", "DSP Imag", "SW Real", "SW Imag", "Diff Real", "Diff Imag");
    for (i = 0; i < 10; i++) {
        float dsp_real = dsp_output_re[i * 2];
        float dsp_imag = dsp_output_re[i * 2 + 1];
        float sw_real = sw_input[i].re;
        float sw_imag = sw_input[i].im;
        float diff_real = fabsf(dsp_real - sw_real);
        float diff_imag = fabsf(dsp_imag - sw_imag);
        
        printf("%6d %12.4f %12.4f %12.4f %12.4f %12.4f %12.4f\n",
               i, dsp_real, dsp_imag, sw_real, sw_imag, diff_real, diff_imag);
    }

    /* Calculate average and max error */
    for (i = 0; i < FFT_SIZE; i++) {
        float diff_real = fabsf(dsp_output_re[i * 2] - sw_input[i].re);
        float diff_imag = fabsf(dsp_output_re[i * 2 + 1] - sw_input[i].im);
        total_error_real += diff_real;
        total_error_imag += diff_imag;
        if (diff_real > max_error_real) {
            max_error_real = diff_real;
        }
        if (diff_imag > max_error_imag) {
            max_error_imag = diff_imag;
        }
    }
    avg_error_real = total_error_real / FFT_SIZE;
    avg_error_imag = total_error_imag / FFT_SIZE;
        
    printf("Avg error: Real=%.4f, Imag=%.4f\n", avg_error_real, avg_error_imag);
    printf("Max error: Real=%.4f, Imag=%.4f\n", max_error_real, max_error_imag);

    return 0;
}