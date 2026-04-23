/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#include <string.h>
#include <aic_core.h>
#include <aic_common.h>
#include <aic_utils.h>
#include <hal_ce.h>
#include <ssram.h>
#include "hash.h"
#include "akcipher.h"
#include "sm2.h"

#define PUTU32(p,V) \
	((p)[0] = (uint8_t)((V) >> 24), \
	 (p)[1] = (uint8_t)((V) >> 16), \
	 (p)[2] = (uint8_t)((V) >>  8), \
	 (p)[3] = (uint8_t)(V))

/* The default user id as specified in GM/T 0009-2012 */
#define SM2_DEFAULT_USERID "1234567812345678"
#define SM2_DEFAULT_USERID_LEN 16

static __attribute__((aligned(64)))u32 sm2p256v1_p[8] = {
	0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFE};

static __attribute__((aligned(64)))u32 sm2p256v1_a[8] = {
	0xFFFFFFFC, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFE};

static __attribute__((aligned(64)))u32 sm2p256v1_b[8] = {
	0x4D940E93, 0xDDBCBD41, 0x15AB8F92, 0xF39789F5,
	0xCF6509A7, 0x4D5A9E4B, 0x9D9F5E34, 0x28E9FA9E};

static __attribute__((aligned(64)))u32 sm2p256v1_g[2][8] = {
	{0x334C74C7, 0x715A4589, 0xF2660BE1, 0x8FE30BBF,
	 0x6A39C994, 0x5F990446, 0x1F198119, 0x32C4AE2C},
	{0x2139F0A0, 0x02DF32E5, 0xC62A4740, 0xD0A9877C,
	0x6B692153, 0x59BDCEE3, 0xF4F6779C, 0xBC3736A2}
};

static __attribute__((aligned(64)))u32 sm2p256v1_n[8] = {
	0x39D54123, 0x53BBF409, 0x21C6052B, 0x7203DF6B,
	0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFE};

static __attribute__((aligned(64)))u32 sm2p256v1_h[8] = {
	0x00000001, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000};

static __attribute__((aligned(64)))u32 sm2p256v1_k[8] = {
	0x276E2759, 0x1A8606D5, 0x3A0F6816, 0xCC2DC0D9,
	0xFAC13CEF, 0xCEE4DB3C, 0x0DB8546D, 0x21BCC1EA};

static int aic_sm2_sign_task_cfg(struct aic_akcipher_req_ctx *rctx,
                                 u8 *din, u8 *dout,
                                 u8 *sm2_rand_k, u8 *sm2_priv_bin)
{
	pr_debug("%s\n", __func__);
	rctx->tasklen = sizeof(struct crypto_task);
	rctx->task = aicos_malloc_align(0, rctx->tasklen, CACHE_LINE_SIZE);
	if (!rctx->task) {
        pr_err("Failed to allocate task(%d).\n", rctx->tasklen);
		return -ENOMEM;
    }

	memset(rctx->task, 0, rctx->tasklen);

	rctx->task->alg.alg_tag = ALG_SM2_SIGN;
	rctx->task->alg.sm2_sign.op_siz = KEY_SIZE_256;
	rctx->task->alg.sm2_sign.p_addr = PTR2U32(sm2p256v1_p);
	rctx->task->alg.sm2_sign.G_addr = PTR2U32(sm2p256v1_g);
	rctx->task->alg.sm2_sign.a_addr = PTR2U32(sm2p256v1_a);
	rctx->task->alg.sm2_sign.k_addr = PTR2U32(sm2_rand_k);
	rctx->task->alg.sm2_sign.n_addr = PTR2U32(sm2p256v1_n);
	rctx->task->alg.sm2_sign.d_addr = PTR2U32(sm2_priv_bin);
	rctx->task->data.in_addr = PTR2U32(din);
	rctx->task->data.out_addr = PTR2U32(dout);
	rctx->task->data.out_len = 0x40;

	rctx->task->next = 0;

	return 0;
}

