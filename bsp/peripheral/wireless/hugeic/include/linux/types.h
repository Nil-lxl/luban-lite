#ifndef _HGIC_LINUX_TYPES_
#define _HGIC_LINUX_TYPES_

#if defined(__MBED__) || defined(__CSKY__)
#include "typesdef.h"
#else
//#include "porting/types.h"
#endif

#define RET_OK    (0)
#define RET_ERR   (-1)

#ifndef __always_inline
#define __always_inline inline
#endif

#ifndef __IO
#define __IO volatile
#endif

#ifndef __I
#define __I volatile
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#define __packed __attribute__((__packed__))

#ifndef __aligned
#define __aligned(n) __attribute__ ((aligned (n)))
#endif

#ifndef RETURN_ADDR
#define RETURN_ADDR() __builtin_return_address(0)
#endif

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
typedef unsigned long long uint64_t;

typedef unsigned char uint8;
typedef unsigned short uint16;
//typedef unsigned long uint32;
typedef unsigned long long uint64;

typedef char  int8_t;
typedef short int16_t;
typedef long   int32_t;
typedef long long int64_t;

typedef char int8;
typedef short int16;
//typedef long int32;
typedef long long int64;

typedef void FILE;
typedef unsigned long		    uintptr_t;
typedef unsigned int    uint;
typedef unsigned short	ushort;
typedef unsigned int size_t;
typedef long ssize_t;
typedef int  time_t;

typedef uint8_t  bool;
typedef int64_t  loff_t;

typedef int32_t    s32;
typedef int16_t  s16;
typedef int8_t   s8;

typedef const int32_t sc32;  /*!< Read Only */
typedef const int16_t sc16;  /*!< Read Only */
typedef const int8_t  sc8;   /*!< Read Only */

typedef __IO int32_t  vs32;
typedef __IO int16_t  vs16;
typedef __IO int8_t   vs8;

typedef __I int32_t   vsc32;  /*!< Read Only */
typedef __I int16_t   vsc16;  /*!< Read Only */
typedef __I int8_t    vsc8;   /*!< Read Only */

typedef uint64_t   u64;
typedef uint32_t   u32;
typedef uint16_t   u16;
typedef uint8_t    u8;

typedef const uint32_t  uc32;  /*!< Read Only */
typedef const uint16_t uc16;  /*!< Read Only */
typedef const uint8_t  uc8;   /*!< Read Only */

typedef __IO uint32_t    vu32;
typedef __IO uint16_t  vu16;
typedef __IO uint8_t   vu8;

typedef __I uint32_t   vuc32;  /*!< Read Only */
typedef __I uint16_t vuc16;  /*!< Read Only */
typedef __I uint8_t  vuc8;   /*!< Read Only */

typedef uint8_t    __le8;
typedef uint8_t    __be8;
typedef uint16_t   __le16;
typedef uint16_t   __be16;
typedef uint32_t   __le32;
typedef uint32_t   __be32;
typedef uint64_t   __le64;
typedef uint64_t   __be64;

typedef uint8_t    __u8;
typedef uint16_t   __u16;
typedef uint32_t   __u32;
typedef uint64_t   __u64;

typedef int8_t  __s8;
typedef int16_t __s16;
typedef int32_t   __s32;
typedef int64_t   __s64;

typedef int64_t    s64;
//typedef uint32_t size_t;
//typedef int32_t   ssize_t;
typedef uint32_t uint32;
typedef int32_t   int32;
typedef int32_t   gfp_t;

typedef int       intptr_t;

typedef uint16_t __sum16;
typedef unsigned long		ulong;
//typedef unsigned long size_t;

#ifndef UINT32_MAX
#define UINT32_MAX (0xffffffff)
#endif

#ifndef U32_MAX
#define U32_MAX (0xffffffff)
#endif

#ifndef S32_MIN
#define S32_MIN (-2147483648)
#endif

#ifndef S32_MAX
#define S32_MAX (2147483648)
#endif

