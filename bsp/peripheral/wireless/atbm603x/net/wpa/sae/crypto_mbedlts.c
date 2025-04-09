#include "atbm_hal.h"
#include "ecp.h"
#include "crypto.h"

struct crypto_bignum * crypto_bignum_init(void)
{
	atbm_mbedtls_mpi *mpi;

	mpi = atbm_kmalloc(sizeof(atbm_mbedtls_mpi), GFP_KERNEL);
	if(!mpi)
		return NULL;

	atbm_mbedtls_mpi_init(mpi);
	return (struct crypto_bignum *) mpi;
}


struct crypto_bignum * crypto_bignum_init_set(const atbm_uint8 *buf, atbm_size_t len)
{
	atbm_mbedtls_mpi *mpi;

	mpi = atbm_kmalloc(sizeof(atbm_mbedtls_mpi), GFP_KERNEL);
	if(!mpi)
		return NULL;
	atbm_mbedtls_mpi_init(mpi);
	if(atbm_mbedtls_mpi_read_binary(mpi, buf, len)){
		atbm_kfree(mpi);
		return NULL;
	}
	return (struct crypto_bignum *) mpi;
}


void crypto_bignum_deinit(struct crypto_bignum *n, int clear)
{
	if (!n)
		return;

	atbm_mbedtls_mpi_free((atbm_mbedtls_mpi *)n);
	if (clear)
		atbm_memset(n, 0, sizeof(atbm_mbedtls_mpi));

	atbm_kfree((atbm_mbedtls_mpi *)n);
}


int crypto_bignum_to_bin(const struct crypto_bignum *a,
			 atbm_uint8 *buf, atbm_size_t buflen, atbm_size_t padlen)
{
	int num_bytes, offset;

	if (padlen > buflen)
		return -1;

	num_bytes = atbm_mbedtls_mpi_size((atbm_mbedtls_mpi *) a);

	if ((atbm_size_t) num_bytes > buflen)
		return -1;
	if (padlen > (atbm_size_t) num_bytes)
		offset = padlen - num_bytes;
	else
		offset = 0;

	atbm_memset(buf, 0, offset);
	atbm_mbedtls_mpi_write_binary((atbm_mbedtls_mpi *) a, buf + offset, num_bytes);

	return num_bytes + offset;
}

extern int atbmwifi_os_get_random(unsigned char *buf, atbm_size_t len);
typedef int(*p_rng_t)(unsigned char *,atbm_size_t);
int os_get_random_f_rng(void *p_rng, unsigned char *buf, atbm_size_t len){
	p_rng_t func = p_rng;
	if(func)
		return func(buf, len);
	return -1;
}
int crypto_bignum_rand(struct crypto_bignum *r, const struct crypto_bignum *m)
{
	if(atbm_mbedtls_mpi_fill_random((atbm_mbedtls_mpi *)r, (atbm_mbedtls_mpi_bitlen((atbm_mbedtls_mpi *) m) + 7) / 8, os_get_random_f_rng, atbmwifi_os_get_random))
		return -1;
	return crypto_bignum_mod(r, m, r);
}


int crypto_bignum_add(const struct crypto_bignum *a,
		      const struct crypto_bignum *b,
		      struct crypto_bignum *r)
{
	return atbm_mbedtls_mpi_add_mpi((atbm_mbedtls_mpi *) r, (atbm_mbedtls_mpi *) a,
		      (atbm_mbedtls_mpi *) b);
}


int crypto_bignum_mod(const struct crypto_bignum *a,
		      const struct crypto_bignum *m,
		      struct crypto_bignum *r)
{
	return atbm_mbedtls_mpi_mod_mpi((atbm_mbedtls_mpi *) r, (atbm_mbedtls_mpi *) a,
		      (atbm_mbedtls_mpi *) m);
}


int crypto_bignum_exptmod(const struct crypto_bignum *b,
			  const struct crypto_bignum *e,
			  const struct crypto_bignum *m,
			  struct crypto_bignum *r)
{
	int ret;
	atbm_mbedtls_mpi ictx;
	atbm_mbedtls_mpi_init(&ictx);

