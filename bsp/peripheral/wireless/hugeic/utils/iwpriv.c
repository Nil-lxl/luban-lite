#include <linux/defs.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>

#include "hgic.h"
#include "fwctrl.h"

#define __packed            __attribute__((packed))
#define le16_to_cpu(p)      (p)

#define HCI_OPCODE(ogf, ocf) ((ocf) | ((ogf) << 10))

#define MACSTR            "%02x:%02x:%02x:%02x:%02x:%02x"
#define MACARG(a)         (a)[0]&0xff, (a)[1]&0xff, (a)[2]&0xff, (a)[3]&0xff, (a)[4]&0xff, (a)[5]&0xff
#define IPSTR             "%d.%d.%d.%d"
#define IPARG(a)          ((a)>>24)&0xff, ((a)>>16)&0xff, ((a)>>8)&0xff, (a)&0xff
#define MAC2STR(mac, str) (sprintf((str), MACSTR, MACARG(mac)))
#define STR_EQ(s1,s2)     (strcmp(s1,s2)==0)
#define MAC_EQ(a1,a2)     (memcmp(a1,a2,6)==0)
#define MAX(a,b)          ((a)>(b)?(a):(b))

#define WLAN_FC_TODS        0x0100
#define WLAN_FC_FROMDS      0x0200
struct ieee80211_hdr {
    uint16 frame_control;
    uint16 duration_id;
    uint8 addr1[6];
    uint8 addr2[6];
    uint8 addr3[6];
    uint16 seq_ctrl;
};

enum FMAC_CONNECT_REASON {
    FMAC_ASSOC_SUCCESS = 0,
    FMAC_ASSOC_REFUSED = 1,
    FMAC_ASSOC_DENIED_NO_MORE_STAS = 17,
    FMAC_ASSOC_DENIED_INSUFFICIENT_BANDWIDTH = 33,
    FMAC_ASSOC_REFUSED_AP_OUT_OF_MEMORY = 93,
    FMAC_ASSOC_NO_AP = 0xff
};

enum HGIC_MODULE_TYPE {
    HGIC_MODULE_TYPE_700M = 1,
    HGIC_MODULE_TYPE_900M = 2,
    HGIC_MODULE_TYPE_860M = 3,
    HGIC_MODULE_TYPE_810M = 5,
};

char *HGIC = "hgics";
extern int   blenc_mode;

unsigned long hgic_fls(int x)
{
    int r = 32;

    if (!x) {
        return 0;
    }

    if (!(x & 0xffff0000u)) {
        x <<= 16;
        r -= 16;
    }
    if (!(x & 0xff000000u)) {
        x <<= 8;
        r -= 8;
    }
    if (!(x & 0xf0000000u)) {
        x <<= 4;
        r -= 4;
    }
    if (!(x & 0xc0000000u)) {
        x <<= 2;
        r -= 2;
    }
    if (!(x & 0x80000000u)) {
        x <<= 1;
        r -= 1;
    }
    return r;
}

void hgic_strcpy(char *dest, char *src, int cnt)
{
    int i = 0;
    while (*src && *src != '\r' && *src != '\n') {
        *dest++ = *src++;
        if (cnt && ++i >= cnt) {
            break;
        }
    }
    *dest = 0;
}

char *hgic_strchr(char *str, char s, int index)
{
    int i = 0;

    while (*str && i != index) {
        if (*str == s) {
            i++;
        }
        str++;
    }

    return (i && i == index) ? str : NULL;
}

void hgic_dump_hex(char *str, char *data, int len, int newline)
{
    int i = 0;
    if (data && len) {
        if (str) {
            PRINTF("%s", str);
        }

        for (i = 0; i < len; i++) {
            if (i > 0 && newline) {
                if ((i & 0x7) == 0) { PRINTF("   "); }
                if ((i & 0xf) == 0) { PRINTF("\r\n"); }
            }
            PRINTF("%02x ", data[i] & 0xFF);
        }

        PRINTF("\r\n");
    }
}

char *hgic_hw_state(int state)
{
    switch (state) {
        case HGICF_HW_DISCONNECTED:
            return "Disconnect";
        case  HGICF_HW_DISABLED:
            return "DISABLED";
        case  HGICF_HW_INACTIVE:
            return "INACTIVE";
        case  HGICF_HW_SCANNING:
            return "SCANNING";
        case  HGICF_HW_AUTHENTICATING:
            return "AUTHENTICATING";
        case  HGICF_HW_ASSOCIATING:
            return "ASSOCIATING";
        case  HGICF_HW_ASSOCIATED:
            return "ASSOCIATED";
        case  HGICF_HW_4WAY_HANDSHAKE:
            return "4WAY_HANDSHAKE";
        case  HGICF_HW_GROUP_HANDSHAKE:
            return "GROUP_HANDSHAKE";
        case  HGICF_HW_CONNECTED:
            return "CONNECTED";
        default:
            return "Unknown";
    }
}

