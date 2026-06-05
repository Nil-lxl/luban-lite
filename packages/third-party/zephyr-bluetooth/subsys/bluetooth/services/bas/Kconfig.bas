# Bluetooth GATT Battery service

# Copyright (c) 2018 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

config LPKG_BT_BAS
	bool "GATT Battery service"

config LPKG_BT_BAS_BLS
	bool "Battery Level Status"
	help
	  Enable this option to include Battery Level Status Characteristic.

if LPKG_BT_BAS_BLS

config LPKG_BT_BAS_BLS_IDENTIFIER_PRESENT
	bool "Battery Level Identifier Present"
	help
	  Enable this option if the Battery Level Identifier is present.

config LPKG_BT_BAS_BLS_BATTERY_LEVEL_PRESENT
	bool "Battery Level Present"
	help
	  Enable this option if the Battery Level is present.

config LPKG_BT_BAS_BLS_ADDITIONAL_STATUS_PRESENT
	bool "Additional Battery Status Present"
	help
	  Enable this option if Additional Battery Status information is present.

config LPKG_BT_BAS_BCS
	bool "Battery Critical Status"
	help
	  Enable this option to include Battery Critical Status Characteristic.
endif
