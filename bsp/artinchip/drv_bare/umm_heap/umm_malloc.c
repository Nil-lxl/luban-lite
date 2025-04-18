/* ----------------------------------------------------------------------------
 * umm_malloc.c - a memory allocator for embedded systems (microcontrollers)
 *
 * See LICENSE for copyright notice
 * See README.md for acknowledgements and description of internals
 * ----------------------------------------------------------------------------
 *
 * R.Hempel 2007-09-22 - Original
 * R.Hempel 2008-12-11 - Added MIT License biolerplate
 *                     - realloc() now looks to see if previous block is free
 *                     - made common operations functions
 * R.Hempel 2009-03-02 - Added macros to disable tasking
 *                     - Added function to dump heap and check for valid free
 *                        pointer
 * R.Hempel 2009-03-09 - Changed name to umm_malloc to avoid conflicts with
 *                        the mm_malloc() library functions
 *                     - Added some test code to assimilate a free block
 *                        with the very block if possible. Complicated and
 *                        not worth the grief.
 * D.Frank 2014-04-02  - Fixed heap configuration when UMM_TEST_MAIN is NOT set,
 *                        added user-dependent configuration file umm_malloc_cfg.h
 * R.Hempel 2016-12-04 - Add support for Unity test framework
 *                     - Reorganize source files to avoid redundant content
 *                     - Move integrity and poison checking to separate file
 * R.Hempel 2017-12-29 - Fix bug in realloc when requesting a new block that
 *                        results in OOM error - see Issue 11
 * R.Hempel 2019-09-07 - Separate the malloc() and free() functionality into
 *                        wrappers that use critical section protection macros
 *                        and static core functions that assume they are
 *                        running in a protected con text. Thanks @devyte
 * R.Hempel 2020-01-07 - Add support for Fragmentation metric - See Issue 14
 * R.Hempel 2020-01-12 - Use explicitly sized values from stdint.h - See Issue 15
 * R.Hempel 2020-01-20 - Move metric functions back to umm_info - See Issue 29
 * R.Hempel 2020-02-01 - Macro functions are uppercased - See Issue 34
 * R.Hempel 2020-06-20 - Support alternate body size - See Issue 42
 * R.Hempel 2021-05-02 - Support explicit memory umm_init_heap() - See Issue 53
 * K.Whitlock 2023-07-06 - Add support for multiple heaps
 * ----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "umm_malloc_cfg.h"   // Override with umm_malloc_cfg_xxx.h
#include "umm_malloc.h"

/* Use the default DBGLOG_LEVEL and DBGLOG_FUNCTION */

// #define DBGLOG_ENABLE

#define DBGLOG_LEVEL 0

#ifdef DBGLOG_ENABLE
    #include "dbglog/dbglog.h"
#endif

extern void *UMM_MALLOC_CFG_HEAP_ADDR;
extern uint32_t UMM_MALLOC_CFG_HEAP_SIZE;

/* ------------------------------------------------------------------------- */

UMM_H_ATTPACKPRE typedef struct umm_ptr_t {
    uint16_t next;
    uint16_t prev;
    uint32_t priv;
} UMM_H_ATTPACKSUF umm_ptr;

UMM_H_ATTPACKPRE typedef struct umm_block_t {
    union {
        umm_ptr used;
    } header;
    union {
        umm_ptr free;
        uint8_t data[UMM_BLOCK_BODY_SIZE - sizeof(struct umm_ptr_t)];
    } body;
} UMM_H_ATTPACKSUF umm_block;

#define UMM_FREELIST_MASK ((uint16_t)(0x8000))
#define UMM_BLOCKNO_MASK  ((uint16_t)(0x7FFF))

/* ------------------------------------------------------------------------- */

#define UMM_HEAP       ((umm_block *)heap->pheap)
#define UMM_HEAPSIZE   (heap->heap_size)
#define UMM_NUMBLOCKS  (heap->numblocks)

#define UMM_BLOCKSIZE  (sizeof(umm_block))
#define UMM_BLOCK_LAST (UMM_NUMBLOCKS - 1)

/* -------------------------------------------------------------------------
 * These macros evaluate to the address of the block and data respectively
 */

#define UMM_BLOCK(b)  (UMM_HEAP[b])
#define UMM_DATA(b)   (UMM_BLOCK(b).body.data)

/* -------------------------------------------------------------------------
 * These macros evaluate to the index of the block - NOT the address!!!
 */