int hgic_str2mac(const char *mac_str, unsigned char *mac)
{
    int tmp[6];
    if (mac_str && mac) {
        memset(tmp, 0, sizeof(tmp));
        memset(mac, 0, 6);
        if (6 == sscanf(mac_str, MACSTR, &tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4], &tmp[5])) {
            mac[0] = (unsigned char)tmp[0];
            mac[1] = (unsigned char)tmp[1];
            mac[2] = (unsigned char)tmp[2];
            mac[3] = (unsigned char)tmp[3];
            mac[4] = (unsigned char)tmp[4];
            mac[5] = (unsigned char)tmp[5];
            return 1;
        }
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

int hgic_iwpriv_open_dev(char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_open_dev(ndev->ctrl, ndev->fwifidx);
}
int hgic_iwpriv_close_dev(char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_close_dev(ndev->ctrl, ndev->fwifidx);
}
int hgic_iwpriv_set_ssid(char *ifname, char *ssid)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if(ssid == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_set_ssid(ndev->ctrl, ndev->fwifidx, ssid);
}
int hgic_iwpriv_set_bssid(char *ifname, char *bssid)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if(bssid == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_set_bssid(ndev->ctrl, ndev->fwifidx, bssid);
}
int hgic_iwpriv_set_channel(char *ifname, int channel)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_channel(ndev->ctrl, ndev->fwifidx, channel);
}
int hgic_iwpriv_set_keymgmt(char *ifname, char *key_mgmt)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if(key_mgmt == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_set_key_mgmt(ndev->ctrl, ndev->fwifidx, key_mgmt);
}
int hgic_iwpriv_set_wpapsk(char *ifname, char *wpa_psk)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if(wpa_psk == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_set_wpa_psk(ndev->ctrl, ndev->fwifidx, wpa_psk);
}
int hgic_iwpriv_set_freqrange(char *ifname, int freq_start, int freq_end, int bw)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_freq_range(ndev->ctrl, ndev->fwifidx, freq_start, freq_end, bw);
}
int hgic_iwpriv_set_bss_bw(char *ifname, int bss_bw)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_bss_bw(ndev->ctrl, ndev->fwifidx, bss_bw);
}
int hgic_iwpriv_set_tx_bw(char *ifname, int tx_bw)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_tx_bw(ndev->ctrl, ndev->fwifidx, tx_bw);
}
int hgic_iwpriv_set_tx_mcs(char *ifname, int tx_mcs)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_tx_mcs(ndev->ctrl, ndev->fwifidx, tx_mcs);
}
int hgic_iwpriv_set_max_txcnt(char *ifname, int max_txcnt)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_frm_tx_maxcnt(ndev->ctrl, ndev->fwifidx, max_txcnt);
}
int hgic_iwpriv_set_acs(char *ifname, int start, int tmo)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_acs(ndev->ctrl, ndev->fwifidx,start,tmo);
}
int hgic_iwpriv_set_chan_list(char *ifname, unsigned short *chan_list, int chan_count)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if(chan_list == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_set_chan_list(ndev->ctrl, ndev->fwifidx, chan_list, chan_count);
}
int hgic_iwpriv_set_mode(char *ifname, char *mode)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if(mode == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_set_mode(ndev->ctrl, ndev->fwifidx, mode);
}
int hgic_iwpriv_set_paired_stas(char *ifname, char *paired_stas)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if(paired_stas == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_set_paired_stas(ndev->ctrl, ndev->fwifidx, paired_stas, strlen(paired_stas));
}
int hgic_iwpriv_set_pairing(char *ifname, int pair_number)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_pairing(ndev->ctrl, ndev->fwifidx, pair_number);
}
int hgic_iwpriv_set_beacon_int(char *ifname, int beacon_int)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_beacon_int(ndev->ctrl, ndev->fwifidx, beacon_int);
}
int hgic_iwpriv_set_radio_onoff(char *ifname, int off)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_radio_onoff(ndev->ctrl, ndev->fwifidx, off);
}
int hgic_iwpriv_set_join_group(char *ifname, char *mcast_addr, int aid)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if(mcast_addr == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_join_group(ndev->ctrl, ndev->fwifidx, mcast_addr, aid);
}
int hgic_iwpriv_set_txpower(char *ifname, int txpower)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_txpower(ndev->ctrl, ndev->fwifidx, txpower);
}
int hgic_iwpriv_set_ps_connect(char *ifname, int period, int roundup)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }

    return hgic_fwctrl_set_ps_connect(ndev->ctrl, ndev->fwifidx, period, roundup);
}
int hgic_iwpriv_set_bss_max_idle(char *ifname, int bss_max_idle)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_bss_max_idle(ndev->ctrl, ndev->fwifidx, bss_max_idle);
}
int hgic_iwpriv_set_wkio_mode(char *ifname, int wkio_mode)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_wkio_mode(ndev->ctrl, ndev->fwifidx, wkio_mode);
}
int hgic_iwpriv_set_dtim_period(char *ifname, int dtim_period)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_dtim_period(ndev->ctrl, ndev->fwifidx, dtim_period);
}
int hgic_iwpriv_set_ps_mode(char *ifname, int ps_mode)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_ps_mode(ndev->ctrl, ndev->fwifidx, ps_mode);
}
int hgic_iwpriv_set_aplost_time(char *ifname, int aplost_time)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_aplost_time(ndev->ctrl, ndev->fwifidx, aplost_time);
}
int hgic_iwpriv_unpair(char *ifname, char *mac)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if(mac == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_unpair(ndev->ctrl, ndev->fwifidx, mac);
}
int hgic_iwpriv_scan(char *ifname,u8 scan_cmd)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_scan(ndev->ctrl, ndev->fwifidx,scan_cmd);
}
int hgic_iwpriv_save(char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_save_cfg(ndev->ctrl, ndev->fwifidx);
}
int hgic_iwpriv_set_auto_chswitch(char *ifname, int enable)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_auto_chswitch(ndev->ctrl, ndev->fwifidx, enable);
}
int hgic_iwpriv_set_mcast_key(char *ifname, char *mcast_key)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if(mcast_key == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_set_mcast_key(ndev->ctrl, ndev->fwifidx, mcast_key);
}
int hgic_iwpriv_set_reassoc_wkhost(char *ifname, int enable)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_reassoc_wkhost(ndev->ctrl, ndev->fwifidx, enable);
}
int hgic_iwpriv_set_wakeup_io(char *ifname, int wakeup_io, char edge)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_wakeup_io(ndev->ctrl, ndev->fwifidx, wakeup_io, edge);
}
int hgic_iwpriv_set_dbginfo(char *ifname, int enable)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_dbginfo_output(ndev->ctrl, ndev->fwifidx, enable);
}
int hgic_iwpriv_set_sysdbg(char *ifname, char *sysdbg)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if(sysdbg == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_set_sysdbg(ndev->ctrl, ndev->fwifidx, sysdbg);
}
int hgic_iwpriv_set_primary_chan(char *ifname, int primary_chan)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_primary_chan(ndev->ctrl, ndev->fwifidx, primary_chan);
}
int hgic_iwpriv_set_autosleep_time(char *ifname, int autosleep_time)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_autosleep_time(ndev->ctrl, ndev->fwifidx, autosleep_time);
}
int hgic_iwpriv_set_super_pwr(char *ifname, int enable)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_super_pwr(ndev->ctrl, ndev->fwifidx, enable);
}
int hgic_iwpriv_set_repeater_ssid(char *ifname, char *r_ssid)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if(r_ssid == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_set_repeater_ssid(ndev->ctrl, ndev->fwifidx, r_ssid);
}
int hgic_iwpriv_set_repeater_psk(char *ifname, char *r_psk)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if(r_psk == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_set_repeater_psk(ndev->ctrl, ndev->fwifidx, r_psk);
}
int hgic_iwpriv_set_auto_save(char *ifname, int enable)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_auto_save(ndev->ctrl, ndev->fwifidx, enable);
}
int hgic_iwpriv_set_pair_autostop(char *ifname, int enable)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_pair_autostop(ndev->ctrl, ndev->fwifidx, enable);
}
int hgic_iwpriv_set_dcdc13(char *ifname, int enable)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_dcdc13v(ndev->ctrl, ndev->fwifidx, enable);
}
int hgic_iwpriv_set_acktmo(char *ifname, int acktmo)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_acktmo(ndev->ctrl, ndev->fwifidx, acktmo);
}
int hgic_iwpriv_get_sta_list(char *ifname, char *buff, int size)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if(buff == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_get_sta_list(ndev->ctrl, ndev->fwifidx, buff, size);//RETURN COUNT
}
int hgic_iwpriv_get_scan_list(char *ifname, char *buff, int size)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if(buff == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_get_scan_list(ndev->ctrl, ndev->fwifidx, buff, size);
}
int hgic_iwpriv_get_ssid(char *ifname, char *buff, int size)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if(buff == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_get_ssid(ndev->ctrl, ndev->fwifidx, buff, size);
}
int hgic_iwpriv_get_bssid(char *ifname, char *mac)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if(mac == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_get_bssid(ndev->ctrl, ndev->fwifidx, mac, 6);
}
int hgic_iwpriv_get_wpa_psk(char *ifname, char *buff, int size)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if(buff == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_get_wpapsk(ndev->ctrl, ndev->fwifidx, buff, size);
}
int hgic_iwpriv_get_txpower(char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_get_txpower(ndev->ctrl, ndev->fwifidx);
}
int hgic_iwpriv_get_aggcnt(char *ifname, int *tx_agg, int *rx_agg)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    char buff[32] = {0};
    int  ret = 0;

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if(tx_agg == NULL || rx_agg == NULL) {
        return -EINVAL;
    }
    ret =  hgic_fwctrl_get_agg_cnt(ndev->ctrl, ndev->fwifidx, buff, sizeof(buff));
    if (ret > 0) {
        sscanf(buff, "tx:%d,rx:%d", tx_agg, rx_agg);
        return 0;
    }
    return -1;
}
int hgic_iwpriv_set_aggcnt(char *ifname, u8 tx_agg, u8 rx_agg)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    u8 agg[2] = {0};
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    agg[0] = tx_agg;
    agg[1] = rx_agg;
    return hgic_fwctrl_set_agg_cnt(ndev->ctrl, ndev->fwifidx, agg);
}
int hgic_iwpriv_set_load_def(char *ifname, int reboot)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_load_def(ndev->ctrl, ndev->fwifidx, reboot);
}
int hgic_iwpriv_set_dbginfo_output(char *ifname, int enable)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_dbginfo_output(ndev->ctrl, ndev->fwifidx, enable);
}
int hgic_iwpriv_set_unpair(char *ifname, char *sta_mac)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if(sta_mac == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_unpair(ndev->ctrl, ndev->fwifidx, sta_mac);
}
int hgic_iwpriv_get_bss_bw(char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_get_bss_bw(ndev->ctrl, ndev->fwifidx);
}

