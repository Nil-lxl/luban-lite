#include "aic_common.h"

#define CONFIG_PLATFOMR_CUSTOMER_RTOS
#define DRV_NAME "RTL8733BS"

#ifdef REALTEK_FULL_FNC_MODE
#define DRIVERVERSION "460653b8f542ceeb2c9019ac6c38cd8ec20a4424"
#else
#define DRIVERVERSION "57999b1af4f228855793e1801ce9bbea4565e7a8"
#endif

#define CONFIG_DEBUG	1

#define CONFIG_MP_INCLUDED                  1
#define CONFIG_MP_NORMAL_IWPRIV_SUPPORT     1


#define CONFIG_NO_REFERENCE_FOR_COMPILER    1
#define MAX_TX_SKB_DATA_NUM	    100

#ifdef REALTEK_FULL_FNC_MODE
#define CONFIG_AP_MODE                      1
//20240904_v1.4
#define CONFIG_CONCURRENT_MODE              1
#define CONFIG_PROMISC                      1
#define CONFIG_WPS                          1
#define CONFIG_ENABLE_P2P                   1
#endif

#ifndef CONFIG_WLAN
#define CONFIG_WLAN 1
#endif

#define IN
#define OUT
