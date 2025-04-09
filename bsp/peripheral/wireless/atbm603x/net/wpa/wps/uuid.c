/*
 * Universally Unique IDentifier (UUID)
 * Copyright (c) 2008, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "atbm_hal.h"
#include "uuid.h"

 int uuid_str2bin(const char *str, atbm_uint8 *bin)
{
	const char *pos;
	atbm_uint8 *opos;

	pos = str;
	opos = bin;

	if (atbmwifi_hexstr2bin(opos, pos, 4))
		return -1;
	pos += 8;
	opos += 4;

	if (*pos++ != '-' || atbmwifi_hexstr2bin(opos,pos, 2))
		return -1;
	pos += 4;
	opos += 2;

	if (*pos++ != '-' || atbmwifi_hexstr2bin(opos, pos,  2))
		return -1;
	pos += 4;
	opos += 2;

	if (*pos++ != '-' || atbmwifi_hexstr2bin(opos, pos, 2))
		return -1;
	pos += 4;
	opos += 2;

	if (*pos++ != '-' || atbmwifi_hexstr2bin(opos, pos, 6))
		return -1;

	return 0;
}


 int uuid_bin2str(const atbm_uint8 *bin, char *str, atbm_size_t max_len)
{
	int len;
	len = sprintf(str,"%02x%02x%02x%02x-%02x%02x-%02x%02x-"
			  "%02x%02x-%02x%02x%02x%02x%02x%02x",
			  bin[0], bin[1], bin[2], bin[3],
			  bin[4], bin[5], bin[6], bin[7],
			  bin[8], bin[9], bin[10], bin[11],
			  bin[12], bin[13], bin[14], bin[15]);
	//if (os_snprintf_error(max_len, len))
	//	return -1;
	return 0;
}


 int is_nil_uuid(const atbm_uint8 *uuid)
{
	int i;
	for (i = 0; i < UUID_LEN; i++)
		if (uuid[i])
			return 0;
	return 1;
}
