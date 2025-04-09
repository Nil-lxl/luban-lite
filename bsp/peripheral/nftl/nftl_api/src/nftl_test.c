/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Mingfeng.Li <mingfeng.li@artinchip.com>
 */

#include "nftl_port.h"
#include "nftl_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dfs_fs.h>
#include <msh.h>
#include "../../../../kernel/rt-thread/components/drivers/include/drivers/mtd_nand.h"

#ifdef RT_USING_FINSH

#ifdef AIC_NFLT_TEST

#define BUF_1MB_SIZE  1024 * 1024
#define BUF_256K_SIZE (256 * 1024)

typedef enum {
    NFTL_TEST_SUCCESS = 0,
    NFTL_TEST_FAILURE = 1,
    NFTL_TEST_MALLOC_FAILED = 2,
    NFTL_TEST_FILE_OPEN_FAILED = 3,
    NFTL_TEST_DATA_CHECK_FAILED = 4,
} _nftl_test_error;

static void nftl_show_speed(char *msg, u32 len, u32 us)
{
    u32 tmp, speed;

    /* Split to serval step to avoid overflow */
    tmp = 1000 * len;
    tmp = tmp / us;
    tmp = 1000 * tmp;
    speed = tmp / 1024;

    rt_kprintf("%s: %d byte, %d us -> %d KB/s\n", msg, len, us, speed);
}

static void write_speed(int argc, char **argv)
{
    uint32_t start_us;
    int fd = -1;
    int size_b = 1024;
    int remain_size = 0;
    char file_path[128] = { 0 };
    u32 *buf = (u32 *)0x40000000;

    rt_kprintf("File Write Speed Test\n");
    rt_sprintf(file_path, "%s\n", "/data/temp.txt");

    switch (argc) {
        case 3:
            size_b = atoi(argv[2]);
        case 2:
            rt_sprintf(file_path, "%s\n", argv[1]);
            break;
        default:
            rt_kprintf("write_speed <file_path> <size_b>\r\n");
            rt_kprintf("file_path  : write files (default is %s)\r\n",
                       file_path);
            rt_kprintf("size_b  : write size (B)(default is %d B)\r\n", size_b);
            return;
    }

    rt_kprintf("%s:%d buf=0x%x, size_b=%d, file_path=%s\n", __FUNCTION__,
               __LINE__, buf, size_b, file_path);

    fd = open(file_path, O_RDWR | O_CREAT);
    if (fd == -1) {
        rt_kprintf("Failed to open file for write.\n");
        return;
    }

    remain_size = size_b;

    start_us = aic_get_time_us();

    while (remain_size > 0) {
        if (remain_size > BUF_1MB_SIZE) {
            write(fd, (uint32_t *)buf, BUF_1MB_SIZE);
            remain_size -= BUF_1MB_SIZE;
        } else {
            write(fd, (uint32_t *)buf, remain_size);
            break;
        }
    }

    fsync(fd);

    nftl_show_speed("Write speed:", size_b, aic_get_time_us() - start_us);
    close(fd);
}
MSH_CMD_EXPORT(write_speed, write_speed function);

static void read_speed(int argc, char **argv)
{
    uint32_t start_us;
    int fd = -1;
    char file_path[128] = { 0 };

    int size_b = 1024;
    int read_size = 0;
    int file_size = 0;
    int remain_size = 0;
    void *addr = NULL;

    rt_sprintf(file_path, "%s\n", "/data/temp.txt");

    switch (argc) {
        case 4:
            size_b = atoi(argv[3]);
        case 3:
            rt_sprintf(file_path, "%s\n", argv[2]);
        case 2:
            addr = (void *)strtoul(argv[1], NULL, 0);
            break;
        default:
            rt_kprintf("read_speed <addr> <file_path> <read_size>\r\n");
            rt_kprintf("addr  : read to addr\r\n");
            rt_kprintf("file_path  : write files (default is %s)\r\n",
                       file_path);
            return;
    }

    rt_kprintf("%s:%d addr=0x%x, size_b=%d, file_path=%s\n", __FUNCTION__,
               __LINE__, addr, size_b, file_path);

    fd = open(file_path, O_RDONLY);
    file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    read_size = size_b < file_size ? size_b : file_size;

    remain_size = read_size;

    start_us = aic_get_time_us();

    while (remain_size > 0) {
        if (remain_size > BUF_1MB_SIZE) {
            read(fd, (uint32_t *)addr, BUF_1MB_SIZE);
            remain_size -= BUF_1MB_SIZE;
        } else {
            read(fd, (uint32_t *)addr, remain_size);
            break;
        }
    }

    nftl_show_speed("fatfs read speed", read_size,
                    aic_get_time_us() - start_us);

    close(fd);
}
MSH_CMD_EXPORT(read_speed, read_speed function);

