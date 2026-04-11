/*
 * EAP method registration
 * Copyright (c) 2004-2009, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "atbm_hal.h"

/**
 * eap_server_register_methods - Register statically linked EAP server methods
 * Returns: 0 on success, -1 or -2 on failure
 *
 * This function is called at program initialization to register all EAP
 * methods that were linked in statically.
 */
 int eap_server_register_methods(void)
{
	int ret = 0;

	return ret;
}
