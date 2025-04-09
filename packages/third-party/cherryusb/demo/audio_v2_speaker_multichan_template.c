/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "usbd_core.h"
#include "usbd_audio.h"
#include <rtthread.h>
#include <rtdevice.h>
#include "aic_audio_render_manager.h"


#define USBD_VID           0x33C3
#define USBD_PID           0xffff
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

#ifdef CONFIG_USB_HS
#define EP_INTERVAL 0x04
#else
#define EP_INTERVAL 0x01
#endif

#define AUDIO_OUT_EP 0x01

#define AUDIO_OUT_CLOCK_ID 0x01
#define AUDIO_OUT_FU_ID    0x03

#define AUDIO_FREQ      48000
#define HALF_WORD_BYTES 2  //2 half word (one channel)
#define SAMPLE_BITS     16 //16 bit per channel

#define BMCONTROL (AUDIO_V2_FU_CONTROL_MUTE | AUDIO_V2_FU_CONTROL_VOLUME)

#define OUT_CHANNEL_NUM 2

#if OUT_CHANNEL_NUM == 1
#define OUTPUT_CTRL      DBVAL(BMCONTROL), DBVAL(BMCONTROL)
#define OUTPUT_CH_ENABLE 0x00000000
#elif OUT_CHANNEL_NUM == 2
#define OUTPUT_CTRL      DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL)
#define OUTPUT_CH_ENABLE 0x00000003
#elif OUT_CHANNEL_NUM == 3
#define OUTPUT_CTRL      DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL)
#define OUTPUT_CH_ENABLE 0x00000007
#elif OUT_CHANNEL_NUM == 4
#define OUTPUT_CTRL      DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL)
#define OUTPUT_CH_ENABLE 0x0000000f
#elif OUT_CHANNEL_NUM == 5
#define OUTPUT_CTRL      DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL)
#define OUTPUT_CH_ENABLE 0x0000001f
#elif OUT_CHANNEL_NUM == 6
#define OUTPUT_CTRL      DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL)
#define OUTPUT_CH_ENABLE 0x0000003F
#elif OUT_CHANNEL_NUM == 7
#define OUTPUT_CTRL      DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL)
#define OUTPUT_CH_ENABLE 0x0000007f
#elif OUT_CHANNEL_NUM == 8
#define OUTPUT_CTRL      DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL)
#define OUTPUT_CH_ENABLE 0x000000ff
#endif

#define AUDIO_OUT_PACKET ((uint32_t)((AUDIO_FREQ * HALF_WORD_BYTES * OUT_CHANNEL_NUM) / 1000))

#define USB_AUDIO_CONFIG_DESC_SIZ (9 +                                                     \
                                   AUDIO_V2_AC_DESCRIPTOR_INIT_LEN +                       \
                                   AUDIO_V2_SIZEOF_AC_CLOCK_SOURCE_DESC +                  \
                                   AUDIO_V2_SIZEOF_AC_INPUT_TERMINAL_DESC +                \
                                   AUDIO_V2_SIZEOF_AC_FEATURE_UNIT_DESC(OUT_CHANNEL_NUM) + \
                                   AUDIO_V2_SIZEOF_AC_OUTPUT_TERMINAL_DESC +               \
                                   AUDIO_V2_AS_DESCRIPTOR_INIT_LEN)

#define AUDIO_AC_SIZ (AUDIO_V2_SIZEOF_AC_HEADER_DESC +                        \
                      AUDIO_V2_SIZEOF_AC_CLOCK_SOURCE_DESC +                  \
                      AUDIO_V2_SIZEOF_AC_INPUT_TERMINAL_DESC +                \
                      AUDIO_V2_SIZEOF_AC_FEATURE_UNIT_DESC(OUT_CHANNEL_NUM) + \
                      AUDIO_V2_SIZEOF_AC_OUTPUT_TERMINAL_DESC)

