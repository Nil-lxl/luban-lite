/*
 * Copyright (C) 2022-2026 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  fangjie.wang <fangjie.wang@artinchip.com>
 */

#ifndef __MSG_UTILS_H__
#define __MSG_UTILS_H__

#include "msg_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Convert emotion string to emotion enum value
 * @param emotion_str Emotion string (e.g. "happy", "sad")
 * @return emotion_status_t Emotion enum value
 */
emotion_status_t msg_emotion_str_to_enum(const char *emotion_str);

#ifdef __cplusplus
}
#endif

#endif /* __MSG_UTILS_H__ */

