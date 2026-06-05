/* main.c - Application main entry point */

/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/a2dp_codec_sbc.h>
#include <zephyr/bluetooth/classic/a2dp.h>
#include <zephyr/bluetooth/classic/avrcp.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/settings/settings.h>
#include "audio_buf.h"
#include "codec_play.h"

#define SAMPLE_BIT_WIDTH (16U)

struct bt_a2dp *default_a2dp;
struct bt_avrcp_ct *default_avrcp_ct;
BT_A2DP_SBC_SINK_EP_DEFAULT(sbc_sink_ep);
static struct bt_a2dp_stream sbc_stream;
BT_A2DP_SBC_EP_CFG_DEFAULT(sbc_cfg, A2DP_SBC_SAMP_FREQ_44100);

#define A2DP_VERSION 0x0104
#define AVRCP_VERSION 0x0106
#define AVCTP_VERSION 0x0103

/* AVRCP metadata cache */
static char current_title[256] = {0};

static struct bt_sdp_attribute a2dp_sink_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), /* 35 03 */
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16), /* 19 */
			BT_SDP_ARRAY_16(BT_SDP_AUDIO_SINK_SVCLASS) /* 11 0B */
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 16),/* 35 10 */
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),/* 35 06 */
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), /* 19 */
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP) /* 01 00 */
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16), /* 09 */
				BT_SDP_ARRAY_16(BT_UUID_AVDTP_VAL) /* 00 19 */
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),/* 35 06 */
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), /* 19 */
				BT_SDP_ARRAY_16(BT_UUID_AVDTP_VAL) /* 00 19 */
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16), /* 09 */
				BT_SDP_ARRAY_16(AVDTP_VERSION) /* AVDTP version: 01 03 */
			},
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8), /* 35 08 */
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6), /* 35 06 */
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), /* 19 */
				BT_SDP_ARRAY_16(BT_SDP_ADVANCED_AUDIO_SVCLASS) /* 11 0d */
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16), /* 09 */
				BT_SDP_ARRAY_16(A2DP_VERSION) /* 01 04 */
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("A2DPSink"),
	BT_SDP_SUPPORTED_FEATURES(0x0001U),
};

static struct bt_sdp_record a2dp_sink_rec = BT_SDP_RECORD(a2dp_sink_attrs);

static struct bt_sdp_attribute avrcp_ct_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_AV_REMOTE_SVCLASS)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 16),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(BT_UUID_AVCTP_VAL)
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_UUID_AVCTP_VAL)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(AVCTP_VERSION)
			},
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_AV_REMOTE_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(AVRCP_VERSION)
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("AVRCP_CT"),
	BT_SDP_SUPPORTED_FEATURES(0x0001U),
};

static struct bt_sdp_record avrcp_ct_rec = BT_SDP_RECORD(avrcp_ct_attrs);

/* Print current song information (lyrics style) */
static void print_current_song_info(void)
{
	if (current_title[0]) {
		printf("%s\n", current_title);
	}
}

