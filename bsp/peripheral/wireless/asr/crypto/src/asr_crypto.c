/*
 * Copyright (c) 2023 ASR Microelectronics (Shanghai) Co., Ltd. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "common.h"
#include "crypto.h"
#include "wpa_common.h"
#include "wpas_aes.h"
/* mbedtls headers */
#include "mbedtls/platform.h"
#include "mbedtls/bignum.h"
#include "mbedtls/ecp.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/md.h"
#include "mbedtls/aes.h"
#include "mbedtls/cmac.h"
#include "mbedtls/des.h"
#include "mbedtls/ccm.h"
//#include "mbedtls/bn_mul.h"
#include "bn_mul.h"

#define MBEDTLS_ALLOW_PRIVATE_ACCESS

#define ECC_POINT_COMP_EVEN 0x02
#define ECC_POINT_COMP_ODD  0x03
#define ciL (sizeof(mbedtls_mpi_uint)) /* chars in limb  */
#define CHARS_TO_LIMBS(i) ( (i) / ciL + ( (i) % ciL != 0 ) )

struct asr_ec {
    mbedtls_ecp_keypair key;
};

static const mbedtls_mpi_uint secp256r1_a[] = {
    MBEDTLS_BYTES_TO_T_UINT_8(0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF),
    MBEDTLS_BYTES_TO_T_UINT_8(0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00),
    MBEDTLS_BYTES_TO_T_UINT_8(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),
    MBEDTLS_BYTES_TO_T_UINT_8(0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF),
};

static inline void asr_mpi_load(mbedtls_mpi *X, const mbedtls_mpi_uint *p, size_t len)
{
    X->s = 1;
    X->n = len / sizeof(mbedtls_mpi_uint);
    X->p = (mbedtls_mpi_uint *) p;
}

static int mbedtls_mpi_read(uint8_t *out, uint16_t *out_len, const mbedtls_mpi *X, size_t buflen)
{
    size_t i, j, n;

    n = mbedtls_mpi_size( X );

    if(buflen < n)
        return MBEDTLS_ERR_MPI_BUFFER_TOO_SMALL;

    memset(out, 0, buflen);

    if((*out_len != 0) && ((*out_len > n) && (*out_len <= X->n * ciL)))
    {
        n = *out_len;
    }

    for(i = 0, j = n - 1; i < n; i++, j--)
    {
        out[i] = (unsigned char)(X->p[j / ciL] >> ((j % ciL) << 3));
    }

    *out_len = n;
    return 0;
}

static int mbedtls_mpi_write(const mbedtls_mpi *X, const uint8_t *indata, uint16_t inlen)
{
    int ret;
    size_t i, j;

    MBEDTLS_MPI_CHK( mbedtls_mpi_grow((struct mbedtls_mpi *)X, CHARS_TO_LIMBS(inlen)) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_lset((struct mbedtls_mpi *)X, 0) );

    for(i = inlen, j = 0; i > 0; i--, j++)
    {
        X->p[j / ciL] |= ((mbedtls_mpi_uint)indata[i - 1]) << ((j % ciL) << 3);
    }

cleanup:

    return ret;
}

struct asr_bignum * asr_bignum_init(void)
{
    mbedtls_mpi *a;

    a = os_malloc(sizeof(mbedtls_mpi));
    if (!a)
    {
        wpa_printf(MSG_DEBUG, "crypto_bignum_int os_malloc error\r\n");
        return NULL;
    }

    mbedtls_mpi_init(a);

    return (struct asr_bignum *)a;
}

struct asr_bignum * asr_bignum_init_set(const uint8_t *buf, size_t len)
{
    int iRet;
    mbedtls_mpi *a = NULL;

    a = (mbedtls_mpi *)asr_bignum_init();
    if (!a)
    {
        wpa_printf(MSG_DEBUG, "asr_bignum_init_set asr_bignum_init error\r\n");
        return NULL;
    }
    // wpa_hexdump(MSG_INFO, "bignum_init_set", buf, len);
    iRet = mbedtls_mpi_write(a, buf, len);
    if (iRet != 0)
    {
        wpa_printf(MSG_DEBUG, "asr_bignum_init_set mbedtls_mpi_write error\r\n");
        os_free(a);
        a = NULL;
    }
    // mbed_dump_u32("bignum_init_set", a->p, a->n);

    return (struct asr_bignum *)a;
}

struct asr_bignum * asr_bignum_init_uint(unsigned int val)
{
    mbedtls_mpi *a;

    a = (mbedtls_mpi *) asr_bignum_init();
    if (!a)
    {
        wpa_printf(MSG_DEBUG, "asr_bignum_init_uint asr_bignum_init error\r\n");
        return NULL;
    }

    if (mbedtls_mpi_lset(a, val) != 0) {
        os_free(a);
        wpa_printf(MSG_DEBUG, "asr_bignum_init_uint mbedtls_mpi_lset error\r\n");
        a = NULL;
    }

    return (struct asr_bignum *) a;
}

void asr_bignum_deinit(struct asr_bignum *n, int clear)
{
    if (!n)
        return;

    mbedtls_mpi_free((mbedtls_mpi *)n);
    os_free((mbedtls_mpi *)n);
}

int asr_bignum_to_bin(const struct asr_bignum *a,
             uint8_t *buf, size_t buflen, size_t padlen)
{
    int num_bytes, offset, iRet;
    uint16_t out_len = 0;

    if (padlen > buflen)
        return -1;

    num_bytes = (mbedtls_mpi_bitlen((mbedtls_mpi *) a) + 7) / 8;
    if ((size_t) num_bytes > buflen)
        return -1;
    if (padlen > (size_t) num_bytes)
        offset = padlen - num_bytes;
    else
        offset = 0;

    if(offset > 0)
        os_memset(buf, 0, offset);

    iRet = mbedtls_mpi_read(buf + offset, &out_len, (mbedtls_mpi *)a, buflen - offset);
    if (iRet != 0)
    {
        wpa_printf(MSG_DEBUG, "asr_bignum_to_bin mbedtls_mpi_write_binary error\r\n");
    }
    // wpa_hexdump(MSG_INFO, "bignum_to_bin", buf, buflen);
    return num_bytes + offset;
}

