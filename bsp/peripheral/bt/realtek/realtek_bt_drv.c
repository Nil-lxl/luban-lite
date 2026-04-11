/*
 * Copyright (C) 2015-2026 Endless Mobile, Inc.
 *
 * Author: Thomas_li <thomas_li@realsil.com.cn>
 */

#include <stdio.h>
#include <aic_core.h>
#include <nimble/host/src/ble_hs_hci_priv.h>
#include <nimble/host/src/ble_hs_priv.h>
#include <realtek_bt_drv.h>

#include "inc/fw_rtl8733bs_d7b8_0da7.h"
#include "inc/config_rtl8733bs.h"

static const struct realtek_bt_module module_table[] = {
	/* 8723BS */
	{
      .flags = RTL_FLAG_LMPSUBV | RTL_FLAG_HCIREV | RTL_FLAG_HCIVER | RTL_FLAG_HCIBUS,
	  .bus = HCI_UART,
	  .hci_ver = 0x6,
	  .hci_rev = 0xb,
	  .lmp_subver = RTL_ROM_LMP_8723B,
	  .config_needed = true,
	  .has_rom_version = true,
    },

	/* 8733BS BT5.2 */
	{
      .flags = RTL_FLAG_LMPSUBV | RTL_FLAG_HCIREV | RTL_FLAG_HCIVER | RTL_FLAG_HCIBUS,
	  .bus = HCI_UART,
	  .hci_ver = 0xb,
	  .hci_rev = 0xf,
	  .lmp_subver = RTL_ROM_LMP_8723B,
	  .config_needed = true,
	  .has_rom_version = true,
      .fw = {
       .data = fw_rtl8733bs_d7b8_0da7,
       .len = sizeof(fw_rtl8733bs_d7b8_0da7)
      },
      .cfg = {
       .data = config_rtl8733bs,
       .len = sizeof(config_rtl8733bs)
      }
    },

	/* 8733BS BT4.2 */
	{
      .flags = RTL_FLAG_LMPSUBV | RTL_FLAG_HCIREV | RTL_FLAG_HCIVER | RTL_FLAG_HCIBUS,
	  .bus = HCI_UART,
	  .hci_ver = 0x8,
	  .hci_rev = 0xf,
	  .lmp_subver = RTL_ROM_LMP_8723B,
	  .config_needed = true,
	  .has_rom_version = true,
      .fw = {
       .data = fw_rtl8733bs_d7b8_0da7,
       .len = sizeof(fw_rtl8733bs_d7b8_0da7)
      },
      .cfg = {
       .data = config_rtl8733bs,
       .len = sizeof(config_rtl8733bs)
      }
    },

	/* 8723B */
	{
      .flags = RTL_FLAG_LMPSUBV | RTL_FLAG_HCIREV,
      .hci_rev = 0xb,
      .lmp_subver = RTL_ROM_LMP_8723B,
	  .config_needed = false,
	  .has_rom_version = true,
    },

	/* 8723D */
	{
      .flags = RTL_FLAG_LMPSUBV | RTL_FLAG_HCIREV,
      .hci_rev = 0xd,
      .lmp_subver = RTL_ROM_LMP_8723B,
	  .config_needed = true,
	  .has_rom_version = true,
    },

	/* 8723DS */
	{
      .flags = RTL_FLAG_LMPSUBV | RTL_FLAG_HCIREV | RTL_FLAG_HCIVER | RTL_FLAG_HCIBUS,
	  .bus = HCI_UART,
	  .hci_ver = 0x8,
	  .hci_rev = 0xd,
	  .lmp_subver = RTL_ROM_LMP_8723B,
	  .config_needed = true,
	  .has_rom_version = true,
    },

	/* 8723DU */
	{
      .flags = RTL_FLAG_LMPSUBV | RTL_FLAG_HCIREV,
      .hci_rev = 0x826C,
      .lmp_subver = RTL_ROM_LMP_8723D,
	  .config_needed = true,
	  .has_rom_version = true,
    },
};

static struct realtek_bt_module *realtek_bt_get_module(u8 bus)
{
    struct ble_hci_ip_rd_local_ver_rp rsp;
    u8 hci_ver, lmp_ver __attribute__((unused));
    u16 hci_rev, lmp_subver;
    int ret = 0;
    int i;

