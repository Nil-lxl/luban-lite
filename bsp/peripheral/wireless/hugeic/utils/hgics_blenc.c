/**
  ******************************************************************************
  * @file    hgics_blenc.c
  * @author  HUGE-IC Application Team
  * @version V1.0.0
  * @date    2022-05-18
  * @brief   hgic BLE network configure lib
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

#define ATT_MTU  (512)

#define BT_L2CAP_HDR_SIZE (4)
#define HCI_CONN_HANDLE   (1)
#define HCI_ACL_HDR_SIZE  (4)
#define HCI_ACL_DATA_PACKET (2)
#define HGICS_GATT_CHARAC_Broadcast (0x01)
#define HGICS_GATT_CHARAC_Read      (0x02)
#define HGICS_GATT_CHARAC_Write_Without_Response (0x04)
#define HGICS_GATT_CHARAC_Write (0x08)
#define HGICS_GATT_CHARAC_Notify (0x10)
#define HGICS_GATT_CHARAC_Indicate (0x20)
#define HGICS_GATT_CHARAC_Authenticated_Signed_Writes (0x40)
#define HGICS_GATT_CHARAC_Extended_Properties (0x80)

#define BLENC_STR_CUTDOWN_BUFFER_LEN  (1024)
#define MIN(a, b)       (((a) < (b)) ? (a) : (b))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
int hgic_gatt_notify(uint16_t att_hdl, char *data, int len);

typedef int (*hgic_gatt_valhdl)(int hdl, int offset, char *data, int len);
struct hgic_gatt_hdr {
    uint16_t     att_hdl;
    union {
        uint8_t  att_type_128[16];
        uint16_t att_type;
    };
    uint16_t priv_type;
};
struct hgic_gatt_primary_service {
    struct hgic_gatt_hdr hdr;
    union {
        uint8_t  att_type_128[16];
        uint16_t att_type;
    };
};
struct hgic_gatt_characteristic {
    struct hgic_gatt_hdr hdr;
    uint8_t  properties;
    uint16_t value_hdl;
    union {
        uint8_t  value_type_128[16];
        uint16_t value_type;
    };
};
struct hgic_gatt_characteristic_value {
    struct hgic_gatt_hdr hdr;
    hgic_gatt_valhdl read_cb;
    hgic_gatt_valhdl write_cb;
};

#define HGICS_GATT_PRIMARY_SVR(hdl, type) struct hgic_gatt_primary_service att##hdl = {\
        .hdr = { \
             .att_hdl = (hdl), \
             .att_type = 0x2800,\
             .priv_type = 1,\
         }, \
        .att_type = (type),\
    }

#define HGICS_GATT_PRIMARY_SVR_128(hdl, type_128) struct hgic_gatt_primary_service att##hdl = {\
        .hdr = { \
             .att_hdl = (hdl), \
             .att_type = 0x2800,\
             .priv_type = 11,\
         }, \
        .att_type_128 = type_128,\
    }

#define HGICS_GATT_CHARACTER(hdl, flag, v_hdl, v_type) struct hgic_gatt_characteristic att##hdl = {\
        .hdr = { \
             .att_hdl = (hdl), \
             .att_type = 0x2803,\
             .priv_type = 2,\
         }, \
         .properties = (flag),\
         .value_hdl = (v_hdl),\
         .value_type = (v_type),\
    }

#define HGICS_GATT_CHARACTER_128(hdl, flag, v_hdl, v_type_128) struct hgic_gatt_characteristic att##hdl = {\
        .hdr = { \
            .att_hdl = hdl, \
            .att_type = 0x2803,\
            .priv_type = 21,\
        }, \
        .properties = flag,\
        .value_hdl = v_hdl,\
        .value_type_128 = v_type_128,\
    }

#define HGICS_GATT_CHARACTER_VALUE(hdl, type, read, write) struct hgic_gatt_characteristic_value att##hdl = {\
        .hdr = { \
             .att_hdl = (hdl), \
             .att_type = (type),\
             .priv_type = 4,\
         }, \
        .read_cb = (read),\
        .write_cb = (write),\
    }

#define HGICS_GATT_CHARACTER_VALUE_128(hdl, type_128, read, write) struct hgic_gatt_characteristic_value att##hdl = {\
        .hdr = { \
             .att_hdl = hdl, \
             .att_type_128 = type_128,\
             .priv_type = 41,\
         }, \
        .read_cb = read,\
        .write_cb = write,\
    }

#define HGICS_GATT_CHARACTER_CCCD(hdl, read, write) struct hgic_gatt_characteristic_value att##hdl = {\
        .hdr = { \
             .att_hdl = (hdl), \
             .att_type = (0x2902),\
             .priv_type = 4,\
         }, \
        .read_cb = (read),\
        .write_cb = (write),\
    }

/*
static inline void put_unaligned_le16(unsigned short val, unsigned char *p)
{
    *p++ = val;
    *p++ = val >> 8;
}
static inline unsigned short get_unaligned_le16(const unsigned char *p)
{
    return p[0] | p[1] << 8;
}
*/
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
int ble_connected = 0;
int att_mtu       = 23;
int mtu_exchanged = 0;
int ble_ll_length = 27;

