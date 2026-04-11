/*
 * copyright (c) 2006 Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * @ingroup lavu_mem
 * Memory handling functions
 */

#ifndef AVUTIL_MEM_H
#define AVUTIL_MEM_H

#include <limits.h>
#include <stdint.h>

#define DECLARE_ALIGNED(n,t,v)      t v
#define DECLARE_ASM_ALIGNED(n,t,v)  t v
#define DECLARE_ASM_CONST(n,t,v)    static const t v

static inline int av_size_mult(size_t a, size_t b, size_t *r)
{
    size_t t = a * b;
    /* Hack inspired from glibc: don't try the division if nelem and elsize
     * are both less than sqrt(SIZE_MAX). */
    if ((a | b) >= ((size_t)1 << (sizeof(size_t) * 4)) && a && t / a != b)
        return -1;
    *r = t;
    return 0;
}
/**
 * @defgroup lavu_mem_funcs Heap Management
 * Functions responsible for allocating, freeing, and copying memory.
 *
 * All memory allocation functions have a built-in upper limit of `INT_MAX`
 * bytes. This may be changed with av_max_alloc(), although exercise extreme
 * caution when doing so.
 *
 * @{
 */

/**
 * Allocate a memory block with alignment suitable for all memory accesses
 * (including vectors if available on the CPU).
 *
 * @param size Size in bytes for the memory block to be allocated
 * @return Pointer to the allocated block, or `NULL` if the block cannot
 *         be allocated
 * @see av_mallocz()
 */
void *av_malloc(size_t size);

/**
 * Allocate a memory block with alignment suitable for all memory accesses
 * (including vectors if available on the CPU) and zero all the bytes of the
 * block.
 *
 * @param size Size in bytes for the memory block to be allocated
 * @return Pointer to the allocated block, or `NULL` if it cannot be allocated
 * @see av_malloc()
 */
void *av_mallocz(size_t size);

/**
 * Allocate a memory block for an array with av_malloc().
 *
 * The allocated memory will have size `size * nmemb` bytes.
 *
 * @param nmemb Number of element
 * @param size  Size of a single element
 * @return Pointer to the allocated block, or `NULL` if the block cannot
 *         be allocated
 * @see av_malloc()
 */
void *av_malloc_array(size_t nmemb, size_t size);

/**
 * Allocate a memory block for an array with av_mallocz().
 *
 * The allocated memory will have size `size * nmemb` bytes.
 *
 * @param nmemb Number of elements
 * @param size  Size of the single element
 * @return Pointer to the allocated block, or `NULL` if the block cannot
 *         be allocated
 *
 * @see av_mallocz()
 * @see av_malloc_array()
 */
void *av_mallocz_array(size_t nmemb, size_t size);

/**
 * Non-inlined equivalent of av_mallocz_array().
 *
 * Created for symmetry with the calloc() C function.
 */
void *av_calloc(size_t nmemb, size_t size);

/**
 * Allocate, reallocate, or free a block of memory.
 *
 * If `ptr` is `NULL` and `size` > 0, allocate a new block. If `size` is
 * zero, free the memory block pointed to by `ptr`. Otherwise, expand or
 * shrink that block of memory according to `size`.
 *
 * @param ptr  Pointer to a memory block already allocated with
 *             av_realloc() or `NULL`
 * @param size Size in bytes of the memory block to be allocated or
 *             reallocated
 *
 * @return Pointer to a newly-reallocated block or `NULL` if the block
 *         cannot be reallocated or the function is used to free the memory block
 *
 * @warning Unlike av_malloc(), the returned pointer is not guaranteed to be
 *          correctly aligned.
 * @see av_fast_realloc()
 * @see av_reallocp()
 */
void *av_realloc(void *ptr, size_t size);

/**
 * Allocate, reallocate, or free a block of memory through a pointer to a
 * pointer.
 *
 * If `*ptr` is `NULL` and `size` > 0, allocate a new block. If `size` is
 * zero, free the memory block pointed to by `*ptr`. Otherwise, expand or
 * shrink that block of memory according to `size`.
 *
 * @param[in,out] ptr  Pointer to a pointer to a memory block already allocated
 *                     with av_realloc(), or a pointer to `NULL`. The pointer
 *                     is updated on success, or freed on failure.
 * @param[in]     size Size in bytes for the memory block to be allocated or
 *                     reallocated
 *
 * @return Zero on success, an AVERROR error code on failure
 *
 * @warning Unlike av_malloc(), the allocated memory is not guaranteed to be
 *          correctly aligned.
 */
int av_reallocp(void *ptr, size_t size);

/**
 * Allocate, reallocate, or free a block of memory.
 *
 * This function does the same thing as av_realloc(), except:
 * - It takes two size arguments and allocates `nelem * elsize` bytes,
 *   after checking the result of the multiplication for integer overflow.
 * - It frees the input block in case of failure, thus avoiding the memory
 *   leak with the classic
 *   @code{.c}
 *   buf = realloc(buf);
 *   if (!buf)
 *       return -1;
 *   @endcode
 *   pattern.
 */
void *av_realloc_f(void *ptr, size_t nelem, size_t elsize);

/**
 * Allocate, reallocate, or free an array.
 *
 * If `ptr` is `NULL` and `nmemb` > 0, allocate a new block. If
 * `nmemb` is zero, free the memory block pointed to by `ptr`.
 *
 * @param ptr   Pointer to a memory block already allocated with
 *              av_realloc() or `NULL`
 * @param nmemb Number of elements in the array
 * @param size  Size of the single element of the array
 *
 * @return Pointer to a newly-reallocated block or NULL if the block
 *         cannot be reallocated or the function is used to free the memory block
 *
 * @warning Unlike av_malloc(), the allocated memory is not guaranteed to be
 *          correctly aligned.
 * @see av_reallocp_array()
 */
