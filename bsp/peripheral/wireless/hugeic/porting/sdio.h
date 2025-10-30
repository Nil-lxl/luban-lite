#ifndef _SDIO_H_PORT_
#define _SDIO_H_PORT_

#if defined(FH_RTT)
#include "fh_rtt/sdio.h"
#elif defined(AK_RTT)
#include "ak_rtt/sdio.h"
#elif defined(HW_LOS)
#include "hw_los/sdio.h"
#elif defined(TDS)
#include "tds/os_sdio.h"
#elif defined(ECOS)
#include "ecos/os_sdio.h"
#elif defined(ALIOS)
//#include "ak_alios/os_sdio.h"
#elif defined(JL_RTOS)
#include "jl_rtos/os_sdio.h"
#elif defined(SXW_RTT)
#include "sxw_rtt/os_sdio.h"
#elif defined(HC_RTT)
#include "hc_rtos/os_sdio.h"
#elif defined(HXD_RTT)
#include "hxd_rtos/os_sdio.h"
#elif defined(JXC_RTT)
#include "jxc_rtt/os_sdio.h"
#else
#error "no defined porting platform"
#endif

extern int hgic_sdio_probe(sdio_func_t *func, const sdio_device_id_t *id);
extern void hgic_sdio_remove(sdio_func_t *func);


#endif

