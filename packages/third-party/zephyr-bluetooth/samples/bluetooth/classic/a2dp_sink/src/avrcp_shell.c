/*
 * Copyright 2025 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/net_buf.h>
#include <zephyr/bluetooth/classic/avrcp.h>

/* Extern from main.c */
extern struct bt_avrcp_ct *default_avrcp_ct;

static int avrcp_send_passthrough(uint8_t opid)
{
	int ret;

	if (!default_avrcp_ct) {
		printf("AVRCP not connected\n");
		return -1;
	}

	/* Send pressed */
	ret = bt_avrcp_ct_passthrough(default_avrcp_ct, 0, opid, 0, NULL, 0);
	if (ret < 0) {
		printf("Failed to send passthrough pressed: %d\n", ret);
		return ret;
	}

	/* Send released */
	ret = bt_avrcp_ct_passthrough(default_avrcp_ct, 0, opid, 1, NULL, 0);
	if (ret < 0) {
		printf("Failed to send passthrough released: %d\n", ret);
		return ret;
	}

	return 0;
}

static int avrcp_play(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	printf("AVRCP: play\n");
	return avrcp_send_passthrough(BT_AVRCP_OPID_PLAY);
}

static int avrcp_pause(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	printf("AVRCP: pause\n");
	return avrcp_send_passthrough(BT_AVRCP_OPID_PAUSE);
}

static int avrcp_next(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	printf("AVRCP: next\n");
	return avrcp_send_passthrough(BT_AVRCP_OPID_FORWARD);
}

static int avrcp_prev(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	printf("AVRCP: previous\n");
	return avrcp_send_passthrough(BT_AVRCP_OPID_BACKWARD);
}

static int avrcp_get_info(int argc, char **argv)
{
	struct net_buf *buf;
	struct bt_avrcp_get_element_attrs_cmd *cmd;
	uint32_t *attr_ids;
	int ret;

	(void)argc;
	(void)argv;

	if (!default_avrcp_ct) {
		printf("AVRCP not connected\n");
		return -1;
	}

	buf = bt_avrcp_create_pdu(NULL);
	if (!buf) {
		printf("Failed to create PDU\n");
		return -1;
	}

	cmd = net_buf_add(buf, sizeof(*cmd));
	memset(cmd->identifier, 0, sizeof(cmd->identifier));
	cmd->num_attrs = 1;

	attr_ids = net_buf_add(buf, sizeof(uint32_t));
	attr_ids[0] = sys_cpu_to_be32(BT_AVRCP_MEDIA_ATTR_ID_TITLE);

	ret = bt_avrcp_ct_get_element_attrs(default_avrcp_ct, 0, buf);
	if (ret < 0) {
		printf("Failed to get element attrs: %d\n", ret);
		net_buf_unref(buf);
		return ret;
	}

	return 0;
}

static int avrcp_get_status(int argc, char **argv)
{
	int ret;

	(void)argc;
	(void)argv;

	if (!default_avrcp_ct) {
		printf("AVRCP not connected\n");
		return -1;
	}

	ret = bt_avrcp_ct_get_play_status(default_avrcp_ct, 0);
	if (ret < 0) {
		printf("Failed to get play status: %d\n", ret);
		return ret;
	}

	return 0;
}

MSH_CMD_EXPORT_ALIAS(avrcp_play, avrcp_play, "AVRCP: send play command");
MSH_CMD_EXPORT_ALIAS(avrcp_pause, avrcp_pause, "AVRCP: send pause command");
MSH_CMD_EXPORT_ALIAS(avrcp_next, avrcp_next, "AVRCP: send next track command");
MSH_CMD_EXPORT_ALIAS(avrcp_prev, avrcp_prev, "AVRCP: send previous track command");
MSH_CMD_EXPORT_ALIAS(avrcp_get_info, avrcp_info, "AVRCP: get current song info");
MSH_CMD_EXPORT_ALIAS(avrcp_get_status, avrcp_status, "AVRCP: get playback status");
