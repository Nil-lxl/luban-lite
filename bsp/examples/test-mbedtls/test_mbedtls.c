/*
 * Copyright (c) 2024-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <string.h>
#include <finsh.h>
#include <aic_core.h>
#include <aic_utils.h>
#include "crypto.h"


#define USAGE \
    "mbedtls help : Get this information.\n" \
    "mbedtls sha512\n" \
    "mbedtls aesctr\n" \
    "mbedtls aescbc\n" \
    "mbedtls aesgcm\n" \
    "mbedtls random\n" \
    "mbedtls all\n"

static void mbedtls_usage(void)
{
    printf("%s", USAGE);
}

static uint8_t test_data1[256];
static uint8_t test_data2[256];

static void test_sha512(void)
{
    sha_ctx_t *ctx;
    uint8_t md[64];
    int ret, i;

    for (i = 0; i < 256; i++) {
        test_data1[i] = i;
        test_data2[i] = i;
    }

    memset(md, 0, 64);
    ctx = sha_init();
    if (ctx == NULL) {
        printf("init failure.\n");
        return;
    }

    ret = sha_update(ctx, test_data1, 256);
    if (ret) {
        printf("Update data failure.\n");
        return;
    }
    ret = sha_final(ctx, md, 64);
    if (ret) {
        printf("Final failure.\n");
        return;
    }

    /* result should be:
     * 1e 7b 80 bc 8e dc 55 2c 8f ee b2 78 0e 11 14 77 
     * e5 bc 70 46 5f ac 1a 77 b2 9b 35 98 0c 3f 0c e4 
     * a0 36 a6 c9 46 20 36 82 4b d5 68 01 e6 2a f7 e9 
     * fe ba 5c 22 ed 8a 5a f8 77 bf 7d e1 17 dc ac 6d 
    */
    hexdump(md, 64, 1);

    sha_destroy(ctx);
}

static void test_aes_ctr(void)
{
    uint8_t key[AES_128_BLOCK_SIZE];
    uint8_t iv[AES_128_BLOCK_SIZE];
    int i;
    aes_ctx_t *ctx;

    for (i = 0; i < AES_128_BLOCK_SIZE; i++) {
        key[i] = (128 + i) % 256;
        iv[i] = (128 + i) % 256;
    }
    for (i = 0; i < 256; i++) {
        test_data1[i] = i;
        test_data2[i] = 0;
    }

    ctx = aes_ctr_init(key, iv);

    aes_ctr_encrypt(ctx, test_data1, test_data2, 256);
    printf("AES 128 CTR Encrypt:\n");
    hexdump(test_data2, 256, 1);

    /* Should be:
     * eb e4 ec 93 f6 bf 53 86 90 a1 fa dd 07 65 39 a4 
     * fa 3e 11 8e d5 21 4b 8e 3e ad 05 f1 ba aa 4e ca 
     * a7 8d 7f 93 7f 8b df fb 7c c6 c0 f3 c5 cd 80 de 
     * 20 18 3a c3 2e f8 78 4c b0 f1 79 0e 7e 19 22 11 
     * 7e 0e 42 ff 8e 26 56 82 a0 e7 77 4f ae 35 b1 a3 
     * 7f 4a 55 d0 42 e3 16 53 b8 c0 a5 30 97 ce 2e cb 
     * e1 47 e3 1c e1 f4 50 86 71 61 78 00 a6 d6 81 e6 
     * 9c e4 2b e0 45 97 02 47 b2 ec 38 90 03 9e fc 16 
     * 3c 5d 2e 11 76 c4 8d 47 63 28 34 bd a7 47 08 fa 
     * c0 9f 21 d6 18 b7 88 aa 16 fb 48 fb 51 7b ac 40 
     * 3b c2 19 f4 dd a4 07 f7 c3 02 bd 13 d7 b0 ea aa 
     * f7 b0 18 c0 b4 dd ae f3 15 64 a2 dd b0 14 94 b1 
     * 15 da 51 44 9a df 24 36 8b 3a aa 1b d3 1b 84 1e 
     * ac e9 3c af 27 89 1f 26 13 01 ee df 6a 0e 06 1d 
     * 06 12 f8 44 3f ab ae c7 94 cb 85 5c fd 94 b2 58 
     * 72 60 64 18 34 f0 e4 0e 70 c1 8d 98 b3 d2 78 c1 
     */
    aes_ctr_reset(ctx);
    memset(test_data1, 0, 256);

    aes_ctr_decrypt(ctx, test_data2, test_data1, 256);
    printf("AES 128 CTR Decrypt:\n");
    hexdump(test_data1, 256, 1);

    aes_ctr_destroy(ctx);
}

