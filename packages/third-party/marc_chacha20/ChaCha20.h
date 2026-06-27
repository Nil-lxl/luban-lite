/*
 *  This file is part of the ChaCha20 library (https://github.com/marcizhu/ChaCha20)
 *
 *  Copyright (C) 2022 Marc Izquierdo
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __CHACHA20_H__
#define __CHACHA20_H__

/******************************************************************************
 *                                   HEADER                                   *
 ******************************************************************************/

#include <stdint.h>
#include <stddef.h>

/** @brief Alias for ChaCha20 key type */
typedef uint8_t key256_t[32];

/** @brief Alias for ChaCha20 nonce type */
typedef uint8_t nonce96_t[12];

/** @brief ChaCha20 context */
typedef struct
{
	uint32_t state[4*4];
	uint32_t keystream[4*4];
	uint32_t idx;
} ChaCha20_Ctx;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the ChaCha20 Context
 *
 * Initialize the ChaCha20 context with the given 256-bit key, nonce and block
 * count. The block count can be safely set to 0.
 *
 * @param ctx    Pointer to ChaCha20 context
 * @param key    256-bit key
 * @param nonce  96-bit nonce
 * @param count  32-bit block count
 */
void ChaCha20_init(ChaCha20_Ctx* ctx, const key256_t key, const nonce96_t nonce, uint32_t count);

/**
 * @brief XOR a given buffer
 *
 * Encrypts/decrypts a given buffer, automatically incrementing the block count
 * if necessary. It is possible to encript/decrypt a document using multiple
 * calls to this function, but in such case it is required that all but the last
 * call use a buffer length that is integer multiple of 64 bytes (e.g. 256 or
 * 65536 bytes).
 *
 * In ChaCha20, encryption and decryption are the same opperation since it uses
 * an XOR between the given buffer and the key stream. Thus, you can use this
 * function both for encryption and decryption.
 *
 * @pre The context must be initialized prior to this call, and the buffer must
 *      not be @code{c} NULL @endcode.
 *
 * @param ctx      Pointer to ChaCha20 context
 * @param buffer   Pointer to buffer
 * @param bufflen  Length of the buffer
 */
void ChaCha20_xor(ChaCha20_Ctx* ctx, uint8_t* buffer, size_t bufflen);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __CHACHA20_H__
