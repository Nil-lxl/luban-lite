/**
  ******************************************************************************
  * @file    hgics.c
  * @author  HUGE-IC Application Team
  * @version V1.0.0
  * @date    2022-05-18
  * @brief   hgic smac driver daemon.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2022 HUGE-IC</center></h2>
  *
  ******************************************************************************
  */

#ifndef __RTOS__
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#else
#include <linux/defs.h>
#include <linux/types.h>
#endif
#include "hgic.h"
#include "hgic_def.h"

#include "iwpriv.h"

#define AUTO_RELAY_EN 1
#define BELNC_EN 1

struct hgic_fw_info hgic_fwinfo;
int   blenc_mode = 0;
//////////////////////////////////////////////////////////////////////////////////////////////////

static void hgics_parse_blenc_param(u8 *data, int len)
{
    u8 *ptr = data;
#if 1 //sample code
    u8 buff[33];
    u8 cmd[64];
    char out[128];
    while (ptr < data + len) {
        switch (ptr[0]) {
            case 1: //SSID
                memset(buff, 0, sizeof(buff));
                memcpy(buff, ptr + 2, ptr[1]);
                hgic_dbg("SET ssid:%s\r\n", buff);
                hgics_wpacli_set_ssid("hg0", buff);
                break;
            case 2: //PassWord
                memset(buff, 0, sizeof(buff));
                memcpy(buff, ptr + 2, ptr[1]);
                hgic_dbg("SET passwd:%s\r\n", buff);
                hgics_wpacli_set_psk("hg0", buff);
                break;
            case 3: //Keymgmt
                hgic_dbg("SET keymgmt %s\r\n", ptr[2] ? "WPA-PSK" : "NONE");
                hgics_wpacli_set_keymgmt("hg0", ptr[2] ? "WPA-PSK" : "NONE");
                break;
            case 4: //auth
                hgic_dbg("AUTH %d\r\n", ptr[2]);
                //sprintf(cmd, "ifconfig wlan0 %s", ptr[2] ? "up" : "down");
                break;
            default:
                hgic_dbg("Unsupport ID:%d\r\n", ptr[0]);
                break;
        }
        ptr += (ptr[1] + 2);
    }

    hgic_dbg("Would run blenc start\n");
    hgic_blenc_release();
    hgic_iwpriv_blenc_start("hg0", 0, 38);
    hgics_wpacli_disable_network("hg0");
    hgics_wpacli_enable_network("hg0");
    //restart hostapd or wpa_supplicant
    //system("hgic2g.sh");
#endif
}

/*??????????? | Weixin applet data*/
void hgics_recv_blenc_data(uint8 *data, uint32 len)
{
    uint8 *ncdata = NULL;
    uint32 data_len = 0;

    if(!data) {
        hgic_err("Input param error!\n");
        return;
    }

    if (hgic_blenc_parse_data(data, len)) {
        data_len = hgic_blenc_get_data(&ncdata);
        if (data_len && ncdata) {
            hgics_parse_blenc_param(ncdata, data_len);
        }
    } else {
        //hgic_err("Parse data error!\n");
    }
}

/* customer protocol
   data: BLE PDU data
   len:  data length
*/
static void hgics_recv_customer_ble_data(u8 *data, int len)
{
    hgic_dbg("Recv customer blenc data!\n");
}

void hgics_proc_fwevent(u8 *event_data, u32 event_len)
{
    u32   data_len = 0;
    u32   evt_id   = 0;
    char *data     = NULL;
    struct hgic_ctrl_hdr *evt = (struct hgic_ctrl_hdr *)event_data;

    data     = (char *)(evt + 1);
    data_len = event_len - sizeof(struct hgic_ctrl_hdr);
    evt_id   = HDR_EVTID(evt);

    switch (evt_id) {
        case HGIC_EVENT_BLENC_DATA:
#if 1 /*hgic demo protocol*/
            hgics_recv_blenc_data(data, data_len);
#else
            /*customer protocol*/
            hgics_recv_customer_ble_data(data, data_len);
#endif
            break;
        case HGIC_EVENT_HWSCAN_RESULT:
#if AUTO_RELAY_EN
            hgics_relay_check_hwscan_result(data, data_len);
#endif
            break;
        case HGIC_EVENT_FW_INIT_DONE:
            if(hgic_fwinfo.version){ //maybe wifi chip has been reset.
                printf("firmware init done, restart hostapd or wpa_supplicant!!!!\r\n");
                system("smac.sh");
            }
            break;
        default:
            printf("ignore event %d\r\n", evt_id);
            break;
    }
}



