#include <rthw.h>
#include <rtdef.h>
#include <rtthread.h>
#include "os_porting.h"
#include "rtconfig.h"
#include "aic_core.h"
#include "hgic.h"

rt_mailbox_t g_hgics_ble_mb = NULL;
static struct rt_thread *g_hgics_ble_thread = NULL;

struct hgic_ble_mb {
    u32 event;
    u8 *data;
    u32 data_len;
};

static void hgic_ble_proc_task()
{
    rt_err_t rt_ret;
    struct hgic_ble_mb *ble_mb = NULL;

    while(1) {
        if (rt_mb_recv(g_hgics_ble_mb, (rt_ubase_t *)&ble_mb, RT_WAITING_FOREVER) == RT_EOK) {
            if (ble_mb != NULL) {
                switch (ble_mb->event) {
                case HGIC_EVENT_BLENC_DATA:
                    hgics_recv_blenc_data(ble_mb->data, ble_mb->data_len);
                    break;
                case HGIC_EVENT_HGIC_DATA:
                    hgic_proc_bt_data(ble_mb->data, ble_mb->data_len);
                    break;
                default:
                    break;
                }
                rt_free(ble_mb);
            }
        }
    }
}

int hgic_ble_proc_init()
{
    g_hgics_ble_mb = rt_mb_create("hgic_ble_mb", 32, RT_IPC_FLAG_FIFO);
    if (g_hgics_ble_mb == NULL) {
        hgic_err("Create hgic_ble_mb failed.\n");
        return -RT_ERROR;
    }

    g_hgics_ble_thread = rt_thread_create("hgic_ble_proc", hgic_ble_proc_task, NULL,
                         4096, 13, 20);
    if (g_hgics_ble_thread != RT_NULL)
        rt_thread_startup(g_hgics_ble_thread);

    return RT_EOK;
}

#if BLENC_STA
struct wifi_state {
    unsigned int drv_inited;
    rt_wlan_mode_t mode;
    struct umac_config umac_cfg;
};

extern struct wifi_state g_wifi_state;
extern struct rt_wlan_device * g_hgics_wlan_dev;
extern struct rt_wlan_device * g_hgics_current_dev;
rt_err_t txw_ble(int argc, char** argv)
{
    struct umac_config *conf = NULL;

    /* due to rtt may not call wlan_mode */
    g_wifi_state.mode = RT_WLAN_STATION;
    g_hgics_current_dev = g_hgics_wlan_dev;

    if(!g_wifi_state.drv_inited)
       return -RT_ERROR;

    conf = &g_wifi_state.umac_cfg;
    if (conf == NULL) {
        hgic_err("ERROR:Get umac configuration failed!\n");
        return -RT_ENOMEM;
    }
    memset(conf->hg0, 0, sizeof(conf->hg0));

    strcpy(conf->hg0, "update_config=1\n" \
       "network={\n" \
       "proto=WPA RSN\n"\
       "scan_ssid=1\n" \
       "pairwise=CCMP TKIP\n" \
       "group=CCMP TKIP\n" \
       "}\n");

    wpas_start("hg0");

    hgic_iwpriv_set_max_txcnt("hg0", 15);

    hgic_blenc_init();
    hgic_blenc_test(HGIC_BLE_TEST_MODE, 100);
    return RT_EOK;
}
MSH_CMD_EXPORT(txw_ble, ble provisioning mode);
#endif