	ret = atbm_mbedtls_mpi_exp_mod((atbm_mbedtls_mpi *) r, (atbm_mbedtls_mpi *) b, (atbm_mbedtls_mpi *) e,
			  (atbm_mbedtls_mpi *) m, &ictx);
	atbm_mbedtls_mpi_free(&ictx);
	return ret;
}


int crypto_bignum_inverse(const struct crypto_bignum *a,
			const struct crypto_bignum *b,
			struct crypto_bignum *c){
	return atbm_mbedtls_mpi_inv_mod((atbm_mbedtls_mpi *)c, (atbm_mbedtls_mpi *)a, (atbm_mbedtls_mpi *)b);
}

int crypto_bignum_sub(const struct crypto_bignum *a,
		      const struct crypto_bignum *b,
		      struct crypto_bignum *c)
{
	return atbm_mbedtls_mpi_sub_mpi((atbm_mbedtls_mpi *) c, (atbm_mbedtls_mpi *) a,
		      (atbm_mbedtls_mpi *) b);
}

int crypto_bignum_div(const struct crypto_bignum *a,
			const struct crypto_bignum *b,
			struct crypto_bignum *c)
{
	int ret;
	atbm_mbedtls_mpi ictx;
	atbm_mbedtls_mpi_init(&ictx);
	ret =  atbm_mbedtls_mpi_div_mpi((atbm_mbedtls_mpi *)b, (atbm_mbedtls_mpi *)c,
				(atbm_mbedtls_mpi *)a, &ictx);
	atbm_mbedtls_mpi_free(&ictx);
	return ret;
}

int crypto_bignum_mulmod(const struct crypto_bignum *a,
		   const struct crypto_bignum *b,
		   const struct crypto_bignum *c,
		   struct crypto_bignum *d){
	int ret;
	atbm_mbedtls_mpi tmp;
	atbm_mbedtls_mpi_init(&tmp);
	if(atbm_mbedtls_mpi_mul_mpi(&tmp, (atbm_mbedtls_mpi *)a, (atbm_mbedtls_mpi *)b) || 
		atbm_mbedtls_mpi_mod_mpi((atbm_mbedtls_mpi *)d, &tmp, (atbm_mbedtls_mpi *)c)){
		ret = -1;
		goto out;
	}
	ret = 0;
out:
	atbm_mbedtls_mpi_free(&tmp);
	return ret;
}

int crypto_bignum_cmp(const struct crypto_bignum *a,
			 const struct crypto_bignum *b){
	return atbm_mbedtls_mpi_cmp_mpi((atbm_mbedtls_mpi *)a, (atbm_mbedtls_mpi *)b);
}

int crypto_bignum_is_odd(const struct crypto_bignum *a){
	return atbm_mbedtls_mpi_get_bit((const atbm_mbedtls_mpi *) a, 0) == 1;
}


int crypto_bignum_is_zero(const struct crypto_bignum *a){
	return atbm_mbedtls_mpi_cmp_int((atbm_mbedtls_mpi *)a, 0) == 0;
}

int crypto_bignum_is_one(const struct crypto_bignum *a){
	return atbm_mbedtls_mpi_cmp_int((atbm_mbedtls_mpi *)a, 1) == 0;
}

int crypto_bignum_legendre(const struct crypto_bignum *a,
		  const struct crypto_bignum *p){
	atbm_mbedtls_mpi t1, ictx, t2, r;
	int ret = -2;

	atbm_mbedtls_mpi_init(&ictx);
	atbm_mbedtls_mpi_init(&t1);
	atbm_mbedtls_mpi_init(&t2);
	atbm_mbedtls_mpi_init(&r);

	if(atbm_mbedtls_mpi_sub_int(&t1, (atbm_mbedtls_mpi *)p, 1) ||
	 atbm_mbedtls_mpi_div_int(&t2, &r, &t1, 2) ||
	 atbm_mbedtls_mpi_exp_mod(&t1, (atbm_mbedtls_mpi *) a, &t2,
		   (atbm_mbedtls_mpi *) p, &ictx))
	 goto out;
	if(!atbm_mbedtls_mpi_cmp_int(&t1, 1)){
	 ret = 1;
	}else if(!atbm_mbedtls_mpi_cmp_int(&t1, 0)){
	 ret = 0;
	}else{
	 ret = -1;
	}

out:
 atbm_mbedtls_mpi_free(&ictx);
 atbm_mbedtls_mpi_free(&t1);
 atbm_mbedtls_mpi_free(&t2);
 return ret;
}

