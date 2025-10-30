/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: jiji.chen <jiji.chen@artinchip.com>
 */

#include <rtconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <spinand.h>

#define MIN(x, y) ((x) < (y) ? (x) : (y))

static int spinand_oob_get_user_region(struct aic_spinand *flash, int section,
                            struct aic_oob_region *region)
{
    if (flash->info->oob_get_user)
        return flash->info->oob_get_user(flash, section, region);

    if (section > 3)
      return -SPINAND_ERR;

    region->offset = (16 * section) + 0;
    region->length = 16;

    return 0;
}

static int spinand_ooblayout_find_user_region(struct aic_spinand *flash, int start, int *sectionp, struct aic_oob_region *oobregion)
{
    int pos = 0, ret;
    int section = 0;
    memset(oobregion, 0, sizeof(*oobregion));

    while (1) {
        ret = spinand_oob_get_user_region(flash, section, oobregion);
        if (ret)
            return ret;

        if (oobregion->length + pos > start)
            break;

        pos += oobregion->length;
        section += 1;
    }

    oobregion->offset += start - pos;
    oobregion->length -= start - pos;
    *sectionp = section;

    return 0;
}

int spinand_ooblayout_map_user(struct aic_spinand *flash, u8 *oobbuf,
                       const u8 *spare, int start, int nbytes) {
    struct aic_oob_region oobregion;
    int ret = 0, cnt;
    int section = 0;
    const u8 *buf = spare;

    ret = spinand_ooblayout_find_user_region(flash, start, &section, &oobregion);

    while (!ret)
    {
        pr_debug("docopy: sect: %u, src0x%lu, dstoff:%u\n", section, (buf-spare), oobregion.offset);

        cnt = MIN(nbytes, oobregion.length);
        memcpy(oobbuf + oobregion.offset, buf, cnt);
        buf += cnt;
        nbytes -= cnt;

        if (!nbytes)
            break;

        section += 1;
        ret = spinand_oob_get_user_region(flash, section, &oobregion);
    }

    return ret;
}

int spinand_ooblayout_unmap_user(struct aic_spinand *flash, u8 *dst,
                       u8 *src, int start, int nbytes) {
    struct aic_oob_region oobregion;
    int ret = 0, cnt;
    int section = 0;
    u8 *buf = dst;

    ret = spinand_ooblayout_find_user_region(flash, start, &section, &oobregion);

    while (!ret)
    {
        pr_debug("docopy: sect: %u, srcoff%u, dstoff:%lu\n", section, oobregion.offset, (buf-dst));

        cnt = MIN(nbytes, oobregion.length);
        memcpy(buf, (src + oobregion.offset), cnt);
        buf += cnt;
        nbytes -= cnt;

        if (!nbytes)
            break;

        section += 1;
        ret = spinand_oob_get_user_region(flash, section, &oobregion);
    }

    return ret;
}
