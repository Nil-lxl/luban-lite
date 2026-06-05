/*
 * Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief RTL8733BS HCI extension driver.
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/drivers/bt_uart.h>

#define LOG_LEVEL BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rtl8733_driver);

#include "fw_rtl8733bs_d7b8_0da7.h"
#include "../hci_init.h"

/* BT settling time after power on */
#define BT_POWER_ON_SETTLING_TIME_MS      (500u)
#define BT_POWER_CBUCK_DISCHARGE_TIME_MS  (300u)

/* Stabilization delay after FW loading */
#define BT_STABILIZATION_DELAY_MS         (250u)

/* HCI Command packet from Host to Controller */
#define HCI_COMMAND_PACKET                (0x01)

/* Length of UPDATE BAUD RATE command */
#define HCI_VSC_UPDATE_BAUD_RATE_LENGTH   (6u)

/* Default BAUDRATE */
#define HCI_UART_DEFAULT_BAUDRATE         (115200)

#define RTL_CONFIG_MAGIC     0x8723ab55
#define RTL_FRAG_LEN 252

#define get_unaligned(p)                                        \
({                                                              \
    struct packed_dummy_struct {                                \
        typeof(*(p)) __val;                                     \
    } __attribute__((packed)) *__ptr = (void *) (p);            \
                                                                \
    __ptr->__val;                                               \
})
#define get_unaligned_le16(p)   le16_to_cpu(get_unaligned((u16 *)(p)))
#define get_unaligned_le32(p)   le32_to_cpu(get_unaligned((u32 *)(p)))

/* Externs for CY43xxx controller FW */
extern const uint8_t brcm_patchram_buf[];
extern const int brcm_patch_ram_length;

enum {
	BT_HCI_VND_OP_DOWNLOAD_PATCH            = 0xFC20,
	BT_HCI_VND_OP_UPDATE_BAUDRATE           = 0xFC17,
};

struct bt_dowmload_cmd {
    uint8_t index;
    uint8_t data[RTL_FRAG_LEN];
};

struct bt_dowmload_response {
    uint8_t index;
};
struct bt_vendor_config_entry {
    u16 offset;
    u8 len;
    u8 data[];
};

struct bt_vendor_config {
    u32 signature;
    u16 total_len;
    struct bt_vendor_config_entry entry[];
};

struct bt_custom_config {
    uint32_t baudrate;
    uint32_t baudrate_data;
    uint32_t flow_control;
};

static struct bt_custom_config custom_config = { 0 };

/*  bt_h4_vnd_setup function.
 * This function executes vendor-specific commands sequence to
 * initialize BT Controller before BT Host executes Reset sequence.
 * bt_h4_vnd_setup function must be implemented in vendor-specific HCI
 * extansion module if BT_HCI_SETUP is enabled.
 */
int bt_h4_vnd_setup(struct device *dev);

static int bt_hci_uart_set_baudrate(const struct rt_device *uart, uint32_t baudrate)
{
    return bt_uart_baudrate_update(baudrate);
}

static int bt_hci_uart_set_flow_control(const struct rt_device *uart, uint32_t flow_control)
{
    return bt_uart_flow_control_update(flow_control);
}

static uint32_t bt_convert_baudrate(uint32_t baudrate_data)
{
	switch (baudrate_data) {
	    case 0x0252a00a:
	    	return 230400;
	    case 0x05f75004:
	    	return 921600;
	    case 0x00005004:
	    	return 1000000;
	    case 0x04928002:
	    case 0x01128002:
	    	return 1500000;
	    case 0x00005002:
	    	return 2000000;
	    case 0x0000b001:
	    	return 2500000;
	    case 0x04928001:
	    	return 3000000;
	    case 0x052a6001:
	    	return 3500000;
	    case 0x036d5001:
	    	return 3750000;
	    case 0x00005001:
	    	return 4000000;
	    case 0x0252c014:
	    default:
	    	return 115200;
	}
}

static int bt_get_custom_config(struct bt_custom_config *custom_config)
{
	struct bt_vendor_config *config;
	struct bt_vendor_config_entry *entry;
    int i, total_data_len;
	bool found = false;

    total_data_len = sizeof(config_rtl8733bs) - 6;
	if (total_data_len <= 0) {
		pr_err("no config loaded\n");
		return -EINVAL;
	}

	config = (struct bt_vendor_config *)config_rtl8733bs;
    if (le32_to_cpu(config->signature) != RTL_CONFIG_MAGIC) {
		pr_err("invalid config magic\n");
		return -EINVAL;
	}

	if (total_data_len < le16_to_cpu(config->total_len)) {
		pr_err("config is too short\n");
		return -EINVAL;
	}

	for (i = 0; i < total_data_len; ) {
		entry = ((void *)config->entry) + i;

		switch (le16_to_cpu(entry->offset)) {
		case 0xc:
			if (entry->len < sizeof(uint32_t)) {
				pr_err("invalid UART config entry\n");
				return -EINVAL;
			}

			custom_config->baudrate_data = get_unaligned_le32(entry->data);
            custom_config->baudrate = bt_convert_baudrate(custom_config->baudrate_data);

            if (entry->len >= 13)
				custom_config->flow_control = !!(entry->data[12] & BIT(2));
			else
				custom_config->flow_control = false;

			found = true;
			break;

		default:
            pr_debug("skipping config entry 0x%x (len %u)\n", le16_to_cpu(entry->offset), entry->len);
            break;
		}

		i += sizeof(*entry) + entry->len;
	}

	if (!found) {
		pr_err("no UART config entry found\n");
		return -ENOENT;
	}

	pr_warn("baudrate = %u\n", custom_config->baudrate);
	pr_warn("baudrate date = 0x%08x\n", custom_config->baudrate_data);
	pr_warn("flow control %d\n", custom_config->flow_control);

	return 0;
}

