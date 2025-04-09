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

#ifndef ASR_CRYPTO_H
#define ASR_CRYPTO_H

#include "common.h"

/**
 * aes_encrypt_init - Initialize AES for encryption
 * @key: Encryption key
 * @len: Key length in bytes (usually 16, i.e., 128 bits)
 * Returns: Pointer to context data or %NULL on failure
 */
void * aes_encrypt_init(const u8 *key, size_t len);

/**
 * aes_encrypt - Encrypt one AES block
 * @ctx: Context pointer from aes_encrypt_init()
 * @plain: Plaintext data to be encrypted (16 bytes)
 * @crypt: Buffer for the encrypted data (16 bytes)
 * Returns: 0 on success, -1 on failure
 */
int aes_encrypt_wpa3(void *ctx, const u8 *plain, u8 *crypt);

/**
 * aes_encrypt_deinit - Deinitialize AES encryption
 * @ctx: Context pointer from aes_encrypt_init()
 */
void aes_encrypt_deinit(void *ctx);

/**
 * aes_decrypt_init - Initialize AES for decryption
 * @key: Decryption key
 * @len: Key length in bytes (usually 16, i.e., 128 bits)
 * Returns: Pointer to context data or %NULL on failure
 */
void * aes_decrypt_init(const u8 *key, size_t len);

/**
 * aes_decrypt - Decrypt one AES block
 * @ctx: Context pointer from aes_encrypt_init()
 * @crypt: Encrypted data (16 bytes)
 * @plain: Buffer for the decrypted data (16 bytes)
 * Returns: 0 on success, -1 on failure
 */
int aes_decrypt(void *ctx, const u8 *crypt, u8 *plain);

/**
 * aes_decrypt_deinit - Deinitialize AES decryption
 * @ctx: Context pointer from aes_encrypt_init()
 */
void aes_decrypt_deinit(void *ctx);

/**
 * struct asr_bignum - bignum
 *
 * Internal data structure for bignum implementation. The contents is specific
 * to the used crypto library.
 */
struct asr_bignum;

/**
 * asr_bignum_init - Allocate memory for bignum
 * Returns: Pointer to allocated bignum or %NULL on failure
 */
struct asr_bignum * asr_bignum_init(void);

/**
 * asr_bignum_init_set - Allocate memory for bignum and set the value
 * @buf: Buffer with unsigned binary value
 * @len: Length of buf in octets
 * Returns: Pointer to allocated bignum or %NULL on failure
 */
struct asr_bignum * asr_bignum_init_set(const u8 *buf, size_t len);
struct asr_bignum * asr_bignum_init_uint(unsigned int val);

/**
 * asr_bignum_deinit - Free bignum
 * @n: Bignum from asr_bignum_init() or asr_bignum_init_set()
 * @clear: Whether to clear the value from memory
 */
void asr_bignum_deinit(struct asr_bignum *n, int clear);

/**
 * asr_bignum_to_bin - Set binary buffer to unsigned bignum
 * @a: Bignum
 * @buf: Buffer for the binary number
 * @len: Length of @buf in octets
 * @padlen: Length in octets to pad the result to or 0 to indicate no padding
 * Returns: Number of octets written on success, -1 on failure
 */
int asr_bignum_to_bin(const struct asr_bignum *a,
             uint8_t *buf, size_t buflen, size_t padlen);

/**
 * asr_bignum_rand - Create a random number in range of modulus
 * @r: Bignum; set to a random value
 * @m: Bignum; modulus
 * Returns: 0 on success, -1 on failure
 */
int asr_bignum_rand(struct asr_bignum *r, const struct asr_bignum *m);

/**
 * asr_bignum_add - c = a + b
 * @a: Bignum
 * @b: Bignum
 * @c: Bignum; used to store the result of a + b
 * Returns: 0 on success, -1 on failure
 */
int asr_bignum_add(const struct asr_bignum *a,
              const struct asr_bignum *b,
              struct asr_bignum *c);

/**
 * asr_bignum_mod - c = a % b
 * @a: Bignum
 * @b: Bignum
 * @c: Bignum; used to store the result of a % b
 * Returns: 0 on success, -1 on failure
 */