static int aic_sm2_veri_task_cfg(struct aic_akcipher_req_ctx *rctx,
				                 u8 *din, u8 *dout,
				                 u8 *sm2_sig, u8 *sm2_pub_bin)
{
	pr_debug("%s\n", __func__);
	rctx->tasklen = sizeof(struct crypto_task);
	rctx->task = aicos_malloc_align(0, rctx->tasklen, CACHE_LINE_SIZE);
	if (!rctx->task) {
        pr_err("Failed to allocate task(%d).\n", rctx->tasklen);
		return -ENOMEM;
    }

	memset(rctx->task, 0, rctx->tasklen);

	rctx->task->alg.alg_tag = ALG_SM2_VERI;
	rctx->task->alg.sm2_veri.op_siz = KEY_SIZE_256;
	rctx->task->alg.sm2_veri.p_addr = PTR2U32(sm2p256v1_p);
	rctx->task->alg.sm2_veri.G_addr = PTR2U32(sm2p256v1_g);
	rctx->task->alg.sm2_veri.a_addr = PTR2U32(sm2p256v1_a);
	rctx->task->alg.sm2_veri.Q_addr = PTR2U32(sm2_pub_bin);
	rctx->task->alg.sm2_veri.r_addr = PTR2U32(sm2_sig);
	rctx->task->alg.sm2_veri.s_addr = PTR2U32(sm2_sig) + 32;
	rctx->task->alg.sm2_veri.n_addr = PTR2U32(sm2p256v1_n);
	rctx->task->data.in_addr = PTR2U32(din);
	rctx->task->data.out_addr = PTR2U32(dout);
	rctx->task->data.out_len = 0x20;

	rctx->task->next = 0;

	return 0;
}

static int aic_sm2_enc_task_cfg(struct aic_akcipher_req_ctx *rctx,
				                u8 *din, u8 *dout,
				                u8 *sm2_pub_bin)
{
	pr_debug("%s\n", __func__);
	rctx->tasklen = sizeof(struct crypto_task);
	rctx->task = aicos_malloc_align(0, rctx->tasklen, CACHE_LINE_SIZE);
	if (!rctx->task) {
        pr_err("Failed to allocate task(%d).\n", rctx->tasklen);
		return -ENOMEM;
    }

	memset(rctx->task, 0, rctx->tasklen);

	rctx->task->alg.alg_tag = ALG_SM2_ENC;
	rctx->task->alg.sm2_enc.op_siz = KEY_SIZE_256;
	rctx->task->alg.sm2_enc.p_addr = PTR2U32(sm2p256v1_p);
	rctx->task->alg.sm2_enc.G_addr = PTR2U32(sm2p256v1_g);
	rctx->task->alg.sm2_enc.a_addr = PTR2U32(sm2p256v1_a);
	rctx->task->alg.sm2_enc.Q_addr = PTR2U32(sm2_pub_bin);
	rctx->task->alg.sm2_enc.k_addr = PTR2U32(din);
	rctx->task->alg.sm2_enc.h_addr = PTR2U32(sm2p256v1_h);
	rctx->task->data.out_addr = PTR2U32(dout);
	rctx->task->data.out_len = 0x80;

	rctx->task->next = 0;

	return 0;
}

static int aic_sm2_dec_task_cfg(struct aic_akcipher_req_ctx *rctx,
				                u8 *din, u8 *dout,
				                u8 *sm2_priv_bin)
{
	pr_debug("%s\n", __func__);
	rctx->tasklen = sizeof(struct crypto_task);
	rctx->task = aicos_malloc_align(0, rctx->tasklen, CACHE_LINE_SIZE);
	if (!rctx->task) {
        pr_err("Failed to allocate task(%d).\n", rctx->tasklen);
		return -ENOMEM;
    }

	memset(rctx->task, 0, rctx->tasklen);

	rctx->task->alg.alg_tag = ALG_SM2_DEC;
	rctx->task->alg.sm2_dec.op_siz = KEY_SIZE_256;
	rctx->task->alg.sm2_dec.p_addr = PTR2U32(sm2p256v1_p);
	rctx->task->alg.sm2_dec.R_addr = PTR2U32(din);
	rctx->task->alg.sm2_dec.a_addr = PTR2U32(sm2p256v1_a);
	rctx->task->alg.sm2_dec.h_addr = PTR2U32(sm2p256v1_h);
	rctx->task->alg.sm2_dec.d_addr = PTR2U32(sm2_priv_bin);
	rctx->task->data.out_addr = PTR2U32(dout);
	rctx->task->data.out_len = 0x40;

	rctx->task->next = 0;

	return 0;
}

