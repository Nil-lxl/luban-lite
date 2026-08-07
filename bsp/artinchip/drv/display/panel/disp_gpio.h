#include "rtconfig.h"

#if defined AIC_USING_D213ECV_EzUIX1_DEMO_V1
#define RESET_PIN  "PB.6"
#define CS         "PE.13"
#define SCL        "PE.12"
#define SDI        "PE.18"
#elif defined AIC_USING_D213ECV_EzUIX1_DEMO_V0
#define RESET_PIN  "PA.3"
#define CS         "PE.17"
#define SCL        "PE.16"
#define SDI        "PE.18"
#elif defined AIC_USING_H215_DEMO_A02_V0
#define RESET_PIN  "PF.0"
#define CS         "PC.6"
#define SCL        "PF.15"
#define SDI        "PF.14"
#elif defined AIC_USING_JYX68_RGB01
#define RESET_PIN  "PB.1"
#define CS         "PE.13"
#define SCL        "PE.12"
#define SDI        "PE.14"
#elif defined AIC_USING_HT68_DEMO_A01
#define RESET_PIN  "PC.5"
#define CS         "PE.13"
#define SCL        "PE.12"
#define SDI        "PE.11"
#elif defined AIC_USING_HT68_DEMO_A02_V1
#define RESET_PIN  "PB.1"
#define CS         "PE.15"
#define SCL        "PE.14"
#define SDI        "PE.13"
#endif