struct crypto_ec {
	mbedtls_ecp_group *group;
};

#define INC_MUL_COUNT

#define MOD_MUL( N )    do { MBEDTLS_MPI_CHK( ecp_modp( &N, e->group ) ); INC_MUL_COUNT } \
                        while( 0 )

/*
 * Reduce a atbm_mbedtls_mpi mod p in-place, to use after atbm_mbedtls_mpi_sub_mpi
 * N->s < 0 is a very fast test, which fails only if N is 0
 */
#define MOD_SUB( N )                                \
    while( N.s < 0 && atbm_mbedtls_mpi_cmp_int( &N, 0 ) != 0 )   \
        MBEDTLS_MPI_CHK( atbm_mbedtls_mpi_add_mpi( &N, &N, &e->group->P ) )

/*
 * Reduce a atbm_mbedtls_mpi mod p in-place, to use after atbm_mbedtls_mpi_add_mpi and atbm_mbedtls_mpi_mul_int.
 * We known P, N and the result are positive, so sub_abs is correct, and
 * a bit faster.
 */
#define MOD_ADD( N )                                \
    while( atbm_mbedtls_mpi_cmp_mpi( &N, &e->group->P ) >= 0 )        \
        MBEDTLS_MPI_CHK( atbm_mbedtls_mpi_sub_abs( &N, &N, &e->group->P ) )



struct crypto_ec * crypto_ec_init(int group)
{
	int built = 0;
	struct crypto_ec *e;
	int grp_id;
	char data[100];
	switch (group) {
	case 19:
		grp_id = MBEDTLS_ECP_DP_SECP256R1;
		break;
	case 20:
		grp_id = MBEDTLS_ECP_DP_SECP384R1;
		break;
	case 21:
		grp_id = MBEDTLS_ECP_DP_SECP521R1;
		break;
	case 25:
		grp_id = MBEDTLS_ECP_DP_SECP192R1;
		break;
	case 26:
		grp_id = MBEDTLS_ECP_DP_SECP224R1;
		break;
#ifdef NID_brainpoolP224r1
	case 27:
		grp_id = NID_brainpoolP224r1;
		break;
#endif /* NID_brainpoolP224r1 */
#ifdef NID_brainpoolP256r1
	case 28:
		grp_id = MBEDTLS_ECP_DP_BP256R1;
		break;
#endif /* NID_brainpoolP256r1 */
#ifdef NID_brainpoolP384r1
	case 29:
		grp_id = MBEDTLS_ECP_DP_BP384R1;
		break;
#endif /* NID_brainpoolP384r1 */
#ifdef NID_brainpoolP512r1
	case 30:
		grp_id = MBEDTLS_ECP_DP_BP512R1;
		break;
#endif /* NID_brainpoolP512r1 */
	default:
		return NULL;
	}
	e = atbm_kmalloc(sizeof(struct crypto_ec), GFP_KERNEL);
	if (!e)
		return NULL;
	e->group = atbm_kmalloc(sizeof(mbedtls_ecp_group), GFP_KERNEL);
	mbedtls_ecp_group_init(e->group);
	if (mbedtls_ecp_group_load(e->group, grp_id) != 0)
		goto done;
	built = 1;
	atbm_mbedtls_mpi_write_binary(&e->group->P, data, atbm_mbedtls_mpi_size(&e->group->P));
done:
	if (!built) {
		crypto_ec_deinit(e);
		e = NULL;
	}
	return e;
}


void crypto_ec_deinit(struct crypto_ec* e)
{
	if (!(e && e->group))
		return;

	atbm_kfree(e->group);
	atbm_kfree(e);
}


struct crypto_ec_point * crypto_ec_point_init(struct crypto_ec *e)
{
	mbedtls_ecp_point *pt;
	if (!e || !e->group)
		return NULL;

	pt = atbm_kmalloc(sizeof(mbedtls_ecp_point), GFP_KERNEL);
	if (!pt)
		return NULL;
	mbedtls_ecp_point_init(pt);
	atbm_mbedtls_mpi_lset(&((mbedtls_ecp_point *)pt)->Z, 1);
	return (struct crypto_ec_point *) pt;
}


