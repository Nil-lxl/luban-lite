/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Xiong Hao <hao.xiong@artinchip.com>
 */

#include <string.h>
#include <aic_core.h>
#include <aic_utils.h>
#include <hal_ce.h>
#include <ssram.h>
#include "skcipher.h"

#define CE_CIPHER_MAX_KEY_SIZE  64
#define CE_CIPHER_MAX_IV_SIZE   AES_BLOCK_SIZE
#define CE_CIPHER_MAX_DATA_SIZE 0x3FF00
#define FLG_DEC                 BIT(0)
#define FLG_AES                 BIT(1)
#define FLG_DES                 BIT(2)
#define FLG_TDES                BIT(3)
#define FLG_ECB                 BIT(4)
#define FLG_CBC                 BIT(5)
#define FLG_CTR                 BIT(6)
#define FLG_CTS                 BIT(7)
#define FLG_XTS                 BIT(8)
#define FLG_GENHSK              BIT(9) /* Gen HSK(Hardware Secure Key) to SSRAM */

#define FLG_KEYOFFSET (16)
#define FLG_KEYMASK   GENMASK(19, 16) /* eFuse Key ID */

#define AES_MIN_KEY_SIZE      16
#define AES_MAX_KEY_SIZE      32
#define AES_KEYSIZE_128       16
#define AES_KEYSIZE_192       24
#define AES_KEYSIZE_256       32
#define AES_BLOCK_SIZE        16
#define AES_MAX_KEYLENGTH     (15 * 16)
#define AES_MAX_KEYLENGTH_U32 (AES_MAX_KEYLENGTH / sizeof(u32))

#define DES_KEY_SIZE     8
#define DES_EXPKEY_WORDS 32
#define DES_BLOCK_SIZE   8

#define DES3_EDE_KEY_SIZE     (3 * DES_KEY_SIZE)
#define DES3_EDE_EXPKEY_WORDS (3 * DES_EXPKEY_WORDS)
#define DES3_EDE_BLOCK_SIZE   DES_BLOCK_SIZE

struct aic_skcipher_alg {
    struct skcipher_alg alg;
};

struct aic_skcipher_tfm_ctx {
    unsigned char *inkey;
    int inkeylen;
};

struct aic_skcipher_req_ctx {
    struct crypto_task *task;
    unsigned char *key;
    unsigned char *iv;
    unsigned char *backup_ivs; /* Back up iv for CBC decrypt */
    unsigned int ssram_addr;
    unsigned int next_iv; /* Next IV address for CBC encrypt */
    int tasklen;
    int keylen;
    int ivsize;
    int blocksize;
    int backup_iv_cnt;
    unsigned long flags;
    void *src_cpy_buf;
    void *dst_cpy_buf;
    bool src_map_sg;
    bool dst_map_sg;
};

static inline bool is_aes_ecb(unsigned long flg)
{
    return (flg & FLG_AES && flg & FLG_ECB);
}

static inline bool is_aes_cbc(unsigned long flg)
{
    return (flg & FLG_AES && flg & FLG_CBC);
}

static inline bool is_aes_ctr(unsigned long flg)
{
    return (flg & FLG_AES && flg & FLG_CTR);
}

static inline bool is_aes_cts(unsigned long flg)
{
    return (flg & FLG_AES && flg & FLG_CTS);
}

static inline bool is_aes_xts(unsigned long flg)
{
    return (flg & FLG_AES && flg & FLG_XTS);
}

static inline bool is_des_ecb(unsigned long flg)
{
    return (flg & FLG_DES && flg & FLG_ECB);
}

static inline bool is_des_cbc(unsigned long flg)
{
    return (flg & FLG_DES && flg & FLG_CBC);
}

static inline bool is_tdes_ecb(unsigned long flg)
{
    return (flg & FLG_TDES && flg & FLG_ECB);
}

static inline bool is_tdes_cbc(unsigned long flg)
{
    return (flg & FLG_TDES && flg & FLG_CBC);
}

static inline bool is_cbc_dec(unsigned long flg)
{
    return (flg & FLG_DEC && flg & FLG_CBC);
}

static inline bool is_cbc_enc(unsigned long flg)
{
    return ((~flg) & FLG_DEC && flg & FLG_CBC);
}

static inline bool is_cbc(unsigned long flg)
{
    return (flg & FLG_CBC);
}

static inline bool is_ctr(unsigned long flg)
{
    return (flg & FLG_CTR);
}

static inline bool is_dec(unsigned long flg)
{
    return (flg & FLG_DEC);
}

static inline bool is_enc(unsigned long flg)
{
    return ((~flg) & FLG_DEC);
}

static inline bool is_need_genhsk(unsigned long flg)
{
    return (flg & (FLG_KEYMASK));
}

static inline bool is_gen_hsk(unsigned long flg)
{
    return (flg & FLG_GENHSK);
}

static u32 aic_skcipher_keysize(int keylen)
{
    u32 ksize = 0;

    switch (keylen) {
        case 8:
            ksize = KEY_SIZE_64;
            break;
        case 16:
            ksize = KEY_SIZE_128;
            break;
        case 24:
            ksize = KEY_SIZE_192;
            break;
        case 32:
            ksize = KEY_SIZE_256;
            break;
    }

    return ksize;
}