int hgic_iwpriv_get_chan_list(char *ifname, char *buff, int size)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (buff == NULL || size < sizeof(u16)) {
        return -EINVAL;
    }
    return hgic_fwctrl_get_chan_list(ndev->ctrl, ndev->fwifidx,
                                     buff, (size / sizeof(u16)));
}

int hgic_iwpriv_get_freq_range(char *ifname, char *buff, int size)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    u32 vals[3] = {0};
    u8  str[32] = {0};
    s32 ret = 0;

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    ret = hgic_fwctrl_get_freq_range(ndev->ctrl, ndev->fwifidx, &vals[0], &vals[1], &vals[2]);
    if (ret) {
        sprintf(str, "%d,%d,%d", vals[0], vals[1], vals[2]);
        memset(buff, 0, size);
        strncpy(buff, str, size > strlen(str) ? strlen(str) : size);
    }
    return 0;
}
int hgic_iwpriv_get_txq_param(char *ifname, char *buff, int size)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    struct hgic_txq_param param [4] = {0};
    int ret = 0;

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (buff == NULL) {
        return -EINVAL;
    }
    ret = hgic_fwctrl_get_txq_param(ndev->ctrl, ndev->fwifidx, &param);
    if (ret == sizeof(param)) {
        memcpy(buff, &param, size > sizeof(param) ? sizeof(param) : size);
    }
    return (ret == sizeof(param)) ? 0 : ret;
}
int hgic_iwpriv_get_key_mgmt(char *ifname, char *buff, int size)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    int len = 0;
    char str[32] = {0};

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (buff == NULL) {
        return -EINVAL;
    }
    len = hgic_fwctrl_get_key_mgmt(ndev->ctrl, ndev->fwifidx, str, sizeof(str));
    memcpy(buff, str, size > sizeof(str) ? sizeof(str) : size);
    return 0;
}
int hgic_iwpriv_get_battery_level(char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_get_battery_level(ndev->ctrl, ndev->fwifidx);
}
int hgic_iwpriv_get_module_type(char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_get_module_type(ndev->ctrl, ndev->fwifidx);
}
int hgic_iwpriv_set_pa_pwrctrl_dis(char *ifname, int disable)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_pa_pwrctl_dis(ndev->ctrl, ndev->fwifidx, disable);
}
int hgic_iwpriv_set_dhcpc(char *ifname, int enable)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_dhcpc(ndev->ctrl, ndev->fwifidx, enable);
}
int hgic_iwpriv_get_disassoc_reason(char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_get_disassoc_reason(ndev->ctrl, ndev->fwifidx);
}
int hgic_iwpriv_set_wkdata_save(char *ifname, int enable)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_wkdata_save(ndev->ctrl, ndev->fwifidx, enable);
}
int hgic_iwpriv_set_mcast_txparam(char *ifname, int dupcnt, int tx_bw, int tx_mcs, int clearch)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    struct hgic_mcast_txparam txparam;

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    memset(&txparam, 0, sizeof(struct hgic_mcast_txparam));
    txparam.dupcnt = dupcnt;
    txparam.tx_bw  = tx_bw;
    txparam.tx_mcs = tx_mcs;
    txparam.clearch = clearch;
    return hgic_fwctrl_set_mcast_txparam(ndev->ctrl, ndev->fwifidx, &txparam);
}
int hgic_iwpriv_reset_sta(char *ifname, char *mac_addr)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (mac_addr == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_reset_sta(ndev->ctrl, ndev->fwifidx, mac_addr);
}
int hgic_iwpriv_ant_auto(char *ifname, int en)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_ant_auto(ndev->ctrl, ndev->fwifidx, en);
}
int hgic_iwpriv_set_ant_sel(char *ifname, int ant)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_select_ant(ndev->ctrl, ndev->fwifidx, ant);
}
int hgic_iwpriv_get_ant_sel(char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_get_ant_sel(ndev->ctrl, ndev->fwifidx);
}
int hgic_iwpriv_set_macfilter(char *ifname, int enable)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_mac_filter(ndev->ctrl, ndev->fwifidx, enable);
}
int hgic_iwpriv_send_atcmd(char *ifname, char *atcmd)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (atcmd == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_set_atcmd(ndev->ctrl, ndev->fwifidx, atcmd);
}
int hgic_iwpriv_set_roaming(char *ifname, int enable, int same_freq, int threshold, int rssi_diff, int rssi_interval)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    int val[5] = {0};
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    val[0] = enable;
    val[1] = same_freq;
    val[2] = threshold;
    val[3] = rssi_diff;
    val[4] = rssi_interval;
    return hgic_fwctrl_set_roaming(ndev->ctrl, ndev->fwifidx, &val, sizeof(val));
}
int hgic_iwpriv_get_conn_state(char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_get_conn_state(ndev->ctrl, ndev->fwifidx);
}

