/**************************************************************************************************************
 * altobeam RTOS WSM host interface (HI) implementation
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/


#ifndef _ATBM_SHA1_H
#define _ATBM_SHA1_H

#define SHA1_MAC_LEN 20

#define SHA1HANDSOFF
#if 0
typedef unsigned char atbm_uint8;
typedef unsigned int atbm_uint32;
typedef unsigned int atbm_size_t;
#endif
struct SHA1Context {
	atbm_uint32 state[5];
	atbm_uint32 count[2];
	unsigned char buffer[64];
};
typedef struct SHA1Context SHA1_CTX;

#define MD5_MAC_LEN 16

#define PROTOTYPES 0
typedef unsigned char *POINTER;

/* UINT2 defines a two byte word */
typedef atbm_uint16 UINT2;

/* UINT4 defines a four byte word */
typedef atbm_uint32 UINT4;

#define PROTO_LIST(list) ()
#define ATBM_S11 7
#define ATBM_S12 12
#define ATBM_S13 17
#define ATBM_S14 22
#define ATBM_S21 5
#define ATBM_S22 9
#define ATBM_S23 14
#define ATBM_S24 20
#define ATBM_S31 4
#define ATBM_S32 11
#define ATBM_S33 16
#define ATBM_S34 23
#define ATBM_S41 6
#define ATBM_S42 10
#define ATBM_S43 15
#define ATBM_S44 21

#if 1 
typedef struct {
UINT4 state[4]; /* state (ABCD) */
UINT4 count[2]; /* number of bits, modulo 2^64 (lsb first) */
unsigned char buffer[64]; /* input buffer */
} MD5_CTX;
#endif

typedef struct{  
	atbm_uint32 total[2];  
	atbm_uint32 state[5];  
	atbm_uint8 buffer[64];
}sha1_context;

#define GET_UINT32(n,b,i)       \
{                               \
	(n) = ( (atbm_uint32) (b)[(i)] << 24 )     \
	| ( (atbm_uint32) (b)[(i) + 1] << 16 )     \
	| ( (atbm_uint32) (b)[(i) + 2] << 8 )     \
	| ( (atbm_uint32) (b)[(i) + 3]     );     \
}

#define PUT_UINT32(n,b,i)               \
{                               \
	(b)[(i)] = (atbm_uint8) ( (n) >> 24 );     \
	(b)[(i) + 1] = (atbm_uint8) ( (n) >> 16 );     \
	(b)[(i) + 2] = (atbm_uint8) ( (n) >> 8 );     \
	(b)[(i) + 3] = (atbm_uint8) ( (n)     );     \
}


#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

/* blk0() and blk() perform the initial expand. */
/* I got the idea of expanding during the round function from SSLeay */
#define blk0(i) block->l[i]

#define blk(i) (block->l[i & 15] = rol(block->l[(i + 13) & 15] ^ \
	block->l[(i + 8) & 15] ^ block->l[(i + 2) & 15] ^ block->l[i & 15], 1))

/* (R0+R1), R2, R3, R4 are the different operations used in SHA1 */
#define R0(v,w,x,y,z,i) \
	z += ((w & (x ^ y)) ^ y) + blk0(i) + 0x5A827999 + rol(v, 5); \
	w = rol(w, 30);
#define R1(v,w,x,y,z,i) \
	z += ((w & (x ^ y)) ^ y) + blk(i) + 0x5A827999 + rol(v, 5); \
	w = rol(w, 30);
#define R2(v,w,x,y,z,i) \
	z += (w ^ x ^ y) + blk(i) + 0x6ED9EBA1 + rol(v, 5); w = rol(w, 30);
#define R3(v,w,x,y,z,i) \
	z += (((w | x) & y) | (w & x)) + blk(i) + 0x8F1BBCDC + rol(v, 5); \
	w = rol(w, 30);
#define R4(v,w,x,y,z,i) \
	z += (w ^ x ^ y) + blk(i) + 0xCA62C1D6 + rol(v, 5); \
	w=rol(w, 30);

int atbm_pbkdf2_sha1(const char *passphrase, const char *ssid, atbm_size_t ssid_len,
		int iterations, atbm_uint8 *buf, atbm_size_t buflen);

int atbmwifi_md5_vector(atbm_size_t num_elem, const atbm_uint8 *addr[], const atbm_size_t *len, atbm_uint8 *mac);

int atbm_sha1_prf(const atbm_uint8 *key, atbm_size_t key_len, const char *label,
	     const atbm_uint8 *data, atbm_size_t data_len, atbm_uint8 *buf, atbm_size_t buf_len);


int atbm_hmac_sha1(const atbm_uint8 *key, atbm_size_t key_len, const atbm_uint8 *data, atbm_size_t data_len,
	       atbm_uint8 *mac);

#endif //_ATBM_SHA1_H