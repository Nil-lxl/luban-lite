/*
 * Copyright (C) 2018-2024 AICSemi Ltd.
 *
 * All Rights Reserved
 */

#ifndef _WIFI_PORT_H_
#define _WIFI_PORT_H_

#include "wifi_def.h"
#include "aic_plat_types.h"

#ifndef os_strcmp
#define os_strcmp(s1, s2) strcmp((s1), (s2))
#endif
#ifndef os_strcat
#define os_strcat(s1, s2) strcat(s1, s2)
#endif
#ifndef os_strlen
#define os_strlen(s) strlen(s)
#endif

struct aic_wifi_dev
{
    rtos_semaphore   wifi_task_sem;
    rtos_task_handle wifi_task_init_hdl;

    uint32_t         wifi_drive_op;
    uint32_t         wifi_config_st;
    uint32_t         wifi_init_status;
    AIC_WIFI_MODE    wifi_mode;
    int              g_wifi_init;
};

extern struct aic_wifi_dev wifi_dev;

extern struct rt_wlan_device * s_wlan_dev;
extern struct rt_wlan_device * s_ap_dev;

void aic_driver_reboot(unsigned int mode);
void aic_driver_unexcepted(unsigned int evt);

#endif /* _WIFI_PORT_H_ */