static int aic_hash(char *cipher_name, u8 *data, u32 dlen, u8 *digest)
{
    struct aic_hash_handle *handle = NULL;
    u32 olen = CE_MAX_DIGEST_SIZE;
    int ret = -1;

    handle = aic_md_init(cipher_name, 0);
    if (handle == NULL) {
        pr_err("Allocation of %s cipher failed\n", cipher_name);
        ret = -ENOMEM;
        goto out;
    }

    ret = aic_md_update(handle, data, dlen);
    if (ret) {
        pr_err("Update failed\n");
        goto out;
    }

    ret = aic_md_final(handle, digest, olen);
    if (ret <= 0) {
        pr_err("Get digest failed\n");
        goto out;
    }
    ret = 0;

out:
    if (handle)
        aic_md_destroy(handle);

    return ret;
}

static int aic_sm2_get_z(u8 *id, u32 idlen, u8 *qx, u8 *qy, u8 *z)
{
	u8 idbits[2];
	u8 a[32], b[32], gx[32], gy[32];
	u8 wbuf[2 + idlen + 192];

	idbits[0] = (u8)idlen >> 5;
	idbits[1] = (u8)idlen << 3;

	hal_crypto_bignum_be2le((u8 *)sm2p256v1_a, 32, a, 32);
	hal_crypto_bignum_be2le((u8 *)sm2p256v1_b, 32, b, 32);
	hal_crypto_bignum_be2le((u8 *)sm2p256v1_g, 32, gx, 32);
	hal_crypto_bignum_be2le((u8 *)sm2p256v1_g + 32, 32, gy, 32);

	memcpy(wbuf, idbits, 2);
	memcpy(&wbuf[2], id, idlen);
	memcpy(&wbuf[2 + idlen], a, 32);
	memcpy(&wbuf[2 + idlen + 32], b, 32);
	memcpy(&wbuf[2 + idlen + 64], gx, 32);
	memcpy(&wbuf[2 + idlen + 96], gy, 32);
	memcpy(&wbuf[2 + idlen + 128], qx, 32);
	memcpy(&wbuf[2 + idlen + 160], qy, 32);

	//hexdump_msg("wbuf: ", wbuf, 2 + idlen + 192, 1);

	return aic_hash("sm3", wbuf, 2 + idlen + 192, z);
}

static int compute_sm2_hash(u8 *id, u32 idlen,
			                u8 *data, u32 dlen,
			                u8 *qx, u8 *qy,
			                u8 *out)
{
	u8 z[32], e[32], zm[32 + dlen];
	int ret;

	ret = aic_sm2_get_z(id, idlen, qx, qy, z);
	if (ret) {
		pr_err("SM2 getZ failed: %d\n", ret);
		return ret;
	}

	hexdump_msg("z: ", z, 32, 1);

	// e = Hash(Z || M)
	memcpy(zm, z, 32);
	memcpy(&zm[32], data, dlen);

	hexdump_msg("zm: ", zm, 32 + dlen, 1);

	ret = aic_hash("sm3", zm, 32 + dlen, e);
	if (ret) {
		pr_err("SM3 hash failed: %d\n", ret);
		return ret;
	}

	hexdump_msg("e: ", e, 32, 1);

	memcpy(out, e, 32);
	return 0;
}

