/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#include <string.h>
#include <aic_core.h>
#include <aic_utils.h>
#include <hal_ce.h>
#include <ssram.h>
#include "akcipher.h"
#include "rsa.h"
#include "sm2.h"

static struct aic_akcipher_alg ak_algs[] = {
{
	.alg = {
		.encrypt = aic_akcipher_rsa_encrypt,
		.decrypt = aic_akcipher_rsa_decrypt,
		.set_pub_key = aic_akcipher_rsa_set_pub_key,
		.set_priv_key = aic_akcipher_rsa_set_priv_key,
		.max_size = aic_akcipher_rsa_max_size,
		.reqsize = sizeof(struct aic_akcipher_req_ctx),
		.base = {
			.cra_name = "rsa",
			.cra_driver_name = "rsa-aic",
			.cra_priority = 400,
			.cra_ctxsize = sizeof(struct aic_akcipher_tfm_ctx),
		},
	},
},
{
	.alg = {
		.encrypt = aic_akcipher_pnk_rsa_encrypt,
		.decrypt = aic_akcipher_pnk_rsa_decrypt,
		.set_pub_key = aic_akcipher_rsa_set_pub_key,
		.set_priv_key = aic_akcipher_rsa_set_priv_key,
		.max_size = aic_akcipher_rsa_max_size,
		.reqsize = sizeof(struct aic_akcipher_req_ctx),
		.base = {
			.cra_name = "pnk-protected(rsa)",
			.cra_driver_name = "pnk-protected-rsa-aic",
			.cra_priority = 400,
			.cra_ctxsize = sizeof(struct aic_akcipher_tfm_ctx),
		},
	},
},
{
	.alg = {
		.encrypt = aic_akcipher_psk0_rsa_encrypt,
		.decrypt = aic_akcipher_psk0_rsa_decrypt,
		.set_pub_key = aic_akcipher_rsa_set_pub_key,
		.set_priv_key = aic_akcipher_rsa_set_priv_key,
		.max_size = aic_akcipher_rsa_max_size,
		.reqsize = sizeof(struct aic_akcipher_req_ctx),
		.base = {
			.cra_name = "psk0-protected(rsa)",
			.cra_driver_name = "psk0-protected-rsa-aic",
			.cra_priority = 400,
			.cra_ctxsize = sizeof(struct aic_akcipher_tfm_ctx),
		},
	},
},
{
	.alg = {
		.encrypt = aic_akcipher_psk1_rsa_encrypt,
		.decrypt = aic_akcipher_psk1_rsa_decrypt,
		.set_pub_key = aic_akcipher_rsa_set_pub_key,
		.set_priv_key = aic_akcipher_rsa_set_priv_key,
		.max_size = aic_akcipher_rsa_max_size,
		.reqsize = sizeof(struct aic_akcipher_req_ctx),
		.base = {
			.cra_name = "psk1-protected(rsa)",
			.cra_driver_name = "psk1-protected-rsa-aic",
			.cra_priority = 400,
			.cra_ctxsize = sizeof(struct aic_akcipher_tfm_ctx),
		},
	},
},
{
	.alg = {
		.encrypt = aic_akcipher_psk2_rsa_encrypt,
		.decrypt = aic_akcipher_psk2_rsa_decrypt,
		.set_pub_key = aic_akcipher_rsa_set_pub_key,
		.set_priv_key = aic_akcipher_rsa_set_priv_key,
		.max_size = aic_akcipher_rsa_max_size,
		.reqsize = sizeof(struct aic_akcipher_req_ctx),
		.base = {
			.cra_name = "psk2-protected(rsa)",
			.cra_driver_name = "psk2-protected-rsa-aic",
			.cra_priority = 400,
			.cra_ctxsize = sizeof(struct aic_akcipher_tfm_ctx),
		},
	},
},
{
	.alg = {
		.encrypt = aic_akcipher_psk3_rsa_encrypt,
		.decrypt = aic_akcipher_psk3_rsa_decrypt,
		.set_pub_key = aic_akcipher_rsa_set_pub_key,
		.set_priv_key = aic_akcipher_rsa_set_priv_key,
		.max_size = aic_akcipher_rsa_max_size,
		.reqsize = sizeof(struct aic_akcipher_req_ctx),
		.base = {
			.cra_name = "psk3-protected(rsa)",
			.cra_driver_name = "psk3-protected-rsa-aic",
			.cra_priority = 400,
			.cra_ctxsize = sizeof(struct aic_akcipher_tfm_ctx),
		},
	},
},
{
	.alg = {
        .sign = aic_akcipher_sm2_sign,
        .verify = aic_akcipher_sm2_verify,
		.encrypt = aic_akcipher_sm2_encrypt,
		.decrypt = aic_akcipher_sm2_decrypt,
		.set_pub_key = aic_akcipher_sm2_set_pub_key,
		.set_priv_key = aic_akcipher_sm2_set_priv_key,
		.max_size = aic_akcipher_sm2_max_size,
		.reqsize = sizeof(struct aic_akcipher_req_ctx),
		.base = {
			.cra_name = "sm2",
			.cra_driver_name = "sm2-aic",
			.cra_priority = 400,
			.cra_ctxsize = sizeof(struct aic_akcipher_tfm_ctx),
		},
	},
},
};

void aic_akcipher_destroy(struct aic_akcipher_handle *handle)
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

struct aic_akcipher_handle *aic_akcipher_init(const char *ciphername, u32 flags)
{
    struct aic_akcipher_handle *handle = NULL;
    struct aic_akcipher_tfm_ctx *ctx = NULL;
    struct aic_akcipher_req_ctx *rctx = NULL;
    struct akcipher_request *req = NULL;
    struct akcipher_tfm *tfm = NULL;

    handle = aicos_malloc(0, sizeof(struct aic_akcipher_handle));
    if (handle == NULL) {
        pr_err("malloc handle failed.\n");
        return NULL;
    }
    memset(handle, 0, sizeof(struct aic_akcipher_handle));

    tfm = aicos_malloc(0, sizeof(struct akcipher_tfm));
    if (tfm == NULL) {
        pr_err("malloc tfm failed.\n");
        goto out;
    }
    memset(tfm, 0, sizeof(struct akcipher_tfm));
    handle->tfm = tfm;

    ctx = aicos_malloc(0, sizeof(struct aic_akcipher_tfm_ctx));
    if (ctx == NULL) {
        pr_err("malloc ctx failed.\n");
        goto out;
    }
    memset(ctx, 0, sizeof(struct aic_akcipher_tfm_ctx));
    tfm->__crt_ctx = (void *)ctx;

    req = aicos_malloc(0, sizeof(struct akcipher_request));
    if (req == NULL) {
        pr_err("malloc req failed.\n");
        goto out;
    }
    memset(req, 0, sizeof(struct akcipher_request));
    handle->req = req;

    rctx = aicos_malloc(0, sizeof(struct aic_akcipher_req_ctx));
    if (rctx == NULL) {
        pr_err("malloc rctx failed.\n");
        goto out;
    }
    memset(rctx, 0, sizeof(struct aic_akcipher_req_ctx));
    req->__ctx = (void *)rctx;

    for (int i = 0; i < ARRAY_SIZE(ak_algs); i++) {
        if (!strcmp(ak_algs[i].alg.base.cra_name, ciphername) ||
            !strcmp(ak_algs[i].alg.base.cra_driver_name, ciphername)) {
            tfm->alg = &ak_algs[i].alg;
            return handle;
        }
    }
    pr_warn("not found %s algo.\n", ciphername);

out:
    aic_akcipher_destroy(handle);

    return NULL;
}
