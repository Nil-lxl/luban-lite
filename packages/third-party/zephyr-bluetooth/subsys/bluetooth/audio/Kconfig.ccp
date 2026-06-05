# Bluetooth Audio - Call Control Profile (CCP) configuration options
#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

if LPKG_BT_AUDIO

config LPKG_BT_CCP_CALL_CONTROL_CLIENT
	bool "Call Control Profile Client Support"
	depends on LPKG_BT_EXT_ADV
	depends on LPKG_BT_TBS_CLIENT
	depends on LPKG_BT_BONDABLE
	help
	  This option enables support for the Call Control Profile Client which uses the Telephone
	  Bearer Service (TBS) client to control calls on a remote device.

if LPKG_BT_CCP_CALL_CONTROL_CLIENT

config LPKG_BT_CCP_CALL_CONTROL_CLIENT_BEARER_COUNT
	int "Telephone bearer count"
	default 1
	range 1 255 if LPKG_BT_TBS_CLIENT_TBS
	range 1 1
	help
	  The number of supported telephone bearers on the CCP Call Control Client

config LPKG_BT_CCP_CALL_CONTROL_CLIENT_CB_USER_DATA
	bool "Call Control Profile Client support for user_data in callbacks"
	help
	  This option enables support for user_data in Call Control Profile Client callbacks.

#module = LPKG_BT_CCP_CALL_CONTROL_CLIENT
#module-str = "Call Control Profile Client"
#source "subsys/logging/Kconfig.template.log_config"

endif # LPKG_BT_CCP_CALL_CONTROL_CLIENT

config LPKG_BT_CCP_CALL_CONTROL_SERVER
	bool "Call Control Profile Call Control Server Support"
	depends on LPKG_BT_EXT_ADV
	depends on LPKG_BT_TBS
	depends on LPKG_BT_BONDABLE
	help
	  This option enables support for the Call Control Profile Call Control Server which uses
	  the Telephone Bearer Service (TBS) to hold and control calls on a device.

if LPKG_BT_CCP_CALL_CONTROL_SERVER

config LPKG_BT_CCP_CALL_CONTROL_SERVER_BEARER_COUNT
	int "Telephone bearer count"
	default 1
	range 1 255
	help
	  The number of supported telephone bearers on the CCP Call Control Server

config LPKG_BT_CCP_CALL_CONTROL_SERVER_PROVIDER_NAME_MAX_LENGTH
	int "The maximum length of the bearer provider name excluding null terminator"
	default LPKG_BT_TBS_MAX_PROVIDER_NAME_LENGTH
	range 1 LPKG_BT_TBS_MAX_PROVIDER_NAME_LENGTH
	help
	  Sets the maximum length of the bearer provider name.

#module = LPKG_BT_CCP_CALL_CONTROL_SERVER
#module-str = "Call Control Profile Call Control Server"
#source "subsys/logging/Kconfig.template.log_config"

endif # LPKG_BT_CCP_CALL_CONTROL_SERVER

endif # LPKG_BT_AUDIO