static void aic_skcipher_task_cfg(struct crypto_task *task,
                                  struct aic_skcipher_req_ctx *rctx, u32 din,
                                  u32 dout, u32 dlen, u32 first_flag,
                                  u32 last_flag)
{
    u32 opdir, keysrc, keysize;

    if (is_aes_xts(rctx->flags))
        keysize = aic_skcipher_keysize(rctx->keylen / 2);
    else
        keysize = aic_skcipher_keysize(rctx->keylen);
    opdir = rctx->flags & FLG_DEC;

    task->data.in_addr = cpu_to_le32(din);
    task->data.in_len = cpu_to_le32(dlen);
    task->data.out_addr = cpu_to_le32(dout);
    task->data.out_len = cpu_to_le32(dlen);

    if (is_gen_hsk(rctx->flags))
        keysrc = ((rctx->flags) & FLG_KEYMASK) >> FLG_KEYOFFSET;
    else
        keysrc = CE_KEY_SRC_USER;

    if (is_aes_ecb(rctx->flags)) {
        task->alg.aes_ecb.alg_tag = ALG_AES_ECB;
        task->alg.aes_ecb.direction = opdir;
        task->alg.aes_ecb.key_siz = keysize;
        task->alg.aes_ecb.key_src = keysrc;
        if (rctx->ssram_addr)
            task->alg.aes_ecb.key_addr = rctx->ssram_addr;
        else
            task->alg.aes_ecb.key_addr = (u32)(uintptr_t)rctx->key;
    } else if (is_aes_cbc(rctx->flags)) {
        task->alg.aes_cbc.alg_tag = ALG_AES_CBC;
        task->alg.aes_cbc.direction = opdir;
        task->alg.aes_cbc.key_siz = keysize;
        task->alg.aes_cbc.key_src = keysrc;
        if (rctx->ssram_addr)
            task->alg.aes_cbc.key_addr = rctx->ssram_addr;
        else
            task->alg.aes_cbc.key_addr = (u32)(uintptr_t)rctx->key;
        if (rctx->next_iv)
            task->alg.aes_cbc.iv_addr = rctx->next_iv;
        else
            task->alg.aes_cbc.iv_addr = (u32)(uintptr_t)rctx->iv;
    } else if (is_aes_ctr(rctx->flags)) {
        task->alg.aes_ctr.alg_tag = ALG_AES_CTR;
        task->alg.aes_ctr.direction = opdir;
        task->alg.aes_ctr.key_siz = keysize;
        task->alg.aes_ctr.key_src = keysrc;
        task->alg.aes_ctr.key_addr = (u32)(uintptr_t)rctx->key;
        task->alg.aes_ctr.ctr_in_addr = (u32)(uintptr_t)rctx->iv;
        task->alg.aes_ctr.ctr_out_addr = (u32)(uintptr_t)rctx->iv;
    } else if (is_aes_cts(rctx->flags)) {
        task->alg.aes_cts.alg_tag = ALG_AES_CTS;
        task->alg.aes_cts.direction = opdir;
        task->alg.aes_cts.key_siz = keysize;
        task->alg.aes_cts.key_src = keysrc;
        if (rctx->ssram_addr)
            task->alg.aes_cts.key_addr = rctx->ssram_addr;
        else
            task->alg.aes_cts.key_addr = (u32)(uintptr_t)rctx->key;
        if (rctx->next_iv)
            task->alg.aes_cts.iv_addr = rctx->next_iv;
        else
            task->alg.aes_cts.iv_addr = (u32)(uintptr_t)rctx->iv;
        task->data.last_flag = last_flag;
    } else if (is_aes_xts(rctx->flags)) {
        task->alg.aes_xts.alg_tag = ALG_AES_XTS;
        task->alg.aes_xts.direction = opdir;
        task->alg.aes_xts.key_siz = keysize;
        task->alg.aes_xts.key_src = keysrc;
        if (rctx->ssram_addr)
            task->alg.aes_xts.key_addr = rctx->ssram_addr;
        else
            task->alg.aes_xts.key_addr = (u32)(uintptr_t)rctx->key;
        task->alg.aes_xts.tweak_addr = (u32)(uintptr_t)rctx->iv;
        task->data.first_flag = first_flag;
        task->data.last_flag = last_flag;
    } else if (is_des_ecb(rctx->flags)) {
        task->alg.des_ecb.alg_tag = ALG_DES_ECB;
        task->alg.des_ecb.direction = opdir;
        task->alg.des_ecb.key_siz = keysize;
        task->alg.des_ecb.key_src = keysrc;
        task->alg.des_ecb.key_addr = (u32)(uintptr_t)rctx->key;
    } else if (is_des_cbc(rctx->flags)) {
        task->alg.des_cbc.alg_tag = ALG_DES_CBC;
        task->alg.des_cbc.direction = opdir;
        task->alg.des_cbc.key_siz = keysize;
        task->alg.des_cbc.key_src = keysrc;
        task->alg.des_cbc.key_addr = (u32)(uintptr_t)rctx->key;
        if (rctx->next_iv)
            task->alg.des_cbc.iv_addr = rctx->next_iv;
        else
            task->alg.des_cbc.iv_addr = (u32)(uintptr_t)rctx->iv;
    } else if (is_tdes_ecb(rctx->flags)) {
        task->alg.des_ecb.alg_tag = ALG_TDES_ECB;
        task->alg.des_ecb.direction = opdir;
        task->alg.des_ecb.key_siz = keysize;
        task->alg.des_ecb.key_src = keysrc;
        task->alg.des_ecb.key_addr = (u32)(uintptr_t)rctx->key;
    } else if (is_tdes_cbc(rctx->flags)) {
        task->alg.des_cbc.alg_tag = ALG_TDES_CBC;
        task->alg.des_cbc.direction = opdir;
        task->alg.des_cbc.key_siz = keysize;
        task->alg.des_cbc.key_src = keysrc;
        task->alg.des_cbc.key_addr = (u32)(uintptr_t)rctx->key;
        if (rctx->next_iv)
            task->alg.des_cbc.iv_addr = rctx->next_iv;
        else
            task->alg.des_cbc.iv_addr = (u32)(uintptr_t)rctx->iv;
    }
    aicos_dcache_clean_range((void *)(uintptr_t)rctx->key, keysize);
    aicos_dcache_clean_range((void *)(uintptr_t)din, dlen);
    aicos_dcache_clean_range((void *)(uintptr_t)task, sizeof(struct crypto_task));
}