static void aic_akcipher_sm2_clear_key(struct aic_akcipher_tfm_ctx *ctx)
{
    if (ctx->alg_ctx.sm2.x)
        aicos_free_align(0, ctx->alg_ctx.sm2.x);
    if (ctx->alg_ctx.sm2.y)
        aicos_free_align(0, ctx->alg_ctx.sm2.y);
    if (ctx->alg_ctx.sm2.d)
        aicos_free_align(0, ctx->alg_ctx.sm2.d);
    ctx->alg_ctx.sm2.x = NULL;
    ctx->alg_ctx.sm2.y = NULL;
    ctx->alg_ctx.sm2.d = NULL;
}

static int aic_akcipher_sm2_unprepare_req(struct akcipher_tfm *tfm,
                                          struct akcipher_request *req)
{
    struct aic_akcipher_tfm_ctx *ctx;
    struct aic_akcipher_req_ctx *rctx;

    pr_debug("%s\n", __func__);
    ctx = akcipher_tfm_ctx(tfm);
    rctx = akcipher_request_ctx(req);

    if (rctx->task) {
        aicos_free_align(0, rctx->task);
        rctx->task = NULL;
        rctx->tasklen = 0;
    }
    if (rctx->wbuf) {
        aicos_free_align(0, rctx->wbuf);
        rctx->wbuf = NULL;
        rctx->wbuf_size = 0;
    }
    if (rctx->ssram_addr) {
        aic_ssram_free(rctx->ssram_addr, ctx->alg_ctx.sm2.x_sz * 3);
        rctx->ssram_addr = 0;
    }

    aic_akcipher_sm2_clear_key(ctx);

    return 0;
}

#define SM2_KEY_SIZE 32
#define SM2_OPERAND_COUNT 14 // d, x, y, r, s, k, data, output
static int aic_akcipher_sm2_prepare_req(struct akcipher_tfm *tfm,
                                        struct akcipher_request *req)
{
    struct aic_akcipher_tfm_ctx *ctx;
    struct aic_akcipher_req_ctx *rctx;
	unsigned char *d, *x, *y, *r, *s, *k, *data, *output;
    int ret;

    pr_debug("%s\n", __func__);
    ctx = akcipher_tfm_ctx(tfm);
    rctx = akcipher_request_ctx(req);

	rctx->wbuf_size = SM2_KEY_SIZE * SM2_OPERAND_COUNT;
	rctx->wbuf = aicos_malloc_align(0, rctx->wbuf_size, CACHE_LINE_SIZE);
	if (!rctx->wbuf) {
		pr_err("Failed to alloc work buffer.\n");
		return -ENOMEM;
	}

	/* copy d, x, y, data to working buffer */
	d = rctx->wbuf + (0 * SM2_KEY_SIZE);
	x = rctx->wbuf + (1 * SM2_KEY_SIZE);
	y = rctx->wbuf + (2 * SM2_KEY_SIZE);
	r = rctx->wbuf + (3 * SM2_KEY_SIZE);
	s = rctx->wbuf + (4 * SM2_KEY_SIZE);
	k = rctx->wbuf + (5 * SM2_KEY_SIZE);
	data = rctx->wbuf + (6 * SM2_KEY_SIZE);
	output = rctx->wbuf + (10 * SM2_KEY_SIZE);

	if (rctx->flags & FLG_SIGN) {
		u8 e[32];

		memcpy(data, req->src, req->src_len);

		ret = compute_sm2_hash((u8 *)SM2_DEFAULT_USERID,
				               SM2_DEFAULT_USERID_LEN,
				               data, req->src_len,
				               ctx->alg_ctx.sm2.x,
				               ctx->alg_ctx.sm2.y,
				               e);
		if (ret) {
			pr_err("SM2 hash failed\n");
			return ret;
		}

		memcpy(data, e, 32);
		hal_crypto_bignum_be2le((u8 *)sm2p256v1_k, 32, k, 32);
	} else if (rctx->flags & FLG_VERI) {
		u8 e[32];

		pr_info("req->src_len:0x%x\n", req->src_len);
		memcpy(data, req->src, req->src_len);
		memcpy(r, data, 64);
		memcpy(data, data + 64, req->src_len - 64);

		ret = compute_sm2_hash((u8 *)SM2_DEFAULT_USERID,
				               SM2_DEFAULT_USERID_LEN,
				               data, req->src_len - 64,
				               ctx->alg_ctx.sm2.x,
				               ctx->alg_ctx.sm2.y,
				               e);
		if (ret) {
			pr_err("SM2 hash failed\n");
			return ret;
		}

		memcpy(data, e, 32);
		hal_crypto_bignum_be2le((u8 *)sm2p256v1_k, 32, k, 32);
		hal_crypto_bignum_byteswap(r, 32);
		hal_crypto_bignum_byteswap(s, 32);
	}