uint8_t audio_v2_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0001, 0x01),
    USB_CONFIG_DESCRIPTOR_INIT(USB_AUDIO_CONFIG_DESC_SIZ, 0x02, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    AUDIO_V2_AC_DESCRIPTOR_INIT(0x00, 0x02, AUDIO_AC_SIZ, AUDIO_CATEGORY_SPEAKER, 0x00, 0x00),
    AUDIO_V2_AC_CLOCK_SOURCE_DESCRIPTOR_INIT(AUDIO_OUT_CLOCK_ID, 0x03, 0x03),
    AUDIO_V2_AC_INPUT_TERMINAL_DESCRIPTOR_INIT(0x02, AUDIO_TERMINAL_STREAMING, 0x01, OUT_CHANNEL_NUM, OUTPUT_CH_ENABLE, 0x0000),
    AUDIO_V2_AC_FEATURE_UNIT_DESCRIPTOR_INIT(AUDIO_OUT_FU_ID, 0x02, OUTPUT_CTRL),
    AUDIO_V2_AC_OUTPUT_TERMINAL_DESCRIPTOR_INIT(0x04, AUDIO_OUTTERM_SPEAKER, 0x03, 0x01, 0x0000),
    AUDIO_V2_AS_DESCRIPTOR_INIT(0x01, 0x02, OUT_CHANNEL_NUM, OUTPUT_CH_ENABLE, HALF_WORD_BYTES, SAMPLE_BITS, AUDIO_OUT_EP, 0x09, AUDIO_OUT_PACKET, EP_INTERVAL),
    ///////////////////////////////////////
    /// string0 descriptor
    ///////////////////////////////////////
    USB_LANGID_INIT(USBD_LANGID_STRING),
    ///////////////////////////////////////
    /// string1 descriptor
    ///////////////////////////////////////
    0x14,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'C', 0x00,                  /* wcChar0 */
    'h', 0x00,                  /* wcChar1 */
    'e', 0x00,                  /* wcChar2 */
    'r', 0x00,                  /* wcChar3 */
    'r', 0x00,                  /* wcChar4 */
    'y', 0x00,                  /* wcChar5 */
    'U', 0x00,                  /* wcChar6 */
    'S', 0x00,                  /* wcChar7 */
    'B', 0x00,                  /* wcChar8 */
    ///////////////////////////////////////
    /// string2 descriptor
    ///////////////////////////////////////
    0x26,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'C', 0x00,                  /* wcChar0 */
    'h', 0x00,                  /* wcChar1 */
    'e', 0x00,                  /* wcChar2 */
    'r', 0x00,                  /* wcChar3 */
    'r', 0x00,                  /* wcChar4 */
    'y', 0x00,                  /* wcChar5 */
    'U', 0x00,                  /* wcChar6 */
    'S', 0x00,                  /* wcChar7 */
    'B', 0x00,                  /* wcChar8 */
    ' ', 0x00,                  /* wcChar9 */
    'U', 0x00,                  /* wcChar10 */
    'A', 0x00,                  /* wcChar11 */
    'C', 0x00,                  /* wcChar12 */
    ' ', 0x00,                  /* wcChar13 */
    'D', 0x00,                  /* wcChar14 */
    'E', 0x00,                  /* wcChar15 */
    'M', 0x00,                  /* wcChar16 */
    'O', 0x00,                  /* wcChar17 */
    ///////////////////////////////////////
    /// string3 descriptor
    ///////////////////////////////////////
    0x16,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    '2', 0x00,                  /* wcChar0 */
    '0', 0x00,                  /* wcChar1 */
    '2', 0x00,                  /* wcChar2 */
    '1', 0x00,                  /* wcChar3 */
    '0', 0x00,                  /* wcChar4 */
    '3', 0x00,                  /* wcChar5 */
    '1', 0x00,                  /* wcChar6 */
    '0', 0x00,                  /* wcChar7 */
    '0', 0x00,                  /* wcChar8 */
    '3', 0x00,                  /* wcChar9 */
#ifdef CONFIG_USB_HS
    ///////////////////////////////////////
    /// device qualifier descriptor
    ///////////////////////////////////////
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x01,
    0x00,
#endif
    0x00
};