atbm_size_t crypto_ec_prime_len(struct crypto_ec *e)
{
	return (e->group->pbits + 7)/8;
}


atbm_size_t crypto_ec_prime_len_bits(struct crypto_ec *e)
{
	return e->group->pbits;
}


atbm_size_t crypto_ec_order_len(struct crypto_ec *e)
{
	return atbm_mbedtls_mpi_size(&e->group->N);
}


const struct crypto_bignum * crypto_ec_get_prime(struct crypto_ec *e)
{
	return (const struct crypto_bignum *) &e->group->P;
}


const struct crypto_bignum * crypto_ec_get_order(struct crypto_ec *e)
{
	return (const struct crypto_bignum *) &e->group->N;
}


void crypto_ec_point_deinit(struct crypto_ec_point *p, int clear)
{
	mbedtls_ecp_point *point = (mbedtls_ecp_point *) p;

	if (!p)
		return;
	mbedtls_ecp_point_free(point);
	atbm_kfree(point);
}


int crypto_ec_point_x(struct crypto_ec *e, const struct crypto_ec_point *p,
		      struct crypto_bignum *x)
{
	return atbm_mbedtls_mpi_copy(&((mbedtls_ecp_point *)p)->X, (atbm_mbedtls_mpi *)x);
}


int crypto_ec_point_to_bin(struct crypto_ec *e,
			   const struct crypto_ec_point *point, atbm_uint8 *x, atbm_uint8 *y)
{
	mbedtls_ecp_point *p = (mbedtls_ecp_point *) point;

	if (x) {
		if (atbm_mbedtls_mpi_write_binary(&(p->X), x, crypto_ec_prime_len(e)))
			return -1;
	}

	if (y) {
		if (atbm_mbedtls_mpi_write_binary(&(p->Y), y, crypto_ec_prime_len(e)))
			return -1;
	}

	return 0;
}

struct crypto_ec_point * crypto_ec_point_from_bin(struct crypto_ec *e,
						  const atbm_uint8 *val)
{
	mbedtls_ecp_point *point = NULL;
	int loaded = 0;

	point = (mbedtls_ecp_point *)crypto_ec_point_init(e);
	if (!point)
		goto done;

	if(atbm_mbedtls_mpi_read_binary(&(point->X), val, crypto_ec_prime_len(e)))
		goto done;
	val += crypto_ec_prime_len(e);
	if(atbm_mbedtls_mpi_read_binary(&(point->Y), val, crypto_ec_prime_len(e)))
		goto done;
	atbm_mbedtls_mpi_lset(&((mbedtls_ecp_point *)point)->Z, 1);

	loaded = 1;
done:
	if (!loaded) {
		crypto_ec_point_deinit((struct crypto_ec_point *)point, 0);
		point = NULL;
	}
	return (struct crypto_ec_point *) point;
}


int crypto_ec_point_add(struct crypto_ec *e, const struct crypto_ec_point *a,
			const struct crypto_ec_point *b,
			struct crypto_ec_point *c)
{
	int ret;
	atbm_mbedtls_mpi mul;
	atbm_mbedtls_mpi_init(&mul);
	atbm_mbedtls_mpi_lset(&mul, 1);

	ret = mbedtls_ecp_muladd(e->group, (mbedtls_ecp_point *)c, &mul, (mbedtls_ecp_point *)a, &mul, (mbedtls_ecp_point *)b);

	atbm_mbedtls_mpi_free(&mul);
	return ret == 0 ? 0 : -1;
}


int crypto_ec_point_mul(struct crypto_ec *e, const struct crypto_ec_point *p,
			const struct crypto_bignum *b,
			struct crypto_ec_point *res)
{
	int ret = mbedtls_ecp_mul( e->group, (mbedtls_ecp_point *)res, (atbm_mbedtls_mpi *)b, (mbedtls_ecp_point *)p, NULL, NULL );
	return ret == 0 ? 0 : -1;
}


int crypto_ec_point_invert(struct crypto_ec *e, struct crypto_ec_point *p)
{
	int ret = ecp_safe_invert_jac(e->group, (mbedtls_ecp_point *)p, 1);
	return ret == 0 ? 0 : -1;
}