	hal_crypto_bignum_byteswap(data, 32);

	if (ctx->alg_ctx.sm2.d)
		hal_crypto_bignum_be2le(ctx->alg_ctx.sm2.d,
					            ctx->alg_ctx.sm2.d_sz,
					            d, SM2_KEY_SIZE);

	if (ctx->alg_ctx.sm2.x)
		hal_crypto_bignum_be2le(ctx->alg_ctx.sm2.x,
					            ctx->alg_ctx.sm2.x_sz,
					            x, SM2_KEY_SIZE);

	if (ctx->alg_ctx.sm2.y)
		hal_crypto_bignum_be2le(ctx->alg_ctx.sm2.y,
					            ctx->alg_ctx.sm2.y_sz,
					            y, SM2_KEY_SIZE);

	if (DEBUG_CE)
		hexdump_msg("wbuf: ", rctx->wbuf, rctx->wbuf_size, 1);

	if (rctx->flags & FLG_SIGN) {
		ret = aic_sm2_sign_task_cfg(rctx, data, output, k, d);
	} else if (rctx->flags & FLG_VERI) {
		ret = aic_sm2_veri_task_cfg(rctx, data, output, r, x);
	} else if (rctx->flags & FLG_ENC) {
		ret = aic_sm2_enc_task_cfg(rctx, data, output, x);
	} else if (rctx->flags & FLG_DEC) {
		ret = aic_sm2_dec_task_cfg(rctx, data, output, d);
	}

    if (ret) {
        pr_err("Failed to cfg task\n");
        goto err;
    }

    return 0;

err:
    aic_akcipher_sm2_unprepare_req(tfm, req);
    return ret;
}

static int aic_akcipher_sm2_do_one_req(struct akcipher_request *req)
{
    struct aic_akcipher_req_ctx *rctx;
    unsigned char *outbuf;
    u32 timeout = 300 * 1000;

    pr_debug("%s\n", __func__);
    rctx = akcipher_request_ctx(req);

    if (!hal_crypto_is_start()) {
        pr_err("Crypto engine is busy.\n");
        return -EBUSY;
    }

    if (DEBUG_CE)
        hal_crypto_dump_task(rctx->task, rctx->tasklen);

    hal_crypto_init();
    hal_crypto_irq_enable(ALG_AK_ACCELERATOR);
    hal_crypto_start_asym(rctx->task);
    if (hal_crypto_poll_finish(ALG_AK_ACCELERATOR, timeout)) {
        pr_err("AK ACCELERATOR run timeout.\n");
        return -ETIMEDOUT;
    }
    hal_crypto_pending_clear(ALG_AK_ACCELERATOR);

    if (hal_crypto_get_err(ALG_AK_ACCELERATOR)) {
        pr_err("AK ACCELERATOR run error.\n");
        return -1;
    }

    aicos_dma_sync();

	outbuf = rctx->wbuf + (10 * SM2_KEY_SIZE);
    aicos_dcache_invalid_range((void *)(uintptr_t)outbuf, req->dst_len);
	if (rctx->flags & FLG_SIGN) {
		hal_crypto_bignum_byteswap(outbuf, SM2_KEY_SIZE);
		hal_crypto_bignum_byteswap(outbuf + 32, SM2_KEY_SIZE);
		memcpy(outbuf, req->dst, req->dst_len);
	} else if (rctx->flags & FLG_VERI) {
		hal_crypto_bignum_byteswap(outbuf, SM2_KEY_SIZE);
		memcpy(outbuf, req->dst, req->dst_len);
	} else if (rctx->flags & FLG_ENC) {
	} else if (rctx->flags & FLG_DEC) {
	}

    if (DEBUG_CE)
        hal_crypto_dump_reg();

    hal_crypto_deinit();

    return 0;
}