int asr_bignum_rand(struct asr_bignum *r, const struct asr_bignum *m)
{
    int ret = 0;

    int len = (mbedtls_mpi_bitlen((mbedtls_mpi *) m) + 7) / 8 * 2;
    unsigned char *buf;
    mbedtls_mpi *a;

    a = os_malloc(sizeof(mbedtls_mpi));
    if (!a)
    {
        wpa_printf(MSG_ERROR, "asr_bignum_rand a malloc fail\r\n");
        return -1;
    }

    mbedtls_mpi_init(a);

    buf = (unsigned char *)os_malloc(len);
    if (NULL == buf){
      wpa_printf(MSG_ERROR, "rand malloc fail\n");
      return -1;
    }

    os_get_random(buf, len);
    // wpa_hexdump(MSG_INFO, "bignum_rand", buf, len);
    if (mbedtls_mpi_write((mbedtls_mpi *)a, buf, len) != 0)
    {
       wpa_printf(MSG_ERROR, "read bin fail\n");
       os_free(buf);
       return -1;
    }

    os_free(buf);

    if (mbedtls_mpi_mod_mpi((mbedtls_mpi *)r, a, (mbedtls_mpi *)m) != 0)
    {
        wpa_printf(MSG_INFO, "mp_mod fail");
        ret = -1;
    }

    mbedtls_mpi_free(a);
    os_free(a);

    return ret;
}

int asr_bignum_mod(const struct asr_bignum *a,
              const struct asr_bignum *m,
              struct asr_bignum *r)
{
    int ret;

    ret = mbedtls_mpi_mod_mpi((mbedtls_mpi *)r, (mbedtls_mpi *)a, (mbedtls_mpi *)m);

    return( ret );
}

int asr_bignum_exptmod(const struct asr_bignum *a,
              const struct asr_bignum *e,
              const struct asr_bignum *n,
              struct asr_bignum *x)
{
    int ret;

    // mbed_dump_u32("exptmod a", ((mbedtls_mpi *)a)->p, ((mbedtls_mpi *)a)->n);
    // mbed_dump_u32("exptmod e", ((mbedtls_mpi *)e)->p, ((mbedtls_mpi *)e)->n);
    // mbed_dump_u32("exptmod n", ((mbedtls_mpi *)n)->p, ((mbedtls_mpi *)n)->n);

    // x = a^e mod n
    mbedtls_mpi x_tmp;

    mbedtls_mpi_init( &x_tmp );

    ret = mbedtls_mpi_exp_mod((mbedtls_mpi *)&x_tmp, (mbedtls_mpi *)a, (mbedtls_mpi *)e, (mbedtls_mpi *)n, NULL);
    //ret = mbedtls_mpi_exp_mod((mbedtls_mpi *)x, (mbedtls_mpi *)a, (mbedtls_mpi *)e, (mbedtls_mpi *)n, NULL);
    mbedtls_mpi_copy((mbedtls_mpi *)x, (mbedtls_mpi *)&x_tmp);

    // mbed_dump_u32("exptmod x", ((mbedtls_mpi *)x)->p, ((mbedtls_mpi *)x)->n);

    return( ret );
}

int asr_bignum_inverse(const struct asr_bignum *a,
              const struct asr_bignum *m,
              struct asr_bignum *r)
{
    int ret;

    ret = mbedtls_mpi_inv_mod((mbedtls_mpi *) r, (mbedtls_mpi *) a, (mbedtls_mpi *) m);

    return( ret );
}

int asr_bignum_add(const struct asr_bignum *a,
              const struct asr_bignum *b,
              struct asr_bignum *r)
{
    int ret = mbedtls_mpi_add_mpi((mbedtls_mpi *)r,(mbedtls_mpi *)a, (mbedtls_mpi *)b);

    if(ret == 0)
        return 0;
    else
        return -1;
}

int asr_bignum_sub(const struct asr_bignum *a,
              const struct asr_bignum *b,
              struct asr_bignum *r)
{
    int ret = mbedtls_mpi_sub_mpi((mbedtls_mpi *)r,(mbedtls_mpi *)a, (mbedtls_mpi *)b);

    if(ret == 0)
        return 0;
    else
        return -1;
}

int asr_bignum_div(const struct asr_bignum *a,
              const struct asr_bignum *b,
              struct asr_bignum *d)
{
    int ret;

    // d = a / b
    ret = mbedtls_mpi_div_mpi((mbedtls_mpi *)d, NULL, (mbedtls_mpi *)a, (mbedtls_mpi *)b);

    return( ret );
}


int asr_bignum_mulmod(const struct asr_bignum *a,
             const struct asr_bignum *b,
             const struct asr_bignum *m,
             struct asr_bignum *r)
{
    int ret;
    mbedtls_mpi temp;

    mbedtls_mpi_init(&temp);

    // temp = a * b
    if (mbedtls_mpi_mul_mpi(&temp, (mbedtls_mpi *)a, (mbedtls_mpi *)b) != 0)
    {
        return -1;
    }

    // r = temp mod m
    ret = mbedtls_mpi_mod_mpi((mbedtls_mpi *)r, &temp, (mbedtls_mpi *)m);

    mbedtls_mpi_free(&temp);

    return ret;
}

int asr_bignum_addmod(const struct asr_bignum *a,
             const struct asr_bignum *b,
             const struct asr_bignum *m,
             struct asr_bignum *r)
{
    int ret;
    mbedtls_mpi temp;

    mbedtls_mpi_init(&temp);

