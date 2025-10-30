
#ifndef __UMAC_ALLOC_H__
#define __UMAC_ALLOC_H__


#ifdef __cplusplus
extern "C"
{
#endif

#define	__malloc_size_t		unsigned int
#define	__malloc_ptrdiff_t	int
#define	__ptr_t		void *


__ptr_t u_malloc (__malloc_size_t size);
__ptr_t u_realloc (__ptr_t ptr, __malloc_size_t size);
void u_free (__ptr_t ptr);
__ptr_t u_calloc(__malloc_size_t nelem, __malloc_size_t elsize);


#ifdef __cplusplus
}
#endif

#endif /* __LIB_C_ALLOC_H__ */