int crypto_ec_point_solve_y_coord(struct crypto_ec *e,
				  struct crypto_ec_point *p,
				  const struct crypto_bignum *x, int y_bit)
{
	int ret;
	atbm_mbedtls_mpi RSN1, RSN2, r, ictx;
	struct crypto_bignum *YY;

	atbm_mbedtls_mpi_init(&RSN1);
	atbm_mbedtls_mpi_init(&RSN2);
	atbm_mbedtls_mpi_init(&r);
	atbm_mbedtls_mpi_init(&ictx);

	YY = crypto_ec_point_compute_y_sqr(e, x);
	if(!YY)
		return -1;

	MBEDTLS_MPI_CHK( atbm_mbedtls_mpi_add_int( &RSN1, &e->group->P, 1 ) );
	//MBEDTLS_MPI_CHK( atbm_mbedtls_mpi_div_int( &RSN1, &r, &RSN1, 4) );
	MBEDTLS_MPI_CHK( atbm_mbedtls_mpi_shift_r(&RSN1, 2) );
	MBEDTLS_MPI_CHK(atbm_mbedtls_mpi_exp_mod( &RSN2, (atbm_mbedtls_mpi *)YY, &RSN1, &e->group->P, &ictx));

	if(atbm_mbedtls_mpi_get_bit(&RSN2, 0) == (y_bit ? 1 : 0)){
		atbm_mbedtls_mpi_copy(&((mbedtls_ecp_point *)p)->Y, &RSN2);
	}else{
		atbm_mbedtls_mpi_sub_mpi(&((mbedtls_ecp_point *)p)->Y, &e->group->P, &RSN2);
	}
	ret = 0;
cleanup:
	crypto_bignum_deinit(YY, 1);
	atbm_mbedtls_mpi_copy(&((mbedtls_ecp_point *)p)->X, (atbm_mbedtls_mpi*)x);
	atbm_mbedtls_mpi_free(&RSN1);
	atbm_mbedtls_mpi_free(&RSN2);
	atbm_mbedtls_mpi_free(&r);
	atbm_mbedtls_mpi_free(&ictx);
	return ret;
}


struct crypto_bignum *
crypto_ec_point_compute_y_sqr(struct crypto_ec *e,
			      const struct crypto_bignum *x)
{
	int ret;
	atbm_mbedtls_mpi *YY;

	/* pt coordinates must be normalized for our checks */
	if( atbm_mbedtls_mpi_cmp_int( (atbm_mbedtls_mpi *)x, 0 ) < 0 ||
		atbm_mbedtls_mpi_cmp_mpi( (atbm_mbedtls_mpi *)x, &e->group->P ) >= 0)
		return NULL;

	YY	= atbm_kmalloc(sizeof(atbm_mbedtls_mpi), GFP_KERNEL);
	if(!YY){
		return NULL;
	}
	atbm_mbedtls_mpi_init( YY );

	/*
	 * YY = Y^2
	 * RHS = X (X^2 + A) + B = X^3 + A X + B
	 */
	MBEDTLS_MPI_CHK( atbm_mbedtls_mpi_mul_mpi( YY, (atbm_mbedtls_mpi *)x, (atbm_mbedtls_mpi *)x));	MOD_MUL( (*YY) );

	/* Special case for A = -3 */
	if( e->group->A.p == NULL )
	{
		MBEDTLS_MPI_CHK( atbm_mbedtls_mpi_sub_int( YY, YY, 3 	  ) );	MOD_SUB( (*YY) );
	}
	else
	{
		MBEDTLS_MPI_CHK( atbm_mbedtls_mpi_add_mpi( YY, YY, &e->group->A ) );	MOD_ADD( (*YY) );
	}

	MBEDTLS_MPI_CHK( atbm_mbedtls_mpi_mul_mpi( YY, YY, (atbm_mbedtls_mpi *)x) );	MOD_MUL( (*YY) );
	MBEDTLS_MPI_CHK( atbm_mbedtls_mpi_add_mpi( YY, YY,	&e->group->B ));	MOD_ADD( (*YY) );

cleanup:
	if(ret != 0){
		atbm_kfree(YY);
		return NULL;
	}
	return (struct crypto_bignum *)YY;
}