/* Parse and update song title */
static void parse_song_title(struct net_buf *buf)
{
	struct bt_avrcp_get_element_attrs_rsp *rsp;
	struct bt_avrcp_media_attr *attr;
	uint8_t num_attrs;
	int i;
	uint16_t attr_len;
	bool updated = false;
	char new_title[sizeof(current_title)];

	if (!buf || buf->len < sizeof(*rsp)) {
		return;
	}

	rsp = (struct bt_avrcp_get_element_attrs_rsp *)buf->data;
	num_attrs = rsp->num_attrs;

	attr = rsp->attrs;
	for (i = 0; i < num_attrs; i++) {
		uint32_t attr_id = sys_be32_to_cpu(attr->attr_id);
		uint16_t charset_id = sys_be16_to_cpu(attr->charset_id);
		attr_len = sys_be16_to_cpu(attr->attr_len);

		/* Only process UTF-8 encoding (0x006a) or unspecified (0x0000) */
		if (charset_id != 0x006a && charset_id != 0x0000) {
			attr = (struct bt_avrcp_media_attr *)((uint8_t *)attr +
					sizeof(struct bt_avrcp_media_attr) + attr_len);
			continue;
		}

		if (attr_id == BT_AVRCP_MEDIA_ATTR_ID_TITLE) {
			if (attr_len < sizeof(new_title)) {
				memcpy(new_title, attr->attr_val, attr_len);
				new_title[attr_len] = '\0';
			} else {
				new_title[0] = '\0';
			}
		}

		/* Move to next attribute */
		attr = (struct bt_avrcp_media_attr *)((uint8_t *)attr +
				sizeof(struct bt_avrcp_media_attr) + attr_len);
	}

	/* Check if title has changed */
	if (strcmp(new_title, current_title) != 0) {
		updated = true;
		strcpy(current_title, new_title);
	}

	/* Only print if metadata has been updated */
	if (updated) {
		print_current_song_info();
	}
}

/* AVRCP Transaction Labels - different TID for different command types */
#define AVRCP_TID_GET_ATTRS     1  /* GetElementAttributes */
#define AVRCP_TID_REG_TRACK     2  /* RegisterNotification: TRACK_CHANGED */

/* Forward declaration for notification callback used in handlers */
static void track_changed_notification_cb(struct bt_avrcp_ct *ct, uint8_t event_id,
					  struct bt_avrcp_event_data *data);

/* Send get element attributes request */
static void request_element_attrs(void)
{
	struct net_buf *buf;
	struct bt_avrcp_get_element_attrs_cmd *cmd;
	uint32_t *attr_ids;
	int ret;

	if (!default_avrcp_ct) {
		return;
	}

	/* Use NULL to let AVRCP use default pool internally */
	buf = bt_avrcp_create_pdu(NULL);
	if (!buf) {
		return;
	}

	cmd = net_buf_add(buf, sizeof(*cmd));
	memset(cmd->identifier, 0, sizeof(cmd->identifier)); /* 0 = currently playing */
	cmd->num_attrs = 1; /* Request title only */

	attr_ids = net_buf_add(buf, sizeof(uint32_t));
	attr_ids[0] = sys_cpu_to_be32(BT_AVRCP_MEDIA_ATTR_ID_TITLE);

	ret = bt_avrcp_ct_get_element_attrs(default_avrcp_ct, AVRCP_TID_GET_ATTRS, buf);
	if (ret < 0) {
		net_buf_unref(buf);
	}
}

/* Handler for TRACK_CHANGED notification - request metadata then re-register */
static void avrcp_handle_track_changed(struct bt_avrcp_ct *ct)
{
	static uint32_t track_change_count = 0;
	int ret;

	request_element_attrs();

	/* Re-register TRACK_CHANGED notification immediately (cb is already cleared by stack) */
	ret = bt_avrcp_ct_register_notification(ct, AVRCP_TID_REG_TRACK,
						BT_AVRCP_EVT_TRACK_CHANGED, 0,
						track_changed_notification_cb);
	if (ret < 0) {
		printf("[AVRCP_ERR] Re-register TRACK_CHANGED failed: %d\n", ret);
	}
}

/* Track changed notification callback */
static void track_changed_notification_cb(struct bt_avrcp_ct *ct, uint8_t event_id,
					  struct bt_avrcp_event_data *data)
{
	if (event_id == BT_AVRCP_EVT_TRACK_CHANGED) {
		avrcp_handle_track_changed(ct);
	}
}

static void sbc_stop_play(void)
{
	printf("stream stopped\n");
	codec_play_stop();
}