static void nftl_stress_test_usage()
{
    rt_kprintf("Usage:\r\n");
    rt_kprintf(
        "nftl_stress rw <count> --- Random Read And Write Stress Test \r\n");
    rt_kprintf("nftl_stress full <count> --- Full Write Stress Test \r\n");
    rt_kprintf("nftl_stress api <count> --- API Stress Test \r\n");
    rt_kprintf(
        "nftl_stress build <start(or end)> --- Startup Build Flow Test\r\n");
}

static void nftl_stress_rw_test(int count)
{
    int ret = 0;
    int i = 0, j = 0;
    int fd = -1;
    int rand_size = 0;
    int buffer_size = BUF_256K_SIZE;
    char file_path[128] = { 0 };
    int *write_buf = NULL;
    int *read_buf = NULL;

    rt_kprintf("NFTL Stress Test Read And Write Start\n");
    /*malloc buffer for write and read data*/
    write_buf = aicos_malloc(MEM_CMA, buffer_size);
    if (write_buf == RT_NULL) {
        ret = NFTL_TEST_MALLOC_FAILED;
        goto NFTL_STRESS_TEST_FAILED;
    }

    read_buf = aicos_malloc(MEM_CMA, buffer_size);
    if (read_buf == RT_NULL) {
        ret = NFTL_TEST_MALLOC_FAILED;
        goto NFTL_STRESS_TEST_FAILED;
    }

    for (i = 0; i < count; i++) {
        memset(file_path, 0, sizeof(file_path));
        snprintf(file_path, sizeof(file_path), "/data/temp%d.txt\n", i);

        /*write data to file*/
        fd = open(file_path, O_RDWR | O_CREAT);
        if (fd == -1) {
            ret = NFTL_TEST_FILE_OPEN_FAILED;
            perror("Error");
            goto NFTL_STRESS_TEST_FAILED;
        }
        rand_size = rand() % buffer_size;
        rand_size = rand_size >= 512 ? rand_size : 512;
        memset(read_buf, 0, rand_size);

        write(fd, (int *)write_buf, rand_size);
        fsync(fd);

        lseek(fd, 0, SEEK_SET);
        read(fd, (int *)read_buf, rand_size);
        close(fd);

        /*remove file*/
        unlink(file_path);

        /*check read and write data*/
        for (j = 0; j < rand_size >> 2; j++) {
            if (read_buf[j] != write_buf[j]) {
                ret = NFTL_TEST_DATA_CHECK_FAILED;
                rt_kprintf("nftl test err %d(0x%x~0x%x)!\n", j, read_buf[j],
                           write_buf[j]);
                goto NFTL_STRESS_TEST_FAILED;
            }
        }

        if (i%100 == 0) {
            rt_kprintf("NFTL Stress Test Read And Write count %d\n", i);
        }
    }
    aicos_free(MEM_CMA, write_buf);
    aicos_free(MEM_CMA, read_buf);
    rt_kprintf("NFTL Stress Test Success!\n");
    return;

NFTL_STRESS_TEST_FAILED:

    if (write_buf != NULL)
        aicos_free(MEM_CMA, write_buf);
    if (read_buf != NULL)
        aicos_free(MEM_CMA, read_buf);
    rt_kprintf("NFTL Stress Test Failed(%d,%d)!\n", ret, i);
}

