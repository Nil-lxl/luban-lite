# Bluetooth GATT Battery service

# Copyright (c) 2024 Croxel Inc.
# SPDX-License-Identifier: Apache-2.0

config LPKG_BT_CTS
	bool "GATT Current Time service"

if LPKG_BT_CTS

config LPKG_BT_CTS_HELPER_API
	bool "Helper APIs to encode and decode CTS formatted time"

endif