void sbc_stream_configured(struct bt_a2dp_stream *stream)
{
	uint32_t sample_freq;
	uint8_t channel_num;
	struct bt_a2dp_codec_sbc_params *sbc_config = (struct bt_a2dp_codec_sbc_params *)
						      &sbc_cfg.codec_config->codec_ie[0];

	channel_num = bt_a2dp_sbc_get_channel_num(sbc_config);
	sample_freq = bt_a2dp_sbc_get_sampling_frequency(sbc_config);
	codec_play_configure(sample_freq, SAMPLE_BIT_WIDTH, channel_num);

	printf("stream configured\n");
}

void sbc_stream_established(struct bt_a2dp_stream *stream)
{
	printf("stream established\n");
}

void sbc_stream_released(struct bt_a2dp_stream *stream)
{
	sbc_stop_play();
}

void sbc_stream_started(struct bt_a2dp_stream *stream)
{
	printf("stream started\n");

	uint32_t sample_freq;
	struct bt_a2dp_codec_sbc_params *sbc_config = (struct bt_a2dp_codec_sbc_params *)
							      &sbc_cfg.codec_config->codec_ie[0];

	sample_freq = bt_a2dp_sbc_get_sampling_frequency(sbc_config);
	audio_buf_reset(sample_freq);
	codec_play_start();
}

void sbc_stream_suspended(struct bt_a2dp_stream *stream)
{
	sbc_stop_play();
}

void sbc_stream_recv(struct bt_a2dp_stream *stream, struct net_buf *buf, uint16_t seq_num,
		     uint32_t ts)
{
	uint8_t channel_num;
	struct bt_a2dp_codec_sbc_params *sbc_config = (struct bt_a2dp_codec_sbc_params *)
						      &sbc_cfg.codec_config->codec_ie[0];

	channel_num = bt_a2dp_sbc_get_channel_num(sbc_config);

	audio_process_sbc_buf(net_buf_pull_u8(buf), buf->data, buf->len, seq_num, ts, channel_num);
}

static struct bt_a2dp_stream_ops stream_ops = {
	.configured = sbc_stream_configured,
	.established = sbc_stream_established,
	.released = sbc_stream_released,
	.started = sbc_stream_started,
	.suspended = sbc_stream_suspended,
	.recv = sbc_stream_recv,
};

void app_a2dp_connected(struct bt_a2dp *a2dp, int err)
{
	if (err == 0) {
		default_a2dp = a2dp;
		printf("a2dp connected success\n");

		/* Request song metadata after A2DP connection */
		if (default_avrcp_ct) {
			request_element_attrs();
		}
	} else {
		printf("a2dp connected fail\n");
	}
}

void app_a2dp_disconnected(struct bt_a2dp *a2dp)
{
	default_a2dp = NULL;
	codec_play_stop();
	printf("a2dp disconnected\n");
}

int app_a2dp_config_req(struct bt_a2dp *a2dp, struct bt_a2dp_ep *ep,
			struct bt_a2dp_codec_cfg *codec_cfg, struct bt_a2dp_stream **stream,
			uint8_t *rsp_err_code)
{
	uint32_t sample_rate;

	*sbc_cfg.codec_config = *codec_cfg->codec_config;

	bt_a2dp_stream_cb_register(&sbc_stream, &stream_ops);
	*stream = &sbc_stream;
	*rsp_err_code = 0;

	printf("receive requesting config and accept\n");
	sample_rate = bt_a2dp_sbc_get_sampling_frequency(
		(struct bt_a2dp_codec_sbc_params *)&codec_cfg->codec_config->codec_ie[0]);
	printf("sample rate %dHz\n", sample_rate);

	return 0;
}

/* AVRCP CT notification callback (for interim responses) */
static void avrcp_ct_notification_cb(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status,
				     uint8_t event_id, struct bt_avrcp_event_data *data)
{
}

static struct bt_a2dp_cb a2dp_cb = {
	.connected = app_a2dp_connected,
	.disconnected = app_a2dp_disconnected,
	.config_req = app_a2dp_config_req,
};

