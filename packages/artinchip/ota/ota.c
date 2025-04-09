/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */
#define DBG_ENABLE
#define DBG_SECTION_NAME "ota"
#ifdef OTA_DOWNLOADER_DEBUG
#define DBG_LEVEL DBG_LOG
#else
#define DBG_LEVEL DBG_INFO
#endif
#define DBG_COLOR
#include <rtconfig.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <finsh.h>
#include <string.h>
#include "aic_core.h"
#include <ota.h>
#include <env.h>
#include <absystem_os.h>
#include <burn.h>
#include <ctype.h>
#include <rtdbg.h>

#define ALIGN_xB_UP(x, y) (((x) + (y - 1)) & ~(y - 1))

#define MAX_CPIO_FILE_NAME 18
#define MAX_IMAGE_FNAME 32

enum flag_cpio {
    FLAG_CPIO_HEAD1,
    FLAG_CPIO_HEAD2,
    FLAG_CPIO_HEAD3,
    FLAG_CPIO_HEAD4,
    FLAG_CPIO_HEAD,
    FLAG_CPIO_FILE,
};

struct fileinfo {
    unsigned int file_size;
    char file_version[32];
    char file[MAX_CPIO_FILE_NAME][MAX_IMAGE_FNAME];
    char device[MAX_CPIO_FILE_NAME][MAX_IMAGE_FNAME];
};

struct filehdr {
    unsigned int format;
    unsigned int size;
    unsigned int size_align;
    unsigned int begin_offset;
    unsigned int cpio_header_len;
    unsigned int namesize;
    unsigned int filename_size;
    unsigned int filename_align;
    unsigned int burn_len;
    unsigned int chksum;
    unsigned int sum;
    char filename[MAX_IMAGE_FNAME];
};

struct bufhdr {
    int valid_size; //valid data size
    int head;   //start address of valid data
    int size;   //buffer size
    char *buf;
};

enum cpio_fields {
    C_MAGIC,
    C_INO,
    C_MODE,
    C_UID,
    C_GID,
    C_NLINK,
    C_MTIME,
    C_FILESIZE,
    C_MAJ,
    C_MIN,
    C_RMAJ,
    C_RMIN,
    C_NAMESIZE,
    C_CHKSUM,
    C_NFIELDS
};

#ifdef OTA_DOWNLOADER_DEBUG
static unsigned int test_sum = 0;
static unsigned int test_leng = 0;
#endif

static unsigned char flag_cpio = FLAG_CPIO_HEAD1; /*upgrade file name index*/
static unsigned char cpio_or_file = FLAG_CPIO_HEAD;
/*parse header info or file content*/

static struct bufhdr bhdr = { 0 };  /*burn buffer*/
static struct bufhdr shdr = { 0 };  /*head buffer*/
static struct filehdr fhdr = { 0 }; /*upgrade file info*/
static struct fileinfo finfo = { 0 }; /* cpio file info */

unsigned int cpio_file_checksum(unsigned char *buffer, unsigned int length)
{
    unsigned int sum = 0;
    int i = 0;

    for (i = 0; i < length; i++)
        sum += buffer[i];

#ifdef OTA_DOWNLOADER_DEBUG
    test_sum += sum;
    test_leng += length;
    printf("%s sum = 0x%x length = %u\n", __func__, test_sum, test_leng);
#endif
    return sum;
}

static void ota_buf_init(struct bufhdr *hdr, char *buf, int size)
{
    hdr->buf = buf;
    hdr->size = size;
    hdr->valid_size = 0;
    hdr->head = 0;
}

static int ota_buf_push(struct bufhdr *hdr, char *data, int len)
{
    if ((hdr->valid_size + len) > hdr->size) {
        LOG_E("Queue overflow,please increase buffer size.data len:%d buffer total size:%d",
            len, hdr->size);
        return -RT_ERROR;
    }

    rt_memcpy(hdr->buf + hdr->valid_size, data, len);
    hdr->valid_size += len;

#ifdef OTA_DOWNLOADER_DEBUG
    int i;
    printf("%s:\n", __func__);
    for (i = 0; i < 20; i++)
        printf("0x%x ", data[i]);
    printf("\n");

    printf("%s hdr->valid_size = %d\n", __func__, hdr->valid_size);
#endif

    return RT_EOK;
}