    // temp = a + b
    if (mbedtls_mpi_add_mpi(&temp, (mbedtls_mpi *)a, (mbedtls_mpi *)b) != 0)
    {
        return -1;
    }

    // r = temp mod m
    ret = mbedtls_mpi_mod_mpi((mbedtls_mpi *)r, &temp, (mbedtls_mpi *)m);

    mbedtls_mpi_free(&temp);

    return ret;
}

int asr_bignum_sqrmod(const struct asr_bignum *a,
             const struct asr_bignum *m,
             struct asr_bignum *r)
{
    int ret;
    mbedtls_mpi temp;

    mbedtls_mpi_init(&temp);

    // temp = a * a
    if (mbedtls_mpi_mul_mpi(&temp, (mbedtls_mpi *)a, (mbedtls_mpi *)a) != 0)
    {
        return -1;
    }

    // r = temp mod m
    ret = mbedtls_mpi_mod_mpi((mbedtls_mpi *)r, &temp, (mbedtls_mpi *)m);

    mbedtls_mpi_free(&temp);

    return ret;
}

int asr_bignum_rshift(const struct asr_bignum *a, int n,
             struct asr_bignum *r)
{
    if (mbedtls_mpi_copy((mbedtls_mpi *)r, (mbedtls_mpi *)a) != 0)
        return -1;

    return mbedtls_mpi_shift_r((mbedtls_mpi *)r, n);
}

int asr_bignum_cmp(const struct asr_bignum *a,
              const struct asr_bignum *b)
{
    return mbedtls_mpi_cmp_mpi((mbedtls_mpi *)a, (mbedtls_mpi *)b);
}

int asr_bignum_is_zero(const struct asr_bignum *a)
{
    int ret = mbedtls_mpi_cmp_int((mbedtls_mpi *)a, 0);

    if(ret == 0)
        return 1;
    else
        return 0;
}

int asr_bignum_is_one(const struct asr_bignum *a)
{
    int ret = mbedtls_mpi_cmp_int((mbedtls_mpi *)a, 1);

    if(ret == 0)
        return 1;
    else
        return 0;
}

int asr_bignum_is_odd(const struct asr_bignum *a)
{
    mbedtls_mpi_uint r;

    mbedtls_mpi_mod_int(&r, (mbedtls_mpi *)a, 2);

    if(r == 1)
        return 1;
    else
        return 0;
}

int asr_bignum_legendre(const struct asr_bignum *a,
               const struct asr_bignum *p)
{
    mbedtls_mpi *t = NULL, *r = NULL;
    int ret;
    int res = -2;

    if ((a == NULL) || (p == NULL))
        return -2;

    t = os_malloc(sizeof(mbedtls_mpi));
    r = os_malloc(sizeof(mbedtls_mpi));
    if ((t == NULL) || (r == NULL))
    {
        if(r != NULL)
            os_free(r);

        if(t != NULL)
            os_free(t);

        return -1;
    }

    mbedtls_mpi_init(t);
    mbedtls_mpi_init(r);

    /* t = (p-1) / 2 */
    ret = mbedtls_mpi_sub_int(t, (mbedtls_mpi *)p, 1);
    // mbed_dump_u32("p-1", t->p, t->n);
    if (ret == 0)
    {
        mbedtls_mpi_shift_r(t, 1);
        // mbed_dump_u32("((p-1)/2)", t->p, t->n);
        // mbed_dump_u32("a", ((mbedtls_mpi *)a)->p, ((mbedtls_mpi *)a)->n);
        // mbed_dump_u32("p", ((mbedtls_mpi *)p)->p, ((mbedtls_mpi *)p)->n);
        ret = asr_bignum_exptmod((struct asr_bignum *)a, (const struct asr_bignum *)t, p, (struct asr_bignum *)r);
    }

    if (ret == 0)
    {
        if(asr_bignum_is_one((const struct asr_bignum *)r))
        {
            res = 1;
        }
        else if(asr_bignum_is_zero((const struct asr_bignum *)r))
        {
            res = 0;
        }
        else
        {
            res = -1;
        }
        // mbed_dump_u32("r = a^((p-1)/2)", r->p, r->n);
    }

    mbedtls_mpi_free(t);
    mbedtls_mpi_free(r);
    os_free(t);
    os_free(r);

    return res;
}

struct asr_ec * asr_ec_init(int group)
{
    struct asr_ec *e = NULL;
    int curve_id;

    /* Map from IANA registry for IKE D-H groups to OpenSSL NID */
    switch (group) {
    case 19:
        curve_id = MBEDTLS_ECP_DP_SECP256R1;
        break;
    case 20:
        curve_id = MBEDTLS_ECP_DP_SECP384R1;
        break;
    case 21:
        curve_id = MBEDTLS_ECP_DP_SECP521R1;
        break;
    case 25:
        curve_id = MBEDTLS_ECP_DP_SECP192R1;
        break;
    case 26:
        curve_id = MBEDTLS_ECP_DP_SECP224R1;
        break;
#ifdef HAVE_ECC_BRAINPOOL
    case 27:
        curve_id = MBEDTLS_ECP_DP_BRAINPOOLP224R1;
        break;
    case 28:
        curve_id = MBEDTLS_ECP_DP_BRAINPOOLP256R1;
        break;
    case 29:
        curve_id = MBEDTLS_ECP_DP_BRAINPOOLP384R1;
        break;
    case 30:
        curve_id = MBEDTLS_ECP_DP_BRAINPOOLP512R1;
        break;
#endif /* HAVE_ECC_BRAINPOOL */
    default:
        return NULL;
    }

    e = os_zalloc(sizeof(struct asr_ec));
    if (!e)
        return NULL;