#define UMM_NBLOCK(b) (UMM_BLOCK(b).header.used.next)
#define UMM_PBLOCK(b) (UMM_BLOCK(b).header.used.prev)
#define UMM_NFREE(b)  (UMM_BLOCK(b).body.free.next)
#define UMM_PFREE(b)  (UMM_BLOCK(b).body.free.prev)

/* -------------------------------------------------------------------------
 * There are additional files that may be included here - normally it's
 * not a good idea to include .c files but in this case it keeps the
 * main umm_malloc file clear and prevents issues with exposing internal
 * data structures to other programs.
 * -------------------------------------------------------------------------
 */

#include "umm_integrity.c"
#include "umm_poison.c"
#include "umm_info.c"

/* ------------------------------------------------------------------------ */

static uint16_t umm_blocks(size_t size) {

    /*
     * The calculation of the block size is not too difficult, but there are
     * a few little things that we need to be mindful of.
     *
     * When a block removed from the free list, the space used by the free
     * pointers is available for data. That's what the first calculation
     * of size is doing.
     *
     * We don't check for the special case of (size == 0) here as this needs
     * special handling in the caller depending on context. For example when we
     * realloc() a block to size 0 it should simply be freed.
     *
     * We do NOT need to check for allocating more blocks than the heap can
     * possibly hold - the allocator figures this out for us.
     *
     * There are only two cases left to consider:
     *
     * 1. (size <= body)  Obviously this is just one block
     * 2. (blocks > (2^15)) This should return ((2^15)) to force a
     *                      failure when the allocator runs
     *
     * If the requested size is greater that 32677-2 blocks (max block index
     * minus the overhead of the top and bottom bookkeeping blocks) then we
     * will return an incorrectly truncated value when the result is cast to
     * a uint16_t.
     */

    if (size <= (sizeof(((umm_block *)0)->body))) {
        return 1;
    }

    /*
     * If it's for more than that, then we need to figure out the number of
     * additional whole blocks the size of an umm_block are required, so
     * reduce the size request by the number of bytes in the body of the
     * first block.
     */

    size -= (sizeof(((umm_block *)0)->body));

    /* NOTE WELL that we take advantage of the fact that INT16_MAX is the
     * number of blocks that we can index in 15 bits :-)
     *
     * The below expression looks wierd, but it's right. Assuming body
     * size of 4 bytes and a block size of 8 bytes:
     *
     * BYTES (BYTES-BODY) (BYTES-BODY-1)/BLOCKSIZE BLOCKS
     *     1        n/a                        n/a      1
     *     5          1                          0      2
     *    12          8                          0      2
     *    13          9                          1      3
     */

    size_t blocks = (2 + ((size-1) / (UMM_BLOCKSIZE)));

    if (blocks > (INT16_MAX)) {
        blocks = INT16_MAX;
    }

    return (uint16_t)blocks;
}

/* ------------------------------------------------------------------------ */
/*
 * Split the block `c` into two blocks: `c` and `c + blocks`.
 *
 * - `new_freemask` should be `0` if `c + blocks` used, or `UMM_FREELIST_MASK`
 *   otherwise.
 *
 * Note that free pointers are NOT modified by this function.
 */
static void umm_split_block(umm_heap *heap,
    uint16_t c,
    uint16_t blocks,
    uint16_t new_freemask) {

    UMM_NBLOCK(c + blocks) = (UMM_NBLOCK(c) & UMM_BLOCKNO_MASK) | new_freemask;
    UMM_PBLOCK(c + blocks) = c;

    UMM_PBLOCK(UMM_NBLOCK(c) & UMM_BLOCKNO_MASK) = (c + blocks);
    UMM_NBLOCK(c) = (c + blocks);
}

/* ------------------------------------------------------------------------ */

static void umm_disconnect_from_free_list(umm_heap *heap, uint16_t c) {
    /* Disconnect this block from the FREE list */

    UMM_NFREE(UMM_PFREE(c)) = UMM_NFREE(c);
    UMM_PFREE(UMM_NFREE(c)) = UMM_PFREE(c);

    /* And clear the free block indicator */

    UMM_NBLOCK(c) &= (~UMM_FREELIST_MASK);
}

/* ------------------------------------------------------------------------
 * The umm_assimilate_up() function does not assume that UMM_NBLOCK(c)
 * has the UMM_FREELIST_MASK bit set. It only assimilates up if the
 * next block is free.
 */