static void print_progress(size_t cur_size, size_t total_size)
{
    static unsigned char progress_sign[100 + 1];
    uint8_t i, per = cur_size * 100 / total_size;
    static unsigned char per_size = 0;

    if (per > 100) {
        per = 100;
    }

    if (per_size == per)
        return;

    for (i = 0; i < 100; i++) {
        if (i < per) {
            progress_sign[i] = '=';
        } else if (per == i) {
            progress_sign[i] = '>';
        } else {
            progress_sign[i] = ' ';
        }
    }

    progress_sign[sizeof(progress_sign) - 1] = '\0';

    LOG_I("Download: [%s] %03d%%\033[1A", progress_sign, per);

    per_size = per;
}

static int parse_cpio_file_info(struct bufhdr *bhdr, struct filehdr *fhdr, struct fileinfo *finfo)
{
    int ret = RT_EOK;

    if (fhdr->size != OTA_CPIO_INFO_LEN) {
        LOG_E("please check the ota_info.bin size! error size:%d", fhdr->size);
        return -1;
    }

    /*when the buffer len is insufficient, process after receive data next time*/
    if (bhdr->valid_size < OTA_CPIO_INFO_LEN)
        return 0;

    char *ptr = bhdr->buf;
    char cpio_file_size[32] = {0};
    char type[16] = {0};//store [image]or[file]...

    /* ota_info.bin file parse code */
    while(ptr < bhdr->buf + OTA_CPIO_INFO_LEN) {
        if (*ptr == '[') {
            if (strncmp(ptr, "[image]", 7) == 0) {
                strncpy(type, ptr, 7);
                ptr += strlen(type) + 1;//skip the type and zero
                /* start to parse image content */
                if (*ptr == 's' && *(ptr + 1) == 'i' && \
                    *(ptr + 2) == 'z' && *(ptr + 3) == 'e') {
                    ptr += 8;
                    char *size_start = ptr;
                    while (*ptr != '"' && (ptr < bhdr->buf + OTA_CPIO_INFO_LEN))
                        ptr++;
                    int size_len = ptr - size_start;
                    strncpy(cpio_file_size, size_start, size_len);
                    finfo->file_size = atoi(cpio_file_size);
                    LOG_I("cpio file size:%d", finfo->file_size);
                    ptr += 3;//skip the semicolon and zero
                } else {
                    LOG_E("no cpio file size content!");
                }
                if (*ptr == 'v' && *(ptr + 1) == 'e' && *(ptr + 2) == 'r' && *(ptr + 3) == 's' \
                    && *(ptr + 4) == 'i' && *(ptr + 5) == 'o' && *(ptr + 6) == 'n') {
                    ptr += 11;
                    char *version_start = ptr;
                    while (*ptr != '"' && (ptr < bhdr->buf + OTA_CPIO_INFO_LEN))
                        ptr++;
                    int version_len = ptr - version_start;
                    strncpy(finfo->file_version, version_start, version_len);
                    LOG_I("cpio file version:%s", finfo->file_version);
                    ptr += 3;//skip the semicolon and zero
                } else {
                    LOG_E("no cpio file version content!");
                }
            } else if (strncmp(ptr, "[file]", 6) == 0) {
                strncpy(type, ptr, 6);
                ptr += strlen(type) + 1;//skip the type and zero
                /* start to parse file content */
                char *file_start = ptr;
                char *deivce_start = NULL;
                int colon_num = 0, semico_num = 0, file_name_len = 0, device_name_len = 0;
                while (ptr < bhdr->buf + OTA_CPIO_INFO_LEN) {
                    if (*ptr == ':') {
                        file_name_len = ptr - file_start;
                        strncpy(finfo->file[colon_num], file_start, file_name_len);
                        finfo->file[colon_num][file_name_len] = '\0';
                        ptr++;
                        colon_num++;
                        deivce_start = ptr;
                        continue;
                    }
                    if (*ptr == ';') {
                        device_name_len = ptr - deivce_start;
                        strncpy(finfo->device[semico_num], deivce_start, device_name_len);
                        ptr += 2;//skip the zero
                        semico_num++;
                        file_start = ptr;
                        continue;
                    }
                    ptr++;
                }
            }
        } else {
            //not '['
            ptr++;
        }
    }

#ifdef OTA_DOWNLOADER_DEBUG
    for (int i = 0; i < MAX_CPIO_FILE_NAME; i++)
        printf("[%d]: file name= %s device info= %s\n", i, finfo->file[i], finfo->device[i]);
#endif

    rt_memcpy(bhdr->buf, bhdr->buf + OTA_CPIO_INFO_LEN,
                  bhdr->valid_size - OTA_CPIO_INFO_LEN);
    bhdr->valid_size -= OTA_CPIO_INFO_LEN;
    /*transfer bhdr all data to shdr*/
    ret = ota_buf_push(&shdr, bhdr->buf + bhdr->head, bhdr->valid_size);
    if (ret < 0) {
        return ret;
    } else {
        /*bhdr all data has been passed to shdr*/
        bhdr->valid_size = 0;
        bhdr->head = 0;
    }

    return 1;
}