static inline bool is_sk_block_aligned(unsigned int val, unsigned long flg)
{
    if (flg & FLG_AES) {
        if (val % AES_BLOCK_SIZE)
            return false;
        return true;
    }

    if (flg & FLG_DES) {
        if (val % DES_BLOCK_SIZE)
            return false;
        return true;
    }

    return false;
}

static inline bool is_need_copy_dst(struct skcipher_request *req)
{
    struct aic_skcipher_req_ctx *rctx;

    rctx = skcipher_request_ctx(req);
    if (!is_sk_block_aligned((u32)(uintptr_t)req->dst, rctx->flags)) {
        pr_debug("%s, offset(%d) is not aligned.\n", __func__, (u32)(uintptr_t)req->dst);
        return true;
    }

    /*
	 * Even it is ctr mode, if data length is not block size alignment,
	 * still need to use dst copy buffer.
	 */
    if (!is_sk_block_aligned(req->cryptlen, rctx->flags)) {
        pr_debug("%s, cryptlen(%d) is not aligned.\n", __func__, req->cryptlen);
        return true;
    }

    return false;
}

static inline bool is_need_copy_src(struct skcipher_request *req)
{
    struct aic_skcipher_req_ctx *rctx;

    rctx = skcipher_request_ctx(req);
    if (!is_sk_block_aligned((u32)(uintptr_t)req->src, rctx->flags)) {
        pr_debug("%s, offset(%d) is not aligned.\n", __func__, (u32)(uintptr_t)req->src);
        return true;
    }

    return false;
}

/*
 * Try best to avoid data copy
 */
static int aic_skcipher_prepare_data_buf(struct skcipher_request *req)
{
    struct aic_skcipher_req_ctx *rctx;
    bool cpydst, cpysrc;
    int pages;

    rctx = skcipher_request_ctx(req);

    pages = ALIGN_UP(req->cryptlen, rctx->blocksize);

    cpysrc = is_need_copy_src(req);
    if (cpysrc) {
        rctx->src_cpy_buf = (void *)aicos_malloc_align(0, pages, CACHE_LINE_SIZE);
        if (!rctx->src_cpy_buf) {
            pr_err("Failed to allocate pages for src.\n");
            return -ENOMEM;
        }

        memset(rctx->src_cpy_buf, 0, pages);
        memcpy(rctx->src_cpy_buf, req->src, req->cryptlen);
    }

    cpydst = is_need_copy_dst(req);
    if (cpydst) {
        rctx->dst_cpy_buf =
            (void *)aicos_malloc_align(0, pages, CACHE_LINE_SIZE);
        if (!rctx->dst_cpy_buf) {
            pr_err("Failed to allocate pages for dst.\n");
            return -ENOMEM;
        }
    }

    return 0;
}

#if 0
static int backup_iv_for_cbc_decrypt(struct skcipher_request *req)
{
	struct aic_skcipher_req_ctx *rctx;
	int i, iv_cnt, ivbufsiz;
	u8 *ps, *pd;

	rctx = skcipher_request_ctx(req);

	rctx->backup_ivs = NULL;
	rctx->backup_phy_ivs = 0;

	if (rctx->src_cpy_buf && rctx->dst_cpy_buf) {
		/*
		 * Both src sg & dst sg are not hardware friendly, all of them
		 * are copied to physical continuous pages, only need to backup
		 * the last ciphertext block as next iv
		 */
		iv_cnt = 1;
		ivbufsiz = rctx->ivsize * iv_cnt;
		rctx->backup_ivs = aicos_malloc_align(0, ivbufsiz, CACHE_LINE_SIZE);
		if (!rctx->backup_ivs)
			return -ENOMEM;

		rctx->backup_iv_cnt = iv_cnt;
		ps = ((u8 *)rctx->src_cpy_buf) + req->cryptlen - rctx->ivsize;
		pd = rctx->backup_ivs;
		memcpy(pd, ps, rctx->ivsize);
	} else if (rctx->src_cpy_buf) {
		/*
		 * src sg is not hardware friendly, but dst sg is,
		 * backup iv according dst sg
		 */
		iv_cnt = sg_nents_for_len(req->dst, req->cryptlen);
		ivbufsiz = rctx->ivsize * iv_cnt;
		rctx->backup_ivs = aicos_malloc_align(0, ivbufsiz, CACHE_LINE_SIZE);
		if (!rctx->backup_ivs)
			return -ENOMEM;

		rctx->backup_iv_cnt = iv_cnt;
		for (i = 0, sg = req->dst; i < iv_cnt; i++, sg = sg_next(sg)) {
			ps = (u8 *)sg_virt(sg) + sg_dma_len(sg) - rctx->ivsize;
			pd = rctx->backup_ivs + i * rctx->ivsize;
			memcpy(pd, ps, rctx->ivsize);
		}
	} else {
		/*
		 * src sg is hardware friendly,
		 * backup iv according src sg
		 */
		iv_cnt = sg_nents_for_len(req->src, req->cryptlen);
		ivbufsiz = rctx->ivsize * iv_cnt;
		rctx->backup_ivs = aicos_malloc_align(0, ivbufsiz, CACHE_LINE_SIZE);
		if (!rctx->backup_ivs)
			return -ENOMEM;

		rctx->backup_iv_cnt = iv_cnt;
		for (i = 0, sg = req->src; i < iv_cnt; i++, sg = sg_next(sg)) {
			ps = (u8 *)sg_virt(sg) + sg_dma_len(sg) - rctx->ivsize;
			pd = rctx->backup_ivs + i * rctx->ivsize;
			memcpy(pd, ps, rctx->ivsize);
		}
	}

	return 0;
}
#endif