static void umm_assimilate_up(umm_heap *heap, uint16_t c) {

    if (UMM_NBLOCK(UMM_NBLOCK(c)) & UMM_FREELIST_MASK) {

        UMM_FRAGMENTATION_METRIC_REMOVE(UMM_NBLOCK(c));

        /*
         * The next block is a free block, so assimilate up and remove it from
         * the free list
         */

        DBGLOG_DEBUG("Assimilate up to next block, which is FREE\n");

        /* Disconnect the next block from the FREE list */

        umm_disconnect_from_free_list(heap, UMM_NBLOCK(c));

        /* Assimilate the next block with this one */

        UMM_PBLOCK(UMM_NBLOCK(UMM_NBLOCK(c)) & UMM_BLOCKNO_MASK) = c;
        UMM_NBLOCK(c) = UMM_NBLOCK(UMM_NBLOCK(c)) & UMM_BLOCKNO_MASK;
    }
}

/* ------------------------------------------------------------------------
 * The umm_assimilate_down() function assumes that UMM_NBLOCK(c) does NOT
 * have the UMM_FREELIST_MASK bit set. In other words, try to assimilate
 * up before assimilating down.
 */

static uint16_t umm_assimilate_down(umm_heap *heap, uint16_t c, uint16_t freemask) {

    // We are going to assimilate down to the previous block because
    // it was free, so remove it from the fragmentation metric

    UMM_FRAGMENTATION_METRIC_REMOVE(UMM_PBLOCK(c));

    UMM_NBLOCK(UMM_PBLOCK(c)) = UMM_NBLOCK(c) | freemask;
    UMM_PBLOCK(UMM_NBLOCK(c)) = UMM_PBLOCK(c);

    if (freemask) {
        // We are going to free the entire assimilated block
        // so add it to the fragmentation metric. A good
        // compiler will optimize away the empty if statement
        // when UMM_INFO is not defined, so don't worry about
        // guarding it.

        UMM_FRAGMENTATION_METRIC_ADD(UMM_PBLOCK(c));
    }

    return UMM_PBLOCK(c);
}

/* ------------------------------------------------------------------------- */

void umm_init_heap(umm_heap *heap, void *ptr, size_t size)
{
    /* init heap pointer and size, and memset it to 0 */
    heap->pheap = ptr;
    UMM_HEAPSIZE = size;
    UMM_NUMBLOCKS = (UMM_HEAPSIZE / UMM_BLOCKSIZE);

    /* setup initial blank heap structure */
    UMM_FRAGMENTATION_METRIC_INIT();

    /* Set up umm_block[0], which just points to umm_block[1] */
    memset(&UMM_BLOCK(0), 0x00, UMM_BLOCKSIZE);
    UMM_NBLOCK(0) = 1;
    UMM_NFREE(0) = 1;
    UMM_PFREE(0) = 1;

    /*
     * Now, we need to set the whole heap space as a huge free block. We should
     * not touch umm_block[0], since it's special: umm_block[0] is the head of
     * the free block list. It's a part of the heap invariant.
     *
     * See the detailed explanation at the beginning of the file.
     *
     * umm_block[1] has pointers:
     *
     * - next `umm_block`: the last one umm_block[n]
     * - prev `umm_block`: umm_block[0]
     *
     * Plus, it's a free `umm_block`, so we need to apply `UMM_FREELIST_MASK`
     *
     * And it's the last free block, so the next free block is 0 which marks
     * the end of the list. The previous block and free block pointer are 0
     * too, there is no need to initialize these values due to the init code
     * that memsets the entire umm_ space to 0.
     */
    memset(&UMM_BLOCK(1), 0x00, UMM_BLOCKSIZE);
    UMM_NBLOCK(1) = UMM_BLOCK_LAST | UMM_FREELIST_MASK;

    /*
     * Last umm_block[n] has the next block index at 0, meaning it's
     * the end of the list, and the previous block is umm_block[1].
     *
     * The last block is a special block and can never be part of the
     * free list, so its pointers are left at 0 too.
     */

    memset(&UMM_BLOCK(UMM_BLOCK_LAST), 0x00, UMM_BLOCKSIZE);
    UMM_PBLOCK(UMM_BLOCK_LAST) = 1;

// DBGLOG_FORCE(true, "nblock(0) %04x pblock(0) %04x nfree(0) %04x pfree(0) %04x\n", UMM_NBLOCK(0) & UMM_BLOCKNO_MASK, UMM_PBLOCK(0), UMM_NFREE(0), UMM_PFREE(0));
// DBGLOG_FORCE(true, "nblock(1) %04x pblock(1) %04x nfree(1) %04x pfree(1) %04x\n", UMM_NBLOCK(1) & UMM_BLOCKNO_MASK, UMM_PBLOCK(1), UMM_NFREE(1), UMM_PFREE(1));

}

