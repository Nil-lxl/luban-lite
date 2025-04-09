#ifndef WPAS_AES_H
#define WPAS_AES_H

typedef struct
{
    uint32_t erk[64];        /* encryption round keys */
    uint32_t drk[64];        /* decryption round keys */
    int32_t nr;           /* number of rounds */
}aes_context;

#define AES_BLOCKSIZE8  8
#define AES_BLK_SIZE    16  // # octets in an AES block
typedef union _aes_block    // AES cipher block
{
    uint32_t  x[AES_BLK_SIZE/4];  // access as 8-bit octets or 32-bit words
    uint8_t  b[AES_BLK_SIZE];
}aes_block;

void asr_aes_wrap(uint8_t * plain, int32_t plain_len,
                    uint8_t * iv, int32_t iv_len,
                    uint8_t * kek, int32_t kek_len,
                    uint8_t *cipher, uint16_t *cipher_len);

void asr_aes_unwrap(uint8_t * cipher, int32_t cipher_len,
                    uint8_t * kek, int32_t kek_len,
                    uint8_t * plain);

#endif //WPAS_AES_H