/*
 * Driver try to avoid data copy, but if the input buffer is not friendly
 * to hardware, it is need to allocate physical continuous pages as working
 * buffer.
 *
 * This case is both input and output working buffer are new allocated
 * physical continuous pages.
 */
static int prepare_task_with_both_cpy_buf(struct skcipher_request *req)
{
    unsigned int din, dout;
    struct aic_skcipher_req_ctx *rctx;
    u32 bytelen;

    pr_debug("%s\n", __func__);
    rctx = skcipher_request_ctx(req);
    rctx->tasklen = sizeof(struct crypto_task);
    rctx->task = aicos_malloc_align(0, rctx->tasklen, CACHE_LINE_SIZE);
    if (!rctx->task)
        return -ENOMEM;

    memset(rctx->task, 0, rctx->tasklen);

    rctx->next_iv = 0;
    din = (u32)(uintptr_t)rctx->src_cpy_buf;
    dout = (u32)(uintptr_t)rctx->dst_cpy_buf;
    bytelen = ALIGN_UP(req->cryptlen, rctx->blocksize);
    aic_skcipher_task_cfg(rctx->task, rctx, din, dout, bytelen, 1, 1);

    return 0;
}

/*
 * In this case, dst is not hardware friendly, but src is.
 * ce output data is saved to dst_cpy_buf firstly, then copied to dst.
 */
static int prepare_task_with_dst_cpy_buf(struct skcipher_request *req)
{
    unsigned int din, dout, next_addr;
    struct aic_skcipher_req_ctx *rctx;
    u32 first_flag, last_flag;
    struct crypto_task *task;
    unsigned int bytelen, remain, chunk = CE_CIPHER_MAX_DATA_SIZE;
    int i, task_cnt;

    pr_debug("%s\n", __func__);
    rctx = skcipher_request_ctx(req);

    task_cnt = DIV_ROUND_UP(req->cryptlen, chunk);

    rctx->tasklen = sizeof(struct crypto_task) * task_cnt;
    rctx->task = aicos_malloc_align(0, rctx->tasklen, CACHE_LINE_SIZE);
    if (!rctx->task)
        return -ENOMEM;

    rctx->next_iv = 0;
    remain = req->cryptlen;
    din = (u32)(uintptr_t)req->src;
    dout = (u32)(uintptr_t)rctx->dst_cpy_buf;
    for (i = 0; i < task_cnt; i++) {
        task = &rctx->task[i];
        next_addr = (u32)(uintptr_t)rctx->task + ((i + 1) * sizeof(*task));

        first_flag = (i == 0);
        last_flag = ((i + 1) == task_cnt);
        bytelen = min(remain, chunk);
        bytelen = ALIGN_UP(bytelen, rctx->blocksize);
        aic_skcipher_task_cfg(task, rctx, din, dout, bytelen, first_flag, last_flag);
        if (last_flag)
            task->next = 0;
        else
            task->next = cpu_to_le32(next_addr);

        din += bytelen;
        dout += bytelen;
        if (is_cbc_enc(rctx->flags)) {
            /* Last output as next iv, only CBC mode use it */
            rctx->next_iv = dout + bytelen - rctx->ivsize;
        }
        if (is_cbc_dec(rctx->flags)) {
            /* For decryption, next iv is last ciphertext block */
            rctx->next_iv = (u32)(uintptr_t)(rctx->backup_ivs) + i * rctx->ivsize;
        }
    }

    return 0;
}

/*
 * In this case, src is not hardware friendly, but dst is.
 * src data is copied to src_cpy_buf, but task need to build according dst.
 */
static int prepare_task_with_src_cpy_buf(struct skcipher_request *req)
{
    unsigned int din, dout, next_addr;
    struct aic_skcipher_req_ctx *rctx;
    u32 first_flag, last_flag;
    struct crypto_task *task;
    unsigned int bytelen, remain, chunk = CE_CIPHER_MAX_DATA_SIZE;
    int i, task_cnt;

    pr_debug("%s\n", __func__);
    rctx = skcipher_request_ctx(req);

    task_cnt = DIV_ROUND_UP(req->cryptlen, chunk);

    rctx->tasklen = sizeof(struct crypto_task) * task_cnt;
    rctx->task = aicos_malloc_align(0, rctx->tasklen, CACHE_LINE_SIZE);
    if (!rctx->task)
        return -ENOMEM;

    remain = req->cryptlen;
    din = (u32)(uintptr_t)rctx->src_cpy_buf;
    dout = (u32)(uintptr_t)req->dst;
    for (i = 0; i < task_cnt; i++) {
        task = &rctx->task[i];
        next_addr = (u32)(uintptr_t)rctx->task + ((i + 1) * sizeof(*task));

        first_flag = (i == 0);
        last_flag = ((i + 1) == task_cnt);
        bytelen = min(remain, chunk);
        bytelen = ALIGN_UP(bytelen, rctx->blocksize);
        aic_skcipher_task_cfg(task, rctx, din, dout, bytelen, first_flag, last_flag);
        if (last_flag)
            task->next = 0;
        else
            task->next = cpu_to_le32(next_addr);

        din += bytelen;
        dout += bytelen;
        if (is_cbc_enc(rctx->flags)) {
            /* Last output as next iv, only CBC mode use it */
            rctx->next_iv = dout + bytelen - rctx->ivsize;
        }
        if (is_cbc_dec(rctx->flags)) {
            /* For decryption, next iv is last ciphertext block */
            rctx->next_iv = (u32)(uintptr_t)(rctx->backup_ivs) + i * rctx->ivsize;
        }
    }

    return 0;
}