int hgic_iwpriv_get_wkreason(char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_get_wkreason(ndev->ctrl, ndev->fwifidx);
}

int hgic_iwpriv_set_rts_threshold(char *ifname, int rts_threshold)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_rts_threshold(ndev->ctrl, ndev->fwifidx, rts_threshold);
}

int hgic_iwpriv_set_frag_threshold(char *ifname, int frag_threshold)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_frag_threshold(ndev->ctrl, ndev->fwifidx, frag_threshold);
}

int hgic_iwpriv_set_heartbeat(char *ifname, int ipaddr, int port, int period, int timeout)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_ps_heartbeat(ndev->ctrl, ndev->fwifidx, ipaddr, port, period, timeout);
}
int hgic_iwpriv_set_heartbeat_resp_data(char *ifname, char *resp_data, int len)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (resp_data == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_set_ps_heartbeat_resp(ndev->ctrl, ndev->fwifidx, resp_data, len);
}
int hgic_iwpriv_set_wakeup_data(char *ifname, char *wakeup_data, int len)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (wakeup_data == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_set_ps_wakeup_data(ndev->ctrl, ndev->fwifidx, wakeup_data, len);
}
int hgic_iwpriv_wakeup_sta(char *ifname, char *addr)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (addr == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_wakeup_sta(ndev->ctrl, ndev->fwifidx, addr);
}
int hgic_iwpriv_sleep(char *ifname, int sleep_type, unsigned int sleep_ms)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_enter_sleep(ndev->ctrl, ndev->fwifidx, sleep_type, sleep_ms);
}
int hgic_iwpriv_send_custmgmt(char *ifname, char *dest, struct hgic_tx_info *txinfo, char *data, int len)
{
    /* "data" is just payload data, the firmware will build WiFi management frame. */
    int ret = -1;
    char *buf = NULL;
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (txinfo == NULL || data == NULL) {
        return -EINVAL;
    }
    buf = MALLOC(len + sizeof(struct hgic_tx_info) + 6);
    if (buf) {
        memcpy(buf, dest, 6);
        memcpy(buf + 6, txinfo, sizeof(struct hgic_tx_info));
        memcpy(buf + 6 + sizeof(struct hgic_tx_info), data, len);
        ret = hgic_fwctrl_send_cust_mgmt(ndev->ctrl, ndev->fwifidx,
                                         buf, len + 6 + sizeof(struct hgic_tx_info));
        FREE(buf);
    }
    return ret;
}
int hgic_iwpriv_send_mgmtframe(char *ifname, struct hgic_tx_info *txinfo, char *mgmt, int len)
{
    /* "mgmt" should be a whole WiFi management frame. */
    int ret = -1;
    char *buf = NULL;
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (txinfo == NULL || mgmt == NULL) {
        return -EINVAL;
    }
    buf = MALLOC(len + sizeof(struct hgic_tx_info));
    if (buf) {
        memcpy(buf, txinfo, sizeof(struct hgic_tx_info));
        memcpy(buf + sizeof(struct hgic_tx_info), mgmt, len);
        ret = hgic_fwctrl_send_mgmtframe(ndev->ctrl, ndev->fwifidx, buf, len + sizeof(struct hgic_tx_info));
        FREE(buf);
    }
    return ret;
}
int hgic_iwpriv_read_wkdata_buff(char *ifname, char *buff, int len)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (buff == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_get_wkdata_buff(ndev->ctrl, ndev->fwifidx, buff, len);
}
int hgic_proc_set_wkdata_mask(char *ifname, unsigned short offset, char *mask, int mask_len)
{
    char buff[20] = {0};
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (mask == NULL) {
        return -EINVAL;
    }
    if (mask_len > 16) { mask_len = 16; }
    memcpy(buff, &offset, 2);
    memcpy(buff + 2, mask, mask_len);
    return hgic_proc_set_wkdata_mask(ndev->ctrl, ndev->fwifidx, buff, mask_len + 2);
}
int hgic_iwpriv_set_cust_driverdata(char *ifname, char *buff, int len)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (len > 1500) {
        PRINTF("data len:%d too big\r\n", len);
        return -1;
    }
    if (buff == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_set_cust_driver_data(ndev->ctrl, ndev->fwifidx, buff, len);
}
int hgic_iwpriv_set_stafreqinfo(char *ifname, char *sta_addr, struct hgic_freqinfo *freqinfo)
{
    int ret = -1;
    char *buff = NULL;
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    buff = MALLOC(6 + sizeof(struct hgic_freqinfo));
    if (buff) {
        memset(buff, 0, 6 + sizeof(struct hgic_freqinfo));
        if (sta_addr) { memcpy(buff, sta_addr, 6); }
        memcpy(buff + 6, freqinfo, sizeof(struct hgic_freqinfo));
        ret = hgic_fwctrl_set_freqinfo(ndev->ctrl, ndev->fwifidx, buff,
                                       6 + sizeof(struct hgic_freqinfo));
        FREE(buff);
    }
    return ret;
}