static const uint8_t default_sampling_freq_table[] = {
    AUDIO_SAMPLE_FREQ_NUM(5),
    AUDIO_SAMPLE_FREQ_4B(8000),
    AUDIO_SAMPLE_FREQ_4B(8000),
    AUDIO_SAMPLE_FREQ_4B(0x00),
    AUDIO_SAMPLE_FREQ_4B(16000),
    AUDIO_SAMPLE_FREQ_4B(16000),
    AUDIO_SAMPLE_FREQ_4B(0x00),
    AUDIO_SAMPLE_FREQ_4B(32000),
    AUDIO_SAMPLE_FREQ_4B(32000),
    AUDIO_SAMPLE_FREQ_4B(0x00),
    AUDIO_SAMPLE_FREQ_4B(48000),
    AUDIO_SAMPLE_FREQ_4B(48000),
    AUDIO_SAMPLE_FREQ_4B(0x00),
    AUDIO_SAMPLE_FREQ_4B(96000),
    AUDIO_SAMPLE_FREQ_4B(96000),
    AUDIO_SAMPLE_FREQ_4B(0x00),
};

#define UAC_CTRL_START          BIT(0)
#define UAC_CTRL_STOP           BIT(1)
#define UAC_CTRL_SET_VOLUME     BIT(2)
#define UAC_CTRL_MUTE           BIT(3)
#define UAC_CTRL_RECONFIG       BIT(4)

void usbd_audio_iso_out_callback(uint8_t ep, uint32_t nbytes);

struct audio_volume_mute {
    int  volume;
    bool mute;
} usbd_volume_mute;

struct uac_msg {
    u32 cmd;
    u32 data;
};


static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t read_buffer[AUDIO_OUT_PACKET];

struct usbd_uac_t {
    struct aic_audio_render *render;
    struct aic_audio_render_attr ao_attr;
    struct aic_audio_render_transfer_buffer transfer_buffer;
    bool reconfig_flag;
};

static struct usbd_uac_t g_usbd_audio = {0};
static char g_uac_mq_buf[128] = {0};
static struct rt_messagequeue g_uac_mq = {0};

static struct usbd_endpoint audio_out_ep = {
    .ep_cb = usbd_audio_iso_out_callback,
    .ep_addr = AUDIO_OUT_EP
};

struct usbd_interface uac_intf0;
struct usbd_interface uac_intf1;

struct audio_entity_info audio_entity_table[] = {
    { .bEntityId = AUDIO_OUT_CLOCK_ID,
      .bDescriptorSubtype = AUDIO_CONTROL_CLOCK_SOURCE,
      .ep = AUDIO_OUT_EP },
    { .bEntityId = AUDIO_OUT_FU_ID,
      .bDescriptorSubtype = AUDIO_CONTROL_FEATURE_UNIT,
      .ep = AUDIO_OUT_EP },
};

void usbd_clear_buffer(void)
{
    static uint8_t reset_num = 1;
    /* The usb enumeration generates two reset operations.*/
    if (reset_num++>2) {
        memset(read_buffer, 0, AUDIO_OUT_PACKET);
    }
}

#ifdef LPKG_CHERRYUSB_DEVICE_COMPOSITE
void usbd_comp_uac_event_handler(uint8_t event)
#else
void usbd_event_handler(uint8_t event)
#endif
{
    switch (event) {
        case USBD_EVENT_RESET:
            usbd_clear_buffer();
            break;
        case USBD_EVENT_CONNECTED:
            break;
        case USBD_EVENT_DISCONNECTED:
            break;
        case USBD_EVENT_RESUME:
            break;
        case USBD_EVENT_SUSPEND:
            break;
        case USBD_EVENT_CONFIGURED:
            break;
        case USBD_EVENT_SET_REMOTE_WAKEUP:
            break;
        case USBD_EVENT_CLR_REMOTE_WAKEUP:
            break;

        default:
            break;
    }
}