void umm_init(umm_heap *heap) {
    /* Initialize the heap from linker supplied values */

    umm_init_heap(heap, UMM_MALLOC_CFG_HEAP_ADDR, UMM_MALLOC_CFG_HEAP_SIZE);
}

/* ------------------------------------------------------------------------
 * Must be called only from within critical sections guarded by
 * UMM_CRITICAL_ENTRY(id) and UMM_CRITICAL_EXIT(id).
 */

static void umm_free_core(umm_heap *heap, void *ptr) {

    uint16_t c;

    /*
     * FIXME: At some point it might be a good idea to add a check to make sure
     *        that the pointer we're being asked to free up is actually within
     *        the umm_heap!
     *
     * NOTE:  See the new umm_info() function that you can use to see if a ptr is
     *        on the free list!
     */

    /* Figure out which block we're in. Note the use of truncated division... */

    c = (((uint8_t *)ptr) - (uint8_t *)(&(UMM_HEAP[0]))) / UMM_BLOCKSIZE;

    DBGLOG_DEBUG("Freeing block %6i\n", c);

    /* Now let's assimilate this block with the next one if possible. */

    umm_assimilate_up(heap, c);

    /* Then assimilate with the previous block if possible */

    if (UMM_NBLOCK(UMM_PBLOCK(c)) & UMM_FREELIST_MASK) {

        DBGLOG_DEBUG("Assimilate down to previous block, which is FREE\n");

        c = umm_assimilate_down(heap, c, UMM_FREELIST_MASK);
    } else {
        /*
         * The previous block is not a free block, so add this one to the head
         * of the free list
         */
        UMM_FRAGMENTATION_METRIC_ADD(c);

        DBGLOG_DEBUG("Just add to head of free list\n");

        UMM_PFREE(UMM_NFREE(0)) = c;
        UMM_NFREE(c) = UMM_NFREE(0);
        UMM_PFREE(c)            = 0;
        UMM_NFREE(0) = c;

        UMM_NBLOCK(c) |= UMM_FREELIST_MASK;
    }
}

/* ------------------------------------------------------------------------ */

void umm_free(umm_heap *heap, void *ptr) {
    UMM_CRITICAL_DECL(id_free);

    UMM_CHECK_INITIALIZED();

    /* If we're being asked to free a NULL pointer, well that's just silly! */

    if ((void *)0 == ptr) {
        DBGLOG_DEBUG("free a null pointer -> do nothing\n");

        return;
    }

    /* If we're being asked to free an out of range pointer - do nothing */
    /* TODO: remove the check for NULL pointer later */

    if ((ptr < heap->pheap) || ((size_t)ptr >= (size_t)heap->pheap + heap->heap_size)) {
        DBGLOG_DEBUG("free an out of range pointer -> do nothing\n");

        return;
    }

    /* Free the memory withing a protected critical section */

    UMM_CRITICAL_ENTRY(id_free);

    umm_free_core(heap, ptr);

    UMM_CRITICAL_EXIT(id_free);
}

/* ------------------------------------------------------------------------
 * Must be called only from within critical sections guarded by
 * UMM_CRITICAL_ENTRY(id) and UMM_CRITICAL_EXIT(id).
 */