static void nftl_stress_full_test(int count)
{
    int ret = 0;
    int i = 0, j = 0;
    int fd = -1;
    int rand_size = 0;
    int buffer_size = BUF_256K_SIZE;
    char file_path[128] = { 0 };
    int *write_buf = NULL;
    int last_index = 0;
    struct statfs fs_stat;
    unsigned long free_size;

    rt_kprintf("NFTL Stress Test Full Start\n");
    /*malloc buffer for write and read data*/
    write_buf = aicos_malloc(MEM_CMA, buffer_size);
    if (write_buf == RT_NULL) {
        ret = NFTL_TEST_MALLOC_FAILED;
        goto NFTL_STRESS_TEST_FAILED;
    }

    for (i = 0; i < count; i++) {
        memset(file_path, 0, sizeof(file_path));
        snprintf(file_path, sizeof(file_path), "/data/temp%d.txt\n", i);

        /*write data to file*/
        fd = open(file_path, O_RDWR | O_CREAT);
        if (fd == -1) {
            ret = NFTL_TEST_FILE_OPEN_FAILED;
            rt_kprintf("open %s ", file_path);
            perror("Error");
            goto NFTL_STRESS_TEST_FAILED;
        }
        rand_size = rand() % buffer_size;
        rand_size = rand_size >= 512 ? rand_size : 512;

        write(fd, (int *)write_buf, rand_size);
        fsync(fd);
        close(fd);

        /*get file system info and remove old file*/
        if (statfs("/data", &fs_stat) == 0) {
            free_size = fs_stat.f_bfree * fs_stat.f_bsize;
            if (free_size <= BUF_256K_SIZE) {
                rt_kprintf("FS /data Free: %lu bytes, calc times %d\n",
                           free_size, i);

                /*remove 10 files every times*/
                for (j = 0; j < 10; j++) {
                    memset(file_path, 0, sizeof(file_path));
                    snprintf(file_path, sizeof(file_path), "/data/temp%d.txt\n",
                             last_index + j);

                    /*remove file*/
                    unlink(file_path);
                }
                last_index += 10;
            }
        }
    }

    aicos_free(MEM_CMA, write_buf);

    rt_kprintf("NFTL Stress Test Success!\n");
    return;

NFTL_STRESS_TEST_FAILED:

    if (write_buf != NULL)
        aicos_free(MEM_CMA, write_buf);
    rt_kprintf("NFTL Stress Test Failed(%d,%d)!\n", ret, i);
}

static void nftl_stress_api_test(int count);

static void nftl_stress_test(int argc, char **argv)
{
    if (argc < 3) {
        nftl_stress_test_usage();
        return;
    }

    if (strcmp(argv[1], "rw") == 0) {
        nftl_stress_rw_test(atoi(argv[2]));
    } else if (strcmp(argv[1], "full") == 0) {
        nftl_stress_full_test(atoi(argv[2]));
    } else if (strcmp(argv[1], "api") == 0) {
        nftl_stress_api_test(atoi(argv[2]));
    } else if (strcmp(argv[1], "build") == 0) {
        if (strcmp("start", argv[2]) == 0) {
            rt_kprintf("NFTL Build Test Start\n");
            msh_exec("aicupg \n", 8);
        } else if (strcmp("end", argv[2]) == 0) {
            rt_kprintf("NFTL Stress Test Success\n");
        }
    } else {
        nftl_stress_test_usage();
    }
}
MSH_CMD_EXPORT(nftl_stress_test, nftl_stress_test function);

//#include <dfs_posix.h>
#define PATH_MAX 512
int df_folder(const char *folder_path)
{
    struct statfs fs_stat;

    // get file system info
    if (statfs(folder_path, &fs_stat) == 0) {
        unsigned long total_size = fs_stat.f_blocks * fs_stat.f_bsize;
        unsigned long free_size = fs_stat.f_bfree * fs_stat.f_bsize;
        unsigned long used_size = total_size - free_size;

        rt_kprintf("Total: %lu bytes\n", total_size);
        rt_kprintf("Used: %lu bytes\n", used_size);
        rt_kprintf("Free: %lu bytes\n", free_size);
    } else {
        rt_kprintf("Failed to get filesystem information\n");
    }

    return 0;
}

static void df_cmd(int argc, char **argv)
{
    df_folder(argv[1]);
}
MSH_CMD_EXPORT(df_cmd, NFTL nand device df_cmd function);

int weak_debug(void)
{
    rt_kprintf("%s:%d ...\n", __FUNCTION__, __LINE__);
    return 0;
}

static void weak_test(int argc, char **argv)
{
    weak_debug();
}
MSH_CMD_EXPORT(weak_test, weak_test function);