#if 0
int main(int argc, char *argv[])
{
    int i = 0;
    int ret = 0;
    int fd  = -1;
    int wifi_mode = 0;
    struct hgic_hdr *hdr;
    char *ssid = NULL;
    char *buff = NULL;

    //HGIC = "hgics"; //for iwpriv.c

    buff = malloc(4096);
    if (buff == NULL) {
        printf("malloc fail\r\n");
        return -1;
    }

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "sta") == 0) {
            wifi_mode = 0;
        } else if (strcmp(argv[i], "ap") == 0) {
            wifi_mode = 1;
        } else if (strcmp(argv[i], "apsta") == 0) {
            wifi_mode = 2;
        } else {
            ssid = argv[i];
        }
    }

    hgic_blenc_init(); /* ??????????? | Just for Weixin applet*/

#if AUTO_RELAY_EN //sample code
    hgics_relay_init(ssid, wifi_mode);
    if (wifi_mode == 2) {
        system("brctl delif br0 eth2");
        system("brctl addif br0 wlan0");
        system("brctl addif br0 wlan1");
        system("ifconfig br0 192.168.1.30");
        system("ifconfig eth2 10.10.10.30");
    }
#endif

__open:
    fd = open("/proc/hgics/fwevnt", O_RDONLY);
    if (fd == -1) {
        sleep(1); //sleep 1s and try again.
        goto __open;
    }

    //get firmware version
    hgic_iwpriv_get_fwinfo("wlan0", &hgic_fwinfo);
    printf("firmware version: v%d.%d.%d.%d\r\n",
                (hgic_fwinfo.version >> 24) & 0xff, (hgic_fwinfo.version >> 16) & 0xff,
                (hgic_fwinfo.version >> 8) & 0xff, (hgic_fwinfo.version & 0xff));

#if 1 //BLE demo code for work mode 3.
    do {
        /*
           ???? ? ?????? ?????:[??,??,??...]
           ?????? ?????�BLE_Assigned_Numbers.pdf�, ??:2.3 Common Data Types.
           ??????,????????????

           Advertising_Data and Scan_Response Format: [length,Type,Data...]
           Document: BLE_Assigned_Numbers.pdf, Section 2.3,  Common Data Types.
           You can modify the data or add more data in adv_data and scan_resp.
        */

        /* ?????? Scan_Response Data */
        u8 scan_resp[] = {
                          /*Length  Type   Data */
                            0x04,  0x09,  'S','S','S', /* the name is "SSS" */
                          /* type 0x09: Complete Local Name. document: Core_Specification_Supplement.pdf, Part A, Section 1.2 */
                          /* ?? 0x09: ????. ????:Core_Specification_Supplement.pdf, Part A, Section 1.2               */

                          /*Length  Type   Data */
                            0x19,  0xFF,  0xD0,0x07,0x01,0x03,0x00,0x00,0x0C,0x00,0x88,0xD1,0xC4,0x89,0x2B,0x56,0x7D,0xE5,0x65,0xAC,0xA1,0x3F,0x09,0x1C,0x43,0x92
                          /* type 0xFF: Manufacturer Specific Data. */
                          /* ?? 0xFF: ???????. */
         };

        /* ???? Advertising_Data */
        u8 adv_data[] = {
                        /*Length  Type   Data */
                          0x02,   0x01, 0x06,
                        /* type 0x01: Flags. document: Core_Specification_Supplement.pdf, Part A, Section 1.3 */
                        /* ?? 0x01: ????,??bit????????:Core_Specification_Supplement.pdf, Part A, Section 1.3 */
                        /////////////////////////////////////////////////////////////////////////////////////

                        /*Length  Type   Data */
                          0x03,   0x02, 0x01,0xA2, /* uuid 0x2A01 */
                        /* type 0x02: Incomplete List of 16�bit Service Class UUIDs. document: Core_Specification_Supplement.pdf, Part A, Section 1.1 */
                        /* ?? 0x02: 16bit UUID??:???ATT Table?????UUID. ????: Core_Specification_Supplement.pdf, Part A, Section 1.1 */
                        /////////////////////////////////////////////////////////////////////////////////////

                        /*Length  Type   Data */
                          0x14,   0x16, 0x01,0xA2,0x01,0x6B,0x65,0x79,0x79,0x66,0x67,0x35,0x79,0x33,0x34,0x79,0x71,0x78,0x71,0x67,0x64
                        /* type 0x16: Service Data - 16 bit UUID. document: Core_Specification_Supplement.pdf, Part A, Section 1.11 */
                        /* ?? 0x16:   16bit UUID?????. ????: Core_Specification_Supplement.pdf, Part A, Section 1.11 */
                        /////////////////////////////////////////////////////////////////////////////////////
          };

        /*?????? |       Config the Advertising Data*/
        hgic_iwpriv_blenc_set_advdata("wlan0", adv_data, sizeof(adv_data));

        /*???????? |         Config the Scan Response Data*/
        hgic_iwpriv_blenc_set_scanresp("wlan0", scan_resp, sizeof(scan_resp));

        /* ?????? |       Enable Advertising Function */
        hgic_iwpriv_blenc_start_adv("wlan0", 1);

        /* ????BLE??3 |       Start BLE with work mode 3*/
        //hgic_iwpriv_blenc_start("wlan0", 3, 38); //start BLE MODE 3

        /* ????BLE?? | Stop BLE */
        //hgic_iwpriv_blenc_start("wlan0", 0, 38); //stop BLE
    } while (0);