    mbedtls_ecp_keypair_init(&e->key);
    if (mbedtls_ecp_group_load(&(e->key.grp), curve_id) != 0)
    {
        wpa_printf(MSG_DEBUG, "asr_ec_init mbedtls_ecp_group_load error\r\n");
        asr_ec_deinit(e);
        e = NULL;
    }

    if( (e != NULL) && (e->key.grp.A.p == NULL))
    {
       if(curve_id == MBEDTLS_ECP_DP_SECP256R1)
       {
           asr_mpi_load(&(e->key.grp.A), secp256r1_a ,sizeof(secp256r1_a));
       }
    }

    return e;
}

void asr_ec_deinit(struct asr_ec* e)
{
    if (!e)
        return;

    mbedtls_ecp_keypair_free(&e->key);
    os_free(e);
}


struct asr_ec_point * asr_ec_point_init(struct asr_ec *e)
{
    if (!e)
        return NULL;

    mbedtls_ecp_point *mbedtls_point;
    mbedtls_point = os_malloc(sizeof(mbedtls_ecp_point));
    if (mbedtls_point == NULL)
    {
        wpa_printf(MSG_DEBUG, "asr_ec_point_init asr_ec_deinit\r\n");
        return NULL;
    }
    mbedtls_ecp_point_init(mbedtls_point);
    return (struct asr_ec_point *)mbedtls_point;
}

size_t asr_ec_prime_len(struct asr_ec *e)
{
    return (mbedtls_mpi_bitlen(&e->key.grp.P) + 7) / 8;
}

size_t asr_ec_prime_len_bits(struct asr_ec *e)
{
    return mbedtls_mpi_bitlen(&e->key.grp.P);
}


size_t asr_ec_order_len(struct asr_ec *e)
{
    return (mbedtls_mpi_bitlen(&e->key.grp.N) + 7) / 8;
}

const struct asr_bignum * asr_ec_get_prime(struct asr_ec *e)
{
    return (const struct asr_bignum *)&e->key.grp.P;
}

const struct asr_bignum * asr_ec_get_order(struct asr_ec *e)
{
    return (const struct asr_bignum *)&e->key.grp.N;
}

const struct asr_bignum * asr_ec_get_a(struct asr_ec *e)
{
    return (const struct asr_bignum *) &e->key.grp.A;
}

const struct asr_bignum * asr_ec_get_b(struct asr_ec *e)
{
    return (const struct asr_bignum *) &e->key.grp.B;
}

void asr_ec_point_deinit(struct asr_ec_point *p, int clear)
{
    mbedtls_ecp_point *point = NULL;

    if (!p)
        return;

    point = (mbedtls_ecp_point *)p;

    mbedtls_ecp_point_free(point);
    os_free(point);
}

int asr_ec_point_x(struct asr_ec *e, const struct asr_ec_point *p,
              struct asr_bignum *x)
{
    return mbedtls_mpi_copy((mbedtls_mpi *)x, &(((mbedtls_ecp_point *)p)->X)) == 0 ? 0 : -1;
}

int asr_ec_point_to_bin(struct asr_ec *e, const struct asr_ec_point *point, uint8_t *x, uint8_t *y)
{
    int plen;
    mbedtls_ecp_point *p = (mbedtls_ecp_point *)point;

    //if p->z != 1 then normalize
    if (!asr_bignum_is_one((const struct asr_bignum *)&(p->Z)))
    {
        if (asr_bignum_is_zero((const struct asr_bignum *)&(p->Z)))
        {
            mbedtls_mpi_lset(&p->X, 0);
            mbedtls_mpi_lset(&p->Y, 0);
            mbedtls_mpi_lset(&p->Z, 1);
        }
        else
        {
            // if(ecp_normalize_jac(&e->key.grp, p) != 0)
            //     return -1;
            wpa_printf(MSG_DEBUG, "asr_ec_point_to_bin point must to do normalize\r\n");
            return -1;
        }
    }

    plen = mbedtls_mpi_size(&e->key.grp.P);
    if (x)
    {
        if (asr_bignum_to_bin((struct asr_bignum *)&(p->X), x,
                    plen,//e->key.grp.T_size,
                    plen//e->key.grp.T_size
                    ) <= 0)
        {
            wpa_printf(MSG_DEBUG, "asr_ec_point_to_bin asr_bignum_to_bin x error\r\n");
            return -1;
        }
    }

    if (y)
    {
        if (asr_bignum_to_bin((struct asr_bignum *)&(p->Y), y,
                plen,///e->key.grp.T_size,
                plen///e->key.grp.T_size
                ) <= 0)
            {
                wpa_printf(MSG_DEBUG, "asr_ec_point_to_bin asr_bignum_to_bin y error\r\n");
                return -1;
            }
    }

    return 0;
}


struct asr_ec_point * asr_ec_point_from_bin(struct asr_ec *e,
                          const uint8_t *val)
{
    mbedtls_ecp_point *point = NULL;
    int loaded = 0;
    int plen = mbedtls_mpi_size(&e->key.grp.P);

    point = os_malloc(sizeof(mbedtls_ecp_point));
    if (point == NULL)
    {
        wpa_printf(MSG_DEBUG, "asr_ec_point_from_bin os_malloc error\r\n");
        return NULL;
    }
    mbedtls_ecp_point_init(point);

    if (!point)
        goto done;

    if (mbedtls_mpi_write(&point->X, val, plen) != 0)
    {
        wpa_printf(MSG_DEBUG, "asr_ec_point_from_bin mbedtls_mpi_read_binary X error\r\n");
        goto done;
    }

    val += plen;//e->key.grp.T_size;
    if (mbedtls_mpi_write(&point->Y, val, plen) != 0)
    {
        wpa_printf(MSG_DEBUG, "asr_ec_point_from_bin mbedtls_mpi_read_binary Y error\r\n");
        goto done;
    }

    mbedtls_mpi_lset(&point->Z, 1);

    loaded = 1;
done:
    if (!loaded) {
        mbedtls_ecp_point_free(point);
        os_free(point);
        point = NULL;
    }
    return (struct asr_ec_point *)point;
}

