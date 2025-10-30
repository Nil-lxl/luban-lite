#ifndef _LINUX__HWEIGHT_H_
#define _LINUX__HWEIGHT_H_

#define __const_hweight8(w)     \
    ((unsigned int)         \
     ((!!((w) & (1ULL << 0))) + \
      (!!((w) & (1ULL << 1))) + \
      (!!((w) & (1ULL << 2))) + \
      (!!((w) & (1ULL << 3))) + \
      (!!((w) & (1ULL << 4))) + \
      (!!((w) & (1ULL << 5))) + \
      (!!((w) & (1ULL << 6))) + \
      (!!((w) & (1ULL << 7)))))

#define __const_hweight16(w) (__const_hweight8(w)  + __const_hweight8((w)  >> 8 ))
#define __const_hweight32(w) (__const_hweight16(w) + __const_hweight16((w) >> 16))
#define __const_hweight64(w) (__const_hweight32(w) + __const_hweight32((w) >> 32))

unsigned int __sw_hweight32(unsigned int w);
unsigned int __sw_hweight16(unsigned int w);
unsigned int __sw_hweight8(unsigned int w);
unsigned long __sw_hweight64(__u64 w);

static inline unsigned int __arch_hweight32(unsigned int w)
{
    return __sw_hweight32(w);
}

static inline unsigned int __arch_hweight16(unsigned int w)
{
    return __sw_hweight16(w);
}

static inline unsigned int __arch_hweight8(unsigned int w)
{
    return __sw_hweight8(w);
}

static inline unsigned long __arch_hweight64(__u64 w)
{
    return __sw_hweight64(w);
}

#define hweight8(w)  (__builtin_constant_p(w) ? __const_hweight8(w)  : __arch_hweight8(w))
#define hweight16(w) (__builtin_constant_p(w) ? __const_hweight16(w) : __arch_hweight16(w))
#define hweight32(w) (__builtin_constant_p(w) ? __const_hweight32(w) : __arch_hweight32(w))
#define hweight64(w) (__builtin_constant_p(w) ? __const_hweight64(w) : __arch_hweight64(w))

static inline unsigned long hweight_long(unsigned long w)
{
	return sizeof(w) == 4 ? hweight32(w) : hweight64(w);
}

#endif