static void test_aes_cbc(void)
{
    uint8_t key[AES_128_BLOCK_SIZE];
    uint8_t iv[AES_128_BLOCK_SIZE];
    int i;
    aes_ctx_t *ctx;

    for (i = 0; i < AES_128_BLOCK_SIZE; i++) {
        key[i] = (128 + i) % 256;
        iv[i] = (128 + i) % 256;
    }
    for (i = 0; i < 256; i++) {
        test_data1[i] = i;
        test_data2[i] = 0;
    }

    /* Encrypt test */
    ctx = aes_cbc_init(key, iv, AES_ENCRYPT);
    aes_cbc_encrypt(ctx, test_data1, test_data2, 256);
    printf("AES 128 CBC Encrypt:\n");
    hexdump(test_data2, 256, 1);
    aes_cbc_destroy(ctx);

    /* Should be:
     * b5 41 37 d1 62 6a 26 ae fa 99 4b f3 b4 63 20 ad 
     * c0 3e 17 c7 29 4a f1 c2 86 ce f1 d5 04 96 c6 50 
     * 1e 44 6c 7e 63 66 75 07 69 00 87 ed 0e 70 c5 df 
     * a1 0e 70 59 1e 67 1a 91 24 3c 16 e5 29 71 25 cf 
     * f5 c1 b0 90 01 70 be 3d 63 36 d8 0a b8 ff 08 32 
     * 9d 44 7d 78 0f ad d2 09 5f 59 a3 05 41 5f 8b 9d 
     * 96 21 1c 9b d0 2b 0c 4b b3 13 cc 1e 1b 40 93 7d 
     * 8b 99 8a c4 b7 d9 7a 4b 76 87 27 0d e6 3f b8 4e 
     * df 3c 4c be c9 62 09 db fb 74 8f a5 73 37 4c bc 
     * d7 9c 44 26 33 5f 16 1c 91 6f 4c 94 30 b0 3b 52 
     * f3 4b 70 53 f6 79 5e a5 6a 89 15 f6 b8 a2 84 40 
     * 52 36 35 71 d0 b3 04 57 7f 31 e0 57 6b 20 e2 11 
     * e5 c5 20 12 20 8a 7c 39 2d b7 bf b7 6a cc 9d 17 
     * 1a 67 8d 15 f8 7e ac 29 b6 49 f2 cf cf 73 8d da 
     * f2 19 c5 c7 6b cd bd d9 10 04 95 8f 0c 12 ed 95 
     * a5 d0 0d e3 59 05 5d 77 d7 a5 88 87 1e 35 b9 7d 
     */

    /* Decrypt test */
    ctx  = aes_cbc_init(key, iv, AES_DECRYPT);
    memset(test_data1, 0, 256);

    aes_cbc_decrypt(ctx, test_data2, test_data1, 256);
    printf("AES 128 CBC Decrypt:\n");
    hexdump(test_data1, 256, 1);

    aes_cbc_destroy(ctx);
}

