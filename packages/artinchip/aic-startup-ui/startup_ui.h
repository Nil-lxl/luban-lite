/*
 * Copyright (c) 2020-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: ArtInChip
 */

#ifndef STARTUP_UI_H
#define STARTUP_UI_H

#ifdef __cplusplus
extern "C" {
#endif

struct startup_ui_fb {
    struct mpp_fb *startup_fb;
    struct aicfb_screeninfo startup_fb_data;
};

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