/*
 * In this case, both src & dst sg list are hardware friendly, and src & dst
 * are match to each other.
 */
static int prepare_task_with_no_cpy_buf(struct skcipher_request *req)
{
    u32 din, dout, next_addr;
    struct aic_skcipher_req_ctx *rctx;
    u32 first_flag, last_flag;
    struct crypto_task *task;
    unsigned int bytelen, remain, chunk = CE_CIPHER_MAX_DATA_SIZE;
    int i, task_cnt;

    pr_debug("%s\n", __func__);
    rctx = skcipher_request_ctx(req);

    task_cnt = DIV_ROUND_UP(req->cryptlen, chunk);

    rctx->tasklen = sizeof(struct crypto_task) * task_cnt;
    rctx->task = aicos_malloc_align(0, rctx->tasklen, CACHE_LINE_SIZE);
    if (!rctx->task)
        return -ENOMEM;

    rctx->next_iv = 0;
    remain = req->cryptlen;
    din = (u32)(uintptr_t)req->src;
    dout = (u32)(uintptr_t)req->dst;
    for (i = 0; i < task_cnt; i++) {
        task = &rctx->task[i];
        next_addr = (u32)(uintptr_t)rctx->task + ((i + 1) * sizeof(*task));

        first_flag = (i == 0);
        last_flag = ((i + 1) == task_cnt);
        bytelen = min(remain, chunk);
        bytelen = ALIGN_UP(bytelen, rctx->blocksize);
        aic_skcipher_task_cfg(task, rctx, din, dout, bytelen, first_flag, last_flag);
        if (last_flag)
            task->next = 0;
        else
            task->next = cpu_to_le32(next_addr);

        din += bytelen;
        dout += bytelen;
        if (is_cbc_enc(rctx->flags)) {
            /* For CBC encryption next iv is last output block */
            rctx->next_iv = dout + bytelen - rctx->ivsize;
        }
        if (is_cbc_dec(rctx->flags)) {
            /* For CBC decryption, next iv is last input block */
            rctx->next_iv = (u32)(uintptr_t)(rctx->backup_ivs) + i * rctx->ivsize;
        }
    }

    return 0;
}

static int prepare_task_to_genhsk(struct skcipher_request *req)
{
    struct aic_skcipher_req_ctx *rctx;
    unsigned char *keymaterial;
    unsigned int ssram_addr;
    int pages;
    u32 bytelen;

    pr_debug("%s\n", __func__);
    rctx = skcipher_request_ctx(req);
    rctx->tasklen = sizeof(struct crypto_task);
    rctx->task = aicos_malloc_align(0, rctx->tasklen, CACHE_LINE_SIZE);
    if (!rctx->task)
        return -ENOMEM;

    memset(rctx->task, 0, rctx->tasklen);

    bytelen = ALIGN_UP(req->cryptlen, rctx->blocksize);
    keymaterial = (unsigned char *)req->src;
    ssram_addr = (unsigned int)(uintptr_t)req->dst;
    pages = ALIGN_UP(req->cryptlen, rctx->blocksize);
    rctx->src_cpy_buf = aicos_malloc_align(0, pages, CACHE_LINE_SIZE);
    if (!rctx->src_cpy_buf) {
        pr_err("Failed to allocate pages for src.\n");
        return -ENOMEM;
    }
    memset(rctx->src_cpy_buf, 0, pages);
    memcpy(rctx->src_cpy_buf, keymaterial, req->cryptlen);

    if (is_aes_ecb(rctx->flags))
        rctx->keylen = AES_BLOCK_SIZE;
    if (is_des_ecb(rctx->flags))
        rctx->keylen = DES_BLOCK_SIZE;
    aic_skcipher_task_cfg(rctx->task, rctx, (u32)(uintptr_t)rctx->src_cpy_buf,
                          ssram_addr, bytelen, 1, 1);
    return 0;
}

static int aic_skcipher_unprepare_req(struct skcipher_tfm *tfm,
                                      struct skcipher_request *req)
{
    struct aic_skcipher_req_ctx *rctx;

    pr_debug("%s\n", __func__);
    rctx = skcipher_request_ctx(req);

    if (rctx->task) {
        aicos_free_align(0, rctx->task);
        rctx->task = NULL;
        rctx->tasklen = 0;
    }

    if (rctx->dst_cpy_buf) {
        memset(req->dst, 0, req->cryptlen);
        memcpy(req->dst, rctx->dst_cpy_buf, req->cryptlen);

        aicos_free_align(0, rctx->dst_cpy_buf);
        rctx->dst_cpy_buf = NULL;
    }
    if (rctx->src_cpy_buf) {
        aicos_free_align(0, rctx->src_cpy_buf);
        rctx->src_cpy_buf = NULL;
    }

    if (rctx->ssram_addr) {
        unsigned int hsklen;

        hsklen = ALIGN_UP(rctx->keylen, rctx->blocksize);
        aic_ssram_free(rctx->ssram_addr, hsklen);
        rctx->ssram_addr = 0;
    }

    if (rctx->backup_ivs) {
        aicos_free_align(0, rctx->backup_ivs);
        rctx->backup_ivs = NULL;
    }

    if (rctx->iv) {
        aicos_free_align(0, rctx->iv);
        rctx->iv = NULL;
        rctx->ivsize = 0;
    }