#define __is_print(ch) ((unsigned int)((ch) - ' ') < 127u - ' ')
static void rt_nftl_dump_hex(const rt_uint8_t *ptr, rt_size_t buflen)
{
    unsigned char *buf = (unsigned char *)ptr;
    int i, j;
    for (i = 0; i < buflen; i += 16) {
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

struct spinand_blk_device {
    struct rt_device parent;
    struct rt_device_blk_geometry geometry;
    struct rt_mtd_nand_device *mtd_device;
#ifdef AIC_NFTL_SUPPORT
    struct nftl_api_handler_t *nftl_handler;
#endif
    char name[32];
    u8 *pagebuf;
};
#define RT_BLOCK_NAND_DEVICE(device) ((struct spinand_blk_device *)(device))
static struct nftl_api_handler_t *handler = NULL;
void _nftl_api_init(char *name)
{
    struct spinand_blk_device *blk_device;
    blk_device = RT_BLOCK_NAND_DEVICE(rt_device_find(name));
    if (blk_device == RT_NULL) {
        rt_kprintf("no blk_device device found!\n");
        return;
    }
    handler = blk_device->nftl_handler;

    rt_kprintf("handler->priv=0x%lx\n", (ulong)handler->priv);
}

static void nftl_api_inti_test(int argc, char **argv)
{
    if (handler == NULL) {
        if (argc < 2) {
            rt_kprintf("input blk name\n");
            return;
        }
        _nftl_api_init(argv[1]);
    } else {
        rt_kprintf("inited, handler->nandt=0x%lx\n", (ulong)handler->nandt);
    }
}
MSH_CMD_EXPORT(nftl_api_inti_test, NFTL nand device nftl_api_inti_test function);

static void nftl_api_dump_info_test(int argc, char **argv)
{
    if (handler == NULL) {
        if (argc < 2) {
            rt_kprintf("input blk name\n");
            return;
        }
        _nftl_api_init(argv[1]);
    } else {
        rt_kprintf("inited, handler->nandt=0x%lx\n", (ulong)handler->nandt);
    }
    nftl_api_print_nftl_area_node(handler);
}
MSH_CMD_EXPORT(nftl_api_dump_info_test, NFTL nand device nftl_api_dump_info_test function);

static void nftl_api_print_free_list_test(int argc, char **argv)
{
    if (handler == NULL) {
        if (argc < 2) {
            rt_kprintf("input blk name\n");
            return;
        }
        _nftl_api_init(argv[1]);
    } else {
        rt_kprintf("inited, handler->nandt=0x%lx\n", (ulong)handler->nandt);
    }
    nftl_api_print_free_list(handler);
}
MSH_CMD_EXPORT(nftl_api_print_free_list_test, NFTL nand device nftl_api_print_free_list_test function);

static void nftl_api_print_block_invalid_list_test(int argc, char **argv)
{
    if (handler == NULL) {
        if (argc < 2) {
            rt_kprintf("input blk name\n");
            return;
        }
        _nftl_api_init(argv[1]);
    } else {
        rt_kprintf("inited, handler->nandt=0x%lx\n", (ulong)handler->nandt);
    }
    nftl_api_print_block_invalid_list(handler);
}
MSH_CMD_EXPORT(nftl_api_print_block_invalid_list_test, NFTL nftl_api_print_block_invalid_list_test function);

static void nftl_api_print_logic_page_map_test(int argc, char **argv)
{
    if (handler == NULL) {
        if (argc < 2) {
            rt_kprintf("input blk name\n");
            return;
        }
        _nftl_api_init(argv[1]);
    } else {
        rt_kprintf("inited, handler->nandt=0x%lx\n", (ulong)handler->nandt);
    }
    nftl_api_print_logic_page_map(handler);
}
MSH_CMD_EXPORT(nftl_api_print_logic_page_map_test, nftl_api_print_free_list_test function);

static void nftl_api_cache_flush_test(int argc, char **argv)
{
    if (handler == NULL) {
        if (argc < 2) {
            rt_kprintf("input blk name\n");
            return;
        }
        _nftl_api_init(argv[1]);
    } else {
        rt_kprintf("inited, handler->nandt=0x%lx\n", (ulong)handler->nandt);
    }
    nftl_api_write_cache(handler, 0xffff);
}
MSH_CMD_EXPORT(nftl_api_cache_flush_test, NFTL nftl_api_cache_flush_test function);

/*argv0: cmd; argv1: blk_devicce, argv2: addr, argv3: start_sector, argv4: count, argv5: loop_times*/
static void nftl_api_test(int argc, char **argv)
{
    if (handler == NULL) {
        if (argc < 2) {
            rt_kprintf("input blk name\n");
            return;
        }
        _nftl_api_init(argv[1]);
    }

    u8 *write_buf_512;
    write_buf_512 = rt_malloc(NFTL_SECTOR_SIZE);
    if (write_buf_512 == RT_NULL) {
        rt_kprintf("%s:%d: write_buf_512 out of memory!\n", __FUNCTION__,
                   __LINE__);
        return;
    }
    memset(write_buf_512, 0x5a, NFTL_SECTOR_SIZE);

    nftl_api_write(handler, 125, 1, write_buf_512);
    memset(write_buf_512, 0, NFTL_SECTOR_SIZE);
    nftl_api_read(handler, 125, 1, write_buf_512);
    rt_nftl_dump_hex(write_buf_512, NFTL_SECTOR_SIZE);

    rt_free(write_buf_512);
}
MSH_CMD_EXPORT(nftl_api_test, NFTL nand device nftl_api_test function);

static void nftl_stress_api_test(int count)
{
    if (handler == NULL) {
        _nftl_api_init("blk_data");
    }
    int ret = 0;
    int i = 0, j = 0, k = 0;
    int rand_sector_count = 0;
    int rand_size = 0;
    int buffer_size = BUF_256K_SIZE;
    int *write_buf = NULL;
    int *read_buf = NULL;
    int logic_sector_count = 0;

    struct statfs fs_stat;

    rt_kprintf("NFTL Stress Test Read And Write Start\n");

    /*get logic page number */
    if (statfs("/data", &fs_stat) == 0) {
        logic_sector_count =
            fs_stat.f_blocks * fs_stat.f_bsize / NFTL_SECTOR_SIZE;
        rt_kprintf("Total sector count: %d\n", logic_sector_count);
    }

    if (logic_sector_count == 0) {
        rt_kprintf("NFTL Stress Test Failed!\n");
        return;
    }

    /*malloc buffer for write and read data*/
    write_buf = aicos_malloc(MEM_CMA, buffer_size);
    if (write_buf == RT_NULL) {
        ret = NFTL_TEST_MALLOC_FAILED;
        goto NFTL_STRESS_TEST_FAILED;
    }

    read_buf = aicos_malloc(MEM_CMA, buffer_size);
    if (read_buf == RT_NULL) {
        ret = NFTL_TEST_MALLOC_FAILED;
        goto NFTL_STRESS_TEST_FAILED;
    }

    for (i = 0; i < count; i++) {
        j = 0;
        rt_kprintf("NFTL API Stress Test Times: %d\n", i);

        while (j < logic_sector_count) {
            rand_sector_count = rand() % logic_sector_count;
            rand_size = rand_sector_count * NFTL_SECTOR_SIZE;

            /*check buffer size*/
            if (rand_size > buffer_size) {
                rand_sector_count = buffer_size / NFTL_SECTOR_SIZE;
                rand_size = rand_sector_count * NFTL_SECTOR_SIZE;
            }

            if (j + rand_sector_count >= logic_sector_count) {
                break;
            }

            /*write and read logic page*/
            nftl_api_write(handler, j, rand_sector_count,
                           (unsigned char *)write_buf);
            nftl_api_read(handler, j, rand_sector_count,
                          (unsigned char *)read_buf);

            /*check read and write data*/
            for (k = 0; k < (rand_size >> 2); k++) {
                if (read_buf[k] != write_buf[k]) {
                    ret = NFTL_TEST_DATA_CHECK_FAILED;
                    rt_kprintf("nftl test err page%d %d(0x%x~0x%x)!\n", j, k,
                               read_buf[k], write_buf[k]);
                    goto NFTL_STRESS_TEST_FAILED;
                }
            }
            j += rand_sector_count;
        }
    }

    aicos_free(MEM_CMA, write_buf);
    aicos_free(MEM_CMA, read_buf);
    rt_kprintf("NFTL Stress Test Success!\n");
    return;

NFTL_STRESS_TEST_FAILED:

    if (write_buf != NULL)
        aicos_free(MEM_CMA, write_buf);
    if (read_buf != NULL)
        aicos_free(MEM_CMA, read_buf);
    rt_kprintf("NFTL Stress Test Failed(%d,%d)!\n", ret, i);
}

#define BUFFER_SIZE 8192
static void cat_file(int argc, char **argv)
{
    FILE *fd;
    uint8_t *buffer;

    if (argc != 2) {
        pr_err("Usage %s: %s <file name>.\n", __func__, __func__);
        return;
    }

    buffer = (uint8_t *)aicos_malloc(MEM_CMA, BUFFER_SIZE);
    if (buffer == RT_NULL) {
        pr_err("buffer: no memory\n");
        return;
    }

    u32 read_bytes = 0;
    u32 file_sizes = 0;
    /* open the ‘/text.txt’ file in read-only mode */

    fd = fopen(argv[1], "rb+");
    if (fd >= 0) {
        while ((read_bytes = fread(buffer, sizeof(uint8_t), BUFFER_SIZE, fd))) {
            file_sizes += read_bytes;
            rt_nftl_dump_hex(buffer, read_bytes);
        }

        pr_info("sizes: %d\n", file_sizes);
        fclose(fd);
    }

    if (buffer)
        aicos_free(MEM_CMA, buffer);

    return;
}

MSH_CMD_EXPORT(cat_file, cat cat_file and Calculate the file crc32);

static void mtd_dump_oob(int argc, char **argv)
{
    struct rt_mtd_nand_device *mtd_device;
    mtd_device = RT_MTD_NAND_DEVICE(rt_device_find(argv[1]));
    if (mtd_device == RT_NULL) {
        rt_kprintf("no mtd_device device found!\n");
        rt_free(mtd_device);
        return;
    }

    u8 *nftl_oob_buf;
    nftl_oob_buf = rt_malloc(16);
    if (nftl_oob_buf == RT_NULL) {
        rt_kprintf("%s:%d: nftl_oob_buf out of memory!\n", __FUNCTION__,
                   __LINE__);
        return;
    }
    memset(nftl_oob_buf, 0x00, 16);

#define OOB_LEN mtd_device->oob_size
    u8 *oob_buf;
    oob_buf = rt_malloc(OOB_LEN);
    if (oob_buf == RT_NULL) {
        rt_kprintf("%s:%d: oob_buf out of memory!\n", __FUNCTION__, __LINE__);
        return;
    }
    memset(oob_buf, 0x00, OOB_LEN);

    int rodata_pages = (mtd_device->block_total) * mtd_device->pages_per_block;
    int block = 0;
    for (int i = 0; i < rodata_pages; i++) {
        rt_mtd_nand_read(mtd_device, i, NULL, 0, oob_buf, OOB_LEN);
        rt_kprintf("block[%d].page[%d].oob:\n", block, i);
        if ((i % (mtd_device->pages_per_block - 1) == 0) && (i != 0)) {
            block++;
        }
        rt_nftl_dump_hex(oob_buf, OOB_LEN);
        memcpy(nftl_oob_buf, oob_buf, 4);
        memcpy(nftl_oob_buf + 4, oob_buf + 16, 4);
        memcpy(nftl_oob_buf + 8, oob_buf + 32, 4);
        memcpy(nftl_oob_buf + 12, oob_buf + 48, 4);
        nftl_spare_info(nftl_oob_buf);
    }
}
MSH_CMD_EXPORT(mtd_dump_oob, mtd_dump_oob function);

static void mtd_dump_page(int argc, char **argv)
{
    struct rt_mtd_nand_device *mtd_device;

    if (argc < 3) {
        rt_kprintf("mtd_dump_page <mtd_name> <block> <page>\n");
        return;
    }

    mtd_device = RT_MTD_NAND_DEVICE(rt_device_find(argv[1]));
    if (mtd_device == RT_NULL) {
        rt_kprintf("no mtd_device device found!\n");
        rt_free(mtd_device);
        return;
    }

#define PAGE_SIZE mtd_device->page_size
    u8 *page_buf;
    page_buf = rt_malloc(PAGE_SIZE);
    if (page_buf == RT_NULL) {
        rt_kprintf("%s:%d: page_buf out of memory!\n", __FUNCTION__, __LINE__);
        return;
    }
    memset(page_buf, 0x00, PAGE_SIZE);

    int page = atoi(argv[2]) * mtd_device->pages_per_block + atoi(argv[3]);

    rt_mtd_nand_read(mtd_device, page, page_buf, PAGE_SIZE, NULL, 0);
    rt_kprintf("block[%d].page[%d]:\n", atoi(argv[2]), atoi(argv[3]));
    rt_nftl_dump_hex(page_buf, PAGE_SIZE);

    rt_free(page_buf);
}
MSH_CMD_EXPORT(mtd_dump_page, mtd_dump_page function);

#endif // NFTL CMD DEBUG

static void nftl_show_version(int argc, char **argv)
{
    nftl_api_lib_info();
}
MSH_CMD_EXPORT(nftl_show_version, show nftl version);

#endif /* RT_USING_FINSH */