static int aic_akcipher_sm2_crypt(struct aic_akcipher_handle *handle,
                                  unsigned long flag)
{
    struct aic_akcipher_tfm_ctx *ctx;
    struct aic_akcipher_req_ctx *rctx;
    struct akcipher_tfm *tfm;
    struct akcipher_request *req;
    int ret;

    pr_debug("%s\n", __func__);
    tfm = akcipher_handle_tfm(handle);
    req = akcipher_handle_req(handle);
    ctx = akcipher_tfm_ctx(tfm);
    rctx = akcipher_request_ctx(req);

    if (!ctx) {
        pr_err("aic akcipher, device is null\n");
        return -ENODEV;
    }

    rctx->flags = flag;

    ret = aic_akcipher_sm2_prepare_req(tfm, req);
    if (ret) {
        pr_err("akcipher prepare req failed.\n");
        return ret;
    }
    ret = aic_akcipher_sm2_do_one_req(req);
    if (ret) {
        pr_err("akcipher do one req failed.\n");
        return ret;
    }
    ret = aic_akcipher_sm2_unprepare_req(tfm, req);
    if (ret) {
        pr_err("akcipher unprepare req failed.\n");
        return ret;
    }

    return ret;
}

int aic_akcipher_sm2_sign(struct aic_akcipher_handle *handle)
{
    return aic_akcipher_sm2_crypt(handle, FLG_SM2 | FLG_SIGN);
}

int aic_akcipher_sm2_verify(struct aic_akcipher_handle *handle)
{
    return aic_akcipher_sm2_crypt(handle, FLG_SM2 | FLG_VERI);
}

int aic_akcipher_sm2_encrypt(struct aic_akcipher_handle *handle)
{
    return aic_akcipher_sm2_crypt(handle, FLG_SM2 | FLG_ENC);
}

int aic_akcipher_sm2_decrypt(struct aic_akcipher_handle *handle)
{
    return aic_akcipher_sm2_crypt(handle, FLG_SM2 | FLG_DEC);
}

int aic_akcipher_sm2_set_pub_key(struct aic_akcipher_handle *handle,
                                 const void *key, unsigned int keylen)
{
    struct akcipher_tfm *tfm = handle->tfm;
    struct aic_akcipher_tfm_ctx *ctx = akcipher_tfm_ctx(tfm);
    struct sm2_key sm2_key;
    int ret;

    pr_debug("%s\n", __func__);
    aic_akcipher_sm2_clear_key(ctx);
    if (DEBUG_CE)
        hexdump_msg("pubkey: ", (unsigned char *)key, keylen, 1);
    ret = sm2_parse_pub_key(&sm2_key, key, keylen);
    if (ret) {
        pr_err("Parse pub key failed.\n");
        goto err;
    }

    if (DEBUG_CE) {
        hexdump_msg("x: ", (unsigned char *)sm2_key.x, sm2_key.x_sz, 1);
        hexdump_msg("y: ", (unsigned char *)sm2_key.y, sm2_key.y_sz, 1);
    }

    ctx->alg_ctx.sm2.x = aicos_memdup_align(0, (void *)sm2_key.x,
                                ALIGN_UP(sm2_key.x_sz, CACHE_LINE_SIZE),
                                CACHE_LINE_SIZE);
    if (!ctx->alg_ctx.sm2.x) {
        pr_err("Copy RSA Key x failed.\n");
        ret = -ENOMEM;
        goto err;
    }
    ctx->alg_ctx.sm2.x_sz = sm2_key.x_sz;

    ctx->alg_ctx.sm2.y = aicos_memdup_align(0, (void *)sm2_key.y,
                                ALIGN_UP(sm2_key.y_sz, CACHE_LINE_SIZE),
                                CACHE_LINE_SIZE);
    if (!ctx->alg_ctx.sm2.y) {
        pr_err("Copy RSA Key y failed.\n");
        ret = -ENOMEM;
        goto err;
    }
    ctx->alg_ctx.sm2.y_sz = sm2_key.y_sz;

    return 0;
err:
    if (ctx->alg_ctx.sm2.x)
        aicos_free_align(0, ctx->alg_ctx.sm2.x);
    if (ctx->alg_ctx.sm2.y)
        aicos_free_align(0, ctx->alg_ctx.sm2.y);
    ctx->alg_ctx.sm2.x = NULL;
    ctx->alg_ctx.sm2.y = NULL;

    return ret;
}