void usbd_audio_event_handler(int event)
{
    struct uac_msg msg = {0, 0};
    switch (event) {
        case AUDIO_RENDER_EVENT_RECONFIG:
            msg.cmd = UAC_CTRL_RECONFIG;
            g_usbd_audio.reconfig_flag = 1;
            rt_mq_send(&g_uac_mq, &msg, sizeof(struct uac_msg));
            break;

        default:
            break;
    }
}

static void uac_audio_ctrl_set_volume(int volume)
{
    struct usbd_uac_t *usbd_audio = &g_usbd_audio;

    if (aic_audio_render_control(usbd_audio->render,
                                 AUDIO_RENDER_CMD_SET_VOL,
                                 &volume)) {
        USB_LOG_ERR("aic_audio_render_control set priority failed\n");
    }
}

void usbd_audio_set_volume(uint8_t ep, uint8_t ch, int volume)
{
    struct uac_msg msg = {UAC_CTRL_SET_VOLUME, volume};

    usbd_volume_mute.volume = volume;
    rt_mq_send(&g_uac_mq, &msg, sizeof(struct uac_msg));
}

int usbd_audio_get_volume(uint8_t ep, uint8_t ch)
{
    return usbd_volume_mute.volume;
}

void usbd_audio_set_mute(uint8_t ep, uint8_t ch, bool mute)
{
    struct uac_msg msg = {UAC_CTRL_MUTE, 0};

    if (usbd_volume_mute.mute == mute)
        return;

    USB_LOG_DBG("UAC Set mute: %d\n", mute);
    usbd_volume_mute.mute = mute;
    if (mute)
        rt_mq_send(&g_uac_mq, &msg, sizeof(struct uac_msg));
}

bool usbd_audio_get_mute(uint8_t ep, uint8_t ch)
{
    return usbd_volume_mute.mute;
}

void usbd_audio_set_sampling_freq(uint8_t ep, uint32_t sampling_freq)
{
    uint16_t packet_size = 0;

    USB_LOG_DBG("UAC set sampling rate %ld Hz\n", (long)sampling_freq);
    if (ep == audio_out_ep.ep_addr) {
        packet_size = ((sampling_freq * 2 * OUT_CHANNEL_NUM) / 1000);
        audio_v2_descriptor[18 + USB_AUDIO_CONFIG_DESC_SIZ - 11] = packet_size;
        audio_v2_descriptor[18 + USB_AUDIO_CONFIG_DESC_SIZ - 10] = packet_size >> 8;
    }
}

void usbd_audio_get_sampling_freq_table(uint8_t ep, uint8_t **sampling_freq_table)
{
    if (ep == audio_out_ep.ep_addr) {
        *sampling_freq_table = (uint8_t *)default_sampling_freq_table;
    }
}

rt_mutex_t usbd_uac_mutex;