#endif

    /* start read event from driver and process the event data.*/
    hdr = (struct hgic_hdr *)buff;
    while (1) {
        /* If there is no event data when calling the read API, the driver will block the current thread for 100ms.
           you can see it in the function hgics_fwevent_read. */
        ret = read(fd, buff, 4096);

        if (ret > 0) {
            switch (hdr->type) {
                case HGIC_HDR_TYPE_EVENT:
                case HGIC_HDR_TYPE_EVENT2:
                    hgics_proc_fwevent(buff, ret); //process the events from firmware.
                    break;
                case HGIC_HDR_TYPE_BLUETOOTH:      //process the bluetooth data from firmware. [ble work mode 3]
                    #if BELNC_EN
                    hgic_proc_bt_data(buff, ret);
                    #endif
                    break;
                default:
                    printf("unknown hdr type:%d\r\n", hdr->type);
                    break;
            }
        } else if (ret == 0) { //no data and timeout.
            #if AUTO_RELAY_EN
            if(wifi_mode == 2){
                hgics_relay_check_status();
            }
            #endif

            #if 1 //BLE Notify test code
            hgic_gatt_notify_test();
            #endif
        } else { // read error.
            printf("read error, ret=%d\r\n", ret);
            close(fd);
            goto __open;
        }
    }

    close(fd);
    free(buff);
    return 0;
}


