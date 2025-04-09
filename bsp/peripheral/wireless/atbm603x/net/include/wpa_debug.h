/**************************************************************************************************************
* altobeam IOT Wi-Fi
*
* Copyright (c) 2018, altobeam.inc   All rights reserved.
*
* The source code contains proprietary information of AltoBeam, and shall not be distributed, 
* copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/

#ifndef __WPS_DEBUG_H__
#define __WPS_DEBUG_H__

//#include "debug.h"

enum {
	MSG_EXCESSIVE, MSG_MSGDUMP, MSG_DEBUG, MSG_INFO, MSG_WARNING, MSG_ERROR, MSG_ALWAYS
};

#define wpa_debug_timestamp 1
#define wpa_debug_level 	MSG_INFO

#define wpa_printf(_level,...)	do {if((_level >= wpa_debug_level)) {iot_printf(__VA_ARGS__);iot_printf("\n");}} while (0)


atbm_void wpa_hexdump(int level, const char *title, const atbm_uint8 *buf, atbm_size_t len);
atbm_void wpa_hexdump_ascii(int level, const char *title, const atbm_uint8 *buf, atbm_size_t len);
atbm_void wpa_hexdump_key(int level, const char *title, const void *buf, atbm_size_t len);
atbm_void wpa_hexdump_ascii_key(int level, const char *title, const void *buf,
			   atbm_size_t len);

static inline void wpa_hexdump_buf(int level, const char *title,
				   const struct wpabuf *buf)
{
	wpa_hexdump(level, title, buf ? wpabuf_head(buf) : NULL,
		    buf ? wpabuf_len(buf) : 0);
}

static inline void wpa_hexdump_buf_key(int level, const char *title,
					  const struct wpabuf *buf)
{
   wpa_hexdump_key(level, title, buf ? wpabuf_head(buf) : NULL,
		   buf ? wpabuf_len(buf) : 0);
}


#endif