/*
 * mmc defaults to the next partition as the
 * back up partition
 */
static void increment_last_digit(char *str)
{
    int len = strlen(str);

    for (int i = len - 1; i >= 0; i--) {
        int temp = (int)str[i];
        if (isdigit(temp)) {
            str[i]++;
            break;
        }
    }
}

char *ota_upgrade_get_partname(char *file_name)
{
    int i = 0;
    char *now = NULL;
    char *ret = NULL;

    if (strlen(file_name) > MAX_IMAGE_FNAME) {
        LOG_E("file name %s is too long, max size %d", file_name, MAX_IMAGE_FNAME);
        return ret;
    }

    for (i = 0; i < MAX_CPIO_FILE_NAME; i++) {
        if (strncmp(file_name, finfo.file[i], strlen(file_name)) == 0)
            break;
    }

    if (i == MAX_CPIO_FILE_NAME) {
        LOG_E("not found the file:%s index", file_name);
        return NULL;
    }

    enum boot_device boot_dev = aic_get_boot_device();

    if (fw_env_open()) {
        LOG_E("Open env failed");
        return ret;
    }

    if (strncmp("rodata.fatfs", file_name, 12) == 0) {
        now = fw_getenv("rodataAB_now");
        LOG_I("rodataAB_now = %s", now);
        if (strncmp(now, "A", 1) == 0) {
            LOG_I("Upgrade B rodatafs");
            if (boot_dev == BD_SDMC0)
                increment_last_digit(finfo.device[i]);
            else
                strncat(finfo.device[i], "_r", sizeof(finfo.device[i]) - strlen(finfo.device[i]) - 1);

            ret = finfo.device[i];
        } else if (strncmp(now, "B", 1) == 0) {
            LOG_I("Upgrade A rodatafs");
            ret = finfo.device[i];
        } else {
            ret = NULL;
            LOG_E("invalid rodataAB_now");
        }
    } else if (strncmp("data.fatfs", file_name, 10) == 0) {
        now = fw_getenv("dataAB_now");
        LOG_I("dataAB_now = %s", now);
        if (strncmp(now, "A", 1) == 0) {
            LOG_I("Upgrade B datafs");
            if (boot_dev == BD_SDMC0)
                increment_last_digit(finfo.device[i]);
            else
                strncat(finfo.device[i], "_r", sizeof(finfo.device[i]) - strlen(finfo.device[i]) - 1);

            ret = finfo.device[i];
        } else if (strncmp(now, "B", 1) == 0) {
            LOG_I("Upgrade A datafs");
            ret = finfo.device[i];
        } else {
            ret = NULL;
            LOG_E("invalid dataAB_now");
        }
    } else if (strstr(file_name, ".itb") != NULL) {
        now = fw_getenv("osAB_now");
        LOG_I("osAB_now = %s", now);
        if (strncmp(now, "A", 1) == 0) {
            LOG_I("Upgrade B system");
            if (boot_dev == BD_SDMC0)
                increment_last_digit(finfo.device[i]);
            else
                strncat(finfo.device[i], "_r", sizeof(finfo.device[i]) - strlen(finfo.device[i]) - 1);

            ret = finfo.device[i];
        } else if (strncmp(now, "B", 1) == 0) {
            LOG_I("Upgrade A system");
            ret = finfo.device[i];
        } else {
            ret = NULL;
            LOG_E("invalid osAB_now");
        }
    } else {
        ret = NULL;
        LOG_E("Get partname error, please check the file name!");
    }

    fw_env_close();

    return ret;
}