int hgic_iwpriv_blenc_start(char *ifname, int start, int channel)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    u8 val[2] = {0};
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    val[0] = start;
    val[1] = channel;

    return hgic_fwctrl_set_blenc_en(ndev->ctrl, ndev->fwifidx, &val);
}
int hgic_iwpriv_send_blenc_data(char *ifname, char *data, int len)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (data == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_blenc_send_data(ndev->ctrl, ndev->fwifidx, data, len);
}
int hgic_iwpriv_blenc_send_hcidata(char *ifname, int type, char *data, int len)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (data == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_send_hci_data(ndev->ctrl, type, data, len);
}

int hgic_iwpriv_blenc_set_advdata(char *ifname, char *adv_data, int len)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (adv_data == NULL) {
        return -EINVAL;
    }
    if (blenc_mode == 3) {
        int ret = -ENOMEM;
        int cmd_len = 3 + len;
        char *cmd = MALLOC(cmd_len);
        if (cmd) {
            put_unaligned_le16(HCI_OPCODE(0x08, 0x08), cmd);
            cmd[2] = len;
            memcpy(cmd + 3, adv_data, len);
            ret = hgic_fwctrl_send_hci_data(ndev->ctrl, 0x01, cmd, cmd_len);
            FREE(cmd);
        }
        return ret;
    } else {
        return hgic_fwctrl_blenc_set_advdata(ndev->ctrl, ndev->fwifidx, adv_data, len);
    }
}

int hgic_iwpriv_blenc_set_scanresp(char *ifname, char *scan_resp, int len)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (scan_resp == NULL) {
        return -EINVAL;
    }
    if (blenc_mode == 3) {
        int ret = -ENOMEM;
        int cmd_len = 3 + len;
        char *cmd = MALLOC(cmd_len);
        if (cmd) {
            put_unaligned_le16(HCI_OPCODE(0x08, 0x09), cmd);
            cmd[2] = len;
            memcpy(cmd + 3, scan_resp, len);
            ret = hgic_fwctrl_send_hci_data(ndev->ctrl, 0x01, cmd, cmd_len);
            FREE(cmd);
        }
        return ret;
    } else {
        return hgic_fwctrl_blenc_set_scanresp(ndev->ctrl, ndev->fwifidx, scan_resp, len);
    }
}

