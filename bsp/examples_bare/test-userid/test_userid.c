/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <console.h>
#include <aic_common.h>
#include <aic_utils.h>
#include <aic_log.h>
#include <userid.h>

static int do_userid_list(int argc, char **argv)
{
	int i, cnt, ret;
	char name[256];

	cnt = userid_get_count();
	printf("ID count %d\n", cnt);
	for (i = 0; i < cnt; i++) {
		memset(name, 0, 256);
		ret = userid_get_name(i, name, 255);
		printf("  %s\n", name);
		if (ret <= 0)
			continue;
	}
	return 0;
}

static int do_userid_init(int argc, char **argv)
{
	int ret;

	ret = userid_init();
	if (ret)
		printf("UserID initialization failed.\n");

	return 0;
}

static int do_userid_dump(int argc, char **argv)
{
	unsigned long size, offset;
	char *name, *pe;
	u8 *p;
	int rdcnt;

	if (argc != 4 && argc != 2)
		return -1;
	name = argv[1];
	if (argc == 2) {
		offset = 0;
		size = userid_get_data_length(name);
	} else {
		offset = strtoul(argv[2], &pe, 0);
		size = strtoul(argv[3], &pe, 0);
	}

	p = malloc(size);
	rdcnt = userid_read(name, offset, p, size);
	if (rdcnt > 0)
		hexdump(p, rdcnt, 1);
	else
		pr_err("Read failed. ret = %d\n", rdcnt);
	free(p);
	return 0;
}

static int do_userid_size(int argc, char **argv)
{
	unsigned long addr;
	char *name, *pe;
	int size;
	u32 *p;

	if (argc != 3 && argc != 2)
		return -1;
	name = argv[1];
	size = userid_get_data_length(name);
	if (size < 0)
		size = 0;
	if (argc == 2) {
		printf("%d\n", size);
	} else {
		addr = strtoul(argv[2], &pe, 0);
		p = (u32 *)addr;
		*p = size;
	}

	return 0;
}

static int do_userid_remove(int argc, char **argv)
{
	int ret;
	char *name;

	if ((argc != 2))
		return -1;
	name = argv[1];
	ret = userid_remove(name);
	if (ret) {
		pr_err("Failed to remove %s\n", name);
		return -1;
	}
	return 0;
}

static int do_userid_import(int argc, char **argv)
{
	unsigned long addr;
	char *pe;
	int ret;

	if ((argc != 2))
		return -1;
	addr = strtoul(argv[1], &pe, 0);
	ret = userid_import((void *)addr);
	if (ret) {
		pr_err("Failed to import from 0x%lx\n", addr);
		return -1;
	}
	return 0;
}

static int do_userid_export(int argc, char **argv)
{
	unsigned long addr;
	char *pe;
	int ret;

	if (argc != 2)
		return -1;
	addr = strtoul(argv[1], &pe, 0);
	ret = userid_export((void *)addr);
	if (ret <= 0) {
		pr_err("Failed to export to 0x%lx\n", addr);
		return -1;
	}
	return 0;
}

static int do_userid_read(int argc, char **argv)
{
	unsigned long addr, size, offset;
	char *name, *pe;
	int rdcnt;

	if (argc < 5)
		return -1;

	name = argv[1];
	offset = strtoul(argv[2], &pe, 0);
	size = strtoul(argv[3], &pe, 0);
	addr = strtoul(argv[4], &pe, 0);

	rdcnt = userid_read(name, offset, (void *)addr, size);
	if (rdcnt <= 0) {
		pr_err("Read %s failed.\n", name);
		return -1;
	}
	return 0;
}

static int do_userid_write(int argc, char **argv)
{
	unsigned long addr, size, offset;
	char *name, *pe;
	int ret;

	if (argc < 5)
		return -1;
	name = argv[1];
	offset = strtoul(argv[2], &pe, 0);
	size = strtoul(argv[3], &pe, 0);
	addr = strtoul(argv[4], &pe, 0);

	ret = userid_write(name, offset, (void *)addr, size);
	if ((unsigned long)ret != size) {
		pr_err("UserID write failed.\n");
		return -1;
	}

	return 0;
}

static int do_userid_writehex(int argc, char **argv)
{
	int i, j, wrcnt;
	unsigned long size, offset, dlen;
	u8 *data, buf[64];
	u8 byte[3] = {0x00, 0x00, 0x00};
	char *name, *pe;

	if (argc < 4)
		return -1;

	name = argv[1];
	offset = strtoul(argv[2], &pe, 0);
	data = (void *)argv[3];
    dlen = strlen((char *)data);
	size = dlen / 2;
	if (dlen % 2) {
		pr_err("HEX string length shold be even.\n");
		return -1;
	}
	if (size > 64) {
		pr_err("Data size is too large.\n");
		return -1;
	}

	/* hex string to hex value */
	for (i = 0, j = 0; i < dlen - 1; i += 2, j += 1) {
		byte[0] = data[i];
		byte[1] = data[i + 1];
		buf[j] = strtol((char *)byte, &pe, 16);
	}

	wrcnt = userid_write(name, offset, buf, size);
	if (wrcnt != size) {
		pr_err("Failed to write userid %s.\n", name);
		return -1;
	}
	return 0;
}