int asr_bignum_mod(const struct asr_bignum *a,
              const struct asr_bignum *b,
              struct asr_bignum *c);

/**
 * asr_bignum_exptmod - Modular exponentiation: x = a^e (mod n)
 * @a: Bignum; base
 * @e: Bignum; exponent
 * @n: Bignum; modulus
 * @x: Bignum; used to store the result of a^e (mod n)
 * Returns: 0 on success, -1 on failure
 */
int asr_bignum_exptmod(const struct asr_bignum *a,
              const struct asr_bignum *e,
              const struct asr_bignum *n,
              struct asr_bignum *x);

/**
 * asr_bignum_inverse - Inverse a bignum so that a * c = 1 (mod b)
 * @a: Bignum
 * @b: Bignum
 * @c: Bignum; used to store the result
 * Returns: 0 on success, -1 on failure
 */
int asr_bignum_inverse(const struct asr_bignum *a,
              const struct asr_bignum *b,
              struct asr_bignum *c);

/**
 * asr_bignum_sub - c = a - b
 * @a: Bignum
 * @b: Bignum
 * @c: Bignum; used to store the result of a - b
 * Returns: 0 on success, -1 on failure
 */
int asr_bignum_sub(const struct asr_bignum *a,
              const struct asr_bignum *b,
              struct asr_bignum *c);

/**
 * asr_bignum_div - c = a / b
 * @a: Bignum
 * @b: Bignum
 * @c: Bignum; used to store the result of a / b
 * Returns: 0 on success, -1 on failure
 */
int asr_bignum_div(const struct asr_bignum *a,
              const struct asr_bignum *b,
              struct asr_bignum *c);

int asr_bignum_addmod(const struct asr_bignum *a,
             const struct asr_bignum *b,
             const struct asr_bignum *c,
             struct asr_bignum *d);

/**
 * asr_bignum_mulmod - d = a * b (mod c)
 * @a: Bignum
 * @b: Bignum
 * @c: Bignum
 * @d: Bignum; used to store the result of (a * b) % c
 * Returns: 0 on success, -1 on failure
 */
int asr_bignum_mulmod(const struct asr_bignum *a,
             const struct asr_bignum *b,
             const struct asr_bignum *c,
             struct asr_bignum *d);

int asr_bignum_sqrmod(const struct asr_bignum *a,
             const struct asr_bignum *b,
             struct asr_bignum *c);

/**
 * asr_bignum_rshift - r = a >> n
 * @a: Bignum
 * @n: Number of bits
 * @r: Bignum; used to store the result of a >> n
 * Returns: 0 on success, -1 on failure
 */
int asr_bignum_rshift(const struct asr_bignum *a, int n,
             struct asr_bignum *r);

/**
 * asr_bignum_cmp - Compare two bignums
 * @a: Bignum
 * @b: Bignum
 * Returns: -1 if a < b, 0 if a == b, or 1 if a > b
 */
int asr_bignum_cmp(const struct asr_bignum *a,
              const struct asr_bignum *b);

/**
 * asr_bignum_is_zero - Is the given bignum zero
 * @a: Bignum
 * Returns: 1 if @a is zero or 0 if not
 */
int asr_bignum_is_zero(const struct asr_bignum *a);

/**
 * asr_bignum_is_one - Is the given bignum one
 * @a: Bignum
 * Returns: 1 if @a is one or 0 if not
 */
int asr_bignum_is_one(const struct asr_bignum *a);

/**
 * asr_bignum_is_odd - Is the given bignum odd
 * @a: Bignum
 * Returns: 1 if @a is odd or 0 if not
 */
int asr_bignum_is_odd(const struct asr_bignum *a);

/**
 * asr_bignum_legendre - Compute the Legendre symbol (a/p)
 * @a: Bignum
 * @p: Bignum
 * Returns: Legendre symbol -1,0,1 on success; -2 on calculation failure
 */
int asr_bignum_legendre(const struct asr_bignum *a,
               const struct asr_bignum *p);

