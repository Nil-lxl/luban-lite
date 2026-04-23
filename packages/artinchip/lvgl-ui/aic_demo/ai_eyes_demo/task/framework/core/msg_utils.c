/*
 * Copyright (C) 2022-2026 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  fangjie.wang <fangjie.wang@artinchip.com>
 */

#include "msg_utils.h"
#include <string.h>

emotion_status_t msg_emotion_str_to_enum(const char *emotion_str)
{
    if (emotion_str == RT_NULL) {
        return EMOTION_STATUS_NEUTRAL;
    }

    if (strcmp(emotion_str, "neutral") == 0) {
        return EMOTION_STATUS_NEUTRAL;
    } else if (strcmp(emotion_str, "happy") == 0) {
        return EMOTION_STATUS_HAPPY;
    } else if (strcmp(emotion_str, "sad") == 0) {
        return EMOTION_STATUS_SAD;
    } else if (strcmp(emotion_str, "angry") == 0) {
        return EMOTION_STATUS_ANGRY;
    } else if (strcmp(emotion_str, "laughing") == 0) {
        return EMOTION_STATUS_LAUGHING;
    } else if (strcmp(emotion_str, "funny") == 0) {
        return EMOTION_STATUS_FUNNY;
    } else if (strcmp(emotion_str, "crying") == 0) {
        return EMOTION_STATUS_CRYING;
    } else if (strcmp(emotion_str, "loving") == 0) {
        return EMOTION_STATUS_LOVING;
    } else if (strcmp(emotion_str, "embarrassed") == 0) {
        return EMOTION_STATUS_EMBARRASSED;
    } else if (strcmp(emotion_str, "surprised") == 0) {
        return EMOTION_STATUS_SURPRISED;
    } else if (strcmp(emotion_str, "shocked") == 0) {
        return EMOTION_STATUS_SHOCKED;
    } else if (strcmp(emotion_str, "thinking") == 0) {
        return EMOTION_STATUS_THINKING;
    } else if (strcmp(emotion_str, "confused") == 0) {
        return EMOTION_STATUS_CONFUSED;
    } else if (strcmp(emotion_str, "sleepy") == 0) {
        return EMOTION_STATUS_SLEEPY;
    } else if (strcmp(emotion_str, "kissy") == 0) {
        return EMOTION_STATUS_KISSY;
    } else if (strcmp(emotion_str, "cool") == 0) {
        return EMOTION_STATUS_COOL;
    }

    return EMOTION_STATUS_NEUTRAL;
}