int crypto_ec_point_is_at_infinity(struct crypto_ec *e,
				   const struct crypto_ec_point *p)
{
	return mbedtls_ecp_is_zero((mbedtls_ecp_point *) p);
}

int crypto_ec_point_is_on_curve(struct crypto_ec *e,
				const struct crypto_ec_point *p)
{
	return !mbedtls_ecp_check_pubkey(e->group, (mbedtls_ecp_point *)p);
}


int crypto_ec_point_cmp(const struct crypto_ec *e,
			const struct crypto_ec_point *a,
			const struct crypto_ec_point *b)
{
	return mbedtls_ecp_point_cmp((mbedtls_ecp_point *) a, (mbedtls_ecp_point *) b);
}

#if 0
struct crypto_ecdh {
	struct crypto_ec *ec;
};

struct crypto_ecdh * crypto_ecdh_init(int group)
{
	struct crypto_ecdh *ecdh = NULL;
	WC_RNG rng;
	int ret;

	if (wc_InitRng(&rng) != 0)
		goto fail;

	ecdh = os_zalloc(sizeof(*ecdh));
	if (!ecdh)
		goto fail;

	ecdh->ec = crypto_ec_init(group);
	if (!ecdh->ec)
		goto fail;

	ret = wc_ecc_make_key_ex(&rng, ecdh->ec->key.dp->size, &ecdh->ec->key,
				 ecdh->ec->key.dp->id);
	if (ret < 0)
		goto fail;

done:
	wc_FreeRng(&rng);

	return ecdh;
fail:
	crypto_ecdh_deinit(ecdh);
	ecdh = NULL;
	goto done;
}


void crypto_ecdh_deinit(struct crypto_ecdh *ecdh)
{
	if (ecdh) {
		crypto_ec_deinit(ecdh->ec);
		os_free(ecdh);
	}
}


struct wpabuf * crypto_ecdh_get_pubkey(struct crypto_ecdh *ecdh, int inc_y)
{
	struct wpabuf *buf = NULL;
	int ret;
	int len = ecdh->ec->key.dp->size;

	buf = wpabuf_alloc(inc_y ? 2 * len : len);
	if (!buf)
		goto fail;

	ret = crypto_bignum_to_bin((struct crypto_bignum *)
				   ecdh->ec->key.pubkey.x, wpabuf_put(buf, len),
				   len, len);
	if (ret < 0)
		goto fail;
	if (inc_y) {
		ret = crypto_bignum_to_bin((struct crypto_bignum *)
					   ecdh->ec->key.pubkey.y,
					   wpabuf_put(buf, len), len, len);
		if (ret < 0)
			goto fail;
	}

done:
	return buf;
fail:
	wpabuf_free(buf);
	buf = NULL;
	goto done;
}


struct wpabuf * crypto_ecdh_set_peerkey(struct crypto_ecdh *ecdh, int inc_y,
					const atbm_uint8 *key, atbm_size_t len)
{
	int ret;
	struct wpabuf *pubkey = NULL;
	struct wpabuf *secret = NULL;
	word32 key_len = ecdh->ec->key.dp->size;
	ecc_point *point = NULL;
	atbm_size_t need_key_len = inc_y ? 2 * key_len : key_len;

	if (len < need_key_len)
		goto fail;
	pubkey = wpabuf_alloc(1 + 2 * key_len);
	if (!pubkey)
		goto fail;
	wpabuf_put_u8(pubkey, inc_y ? ECC_POINT_UNCOMP : ECC_POINT_COMP_EVEN);
	wpabuf_put_data(pubkey, key, need_key_len);

	point = wc_ecc_new_point();
	if (!point)
		goto fail;

	ret = wc_ecc_import_point_der(wpabuf_mhead(pubkey), 1 + 2 * key_len,
				      ecdh->ec->key.idx, point);
	if (ret != MP_OKAY)
		goto fail;

	secret = wpabuf_alloc(key_len);
	if (!secret)
		goto fail;

	ret = wc_ecc_shared_secret_ex(&ecdh->ec->key, point,
				      wpabuf_put(secret, key_len), &key_len);
	if (ret != MP_OKAY)
		goto fail;

done:
	wc_ecc_del_point(point);
	wpabuf_free(pubkey);
	return secret;
fail:
	wpabuf_free(secret);
	secret = NULL;
	goto done;
}
#endif