/**
 * struct asr_ec - Elliptic curve context
 *
 * Internal data structure for EC implementation. The contents is specific
 * to the used crypto library.
 */
struct asr_ec;

/**
 * asr_ec_init - Initialize elliptic curve context
 * @group: Identifying number for the ECC group (IANA "Group Description"
 *    attribute registrty for RFC 2409)
 * Returns: Pointer to EC context or %NULL on failure
 */
struct asr_ec * asr_ec_init(int group);

/**
 * asr_ec_deinit - Deinitialize elliptic curve context
 * @e: EC context from asr_ec_init()
 */
void asr_ec_deinit(struct asr_ec *e);

/**
 * asr_ec_prime_len - Get length of the prime in octets
 * @e: EC context from asr_ec_init()
 * Returns: Length of the prime defining the group
 */
size_t asr_ec_prime_len(struct asr_ec *e);

/**
 * asr_ec_prime_len_bits - Get length of the prime in bits
 * @e: EC context from asr_ec_init()
 * Returns: Length of the prime defining the group in bits
 */
size_t asr_ec_prime_len_bits(struct asr_ec *e);

/**
 * asr_ec_order_len - Get length of the order in octets
 * @e: EC context from asr_ec_init()
 * Returns: Length of the order defining the group
 */
size_t asr_ec_order_len(struct asr_ec *e);

/**
 * asr_ec_get_prime - Get prime defining an EC group
 * @e: EC context from asr_ec_init()
 * Returns: Prime (bignum) defining the group
 */
const struct asr_bignum * asr_ec_get_prime(struct asr_ec *e);

/**
 * asr_ec_get_order - Get order of an EC group
 * @e: EC context from asr_ec_init()
 * Returns: Order (bignum) of the group
 */
const struct asr_bignum * asr_ec_get_order(struct asr_ec *e);
const struct asr_bignum * asr_ec_get_a(struct asr_ec *e);
const struct asr_bignum * asr_ec_get_b(struct asr_ec *e);

/**
 * struct asr_ec_point - Elliptic curve point
 *
 * Internal data structure for EC implementation to represent a point. The
 * contents is specific to the used crypto library.
 */
struct asr_ec_point;

/**
 * asr_ec_point_init - Initialize data for an EC point
 * @e: EC context from asr_ec_init()
 * Returns: Pointer to EC point data or %NULL on failure
 */
struct asr_ec_point * asr_ec_point_init(struct asr_ec *e);

/**
 * asr_ec_point_deinit - Deinitialize EC point data
 * @p: EC point data from asr_ec_point_init()
 * @clear: Whether to clear the EC point value from memory
 */
void asr_ec_point_deinit(struct asr_ec_point *p, int clear);

/**
 * asr_ec_point_x - Copies the x-ordinate point into big number
 * @e: EC context from asr_ec_init()
 * @p: EC point data
 * @x: Big number to set to the copy of x-ordinate
 * Returns: 0 on success, -1 on failure
 */
int asr_ec_point_x(struct asr_ec *e, const struct asr_ec_point *p,
              struct asr_bignum *x);

/**
 * asr_ec_point_to_bin - Write EC point value as binary data
 * @e: EC context from asr_ec_init()
 * @p: EC point data from asr_ec_point_init()
 * @x: Buffer for writing the binary data for x coordinate or %NULL if not used
 * @y: Buffer for writing the binary data for y coordinate or %NULL if not used
 * Returns: 0 on success, -1 on failure
 *
 * This function can be used to write an EC point as binary data in a format
 * that has the x and y coordinates in big endian byte order fields padded to
 * the length of the prime defining the group.
 */
int asr_ec_point_to_bin(struct asr_ec *e,
               const struct asr_ec_point *point, u8 *x, u8 *y);

/**
 * asr_ec_point_from_bin - Create EC point from binary data
 * @e: EC context from asr_ec_init()
 * @val: Binary data to read the EC point from
 * Returns: Pointer to EC point data or %NULL on failure
 *
 * This function readers x and y coordinates of the EC point from the provided
 * buffer assuming the values are in big endian byte order with fields padded to
 * the length of the prime defining the group.
 */