int asr_ec_point_add(struct asr_ec *e, const struct asr_ec_point *a,
            const struct asr_ec_point *b,
            struct asr_ec_point *c)
{
    int iRet;
    mbedtls_mpi m, n;

    mbedtls_mpi_init(&m);
    mbedtls_mpi_init(&n);

    iRet =  mbedtls_mpi_lset(&m, 1);
    if (iRet != 0)
    {
        wpa_printf(MSG_DEBUG, "asr_ec_point_add mbedtls_mpi_lset m error\r\n");
        goto done;
    }

    iRet =  mbedtls_mpi_lset(&n, 1);
    if (iRet != 0)
    {
        wpa_printf(MSG_DEBUG, "asr_ec_point_add mbedtls_mpi_lset n error\r\n");
        goto done;
    }

    //c = m*a + n*b (m=1, n=1)
    iRet = mbedtls_ecp_muladd(&(e->key.grp), (mbedtls_ecp_point *)c, &m, (mbedtls_ecp_point *)a, &n, (mbedtls_ecp_point *)b);
    if (iRet != 0)
    {
        wpa_printf(MSG_DEBUG, "asr_ec_point_add mbedtls_ecp_muladd error\r\n");
        goto done;
    }

done:
    mbedtls_mpi_free(&m);
    mbedtls_mpi_free(&n);
    return iRet;
}

static int asr_rng( void *ctx, unsigned char *out, size_t len )
{
    static uint32_t state = 42;

    (void) ctx;
    size_t i;
    for( i = 0; i < len; i++ )
    {
        state = state * 1664525u + 1013904223u;
        out[i] = (unsigned char) state;
    }

    return( 0 );
}

int asr_ec_point_mul(struct asr_ec *e, const struct asr_ec_point *p,
            const struct asr_bignum *b,
            struct asr_ec_point *res)
{
    int ret = -1;

    ret = mbedtls_ecp_mul(&e->key.grp, (mbedtls_ecp_point *)res, (mbedtls_mpi *)b, (mbedtls_ecp_point *)p, asr_rng, NULL);
    if (ret != 0)
    {
        wpa_printf(MSG_DEBUG, "asr_ec_point_mul mbedtls_ecp_mul_restartable %d error\r\n", ret);
        return -1;
    }

    return ret == 0 ? 0 : -1;
}

int asr_ec_point_invert(struct asr_ec *e, struct asr_ec_point *p)
{
    mbedtls_ecp_point *point = (mbedtls_ecp_point *)p;

    if (mbedtls_mpi_sub_mpi(&point->Y, &e->key.grp.P, &point->Y) != 0)
    {
        wpa_printf(MSG_DEBUG, "asr_ec_point_invert mbedtls_mpi_sub_mpi error\r\n");
        return -1;
    }
    return 0;
}

/* solves the modular equation r^2 = n (mod prime) */
static int asr_sqrtmod_prime(struct asr_bignum *n, struct asr_bignum *prime, struct asr_bignum *r)
{
    int iRet = -1;
    mbedtls_mpi *t1, *e;

    e = os_malloc(sizeof(mbedtls_mpi));
    t1 = os_malloc(sizeof(mbedtls_mpi));
    if ((NULL == e) || (NULL == t1))
    {
        if(e != NULL)
            os_free(e);
        if(t1 != NULL)
            os_free(t1);
        wpa_printf(MSG_DEBUG, "asr_import_point_der os_malloc eTemp error\r\n");
        iRet = -1;
        return iRet;
    }

    mbedtls_mpi_init(e);
    mbedtls_mpi_init(t1);

    // e = prime + 1
    iRet = mbedtls_mpi_add_int(e, (mbedtls_mpi *)prime, 1);
    if (iRet != 0)
    {
        wpa_printf(MSG_DEBUG, "asr_import_point_der prime + 1 error\r\n");
        goto done;
    }
    // mbed_dump_u32("e = prime + 1", e->p, e->n);
    // t1 = e / 4
    iRet = mbedtls_mpi_div_int(t1, NULL, e, 4);
    if (iRet != 0)
    {
        wpa_printf(MSG_DEBUG, "asr_import_point_der mbedtls_mpi_div_int e / 4 error\r\n");
        goto done;
    }
    // mbed_dump_u32("t1 = e / 4", t1->p, t1->n);

    iRet = asr_bignum_exptmod(n, (const struct asr_bignum *)t1, prime, r);
    if (iRet != 0)
    {
        wpa_printf(MSG_DEBUG, "asr_import_point_der asr_bignum_exptmod error\r\n");
        goto done;
    }
    // mbed_dump_u32("r = n^t1 mod prime", ((mbedtls_mpi *)r)->p, ((mbedtls_mpi *)r)->n);

done:
    mbedtls_mpi_free(e);
    mbedtls_mpi_free(t1);
    os_free(t1);
    os_free(e);

    return iRet;
}

// extern mbedtls_mpi * ecp_x_to_y2( const mbedtls_ecp_group *grp, const mbedtls_mpi *x );
struct asr_bignum *asr_ec_point_compute_y_sqr(struct asr_ec *e, const struct asr_bignum *x)
{
    int ret = -1;
    mbedtls_mpi RHS, *Y2 = NULL, *prime = NULL, *temp_x = NULL;

    if ((e == NULL) || (x == NULL))
        return NULL;

    prime = &e->key.grp.P;
    temp_x = (mbedtls_mpi *)x;

    /* pt coordinates must be normalized for our checks */
    if( mbedtls_mpi_cmp_int( temp_x, 0 ) < 0 ||
        mbedtls_mpi_cmp_mpi( temp_x, prime ) >= 0 )
        return NULL;