//////////////////////////////////////////////////////////////////////////////
/*                  Define GATT Service --- Demo Service                    */
//////////////////////////////////////////////////////////////////////////////
extern int demo_app_recv_attdata(int hdl, int offset, char *data, int len);
extern int demo_app_read_attdata(int hdl, int offset, char *data, int len);

/* 通用UUID的定义，参考文档《BLE_Assigned_Numbers.pdf》, section 3: 16-bit UUIDs*/

/* Primary Service 1 */
HGICS_GATT_PRIMARY_SVR(1, 0x1800);
HGICS_GATT_CHARACTER(2, HGICS_GATT_CHARAC_Read, 3, 0x2a00);
HGICS_GATT_CHARACTER_VALUE(3, 0x2a00, demo_app_read_attdata, NULL);
HGICS_GATT_CHARACTER(4, HGICS_GATT_CHARAC_Read, 5, 0x2a01);
HGICS_GATT_CHARACTER_VALUE(5, 0x2a01, demo_app_read_attdata, NULL);

/* Primary Service 2 */
#if 0 //demo1, UUID 128
#define server_uuid {0x00,0x00,0x18,0x2c,0x00,0x00,0x10,0x00,0x80,0x00,0x00,0x80,0x5f,0x9b,0x34,0xfb}
#define data_uuid   {0x00,0x00,0x2A,0x7A,0x00,0x00,0x10,0x00,0x80,0x00,0x00,0x80,0x5f,0x9b,0x34,0xfb}
#define notity_uuid {0x00,0x00,0x2A,0x80,0x00,0x00,0x10,0x00,0x80,0x00,0x00,0x80,0x5f,0x9b,0x34,0xfb}
HGICS_GATT_PRIMARY_SVR_128(6, server_uuid);
HGICS_GATT_CHARACTER_128(7, HGICS_GATT_CHARAC_Write_Without_Response, 8, data_uuid);
HGICS_GATT_CHARACTER_VALUE_128(8, data_uuid, demo_app_read_attdata, demo_app_recv_attdata);
HGICS_GATT_CHARACTER_128(9, HGICS_GATT_CHARAC_Notify, 10, notity_uuid);
HGICS_GATT_CHARACTER_VALUE_128(10, notity_uuid, NULL, NULL);
HGICS_GATT_CHARACTER_CCCD(11, NULL, NULL);
#endif
#if 0 //demo2, UUID 128
#define server_uuid {0x06,0x05,0x04,0x03,0x02,0x01,0x03,0x00,0x02,0x00,0x01,0x00,0x01,0x00,0x00,0x36}
#define data_uuid   {0x06,0x05,0x04,0x03,0x02,0x01,0x03,0x00,0x02,0x00,0x01,0x00,0x02,0x00,0x00,0x36}
#define notity_uuid {0x06,0x05,0x04,0x03,0x02,0x01,0x03,0x00,0x02,0x00,0x01,0x00,0x03,0x00,0x00,0x36}
HGICS_GATT_PRIMARY_SVR_128(6, server_uuid);
HGICS_GATT_CHARACTER_128(7, HGICS_GATT_CHARAC_Write_Without_Response, 8, data_uuid);
HGICS_GATT_CHARACTER_VALUE_128(8, data_uuid, demo_app_read_attdata, demo_app_recv_attdata);
HGICS_GATT_CHARACTER_128(9, HGICS_GATT_CHARAC_Notify, 10, notity_uuid);
HGICS_GATT_CHARACTER_VALUE_128(10, notity_uuid, NULL, NULL);
HGICS_GATT_CHARACTER_CCCD(11, NULL, NULL);
#endif
#if 0 //demo 3: UUID 128
HGICS_GATT_PRIMARY_SVR(6, 0x1912);
HGICS_GATT_CHARACTER(7, HGICS_GATT_CHARAC_Read|HGICS_GATT_CHARAC_Write, 8, 0x2b13);
HGICS_GATT_CHARACTER_VALUE(8, 0x2b13, demo_app_read_attdata, demo_app_recv_attdata);
#define UUID1 {0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11}
HGICS_GATT_CHARACTER_128(7, HGICS_GATT_CHARAC_Write_Without_Response, 8, UUID1);
HGICS_GATT_CHARACTER_VALUE_128(8, UUID1, demo_app_read_attdata, demo_app_recv_attdata);
#endif
#if 1 //default demo: UUID 16
HGICS_GATT_PRIMARY_SVR(6, 0x1912);
HGICS_GATT_CHARACTER(7, HGICS_GATT_CHARAC_Read|HGICS_GATT_CHARAC_Write, 8, 0x2b13);
HGICS_GATT_CHARACTER_VALUE(8, 0x2b13, demo_app_read_attdata, demo_app_recv_attdata);
HGICS_GATT_CHARACTER(9, HGICS_GATT_CHARAC_Notify, 10, 0x2b10);
HGICS_GATT_CHARACTER_VALUE(10, 0x2b10, NULL, NULL);
HGICS_GATT_CHARACTER_CCCD(11, NULL, NULL);
#endif