int hgic_iwpriv_blenc_set_devaddr(char *ifname, char *addr)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (addr == NULL) {
        return -EINVAL;
    }
    if (blenc_mode == 3) {
        int ret = -ENOMEM;
        int cmd_len = 3 + 6;
        char *cmd = MALLOC(cmd_len);
        if (cmd) {
            put_unaligned_le16(HCI_OPCODE(0x08, 0x05), cmd);
            cmd[2] = 6;
            memcpy(cmd + 3, addr, 6);
            ret = hgic_fwctrl_send_hci_data(ndev->ctrl, 0x01, cmd, cmd_len);
            FREE(cmd);
        }
        return ret;
    } else {
        return hgic_fwctrl_blenc_set_devaddr(ndev->ctrl, ndev->fwifidx, addr);
    }
}

int hgic_iwpriv_blenc_set_adv_interval(char *ifname, int interval)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (blenc_mode == 3) {
        return 0;
    } else {
        return hgic_fwctrl_blenc_set_advinterval(ndev->ctrl, ndev->fwifidx, interval);
    }
}

int hgic_iwpriv_blenc_set_adv_filter(char *ifname, int filter)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (blenc_mode == 3) {
        return 0;
    } else {
        return hgic_fwctrl_set_blenc_adv_filter(ndev->ctrl, ndev->fwifidx, filter);
    }
}

int hgic_iwpriv_blenc_start_adv(char *ifname, int start)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        PRINTF("can not find device!!!!!!!,ndev:%p ndev->ctrl:%p\n", ndev, ndev->ctrl);
        return -ENODEV;
    }
    if (blenc_mode == 3) {
        int ret = -ENOMEM;
        int cmd_len = 3 + 1;
        char *cmd = MALLOC(cmd_len);
        if (cmd) {
            put_unaligned_le16(HCI_OPCODE(0x08, 0x0A), cmd);
            cmd[2] = 1;
            cmd[3] = start;
            ret = hgic_fwctrl_send_hci_data(ndev->ctrl, 0x01, cmd, cmd_len);
            FREE(cmd);
        }
        return ret;
    } else {
        return hgic_fwctrl_blenc_start_adv(ndev->ctrl, ndev->fwifidx, start);
    }
}

int hgic_iwpriv_blenc_send_gatt_data(char *ifname, char *att_data, int len)
{
    int ret = -ENOMEM;
    char *data = NULL;//MALLOC(8 + len);
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (att_data == NULL) {
        return -EINVAL;
    }
    data = MALLOC(8 + len);
    if (data) {
        //hgic_dump_hex("SEND:\r\n", att_data, len, 1);
        // 0 - Connection handle : PB=pb : BC=00
        put_unaligned_le16(0x1, data);
        // 2 - ACL length
        put_unaligned_le16(len + 4u, data + 2);
        // 4 - L2CAP packet length
        put_unaligned_le16(len + 0u, data + 4);
        // 6 - L2CAP CID
        put_unaligned_le16(0x04, data + 6);
        memcpy(data + 8, att_data, len);
        ret = hgic_fwctrl_send_hci_data(ndev->ctrl, 0x02, data, 8 + len);
        FREE(data);
    }
    return ret;
}

int hgic_iwpriv_set_hwscan(char *ifname, int period, int chan_tmo, int chan_bitmap, int max_cnt)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_hwscan(ndev->ctrl, ndev->fwifidx, period, chan_tmo, chan_bitmap, max_cnt);
}
int hgic_proc_ota(char *ifname, char *fw_file)
{
    return hgic_ota_start(ifname, fw_file);
}
int hgic_iwpriv_set_user_edca(char *ifname, int ac, struct hgic_txq_param *txq)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if(txq == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_set_user_edca(ndev->ctrl, ndev->fwifidx, ac, txq);
}
int hgic_iwpriv_set_fix_txrate(char *ifname, unsigned int txrate)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_fix_txrate(ndev->ctrl, ndev->fwifidx, txrate);
}
int hgic_iwpriv_set_nav_max(char *ifname, unsigned int nav_max)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_nav_max(ndev->ctrl, ndev->fwifidx, nav_max);
}
int hgic_iwpriv_clear_nav(char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_clear_nav(ndev->ctrl, ndev->fwifidx);
}
int hgic_iwpriv_set_cca_param(char *ifname, struct hgic_cca_ctl *cca)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (cca == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_set_cca_param(ndev->ctrl, ndev->fwifidx, cca);
}
int hgic_iwpriv_set_tx_modulation_gain(char *ifname, char *gain_table, int size)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (gain_table == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_set_tx_modulation_gain(ndev->ctrl, ndev->fwifidx, gain_table, size);
}
int hgic_iwpriv_get_nav(char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_get_nav(ndev->ctrl, ndev->fwifidx);
}

int hgic_iwpriv_get_bgrssi(char *ifname, int channel, int bgrssi[3])
{
    char resp[3] = {0};
    int ret = 0;
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    ret = hgic_fwctrl_get_bgrssi(ndev->ctrl, ndev->fwifidx, channel, resp);
    if (ret = 3) {
        bgrssi[0] = resp[0];
        bgrssi[1] = resp[1];
        bgrssi[2] = resp[2];
        return 0;
    }
    return -1;
}

int hgic_iwpriv_reset(char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_reset(ndev->ctrl, ndev->fwifidx);
}