    if (rctx->key) {
        aicos_free_align(0, rctx->key);
        rctx->key = NULL;
        rctx->keylen = 0;
    }

    return 0;
}

static int aic_skcipher_prepare_req(struct skcipher_tfm *tfm,
                                    struct skcipher_request *req)
{
    struct aic_skcipher_tfm_ctx *ctx;
    struct aic_skcipher_req_ctx *rctx;
    unsigned int ivsize;
    int ret;

    pr_debug("%s\n", __func__);
    ctx = skcipher_tfm_ctx(tfm);
    rctx = skcipher_request_ctx(req);

    rctx->blocksize = skcipher_tfm_blocksize(tfm);
    if (is_gen_hsk(rctx->flags)) {
        /* Prepare request for key decryption to SSRAM */
        ret = prepare_task_to_genhsk(req);
        if (ret) {
            pr_err("Failed to prepare task\n");
            goto err;
        }
        return 0;
    }

    /* Prepare request for data encrypt/decrypt */
    if (ctx->inkey) {
        /* Copy key for hardware */
        rctx->key = aicos_memdup_align(0, (void *)ctx->inkey, ctx->inkeylen,
                                       CACHE_LINE_SIZE);
        if (!rctx->key) {
            ret = -ENOMEM;
            pr_err("Failed to prepare key\n");
            goto err;
        }
        rctx->keylen = ctx->inkeylen;
    }

    ivsize = skcipher_tfm_ivsize(tfm);
    if (req->iv && ivsize > 0) {
        /* Copy IV for hardware */
        rctx->iv = aicos_memdup_align(0, req->iv, ivsize, CACHE_LINE_SIZE);
        if (!rctx->iv) {
            ret = -ENOMEM;
            pr_err("Failed to prepare iv\n");
            goto err;
        }
        rctx->ivsize = ivsize;
    }

    ret = aic_skcipher_prepare_data_buf(req);
    if (ret) {
        pr_err("Failed to prepare data buf\n");
        goto err;
    }

    if (is_cbc_dec(rctx->flags)) {
        // ret = backup_iv_for_cbc_decrypt(req); TODO
        if (ret) {
            pr_err("Failed to backup iv\n");
            goto err;
        }
    }

    if (rctx->src_cpy_buf && rctx->dst_cpy_buf)
        ret = prepare_task_with_both_cpy_buf(req);
    else if (rctx->src_cpy_buf)
        ret = prepare_task_with_src_cpy_buf(req);
    else if (rctx->dst_cpy_buf)
        ret = prepare_task_with_dst_cpy_buf(req);
    else
        ret = prepare_task_with_no_cpy_buf(req);

    if (ret) {
        pr_err("Failed to prepare task\n");
        goto err;
    }

    return 0;

err:
    aic_skcipher_unprepare_req(tfm, req);
    return ret;
}

static int aic_skcipher_do_one_req(struct skcipher_request *req)
{
    struct aic_skcipher_req_ctx *rctx;

    pr_debug("%s\n", __func__);
    rctx = skcipher_request_ctx(req);

    if (!hal_crypto_is_start()) {
        pr_err("Crypto engine is busy.\n");
        return -EBUSY;
    }

    if (DEBUG_CE)
        hal_crypto_dump_task(rctx->task, rctx->tasklen);

    hal_crypto_init();
    hal_crypto_irq_enable(ALG_SK_ACCELERATOR);
    hal_crypto_start_symm(rctx->task);
    while (!hal_crypto_poll_finish(ALG_SK_ACCELERATOR)) {
        continue;
    }
    hal_crypto_pending_clear(ALG_SK_ACCELERATOR);

    if (hal_crypto_get_err(ALG_SK_ACCELERATOR)) {
        pr_err("SK ACCELERATOR run error.\n");
        return -1;
    }

    aicos_dma_sync();
    if (rctx->dst_cpy_buf) {
        aicos_dcache_invalid_range((void *)(uintptr_t)rctx->dst_cpy_buf,
                                   req->cryptlen);
        memcpy(req->dst, rctx->dst_cpy_buf, req->cryptlen);
    } else {
        aicos_dcache_invalid_range((void *)(uintptr_t)req->dst, req->cryptlen);
    }

    if (DEBUG_CE)
        hal_crypto_dump_reg();

    hal_crypto_deinit();

    return 0;
}

static int aic_skcipher_alg_setkey(struct aic_skcipher_handle *handle,
                                   const u8 *key, unsigned int keylen)
{
    struct skcipher_tfm *tfm;
    struct aic_skcipher_tfm_ctx *ctx;

    pr_debug("%s\n", __func__);
    tfm = skcipher_handle_tfm(handle);
    ctx = skcipher_tfm_ctx(tfm);

    if (ctx->inkey) {
        aicos_free_align(0, ctx->inkey);
        ctx->inkey = NULL;
    }
    ctx->inkeylen = 0;

    ctx->inkey = aicos_memdup_align(0, (void *)key, keylen, CACHE_LINE_SIZE);
    if (!ctx->inkey)
        return -ENOMEM;
    ctx->inkeylen = keylen;
    return 0;
}