/*
 * if the valid data in the buffer exceeds OTA_BURN_LEN,
 * start burn data
 */
static int download_buf_pop(struct bufhdr *bhdr, struct filehdr *fhdr)
{
    int ret = RT_EOK;
    int burn_len = 0;
    int burn_last = 0;

download_buf_pop_last:
    /*Last upgrade data*/
    if (fhdr->size - fhdr->begin_offset < bhdr->valid_size) {
        /*upgrade in two stags*/
        if (fhdr->size - fhdr->begin_offset > OTA_BURN_LEN) {
            burn_len = OTA_BURN_LEN;
            burn_last = 1;
        } else if (fhdr->size - fhdr->begin_offset == OTA_BURN_LEN) {
            burn_len = OTA_BURN_LEN;
#ifdef OTA_DOWNLOADER_DEBUG
            LOG_I("Burn the last data size = %d!", OTA_BURN_LEN);
#endif
        } else {
            burn_len = fhdr->size - fhdr->begin_offset;
#ifdef OTA_DOWNLOADER_DEBUG
            LOG_I("Burn the last data size = %d!", burn_len);
#endif
        }
    } else {
        /*when the buffer len is insufficient, process after receive data next time*/
        if (bhdr->valid_size < OTA_BURN_LEN)
            return 0;
        else
            burn_len = OTA_BURN_LEN;
    }

    /* Write the data to the corresponding partition address */
    ret = aic_ota_part_write(fhdr->begin_offset, (const uint8_t *)bhdr->buf,
                             burn_len);
    if (ret)
        return -RT_ERROR;

    fhdr->sum += cpio_file_checksum((unsigned char *)bhdr->buf, burn_len);

    fhdr->begin_offset += burn_len;
    print_progress(fhdr->begin_offset, fhdr->size);

    if (burn_len == OTA_BURN_LEN) {
        rt_memcpy(bhdr->buf, bhdr->buf + OTA_BURN_LEN,
                  bhdr->valid_size - OTA_BURN_LEN);
        bhdr->valid_size -= OTA_BURN_LEN;
        if (fhdr->size - fhdr->begin_offset <= 0) {
            bhdr->head += fhdr->size_align;
            bhdr->valid_size -= fhdr->size_align;
            /*transfer bhdr all data to shdr*/
            ret = ota_buf_push(&shdr, bhdr->buf + bhdr->head, bhdr->valid_size);
            if (ret < 0) {
                return ret;
            } else {
                /*bhdr all data has been passed to shdr*/
                bhdr->valid_size = 0;
                bhdr->head = 0;
            }
        }
    } else {
        bhdr->head += (burn_len + fhdr->size_align);
        bhdr->valid_size -= (burn_len + fhdr->size_align);
        /*transfer bhdr all data to shdr*/
        ret = ota_buf_push(&shdr, bhdr->buf + bhdr->head, bhdr->valid_size);
        if (ret < 0) {
            return ret;
        } else {
            /*bhdr all data has been passed to shdr*/
            bhdr->valid_size = 0;
            bhdr->head = 0;
        }
    }

    /*Start upgrade of remain data*/
    if (burn_last == 1) {
        burn_last = 0;
        goto download_buf_pop_last;
    }

    return 0;
}

