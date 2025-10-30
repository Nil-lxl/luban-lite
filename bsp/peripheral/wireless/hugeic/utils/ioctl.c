
#ifdef __RTOS__

#include <linux/types.h>
#include <linux/unaligned.h>
#include <linux/bitops.h>
#include <linux/jiffies.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/completion.h>
#include <linux/rcu.h>
#include "hgic_def.h"
#include "fwctrl.h"

int hgic_ioctl(struct hgic_fwctrl *fwctrl, u32 ifidx, u32 cmd, u32 param1, u32 param2)
{
    int ret = 0;
    u8 val[2] = {0};

    switch (cmd) {
        case HGIC_CMD_DEV_OPEN:
            ret = hgic_fwctrl_open_dev(fwctrl, ifidx);
            break;
        case HGIC_CMD_DEV_CLOSE:
            ret = hgic_fwctrl_close_dev(fwctrl, ifidx);
            break;
        case HGIC_CMD_SET_MAC:
            ret = hgic_fwctrl_set_mac(fwctrl, ifidx, (u8 *)param1);
            break;
        case HGIC_CMD_SET_SSID:
            ret = hgic_fwctrl_set_ssid(fwctrl, ifidx, (u8 *)param1);
            break;
        case HGIC_CMD_SET_BSSID:
            ret = hgic_fwctrl_set_bssid(fwctrl, ifidx, (u8 *)param1);
            break;
        case HGIC_CMD_SET_COUNTRY:
            ret = hgic_fwctrl_set_countryregion(fwctrl, ifidx, (u8 *)param1);
            break;
        case HGIC_CMD_SET_CHANNEL:
            ret = hgic_fwctrl_set_channel(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_SET_CENTER_FREQ:
            ret = hgic_fwctrl_set_center_freq(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_SET_RTS_THRESHOLD:
            ret = hgic_fwctrl_set_rts_threshold(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_SET_FRG_THRESHOLD:
            ret = hgic_fwctrl_set_frag_threshold(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_SET_KEY_MGMT:
            ret = hgic_fwctrl_set_key_mgmt(fwctrl, ifidx, (u8 *)param1);
            break;
        case HGIC_CMD_SET_WPA_PSK:
            ret = hgic_fwctrl_set_wpa_psk(fwctrl, ifidx, (u8 *)param1);
            break;
        //HGIC_CMD_SET_KEY,               // 13
        case HGIC_CMD_SCAN:
            ret = hgic_fwctrl_scan(fwctrl, ifidx,param1);
            break;
        case HGIC_CMD_GET_SCAN_LIST:
            ret = hgic_fwctrl_get_scan_list(fwctrl, ifidx, (u8 *)param1, param2);
            if (ret > 0) ret /= sizeof(struct hgic_bss_info);
            break;
        case HGIC_CMD_SET_BSSID_FILTER:
            ret = hgic_fwctrl_set_bssid_filter(fwctrl, ifidx, (u8 *)param1);
            break;
        //HGIC_CMD_DISCONNECT,            // 17
        case HGIC_CMD_GET_BSSID:
            ret = hgic_fwctrl_get_bssid(fwctrl, ifidx, (u8 *)param1,param2);
            break;
        case HGIC_CMD_SET_WBNAT:
            ret = hgic_fwctrl_set_wbnat(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_GET_STATUS:
            ret = hgic_fwctrl_get_status(fwctrl, ifidx, (u8 *)param1, param2);
            break;
        case HGIC_CMD_SET_LISTEN_INTERVAL:
            ret = hgic_fwctrl_set_listen_interval(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_SET_TX_POWER:
            ret = hgic_fwctrl_set_txpower(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_GET_TX_POWER:
            ret = hgic_fwctrl_get_txpower(fwctrl, ifidx);
            break;
        //HGIC_CMD_SET_TX_LCOUNT,         // 24
        //HGIC_CMD_SET_TX_SCOUNT,         // 25
        //HGIC_CMD_ADD_STA,               // 26
        //HGIC_CMD_REMOVE_STA,            // 27
        case HGIC_CMD_SET_TX_BW:
            ret = hgic_fwctrl_set_tx_bw(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_SET_TX_MCS:
            ret = hgic_fwctrl_set_tx_mcs(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_SET_FREQ_RANGE:
            ret = hgic_fwctrl_set_freq_range(fwctrl, ifidx, *(u32 *)param1, *(u32 *)(param1 + 4), *(u32 *)(param1 + 8));
            break;
        case HGIC_CMD_ACS_ENABLE:
            ret = hgic_fwctrl_set_acs(fwctrl, ifidx, param1, param2);
            break;
        case HGIC_CMD_SET_PRIMARY_CHAN:
            ret = hgic_fwctrl_set_primary_chan(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_SET_BG_RSSI:
            ret = hgic_fwctrl_set_bgrssi(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_SET_BSS_BW:
            ret = hgic_fwctrl_set_bss_bw(fwctrl, ifidx, param1);
            break;
        //HGIC_CMD_TESTMODE_CMD,          // 35
        //HGIC_CMD_SET_AID,               // 36
        //HGIC_CMD_GET_FW_STATE,          // 37
        //HGIC_CMD_SET_TXQ_PARAM,         // 38
        case HGIC_CMD_SET_CHAN_LIST:
            ret = hgic_fwctrl_set_chan_list(fwctrl, ifidx, (u16 *)param1, param2);
            break;
        case HGIC_CMD_GET_CONN_STATE:
            ret = (hgic_fwctrl_get_conn_state(fwctrl, ifidx) == 9);
            break;
        case HGIC_CMD_SET_WORK_MODE:
            ret = hgic_fwctrl_set_mode(fwctrl, ifidx, (u8 *)param1);
            break;
        case HGIC_CMD_SET_PAIRED_STATIONS:
            ret = hgic_fwctrl_set_paired_stas(fwctrl, ifidx, (u8 *)param1, param2);
            break;
        case HGIC_CMD_GET_FW_INFO:
            ret = hgic_fwctrl_get_fwinfo(fwctrl, ifidx, (struct hgic_fw_info *)param1);
            break;
        case HGIC_CMD_PAIRING:
            ret = hgic_fwctrl_set_pairing(fwctrl, ifidx, (u8)param1);
            break;
        case HGIC_CMD_GET_TEMPERATURE:
            ret = hgic_fwctrl_get_temperature(fwctrl);
            break;
        case HGIC_CMD_ENTER_SLEEP:
            if (param1) {
                if (!test_bit(HGIC_BUS_FLAGS_SLEEP, &fwctrl->bus->flags)) {
                    if (fwctrl->fwinfo->version < 0x02000000) { param1 = 0xffff; }
                    hgic_dbg("enter sleep : type:%d, sleep_ms:%d ...\r\n", param1, param2);
                    ret = hgic_fwctrl_enter_sleep(fwctrl, ifidx, param1, param2);
                    if (ret) {
                        clear_bit(HGIC_BUS_FLAGS_SLEEP, &fwctrl->bus->flags);
                        hgic_err("sleep fail, ret=%d\r\n", ret);
                    }
                }
            } else {
                hgic_dbg("exit sleep, SLEEP=%d\r\n", test_bit(HGIC_BUS_FLAGS_SLEEP, &fwctrl->bus->flags));
                if (test_bit(HGIC_BUS_FLAGS_SLEEP, &fwctrl->bus->flags)) {
                    clear_bit(HGIC_BUS_FLAGS_SLEEP, &fwctrl->bus->flags);
                    if (fwctrl->bus->reinit) {
                        fwctrl->bus->reinit(fwctrl->bus);
                    }
                    ret = hgic_fwctrl_enter_sleep(fwctrl, ifidx, 0, 0);
                    if (ret) {
                        hgic_err("exit sleep fail, ret=%d\r\n", ret);
                    }
                }
            }
            break;
        //HGIC_CMD_OTA,                   // 47
        case HGIC_CMD_GET_SSID:
            ret = hgic_fwctrl_get_ssid(fwctrl, ifidx, (u8 *)param1, param2);
            break;
        case HGIC_CMD_GET_WPA_PSK:
            ret = hgic_fwctrl_get_wpapsk(fwctrl, ifidx, (u8 *)param1, param2);
            break;
        case HGIC_CMD_SET_BEACON_INT:
            ret = hgic_fwctrl_set_beacon_int(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_GET_STA_LIST:
            ret = hgic_fwctrl_get_sta_list(fwctrl, ifidx, (u8 *)param1, param2);
            if (ret > 0) { ret /= 6; }
            break;
        case HGIC_CMD_SAVE_CFG:
            ret = hgic_fwctrl_save_cfg(fwctrl, ifidx);
            break;
        case HGIC_CMD_JOIN_GROUP:
            ret = hgic_fwctrl_join_group(fwctrl, ifidx, (u8 *)param1, param2);
            break;
        case HGIC_CMD_SET_ETHER_TYPE:
            ret = hgic_fwctrl_set_ethertype(fwctrl, ifidx, (u16)param1);
            break;
        case HGIC_CMD_GET_STA_COUNT:
            ret = hgic_fwctrl_get_sta_count(fwctrl, ifidx);
            break;
        case HGIC_CMD_SET_HEARTBEAT_INT:
            ret = hgic_fwctrl_set_beacon_int(fwctrl, ifidx, param1);
            break;
        //HGIC_CMD_SET_MCAST_KEY,         // 59
        case HGIC_CMD_SET_AGG_CNT:
            ret = hgic_fwctrl_set_agg_cnt(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_GET_AGG_CNT:
            ret = hgic_fwctrl_get_agg_cnt(fwctrl, ifidx,(u8 *)param1,param2);
            break;
        case HGIC_CMD_GET_BSS_BW:
            ret = hgic_fwctrl_get_bss_bw(fwctrl, ifidx);
            break;
        case HGIC_CMD_GET_FREQ_RANGE:
            ret = hgic_fwctrl_get_freq_range(fwctrl, ifidx, (u32 *)param1,
                                             (u32 *)(param1 + 4), (u32 *)(param1 + 8));
            break;
        case HGIC_CMD_GET_CHAN_LIST:
            ret = hgic_fwctrl_get_chan_list(fwctrl, ifidx, (u16 *)param1, param2);
            break;
        case HGIC_CMD_RADIO_ONOFF:
            ret = hgic_fwctrl_radio_onoff(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_SET_PS_HEARTBEAT:
            ret = hgic_fwctrl_set_ps_heartbeat(fwctrl, ifidx, ((u32 *)param1)[0],
                                               ((u32 *)param1)[1], ((u32 *)param1)[2], ((u32 *)param1)[3]);
            break;
        case HGIC_CMD_SET_WAKEUP_STA:
            ret = hgic_fwctrl_wakeup_sta(fwctrl, ifidx, (u8 *)param1);
            break;
        case HGIC_CMD_SET_PS_HEARTBEAT_RESP:
            ret = hgic_fwctrl_set_ps_heartbeat_resp(fwctrl, ifidx, (u8 *)param1, param2);
            break;
        case HGIC_CMD_SET_PS_WAKEUP_DATA:
            ret = hgic_fwctrl_set_ps_wakeup_data(fwctrl, ifidx, (u8 *)param1, param2);
            break;
        case HGIC_CMD_SET_PS_CONNECT:
            ret = hgic_fwctrl_set_ps_connect(fwctrl, ifidx, param1, param2);
            break;
        case HGIC_CMD_SET_BSS_MAX_IDLE:
            ret = hgic_fwctrl_set_bss_max_idle(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_SET_WKIO_MODE:
            ret = hgic_fwctrl_set_wkio_mode(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_SET_DTIM_PERIOD:
            ret = hgic_fwctrl_set_dtim_period(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_SET_PS_MODE:
            ret = hgic_fwctrl_set_ps_mode(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_LOAD_DEF:
            ret = hgic_fwctrl_set_load_def(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_DISASSOC_STA:
            ret = hgic_fwctrl_disassoc_sta(fwctrl, ifidx, (u8 *)param1);
            break;
        case HGIC_CMD_SET_APLOST_TIME:
            ret = hgic_fwctrl_set_aplost_time(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_GET_WAKEUP_REASON:
            ret = hgic_fwctrl_get_wkreason(fwctrl, ifidx);
            break;
        case HGIC_CMD_UNPAIR:
            ret = hgic_fwctrl_unpair(fwctrl, ifidx, (u8 *)param1);
            break;
        case HGIC_CMD_SET_AUTO_CHAN_SWITCH:
            ret = hgic_fwctrl_set_auto_chswitch(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_SET_REASSOC_WKHOST:
            ret = hgic_fwctrl_set_reassoc_wkhost(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_SET_WAKEUP_IO:
            ret = hgic_fwctrl_set_wakeup_io(fwctrl, ifidx, param1, param2);
            break;
        case HGIC_CMD_DBGINFO_OUTPUT:
            ret = hgic_fwctrl_set_dbginfo_output(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_SET_SYSDBG:
            ret = hgic_fwctrl_set_sysdbg(fwctrl, ifidx, (u8 *)param1);
            break;
        case HGIC_CMD_SET_AUTO_SLEEP_TIME:
            ret = hgic_fwctrl_set_autosleep_time(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_GET_KEY_MGMT:
            ret = hgic_fwctrl_get_key_mgmt(fwctrl, ifidx, (u8 *)param1, param2);
            break;
        case HGIC_CMD_SET_PAIR_AUTOSTOP:
            ret = hgic_fwctrl_set_pair_autostop(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_SET_SUPER_PWR:
            ret = hgic_fwctrl_set_super_pwr(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_SET_REPEATER_SSID:
            ret = hgic_fwctrl_set_repeater_ssid(fwctrl, ifidx, (u8 *)param1);
            break;
        case HGIC_CMD_SET_REPEATER_PSK:
            ret = hgic_fwctrl_set_repeater_psk(fwctrl, ifidx, (u8 *)param1);
            break;
        case HGIC_CMD_CFG_AUTO_SAVE:
            ret = hgic_fwctrl_set_auto_save(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_SET_BLENC_EN:
            val[0] = param1;
            val[1] = param2;
            ret = hgic_fwctrl_set_blenc_en(fwctrl, ifidx, val);
            break;
        case HGIC_CMD_SEND_BLENC_DATA:
            ret = hgic_fwctrl_blenc_send_data(fwctrl, ifidx, (u8 *)param1, param2);
            break;
        case HGIC_CMD_SEND_BLENC_DEVADDR:
            ret = hgic_fwctrl_blenc_set_devaddr(fwctrl, ifidx, (u8 *)param1);
            break;
        case HGIC_CMD_SEND_BLENC_ADVDATA:
            ret = hgic_fwctrl_blenc_set_advdata(fwctrl, ifidx, (u8 *)param1, param2);
            break;
        case HGIC_CMD_SEND_BLENC_SCANRESP:
            ret = hgic_fwctrl_blenc_set_scanresp(fwctrl, ifidx, (u8 *)param1, param2);
            break;
        case HGIC_CMD_SEND_BLENC_ADVINTERVAL:
            ret = hgic_fwctrl_blenc_set_advinterval(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_SEND_BLENC_STARTADV:
            ret = hgic_fwctrl_blenc_start_adv(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_SET_BLENC_ADVFILTER:
            ret = hgic_fwctrl_set_blenc_adv_filter(fwctrl, ifidx, param1);
            break;
        case HGIC_CMD_SEND_BLENC_HCI_DATA:
            hgic_dbg("Check param:type:%d,data:%p,len:%d\n",
                *(u32 *)param1, *(u32 *)(param1 + 4), *(u32 *)(param1 + 8));
            ret = hgic_fwctrl_send_hci_data(fwctrl, *(u32 *)param1, *(u32 *)(param1 + 4), *(u32 *)(param1 + 8));
        default:
            hgic_err("not support cmd: %d\r\n", cmd);
            break;
    }
    return ret;
}

#endif