static void test_aes_gcm(void)
{
    uint8_t key[AES_128_BLOCK_SIZE];
    uint8_t iv[AES_128_BLOCK_SIZE];
    uint8_t tag[AES_128_BLOCK_SIZE];
    int i;

    for (i = 0; i < AES_128_BLOCK_SIZE; i++) {
        key[i] = (128 + i) % 256;
        iv[i] = (128 + i) % 256;
    }
    for (i = 0; i < 256; i++) {
        test_data1[i] = i;
        test_data2[i] = 0;
    }

    /* Encrypt test */
    aes_gcm_encrypt(test_data1, 256, test_data2, key, iv, tag);
    printf("AES 128 GCM Encrypt:\n");
    hexdump(test_data2, 256, 1);
    /*
     * 19 e7 99 38 e5 ed af 67 84 aa 6f 08 6d df 8c 01 
     * 6d fb 9f 8e d2 e7 38 95 47 ea 2c e0 4f 5f bc de 
     * 2a 5e 0d 02 0e f8 88 85 c7 44 77 f2 91 30 a5 9e 
     * 1e 40 59 14 9b 0f 61 57 91 3c 2f 2b a2 0e 74 ce 
     * 2a 10 66 5f ac 51 e6 82 d5 b8 e7 4b 7a b4 4e b6 
     * fd a4 e8 f9 88 03 bb 36 11 03 58 2c f0 7c be 1e 
     * d2 b4 7e 9a 80 a8 75 70 ab a1 9b 6f 96 26 30 46 
     * bb a8 12 69 d5 3f f3 20 2f 59 14 48 5f 5e 15 76 
     * 9b 96 fb eb e7 32 54 57 fc dd 11 cb 2b a7 0c 88 
     * ce 3a b0 b9 45 84 2a a5 a8 d6 ca b9 f5 d2 b5 12 
     * fe fe 2e bc ca cf a7 a9 3b 54 65 5f 4d ff 2e 53 
     * 3a dd a3 7d 4c a8 4e 75 b6 72 4d 8d 1d 95 7f ee 
     * 69 18 0a b4 de b0 9f 5f c0 15 cb 80 6c 2b 85 93 
     * 2b d7 c6 53 60 4a 7a 2a 8a ed e1 0f f9 b6 c9 25 
     * 07 b1 0b 94 69 7c cd f6 79 d5 ee c3 dc c9 1b a4 
     * af 72 14 45 c1 9e f1 4f a8 8f 91 59 a4 c6 53 8c 
     */
    printf("AES 128 GCM tag:\n");
    hexdump(tag, 16, 1);
    /*
     * 16 9a 6c 0f 32 de ba 5c 08 b5 15 a1 dc 92 e5 de
     */

    memset(test_data1, 0, 256);
    aes_gcm_decrypt(test_data2, 256, test_data1, key, iv, tag);
    printf("AES 128 GCM Decrypt:\n");
    hexdump(test_data1, 256, 1);
}

static void test_rand(void)
{
    int ret;

    ret = get_random_bytes(test_data1, 16);

    printf("rand 16, ret = %d\n", ret);
    hexdump(test_data1, 16, 1);

    ret = get_random_bytes(test_data1, 32);

    printf("rand 32, ret = %d\n", ret);
    hexdump(test_data1, 32, 1);
}

static int cmd_test_mbedtls(int argc, char **argv)
{
    if (argc < 2)
        goto help;

    if (!rt_strcmp(argv[1], "help")) {
        goto help;
    }
    if (!rt_strcmp(argv[1], "sha512")) {
        test_sha512();
        return -1;
    }
    if (!rt_strcmp(argv[1], "aesctr")) {
        test_aes_ctr();
        return -1;
    }
    if (!rt_strcmp(argv[1], "aescbc")) {
        test_aes_cbc();
        return -1;
    }
    if (!rt_strcmp(argv[1], "aesgcm")) {
        test_aes_gcm();
        return -1;
    }
    if (!rt_strcmp(argv[1], "random")) {
        test_rand();
        return -1;
    }
    if (!rt_strcmp(argv[1], "all")) {
        test_sha512();
        test_aes_ctr();
        test_aes_cbc();
        test_aes_gcm();
        test_rand();
        return -1;
    }

    return 0;
help:
    mbedtls_usage();
    return -1;
}

MSH_CMD_EXPORT_ALIAS(cmd_test_mbedtls, mbedtls, mbedtls example);