static int do_userid_writestr(int argc, char **argv)
{
	int wrcnt;
	unsigned long size, offset;
	u8 *data;
	char *name, *pe;

	if (argc < 4)
		return -1;

	name = argv[1];
	offset = strtoul(argv[2], &pe, 0);
	data = (void *)argv[3];
	size = strlen((char *)data);

	wrcnt = userid_write(name, offset, data, size);
	if (wrcnt != size) {
		pr_err("Failed to write userid %s.\n", name);
		return -1;
	}
	return 0;
}

static int do_userid_save(int argc, char **argv)
{
	int ret;

	ret = userid_save();
	if (ret)
		printf("Failed to save UserID to storage.\n");

	return 0;
}

static int do_lock(int flag)
{
	int ret;
	u8 lock;

	lock = flag;
	ret = userid_write("lock", 0, &lock, 1);
	if (ret != 1) {
		pr_err("Failed to write userid lock.\n");
		return -1;
	}
	ret = userid_save();
	if (ret)
		printf("Failed to save UserID to storage.\n");
	return 0;
}

static int do_userid_lock(int argc, char **argv)
{
	return do_lock(1);
}

static int do_userid_unlock(int argc, char **argv)
{
	return do_lock(0);
}

static void do_userid_help(void)
{
    printf("ArtInChip UserID read/write command\n\n");
    printf("userid help                             : Show this help\n");
    printf("userid init                             : Initialize UserID\n");
    printf("userid list                             : List all items' name\n");
    printf("userid import   addr                    : Import userid from RAM\n");
    printf("userid export   addr                    : Export userid to RAM\n");
    printf("userid dump     name offset size        : Dump data in name\n");
    printf("userid size     name [addr]             : Get the ID's data size\n");
    printf("userid read     name offset size addr   : Read data in id to RAM address\n");
    printf("userid write    name offset size addr   : Write data to id from RAM address\n");
    printf("userid remove   name                    : Remove id item\n");
    printf("userid writehex name offset data        : Write data to id from input hex string\n");
    printf("userid writestr name offset data        : Write data to id from input string\n");
    printf("userid lock                             : Lock userid partition\n");
    printf("userid unlock                           : Unlock userid partition\n");
    printf("userid save                             : Save UserID data to storage\n");
    printf("Example:\n");
    printf("    userid init\n");
    printf("    userid list\n");
    printf("    userid writehex testid 0 112233445566778899AABBCCDDEEFF\n");
    printf("    userid dump testid\n");
    printf("    userid save\n");
}
static int cmd_userid_do(int argc, char **argv)
{
    if (argc <= 1)
        goto help;

    if (!strcmp(argv[1], "init")) {
        do_userid_init(argc - 1, &argv[1]);
        return 0;
    }
    if (!strcmp(argv[1], "list")) {
        do_userid_list(argc - 1, &argv[1]);
        return 0;
    }
    if (!strcmp(argv[1], "import")) {
        do_userid_import(argc - 1, &argv[1]);
        return 0;
    }
    if (!strcmp(argv[1], "export")) {
        do_userid_export(argc - 1, &argv[1]);
        return 0;
    }
    if (!strcmp(argv[1], "dump")) {
        do_userid_dump(argc - 1, &argv[1]);
        return 0;
    }
    if (!strcmp(argv[1], "size")) {
        do_userid_size(argc - 1, &argv[1]);
        return 0;
    }
    if (!strcmp(argv[1], "read")) {
        do_userid_read(argc - 1, &argv[1]);
        return 0;
    }
    if (!strcmp(argv[1], "write")) {
        do_userid_write(argc - 1, &argv[1]);
        return 0;
    }
    if (!strcmp(argv[1], "remove")) {
        do_userid_remove(argc - 1, &argv[1]);
        return 0;
    }
    if (!strcmp(argv[1], "writehex")) {
        do_userid_writehex(argc - 1, &argv[1]);
        return 0;
    }
    if (!strcmp(argv[1], "writestr")) {
        do_userid_writestr(argc - 1, &argv[1]);
        return 0;
    }
    if (!strcmp(argv[1], "lock")) {
        do_userid_lock(argc - 1, &argv[1]);
        return 0;
    }
    if (!strcmp(argv[1], "unlock")) {
        do_userid_unlock(argc - 1, &argv[1]);
        return 0;
    }
    if (!strcmp(argv[1], "save")) {
        do_userid_save(argc - 1, &argv[1]);
        return 0;
    }

help:
    do_userid_help();

    return 0;
}

CONSOLE_CMD(userid, cmd_userid_do, "ArtInChip UserID command");
