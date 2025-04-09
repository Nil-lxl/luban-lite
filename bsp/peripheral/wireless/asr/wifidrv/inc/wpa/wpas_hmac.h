#ifndef WPAS_HMAC_H
#define WPAS_HMAC_H

/* The SHS block size and message digest sizes, in bytes */

#define SHS_DATASIZE    64
#define SHS_DIGESTSIZE  20

#ifndef SHA_DIGESTSIZE
#define SHA_DIGESTSIZE  20
#endif

#ifndef SHA_BLOCKSIZE
#define SHA_BLOCKSIZE   64
#endif

#ifndef MD5_DIGESTSIZE
#define MD5_DIGESTSIZE  16
#endif

#ifndef MD5_BLOCKSIZE
#define MD5_BLOCKSIZE   64
#endif

#define SHA1HashSize 20

#ifndef TRUE
  #define FALSE 0
  #define TRUE  1
#endif

typedef uint8_t BYTE;

typedef struct
{
    uint32_t digest[ 5 ];            /* Message digest */
    uint32_t countLo, countHi;       /* 64-bit bit count */
    uint32_t data[ 16 ];             /* SHS data buffer */
    int32_t Endianness;
} SHA_CTX;

//typedef uint16_t    int_least16_t;
/*
 *  This structure will hold context information for the SHA-1
 *  hashing operation
 */
typedef struct SHA1Context
{
    uint32_t Intermediate_Hash[SHA1HashSize/4]; /* Message Digest  */

    uint32_t Length_Low;            /* Message length in bits      */
    uint32_t Length_High;           /* Message length in bits      */

                               /* Index into message block array   */
    uint16_t Message_Block_Index;
    uint8_t Message_Block[64];      /* 512-bit message blocks      */

    int32_t Computed;               /* Is the digest computed?         */
    int32_t Corrupted;             /* Is the message digest corrupted? */
} SHA1Context;

#ifndef _SHA_enum_
#define _SHA_enum_
enum
{
    shaSuccess = 0,
    shaNull,            /* Null pointer parameter */
    shaInputTooLong,    /* input data too long */
    shaStateError       /* called Input after Result */
};
#endif


void SHAInit(SHA_CTX *);
void SHAUpdate(SHA_CTX *, BYTE *buffer, int32_t count);
void SHAFinal(BYTE *output, SHA_CTX *);

int32_t SHA1Reset(  SHA1Context *);
int32_t SHA1Input(  SHA1Context *, const uint8_t *, uint32_t);
int32_t SHA1Result( SHA1Context *, uint8_t Message_Digest[SHA1HashSize]);

void SHA1PadMessage(SHA1Context *);
void SHA1ProcessMessageBlock(SHA1Context *);

void hmac_sha(
    uint8_t*  k,     /* secret key */
    int32_t   lk,    /* length of the key in bytes */
    uint8_t*  d,     /* data */
    int32_t   ld,    /* length of data in bytes */
    uint8_t*  out,   /* output buffer, at least "t" bytes */
    int32_t   t
    );

void asr_hmac_sha1(uint8_t *text, int32_t text_len, uint8_t *key,
                int32_t key_len, uint8_t *digest);

void asr_hmac_md5(uint8_t *text, int32_t text_len, uint8_t *key,
                int32_t key_len, void * digest);

#endif //WPAS_HMAC_H