int hgic_iwpriv_set_rts_duration(char *ifname, unsigned int duration_us)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_rts_duration(ndev->ctrl, ndev->fwifidx, duration_us);
}

int hgic_iwpriv_set_disable_print(char *ifname, int disable)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_disable_print(ndev->ctrl, ndev->fwifidx, disable);
}

int hgic_iwpriv_set_conn_paironly(char *ifname, int en)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_conn_paironly(ndev->ctrl, ndev->fwifidx, en);
}

int hgic_iwpriv_get_center_freq(char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_get_center_freq(ndev->ctrl, ndev->fwifidx);
}

int hgic_iwpriv_set_wait_psmode(char *ifname, int mode)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_wait_psmode(ndev->ctrl, ndev->fwifidx, mode);
}

int hgic_iwpriv_set_diffcust_conn(char *ifname, int en)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_diffcust_conn(ndev->ctrl, ndev->fwifidx, en);
}

int hgic_iwpriv_set_standby(char *ifname, int channel, int sleep_period)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_standby(ndev->ctrl, ndev->fwifidx, channel, sleep_period);
}

int hgic_iwpriv_set_ap_chan_switch(char *ifname, int channel, int counter)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_ap_chan_switch(ndev->ctrl, ndev->fwifidx, channel, counter);
}

int hgic_iwpriv_set_cca_for_ce(char *ifname, int enable)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_cca_for_ce(ndev->ctrl, ndev->fwifidx, enable);
}

int hgic_iwpriv_set_rtc(char *ifname, int rtc)
{
    struct net_device *ndev = net_device_get_by_name(ifname);

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_rtc(ndev->ctrl, ndev->fwifidx, rtc);
}

int hgic_iwpriv_get_rtc(char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    int rtc = 0;
    int ret = 0;
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    ret = hgic_fwctrl_get_rtc(ndev->ctrl, ndev->fwifidx, &rtc);
    if (ret > 0)
    { return rtc; }
    else
    { return ret; }
}

int hgic_iwpriv_set_apep_padding(char *ifname, int enable)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_apep_padding(ndev->ctrl, ndev->fwifidx, enable);
}

int hgic_iwpriv_set_freqinfo(char *ifname, char *sta_addr, struct hgic_freqinfo *freqinfo)
{
    int ret = -1;
    char *data = MALLOC(6 + sizeof(struct hgic_freqinfo));
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (freqinfo == NULL) {
        return -EINVAL;
    }
    if (data) {
        memcpy(data, sta_addr, 6);
        memcpy(data + 6, freqinfo, sizeof(struct hgic_freqinfo));
        ret = hgic_fwctrl_set_freqinfo(ndev->ctrl, ndev->fwifidx, data,
                                       6 + sizeof(struct hgic_freqinfo));
        FREE(data);
    }
    return ret;
}

int hgic_iwpriv_get_acs_result(char *ifname, char *buff, int len)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if(buff == NULL || len < sizeof(struct hgic_acs_result)) {
        return -EINVAL;
    }
    return hgic_fwctrl_get_acs_result(ndev->ctrl, ndev->fwifidx, buff,
                                      (len / sizeof(struct hgic_acs_result)));
}

int hgic_iwpriv_get_reason_code(char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_get_reason_code(ndev->ctrl, ndev->fwifidx);
}

int hgic_iwpriv_get_status_code(char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_get_status_code(ndev->ctrl, ndev->fwifidx);
}

int hgic_iwpriv_set_watchdog(char *ifname, int enable)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_watchdog(ndev->ctrl, ndev->fwifidx, enable);
}

int hgic_iwpriv_set_retry_fallback_cnt(char *ifname, int count)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_retry_fallback_cnt(ndev->ctrl, ndev->fwifidx, count);
}

int hgic_iwpriv_set_fallback_mcs(char *ifname, int original_type, int original_mcs, int fallback_type, int fallback_mcs)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    struct hgic_fallback_mcs mcs;
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    memset(&mcs, 0, sizeof(struct hgic_fallback_mcs));
    mcs.original_mcs  = original_type;
    mcs.original_mcs  = original_mcs;
    mcs.fallback_type = fallback_type;
    mcs.fallback_mcs  = fallback_mcs;
    return hgic_fwctrl_set_fallback_mcs(ndev->ctrl, ndev->fwifidx, &mcs);
}

int hgic_iwpriv_get_xosc(char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_get_xosc(ndev->ctrl, ndev->fwifidx);
}

int hgic_iwpriv_get_freq_offset(char *ifname, char *mac_addr)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    char mac[6] = {0};
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (mac_addr == NULL) {
        PRINTF("invalid mac address\r\n");
        return -1;
    }
    if (strlen(mac_addr) == 17 && mac_addr[2] == ':' && mac_addr[5] == ':' &&
        mac_addr[8] == ':' && mac_addr[11] == ':' && mac_addr[14] == ':') {
            hgic_str2mac(mac_addr, mac);
            return hgic_fwctrl_get_freq_offset(ndev->ctrl, ndev->fwifidx, mac);
    } else {
        return hgic_fwctrl_get_freq_offset(ndev->ctrl, ndev->fwifidx, mac_addr);
    }

}

int hgic_iwpriv_set_xosc(char *ifname, int xosc)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_xosc(ndev->ctrl, ndev->fwifidx, xosc);
}

int hgic_iwpriv_set_freq_cali_period(char *ifname, int period)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_freq_cali_period(ndev->ctrl, ndev->fwifidx, period);
}