#endif
#if 0
int hgic_blenc_test(int mode,int adv_interval)
{
    //hgics_blenc_init();
    blenc_mode = mode; /*1:�㲥������2:�㲥����,֧��ɨ��, 3: BLEЭ����������*/
    u8 scan_resp[] = {0x04, 0x09, 0x54, 0x58, 0x57, 0x19, 0xFF, 0xD0, 0x07, 0x01, 0x03, 0x00, 0x00, 0x0C, 0x00, 0x88, 0xD1, 0xC4, 0x89, 0x2B, 0x56, 0x7D, 0xE5, 0x65, 0xAC, 0xA1, 0x3F, 0x09, 0x1C, 0x43, 0x92};
    u8 adv_data[] = {0x02, 0x01, 0x06, 0x03, 0x02, 0x01, 0xA2, 0x14, 0x16, 0x01, 0xA2, 0x01, 0x6B, 0x65, 0x79, 0x79, 0x66, 0x67, 0x35, 0x79, 0x33, 0x34, 0x79, 0x71, 0x78, 0x71, 0x67, 0x64};
    //u8 scan_resp[] = {0x04, 0x09,0x5A, 0x5A, 0x5A, 0x19, 0xFF, 0xD0, 0x07, 0x01, 0x03, 0x00, 0x00, 0x0C, 0x00, 0x88, 0xD1, 0xC4, 0x89, 0x2B, 0x56, 0x7D, 0xE5, 0x65, 0xAC, 0xA1, 0x3F, 0x09, 0x1C, 0x43, 0x92};
    //u8 adv_data[] = {0x02, 0x01, 0x06, 0x03, 0x02, 0x01, 0xA2, 0x14, 0x16, 0x01, 0xA2, 0x01, 0x6B, 0x65, 0x79, 0x79, 0x66, 0x67, 0x35, 0x79, 0x33, 0x34, 0x79, 0x71, 0x78, 0x71, 0x67, 0x64};
    //u8 adv_data[] = {0x2,0x1,0x6,0x3,0x2,0x1,0xa2,0x14,0x16,0x1,0xa2,0x0,0x67,0x79,0x75,0x69,0x76,0x61,0x6d,0x33,0x35,0x6b,0x64,0x37,0x72,0x71,0x6c,0x6a};
    //u8 scan_resp[] = {0x3,0x9,0x54,0x59,0x19,0xff,0xd0,0x7,0x9,0x3,0x3,0x0,0xc,0x0,0x8f,0x61,0x62,0x4e,0x3f,0xe1,0x30,0x28,0x9a,0x73,0x17,0xb1,0xea,0x92,0x45,0x33};
    u8 dev_addr[] = {0x00, 0x12, 0x34, 0x56, 0x78, 0x00};
    //hgic_iwpriv_blenc_set_devaddr("wlan0", dev_addr);
    hgic_iwpriv_blenc_set_advdata("hg0", adv_data, sizeof(adv_data));
    hgic_iwpriv_blenc_set_scanresp("hg0", scan_resp, sizeof(scan_resp));
    hgic_iwpriv_blenc_set_adv_interval("hg0", 100);
    hgic_iwpriv_blenc_start_adv("hg0", 1);
    hgic_iwpriv_blenc_start("hg0", mode, 38);
    hgic_dbg("start hugeic blenc test,mode:%d\n",mode);
}
#endif
int hgic_blenc_test(int mode,int adv_interval)
{
        blenc_mode = mode;
        do {
        /*
           广播数据 和 扫描响应数据 的填充格式：[长度,类型,数据...]
           类型定义列表 请查阅文档《BLE_Assigned_Numbers.pdf》， 章节：2.3
Common Data Types.
           根据应用需求，可以修改或新增填充数据。

           Advertising_Data and Scan_Response Format: [length,Type,Data...]
           Document: BLE_Assigned_Numbers.pdf, Section 2.3,  Common Data Types.
           You can modify the data or add more data in adv_data and scan_resp.
        */

        /* 扫描响应数据 Scan_Response Data */
        u8 scan_resp[] = {
                          /*Length  Type   Data */
                            0x04,  0x09,  'T','X','W', /* the name is "SSS" */
                          /* type 0x09: Complete Local Name. document:
Core_Specification_Supplement.pdf, Part A, Section 1.2 */
                          /* 类型 0x09: 设备名称.
查阅文档：Core_Specification_Supplement.pdf, Part A, Section 1.2               */

                          /*Length  Type   Data */
                            0x19,  0xFF,  0xD0,0x07,0x01,0x03,0x00,0x00,0x0C,0x00,0x88,0xD1,0xC4,
0x89,0x2B,0x56,0x7D,0xE5,0x65,0xAC,0xA1,0x3F,0x09,0x1C,0x43,0x92
                          /* type 0xFF: Manufacturer Specific Data. */
                          /* 类型 0xFF: 厂家自定义数据. */
         };

        /* 广播数据 Advertising_Data */
        u8 adv_data[] = {
                        /*Length  Type   Data */
                          0x02,   0x01, 0x06,
                        /* type 0x01: Flags. document: Core_Specification_Supplement.pdf, Part A,
Section 1.3 */
                        /* 类型 0x01:
广播标识，各个bit的含义请查阅文档：Core_Specification_Supplement.pdf, Part A,
Section 1.3 */

/////////////////////////////////////////////////////////////////////////////////////

                        /*Length  Type   Data */
                          0x03,   0x02, 0x01,0xA2, /* uuid 0x2A01 */
                        /* type 0x02: Incomplete List of 16­bit Service Class UUIDs. document:
Core_Specification_Supplement.pdf, Part A, Section 1.1 */
                        /* 类型 0x02: 16bit UUID列表：对应到ATT Table里面定义的UUID.
查阅文档: Core_Specification_Supplement.pdf, Part A, Section 1.1 */

/////////////////////////////////////////////////////////////////////////////////////

                        /*Length  Type   Data */
                          0x14,   0x16, 0x01,0xA2,0x01,0x6B,0x65,0x79,0x79,0x66,0x67,0x35,0x79,
0x33,0x34,0x79,0x71,0x78,0x71,0x67,0x64
                        /* type 0x16: Service Data - 16 bit UUID. document:
Core_Specification_Supplement.pdf, Part A, Section 1.11 */
                        /* 类型 0x16:   16bit UUID的服务数据. 查阅文档:
Core_Specification_Supplement.pdf, Part A, Section 1.11 */

/////////////////////////////////////////////////////////////////////////////////////
          };

        /*设置广播数据 |       Config the Advertising Data*/
        hgic_iwpriv_blenc_set_advdata("hg0", adv_data, sizeof(adv_data));

        /*设置扫描响应数据 |         Config the Scan Response Data*/
        hgic_iwpriv_blenc_set_scanresp("hg0", scan_resp, sizeof(scan_resp));

        /* 打开广播功能 |       Enable Advertising Function */
        hgic_iwpriv_blenc_start_adv("hg0", 1);

        /* 启动进入BLE模式3 |       Start BLE with work mode 3*/
        hgic_iwpriv_blenc_start("hg0", mode, 38); //start BLE MODE 3

        PRINTF("start the ble mode!\n");

        /* 停止退出BLE模式 | Stop BLE */
        //hgic_iwpriv_blenc_start("wlan0", 0, 38); //stop BLE
    } while (0);
}