    ret = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_INFO_PARAMS,
                            BLE_HCI_OCF_IP_RD_LOCAL_VER), NULL,
                            0, &rsp, sizeof(rsp));
    if (ret) {
        pr_err("hci op read local version failed (%d)\n", ret);
        return NULL;
    }

    hci_ver = rsp.hci_ver;
    lmp_ver = rsp.lmp_ver;
    hci_rev = le16_to_cpu(rsp.hci_rev);
    lmp_subver = le16_to_cpu(rsp.lmp_subver);

    pr_debug("hci_ver=%02x hci_rev=%04x lmp_ver=%02x lmp_subver=%04x\n",
             hci_ver, hci_rev, lmp_ver, lmp_subver);

    for (i = 0; i < ARRAY_SIZE(module_table); i++) {
        if ((module_table[i].flags & RTL_FLAG_LMPSUBV) &&
            (module_table[i].lmp_subver != lmp_subver))
            continue;
        if ((module_table[i].flags & RTL_FLAG_HCIREV) &&
            (module_table[i].hci_rev != hci_rev))
            continue;
        if ((module_table[i].flags & RTL_FLAG_HCIVER) &&
            (module_table[i].hci_ver != hci_ver))
            continue;
        if ((module_table[i].flags & RTL_FLAG_HCIBUS) &&
            (module_table[i].bus != bus))
            continue;

        break;
    }

    if (i >= ARRAY_SIZE(module_table))
        return NULL;

    return (struct realtek_bt_module *)&module_table[i];
}

static int realtek_bt_read_local_version(void)
{
    struct ble_hci_ip_rd_local_ver_rp rsp;
    int ret = 0;

    memset(&rsp, 0, sizeof(rsp));
    ret = ble_hs_hci_cmd_tx(HCI_OP_READ_LOCAL_VERSION, NULL, 0, &rsp, sizeof(rsp));
    if (ret) {
        pr_err("HCI_OP_READ_LOCAL_VERSION failed (%d)\n", ret);
        return ret;
    }

    pr_info("examining hci_ver=%02x hci_rev=%04x lmp_ver=%02x lmp_subver=%04x\n",
            rsp.hci_ver, rsp.hci_rev, rsp.lmp_ver, rsp.lmp_subver);
    pr_info("fw version 0x%04x%04x\n", rsp.hci_rev, rsp.lmp_subver);

    return ret;
}

static int realtek_bt_read_rom_version(u8 *version)
{
	struct realtek_bt_rom_version_evt rom_version;
	int ret = 0;

	/* Read realtek ROM version command */
    ret = ble_hs_hci_cmd_tx(0xfc6d, NULL, 0, &rom_version, sizeof(rom_version));
    if (ret) {
		pr_err("Read ROM version failed (%d)\n", ret);
		return ret;
    }

    pr_info("rom version=%x\n", rom_version.version);

    *version = rom_version.version;

	return 0;
}

static int realtek_bt_download_firmware(struct realtek_bt_patch *fw)
{
    struct realtek_bt_dowmload_cmd *dl_cmd;
    struct realtek_bt_dowmload_response dl_rsp;
    unsigned char *data = fw->data;
    int frag_num = fw->len / RTL_FRAG_LEN + 1;
    int frag_len = RTL_FRAG_LEN;
    int ret = 0;
    int i;

    dl_cmd = malloc(sizeof(struct realtek_bt_dowmload_cmd));
    if (!dl_cmd)
        return -ENOMEM;

    for (i = 0; i < frag_num; i++) {
        pr_info("download fw (%d/%d)\n", i, frag_num);
        if (i > 0x7f)
            dl_cmd->index = (i & 0x7f) + 1;
        else
            dl_cmd->index = i;

        if (i == (frag_num - 1)) {
            dl_cmd->index |= 0x80; /* data end */
            frag_len = fw->len % RTL_FRAG_LEN;
        }
        memcpy(dl_cmd->data, data, frag_len);

        /* Send download command */
        ret = ble_hs_hci_cmd_tx(0xfc20, dl_cmd, frag_len + 1, &dl_rsp, sizeof(dl_rsp));
        if (ret) {
            pr_err("download fw command failed (%d)\n", ret);
            goto out;
        }

        data += RTL_FRAG_LEN;
    }

    ret = realtek_bt_read_local_version();
    if (ret) {
        pr_err("read local version failed\n");
        goto out;
    }

out:
    free(dl_cmd);
    return ret;
}