static void *umm_malloc_core(umm_heap *heap, size_t size) {
    uint16_t blocks;
    uint16_t blockSize = 0;

    uint16_t bestSize;
    uint16_t bestBlock;

    uint16_t cf;

    blocks = umm_blocks(size);

    /*
     * Now we can scan through the free list until we find a space that's big
     * enough to hold the number of blocks we need.
     *
     * This part may be customized to be a best-fit, worst-fit, or first-fit
     * algorithm
     */

    cf = UMM_NFREE(0);

    bestBlock = UMM_NFREE(0);
    bestSize = 0x7FFF;

    while (cf) {
        blockSize = (UMM_NBLOCK(cf) & UMM_BLOCKNO_MASK) - cf;

        DBGLOG_TRACE("Looking at block %6i size %6i\n", cf, blockSize);

        #if defined UMM_BEST_FIT
        if ((blockSize >= blocks) && (blockSize < bestSize)) {
            bestBlock = cf;
            bestSize = blockSize;
        }
        #elif defined UMM_FIRST_FIT
        /* This is the first block that fits! */
        if ((blockSize >= blocks)) {
            break;
        }
        #else
        #error "No UMM_*_FIT is defined - check umm_malloc_cfg.h"
        #endif

        cf = UMM_NFREE(cf);
    }

    if (0x7FFF != bestSize) {
        cf = bestBlock;
        blockSize = bestSize;
    }

    if (UMM_NBLOCK(cf) & UMM_BLOCKNO_MASK && blockSize >= blocks) {

        UMM_FRAGMENTATION_METRIC_REMOVE(cf);

        /*
         * This is an existing block in the memory heap, we just need to split off
         * what we need, unlink it from the free list and mark it as in use, and
         * link the rest of the block back into the freelist as if it was a new
         * block on the free list...
         */

        if (blockSize == blocks) {
            /* It's an exact fit and we don't neet to split off a block. */
            DBGLOG_DEBUG("Allocating %6i blocks starting at %6i - exact\n", blocks, cf);

            /* Disconnect this block from the FREE list */

            umm_disconnect_from_free_list(heap, cf);
        } else {

            /* It's not an exact fit and we need to split off a block. */
            DBGLOG_DEBUG("Allocating %6i blocks starting at %6i - existing\n", blocks, cf);

            /*
             * split current free block `cf` into two blocks. The first one will be
             * returned to user, so it's not free, and the second one will be free.
             */
            umm_split_block(heap, cf, blocks, UMM_FREELIST_MASK /*new block is free*/);

            UMM_FRAGMENTATION_METRIC_ADD(UMM_NBLOCK(cf));

            /*
             * `umm_split_block()` does not update the free pointers (it affects
             * only free flags), but effectively we've just moved beginning of the
             * free block from `cf` to `cf + blocks`. So we have to adjust pointers
             * to and from adjacent free blocks.
             */

            /* previous free block */
            UMM_NFREE(UMM_PFREE(cf)) = cf + blocks;
            UMM_PFREE(cf + blocks) = UMM_PFREE(cf);

            /* next free block */
            UMM_PFREE(UMM_NFREE(cf)) = cf + blocks;
            UMM_NFREE(cf + blocks) = UMM_NFREE(cf);
        }

    } else {
        /* Out of memory */

        DBGLOG_DEBUG("Can't allocate %5i blocks\n", blocks);

        return (void *)NULL;
    }

    return (void *)&UMM_DATA(cf);
}

/* ------------------------------------------------------------------------ */

void *umm_malloc(umm_heap *heap, size_t size) {
    UMM_CRITICAL_DECL(id_malloc);

    void *ptr = NULL;

    UMM_CHECK_INITIALIZED();

    /*
     * the very first thing we do is figure out if we're being asked to allocate
     * a size of 0 - and if we are we'll simply return a null pointer. if not
     * then reduce the size by 1 byte so that the subsequent calculations on
     * the number of blocks to allocate are easier...
     */

    if (0 == size) {
        DBGLOG_DEBUG("malloc a block of 0 bytes -> do nothing\n");

        return ptr;
    }

    /* Allocate the memory withing a protected critical section */

    UMM_CRITICAL_ENTRY(id_malloc);

    ptr = umm_malloc_core(heap, size);

    UMM_CRITICAL_EXIT(id_malloc);

    return ptr;
}

/* ------------------------------------------------------------------------ */

