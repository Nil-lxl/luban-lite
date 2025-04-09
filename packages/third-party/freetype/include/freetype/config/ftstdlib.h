/****************************************************************************
 *
 * ftstdlib.h
 *
 *   ANSI-specific library and header configuration file (specification
 *   only).
 *
 * Copyright (C) 2002-2020 by
 * David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 * This file is part of the FreeType project, and may only be used,
 * modified, and distributed under the terms of the FreeType project
 * license, LICENSE.TXT.  By continuing to use, modify, or distribute
 * this file you indicate that you have read the license and
 * understand and accept it fully.
 *
 */


  /**************************************************************************
   *
   * This file is used to group all `#includes` to the ANSI~C library that
   * FreeType normally requires.  It also defines macros to rename the
   * standard functions within the FreeType source code.
   *
   * Load a file which defines `FTSTDLIB_H_` before this one to override it.
   *
   */


#ifndef FTSTDLIB_H_
#define FTSTDLIB_H_


#include <stddef.h>

#define ft_ptrdiff_t  ptrdiff_t


  /**************************************************************************
   *
   *                          integer limits
   *
   * `UINT_MAX` and `ULONG_MAX` are used to automatically compute the size of
   * `int` and `long` in bytes at compile-time.  So far, this works for all
   * platforms the library has been tested on.
   *
   * Note that on the extremely rare platforms that do not provide integer
   * types that are _exactly_ 16 and 32~bits wide (e.g., some old Crays where
   * `int` is 36~bits), we do not make any guarantee about the correct
   * behaviour of FreeType~2 with all fonts.
   *
   * In these cases, `ftconfig.h` will refuse to compile anyway with a
   * message like 'couldn't find 32-bit type' or something similar.
   *
   */


#include <limits.h>

#define FT_CHAR_BIT    CHAR_BIT
#define FT_USHORT_MAX  USHRT_MAX
#define FT_INT_MAX     INT_MAX
#define FT_INT_MIN     INT_MIN
#define FT_UINT_MAX    UINT_MAX
#define FT_LONG_MIN    LONG_MIN
#define FT_LONG_MAX    LONG_MAX
#define FT_ULONG_MAX   ULONG_MAX


  /**************************************************************************
   *
   *                character and string processing
   *
   */


#include <string.h>

#define ft_memchr   memchr
#define ft_memcmp   memcmp
#define ft_memcpy   memcpy
#define ft_memmove  memmove
#define ft_memset   memset
#define ft_strcat   strcat
#define ft_strcmp   strcmp
#define ft_strcpy   strcpy
#define ft_strlen   strlen
#define ft_strncmp  strncmp
#define ft_strncpy  strncpy
#define ft_strrchr  strrchr
#define ft_strstr   strstr


  /**************************************************************************
   *
   *                          file handling
   *
   */


#include <stdio.h>
#include <rtconfig.h>

#if defined(KERNEL_BAREMETAL)
#include <unistd.h>
#include <fcntl.h>

#define FT_FILE     int
static inline FT_FILE *ft_fopen(const char *pathname, const char *mode)
{
    (void)mode;
    int file;
    file = open(pathname, O_RDONLY);
    if (file < 0)
        return NULL;
    else
        return (FT_FILE *)(long)file;
}

static inline int ft_fclose(FT_FILE *stream)
{
    return close((int)(long)stream);
}

static inline size_t ft_fread(void *ptr, size_t size, size_t nmemb, FT_FILE *stream)
{
    return read((int)(long)stream, ptr, size * nmemb);
}

static inline int ft_fseek(FT_FILE *stream, size_t offset, int whence)
{
    return lseek((int)(long)stream, offset, whence);
}

static inline long ft_ftell(FT_FILE *stream)
{
    return lseek((int)(long)stream, 0, SEEK_CUR);
}

#else

#define FT_FILE     FILE
#define ft_fclose   fclose
#define ft_fopen    fopen
#define ft_fread    fread
#define ft_fseek    fseek
#define ft_ftell    ftell
#endif

#define ft_sprintf  sprintf


  /**************************************************************************
   *
   *                            sorting
   *
   */


#include <stdlib.h>

#define ft_qsort  qsort


  /**************************************************************************
   *
   *                       memory allocation
   *
   */

#ifndef TT_USE_MEM_PSRAM_SW_HEAP

#define ft_scalloc   calloc
#define ft_sfree     free
#define ft_smalloc   malloc
#define ft_srealloc  realloc
#else
#include "aic_osal.h"

static inline void *ft_smalloc(size_t size)
{
    return aicos_malloc(MEM_PSRAM_SW, size);
}

static inline void *ft_scalloc(size_t count, size_t size)
{
    void *p = ft_smalloc(count * size);
    if (p) {
        memset(p, 0, count * size);
    }
    return p;
}

static inline void ft_sfree(void *rmem)
{
    aicos_free(MEM_PSRAM_SW, rmem);
}

static inline void *ft_srealloc(void *rmem, size_t newsize)
{
    (void)rmem;
    (void)newsize;
    return NULL;
}

static inline void *ft_srealloc_cur(long cur_size, long new_size, void *rmem)
{
    void *p = 0;
    if (new_size > cur_size) {
        p = ft_smalloc(new_size);
        if(p) {
            memcpy(p, rmem, cur_size);
            ft_sfree(rmem);
        }
    } else {
        p = rmem;
    }
    return p;
}
#endif


  /**************************************************************************
   *
   *                         miscellaneous
   *
   */


#define ft_strtol  strtol
#define ft_getenv  getenv


  /**************************************************************************
   *
   *                        execution control
   *
   */


#include <setjmp.h>

#define ft_jmp_buf     jmp_buf  /* note: this cannot be a typedef since  */
                                /*       `jmp_buf` is defined as a macro */
                                /*       on certain platforms            */

#define ft_longjmp     longjmp
#define ft_setjmp( b ) setjmp( *(ft_jmp_buf*) &(b) ) /* same thing here */


  /* The following is only used for debugging purposes, i.e., if   */
  /* `FT_DEBUG_LEVEL_ERROR` or `FT_DEBUG_LEVEL_TRACE` are defined. */

#include <stdarg.h>


#endif /* FTSTDLIB_H_ */


/* END */