static int aic_skcipher_crypt(struct aic_skcipher_handle *handle,
                              unsigned long flg)
{
    struct aic_skcipher_tfm_ctx *ctx;
    struct aic_skcipher_req_ctx *rctx;
    struct skcipher_tfm *tfm;
    struct skcipher_request *req;
    int ret = 0;

    pr_debug("%s\n", __func__);
    tfm = skcipher_handle_tfm(handle);
    req = skcipher_handle_req(handle);
    ctx = skcipher_tfm_ctx(tfm);
    rctx = skcipher_request_ctx(req);

    if (!ctx) {
        pr_err("aic skcipher, device is null\n");
        return -ENODEV;
    }

    memset(rctx, 0, sizeof(struct aic_skcipher_req_ctx));

    /* Mark the request, and transfer it to crypto hardware engine */
    rctx->flags = flg;

    if (is_need_genhsk(rctx->flags)) {
        unsigned long efuse;
        unsigned int bs, hsklen;
        int ret;

        efuse = (rctx->flags & FLG_KEYMASK) >> FLG_KEYOFFSET;
        rctx->keylen = ctx->inkeylen;
        bs = skcipher_tfm_blocksize(tfm);
        hsklen = ALIGN_UP(rctx->keylen, bs);
        rctx->ssram_addr = aic_ssram_alloc(hsklen);
        if (!rctx->ssram_addr) {
            pr_err("Failed to allocate ssram key\n");
            return -ENOMEM;
        }

        ret = aic_ssram_aes_genkey(efuse, ctx->inkey, ctx->inkeylen,
                                   rctx->ssram_addr);
        if (ret) {
            aic_ssram_free(rctx->ssram_addr, hsklen);
            rctx->ssram_addr = 0;
            return ret;
        }
    }

    ret = aic_skcipher_prepare_req(tfm, req);
    if (ret) {
        pr_err("skcipher prepare req failed.\n");
        return ret;
    }
    ret = aic_skcipher_do_one_req(req);
    if (ret) {
        pr_err("skcipher do one req failed.\n");
        return ret;
    }
    ret = aic_skcipher_unprepare_req(tfm, req);
    if (ret) {
        pr_err("skcipher unprepare req failed.\n");
        return ret;
    }

    return ret;
}

/*
 * Use eFuse Key to decrypt key material and output to Secure SRAM
 */
static int aic_skcipher_gen_hsk(char *alg_name, unsigned long flg,
                                unsigned char *key_material, unsigned int mlen,
                                unsigned int ssram_addr)
{
    struct aic_skcipher_handle *handle;
    struct skcipher_tfm *tfm;
    struct skcipher_request *req;
    struct aic_skcipher_req_ctx *rctx;
    int ret = 0;

    pr_debug("%s\n", __func__);
    handle = aic_skcipher_init(alg_name, 0);
    if (handle == NULL) {
        pr_err("skcipher init failed.\n");
        goto out;
    }

    tfm = skcipher_handle_tfm(handle);
    req = skcipher_handle_req(handle);
    rctx = skcipher_request_ctx(req);

    rctx->flags = flg;
    /* Special setup for request because here using private api instead of
	 * crypto_skcipher_decrypt to decrypt key material to ssram
	 */
    req->cryptlen = mlen;
    req->src = key_material;
    req->dst = (void *)(uintptr_t)ssram_addr;

    ret = aic_skcipher_prepare_req(tfm, req);
    if (ret) {
        pr_err("skcipher prepare req failed.\n");
        goto out;
    }
    ret = aic_skcipher_do_one_req(req);
    if (ret) {
        pr_err("skcipher do one req failed.\n");
        goto out;
    }
    ret = aic_skcipher_unprepare_req(tfm, req);
    if (ret) {
        pr_err("skcipher unprepare req failed.\n");
        goto out;
    }

out:
    aic_skcipher_destroy(handle);
    return ret;
}

int aic_ssram_des_genkey(unsigned long efuse_key, unsigned char *key_material,
                         unsigned int mlen, unsigned int ssram_addr)
{
    unsigned long flags, kflag;

    kflag = (efuse_key << FLG_KEYOFFSET) & FLG_KEYMASK;
    flags = FLG_DES | FLG_ECB | FLG_DEC | FLG_GENHSK | kflag;

    pr_debug("%s\n", __func__);
    return aic_skcipher_gen_hsk("ecb-des-aic", flags, key_material, mlen,
                                ssram_addr);
}

static int aic_skcipher_aes_ecb_encrypt(struct aic_skcipher_handle *handle)
{
    return aic_skcipher_crypt(handle, FLG_AES | FLG_ECB);
}

static int aic_skcipher_aes_ecb_decrypt(struct aic_skcipher_handle *handle)
{
    return aic_skcipher_crypt(handle, FLG_AES | FLG_ECB | FLG_DEC);
}

static int aic_skcipher_aes_cbc_encrypt(struct aic_skcipher_handle *handle)
{
    return aic_skcipher_crypt(handle, FLG_AES | FLG_CBC);
}

static int aic_skcipher_aes_cbc_decrypt(struct aic_skcipher_handle *handle)
{
    return aic_skcipher_crypt(handle, FLG_AES | FLG_CBC | FLG_DEC);
}

static int aic_skcipher_des_ecb_encrypt(struct aic_skcipher_handle *handle)
{
    return aic_skcipher_crypt(handle, FLG_DES | FLG_ECB);
}

static int aic_skcipher_des_ecb_decrypt(struct aic_skcipher_handle *handle)
{
    return aic_skcipher_crypt(handle, FLG_DES | FLG_ECB | FLG_DEC);
}

static int aic_skcipher_des_cbc_encrypt(struct aic_skcipher_handle *handle)
{
    return aic_skcipher_crypt(handle, FLG_DES | FLG_CBC);
}

static int aic_skcipher_des_cbc_decrypt(struct aic_skcipher_handle *handle)
{
    return aic_skcipher_crypt(handle, FLG_DES | FLG_CBC | FLG_DEC);
}