    if((Y2 = os_malloc(sizeof(mbedtls_mpi))) == NULL)
    {
        goto cleanup;
    }

    mbedtls_mpi_init( &RHS );
    mbedtls_mpi_init( Y2 );

    /*
     * RHS = X (X^2 + A) + B = X^3 + A X + B
     */
    MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi(&RHS, temp_x, temp_x) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi(&RHS, &RHS, prime) );

    /* Special case for A = -3 */
    if( e->key.grp.A.p == NULL )
    {
        MBEDTLS_MPI_CHK( mbedtls_mpi_sub_int(&RHS, &RHS, 3) );
        MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi(&RHS, &RHS, prime) );
    }
    else
    {
        MBEDTLS_MPI_CHK( mbedtls_mpi_add_mpi(&RHS, &RHS, &e->key.grp.A) );
        MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi(&RHS, &RHS, prime) );
    }

    MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi(&RHS, &RHS, temp_x) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi(&RHS, &RHS, prime) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_add_mpi(&RHS, &RHS, &e->key.grp.B) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi(&RHS, &RHS, prime) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_copy(Y2, &RHS) );

cleanup:

    if(ret != 0)
    {
        if(Y2 != NULL)
        {
            mbedtls_mpi_free( Y2 );
            os_free( Y2 );
            Y2 = NULL;
        }
    }
    mbedtls_mpi_free( &RHS );

    return (struct asr_bignum *)Y2;
}

static int asr_import_point_der(struct asr_ec *e, const struct asr_bignum *x, int y_bit, mbedtls_ecp_point *point)
{
    int iRet = -1;
    int pointType;
    mbedtls_mpi *y2 = NULL, *y = NULL, *prime = NULL;

    // check parameter
    if ((x == NULL) || (point == NULL) || (e == NULL))
    {
        wpa_printf(MSG_DEBUG, "asr_import_point_der parameter error\r\n");
        return iRet;
    }

    // check for point type (4,2 or 3)
    pointType = y_bit ? ECC_POINT_COMP_ODD : ECC_POINT_COMP_EVEN;

    y = os_malloc(sizeof(mbedtls_mpi));

    if (NULL == y)
    {
        wpa_printf(MSG_DEBUG, "asr_import_point_der os_malloc y error\r\n");
        return iRet;
    }

    prime = &e->key.grp.P;
    mbedtls_mpi_init(y);

    y2 = (mbedtls_mpi *)asr_ec_point_compute_y_sqr(e, x);
    if (y2 == NULL)
    {
        wpa_printf(MSG_DEBUG, "asr_import_point_der asr_ec_point_compute_y_sqr error\r\n");
        goto done;
    }

    iRet = asr_sqrtmod_prime((struct asr_bignum *)y2, (struct asr_bignum *)prime, (struct asr_bignum *)y);
    if (iRet != 0)
    {
        wpa_printf(MSG_DEBUG, "asr_import_point_der asr_bignum_exptmod error\r\n");
        goto done;
    }

    // adjust y
    if (((asr_bignum_is_odd((const struct asr_bignum *)y) == 1) && (pointType == ECC_POINT_COMP_ODD)) ||
    ((asr_bignum_is_odd((const struct asr_bignum *)y) != 1) && (pointType == ECC_POINT_COMP_EVEN)))
    {
        iRet = mbedtls_mpi_mod_mpi(&point->Y, y, prime);
        if (iRet != 0)
        {
            wpa_printf(MSG_DEBUG, "asr_import_point_der mbedtls_mpi_mod_mpi adjust y error\r\n");
            goto done;
        }
    }
    else
    {
        iRet = mbedtls_mpi_sub_mpi(&point->Y, prime, y);
        if (iRet != 0)
        {
            wpa_printf(MSG_DEBUG, "asr_import_point_der mbedtls_mpi_sub_mpi error\r\n");
            goto done;
        }
        iRet = mbedtls_mpi_mod_mpi(&point->Y, &point->Y, prime);
        if (iRet != 0)
        {
            wpa_printf(MSG_DEBUG, "asr_import_point_der mbedtls_mpi_mod_mpi error\r\n");
            goto done;
        }
    }

    iRet = mbedtls_mpi_copy(&point->X, (mbedtls_mpi *)x);
    if (iRet != 0)
    {
        wpa_printf(MSG_DEBUG, "asr_import_point_der mbedtls_mpi_copy error\r\n");
        goto done;
    }

    iRet = mbedtls_mpi_lset(&point->Z, 1);

done:
    if (y2 != NULL)
    {
        mbedtls_mpi_free(y2);
        os_free(y2);
    }

    if (y != NULL)
    {
        mbedtls_mpi_free(y);
        os_free(y);
    }

    return iRet;
}

int asr_ec_point_solve_y_coord(struct asr_ec *e,
                  struct asr_ec_point *p,
                  const struct asr_bignum *x, int y_bit)
{
    int ret;

    ret = asr_import_point_der(e, x, y_bit, (mbedtls_ecp_point *)p);
    if (ret != 0) {
        wpa_printf(MSG_DEBUG, "SAE: asr_ec_point_solve_y_coord (%d)",ret);
        return -1;
    }

    return 0;
}

int asr_ec_point_is_at_infinity(struct asr_ec *e,
                   const struct asr_ec_point *p)
{
    mbedtls_ecp_point *point = (mbedtls_ecp_point *)p;

    if ((mbedtls_mpi_cmp_int(&point->X, 0) == 0) && (mbedtls_mpi_cmp_int(&point->Y, 0) == 0))
        return 1;
    else
        return 0;
}

int asr_ec_point_is_on_curve(struct asr_ec *e,
                const struct asr_ec_point *p)
{
    return (mbedtls_ecp_check_pubkey(&(e->key.grp), (mbedtls_ecp_point *)p) == 0) ? 1 : 0;
}