static int realtek_bt_module_init(struct realtek_bt_device *btdev)
{
    int ret;

    btdev->module = realtek_bt_get_module(HCI_UART);
    if (!btdev->module) {
        pr_warn("The firmware is up to date.\n");
        return -1;
    }

    if (btdev->module->has_rom_version) {
        ret = realtek_bt_read_rom_version(&btdev->rom_version);
        if (ret) {
            pr_err("read rom version failed (%d)\n", ret);
            return -1;
        }
    }

    if (btdev->module->fw.len <= 0) {
        pr_err("firmware file %s not found\n", btdev->module->fw.name);
        return -1;
    }

    if (btdev->module->config_needed) {
        if (btdev->module->cfg.len <= 0) {
            pr_err("config file %s not found\n", btdev->module->cfg.name);
            return -1;
        }
    }

    return 0;
}

static unsigned int realtek_bt_convert_baudrate(u32 device_baudrate)
{
	switch (device_baudrate) {
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

	case 0x00005001:
		return 4000000;

	case 0x0252c014:
	default:
		return 115200;
	}
}

static int realtek_bt_get_uart_settings(struct realtek_bt_device *btdev,
                                        unsigned int *baudrate,
                                        u32 *device_baudrate, bool *flow_control)
{
	struct realtek_bt_vendor_config *config;
	struct realtek_bt_vendor_config_entry *entry;
	int i, total_data_len;
	bool found = false;

	total_data_len = btdev->module->cfg.len - 6;
	if (total_data_len <= 0) {
		pr_warn("no config loaded\n");
		return -EINVAL;
	}

	config = (struct realtek_bt_vendor_config *)btdev->module->cfg.data;
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
			if (entry->len < sizeof(*device_baudrate)) {
				pr_err("invalid UART config entry\n");
				return -EINVAL;
			}

			*device_baudrate = get_unaligned_le32(entry->data);
            *baudrate = realtek_bt_convert_baudrate(*device_baudrate);

            if (entry->len >= 13)
				*flow_control = !!(entry->data[12] & BIT(2));
			else
				*flow_control = false;

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

	pr_debug("device baudrate = 0x%08x\n", *device_baudrate);
	pr_debug("controller baudrate = %u\n", *baudrate);
	pr_debug("flow control %d\n", *flow_control);

	return 0;
}

void realtek_bt_init(void)
{
	struct realtek_bt_device btdev;
    u32 device_baudrate, baudrate;
    bool flow_control;
	int ret = 0;

	ret = realtek_bt_module_init(&btdev);
	if (ret) {
        pr_err("realtek bluetooth initialize failed.\n");
        return;
    }

    ret = realtek_bt_get_uart_settings(&btdev, &baudrate, &device_baudrate, &flow_control);
    if (ret) {
        pr_err("realtek bluetooth get uart settings failed.\n");
        return;
    }

    //ret = ble_hs_hci_cmd_tx(0xfc17, &device_baudrate, sizeof(device_baudrate), NULL, 0);
    //if (ret) {
    //    pr_err("set baud rate command failed(%d)\n", ret);
    //    return;
    //}

    /* Give the device some time to set up the new baudrate. */
    aicos_udelay(10000);

    //serdev_device_set_baudrate(h5->hu->serdev, baudrate);
    //serdev_device_set_flow_control(h5->hu->serdev, flow_control);

    ret = realtek_bt_download_firmware(&btdev.module->fw);
    if (ret) {
        pr_err("realtek bluetooth download firmware failed.\n");
        return;
    }
}

int realtek_bt_shutdown(void)
{
	int ret = 0;

	/* According to the vendor driver, BT must be reset on close to avoid
	 * firmware crash.
	 */
    ret = ble_hs_hci_cmd_tx(HCI_OP_RESET, NULL, 0, NULL, 0);
    if (ret) {
        pr_err("HCI reset during shutdown failed(%d)\n", ret);
        return ret;
    }

	return 0;
}