int hgic_iwpriv_set_customer_dvrdata(char *ifname, unsigned short cmd_id, char *data, int len)
{
    int ret = 0;
    char *buff = NULL;
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (data == NULL) {
        return -EINVAL;
    }
    buff = MALLOC(2 + len);
    if (buff) {
        put_unaligned_le16(cmd_id, buff);
        memcpy(buff + 2, data, len);
        ret = hgic_fwctrl_set_cust_driver_data(ndev->ctrl, ndev->fwifidx, buff, len + 2);
        FREE(buff);
        return ret;
    }
    return -ENOMEM;
}

int hgic_iwpriv_get_fwinfo(char *ifname, struct hgic_fw_info *fwinfo)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (fwinfo == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_get_fwinfo(ndev->ctrl, ndev->fwifidx, fwinfo);
}

int hgic_iwpriv_set_disassoc_sta(char *ifname, char *sta_mac)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (sta_mac == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_disassoc_sta(ndev->ctrl, ndev->fwifidx, sta_mac);
}

int hgic_iwpriv_set_pa_pwrctl_dis(char *ifname, int disable)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_pa_pwrctl_dis(ndev->ctrl, ndev->fwifidx, disable);
}

int hgic_iwpriv_set_ant_auto(char *ifname, int enable)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_ant_auto(ndev->ctrl, ndev->fwifidx, enable);
}

int hgic_iwpriv_set_select_ant(char *ifname, int ant)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_select_ant(ndev->ctrl, ndev->fwifidx, ant);
}

int hgic_iwpriv_set_wkhost_reasons(char *ifname, int *reasons_array, int array_count)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (reasons_array == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_set_wkhost_reasons(ndev->ctrl, ndev->fwifidx, reasons_array, array_count);
}
int hgic_iwpriv_set_atcmd(char *ifname, char *atcmd)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (atcmd == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_set_atcmd(ndev->ctrl, ndev->fwifidx, atcmd);
}
int hgic_iwpriv_set_ap_hide(char *ifname, int hide)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_ap_hide(ndev->ctrl, ndev->fwifidx, hide);
}
int hgic_iwpriv_set_assert_holdup(char *ifname, int hold)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_assert_holdup(ndev->ctrl, ndev->fwifidx, hold);
}
int hgic_iwpriv_set_ap_psmode(char *ifname, int ap_psmode)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_ap_psmode_en(ndev->ctrl, ndev->fwifidx, ap_psmode);
}
int hgic_iwpriv_set_dupfilter_en(char *ifname, int enable)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_dupfilter_en(ndev->ctrl, ndev->fwifidx, enable);
}
int hgic_iwpriv_set_dis_1v1m2u(char *ifname, int disable)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_1v1_m2u_dis(ndev->ctrl, ndev->fwifidx, disable);
}
int hgic_iwpriv_set_dis_psconnect(char *ifname, int disable)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_psconnect_dis(ndev->ctrl, ndev->fwifidx, disable);
}
int hgic_iwpriv_set_wkdata_mask(char *ifname, int offset/*from IP hdr*/, char *mask, int mask_len)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if(mask == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_set_wkdata_mask(ndev->ctrl, ndev->fwifidx, offset, mask, mask_len);
}
int hgic_iwpriv_get_mode(char *ifname, char *mode)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if(mode == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_get_mode(ndev->ctrl, ndev->fwifidx, mode, 12);
}
int hgic_iwpriv_get_wkdata_buff(char *ifname, char *buff, int buff_size)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if(buff == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_get_wkdata_buff(ndev->ctrl, ndev->fwifidx, buff, buff_size);
}
int hgic_iwpriv_get_temperature(char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_get_temperature(ndev->ctrl);
}
int hgic_iwpriv_get_sta_count(char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_get_sta_count(ndev->ctrl, ndev->fwifidx);
}
int hgic_iwpriv_get_dhcpc_result(char *ifname, struct hgic_dhcp_result *result)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if(result == NULL) {
        return -EINVAL;
    }
    return hgic_fwctrl_get_dhcpc_result(ndev->ctrl, ndev->fwifidx,
                                        result, sizeof(struct hgic_dhcp_result));
}

int hgic_iwpriv_set_max_tx_delay(char *ifname, int tmo)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_max_tx_delay(ndev->ctrl, ndev->fwifidx, tmo);
}

int hgic_iwpriv_set_heartbeat_int(char *ifname, int interval_ms)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_heartbeat_int(ndev->ctrl, ndev->fwifidx, interval_ms);
}

int hgic_iwpriv_get_sta_info(char *ifname, char *mac_addr, struct hgic_sta_info *stainfo)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    char mac[6] = {0};

    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    if (mac_addr == NULL || stainfo == NULL) {
        PRINTF("get sta_info: invalid mac address\r\n");
        return -1;
    }
    if (strlen(mac_addr) == 17 && mac_addr[2] == ':' && mac_addr[5] == ':'
        && mac_addr[8] == ':' && mac_addr[11] == ':' && mac_addr[14] == ':') {
        hgic_str2mac(mac_addr, mac);
        return hgic_fwctrl_get_sta_info(ndev->ctrl, ndev->fwifidx, mac, stainfo);
    } else {
        return hgic_fwctrl_get_sta_info(ndev->ctrl, ndev->fwifidx, mac_addr, stainfo);
    }
}

int hgic_iwpriv_get_signal(char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_get_signal(ndev->ctrl, ndev->fwifidx);
}

int hgic_iwpriv_set_pool_maxusage(char *ifname, int maxusage)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->ctrl == NULL) {
        return -ENODEV;
    }
    return hgic_fwctrl_set_pool_maxusage(ndev->ctrl, ndev->fwifidx, maxusage);
}


