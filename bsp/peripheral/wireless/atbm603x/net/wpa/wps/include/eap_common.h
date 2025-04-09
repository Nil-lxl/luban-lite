/*
 * EAP common peer/server definitions
 * Copyright (c) 2004-2014, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef EAP_COMMON_H
#define EAP_COMMON_H

#include "wpabuf.h"
#include "wpa_main.h"

struct erp_tlvs {
	const atbm_uint8 *keyname;
	const atbm_uint8 *domain;

	atbm_uint8 keyname_len;
	atbm_uint8 domain_len;
};

atbm_int32 eap_hdr_len_valid(const struct wpabuf *msg, atbm_size_t min_payload);
const atbm_uint8 * eap_hdr_validate(EapVenType vendor, EapType eap_type,
			    const struct wpabuf *msg, atbm_size_t *plen);
struct wpabuf * eap_msg_alloc(EapVenType vendor, EapType type, atbm_uint32 payload_len,
			      EapCodeType code, atbm_uint8 identifier);
void eap_update_len(struct wpabuf *msg);
atbm_uint8 eap_get_id(const struct wpabuf *msg);
EapType eap_get_type(const struct wpabuf *msg);
int erp_parse_tlvs(const atbm_uint8 *pos, const atbm_uint8 *end, struct erp_tlvs *tlvs,
		   int stop_at_keyname);
int atbmwifi_wpa_snprintf_hex(char *buf, atbm_size_t buf_size, const atbm_uint8 *data, atbm_size_t len);
 struct wpabuf * eap_wsc_build_start(atbm_uint8 id);

#endif /* EAP_COMMON_H */