#ifndef U8_MAX
#define U8_MAX (0xff)
#endif

#define ATOMIC_INIT(i) { (i) }
#define READ_ONCE(x) (x)
#define WRITE_ONCE(x, val)	(x) = (val)

#define ALIGN(s,a) (((s)+(a)-1) & ~((a)-1))

#define aes_encrypt     os_aes_encrypt
#define aes_decrypt     os_aes_decrypt
#define fputs           os_fputs
#define strdup          _os_strdup
#define schedule_work   os_schdule_work
//#define inet_aton             os_inet_aton
#define find_next_zero_bit os_find_next_zero_bit
#define get_random_bytes    os_get_random_bytes
#define if_nametoindex      os_if_nametoindex
#define ieee802_1d_to_ac    os_ieee802_1d_to_ac
#define buf_shift_right     os_buf_shift_right
#define forced_memzero      os_forced_memzero
#define dup_binstr          os_dup_binstr
#define merge_byte_arrays   os_merge_byte_arrays
#define bin_clear_free      os_bin_clear_free
#define hostapd_get_aid     os_hostapd_get_aid
#define ap_get_sta          os_ap_get_sta
#define wpa_auth_sta_no_wpa os_wpa_auth_sta_no_wpa
#define ieee802_1x_receive  os_ieee802_1x_receive
#define ap_sta_add          os_ap_sta_add
#define wpa_auth_sta_init   os_wpa_auth_sta_init
#define __wpa_send_eapol    __os_wpa_send_eapol
#define wpa_receive         os_wpa_receive
#define wpa_add_kde         os_wpa_add_kde
#define wpa_validate_wpa_ie os_wpa_validate_wpa_ie
#define wpa_write_rsn_ie    os_wpa_write_rsn_ie
#define wpa_cipher_to_suite os_wpa_cipher_to_suite
#define rsn_cipher_put_suites   os_rsn_cipher_put_suites
#define wpa_cipher_put_suites   os_wpa_cipher_put_suites
#define wpa_parse_kde_ies       os_wpa_parse_kde_ies
#define rcons                   os_rcons
#define Te0                     os_Te0
#define Td0                     os_Td0
#define Td4s                    os_Td4s
#define hex2byte                os_hex2byte
#define wpa_supplicant_mark_disassoc    os_wpa_supplicant_mark_disassoc
#define wpa_parse_wpa_ie_wpa    os_wpa_parse_wpa_ie_wpa
#define wpa_compare_rsn_ie      os_wpa_compare_rsn_ie
#define wpa_pmk_to_ptk          os_wpa_pmk_to_ptk
#define wpa_eapol_key_mic       os_wpa_eapol_key_mic
#define SHA1Init                os_SHA1Init
#define SHA1Transform           os_SHA1Transform
#define SHA1Update              os_SHA1Update
#define SHA1Final               os_SHA1Final
#define wpa_eapol_key_send      os_wpa_eapol_key_send
#define wpa_supplicant_send_4_of_4  os_wpa_supplicant_send_4_of_4
#define wpa_supplicant_send_2_of_4  os_wpa_supplicant_send_2_of_4
#define wpa_gen_wpa_ie          os_wpa_gen_wpa_ie
#define wpa_bss_get_vendor_ie   os_wpa_bss_get_vendor_ie
#define wpa_bss_get_ie          os_wpa_bss_get_ie
#define tasklet_init            os_tasklet_init
#define tasklet_schedule        os_tasklet_schedule
#define tasklet_kill            os_tasklet_kill
#define wpa_hexdump_key         os_wpa_hexdump_key
#define wpa_hexdump             os_wpa_hexdump
#define crc32                   os_crc32
#define skb_dequeue             os_skb_dequeue
#define skb_queue_tail          os_skb_queue_tail
#define hex_to_bin              os_hex_to_bin
#define register_netdevice_notifier     os_register_netdevice_notifier
#define _skb_copy               _os_skb_copy
#define __bitmap_full           os___bitmap_full
#define bitmap_clear            os_bitmap_clear
//#define find_next_bit           find_next_bit
#define register_netdev         os_register_netdev
#define dev_alloc_name          os_dev_alloc_name
#define netif_receive_skb       os_netif_receive_skb
#define eth_mac_addr            os_eth_mac_addr
#define unregister_netdev       os_unregister_netdev
#define skb_clone               os_skb_clone
#define skb_trim                os_skb_trim
#define skb_copy_bits           os_skb_copy_bits
#define cancel_work_sync        os_cancel_work_sync
#define cancel_delayed_work_sync    os_cancel_delayed_work_sync
#define flush_work              os_flush_work
#define flush_delayed_work      os_flush_delayed_work
#define flush_workqueue         os_flush_workqueue
#define destroy_workqueue       os_destroy_workqueue
#define __nlmsg_put             os___nlmsg_put
#define nla_reserve             os_nla_reserve
#define nla_put                 os_nla_put
#define __bitmap_equal          os___bitmap_equal
#define __bitmap_subset         os___bitmap_subset