void *umm_realloc(umm_heap *heap, void *ptr, size_t size) {
    UMM_CRITICAL_DECL(id_realloc);

    uint16_t blocks;
    uint16_t blockSize;
    uint16_t prevBlockSize = 0;
    uint16_t nextBlockSize = 0;

    uint16_t c;

    size_t curSize;

    UMM_CHECK_INITIALIZED();

    /*
     * This code looks after the case of a NULL value for ptr. The ANSI C
     * standard says that if ptr is NULL and size is non-zero, then we've
     * got to work the same a malloc(). If size is also 0, then our version
     * of malloc() returns a NULL pointer, which is OK as far as the ANSI C
     * standard is concerned.
     */

    if (((void *)NULL == ptr)) {
        DBGLOG_DEBUG("realloc the NULL pointer - call malloc()\n");

        return umm_malloc(heap, size);
    }

    /*
     * Now we're sure that we have a non_NULL ptr, but we're not sure what
     * we should do with it. If the size is 0, then the ANSI C standard says that
     * we should operate the same as free.
     */

    if (0 == size) {
        DBGLOG_DEBUG("realloc to 0 size, just free the block\n");

        umm_free(heap, ptr);

        return (void *)NULL;
    }

    /*
     * Otherwise we need to actually do a reallocation. A naiive approach
     * would be to malloc() a new block of the correct size, copy the old data
     * to the new block, and then free the old block.
     *
     * While this will work, we end up doing a lot of possibly unnecessary
     * copying. So first, let's figure out how many blocks we'll need.
     */

    blocks = umm_blocks(size);

    /* Figure out which block we're in. Note the use of truncated division... */

    c = (((uint8_t *)ptr) - (uint8_t *)(&(UMM_HEAP[0]))) / UMM_BLOCKSIZE;

    /* Figure out how big this block is ... the free bit is not set :-) */

    blockSize = (UMM_NBLOCK(c) - c);

    /* Figure out how many bytes are in this block */

    curSize = (blockSize * UMM_BLOCKSIZE) - (sizeof(((umm_block *)0)->header));

    /* Protect the critical section... */
    UMM_CRITICAL_ENTRY(id_realloc);

    /* Now figure out if the previous and/or next blocks are free as well as
     * their sizes - this will help us to minimize special code later when we
     * decide if it's possible to use the adjacent blocks.
     *
     * We set prevBlockSize and nextBlockSize to non-zero values ONLY if they
     * are free!
     */

    if ((UMM_NBLOCK(UMM_NBLOCK(c)) & UMM_FREELIST_MASK)) {
        nextBlockSize = (UMM_NBLOCK(UMM_NBLOCK(c)) & UMM_BLOCKNO_MASK) - UMM_NBLOCK(c);
    }

    if ((UMM_NBLOCK(UMM_PBLOCK(c)) & UMM_FREELIST_MASK)) {
        prevBlockSize = (c - UMM_PBLOCK(c));
    }

    DBGLOG_DEBUG("realloc blocks %i blockSize %i nextBlockSize %i prevBlockSize %i\n", blocks, blockSize, nextBlockSize, prevBlockSize);

    /*
     * Ok, now that we're here we know how many blocks we want and the current
     * blockSize. The prevBlockSize and nextBlockSize are set and we can figure
     * out the best strategy for the new allocation as follows:
     *
     * 1. If the new block is the same size or smaller than the current block do
     *    nothing.
     * 2. If the next block is free and adding it to the current block gives us
     *    EXACTLY enough memory, assimilate the next block. This avoids unwanted
     *    fragmentation of free memory.
     *
     * The following cases may be better handled with memory copies to reduce
     * fragmentation
     *
     * 3. If the previous block is NOT free and the next block is free and
     *    adding it to the current block gives us enough memory, assimilate
     *    the next block. This may introduce a bit of fragmentation.
     * 4. If the prev block is free and adding it to the current block gives us
     *    enough memory, remove the previous block from the free list, assimilate
     *    it, copy to the new block.
     * 5. If the prev and next blocks are free and adding them to the current
     *    block gives us enough memory, assimilate the next block, remove the
     *    previous block from the free list, assimilate it, copy to the new block.
     * 6. Otherwise try to allocate an entirely new block of memory. If the
     *    allocation works free the old block and return the new pointer. If
     *    the allocation fails, return NULL and leave the old block intact.
     *
     * TODO: Add some conditional code to optimise for less fragmentation
     *       by simply allocating new memory if we need to copy anyways.
     *
     * All that's left to do is decide if the fit was exact or not. If the fit
     * was not exact, then split the memory block so that we use only the requested
     * number of blocks and add what's left to the free list.
     */

    //  Case 1 - block is same size or smaller
    if (blockSize >= blocks) {
        DBGLOG_DEBUG("realloc the same or smaller size block - %i, do nothing\n", blocks);
        /* This space intentionally left blank */

        //  Case 2 - block + next block fits EXACTLY
    } else if ((blockSize + nextBlockSize) == blocks) {
        DBGLOG_DEBUG("exact realloc using next block - %i\n", blocks);
        umm_assimilate_up(heap, c);
        blockSize += nextBlockSize;

        //  Case 3 - prev block NOT free and block + next block fits
    } else if ((0 == prevBlockSize) && (blockSize + nextBlockSize) >= blocks) {
        DBGLOG_DEBUG("realloc using next block - %i\n", blocks);
        umm_assimilate_up(heap, c);
        blockSize += nextBlockSize;

        //  Case 4 - prev block + block fits
    } else if ((prevBlockSize + blockSize) >= blocks) {
        DBGLOG_DEBUG("realloc using prev block - %i\n", blocks);
        umm_disconnect_from_free_list(heap, UMM_PBLOCK(c));
        c = umm_assimilate_down(heap, c, 0);
        memmove((void *)&UMM_DATA(c), ptr, curSize);
        ptr = (void *)&UMM_DATA(c);
        blockSize += prevBlockSize;

        //  Case 5 - prev block + block + next block fits
    } else if ((prevBlockSize + blockSize + nextBlockSize) >= blocks) {
        DBGLOG_DEBUG("realloc using prev and next block - %i\n", blocks);
        umm_assimilate_up(heap, c);
        umm_disconnect_from_free_list(heap, UMM_PBLOCK(c));
        c = umm_assimilate_down(heap, c, 0);
        memmove((void *)&UMM_DATA(c), ptr, curSize);
        ptr = (void *)&UMM_DATA(c);
        blockSize += (prevBlockSize + nextBlockSize);

        //  Case 6 - default is we need to realloc a new block
    } else {
        DBGLOG_DEBUG("realloc a completely new block %i\n", blocks);
        void *oldptr = ptr;
        if ((ptr = umm_malloc_core(heap, size))) {
            DBGLOG_DEBUG("realloc %i to a bigger block %i, copy, and free the old\n", blockSize, blocks);
            memcpy(ptr, oldptr, curSize);
            umm_free_core(heap, oldptr);
        } else {
            DBGLOG_DEBUG("realloc %i to a bigger block %i failed - return NULL and leave the old block!\n", blockSize, blocks);
            /* This space intentionally left blnk */
        }
        blockSize = blocks;
    }

    /* Now all we need to do is figure out if the block fit exactly or if we
     * need to split and free ...
     */

    if (blockSize > blocks) {
        DBGLOG_DEBUG("split and free %i blocks from %i\n", blocks, blockSize);
        umm_split_block(heap, c, blocks, 0);
        umm_free_core(heap, (void *)&UMM_DATA(c + blocks));
    }

    /* Release the critical section... */
    UMM_CRITICAL_EXIT(id_realloc);

    return ptr;
}