int usbd_audio_render_open()
{
    int value = 0;
    struct usbd_uac_t *usbd_audio = &g_usbd_audio;

    memset(usbd_audio, 0, sizeof(struct usbd_uac_t));

    struct audio_render_create_params create_params;
    create_params.dev_id = 0;
    create_params.scene_type = AUDIO_RENDER_SCENE_UAC;
    if (aic_audio_render_create(&usbd_audio->render, &create_params)) {
        USB_LOG_ERR("aic_audio_render_create failed\n");
        return -1;
    }

    if (aic_audio_render_init(usbd_audio->render)) {
        USB_LOG_ERR("aic_audio_render_init failed\n");
        goto exit;
    }

    if (aic_audio_render_control(usbd_audio->render,
                                 AUDIO_RENDER_CMD_SET_EVENT_HANDLER,
                                 usbd_audio_event_handler)) {
        USB_LOG_ERR("aic_audio_render_control set event handler failed\n");
        goto exit;
    }

    value = AUDIO_RENDER_SCENE_LOWEST_PRIORITY;
    if (aic_audio_render_control(usbd_audio->render,
                                 AUDIO_RENDER_CMD_SET_SCENE_PRIORITY,
                                 &value)) {
        USB_LOG_ERR("aic_audio_render_control set priority failed\n");
        goto exit;
    }

    usbd_audio->ao_attr.sample_rate = AUDIO_FREQ;
    usbd_audio->ao_attr.channels   = OUT_CHANNEL_NUM;
    if (aic_audio_render_control(usbd_audio->render,
                                AUDIO_RENDER_CMD_SET_ATTR,
                                &usbd_audio->ao_attr)) {
        USB_LOG_ERR("aic_audio_render_control set priority failed\n");
        goto exit;
    }

    if (aic_audio_render_control(usbd_audio->render,
                                AUDIO_RENDER_CMD_GET_TRANSFER_BUFFER,
                                &usbd_audio->transfer_buffer)) {
        USB_LOG_ERR("aic_audio_render_control get transfer_buffer failed\n");
        goto exit;
    }

    usbd_volume_mute.volume = 100;
    if (aic_audio_render_control(usbd_audio->render,
                                AUDIO_RENDER_CMD_SET_VOL,
                                &usbd_volume_mute.volume)) {
        USB_LOG_ERR("aic_audio_render_control set volume failed\n");
        goto exit;
    }

    return 0;

exit:
    aic_audio_render_destroy(usbd_audio->render);
    usbd_audio->render = NULL;
    return -1;
}


void usbd_audio_open(uint8_t intf)
{
    struct uac_msg msg = {UAC_CTRL_START, 0};

    /* setup first out ep read transfer */
    usbd_ep_start_read(audio_out_ep.ep_addr, read_buffer, AUDIO_OUT_PACKET);
    USB_LOG_DBG("UAC OPEN\r\n");
    rt_mq_send(&g_uac_mq, &msg, sizeof(struct uac_msg));
}

void usbd_audio_close(uint8_t intf)
{
    struct uac_msg msg = {UAC_CTRL_STOP, 0};

    USB_LOG_DBG("UAC CLOSE\r\n");
    rt_mq_send(&g_uac_mq, &msg, sizeof(struct uac_msg));
}


void usbd_audio_iso_out_callback(uint8_t ep, uint32_t nbytes)
{
    struct usbd_uac_t *usbd_audio = &g_usbd_audio;
    if  (!usbd_audio->render || !usbd_audio->transfer_buffer.buffer) {
        USB_LOG_ERR("%s(%d), render or transfer_buffer is null\n", __func__, __LINE__);
        return;
    }

    /*Need drop the uac data when reconfiging audio render*/
    if (g_usbd_audio.reconfig_flag) {
        usbd_ep_start_read(audio_out_ep.ep_addr, read_buffer, AUDIO_OUT_PACKET);
        return;
    }

    /*if process in local mode need bypass uac packet*/
    if (0 == aic_audio_render_control(usbd_audio->render,
                                      AUDIO_RENDER_CMD_GET_BYPASS,
                                      NULL)) {
        aic_audio_render_rend(usbd_audio->render, usbd_audio->transfer_buffer.buffer, AUDIO_OUT_PACKET);
        usbd_ep_start_read(audio_out_ep.ep_addr, usbd_audio->transfer_buffer.buffer, AUDIO_OUT_PACKET);
    } else {
        usbd_ep_start_read(audio_out_ep.ep_addr, read_buffer, AUDIO_OUT_PACKET);
    }
}