struct asr_ec_point * asr_ec_point_from_bin(struct asr_ec *e,
                          const u8 *val);

/**
 * asr_ec_point_add - c = a + b
 * @e: EC context from asr_ec_init()
 * @a: Bignum
 * @b: Bignum
 * @c: Bignum; used to store the result of a + b
 * Returns: 0 on success, -1 on failure
 */
int asr_ec_point_add(struct asr_ec *e, const struct asr_ec_point *a,
            const struct asr_ec_point *b,
            struct asr_ec_point *c);

/**
 * asr_ec_point_mul - res = b * p
 * @e: EC context from asr_ec_init()
 * @p: EC point
 * @b: Bignum
 * @res: EC point; used to store the result of b * p
 * Returns: 0 on success, -1 on failure
 */
int asr_ec_point_mul(struct asr_ec *e, const struct asr_ec_point *p,
            const struct asr_bignum *b,
            struct asr_ec_point *res);

/**
 * asr_ec_point_invert - Compute inverse of an EC point
 * @e: EC context from asr_ec_init()
 * @p: EC point to invert (and result of the operation)
 * Returns: 0 on success, -1 on failure
 */
int asr_ec_point_invert(struct asr_ec *e, struct asr_ec_point *p);

/**
 * asr_ec_point_solve_y_coord - Solve y coordinate for an x coordinate
 * @e: EC context from asr_ec_init()
 * @p: EC point to use for the returning the result
 * @x: x coordinate
 * @y_bit: y-bit (0 or 1) for selecting the y value to use
 * Returns: 0 on success, -1 on failure
 */
int asr_ec_point_solve_y_coord(struct asr_ec *e,
                  struct asr_ec_point *p,
                  const struct asr_bignum *x, int y_bit);

/**
 * asr_ec_point_compute_y_sqr - Compute y^2 = x^3 + ax + b
 * @e: EC context from asr_ec_init()
 * @x: x coordinate
 * Returns: y^2 on success, %NULL failure
 */
struct asr_bignum *
asr_ec_point_compute_y_sqr(struct asr_ec *e,
                  const struct asr_bignum *x);

/**
 * asr_ec_point_is_at_infinity - Check whether EC point is neutral element
 * @e: EC context from asr_ec_init()
 * @p: EC point
 * Returns: 1 if the specified EC point is the neutral element of the group or
 *    0 if not
 */
int asr_ec_point_is_at_infinity(struct asr_ec *e,
                   const struct asr_ec_point *p);

/**
 * asr_ec_point_is_on_curve - Check whether EC point is on curve
 * @e: EC context from asr_ec_init()
 * @p: EC point
 * Returns: 1 if the specified EC point is on the curve or 0 if not
 */
int asr_ec_point_is_on_curve(struct asr_ec *e,
                const struct asr_ec_point *p);

/**
 * asr_ec_point_cmp - Compare two EC points
 * @e: EC context from asr_ec_init()
 * @a: EC point
 * @b: EC point
 * Returns: 0 on equal, non-zero otherwise
 */
int asr_ec_point_cmp(const struct asr_ec *e,
            const struct asr_ec_point *a,
            const struct asr_ec_point *b);

int hmac_sha1_vector(const u8 *key, size_t key_len, size_t num_elem,
                     const u8 *addr[], const size_t *len, u8 *mac);

int asr_hmac_sha1(const u8 *data, size_t data_len, const u8 *key,
                size_t key_len, u8 *mac);

int asr_hmac_md5(const u8 *data, size_t data_len, const u8 *key, size_t key_len,
         u8 *mac);

void asr_aes_wrap(uint8_t * plain, int32_t plain_len,
                    uint8_t * iv, int32_t iv_len,
                    uint8_t * kek, int32_t kek_len,
                    uint8_t *cipher, uint16_t *cipher_len);

void asr_aes_unwrap(uint8_t * cipher, int32_t cipher_len,
                    uint8_t * kek, int32_t kek_len,
                    uint8_t * plain);

#endif /* ASR_CRYPTO_H */