#define cfg80211_mlme_auth	os_cfg80211_mlme_auth
#define cfg80211_oper_and_ht_capa os_cfg80211_oper_and_ht_capa
#define cfg80211_oper_and_vht_capa os_cfg80211_oper_and_vht_capa
#define cfg80211_mlme_assoc os_cfg80211_mlme_assoc
#define cfg80211_mlme_deauth os_cfg80211_mlme_deauth
#define cfg80211_mlme_disassoc os_cfg80211_mlme_disassoc
#define cfg80211_mlme_down os_cfg80211_mlme_down
#define cfg80211_mlme_register_mgmt os_cfg80211_mlme_register_mgmt
#define cfg80211_mlme_unregister_socket os_cfg80211_mlme_unregister_socket
#define cfg80211_mlme_purge_registrations os_cfg80211_mlme_purge_registrations
#define cfg80211_mlme_mgmt_tx os_cfg80211_mlme_mgmt_tx
#define cfg80211_dfs_channels_update_work os_cfg80211_dfs_channels_update_work
#define cfg80211_radar_event os_cfg80211_radar_event
#define cfg80211_cac_event os_cfg80211_cac_event
#define cfg80211_check_station_change os_cfg80211_check_station_change
#define __cfg80211_alloc_event_skb os___cfg80211_alloc_event_skb
#define cfg80211_rdev_free_coalesce os_cfg80211_rdev_free_coalesce
#define __cfg80211_alloc_reply_skb os___cfg80211_alloc_reply_skb
#define cfg80211_vendor_cmd_reply os_cfg80211_vendor_cmd_reply
#define nl80211_notify_wiphy os_nl80211_notify_wiphy
#define nl80211_send_scan_start os_nl80211_send_scan_start
#define nl80211_build_scan_msg os_nl80211_build_scan_msg
#define nl80211_send_sched_scan os_nl80211_send_sched_scan
#define nl80211_common_reg_change_event os_nl80211_common_reg_change_event
#define nl80211_send_rx_auth os_nl80211_send_rx_auth
#define nl80211_send_rx_assoc os_nl80211_send_rx_assoc
#define nl80211_send_deauth os_nl80211_send_deauth
#define nl80211_send_disassoc os_nl80211_send_disassoc
#define cfg80211_rx_unprot_mlme_mgmt os_cfg80211_rx_unprot_mlme_mgmt
#define nl80211_send_auth_timeout os_nl80211_send_auth_timeout
#define nl80211_send_assoc_timeout os_nl80211_send_assoc_timeout
#define nl80211_send_connect_result os_nl80211_send_connect_result
#define nl80211_send_roamed os_nl80211_send_roamed
#define nl80211_send_disconnected os_nl80211_send_disconnected
#define nl80211_send_ibss_bssid os_nl80211_send_ibss_bssid
#define cfg80211_notify_new_peer_candidate os_cfg80211_notify_new_peer_candidate
#define nl80211_michael_mic_failure os_nl80211_michael_mic_failure
#define nl80211_send_beacon_hint_event os_nl80211_send_beacon_hint_event
#define cfg80211_ready_on_channel os_cfg80211_ready_on_channel
#define cfg80211_remain_on_channel_expired os_cfg80211_remain_on_channel_expired
#define cfg80211_new_sta os_cfg80211_new_sta
#define cfg80211_del_sta_sinfo os_cfg80211_del_sta_sinfo
#define cfg80211_conn_failed os_cfg80211_conn_failed
#define cfg80211_rx_spurious_frame os_cfg80211_rx_spurious_frame
#define cfg80211_rx_unexpected_4addr_frame os_cfg80211_rx_unexpected_4addr_frame
#define nl80211_send_mgmt os_nl80211_send_mgmt
#define cfg80211_mgmt_tx_status os_cfg80211_mgmt_tx_status
#define cfg80211_cqm_rssi_notify os_cfg80211_cqm_rssi_notify
#define cfg80211_cqm_txe_notify os_cfg80211_cqm_txe_notify
#define cfg80211_cqm_pktloss_notify os_cfg80211_cqm_pktloss_notify
#define cfg80211_cqm_beacon_loss_notify os_cfg80211_cqm_beacon_loss_notify
#define cfg80211_gtk_rekey_notify os_cfg80211_gtk_rekey_notify
#define cfg80211_pmksa_candidate_notify os_cfg80211_pmksa_candidate_notify
#define cfg80211_ch_switch_notify os_cfg80211_ch_switch_notify
#define cfg80211_ch_switch_started_notify os_cfg80211_ch_switch_started_notify
#define nl80211_radar_notify os_nl80211_radar_notify
#define cfg80211_probe_status os_cfg80211_probe_status
#define cfg80211_tdls_oper_request os_cfg80211_tdls_oper_request
#define cfg80211_ft_event os_cfg80211_ft_event
#define cfg80211_crit_proto_stopped os_cfg80211_crit_proto_stopped
#define nl80211_send_ap_stopped					 os_nl80211_send_ap_stopped
#define cfg80211_external_auth_request os_cfg80211_external_auth_request
#define ___cfg80211_scan_done os____cfg80211_scan_done
#define __cfg80211_scan_done os___cfg80211_scan_done
#define cfg80211_scan_done os_cfg80211_scan_done
#define cfg80211_sched_scan_results os_cfg80211_sched_scan_results
#define cfg80211_sched_scan_stopped os_cfg80211_sched_scan_stopped
#define __cfg80211_stop_sched_scan os___cfg80211_stop_sched_scan
#define cfg80211_bss_age os_cfg80211_bss_age
#define cfg80211_bss_expire os_cfg80211_bss_expire
#define cfg80211_get_bss os_cfg80211_get_bss
#define cfg80211_ref_bss os_cfg80211_ref_bss
#define cfg80211_put_bss os_cfg80211_put_bss
#define cfg80211_inform_bss_frame_data os_cfg80211_inform_bss_frame_data
#define cfg80211_inform_bss_data os_cfg80211_inform_bss_data
#define cfg80211_unlink_bss os_cfg80211_unlink_bss
#define ieee80211_get_response_rate os_ieee80211_get_response_rate
#define ieee80211_mandatory_rates os_ieee80211_mandatory_rates
#define ieee80211_set_bitrate_flags os_ieee80211_set_bitrate_flags
#define cfg80211_supported_cipher_suite os_cfg80211_supported_cipher_suite
#define cfg80211_validate_key_settings os_cfg80211_validate_key_settings
#define ieee80211_hdrlen os_ieee80211_hdrlen
#define ieee80211_get_hdrlen_from_skb os_ieee80211_get_hdrlen_from_skb
#define ieee80211_get_mesh_hdrlen os_ieee80211_get_mesh_hdrlen
#define rfc1042_header os_rfc1042_header
#define bridge_tunnel_header os_bridge_tunnel_header
#define cfg80211_upload_connect_keys os_cfg80211_upload_connect_keys
#define cfg80211_process_wdev_events					 os_cfg80211_process_wdev_events
#define cfg80211_process_rdev_events os_cfg80211_process_rdev_events
#define cfg80211_change_iface os_cfg80211_change_iface
#define cfg80211_calculate_bitrate os_cfg80211_calculate_bitrate
#define cfg80211_get_p2p_attr os_cfg80211_get_p2p_attr
#define ieee80211_ie_split_ric os_ieee80211_ie_split_ric
#define ieee80211_operating_class_to_band os_ieee80211_operating_class_to_band
#define ieee80211_chandef_to_operating_class os_ieee80211_chandef_to_operating_class
#define cfg80211_validate_beacon_int os_cfg80211_validate_beacon_int
#define cfg80211_iter_combinations os_cfg80211_iter_combinations
#define cfg80211_check_combinations os_cfg80211_check_combinations
#define ieee80211_get_ratemask os_ieee80211_get_ratemask
#define ieee80211_get_num_supported_channels os_ieee80211_get_num_supported_channels
#define cfg80211_get_station os_cfg80211_get_station
#define get_wiphy_regdom os_get_wiphy_regdom
#define reg_get_dfs_region os_reg_get_dfs_region
#define is_world_regdom os_is_world_regdom
#define reg_is_valid_request os_reg_is_valid_request
#define reg_get_max_bandwidth os_reg_get_max_bandwidth
#define freq_reg_info os_freq_reg_info
#define reg_initiator_name os_reg_initiator_name
#define reg_last_request_cell_base os_reg_last_request_cell_base
#define wiphy_apply_custom_regulatory os_wiphy_apply_custom_regulatory
#define regulatory_hint_user os_regulatory_hint_user
#define regulatory_hint_indoor os_regulatory_hint_indoor
#define regulatory_netlink_notify os_regulatory_netlink_notify
#define regulatory_hint os_regulatory_hint
#define regulatory_hint_country_ie os_regulatory_hint_country_ie
#define regulatory_hint_found_beacon os_regulatory_hint_found_beacon
#define reg_supported_dfs_region os_reg_supported_dfs_region
#define set_regdom os_set_regdom
#define regulatory_hint_disconnect os_regulatory_hint_disconnect
#define regulatory_set_wiphy_regd os_regulatory_set_wiphy_regd
#define wiphy_regulatory_register os_wiphy_regulatory_register
#define wiphy_regulatory_deregister os_wiphy_regulatory_deregister
#define cfg80211_get_unii os_cfg80211_get_unii
#define regulatory_indoor_allowed os_regulatory_indoor_allowed
#define regulatory_init os_regulatory_init
#define regulatory_exit os_regulatory_exit
#define crc32_le_generic os_crc32_le_generic
#define crc32_le os_crc32_le
#define crc32_be os_crc32_be
#define unregister_netdevice_notifier os_unregister_netdevice_notifier
#define alloc_netdev_mqs os_alloc_netdev_mqs
#define __rb_erase_color os___rb_erase_color
#define rb_insert_color os_rb_insert_color
#define __rb_erase_augmented os___rb_erase_augmented
#define rb_erase os_rb_erase
#define __rb_insert_augmented os___rb_insert_augmented
#define rb_first os_rb_first
#define rb_last os_rb_last
#define rb_next os_rb_next
#define rb_prev os_rb_prev
#define rb_replace_node os_rb_replace_node
#define pskb_expand_head os_pskb_expand_head
#define skb_copy_expand os_skb_copy_expand
#define cfg80211_rdev_by_wiphy_idx os_cfg80211_rdev_by_wiphy_idx
#define cfg80211_rdev_list os_cfg80211_rdev_list
#define get_wiphy_idx os_get_wiphy_idx
#define wiphy_idx_to_wiphy os_wiphy_idx_to_wiphy
#define cfg80211_stop_p2p_device os_cfg80211_stop_p2p_device
#define cfg80211_shutdown_all_interfaces os_cfg80211_shutdown_all_interfaces
#define wiphy_new_nm os_wiphy_new_nm
#define wiphy_register os_wiphy_register
#define wiphy_unregister os_wiphy_unregister
#define cfg80211_dev_free os_cfg80211_dev_free
#define wiphy_free os_wiphy_free
#define cfg80211_unregister_wdev os_cfg80211_unregister_wdev
#define cfg80211_update_iface_num os_cfg80211_update_iface_num
#define __cfg80211_leave os___cfg80211_leave
#define cfg80211_destroy_ifaces os_cfg80211_destroy_ifaces
#define cfg80211_leave os_cfg80211_leave
#define cfg80211_stop_iface os_cfg80211_stop_iface
#define cfg80211_init os_cfg80211_init
#define __cfg80211_stop_ap os___cfg80211_stop_ap
#define cfg80211_stop_ap os_cfg80211_stop_ap
#define cfg80211_chandef_create os_cfg80211_chandef_create
#define cfg80211_chandef_valid os_cfg80211_chandef_valid
#define cfg80211_chandef_identical os_cfg80211_chandef_identical
#define cfg80211_chandef_compatible os_cfg80211_chandef_compatible
#define cfg80211_set_dfs_state os_cfg80211_set_dfs_state
#define cfg80211_chandef_dfs_required os_cfg80211_chandef_dfs_required
#define cfg80211_chandef_dfs_usable os_cfg80211_chandef_dfs_usable
#define cfg80211_chandef_dfs_cac_time os_cfg80211_chandef_dfs_cac_time
#define cfg80211_chandef_usable os_cfg80211_chandef_usable
#define cfg80211_reg_can_beacon os_cfg80211_reg_can_beacon
#define cfg80211_reg_can_beacon_relax os_cfg80211_reg_can_beacon_relax
#define cfg80211_set_monitor_channel os_cfg80211_set_monitor_channel
#define cfg80211_get_chan_state os_cfg80211_get_chan_state
#define cfg80211_rx_assoc_resp os_cfg80211_rx_assoc_resp
#define cfg80211_rx_mlme_mgmt os_cfg80211_rx_mlme_mgmt
#define cfg80211_auth_timeout os_cfg80211_auth_timeout
#define cfg80211_assoc_timeout os_cfg80211_assoc_timeout
#define cfg80211_abandon_assoc os_cfg80211_abandon_assoc
#define cfg80211_tx_mlme_mgmt os_cfg80211_tx_mlme_mgmt
#define cfg80211_michael_mic_failure os_cfg80211_michael_mic_failure

#define isblank(c)           (c == ' ' || c == '\t')

#define in_range(c, lo, up)  ((unsigned char)c >= lo && (unsigned char)c <= up)
#define islower(c)           in_range(c, 'a', 'z')

struct timeval
{
    long tv_sec;         /* seconds */
    long tv_usec;        /* and microseconds */
};

static inline unsigned char toupper(unsigned char c)
{
	if (islower(c))
		c -= 'a'-'A';
	return c;
}

#define sockaddr_in porting_sockaddr_in
struct porting_in_addr {
  unsigned int s_addr;
};

struct porting_sockaddr_in {
  unsigned char sin_len;
  unsigned char sin_family;
  unsigned short sin_port;
  struct porting_in_addr sin_addr;
  char sin_zero[8];
};

#define AF_INET         2      /* Internet IP Protocol */

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

#define BITS_PER_LONG      32
#define BITS_PER_LONG_LONG 64

#if defined(HGIC_SMAC) || defined(CONFIG_UMAC)
#include <umac_defs.h>
#endif

#include <linux/defs.h>
#include <linux/errno.h>
#include <linux/gfp.h>
#include <byteorder/little_endian.h>
#include <linux/string.h>
#include <linux/unaligned.h>

#endif