int asr_ec_point_cmp(const struct asr_ec *e,
            const struct asr_ec_point *a,
            const struct asr_ec_point *b)
{
    return (mbedtls_ecp_point_cmp((mbedtls_ecp_point *)a, (mbedtls_ecp_point*)b) == 0) ? 0 : -1;
}

//mbedtls enable MBEDTLS_CMAC_C
int omac1_aes_vector(const u8 *key, size_t key_len, size_t num_elem,
             const u8 *addr[], const size_t *len, u8 *mac)
{
    const mbedtls_cipher_info_t *cipher_info;
    int i, ret = 0;
    mbedtls_cipher_type_t cipher_type;
    mbedtls_cipher_context_t ctx;

    switch (key_len) {
    case 16:
        cipher_type = MBEDTLS_CIPHER_AES_128_ECB;
        break;
    case 24:
        cipher_type = MBEDTLS_CIPHER_AES_192_ECB;
        break;
    case 32:
        cipher_type = MBEDTLS_CIPHER_AES_256_ECB;
        break;
    default:
        cipher_type = MBEDTLS_CIPHER_NONE;
        break;
    }
    cipher_info = mbedtls_cipher_info_from_type(cipher_type);
    if (cipher_info == NULL) {
        /* Failing at this point must be due to a build issue */
        ret = MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE;
        goto cleanup;
    }

    if (key == NULL ||  mac == NULL) {
        return -1;
    }

    mbedtls_cipher_init(&ctx);

    ret = mbedtls_cipher_setup(&ctx, cipher_info);
    if (ret != 0) {
        goto cleanup;
    }

    ret = mbedtls_cipher_cmac_starts(&ctx, key, key_len * 8);
    if (ret != 0) {
        goto cleanup;
    }

    for (i = 0 ; i < num_elem; i++) {
        ret = mbedtls_cipher_cmac_update(&ctx, addr[i], len[i]);
        if (ret != 0) {
            goto cleanup;
        }
    }

    ret = mbedtls_cipher_cmac_finish(&ctx, mac);
cleanup:
    mbedtls_cipher_free(&ctx);
    return(ret);
}

int omac1_aes_128_vector(const u8 *key, size_t num_elem,
             const u8 *addr[], const size_t *len, u8 *mac)
{
    return omac1_aes_vector(key, 16, num_elem, addr, len, mac);
}

int omac1_aes_128(const u8 *key, const u8 *data, size_t data_len, u8 *mac)
{
    return omac1_aes_128_vector(key, 1, &data, &data_len, mac);
}

static int hmac_vector(mbedtls_md_type_t md_type,
               const u8 *key, size_t key_len,
               size_t num_elem, const u8 *addr[],
               const size_t *len, u8 *mac)
{
    size_t i;
    const mbedtls_md_info_t *md_info;
    mbedtls_md_context_t md_ctx;
    int ret;

    #ifdef HX_SDK
        // clear watchdog timer
        int wdt_clear(void);
        wdt_clear();
    #endif

    mbedtls_md_init(&md_ctx);

    md_info = mbedtls_md_info_from_type(md_type);
    if (!md_info) {
        return -1;
    }

    ret = mbedtls_md_setup(&md_ctx, md_info, 1);
    if (ret != 0) {
        return(ret);
    }

    ret = mbedtls_md_hmac_starts(&md_ctx, key, key_len);
    if (ret != 0) {
        return(ret);
    }

    for (i = 0; i < num_elem; i++) {
        ret = mbedtls_md_hmac_update(&md_ctx, addr[i], len[i]);
        if (ret != 0) {
            return(ret);
        }

    }

    ret = mbedtls_md_hmac_finish(&md_ctx, mac);

    mbedtls_md_free(&md_ctx);

    return ret;
}

int hmac_sha256_vector(const u8 *key, size_t key_len, size_t num_elem,
               const u8 *addr[], const size_t *len, u8 *mac)
{
    return hmac_vector(MBEDTLS_MD_SHA256, key, key_len, num_elem, addr,
               len, mac);
}

int hmac_sha256(const u8 *key, size_t key_len, const u8 *data,
        size_t data_len, u8 *mac)
{
    return hmac_sha256_vector(key, key_len, 1, &data, &data_len, mac);
}

int hmac_sha1_vector(const u8 *key, size_t key_len, size_t num_elem,
             const u8 *addr[], const size_t *len, u8 *mac)
{
    return hmac_vector(MBEDTLS_MD_SHA1, key, key_len, num_elem, addr,
               len, mac);
}

int asr_hmac_sha1(const u8 *data, size_t data_len, const u8 *key,
                size_t key_len, u8 *mac)
{
    return hmac_sha1_vector(key, key_len, 1, &data, &data_len, mac);
}

int hmac_md5_vector(const u8 *key, size_t key_len, size_t num_elem,
            const u8 *addr[], const size_t *len, u8 *mac)
{
    return hmac_vector(MBEDTLS_MD_MD5, key, key_len,
               num_elem, addr, len, mac);
}

int asr_hmac_md5(const u8 *data, size_t data_len, const u8 *key, size_t key_len,
         u8 *mac)
{
    return hmac_md5_vector(key, key_len, 1, &data, &data_len, mac);
}

static void *aes_crypt_init(int mode, const u8 *key, size_t len)
{
    int ret = -1;
    mbedtls_aes_context *aes = os_malloc(sizeof(*aes));
    if (!aes) {
        return NULL;
    }
    mbedtls_aes_init(aes);

    if (mode == MBEDTLS_AES_ENCRYPT) {
        ret = mbedtls_aes_setkey_enc(aes, key, len);
    } else if (mode == MBEDTLS_AES_DECRYPT){
        ret = mbedtls_aes_setkey_dec(aes, key, len);
    }
    if (ret < 0) {
        mbedtls_aes_free(aes);
        os_free(aes);
        wpa_printf(MSG_ERROR, "%s: mbedtls_aes_setkey_enc/mbedtls_aes_setkey_dec failed", __func__);
        return NULL;
    }

    return (void *) aes;
}

