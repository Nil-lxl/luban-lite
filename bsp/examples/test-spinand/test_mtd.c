/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Jiji.CHen <jiji.chen@artinchip.com>
 */

#include <rtdevice.h>
#include "aic_common.h"
#include <finsh.h>
#define __is_print(ch) ((unsigned int)((ch) - ' ') < 127u - ' ')

static void mtd_dump_hex(const rt_uint8_t *ptr, rt_size_t buflen)
{
    unsigned char *buf = (unsigned char *)ptr;
    int i, j;
    for (i = 0; i < buflen; i += 16)
    {
        rt_kprintf("%06x: ", i);
        for (j = 0; j < 16; j++)
            if (i + j < buflen)
                rt_kprintf("%02x ", buf[i + j]);
            else
                rt_kprintf("   ");
        rt_kprintf(" ");
        for (j = 0; j < 16; j++)
            if (i + j < buflen)
                rt_kprintf("%c", __is_print(buf[i + j]) ? buf[i + j] : '.');
        rt_kprintf("\n");
    }
}

static void show_speed(char *msg, u32 len, u32 us)
{
    u32 tmp, speed;

    /* Split to serval step to avoid overflow */
    tmp = 1000 * len;
    tmp = tmp / us;
    tmp = 1000 * tmp;
    speed = tmp / 1024;

    rt_kprintf("%s: %d byte, %d us -> %d KB/s\n", msg, len, us, speed);
}

int mtd_nandid(const char *name)
{
    struct rt_mtd_nand_device *nand;
    nand = RT_MTD_NAND_DEVICE(rt_device_find(name));
    if (nand == RT_NULL)
    {
        rt_kprintf("no nand device found!\n");
        return -RT_ERROR;
    }

    return rt_mtd_nand_read_id(nand);
}

int mtd_nand_read(const char *name, int block, int page)
{
    rt_err_t result;
    rt_uint8_t *page_ptr;
    rt_uint8_t *oob_ptr;
    struct rt_mtd_nand_device *nand;
    rt_uint64_t start_us;

    nand = RT_MTD_NAND_DEVICE(rt_device_find(name));
    if (nand == RT_NULL)
    {
        rt_kprintf("no nand device found!\n");
        return -RT_ERROR;
    }

    page_ptr = rt_malloc_align(nand->page_size + nand->oob_size, CACHE_LINE_SIZE);
    if (page_ptr == RT_NULL)
    {
        rt_kprintf("out of memory!\n");
        return -RT_ENOMEM;
    }

    oob_ptr = page_ptr + nand->page_size;
    rt_memset(page_ptr, 0xff, nand->page_size + nand->oob_size);

    /* calculate the page number */
    page = block * nand->pages_per_block + page;
    start_us = aic_get_time_us();
    result = rt_mtd_nand_read(nand, page, page_ptr, nand->page_size,
                              oob_ptr, nand->oob_size);

    show_speed("read speed", nand->page_size, aic_get_time_us() - start_us);
    rt_kprintf("read page, rc=%d\n", result);
    mtd_dump_hex(page_ptr, nand->page_size);
    mtd_dump_hex(oob_ptr, nand->oob_size);

    rt_free_align(page_ptr);
    return 0;
}

int mtd_nand_read_cont(const char *name, int block, int page, int size)
{
    rt_err_t result;
    rt_uint8_t *data_ptr;
    struct rt_mtd_nand_device *nand;
    rt_uint64_t start_us;

    nand = RT_MTD_NAND_DEVICE(rt_device_find(name));
    if (nand == RT_NULL)
    {
        rt_kprintf("no nand device found!\n");
        return -RT_ERROR;
    }

    data_ptr = rt_malloc_align(size, CACHE_LINE_SIZE);
    if (data_ptr == RT_NULL)
    {
        rt_kprintf("out of memory!\n");
        return -RT_ENOMEM;
    }

    rt_memset(data_ptr, 0xff, size);

    /* calculate the page number */
    page = block * nand->pages_per_block + page;
    start_us = aic_get_time_us();
    result = rt_mtd_nand_read_cont(nand, page, data_ptr, size);
    show_speed("read speed", size, aic_get_time_us() - start_us);
    rt_kprintf("cont read page, rc=%d\n", result);

    mtd_dump_hex(data_ptr, size);

    rt_free_align(data_ptr);
    return 0;
}

int mtd_nand_readoob(const char *name, int block, int page)
{
    struct rt_mtd_nand_device *nand;
    rt_uint8_t *oob_ptr;

    nand = RT_MTD_NAND_DEVICE(rt_device_find(name));
    if (nand == RT_NULL)
    {
        rt_kprintf("no nand device found!\n");
        return -RT_ERROR;
    }

    oob_ptr = rt_malloc(nand->oob_size);
    if (oob_ptr == RT_NULL)
    {
        rt_kprintf("out of memory!\n");
        return -RT_ENOMEM;
    }

    /* calculate the page number */
    page = block * nand->pages_per_block + page;
    rt_mtd_nand_read(nand, page, RT_NULL, nand->page_size,
                     oob_ptr, nand->oob_size);
    mtd_dump_hex(oob_ptr, nand->oob_size);

    rt_free(oob_ptr);
    return 0;
}