/*
 * find_cpio_data - Search for files in an uncompressed cpio
 * @fhdr:     struct filehdr containing the address, length and
 *              filename (with the directory path cut off) of the found file.
 * @data:       Pointer to the cpio archive or a header inside
 * @len:        Remaining length of the cpio based on data pointer
 * @return:     0 is success,others is failed
 */
int find_cpio_data(struct filehdr *fhdr, void *data, size_t len)
{
    const char *p;
    unsigned int *chp, v;
    unsigned char c, x;
    int i, j;
    unsigned int ch[C_NFIELDS];
    int file_end_offset = 0;
    int cpio_header_len = 0;

    fhdr->cpio_header_len = 8 * C_NFIELDS - 2;

    p = data;

    if (!*p) {
        /* All cpio headers need to be 4-byte aligned */
        LOG_I("*p = 0x%x\n", *p);
        p += 4;
        len -= 4;
    }

    j = 6; /* The magic field is only 6 characters */
    chp = ch;
    for (i = C_NFIELDS; i; i--) {
        v = 0;
        while (j--) {
            v <<= 4;
            c = *p++;

            x = c - '0';
            if (x < 10) {
                v += x;
                continue;
            }

            x = (c | 0x20) - 'a';
            if (x < 6) {
                v += x + 10;
                continue;
            }

            LOG_E("error 1 Invalid hexadecimal\n");
            goto quit; /* Invalid hexadecimal */
        }
        *chp++ = v;
        j = 8; /* All other fields are 8 characters */
    }

    if ((ch[C_MAGIC] - 0x070701) > 1) {
        LOG_E("error 2 Invalid magic\n");
        goto quit; /* Invalid magic */
    }

#ifdef OTA_DOWNLOADER_DEBUG
    for (i = 0; i < 14; i++)
        printf("ch[%d] = 0x%x\n", i, ch[i]);
#endif

    if ((ch[C_MODE] & 0170000) == 0100000) {
        if (ch[C_NAMESIZE] >= MAX_CPIO_FILE_NAME) {
            LOG_E("File %s exceeding MAX_CPIO_FILE_NAME [%d]\n", p,
                  MAX_CPIO_FILE_NAME);
        }
        strncpy(fhdr->filename, p, ch[C_NAMESIZE]);

        fhdr->format = ch[C_MAGIC];
        fhdr->size = ch[C_FILESIZE]; /*upgrade file size*/
        fhdr->begin_offset = 0;
        fhdr->filename_size = ch[C_NAMESIZE];
        fhdr->chksum = ch[C_CHKSUM];
        fhdr->sum = 0;

        /*filename align offset*/
        file_end_offset = fhdr->cpio_header_len + fhdr->filename_size;
        fhdr->filename_align =
            ALIGN_xB_UP(file_end_offset, 4) - file_end_offset;
#ifdef OTA_DOWNLOADER_DEBUG
        printf("fhdr->filename_align = %u %d %d\n", fhdr->filename_align,
               file_end_offset, ALIGN_xB_UP(file_end_offset, 4));
#endif

        /*Parse of cpio header info,filename and align data,next parse if Incomplete*/
        cpio_header_len =
            fhdr->cpio_header_len + fhdr->filename_size + fhdr->filename_align;
        if (len < cpio_header_len) {
            LOG_E("Incomplete filename and align data!\n");
            return -1;
        }

        /*file align offset*/
        file_end_offset = fhdr->size;
        fhdr->size_align = ALIGN_xB_UP(file_end_offset, 4) - file_end_offset;

#ifdef OTA_DOWNLOADER_DEBUG
        printf("fhdr->size_align = %u %d %d\n", fhdr->size_align,
               file_end_offset, ALIGN_xB_UP(file_end_offset, 4));
        test_sum = 0;
        test_leng = 0;
#endif

        LOG_I("find file %s cpio data success\n", fhdr->filename);
        return 0; /* Found it! */
    } else {
        strncpy(fhdr->filename, p, ch[C_NAMESIZE]);
        fhdr->filename_size = ch[C_NAMESIZE];
        LOG_I("find file %s cpio data success\n", fhdr->filename);
        return 0; /* Found it! */
    }

quit:
    LOG_E("find file in cpio data failed\n");
    return -1;
}

