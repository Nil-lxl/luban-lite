
#ifndef _IPERF_DEBUG_H_
#define _IPERF_DEBUG_H_

#if 1 //PRJCONF_NET_EN

#include <stdio.h>
//#include "sys/xr_util.h"
#include "asr_dbg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IPERF_DBG_ON    1
#define IPERF_WARN_ON    1
#define IPERF_ERR_ON    1
#define IPERF_ABORT_ON    0

#ifdef ASR_USE_ZLOG
#define IPERF_SYSLOG    LOG_ERR
#else
#define IPERF_SYSLOG    printf
#endif

#define IPERF_ABORT()    sys_abort()

#define IPERF_LOG(flags, fmt, arg...)    \
    do {                                \
        if (flags)                         \
            IPERF_SYSLOG(fmt, ##arg);    \
    } while (0)

#define IPERF_DBG(fmt, arg...)    IPERF_LOG(IPERF_DBG_ON, "[iperf] "fmt, ##arg)
#define IPERF_WARN(fmt, arg...)    IPERF_LOG(IPERF_WARN_ON, "[iperf WARN] "fmt, ##arg)
#define IPERF_ERR(fmt, arg...)                                \
    do {                                                    \
        IPERF_LOG(IPERF_ERR_ON, "[iperf ERR] %s():%d, "fmt,    \
               __func__, __LINE__, ##arg);                    \
        if (IPERF_ABORT_ON)                                    \
            IPERF_ABORT();                                    \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* PRJCONF_NET_EN */
#endif /* _IPERF_DEBUG_H_ */