static struct aic_skcipher_alg sk_algs[] = {
	{
		.alg = {
			.base.cra_name = "ecb(aes)",
			.base.cra_driver_name = "ecb-aes-aic",
			.base.cra_blocksize = AES_BLOCK_SIZE,
			.base.cra_ctxsize = sizeof(struct aic_skcipher_tfm_ctx),
			.base.cra_alignmask = 0,
			.setkey = aic_skcipher_alg_setkey,
			.decrypt = aic_skcipher_aes_ecb_decrypt,
			.encrypt = aic_skcipher_aes_ecb_encrypt,
			.min_keysize = AES_MIN_KEY_SIZE,
			.max_keysize = AES_MAX_KEY_SIZE,
			.ivsize = 0,
		},
	},
	{
		.alg = {
			.base.cra_name = "cbc(aes)",
			.base.cra_driver_name = "cbc-aes-aic",
			.base.cra_blocksize = AES_BLOCK_SIZE,
			.base.cra_ctxsize = sizeof(struct aic_skcipher_tfm_ctx),
			.base.cra_alignmask = 0,
			.setkey = aic_skcipher_alg_setkey,
			.decrypt = aic_skcipher_aes_cbc_decrypt,
			.encrypt = aic_skcipher_aes_cbc_encrypt,
			.min_keysize = AES_MIN_KEY_SIZE,
			.max_keysize = AES_MAX_KEY_SIZE,
			.ivsize = AES_BLOCK_SIZE,
		},
	},
	{
		.alg = {
			.base.cra_name = "ecb(des)",
			.base.cra_driver_name = "ecb-des-aic",
			.base.cra_blocksize = DES_BLOCK_SIZE,
			.base.cra_ctxsize = sizeof(struct aic_skcipher_tfm_ctx),
			.base.cra_alignmask = 0,
			.setkey = aic_skcipher_alg_setkey,
			.decrypt = aic_skcipher_des_ecb_decrypt,
			.encrypt = aic_skcipher_des_ecb_encrypt,
			.min_keysize = DES_KEY_SIZE,
			.max_keysize = DES_KEY_SIZE,
		},
	},
	{
		.alg = {
			.base.cra_name = "cbc(des)",
			.base.cra_driver_name = "cbc-des-aic",
			.base.cra_blocksize = DES_BLOCK_SIZE,
			.base.cra_ctxsize = sizeof(struct aic_skcipher_tfm_ctx),
			.base.cra_alignmask = 0,
			.setkey = aic_skcipher_alg_setkey,
			.decrypt = aic_skcipher_des_cbc_decrypt,
			.encrypt = aic_skcipher_des_cbc_encrypt,
			.min_keysize = DES_KEY_SIZE,
			.max_keysize = DES_KEY_SIZE,
			.ivsize = DES_BLOCK_SIZE,
		},
	},
};

void aic_skcipher_destroy(struct aic_skcipher_handle *handle)
{
    if (handle->req->__ctx) {
        aicos_free(0, handle->req->__ctx);
        handle->req->__ctx = NULL;
    }

    if (handle->req) {
        aicos_free(0, handle->req);
        handle->req = NULL;
    }

    if (handle->tfm->__crt_ctx) {
        aicos_free(0, handle->tfm->__crt_ctx);
        handle->tfm->__crt_ctx = NULL;
    }

    if (handle->tfm) {
        aicos_free(0, handle->tfm);
        handle->tfm = NULL;
    }

    if (handle) {
        aicos_free(0, handle);
        handle = NULL;
    }
}

struct aic_skcipher_handle *aic_skcipher_init(const char *ciphername, u32 flags)
{
    struct aic_skcipher_handle *handle = NULL;
    struct aic_skcipher_tfm_ctx *ctx = NULL;
    struct aic_skcipher_req_ctx *rctx = NULL;
    struct skcipher_request *req = NULL;
    struct skcipher_tfm *tfm = NULL;

    handle = aicos_malloc(0, sizeof(struct aic_skcipher_handle));
    if (handle == NULL) {
        pr_err("malloc skcipher handle failed.\n");
        return NULL;
    }
    memset(handle, 0, sizeof(struct aic_skcipher_handle));

    tfm = aicos_malloc(0, sizeof(struct skcipher_tfm));
    if (tfm == NULL) {
        pr_err("malloc skcipher tfm failed.\n");
        goto out;
    }
    memset(tfm, 0, sizeof(struct skcipher_tfm));
    handle->tfm = tfm;

    ctx = aicos_malloc(0, sizeof(struct aic_skcipher_tfm_ctx));
    if (ctx == NULL) {
        pr_err("malloc skcipher ctx failed.\n");
        goto out;
    }
    memset(ctx, 0, sizeof(struct aic_skcipher_tfm_ctx));
    tfm->__crt_ctx = (void *)ctx;

    req = aicos_malloc(0, sizeof(struct skcipher_request));
    if (req == NULL) {
        pr_err("malloc skcipher req failed.\n");
        goto out;
    }
    memset(req, 0, sizeof(struct skcipher_request));
    handle->req = req;

    rctx = aicos_malloc(0, sizeof(struct aic_skcipher_req_ctx));
    if (rctx == NULL) {
        pr_err("malloc skcipher rctx failed.\n");
        goto out;
    }
    memset(rctx, 0, sizeof(struct aic_skcipher_req_ctx));
    req->__ctx = (void *)rctx;

    for (int i = 0; i < ARRAY_SIZE(sk_algs); i++) {
        if (!strcmp(sk_algs[i].alg.base.cra_name, ciphername) ||
            !strcmp(sk_algs[i].alg.base.cra_driver_name, ciphername)) {
            tfm->alg = &sk_algs[i].alg;
            return handle;
        }
    }
    pr_warn("not found %s algo\n", ciphername);

out:
    aic_skcipher_destroy(handle);

    return NULL;
}