/*
 * Remove cpio header info, filename and align data
 * Then transfer shdr remain data to bhdr
 */
static int head_buf_pop(struct bufhdr *bhdr, struct bufhdr *shdr,
                        struct filehdr *fhdr)
{
    int ret = RT_EOK;
    int len =
        fhdr->cpio_header_len + fhdr->filename_size + fhdr->filename_align;

    /*Remove cpio header info*/
    shdr->head += len;
    shdr->valid_size -= len;

    if (shdr->valid_size > 0) {
#ifdef OTA_DOWNLOADER_DEBUG
        int i;
        printf("%s shdr->valid_size = %d, len = %d\n", __func__, shdr->valid_size, len);
        printf("%s:\n", __func__);
        for (i = 0; i < 20; i++)
            printf("0x%x ", (shdr->buf + shdr->head)[i]);
#endif
        ret = ota_buf_push(bhdr, shdr->buf + shdr->head, shdr->valid_size);
        if (ret)
            return ret;

        shdr->valid_size = 0;
        shdr->head = 0;
    } else {
        shdr->head = 0;
    }

    return ret;
}

int ota_init(void)
{
    int ret = RT_EOK;
    char *buf = NULL, *buffer = NULL;

    buf = aicos_malloc_align(0, OTA_BURN_BUFF_LEN, CACHE_LINE_SIZE);
    if (!buf) {
        LOG_E("malloc buf failed\n");
        ret = -RT_ERROR;
        goto __exit;
    }

    buffer = rt_malloc(OTA_HEAD_LEN);
    if (!buffer) {
        LOG_E("malloc buffer failed\n");
        ret = -RT_ERROR;
        goto __exit;
    }

    ota_buf_init(&bhdr, buf, OTA_BURN_BUFF_LEN);
    ota_buf_init(&shdr, buffer, OTA_HEAD_LEN);

    return RT_EOK;

__exit:
    if (buf)
        aicos_free_align(0, buf);

    if (buffer)
        rt_free(buffer);

    return ret;
}

void ota_deinit(void)
{
    if (bhdr.buf)
        aicos_free_align(0, bhdr.buf);

    if (shdr.buf)
        rt_free(shdr.buf);
}