int aic_akcipher_sm2_set_priv_key(struct aic_akcipher_handle *handle,
                                  const void *key, unsigned int keylen)
{
    struct akcipher_tfm *tfm = handle->tfm;
    struct aic_akcipher_tfm_ctx *ctx = akcipher_tfm_ctx(tfm);
    struct sm2_key sm2_key;
    int ret;

    pr_debug("%s\n", __func__);
    aic_akcipher_sm2_clear_key(ctx);
    if (DEBUG_CE)
        hexdump_msg("privkey: ", (unsigned char *)key, keylen, 1);

    ret = sm2_parse_priv_key(&sm2_key, key, keylen);
    if (ret) {
        pr_err("parse priv key error.\n");
        goto err;
    }

    if (DEBUG_CE) {
        hexdump_msg("x: ", (unsigned char *)sm2_key.x, sm2_key.x_sz, 1);
        hexdump_msg("y: ", (unsigned char *)sm2_key.y, sm2_key.y_sz, 1);
        hexdump_msg("d: ", (unsigned char *)sm2_key.d, sm2_key.d_sz, 1);
    }

    ctx->alg_ctx.sm2.x = aicos_memdup_align(0, (void *)sm2_key.x,
                                ALIGN_UP(sm2_key.x_sz, CACHE_LINE_SIZE),
                                CACHE_LINE_SIZE);
    if (!ctx->alg_ctx.sm2.x) {
        pr_err("Copy SM2 Key x failed.\n");
        ret = -ENOMEM;
        goto err;
    }
    ctx->alg_ctx.sm2.x_sz = sm2_key.x_sz;

    ctx->alg_ctx.sm2.y = aicos_memdup_align(0, (void *)sm2_key.y,
                                ALIGN_UP(sm2_key.y_sz, CACHE_LINE_SIZE),
                                CACHE_LINE_SIZE);
    if (!ctx->alg_ctx.sm2.y) {
        pr_err("Copy SM2 Key y failed.\n");
        ret = -ENOMEM;
        goto err;
    }
    ctx->alg_ctx.sm2.y_sz = sm2_key.y_sz;

    ctx->alg_ctx.sm2.d = aicos_memdup_align(0, (void *)sm2_key.d,
                                ALIGN_UP(sm2_key.d_sz, CACHE_LINE_SIZE),
                                CACHE_LINE_SIZE);
    if (!ctx->alg_ctx.sm2.d) {
        pr_err("Copy RSA Key D failed.\n");
        ret = -ENOMEM;
        goto err;
    }
    ctx->alg_ctx.sm2.d_sz = sm2_key.d_sz;

    return 0;

err:
    if (ctx->alg_ctx.sm2.x)
        aicos_free_align(0, ctx->alg_ctx.sm2.x);
    if (ctx->alg_ctx.sm2.y)
        aicos_free_align(0, ctx->alg_ctx.sm2.y);
    if (ctx->alg_ctx.sm2.d)
        aicos_free_align(0, ctx->alg_ctx.sm2.d);
    ctx->alg_ctx.sm2.x = NULL;
    ctx->alg_ctx.sm2.y = NULL;
    ctx->alg_ctx.sm2.d = NULL;

    return ret;
}

u32 aic_akcipher_sm2_max_size(struct aic_akcipher_handle *handle)
{
    return 0x80;
}