void *av_realloc_array(void *ptr, size_t nmemb, size_t size);

/**
 * Allocate, reallocate, or free an array through a pointer to a pointer.
 *
 * If `*ptr` is `NULL` and `nmemb` > 0, allocate a new block. If `nmemb` is
 * zero, free the memory block pointed to by `*ptr`.
 *
 * @param[in,out] ptr   Pointer to a pointer to a memory block already
 *                      allocated with av_realloc(), or a pointer to `NULL`.
 *                      The pointer is updated on success, or freed on failure.
 * @param[in]     nmemb Number of elements
 * @param[in]     size  Size of the single element
 *
 * @return Zero on success, an AVERROR error code on failure
 *
 * @warning Unlike av_malloc(), the allocated memory is not guaranteed to be
 *          correctly aligned.
 */
int av_reallocp_array(void *ptr, size_t nmemb, size_t size);

/**
 * Reallocate the given buffer if it is not large enough, otherwise do nothing.
 *
 * If the given buffer is `NULL`, then a new uninitialized buffer is allocated.
 *
 * If the given buffer is not large enough, and reallocation fails, `NULL` is
 * returned and `*size` is set to 0, but the original buffer is not changed or
 * freed.
 *
 * A typical use pattern follows:
 *
 * @code{.c}
 * uint8_t *buf = ...;
 * uint8_t *new_buf = av_fast_realloc(buf, &current_size, size_needed);
 * if (!new_buf) {
 *     // Allocation failed; clean up original buffer
 *     av_freep(&buf);
 *     return AVERROR(ENOMEM);
 * }
 * @endcode
 *
 * @param[in,out] ptr      Already allocated buffer, or `NULL`
 * @param[in,out] size     Pointer to the size of buffer `ptr`. `*size` is
 *                         updated to the new allocated size, in particular 0
 *                         in case of failure.
 * @param[in]     min_size Desired minimal size of buffer `ptr`
 * @return `ptr` if the buffer is large enough, a pointer to newly reallocated
 *         buffer if the buffer was not large enough, or `NULL` in case of
 *         error
 * @see av_realloc()
 * @see av_fast_malloc()
 */
void *av_fast_realloc(void *ptr, unsigned int *size, size_t min_size);

/**
 * Allocate a buffer, reusing the given one if large enough.
 *
 * Contrary to av_fast_realloc(), the current buffer contents might not be
 * preserved and on error the old buffer is freed, thus no special handling to
 * avoid memleaks is necessary.
 *
 * `*ptr` is allowed to be `NULL`, in which case allocation always happens if
 * `size_needed` is greater than 0.
 *
 * @code{.c}
 * uint8_t *buf = ...;
 * av_fast_malloc(&buf, &current_size, size_needed);
 * if (!buf) {
 *     // Allocation failed; buf already freed
 *     return AVERROR(ENOMEM);
 * }
 * @endcode
 *
 * @param[in,out] ptr      Pointer to pointer to an already allocated buffer.
 *                         `*ptr` will be overwritten with pointer to new
 *                         buffer on success or `NULL` on failure
 * @param[in,out] size     Pointer to the size of buffer `*ptr`. `*size` is
 *                         updated to the new allocated size, in particular 0
 *                         in case of failure.
 * @param[in]     min_size Desired minimal size of buffer `*ptr`
 * @see av_realloc()
 * @see av_fast_mallocz()
 */
void av_fast_malloc(void *ptr, unsigned int *size, size_t min_size);

/**
 * Allocate and clear a buffer, reusing the given one if large enough.
 *
 * Like av_fast_malloc(), but all newly allocated space is initially cleared.
 * Reused buffer is not cleared.
 *
 * `*ptr` is allowed to be `NULL`, in which case allocation always happens if
 * `size_needed` is greater than 0.
 *
 * @param[in,out] ptr      Pointer to pointer to an already allocated buffer.
 *                         `*ptr` will be overwritten with pointer to new
 *                         buffer on success or `NULL` on failure
 * @param[in,out] size     Pointer to the size of buffer `*ptr`. `*size` is
 *                         updated to the new allocated size, in particular 0
 *                         in case of failure.
 * @param[in]     min_size Desired minimal size of buffer `*ptr`
 * @see av_fast_malloc()
 */
void av_fast_mallocz(void *ptr, unsigned int *size, size_t min_size);

/**
 * Free a memory block which has been allocated with a function of av_malloc()
 * or av_realloc() family.
 *
 * @param ptr Pointer to the memory block which should be freed.
 *
 * @note `ptr = NULL` is explicitly allowed.
 * @note It is recommended that you use av_freep() instead, to prevent leaving
 *       behind dangling pointers.
 * @see av_freep()
 */
void av_free(void *ptr);

/**
 * Free a memory block which has been allocated with a function of av_malloc()
 * or av_realloc() family, and set the pointer pointing to it to `NULL`.
 *
 * @code{.c}
 * uint8_t *buf = av_malloc(16);
 * av_free(buf);
 * // buf now contains a dangling pointer to freed memory, and accidental
 * // dereference of buf will result in a use-after-free, which may be a
 * // security risk.
 *
 * uint8_t *buf = av_malloc(16);
 * av_freep(&buf);
 * // buf is now NULL, and accidental dereference will only result in a
 * // NULL-pointer dereference.
 * @endcode
 *
 * @param ptr Pointer to the pointer to the memory block which should be freed
 * @note `*ptr = NULL` is safe and leads to no action.
 * @see av_free()
 */
void av_freep(void *ptr);


/**
 * @}
 * @}
 */

#endif /* AVUTIL_MEM_H */
