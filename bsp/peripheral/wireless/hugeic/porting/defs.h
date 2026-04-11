#ifndef _DEFS_H_PORT_
#define _DEFS_H_PORT_

#if defined(FH_RTT)
#include "fh_rtt/defs.h"
#elif defined(AK_RTT)
#include "ak_rtt/defs.h"
#elif defined(HW_LOS)
#include "hw_los/defs.h"
#elif defined(TDS)
#include "tds/os_defs.h"
#elif defined(ECOS)
#include "ecos/os_defs.h"
#elif defined(ALIOS)
#include "ak_alios/os_defs.h"
#elif defined(JL_RTOS)
#include "jl_rtos/os_defs.h"
#elif defined(SXW_RTT)
#include "sxw_rtt/os_defs.h"
#elif defined(HC_RTT)
#include "hc_rtos/os_defs.h"
#elif defined(HXD_RTT)
#include "hxd_rtos/os_defs.h"
#elif defined(JXC_RTT)
#include "jxc_rtt/os_defs.h"
#else
#error "no defined porting platform"
#endif

#ifndef MEMCPY
#define MEMCPY(dst,src,len)             memcpy(dst,src,len)
#endif

#ifndef MEMSET
#define MEMSET(s,c,n)                   memset(s, c, n)
#endif

#ifndef ZALLOC
#define ZALLOC(size)                    hgic_zalloc(size)
#endif

#ifndef STRDUP
#define STRDUP(s)                       hgic_strdup(s)
#endif

#ifndef REALLOC
#define REALLOC(p,s)                    hgic_realloc(p,s)
#endif

#ifndef STRCPY
#define STRCPY(dest, input)             strcpy(dest,input)
#endif

#ifndef STRLEN
#define STRLEN(s)                       strlen(s)
#endif

#ifndef STRCMP
#define STRCMP(s1,s2)                   strcmp(s1, s2)
#endif

#endif