static int bt_update_controller_baudrate(const struct rt_device *uart,
                                         struct bt_custom_config *custom_config)
{
	struct net_buf *buf;
	int err;

	/* Allocate buffer for update uart baudrate command.
	 * It will be BT_HCI_OP_RESET with extra parameters.
	 */
	buf = bt_hci_cmd_alloc(K_FOREVER);
	if (buf == NULL) {
		LOG_ERR("Unable to allocate command buffer");
		return -ENOMEM;
	}

	/* Add data part of packet */
	net_buf_add_mem(buf, &custom_config->baudrate_data, sizeof(uint32_t));

	/* Send update uart baudrate command. */
	err = bt_hci_cmd_send_sync(BT_HCI_VND_OP_UPDATE_BAUDRATE, buf, NULL);
	if (err) {
		return err;
	}

	return 0;
}

static int bt_firmware_download(const uint8_t *firmware,
                                uint32_t firmware_len,
                                const uint8_t *config,
                                uint32_t config_len)
{
    struct bt_dowmload_cmd dl_cmd;
    struct bt_dowmload_response dl_rsp;
    struct net_buf *rsp;
    struct net_buf *buf;
	uint8_t *p, *data = NULL;
    uint32_t dlen = 0;
    int frag_num = 0, frag_len = RTL_FRAG_LEN;
	int i, ret = -1;

	LOG_DBG("Executing Fw downloading for RTL8733BS device");

    dlen = firmware_len + config_len;
    data = malloc(dlen);
    if (!data) {
        pr_err("malloc data failed.\n");
        return ret;
    }

    memcpy(data, firmware, firmware_len);
    memcpy(data + firmware_len, config, config_len);

    frag_num = dlen / RTL_FRAG_LEN + 1;

    p = data;
    for (i = 0; i < frag_num; i++) {
        LOG_DBG("download fw (%d/%d)\n", i, frag_num);

		buf = bt_hci_cmd_alloc(K_FOREVER);
		if (buf == NULL) {
			LOG_ERR("Unable to allocate command buffer");
			goto out;
		}

        if (i > 0x7f)
            dl_cmd.index = (i & 0x7f) + 1;
        else
            dl_cmd.index = i;

        if (i == (frag_num - 1)) {
            dl_cmd.index |= 0x80; /* data end */
            frag_len = dlen % RTL_FRAG_LEN;
        }
        memcpy(dl_cmd.data, p, frag_len);

		/* Add data part of packet */
		net_buf_add_mem(buf, &dl_cmd, frag_len + 1);

        /* Send download command */
		ret = bt_hci_cmd_send_sync(BT_HCI_VND_OP_DOWNLOAD_PATCH, buf, &rsp);
        if (ret) {
            pr_err("download fw command failed (%d)\n", ret);
            goto out;
        }

        p += RTL_FRAG_LEN;

        //dl_rsp = rsp->data;

	    net_buf_unref(rsp);
    }

	LOG_DBG("Fw downloading complete");

out:
    if (data)
        free(data);

	return ret;
}

int bt_h5_vnd_update_controller_baudrate(struct rt_device *uart)
{
	uint32_t default_uart_speed = 115200;
    int err = 0;

    err = bt_get_custom_config(&custom_config);
    if (err) {
		return err;
    }

	if (custom_config.baudrate != default_uart_speed) {
		err = bt_update_controller_baudrate(uart, &custom_config);
		if (err) {
			return err;
		}
	}

    return err;
}

int bt_h5_vnd_update_host_baudrate(struct rt_device *uart)
{
	uint32_t default_uart_speed = 115200;
    int err = 0;

    err = bt_get_custom_config(&custom_config);
    if (err) {
		return err;
    }

	if (custom_config.baudrate != default_uart_speed) {
	    err = bt_hci_uart_set_baudrate(uart, custom_config.baudrate);
	    if (err) {
	    	return err;
	    }
    }

    return err;
}

int bt_h5_vnd_update_host_flowcontrol(struct rt_device *uart)
{
    int err = 0;

    err = bt_get_custom_config(&custom_config);
    if (err) {
		return err;
    }

    err = bt_hci_uart_set_flow_control(uart, custom_config.flow_control);
	if (err) {
		return err;
	}

    return err;
}

int bt_h5_vnd_update_controller_firmware(struct rt_device *uart)
{
    int err = 0;

	err = bt_firmware_download(fw_rtl8733bs_d7b8_0da7,
                               sizeof(fw_rtl8733bs_d7b8_0da7),
                               config_rtl8733bs,
                               sizeof(config_rtl8733bs));
	if (err) {
		return err;
	}

    return err;
}

int bt_h5_vnd_setup(struct rt_device *uart)
{
	int err;
	uint32_t default_uart_speed = 115200;

    bt_ctlr_init();

	/* Set host controller functionality to user defined baudrate
	 * after fw downloading.
	 */
	if (custom_config.baudrate != default_uart_speed) {
		err = bt_update_controller_baudrate(uart, &custom_config);
		if (err) {
			return err;
		}
	}

    err = bt_hci_uart_set_flow_control(uart, custom_config.flow_control);
	if (err) {
		return err;
	}

	k_msleep(300);

	/* BT firmware download */
	err = bt_firmware_download(fw_rtl8733bs_d7b8_0da7,
                               sizeof(fw_rtl8733bs_d7b8_0da7),
                               config_rtl8733bs,
                               sizeof(config_rtl8733bs));
	if (err) {
		return err;
	}

	/* Stabilization delay */
	(void)k_msleep(BT_STABILIZATION_DELAY_MS);

	/* Send HCI_RESET */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_RESET, NULL, NULL);
	if (err) {
		return err;
	}

	return 0;
}
