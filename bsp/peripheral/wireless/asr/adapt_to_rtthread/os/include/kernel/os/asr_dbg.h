#ifndef _ASR_DBG_H_
#define _ASR_DBG_H_

#include "asr_types.h"
#include <stdio.h>
//#include "debug.h"
#include <rtdbg.h>

#include <string.h>
// #include <logging/log.h>

#if 0
#define D_CRT       1
#define D_ERR       2
#define D_WRN       3
#define D_INF       4
#define D_DBG       5

#define D_LWIFI_KE        BIT(0)
#define D_LWIFI_DMA       BIT(1)
#define D_LWIFI_MM        BIT(2)
#define D_LWIFI_TX        BIT(3)
#define D_LWIFI_RX        BIT(4)
#define D_LWIFI_PHY       BIT(5)
#define D_LWIFI           BIT(6)

#define D_UWIFI_DATA      BIT(11)
#define D_UWIFI_CTRL      BIT(12)
#define D_UWIFI_SUPP      BIT(13)

#define D_OS              BIT(14)
#define D_LWIP            BIT(15)

#define D_AT              BIT(16)

extern uint32_t  GlobalDebugComponents;
extern uint32_t  GlobalDebugLevel;
extern uint8_t   GlobalDebugEn;

#if _DEBUG
#ifdef ASR_USE_ZLOG
#define dbg(Level, Comp, Fmt,...)\
    do {\
        if(GlobalDebugEn && (Comp & GlobalDebugComponents) && (Level <= GlobalDebugLevel)) {\
            LOGE("[uwifidrv] "Fmt " \n", ## __VA_ARGS__);\
        }\
    }while(0)
#else

#undef ENTER
#define ENTER()  HX_LOGD("ASR", "%s[%d]", __func__, __LINE__)

#undef LOGE
#define LOGE(fmt, args...) HX_LOGE("ASR", "%s[%u]: "fmt"", __FUNCTION__, __LINE__, ##args)

#define dbg(Level, Comp, Fmt,...)\
    do { \
        if((Level <= D_DBG)) {\
            HX_LOGD("ASR", "[uwifidrv <"#Comp">%s[%d]] "Fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__);\
        }\
    }while(0)

#endif  // !ASR_USE_ZLOG

#else
#define dbg(Level, Comp, Fmt, ...)
#endif  // !ASR_DBG_ENABLE

#endif

#if _DEBUG
//#define dbg(Level, Comp, Fmt,...) rt_kprintf(Fmt"\n", ##__VA_ARGS__)
#define dbg(Level, Comp, Fmt,...) rt_kprintf("\r\n[uwifidrv <"#Comp">] "Fmt, ## __VA_ARGS__)



#else
#define dbg(Level, Comp, Fmt,...)
#endif
extern uint8_t GlobalDebugEn;


#endif //_ASR_DBG_H_