/* AVRCP CT callback functions */
static void avrcp_ct_connected(struct bt_conn *conn, struct bt_avrcp_ct *ct)
{
	int ret;

	default_avrcp_ct = ct;

	memset(current_title, 0, sizeof(current_title));
	request_element_attrs();

	/* Register for track changed notification */
	ret = bt_avrcp_ct_register_notification(ct, AVRCP_TID_REG_TRACK,
						BT_AVRCP_EVT_TRACK_CHANGED,
						0, track_changed_notification_cb);
	if (ret < 0) {
		printf("Failed to register track changed notification: %d\n", ret);
	}
}

static void avrcp_ct_disconnected(struct bt_avrcp_ct *ct)
{
	printf("avrcp disconnected\n");

	default_avrcp_ct = NULL;
}

static void avrcp_ct_get_element_attrs_cb(struct bt_avrcp_ct *ct, uint8_t tid,
					  uint8_t status, struct net_buf *buf)
{
	if (status != BT_AVRCP_STATUS_SUCCESS) {
		printf("[AVRCP] get_element_attrs failed with status: %d\n", status);
		return;
	}

	if (!buf) {
		printf("[AVRCP] get_element_attrs: buf is NULL\n");
		return;
	}

	parse_song_title(buf);
}

static void avrcp_ct_passthrough_rsp(struct bt_avrcp_ct *ct, uint8_t tid,
				     bt_avrcp_rsp_t result,
				     const struct bt_avrcp_passthrough_rsp *rsp)
{
	uint8_t opid = rsp->opid_state & 0x7F;
	uint8_t state = (rsp->opid_state >> 7) & 0x01;

	if (result != BT_AVRCP_RSP_ACCEPTED) {
		printf("AVRCP passthrough rejected, opid=0x%02x, result=%d\n",
		       opid, result);
	}
}

static struct bt_avrcp_ct_cb avrcp_ct_cb = {
	.connected = avrcp_ct_connected,
	.disconnected = avrcp_ct_disconnected,
	.get_element_attrs = avrcp_ct_get_element_attrs_cb,
	.passthrough_rsp = avrcp_ct_passthrough_rsp,
	.notification = avrcp_ct_notification_cb,
};

static void bt_ready(int err)
{
	if (err != 0) {
		printf("Bluetooth init failed (err %d)\n", err);
		return;
	}

	if (IS_ENABLED(SETTINGS)) {
		settings_load();
	}

	printf("Bluetooth initialized\n");

	bt_sdp_register_service(&a2dp_sink_rec);
	bt_sdp_register_service(&avrcp_ct_rec);

	bt_a2dp_register_ep(&sbc_sink_ep, BT_AVDTP_AUDIO, BT_AVDTP_SINK);
	bt_a2dp_register_cb(&a2dp_cb);

	bt_avrcp_ct_register_cb(&avrcp_ct_cb);

	err = bt_br_set_connectable(true, NULL);
	if (err != 0) {
		printf("BR/EDR set/rest connectable failed (err %d)\n", err);
		return;
	}
	err = bt_br_set_discoverable(true, false);
	if (err != 0) {
		printf("BR/EDR set discoverable failed (err %d)\n", err);
		return;
	}

	printf("BR/EDR set connectable and discoverable done\n");
}

int a2dp_sink(int argc, char **argv)
{
	int err;

	if (codec_play_init() != 0) {
		printf("Codec init failed\n");
		return 0;
	}

	err = bt_enable(bt_ready);
	if (err != 0) {
		printf("Bluetooth enable failed: %d\n", err);
		return 0;
	}

	rt_thread_t thread = rt_thread_create("codec_keep_play", codec_keep_play, RT_NULL, 1024*4, 11, 2);
	if (thread != RT_NULL) {
		rt_thread_startup(thread);
	}

	return 0;
}
MSH_CMD_EXPORT_ALIAS(a2dp_sink, a2dp_sink, "bluetoooth a2dp_sink");