static void uac_thread_entry(void *arg)
{
    struct uac_msg msg = {0};
    rt_err_t ret = RT_EOK;

    while (1) {
        ret = rt_mq_recv(&g_uac_mq, &msg, sizeof(struct uac_msg), RT_WAITING_FOREVER);
        if (ret != RT_EOK) {
            LOG_E("UAC: Failed to receive msg, return %d\n", ret);
            continue;
        }
        USB_LOG_DBG("Recv msg: cmd 0x%x, data %d\n", msg.cmd, msg.data);

        switch (msg.cmd) {
        case UAC_CTRL_START:
            // uac_audio_ctrl_start();
            break;

        case UAC_CTRL_STOP:
            // uac_audio_ctrl_stop();
            break;

        case UAC_CTRL_SET_VOLUME:
            uac_audio_ctrl_set_volume(msg.data);
            break;

        case UAC_CTRL_MUTE:
            uac_audio_ctrl_set_volume(0);
            break;

        case UAC_CTRL_RECONFIG:
            if (aic_audio_render_control(g_usbd_audio.render,
                                        AUDIO_RENDER_CMD_SET_ATTR,
                                        &g_usbd_audio.ao_attr)) {
                USB_LOG_ERR("aic_audio_render_control set priority failed\n");
                return;
            }
            g_usbd_audio.reconfig_flag = 0;
            USB_LOG_INFO("aic_audio_render_control reconfig over\n");
            break;
        default:
            USB_LOG_ERR("Invalid msg cmd: 0x%x\n", msg.cmd);
            break;
        }
    }
}

#ifdef LPKG_CHERRYUSB_DEVICE_COMPOSITE
#include "composite_template.h"
int usbd_comp_uac_init(uint8_t *ep_table, void *data)
{
    audio_entity_table[0].ep = ep_table[0];
    audio_entity_table[1].ep = ep_table[0];
    audio_out_ep.ep_addr = ep_table[0];
    usbd_add_interface(usbd_audio_init_intf(&uac_intf0, 0x0200, audio_entity_table, 2));
    usbd_add_interface(usbd_audio_init_intf(&uac_intf1, 0x0200, audio_entity_table, 2));
    usbd_add_endpoint(&audio_out_ep);
    return 0;
}
#endif

int usbd_audio_v2_init(void)
{
    rt_thread_t usb_audio_tid = RT_NULL;
    int ret = 0;

    usbd_uac_mutex = rt_mutex_create("usbd_uac_mutex", RT_IPC_FLAG_PRIO);
    if (usbd_uac_mutex == NULL) {
        USB_LOG_ERR("COMP: create dynamic mutex falied.\n");
        return -1;
    }

    usbd_audio_render_open();

    ret = rt_mq_init(&g_uac_mq, "uac_mq", g_uac_mq_buf,
                     sizeof(struct uac_msg), sizeof(g_uac_mq_buf),
                     RT_IPC_FLAG_FIFO);
    if (ret) {
        LOG_E("Init UAC messagequeue failed\n");
        return -1;
    }

    usb_audio_tid = rt_thread_create("uac", uac_thread_entry, RT_NULL, 2048, 10, 50);
    if (usb_audio_tid != RT_NULL)
        rt_thread_startup(usb_audio_tid);

#ifndef LPKG_CHERRYUSB_DEVICE_COMPOSITE
    usbd_desc_register(audio_v2_descriptor);
    usbd_add_interface(usbd_audio_init_intf(&uac_intf0, 0x0200, audio_entity_table, 2));
    usbd_add_interface(usbd_audio_init_intf(&uac_intf1, 0x0200, audio_entity_table, 2));
    usbd_add_endpoint(&audio_out_ep);
    usbd_initialize();
#else
    usbd_comp_func_register(audio_v2_descriptor,
                            usbd_comp_uac_event_handler,
                            usbd_comp_uac_init, NULL);
#endif

    return 0;
}
#if !defined(LPKG_CHERRYUSB_DYNAMIC_REGISTRATION_MODE)
INIT_APP_EXPORT(usbd_audio_v2_init);
#endif
