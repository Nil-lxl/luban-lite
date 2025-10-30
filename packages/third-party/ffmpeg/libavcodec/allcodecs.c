
/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc: register codec interface
 */

#include "avcodec.h"

extern AVCodec ff_wmav1_decoder;
extern AVCodec ff_wmav2_decoder;

void avcodec_register_all(void)
{
    avcodec_register(&ff_wmav1_decoder);
    avcodec_register(&ff_wmav2_decoder);
}