static struct hgic_gatt_hdr *att_table[] = { (struct hgic_gatt_hdr *)&att1,
                                             (struct hgic_gatt_hdr *)&att2,
                                             (struct hgic_gatt_hdr *)&att3,
                                             (struct hgic_gatt_hdr *)&att4,
                                             (struct hgic_gatt_hdr *)&att5,
                                             (struct hgic_gatt_hdr *)&att6,
                                             (struct hgic_gatt_hdr *)&att7,
                                             (struct hgic_gatt_hdr *)&att8,
                                             (struct hgic_gatt_hdr *)&att9,
                                             (struct hgic_gatt_hdr *)&att10,
                                             (struct hgic_gatt_hdr *)&att11
                                           };
static char test_read_data[ATT_MTU]={0};
int demo_app_read_attdata(int hdl, int offset, char *data, int len)
{
    //setup test data.
    if(test_read_data[0] == 0){
        int i = 0;
        for(i=0; i<ATT_MTU; i++){
            test_read_data[i] = 0x41+(i/10);
        }
    }

    int ret = 0;
    switch (hdl) {
        case 8:
            ret = (ATT_MTU-offset) < len ? (ATT_MTU-offset) : len;
            if(ret > 0){
                memcpy(data, test_read_data+offset, ret);
            }
            printf("hdl 8 read, offset=%d, len=%d\r\n", offset, ret);
            break;
        default:
            sprintf(data, "hdl %d NULL Value", hdl);
            ret = strlen(data);
            break;
    }
    return ret;
}

int demo_app_recv_attdata(int hdl, int offset, char *data, int len)
{
#if 1 //sample code
    char *ssid, *psk, *keymgmt;
    uint8_t cmd[64];
    //char out[128];
    extern char *HGIC;

    printf("hdl %d recv data, len = %d, offset=%d\r\n", hdl, len, offset);
    if (hdl == 8 && data[0] == ':') {
        ssid = data + 1;
        psk = strchr(ssid, ',');
        if(psk == NULL) return -1;
        *psk++ = 0;
        keymgmt = strchr(psk, ',');
        if(keymgmt == NULL) return -1;
        *keymgmt++ = 0;

        printf("SET ssid:%s\r\n", ssid);
        hgics_wpacli_set_ssid("hg0", ssid);

        printf("SET passwd:%s\r\n", psk);
        hgics_wpacli_set_psk("hg0", psk);

        printf("SET keymgmt:%s\r\n", keymgmt[0] == '1' ? "WPA-PSK" : "NONE");
        hgics_wpacli_set_keymgmt("hg0", keymgmt[0] ? "WPA-PSK" : "NONE");

        hgic_blenc_release();

        hgic_iwpriv_blenc_start("hg0", 0, 38);

        hgics_wpacli_disable_network("hg0");
        hgics_wpacli_enable_network("hg0");
    }
#endif
}