/* ------------------------------------------------------------------------ */

void *umm_calloc(umm_heap *heap, size_t num, size_t item_size) {
    void *ret;

    ret = umm_malloc(heap, (size_t)(item_size * num));

    if (ret) {
        memset(ret, 0x00, (size_t)(item_size * num));
    }

    return ret;
}

void *umm_malloc_align(umm_heap *heap, size_t size, size_t align)
{
    void *ptr;
    void *align_ptr;
    int uintptr_size;
    size_t align_size;

    /* sizeof pointer */
    uintptr_size = sizeof(void *);
    uintptr_size -= 1;

    /* align the alignment size to uintptr size byte */
    align = ((align + uintptr_size) & ~uintptr_size);

    /* get total aligned size */
    align_size = ((size + uintptr_size) & ~uintptr_size) + align;
    /* allocate memory block from heap */
    ptr = umm_malloc(heap, align_size);
    if (ptr != NULL) {
        /* the allocated memory block is aligned */
        if (((unsigned long)ptr & (align - 1)) == 0) {
            align_ptr = (void *)((unsigned long)ptr + align);
        } else {
            align_ptr = (void *)(((unsigned long)ptr + (align - 1)) & ~(align - 1));
        }

        /* set the pointer before alignment pointer to the real pointer */
        *((unsigned long *)((unsigned long)align_ptr - sizeof(void *))) =
            (unsigned long)ptr;

        ptr = align_ptr;
    }

    return ptr;
}

void umm_free_align(umm_heap *heap, void *ptr)
{
    void *real_ptr;

    /* NULL check */
    if (ptr == NULL)
        return;
    real_ptr = (void *)*(unsigned long *)((unsigned long)ptr - sizeof(void *));
    umm_free(heap, real_ptr);
}
/* ------------------------------------------------------------------------ */
