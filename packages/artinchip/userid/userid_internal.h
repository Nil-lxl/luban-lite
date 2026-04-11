/*
 * Copyright (c) 2024, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <aic_common.h>

#ifndef _USERID_INTERNAL_H_
#define _USERID_INTERNAL_H_

#define USERID_MAX_SIZE (16 * 1024)

struct userid_driver {
	const char *name;

	/**
	 * load() - Load the userid from storage
	 *
	 * @return 0 if OK
	 */
	int (*load)(void);

	/**
	 * save() - Save the userid to storage
	 *
	 * @return 0 if OK
	 */
	int (*save)(void);

	/**
	 * erase() - Erase the userid on storage
	 *
	 * @return 0 if OK
	 */
	int (*erase)(void);

	/**
	 * init() - Set up the initial userid
	 *
	 * @return 0 if OK, -ENOENT if no initial userid could be found,
	 */
	int (*init)(void);
};

#define USERID_HEADER_MAGIC 0x48444955
#define USERID_ITEM_MAGIC   0x49444955

struct userid_storage_header {
	u32 magic;
	u32 crc32; /* CRC32 result from total_length to data end */
	u32 total_length; /* From this field to data end */
	u32 reserved[5];
};

struct userid_item_header {
	u32 magic;
	u16 name_len;
	u16 data_len;
};

struct userid_entity {
	struct list_head list;
	char *name;
	u8 *data;
	u16 name_len;
	u16 data_len;
};

extern struct userid_driver userid_spinand;
extern struct userid_driver userid_spinor;
extern struct userid_driver userid_mmc;

#endif /* _USERID_INTERNAL_H_ */