void hgic_gatt_notify_test()
{
    static int __loop__  = 0;
    static int __value__ = 0;

    int  notify_hdl = 10;
    char notify_data[ATT_MTU];

    __loop__++;
    if(ble_connected && __loop__ > 50){
        __loop__ = 0;
        memset(notify_data, 'A'+__value__, att_mtu-3);
        __value__ = (__value__ >= 26 ? 0 : __value__+1);
        hgic_gatt_notify(notify_hdl, notify_data, att_mtu-3);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
int hgic_gatt_send_frag(char *att_data, int len, int tot_len, int first)
{
    int off = 0;
    int bc = 0;
    int pb = first ? 0 : 1;
    int ret = -ENOMEM;
    char *data = malloc(ble_ll_length+HCI_ACL_HDR_SIZE+4);
    if(data == NULL){
        printf("malloc fail, ble_ll_length=%d\r\n", ble_ll_length);
        return -1;
    }

    // 0 - Connection handle
    put_unaligned_le16(HCI_CONN_HANDLE|(pb<<12)|(bc<<14), data+off); off += 2;
    // 2 - ACL length
    put_unaligned_le16(len + 4u, data+off); off += 2;

    if(first){
        // 4 - L2CAP packet length: tot_len
        put_unaligned_le16(tot_len, data+off); off += 2;
        // 6 - L2CAP CID
        put_unaligned_le16(0x04, data+off); off += 2;
    }

    memcpy(data+off, att_data, len); off += len;
    ret = hgic_iwpriv_blenc_send_hcidata("hg0", HCI_ACL_DATA_PACKET, data, off);
    //printf("  -->send ble frag, len=%d\r\n", len);

    free(data);
    return ret >= 0 ? len : 0;
}

int hgic_gatt_send_data(char *att_data, int tot_len)
{
    int flen;
    int off = 0;

    //hgic_dump_hex("\r\nSEND:\r\n", att_data, tot_len, 1);
    while(off < tot_len){
        flen = ((off==0)?(ble_ll_length-4):ble_ll_length);
        flen = (tot_len-off)>flen?flen:(tot_len-off);
        flen = hgic_gatt_send_frag(att_data+off, flen, tot_len, (off==0));
        if(flen <= 0) break;
        off += flen;
    }
    return off;
}

// att_hdl: the CHARACTER_VALUE's handle that has Notify properties.
int hgic_gatt_notify(uint16_t att_hdl, char *data, int len)
{
    int ret = -ENOMEM;
    char buff[ATT_MTU];

    if(len > att_mtu - 3){
        printf("BLE: data len exceed MTU size! MTU is %d\r\n", att_mtu);
        return -E2BIG;
    }

    buff[0] = 0x1B;
    put_unaligned_le16(att_hdl, buff + 1);
    memcpy(buff + 3, data, len);
    printf("att hdl %d notify data, len:%d.\r\n", att_hdl, len);
    return hgic_gatt_send_data(buff, 3 + len);
}

void hgic_gatt_send_ATT_ERROR(uint8_t opcode, uint16_t atthandle, uint8_t err_code)
{
    uint8_t buff[5];
    buff[0] = 0x01;
    buff[1] = opcode;
    put_unaligned_le16(atthandle, buff + 2);
    buff[4] = err_code;
    hgic_gatt_send_data( buff, 5);
}

void hgic_gatt_send_PREPARE_WRITE_RESP(uint8_t *req, uint16_t len)
{
    req[0] = 0x17;
    hgic_gatt_send_data(req, len);
}

void hgic_gatt_EXCUTE_WRITE(uint8_t req)
{
    uint8_t opcode = 0x19;
    hgic_gatt_send_data(&opcode, 1);
}

void hgic_gatt_send_EXCHANGE_MTU(uint8_t opcode)
{
    uint8_t buff[5];
    buff[0] = opcode;
    put_unaligned_le16(ATT_MTU, buff + 1);
    hgic_gatt_send_data(buff, 3);
}

void hgic_gatt_EXCHANGE_MTU(uint32_t mtu, uint8_t req)
{
    if(mtu < 23) mtu = 23;
    att_mtu = MIN(mtu, ATT_MTU);
    mtu_exchanged = 1;
    printf("new ATT MTU %d\r\n", att_mtu);
    if(req){
        hgic_gatt_send_EXCHANGE_MTU(0x03);
    }
}

void hgic_gatt_READ_REQ(uint16_t att_hdl, uint32_t offset, uint8_t opcode)
{
    int i = 0;
    int len = -1;
    char resp[ATT_MTU];
    struct hgic_gatt_characteristic_value *v;

    resp[0] = opcode + 1;
    for (i = 0; i < ARRAY_SIZE(att_table); i++) {
        if (att_table[i]->att_hdl == att_hdl && (att_table[i]->priv_type == 4 || att_table[i]->priv_type == 41)) {
            v = (struct hgic_gatt_characteristic_value *)att_table[i];
            if (v->read_cb) {
                len = v->read_cb(att_table[i]->att_hdl, offset, resp + 1, att_mtu-1);
            }
            break;
        }
    }

    if (len >= 0) {
        hgic_gatt_send_data(resp, 1 + len);
    } else {
        printf("==>ATT_NOT_FOUND: att_hdl=%d\r\n", att_hdl);
        hgic_gatt_send_ATT_ERROR(opcode, att_hdl, 0x0a);
    }
}

void hgic_gatt_WRITE(uint16_t att_hdl, uint32_t offset, char *att_value, int value_len, uint8_t resp)
{
    int i = 0;
    struct hgic_gatt_characteristic_value *v;

    for (i = 0; i < ARRAY_SIZE(att_table); i++) {
        if (att_table[i]->att_hdl == att_hdl && (att_table[i]->priv_type == 4 || att_table[i]->priv_type == 41)) {
            v = (struct hgic_gatt_characteristic_value *)att_table[i];
            if (v->write_cb) {
                v->write_cb(att_table[i]->att_hdl, offset, att_value, value_len);
            }

            if (resp) {
                hgic_gatt_send_data(&resp, 1);
            }
            return;
        }
    }

    printf("==>ATT_NOT_FOUND: att_hdl=%d, resp=0x%x\r\n", att_hdl, resp & 0xff);
    hgic_gatt_send_ATT_ERROR(resp, att_hdl, 0x0a);
}

void hgic_gatt_FIND_INFORMATION_REQ(uint16_t start_hdl, uint16_t end_hdl)
{
    int i = 0;
    int off = 2;
    char buff[ATT_MTU];
    int item_len = 0;
    int parse_end = 0;

    buff[0] = 0x05;
    buff[1] = 0x1;

    for (i = 0; buff[1] && i < ARRAY_SIZE(att_table) && !parse_end; i++) {
        if (att_table[i]->att_hdl >= start_hdl && att_table[i]->att_hdl <= end_hdl) {
            if (item_len == 0) {
                item_len = (att_table[i]->priv_type == 21 || att_table[i]->priv_type == 41) ? 18 : 4;
            } else if (item_len != ((att_table[i]->priv_type == 21 || att_table[i]->priv_type == 41) ? 18 : 4)) {
                parse_end = 1;
                break;
            }

            if (att_table[i]->priv_type == 21 || att_table[i]->priv_type == 41) { //uuid 128
                put_unaligned_le16(att_table[i]->att_hdl, buff + off); off += 2;
                memcpy(buff + off, att_table[i]->att_type_128, 16); off += 16;
            }else{ // uuid 16
                put_unaligned_le16(att_table[i]->att_hdl, buff + off); off += 2;
                put_unaligned_le16(att_table[i]->att_type, buff + off); off += 2;
            }
            if (off > (att_mtu - item_len)) {
                parse_end = 1;
                break;
            }
        }
    }

    if (off > 2) {
        buff[1] = (item_len == 4 ? 0x1 : 02); //foramt
        hgic_gatt_send_data(buff, off);
    } else {
        printf("==>ATT_NOT_FOUND: start_hdl=%d, end_hdl=%d\r\n", start_hdl, end_hdl);
        hgic_gatt_send_ATT_ERROR(0x04, start_hdl, 0x0a);
    }
}

void hgic_gatt_READ_BY_TYPE_REQ(uint16_t start_hdl, uint16_t end_hdl, uint16_t att_type)
{
    int item_len = 0;
    int i = 0;
    int off = 2;
    char buff[ATT_MTU];
    int parse_end = 0;
    struct hgic_gatt_characteristic *c;

    buff[0] = 0x09;
    for (i = 0; i < ARRAY_SIZE(att_table) && !parse_end; i++) {
        if (att_table[i]->att_hdl >= start_hdl && att_table[i]->att_hdl <= end_hdl && att_table[i]->att_type == att_type) {
            switch (att_type) {
                case 0x2803: /*read Characteristic*/
                    c = (struct hgic_gatt_characteristic *)att_table[i];
                    if (item_len == 0) {
                        item_len = (c->hdr.priv_type == 21 || c->hdr.priv_type == 41) ? 21 : 7;
                    } else if (item_len != ((c->hdr.priv_type == 21 || c->hdr.priv_type == 41) ? 21 : 7)) {
                        parse_end = 1;
                        break;
                    }

                    if (c->hdr.priv_type == 21 || c->hdr.priv_type == 41) { //uuid 128
                        put_unaligned_le16(c->hdr.att_hdl, buff + off); off += 2;
                        buff[off] = c->properties; off += 1;
                        put_unaligned_le16(c->value_hdl, buff + off); off += 2;
                        memcpy(buff + off, c->value_type_128, 16); off += 16;
                    }else{ // uuid 16
                        put_unaligned_le16(c->hdr.att_hdl, buff + off); off += 2;
                        buff[off] = c->properties; off += 1;
                        put_unaligned_le16(c->value_hdl, buff + off); off += 2;
                        put_unaligned_le16(c->value_type, buff + off); off += 2;
                    }

                    if (off > (att_mtu - item_len)) {
                        parse_end = 1;
                        break;
                    }
                    break;
            };
        }
    }

    if (off > 2) {
        buff[1] = item_len;
        hgic_gatt_send_data(buff, off);
    } else {
        printf("==>ATT_NOT_FOUND: start_hdl=%d, end_hdl=%d, att_type=0x%x\r\n", start_hdl, end_hdl, att_type);
        hgic_gatt_send_ATT_ERROR(0x08, start_hdl, 0x0a);
    }
}

void hgic_gatt_READ_BY_GROUP_TYPE_REQ(uint16_t start_hdl, uint16_t end_hdl, uint16_t group_type)
{
    int item_len = 0;
    int off = 2;
    int i   = 0;
    char buff[ATT_MTU];
    int start_idx = -1;
    int end_indx  = -1;
    int parse_end   = 0;
    struct hgic_gatt_primary_service *p;

    buff[0] = 0x11;
    buff[1] = 0;
    for (i = 0; i < ARRAY_SIZE(att_table) && !parse_end; i++) {
        if (att_table[i]->att_hdl >= start_hdl && att_table[i]->att_hdl <= end_hdl) {
            switch (group_type) {
                case 0x2800: /*read Primary Service*/
                    if (att_table[i]->att_type == 0x2800) { //find a primary service
                        if (end_indx > 0) { //last group end
                            p = (struct hgic_gatt_primary_service *)att_table[start_idx];
                            put_unaligned_le16(att_table[end_indx]->att_hdl, buff + off); off += 2;
                            if (p->hdr.priv_type == 11) { // uuid 128
                                memcpy(buff+off, p->att_type_128, 16); off += 16;
                            } else { // uuid 16
                                put_unaligned_le16(p->att_type, buff + off); off += 2;
                            }
                        }

                        if (item_len == 0) {
                            item_len = (att_table[i]->priv_type == 11 ? 20 : 6);
                        } else if (item_len != (att_table[i]->priv_type == 11 ? 20 : 6)) {
                            parse_end = 1;
                            start_idx = -1;
                            break;
                        }

                        if (off > (att_mtu - item_len)) {
                            parse_end = 1;
                            start_idx = -1;
                            break;
                        }

                        start_idx = i; //new group start
                        put_unaligned_le16(att_table[start_idx]->att_hdl, buff + off); off += 2;
                    }

                    if (start_idx >= 0) end_indx = i;
                    break;
            }
        }
    }

    if (off > 2) {
        buff[1] = item_len;
        if (start_idx >= 0) {
            p = (struct hgic_gatt_primary_service *)att_table[start_idx];
            put_unaligned_le16(att_table[end_indx]->att_hdl, buff + off); off += 2;
            if (p->hdr.priv_type == 11) { // uuid 128
                memcpy(buff+off, p->att_type_128, 16); off += 16;
            } else { // uuid 16
                put_unaligned_le16(p->att_type, buff + off); off += 2;
            }
        }
        hgic_gatt_send_data(buff, off);
    } else {
        printf("==>ATT_NOT_FOUND: start_hdl=%d, end_hdl=%d, att_type=0x%x\r\n", start_hdl, end_hdl, group_type);
        hgic_gatt_send_ATT_ERROR(0x10, start_hdl, 0x0a);
    }
}

static void hgic_recv_ble_gatt_data(char *data, int len)
{
    unsigned char opcode = data[0];
    printf("recv att data, opcode=0x%x\r\n", opcode);
    switch (opcode) {
        case 0x02: //EXCHANGE_MTU_REQ
            hgic_gatt_EXCHANGE_MTU(get_unaligned_le16(data + 1), 1);
            break;
        case 0x03: //EXCHANGE_MTU_RESP
            hgic_gatt_EXCHANGE_MTU(get_unaligned_le16(data + 1), 0);
            break;
        case 0x04: //FIND_INFORMATION_REQ
            hgic_gatt_FIND_INFORMATION_REQ(get_unaligned_le16(data + 1),
                                           get_unaligned_le16(data + 3));
            break;
        case 0x06: //FIND_BY_TYPE_VALUE_REQ
            printf("==>FIND_BY_TYPE_VALUE_REQ: not support\r\n");
            hgic_gatt_send_ATT_ERROR(opcode, get_unaligned_le16(data + 1), 0x06);
            break;
        case 0x08: //READ_BY_TYPE_REQ
            hgic_gatt_READ_BY_TYPE_REQ(get_unaligned_le16(data + 1),
                                       get_unaligned_le16(data + 3),
                                       get_unaligned_le16(data + 5));
            break;
        case 0x0A: //READ_REQ
            hgic_gatt_READ_REQ(get_unaligned_le16(data + 1), 0, 0x0A);
            break;
        case 0x0C: //READ_BLOB_REQ
            hgic_gatt_READ_REQ(get_unaligned_le16(data + 1), get_unaligned_le16(data + 3), 0x0C);
            break;
        case 0x0e: //READ_MULTIPLE_REQ
            printf("==>READ_MULTIPLE_REQ: not support\r\n");
            hgic_gatt_send_ATT_ERROR(opcode, get_unaligned_le16(data + 1), 0x06);
            break;
        case 0x10: // READ_BY_GROUP_TYPE_REQ
            hgic_gatt_READ_BY_GROUP_TYPE_REQ(get_unaligned_le16(data + 1),
                                             get_unaligned_le16(data + 3),
                                             get_unaligned_le16(data + 5));
            break;
        case 0x12: //WRITE_REQ
            hgic_gatt_WRITE(get_unaligned_le16(data + 1), 0, data + 3, len - 3,  0x13);
            break;
        case 0x16: //PREPARE_WRITE_REQ
            hgic_gatt_WRITE(get_unaligned_le16(data + 1), get_unaligned_le16(data + 3), data + 5, len - 5, 0);
            hgic_gatt_send_PREPARE_WRITE_RESP(data, len);
            break;
        case 0x18: //EXECUTE_WRITE_REQ
            hgic_gatt_EXCUTE_WRITE(data[1]);
            break;
        case 0x52: //WRITE_CMD
            hgic_gatt_WRITE(get_unaligned_le16(data + 1), 0, data + 3, len - 3, 0);
            break;
        case 0xD2: //SIGNED_WRITE_CMD
            printf("==>SIGNED_WRITE_CMD: not support\r\n");
            hgic_gatt_send_ATT_ERROR(opcode, get_unaligned_le16(data + 1), 0x06);
            break;
        default:
            printf("==>unknow opcode: %x: not support\r\n", opcode);
            hgic_gatt_send_ATT_ERROR(opcode, get_unaligned_le16(data + 1), 0x06);
            break;
    }

    if(mtu_exchanged == 0){
        hgic_gatt_send_EXCHANGE_MTU(0x02);
    }
}

static void hgic_recv_l2cap_data(char type, char *data, int len)
{
    static char frag_buff[ATT_MTU];
    static int  frag_len = 0;
    static int  tot_len  = 0;
    int hdr_len;
    int hdr_cid;

    if (tot_len == 0) {
        hdr_len = get_unaligned_le16(data);
        hdr_cid = get_unaligned_le16(data + 2);
        if (hdr_cid != 0x4) {
            printf("%s,%d:Not ATT DATA\n",__FUNCTION__,__LINE__);
            frag_len = tot_len = 0;
            return; // Is not ATT data.
        }

        data += BT_L2CAP_HDR_SIZE;
        len  -= BT_L2CAP_HDR_SIZE;
        tot_len  = hdr_len;
        frag_len = 0;
    }

    if (frag_len + len > ATT_MTU) {
        printf("ATT_MTU is not enough. L2CAP_PKTSIZE_MAX=%d, frag_len=%d, len=%d\r\n", ATT_MTU, frag_len, len);
        frag_len = tot_len = 0;
        return;
    }

    memcpy(frag_buff + frag_len, data, len);
    frag_len += len;
    if (frag_len >= tot_len) {
        hgic_dump_hex("\r\nRECV:\r\n", frag_buff, frag_len, 1);
        hgic_recv_ble_gatt_data(frag_buff, frag_len);
        frag_len = tot_len = 0;
    }
}

static void hgic_recv_ble_event(char *data, int len)
{
    switch (data[2]) {
        case 0x1:
            att_mtu = 23;
            mtu_exchanged = 0;
            ble_connected = 1;
            ble_ll_length = 27;
            printf("BLE Connected\r\n");
            //hgic_gatt_notify_test();
            break;
        case 0x07:
            ble_ll_length = get_unaligned_le16(data+3);
            printf("BLE Exchange LL_LENGTH: tx=%d, rx=%d\r\n", get_unaligned_le16(data+3), get_unaligned_le16(data+5));
            break;
    }
}

static void hgic_recv_bt_event(char *data, int len)
{
    switch (data[0]) {
        case 0x05:
            att_mtu = 23;
            mtu_exchanged = 0;
            ble_connected = 0;
            ble_ll_length = 27;
            printf("Disconnect\r\n");
            break;
        case 0x3e:
            hgic_recv_ble_event(data, len);
            break;
        default:
            printf("%s,%d:Recv unknow evt:0x%x\n",__FUNCTION__,__LINE__,data[0]);
            break;
    }
}

void hgic_proc_bt_data(char *data, int len)
{
    extern struct hgic_fw_info hgic_fwinfo;
    struct hgic_ctrl_hdr *hdr = (struct hgic_ctrl_hdr *)data;

    data += sizeof(struct hgic_ctrl_hdr);
    len  -= sizeof(struct hgic_ctrl_hdr);
    switch (hdr->hci.type) {
        case 0x02: // ACL data
            if (hgic_fwinfo.version > 0x02040000) {
                data += sizeof(struct bt_rx_info);
                len  -= sizeof(struct bt_rx_info);
            }

            data += HCI_ACL_HDR_SIZE;
            len  -= HCI_ACL_HDR_SIZE;
            //hgic_dump_hex("BT ACL DATA:\r\n", data, len, 1);
            //hgic_dump_hex("ORI DATA:\r\n", hdr, len + sizeof(struct hgic_ctrl_hdr), 1);
            hgic_recv_l2cap_data(hdr->hci.type, data, len);
            break;
        case 0x04: // Event
            hgic_recv_bt_event(data, len);
            break;
    }
}