static int aes_crypt(void *ctx, int mode, const u8 *in, u8 *out)
{
    return mbedtls_aes_crypt_ecb((mbedtls_aes_context *)ctx,
                     mode, in, out);
}

static void aes_crypt_deinit(void *ctx)
{
    mbedtls_aes_free((mbedtls_aes_context *)ctx);
    os_free(ctx);
}

void *aes_encrypt_init(const u8 *key, size_t len)
{
    return aes_crypt_init(MBEDTLS_AES_ENCRYPT, key, len);
}

int aes_encrypt(void *ctx, const u8 *plain, u8 *crypt)
{
    return aes_crypt(ctx, MBEDTLS_AES_ENCRYPT, plain, crypt);
}

void aes_encrypt_deinit(void *ctx)
{
    return aes_crypt_deinit(ctx);
}

void * aes_decrypt_init(const u8 *key, size_t len)
{
    return aes_crypt_init(MBEDTLS_AES_DECRYPT, key, len);
}

int aes_decrypt(void *ctx, const u8 *crypt, u8 *plain)
{
    return aes_crypt(ctx, MBEDTLS_AES_DECRYPT, crypt, plain);
}

void aes_decrypt_deinit(void *ctx)
{
    return aes_crypt_deinit(ctx);
}

void asr_aes_wrap(uint8_t * plain, int32_t plain_len,
                      uint8_t * iv,    int32_t iv_len,
                    uint8_t * kek,    int32_t kek_len,
                      uint8_t *cipher, uint16_t *cipher_len)
{
    int        i, j, k, nblock = plain_len/AES_BLOCKSIZE8;
    uint8_t    R[32][AES_BLOCKSIZE8], A[AES_BLOCKSIZE8], xor[AES_BLOCKSIZE8];
    aes_block   m,x;
    void *ctx;

    ctx = aes_encrypt_init(kek, 128);
    if (ctx == NULL)
        return;

    //Initialize Variable
    memcpy(A, iv, AES_BLOCKSIZE8);
    for(i = 0; i < nblock ; i++)
        memcpy(&R[i], plain + i*AES_BLOCKSIZE8, AES_BLOCKSIZE8);

    //Calculate Intermediate Values

    for(j = 0 ; j < 6 ; j++ )
        for (i = 0 ; i < nblock ; i++)
        {
            memcpy(&m.b, A, AES_BLOCKSIZE8);
            memcpy((&m.b[0]) + AES_BLOCKSIZE8, &(R[i]), AES_BLOCKSIZE8);
            // => B = AES(K, A|R[i])

            aes_encrypt(ctx, m.b, x.b);

            // => A = MSB(64,B) ^t  where t = (n*j) + i
            memset(xor, 0, sizeof xor);
            xor[7] |= ((nblock * j) + i + 1);
            for(k = 0 ; k < 8 ; k++)
                A[k] = x.b[k] ^ xor[k];
            // => R[i] = LSB(64,B)
            for(k = 0 ; k < 8 ; k++)
                R[i][k] = x.b[k + AES_BLOCKSIZE8];

        }

    //Output the result
    memcpy(cipher, A, AES_BLOCKSIZE8);
    for(i = 0; i<nblock ; i++)
        memcpy(cipher + (i+1)*AES_BLOCKSIZE8, &R[i], AES_BLOCKSIZE8);
    *cipher_len = plain_len + AES_BLOCKSIZE8;

    aes_encrypt_deinit(ctx);
}

void asr_aes_unwrap(uint8_t * cipher, int32_t cipher_len,
                uint8_t * kek,    int32_t kek_len,
                uint8_t * plain)
{
    int            i, j, k, nblock = (cipher_len/AES_BLOCKSIZE8) - 1;
    uint8_t    R[32][AES_BLOCKSIZE8], A[AES_BLOCKSIZE8], xor[AES_BLOCKSIZE8];
    aes_block   m,x;
    void *ctx;

    /* input of asr_aes_unwrap kek_len is in the unit of bute (16 bytes)*/
    /* input of aes_set_key is in the unit of bit (128 bits) */
    ctx = aes_decrypt_init(kek, 128);
    if (ctx == NULL)
        return;

    //Initialize Variable
    memcpy(A, cipher, AES_BLOCKSIZE8);
    for(i = 0; i < nblock ; i++)
        memcpy(&R[i], cipher + (i+1)*AES_BLOCKSIZE8, AES_BLOCKSIZE8);

    //Compute internediate Value
    for(j=5 ; j>=0 ; j--)
    {
        for(i= nblock-1    ; i>=0 ; i--)
        {
            // => B = AES-1((A^t) |R[i])

            memset(xor, 0, sizeof xor);
            xor[7] |= ((nblock * j) + i + 1);
            for(k = 0 ; k < 8 ; k++)
                x.b[k] = A[k] ^ xor[k];
            memcpy((&x.b[0]) + AES_BLOCKSIZE8, &(R[i]), AES_BLOCKSIZE8);

            aes_decrypt(ctx, x.b,m.b);

            memcpy(A, &m.b[0], AES_BLOCKSIZE8);
            //for(k=0 ; k<AES_BLOCKSIZE8 ; k++)
            //    A[k] = m.b[k];
            for(k=0 ; k<AES_BLOCKSIZE8 ; k++)
                R[i][k] = m.b[k + AES_BLOCKSIZE8];

        }
    }
    memcpy(plain, A, AES_BLOCKSIZE8);
    for(i = 0; i < nblock ; i++)
        memcpy(plain + (i+1)*AES_BLOCKSIZE8, &R[i],  AES_BLOCKSIZE8);

    aes_decrypt_deinit(ctx);
}