int mtd_nand_write(const char *name, int block, int page)
{
    rt_err_t result;
    rt_uint8_t *page_ptr;
    rt_uint8_t *oob_ptr;
    rt_uint32_t index;
    struct rt_mtd_nand_device *nand;
    rt_uint64_t start_us;

    nand = RT_MTD_NAND_DEVICE(rt_device_find(name));
    if (nand == RT_NULL)
    {
        rt_kprintf("no nand device found!\n");
        return -RT_ERROR;
    }

    page_ptr = rt_malloc_align(nand->page_size + nand->oob_size, CACHE_LINE_SIZE);
    if (page_ptr == RT_NULL)
    {
        rt_kprintf("out of memory!\n");
        return -RT_ENOMEM;
    }

    oob_ptr = page_ptr + nand->page_size;
    /* prepare page data */
    for (index = 0; index < nand->page_size; index ++)
    {
        page_ptr[index] = index & 0xff;
    }
    /* prepare oob data */
    for (index = 0; index < nand->oob_size; index ++)
    {
        oob_ptr[index] = index & 0xff;
    }

    /* calculate the page number */
    page = block * nand->pages_per_block + page;
    start_us = aic_get_time_us();
    result = rt_mtd_nand_write(nand, page, page_ptr, nand->page_size,
                               oob_ptr, nand->oob_size);

    show_speed("write speed", nand->page_size, aic_get_time_us() - start_us);
    if (result != RT_MTD_EOK)
    {
        rt_kprintf("write page failed!, rc=%d\n", result);
    }

    rt_free_align(page_ptr);
    return 0;
}

int mtd_nand_erase(const char *name, int block)
{
    struct rt_mtd_nand_device *nand;
    nand = RT_MTD_NAND_DEVICE(rt_device_find(name));
    if (nand == RT_NULL)
    {
        rt_kprintf("no nand device found!\n");
        return -RT_ERROR;
    }

    return rt_mtd_nand_erase_block(nand, block);
}

int mtd_nand_erase_all(const char *name)
{
    rt_uint32_t index = 0;
    struct rt_mtd_nand_device *nand;

    nand = RT_MTD_NAND_DEVICE(rt_device_find(name));
    if (nand == RT_NULL)
    {
        rt_kprintf("no nand device found!\n");
        return -RT_ERROR;
    }

    for (index = 0; index < (nand->block_end - nand->block_start); index ++)
    {
        rt_mtd_nand_erase_block(nand, index);
    }

    return 0;
}

static void mtd_nand(int argc, char **argv)
{
    /* If the number of arguments less than 2 */
    if (argc < 3)
    {
help:
        rt_kprintf("\n");
        rt_kprintf("mtd_nand [OPTION] [PARAM ...]\n");
        rt_kprintf("         id       <name>            Get nandid by given name\n");
        rt_kprintf("         read     <name> <bn> <pn>  Read data on page <pn> of block <bn> of device <name>\n");
        rt_kprintf("         readcont     <name> <bn> <pn> <size>  Read size data on page <pn> of block <bn> of device <name>\n");
        rt_kprintf("         readoob  <name> <bn> <pn>  Read oob  on page <pn> of block <bn> of device <name>\n");
        rt_kprintf("         write    <name> <bn> <pn>  Run write test on page <pn> of block <bn> of device <name>\n");
        rt_kprintf("         erase    <name> <bn>       Erase on block <bn> of device <name>\n");
        rt_kprintf("         eraseall <name>            Erase all block on device <name>\n");
        return;
    }
    else if (!rt_strcmp(argv[1], "id"))
    {
        mtd_nandid(argv[2]);
    }
    else if (!rt_strcmp(argv[1], "read"))
    {
        if (argc < 5)
        {
            rt_kprintf("The input parameters are too few!\n");
            goto help;
        }
        mtd_nand_read(argv[2], atoi(argv[3]), atoi(argv[4]));
    }
    else if (!rt_strcmp(argv[1], "readcont"))
    {
        if (argc < 6)
        {
            rt_kprintf("The input parameters are too few!\n");
            goto help;
        }
        mtd_nand_read_cont(argv[2], atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
    }
    else if (!rt_strcmp(argv[1], "readoob"))
    {
        if (argc < 5)
        {
            rt_kprintf("The input parameters are too few!\n");
            goto help;
        }
        mtd_nand_readoob(argv[2], atoi(argv[3]), atoi(argv[4]));
    }
    else if (!rt_strcmp(argv[1], "write"))
    {
        if (argc < 5)
        {
            rt_kprintf("The input parameters are too few!\n");
            goto help;
        }
        mtd_nand_write(argv[2], atoi(argv[3]), atoi(argv[4]));
    }
    else if (!rt_strcmp(argv[1], "erase"))
    {
        if (argc < 4)
        {
            rt_kprintf("The input parameters are too few!\n");
            goto help;
        }
        mtd_nand_erase(argv[2], atoi(argv[3]));
    }
    else if (!rt_strcmp(argv[1], "eraseall"))
    {
        mtd_nand_erase_all(argv[2]);
    }
    else
    {
        rt_kprintf("Input parameters are not supported!\n");
        goto help;
    }
}
MSH_CMD_EXPORT(mtd_nand, MTD nand device test function);

#ifndef RT_USING_FINSH_ONLY
FINSH_FUNCTION_EXPORT_ALIAS(mtd_nandid, nand_id, read ID - nandid(name));
FINSH_FUNCTION_EXPORT_ALIAS(mtd_nand_read, nand_read, read page in nand - nand_read(name, block, page));
FINSH_FUNCTION_EXPORT_ALIAS(mtd_nand_readoob, nand_readoob, read spare data in nand - nand_readoob(name, block, page));
FINSH_FUNCTION_EXPORT_ALIAS(mtd_nand_write, nand_write, write dump data to nand - nand_write(name, block, page));
FINSH_FUNCTION_EXPORT_ALIAS(mtd_nand_erase, nand_erase, nand_erase(name, block));
FINSH_FUNCTION_EXPORT_ALIAS(mtd_nand_erase_all, nand_erase_all, erase all of nand device - nand_erase_all(name, block));
#endif /* RT_USING_FINSH_ONLY */
