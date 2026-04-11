#ifndef _USB_H_PORT_
#define _USB_H_PORT_
#if defined(FH_RTT)
#include "fh_rtt/usb.h"
#elif defined(AK_RTT)
#include "ak_rtt/usb.h"
#elif defined(HW_LOS)
#include "hw_los/usb.h"
#elif defined(TDS)
#include "tds/os_usb.h"
#elif defined(ECOS)
#include "ecos/os_usb.h"
#elif defined(ALIOS)
#include "ak_alios/os_usb.h"
#elif defined(JL_RTOS)
#include "jl_rtos/os_usb.h"
#elif defined(SXW_RTT)
#include "sxw_rtt/os_usb.h"
#elif defined(HC_RTT)
#include "hc_rtos/os_usb.h"
#elif defined(JXC_RTT)
#include "jxc_rtt/os_usb.h"
#else
#error "no defined porting platform"
#endif
#endif