/* function, you can store data and so on */
int ota_shard_download_fun(char *buffer, int length)
{
    int ret = RT_EOK;
    int len = 8 * C_NFIELDS - 2;
    char *partname = NULL;

    /*
     * Start default parsing of cpio header info,filename and align data
     * Afterwards, parse the file content
     */
    if (cpio_or_file != FLAG_CPIO_FILE) {
        ret = ota_buf_push(&shdr, buffer, length);
        if (ret)
            goto __download_exit;

    ota_shard_download_handle_last:

        if (shdr.valid_size < len) {
            LOG_I("Incomplete header info! shdr.valid_size = %d", shdr.valid_size);
            goto __download_exit;
        }

        /*
         * Find complete header info, if not, direct return
         */
        ret = find_cpio_data(&fhdr, shdr.buf, shdr.valid_size);
        if (ret < 0) {
            LOG_E("Not find file info\n");
            goto __download_exit;
        }

        /*TRAILER!!! is the last file, unpack is over*/
        ret = rt_strncmp(fhdr.filename, "TRAILER!!!", fhdr.filename_size);
        if (ret == 0) {
            goto __download_exit;
        }

        ret = head_buf_pop(&bhdr, &shdr, &fhdr);
        if (ret < 0) {
            LOG_E("head_buf_pop error!\n");
            goto __download_exit;
        }

        ret = rt_strncmp(fhdr.filename, "ota_info.bin", fhdr.filename_size);
        if (ret == 0) {
            ret = parse_cpio_file_info(&bhdr, &fhdr, &finfo);
            if (ret < 0) {
                LOG_E("Parsing cpio file info is error!\n");
                goto __download_exit;
            } else if (ret == 0) {
#ifdef OTA_DOWNLOADER_DEBUG
                printf("data is not enough to parse, continue to store data!\n");
                printf("bhdr.valid_size:%d bhdr.size:%d bhdr.head:%d\n", bhdr.valid_size, bhdr.size, bhdr.head);
#endif
            } else {
                LOG_I("Parsing cpio file info once is sufficient and successful!");
                cpio_or_file = FLAG_CPIO_HEAD;
                flag_cpio++;
                ret = RT_EOK;
                goto ota_shard_download_handle_last;
            }
        } else {
            partname = ota_upgrade_get_partname(fhdr.filename);
            if (partname == NULL)
                goto __download_exit;

            LOG_I("Start upgrade to %s, flag_cpio:%d!", partname, flag_cpio);

            ret = aic_ota_find_part(partname);
            if (ret)
                goto __download_exit;

            ret = aic_ota_erase_part();
            if (ret)
                goto __download_exit;

            LOG_I("Start upgrade %s!", fhdr.filename);

            ret = download_buf_pop(&bhdr, &fhdr);
            if (ret < 0) {
                LOG_E("download_buf_pop error! len = %d\n", len);
                goto __download_exit;
            }
        }
        cpio_or_file = FLAG_CPIO_FILE;
    } else { /*Parse file content*/
        ret = ota_buf_push(&bhdr, buffer, length);
        if (ret)
            goto __download_exit;

        ret = rt_strncmp(fhdr.filename, "ota_info.bin", fhdr.filename_size);
        if (ret == 0) {
            ret = parse_cpio_file_info(&bhdr, &fhdr, &finfo);
            if (ret < 0) {
                LOG_E("parse cpio file info error!");
                goto __download_exit;
            } else if (ret == 0) {
#ifdef OTA_DOWNLOADER_DEBUG
                printf("data is not enough to parse, continue to store data!\n");
                printf("bhdr.valid_size:%d bhdr.size:%d bhdr.head:%d\n", bhdr.valid_size, bhdr.size, bhdr.head);
#endif
            } else {
                LOG_I("Parsing cpio file info is successful!");
                cpio_or_file = FLAG_CPIO_HEAD;
                flag_cpio++;
                ret = RT_EOK;
                goto ota_shard_download_handle_last;
            }
        } else {
            ret = download_buf_pop(&bhdr, &fhdr);
            if (ret < 0) {
                LOG_E("download_buf_pop error! len = %d\n", len);
                goto __download_exit;
            }

            if (fhdr.size <= fhdr.begin_offset) {
#ifdef OTA_DOWNLOADER_DEBUG
                LOG_I("fhdr.size = %d fhdr.begin_offset = %d\n", fhdr.size,
                      fhdr.begin_offset);
#endif
                if (fhdr.sum == fhdr.chksum) {
                    LOG_I("Sum check success!");
                    LOG_I("download %s success!\n", fhdr.filename);
                    aic_set_upgrade_status(fhdr.filename);
                } else {
                    LOG_E(
                        "Sum check failed, fhdr->sum = 0x%x,fhdr->chksum = 0x%x\n",
                        fhdr.sum, fhdr.chksum);
                    ret = -RT_ERROR;
                    goto __download_exit;
                }

                cpio_or_file = FLAG_CPIO_HEAD;
                flag_cpio++;

                goto ota_shard_download_handle_last;
            }
        }
    }

__download_exit:
    return ret;
}


