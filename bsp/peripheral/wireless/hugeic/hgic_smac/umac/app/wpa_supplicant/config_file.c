/*
 * WPA Supplicant / Configuration backend: text file
 * Copyright (c) 2003-2019, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 *
 * This file implements a configuration backend for text files. All the
 * configuration information is stored in a text file that uses a format
 * described in the sample configuration file, wpa_supplicant.conf.
 */

#include "utils/includes.h"
#ifdef ANDROID
#include <sys/stat.h>
#endif /* ANDROID */

#include "utils/common.h"
#include "config.h"
#include "utils/base64.h"
#include "utils/uuid.h"
#include "common/ieee802_1x_defs.h"
#include "p2p/p2p.h"
#include "eap_peer/eap_methods.h"
#include "eap_peer/eap.h"
//#include "utils/config.h"

#if 1 /*porting*/
#define CONFIG_BUFF_SIZE (4096)
static  char *wpa_config_buff;
static int    config_read_off = 0;
int ieee80211ah_enabled = 0;

int umac_config_read(const char *name, char *buff, int size);
int umac_config_write(const char *name, char *buff, int size);

static char *hgvsprintf(char *buff, char *fmt, ...)
{
    size_t size  = wpa_config_buff+CONFIG_BUFF_SIZE-buff-1;
	va_list ap;

    ASSERT(wpa_config_buff);    
    if(size > 0){
    	va_start(ap, fmt);
        size = vsnprintf(buff, size, fmt, ap);
    	va_end(ap);
    }else{
        //PRINTF("config buff overflow!\r\n");
    }
    
    return (size>0)?(buff+size):buff;
}

static int wpas_gets(char *s, int size)
{
    int cnt = 0;
    while(wpa_config_buff && wpa_config_buff[config_read_off] && cnt < size){
        *s = wpa_config_buff[config_read_off++];
        cnt++;
        if(*s == '\n'){
            *s = 0;
            break;
        }
        s++;
    }
    if(cnt == size){
        PRINTF("WARING: IVALID CONFIG DATA\r\n");
    }
	return cnt;
}

static char * wpa_config_get_line(char *s, int size, int *line,
			   char **_pos)
{
	char *pos, *end, *sstart;

    *s = 0;
	while (wpas_gets(s, size)) {
		(*line)++;
		s[size - 1] = '\0';
		pos = s;

		/* Skip white space from the beginning of line. */
		while (*pos == ' ' || *pos == '\t' || *pos == '\r')
			pos++;

		/* Skip comment lines and empty lines */
		if (*pos == '#' || *pos == '\n' || *pos == '\0')
			continue;

		/*
		 * Remove # comments unless they are within a double quoted
		 * string.
		 */
		sstart = os_strchr(pos, '"');
		if (sstart)
			sstart = os_strrchr(sstart + 1, '"');
		if (!sstart)
			sstart = pos;
		end = os_strchr(sstart, '#');
		if (end)
			*end-- = '\0';
		else
			end = pos + os_strlen(pos) - 1;

		/* Remove trailing white space. */
		while (end > pos &&
		       (*end == '\n' || *end == ' ' || *end == '\t' ||
			*end == '\r'))
			*end-- = '\0';

		if (*pos == '\0')
			continue;

		if (_pos)
			*_pos = pos;
		return pos;
	}

	if (_pos)
		*_pos = NULL;
	return NULL;
}
#endif

static int wpa_config_validate_network(struct wpa_ssid *ssid, int line)
{
	int errors = 0;

	if (ssid->passphrase) {
		if (ssid->psk_set) {
			wpa_printf(MSG_ERROR, "Line %d: both PSK and "
				   "passphrase configured.", line);
			errors++;
		}
		wpa_config_update_psk(ssid);
	}

	if (ssid->disabled == 2)
		ssid->p2p_persistent_group = 1;

	if ((ssid->group_cipher & WPA_CIPHER_CCMP) &&
	    !(ssid->pairwise_cipher & (WPA_CIPHER_CCMP | WPA_CIPHER_CCMP_256 |
				       WPA_CIPHER_GCMP | WPA_CIPHER_GCMP_256 |
				       WPA_CIPHER_NONE))) {
		/* Group cipher cannot be stronger than the pairwise cipher. */
		wpa_printf(MSG_DEBUG, "Line %d: removed CCMP from group cipher"
			   " list since it was not allowed for pairwise "
			   "cipher", line);
		ssid->group_cipher &= ~WPA_CIPHER_CCMP;
	}

	if (ssid->mode == WPAS_MODE_MESH &&
	    (ssid->key_mgmt != WPA_KEY_MGMT_NONE &&
	    ssid->key_mgmt != WPA_KEY_MGMT_SAE)) {
		wpa_printf(MSG_ERROR,
			   "Line %d: key_mgmt for mesh network should be open or SAE",
			   line);
		errors++;
	}

#ifdef CONFIG_OCV
	if (ssid->ocv && ssid->ieee80211w == NO_MGMT_FRAME_PROTECTION) {
		wpa_printf(MSG_ERROR,
			   "Line %d: PMF needs to be enabled whenever using OCV",
			   line);
		errors++;
	}
#endif /* CONFIG_OCV */

	return errors;
}


static struct wpa_ssid * wpa_config_read_network(int *line, int id) /*porting*/
{
	struct wpa_ssid *ssid;
	int errors = 0, end = 0;
	char *buf, *pos, *pos2;

	wpa_printf(MSG_MSGDUMP, "Line: %d - start of a new network block",
		   *line);
	ssid = os_zalloc(sizeof(*ssid));
	if (ssid == NULL)
		return NULL;
/*porting*/
    buf = os_malloc(2000); 
	if(buf == NULL){
		os_free(ssid);
		return NULL;
	}
    
	dl_list_init(&ssid->psk_list);
	ssid->id = id;

	wpa_config_set_network_defaults(ssid);

	while (wpa_config_get_line(buf, 2000, line, &pos)) { /*porting*/
		if (os_strcmp(pos, "}") == 0) {
			end = 1;
			break;
		}

		pos2 = os_strchr(pos, '=');
		if (pos2 == NULL) {
			wpa_printf(MSG_ERROR, "Line %d: Invalid SSID line "
				   "'%s'.", *line, pos);
			errors++;
			continue;
		}

		*pos2++ = '\0';
		if (*pos2 == '"') {
			if (os_strchr(pos2 + 1, '"') == NULL) {
				wpa_printf(MSG_ERROR, "Line %d: invalid "
					   "quotation '%s'.", *line, pos2);
				errors++;
				continue;
			}
		}

		if (wpa_config_set(ssid, pos, pos2, *line) < 0) {
#ifndef CONFIG_WEP
			if (os_strcmp(pos, "wep_key0") == 0 ||
			    os_strcmp(pos, "wep_key1") == 0 ||
			    os_strcmp(pos, "wep_key2") == 0 ||
			    os_strcmp(pos, "wep_key3") == 0 ||
			    os_strcmp(pos, "wep_tx_keyidx") == 0) {
				wpa_printf(MSG_ERROR,
					   "Line %d: unsupported WEP parameter",
					   *line);
				ssid->disabled = 1;
				continue;
			}
#endif /* CONFIG_WEP */
			errors++;
		}
	}

	if (!end) {
		wpa_printf(MSG_ERROR, "Line %d: network block was not "
			   "terminated properly.", *line);
		errors++;
	}

	errors += wpa_config_validate_network(ssid, *line);

	if (errors) {
		wpa_config_free_ssid(ssid);
		ssid = NULL;
	}

    os_free(buf); /*porting*/
	return ssid;
}


static struct wpa_cred * wpa_config_read_cred(int *line, int id) /*porting*/
{
	struct wpa_cred *cred;
	int errors = 0, end = 0;
	char *buf, *pos, *pos2;

	wpa_printf(MSG_MSGDUMP, "Line: %d - start of a new cred block", *line);
	cred = os_zalloc(sizeof(*cred));
	if (cred == NULL)
		return NULL;
/*porting*/
    buf = os_malloc(256);
    if(buf == NULL){
		os_free(cred);
		return NULL;
	}

	cred->id = id;
	cred->sim_num = DEFAULT_USER_SELECTED_SIM;

	while (wpa_config_get_line(buf, 256, line, &pos)) { /*porting*/
		if (os_strcmp(pos, "}") == 0) {
			end = 1;
			break;
		}

		pos2 = os_strchr(pos, '=');
		if (pos2 == NULL) {
			wpa_printf(MSG_ERROR, "Line %d: Invalid cred line "
				   "'%s'.", *line, pos);
			errors++;
			continue;
		}

		*pos2++ = '\0';
		if (*pos2 == '"') {
			if (os_strchr(pos2 + 1, '"') == NULL) {
				wpa_printf(MSG_ERROR, "Line %d: invalid "
					   "quotation '%s'.", *line, pos2);
				errors++;
				continue;
			}
		}

		if (wpa_config_set_cred(cred, pos, pos2, *line) < 0)
			errors++;
	}

	if (!end) {
		wpa_printf(MSG_ERROR, "Line %d: cred block was not "
			   "terminated properly.", *line);
		errors++;
	}

	if (errors) {
		wpa_config_free_cred(cred);
		cred = NULL;
	}

    os_free(buf); /*porting*/
	return cred;
}


#ifndef CONFIG_NO_CONFIG_BLOBS
static struct wpa_config_blob * wpa_config_read_blob(int *line, /*porting*/
						     const char *name)
{
	struct wpa_config_blob *blob;
	char *buf, *pos;
	char *encoded = NULL, *nencoded;
	int end = 0;
	size_t encoded_len = 0, len;

	wpa_printf(MSG_MSGDUMP, "Line: %d - start of a new named blob '%s'",
		   *line, name);
/*porting*/
    buf = os_malloc(256);
    if(buf == NULL){
		return NULL;
	}

	while (wpa_config_get_line(buf, 256, line, &pos)) { /*porting*/
		if (os_strcmp(pos, "}") == 0) {
			end = 1;
			break;
		}

		len = os_strlen(pos);
		nencoded = os_realloc(encoded, encoded_len + len);
		if (nencoded == NULL) {
			wpa_printf(MSG_ERROR, "Line %d: not enough memory for "
				   "blob", *line);
			os_free(buf); /*porting*/
			os_free(encoded);
			return NULL;
		}
		encoded = nencoded;
		os_memcpy(encoded + encoded_len, pos, len);
		encoded_len += len;
	}

	if (!end || !encoded) {
		wpa_printf(MSG_ERROR, "Line %d: blob was not terminated "
			   "properly", *line);
        os_free(buf); /*porting*/
		os_free(encoded);
		return NULL;
	}

	blob = os_zalloc(sizeof(*blob));
	if (blob == NULL) {
        os_free(buf); /*porting*/
		os_free(encoded);
		return NULL;
	}
	blob->name = os_strdup(name);
	blob->data = base64_decode(encoded, encoded_len, &blob->len);
	os_free(encoded);
    os_free(buf); /*porting*/

	if (blob->name == NULL || blob->data == NULL) {
		wpa_config_free_blob(blob);
		return NULL;
	}

	return blob;
}


static int wpa_config_process_blob(struct wpa_config *config,  /*porting*/
				   int *line, char *bname)
{
	char *name_end;
	struct wpa_config_blob *blob;

	name_end = os_strchr(bname, '=');
	if (name_end == NULL) {
		wpa_printf(MSG_ERROR, "Line %d: no blob name terminator",
			   *line);
		return -1;
	}
	*name_end = '\0';

	blob = wpa_config_read_blob(line, bname); /*porting*/
	if (blob == NULL) {
		wpa_printf(MSG_ERROR, "Line %d: failed to read blob %s",
			   *line, bname);
		return -1;
	}
	wpa_config_set_blob(config, blob);
	return 0;
}
#endif /* CONFIG_NO_CONFIG_BLOBS */


struct wpa_config * wpa_config_read(const char *name, struct wpa_config *cfgp)
{
	char *buf, *pos; /*porting*/
	int errors = 0, line = 0;
	struct wpa_ssid *ssid, *tail, *head;
	struct wpa_cred *cred, *cred_tail, *cred_head;
	struct wpa_config *config;
	int id = 0;
	int cred_id = 0;

	if (name == NULL)
		return NULL;
/*porting*/   
    buf = os_malloc(512);
    if(buf == NULL){
		return NULL;
	}

	if (cfgp)
		config = cfgp;
	else
		config = wpa_config_alloc_empty(NULL, NULL);
	if (config == NULL) {
		wpa_printf(MSG_ERROR, "Failed to allocate config file "
			   "structure");
		os_free(buf); /*porting*/
		return NULL;
	}
	tail = head = config->ssid;
	while (tail && tail->next)
		tail = tail->next;
	cred_tail = cred_head = config->cred;
	while (cred_tail && cred_tail->next)
		cred_tail = cred_tail->next;

	wpa_printf(MSG_DEBUG, "Reading configuration file '%s'", name);
    if(wpa_config_buff == NULL){ /*porting*/
        wpa_config_buff = os_malloc(CONFIG_BUFF_SIZE);
    }
    config_read_off = 0;
    umac_config_read(name, wpa_config_buff, CONFIG_BUFF_SIZE);

	while (wpa_config_get_line(buf, 512, &line, &pos)) { /*porting*/
		if (os_strcmp(pos, "network={") == 0) {
			ssid = wpa_config_read_network(&line, id++);
			if (ssid == NULL) {
				wpa_printf(MSG_ERROR, "Line %d: failed to "
					   "parse network block.", line);
				errors++;
				continue;
			}
			if (head == NULL) {
				head = tail = ssid;
			} else {
				tail->next = ssid;
				tail = ssid;
			}
			if (wpa_config_add_prio_network(config, ssid)) {
				wpa_printf(MSG_ERROR, "Line %d: failed to add "
					   "network block to priority list.",
					   line);
				errors++;
				continue;
			}
		} else if (os_strcmp(pos, "cred={") == 0) {
			cred = wpa_config_read_cred(&line, cred_id++); /*porting*/
			if (cred == NULL) {
				wpa_printf(MSG_ERROR, "Line %d: failed to "
					   "parse cred block.", line);
				errors++;
				continue;
			}
			if (cred_head == NULL) {
				cred_head = cred_tail = cred;
			} else {
				cred_tail->next = cred;
				cred_tail = cred;
			}
#ifndef CONFIG_NO_CONFIG_BLOBS
		} else if (os_strncmp(pos, "blob-base64-", 12) == 0) {
			if (wpa_config_process_blob(config, &line, pos + 12) /*porting*/
			    < 0) {
				wpa_printf(MSG_ERROR, "Line %d: failed to "
					   "process blob.", line);
				errors++;
				continue;
			}
#endif /* CONFIG_NO_CONFIG_BLOBS */
		} else if (wpa_config_process_global(config, pos, line) < 0) {
			wpa_printf(MSG_ERROR, "Line %d: Invalid configuration "
				   "line '%s'.", line, pos);
			errors++;
			continue;
		}
	}

	config->ssid = head;
	wpa_config_debug_dump_networks(config);
	config->cred = cred_head;

#ifndef WPA_IGNORE_CONFIG_ERRORS
	if (errors) {
		if (config != cfgp)
			wpa_config_free(config);
		config = NULL;
		head = NULL;
	}
#endif /* WPA_IGNORE_CONFIG_ERRORS */
/*porting*/
    os_free(buf);
    os_free(wpa_config_buff);
    wpa_config_buff = NULL;
    if(config && config->ssid == NULL){ /*add a default network*/
        wpa_config_add_network(config);
        wpa_config_set_network_defaults(config->ssid);
    }
	return config;
}


#ifndef CONFIG_NO_CONFIG_WRITE

static char * write_str(char *f, const char *field, struct wpa_ssid *ssid) /*porting*/
{
	char *value = wpa_config_get(ssid, field);
	if (value == NULL)
		return f;
	f=hgvsprintf(f, "\t%s=%s\n", field, value);
	str_clear_free(value);
    return f;
}


static char * write_int(char *f, const char *field, int value, int def) /*porting*/
{
	if (value == def)
		return f;
	f=hgvsprintf(f, "\t%s=%d\n", field, value);
    return f;
}


static char * write_bssid(char *f, struct wpa_ssid *ssid) /*porting*/
{
	char *value = wpa_config_get(ssid, "bssid");
	if (value == NULL)
		return f;
	f=hgvsprintf(f, "\tbssid=%s\n", value);
	os_free(value);
    return f;
}


static char * write_bssid_hint(char *f, struct wpa_ssid *ssid) /*porting*/
{
	char *value = wpa_config_get(ssid, "bssid_hint");

	if (!value)
		return f;
	f=hgvsprintf(f, "\tbssid_hint=%s\n", value);
	os_free(value);
    return f;
}


static char *write_psk(char *f, struct wpa_ssid *ssid) /*porting*/
{
	char *value;

	if (ssid->mem_only_psk)
		return f;

	value = wpa_config_get(ssid, "psk");
	if (value == NULL)
		return f;
	f=hgvsprintf(f, "\tpsk=%s\n", value);
	os_free(value);
    return f;
}


static char *write_proto(char *f, struct wpa_ssid *ssid) /*porting*/
{
	char *value;

	if (ssid->proto == DEFAULT_PROTO)
		return f;

	value = wpa_config_get(ssid, "proto");
	if (value == NULL)
		return f;
	if (value[0])
		f=hgvsprintf(f, "\tproto=%s\n", value);
	os_free(value);
    return f;
}


static char *write_key_mgmt(char *f, struct wpa_ssid *ssid) /*porting*/
{
	char *value;

	if (ssid->key_mgmt == DEFAULT_KEY_MGMT)
		return f;

	value = wpa_config_get(ssid, "key_mgmt");
	if (value == NULL)
		return f;
	if (value[0])
		f=hgvsprintf(f, "\tkey_mgmt=%s\n", value);
	os_free(value);
    return f;
}


static char *write_pairwise(char *f, struct wpa_ssid *ssid) /*porting*/
{
	char *value;

	if (ssid->pairwise_cipher == DEFAULT_PAIRWISE)
		return f;

	value = wpa_config_get(ssid, "pairwise");
	if (value == NULL)
		return f;
	if (value[0])
		f=hgvsprintf(f, "\tpairwise=%s\n", value);
	os_free(value);
    return f;
}


static char *write_group(char *f, struct wpa_ssid *ssid) /*porting*/
{
	char *value;

	if (ssid->group_cipher == DEFAULT_GROUP)
		return f;

	value = wpa_config_get(ssid, "group");
	if (value == NULL)
		return f;
	if (value[0])
		f=hgvsprintf(f, "\tgroup=%s\n", value);
	os_free(value);
    return f;
}


static char * write_group_mgmt(char *f, struct wpa_ssid *ssid) /*porting*/
{
	char *value;

	if (!ssid->group_mgmt_cipher)
		return f;

	value = wpa_config_get(ssid, "group_mgmt");
	if (!value)
		return f;
	if (value[0])
		f=hgvsprintf(f, "\tgroup_mgmt=%s\n", value);
	os_free(value);
    return f;
}


static char *write_auth_alg(char *f, struct wpa_ssid *ssid) /*porting*/
{
	char *value;

	if (ssid->auth_alg == 0)
		return f;

	value = wpa_config_get(ssid, "auth_alg");
	if (value == NULL)
		return f;
	if (value[0])
		f=hgvsprintf(f, "\tauth_alg=%s\n", value);
	os_free(value);
    return f;
}



#ifdef IEEE8021X_EAPOL
static char *write_eap(char *f, struct wpa_ssid *ssid) /*porting*/
{
	char *value;

	value = wpa_config_get(ssid, "eap");
	if (value == NULL)
		return f;

	if (value[0])
		f=hgvsprintf(f, "\teap=%s\n", value);
	os_free(value);
    return f;
}
#endif /* IEEE8021X_EAPOL */


#ifdef CONFIG_WEP
static char *write_wep_key(char *f, int idx, struct wpa_ssid *ssid) /*porting*/
{
	char field[20], *value;
	int res;

	res = os_snprintf(field, sizeof(field), "wep_key%d", idx);
	if (os_snprintf_error(sizeof(field), res))
		return f;
	value = wpa_config_get(ssid, field);
	if (value) {
		f=hgvsprintf(f, "\t%s=%s\n", field, value);
		os_free(value);
	}
    return f;
}
#endif /* CONFIG_WEP */


#ifdef CONFIG_P2P

static char *write_go_p2p_dev_addr(char *f, struct wpa_ssid *ssid) /*porting*/
{
	char *value = wpa_config_get(ssid, "go_p2p_dev_addr");
	if (value == NULL)
		return f;
	f=hgvsprintf(f, "\tgo_p2p_dev_addr=%s\n", value);
	os_free(value);
    return f;
}

static char *write_p2p_client_list(char *f, struct wpa_ssid *ssid) /*porting*/
{
	char *value = wpa_config_get(ssid, "p2p_client_list");
	if (value == NULL)
        return f;
	f=hgvsprintf(f, "\tp2p_client_list=%s\n", value);
	os_free(value);
    return f;
}


static char *write_psk_list(char *f, struct wpa_ssid *ssid) /*porting*/
{
	struct psk_list_entry *psk;
	char hex[32 * 2 + 1];

	dl_list_for_each(psk, &ssid->psk_list, struct psk_list_entry, list) {
		wpa_snprintf_hex(hex, sizeof(hex), psk->psk, sizeof(psk->psk));
		f=hgvsprintf(f, "\tpsk_list=%s" MACSTR "-%s\n",
			psk->p2p ? "P2P-" : "", MAC2STR(psk->addr), hex);
	}
    return f;
}

#endif /* CONFIG_P2P */


#ifdef CONFIG_MACSEC

static char * write_mka_cak(char *f, struct wpa_ssid *ssid) /*porting*/
{
	char *value;

	if (!(ssid->mka_psk_set & MKA_PSK_SET_CAK))
        return f;

	value = wpa_config_get(ssid, "mka_cak");
	if (!value)
        return f;
	f=hgvsprintf(f, "\tmka_cak=%s\n", value);
	os_free(value);
    return f;
}


static char * write_mka_ckn(char *f, struct wpa_ssid *ssid) /*porting*/
{
	char *value;

	if (!(ssid->mka_psk_set & MKA_PSK_SET_CKN))
        return f;

	value = wpa_config_get(ssid, "mka_ckn");
	if (!value)
        return f;
	f=hgvsprintf(f, "\tmka_ckn=%s\n", value);
	os_free(value);
    return f;
}

#endif /* CONFIG_MACSEC */


static char *wpa_config_write_network(char *f, struct wpa_ssid *ssid) /*porting*/
{
#define STR(t) f=write_str(f, #t, ssid)
#define INT(t) f=write_int(f, #t, ssid->t, 0)
#define INTe(t, m) f=write_int(f, #t, ssid->eap.m, 0)
#define INT_DEF(t, def) f=write_int(f, #t, ssid->t, def)
#define INT_DEFe(t, m, def) f=write_int(f, #t, ssid->eap.m, def)

	STR(ssid);
	INT(scan_ssid);
	f=write_bssid(f, ssid);
	f=write_bssid_hint(f, ssid);
	f=write_str(f, "bssid_ignore", ssid);
	f=write_str(f, "bssid_accept", ssid);
	f=write_psk(f, ssid);
	INT(mem_only_psk);
	STR(sae_password);
	STR(sae_password_id);
	f=write_proto(f, ssid);
	f=write_key_mgmt(f, ssid);
	INT_DEF(bg_scan_period, DEFAULT_BG_SCAN_PERIOD);
	f=write_pairwise(f, ssid);
	f=write_group(f, ssid);
	f=write_group_mgmt(f, ssid);
	f=write_auth_alg(f, ssid);
	STR(bgscan);
	STR(autoscan);
	STR(scan_freq);
#ifdef IEEE8021X_EAPOL
	f=write_eap(f, ssid);
	STR(identity);
	STR(anonymous_identity);
	STR(imsi_identity);
	STR(machine_identity);
	STR(password);
	STR(machine_password);
	STR(ca_cert);
	STR(ca_path);
	STR(client_cert);
	STR(private_key);
	STR(private_key_passwd);
	STR(dh_file);
	STR(subject_match);
	STR(check_cert_subject);
	STR(altsubject_match);
	STR(domain_suffix_match);
	STR(domain_match);
	STR(ca_cert2);
	STR(ca_path2);
	STR(client_cert2);
	STR(private_key2);
	STR(private_key2_passwd);
	STR(dh_file2);
	STR(subject_match2);
	STR(check_cert_subject2);
	STR(altsubject_match2);
	STR(domain_suffix_match2);
	STR(domain_match2);
	STR(machine_ca_cert);
	STR(machine_ca_path);
	STR(machine_client_cert);
	STR(machine_private_key);
	STR(machine_private_key_passwd);
	STR(machine_dh_file);
	STR(machine_subject_match);
	STR(machine_check_cert_subject);
	STR(machine_altsubject_match);
	STR(machine_domain_suffix_match);
	STR(machine_domain_match);
	STR(phase1);
	STR(phase2);
	STR(machine_phase2);
	STR(pcsc);
	STR(pin);
	STR(engine_id);
	STR(key_id);
	STR(cert_id);
	STR(ca_cert_id);
	STR(key2_id);
	STR(pin2);
	STR(engine2_id);
	STR(cert2_id);
	STR(ca_cert2_id);
	INTe(engine, cert.engine);
	INTe(engine2, phase2_cert.engine);
	INTe(machine_engine, machine_cert.engine);
	INT_DEF(eapol_flags, DEFAULT_EAPOL_FLAGS);
	STR(openssl_ciphers);
	INTe(erp, erp);
#endif /* IEEE8021X_EAPOL */
#ifdef CONFIG_WEP
	{
		int i;

		for (i = 0; i < 4; i++)
			f=write_wep_key(f, i, ssid); /*porting*/
		INT(wep_tx_keyidx);
	}
#endif /* CONFIG_WEP */
	INT(priority);
#ifdef IEEE8021X_EAPOL
	INT_DEF(eap_workaround, DEFAULT_EAP_WORKAROUND);
	STR(pac_file);
	INT_DEFe(fragment_size, fragment_size, DEFAULT_FRAGMENT_SIZE);
	INTe(ocsp, cert.ocsp);
	INTe(ocsp2, phase2_cert.ocsp);
	INTe(machine_ocsp, machine_cert.ocsp);
	INT_DEFe(sim_num, sim_num, DEFAULT_USER_SELECTED_SIM);
#endif /* IEEE8021X_EAPOL */
	INT(mode);
	INT(no_auto_peer);
	INT(frequency);
	INT(enable_edmg);
	INT(edmg_channel);
	INT(fixed_freq);
#ifdef CONFIG_ACS
	INT(acs);
#endif /* CONFIG_ACS */
	f=write_int(f, "proactive_key_caching", ssid->proactive_key_caching, -1); /*porting*/
	INT(disabled);
	INT(mixed_cell);
	INT_DEF(vht, 1);
	INT_DEF(ht, 1);
	INT(ht40);
	INT_DEF(he, 1);
	INT_DEF(max_oper_chwidth, DEFAULT_MAX_OPER_CHWIDTH);
	INT(vht_center_freq1);
	INT(vht_center_freq2);
	INT(pbss);
	INT(wps_disabled);
	INT(fils_dh_group);
	f=write_int(f, "ieee80211w", ssid->ieee80211w, /*porting*/
		  MGMT_FRAME_PROTECTION_DEFAULT);
	STR(id_str);
#ifdef CONFIG_P2P
	f=write_go_p2p_dev_addr(f, ssid); /*porting*/
	f=write_p2p_client_list(f, ssid); /*porting*/
	f=write_psk_list(f, ssid); /*porting*/
#endif /* CONFIG_P2P */
	INT(ap_max_inactivity);
	INT(dtim_period);
	INT(beacon_int);
#ifdef CONFIG_MACSEC
	INT(macsec_policy);
	f=write_mka_cak(f, ssid); /*porting*/
	f=write_mka_ckn(f, ssid); /*porting*/
	INT(macsec_integ_only);
	INT(macsec_replay_protect);
	INT(macsec_replay_window);
	INT(macsec_port);
	INT_DEF(mka_priority, DEFAULT_PRIO_NOT_KEY_SERVER);
#endif /* CONFIG_MACSEC */
#ifdef CONFIG_HS20
	INT(update_identifier);
	STR(roaming_consortium_selection);
#endif /* CONFIG_HS20 */
	f=write_int(f, "mac_addr", ssid->mac_addr, -1); /*porting*/
#ifdef CONFIG_MESH
	STR(mesh_basic_rates);
	INT_DEF(dot11MeshMaxRetries, DEFAULT_MESH_MAX_RETRIES);
	INT_DEF(dot11MeshRetryTimeout, DEFAULT_MESH_RETRY_TIMEOUT);
	INT_DEF(dot11MeshConfirmTimeout, DEFAULT_MESH_CONFIRM_TIMEOUT);
	INT_DEF(dot11MeshHoldingTimeout, DEFAULT_MESH_HOLDING_TIMEOUT);
	INT_DEF(mesh_rssi_threshold, DEFAULT_MESH_RSSI_THRESHOLD);
#endif /* CONFIG_MESH */
	INT(wpa_ptk_rekey);
	INT(wpa_deny_ptk0_rekey);
	INT(group_rekey);
	INT(ignore_broadcast_ssid);
#ifdef CONFIG_DPP
	STR(dpp_connector);
	STR(dpp_netaccesskey);
	INT(dpp_netaccesskey_expiry);
	STR(dpp_csign);
	STR(dpp_pp_key);
	INT(dpp_pfs);
#endif /* CONFIG_DPP */
	INT(owe_group);
	INT(owe_only);
	INT(owe_ptk_workaround);
	INT(multi_ap_backhaul_sta);
	INT(ft_eap_pmksa_caching);
	INT(beacon_prot);
	INT(transition_disable);
	INT(sae_pk);
#ifdef CONFIG_HT_OVERRIDES
	INT_DEF(disable_ht, DEFAULT_DISABLE_HT);
	INT_DEF(disable_ht40, DEFAULT_DISABLE_HT40);
	INT_DEF(disable_sgi, DEFAULT_DISABLE_SGI);
	INT_DEF(disable_ldpc, DEFAULT_DISABLE_LDPC);
	INT(ht40_intolerant);
	INT_DEF(tx_stbc, DEFAULT_TX_STBC);
	INT_DEF(rx_stbc, DEFAULT_RX_STBC);
	INT_DEF(disable_max_amsdu, DEFAULT_DISABLE_MAX_AMSDU);
	INT_DEF(ampdu_factor, DEFAULT_AMPDU_FACTOR);
	INT_DEF(ampdu_density, DEFAULT_AMPDU_DENSITY);
	STR(ht_mcs);
#endif /* CONFIG_HT_OVERRIDES */
#ifdef CONFIG_VHT_OVERRIDES
	INT(disable_vht);
	INT(vht_capa);
	INT(vht_capa_mask);
	INT_DEF(vht_rx_mcs_nss_1, -1);
	INT_DEF(vht_rx_mcs_nss_2, -1);
	INT_DEF(vht_rx_mcs_nss_3, -1);
	INT_DEF(vht_rx_mcs_nss_4, -1);
	INT_DEF(vht_rx_mcs_nss_5, -1);
	INT_DEF(vht_rx_mcs_nss_6, -1);
	INT_DEF(vht_rx_mcs_nss_7, -1);
	INT_DEF(vht_rx_mcs_nss_8, -1);
	INT_DEF(vht_tx_mcs_nss_1, -1);
	INT_DEF(vht_tx_mcs_nss_2, -1);
	INT_DEF(vht_tx_mcs_nss_3, -1);
	INT_DEF(vht_tx_mcs_nss_4, -1);
	INT_DEF(vht_tx_mcs_nss_5, -1);
	INT_DEF(vht_tx_mcs_nss_6, -1);
	INT_DEF(vht_tx_mcs_nss_7, -1);
	INT_DEF(vht_tx_mcs_nss_8, -1);
#endif /* CONFIG_VHT_OVERRIDES */
#ifdef CONFIG_HE_OVERRIDES
	INT(disable_he);
#endif /* CONFIG_HE_OVERRIDES */

#undef STR
#undef INT
#undef INT_DEF
    return f; /*porting*/
}


static char *wpa_config_write_cred(char *f, struct wpa_cred *cred) /*porting*/
{
	size_t i;

	if (cred->priority)
		f=hgvsprintf(f, "\tpriority=%d\n", cred->priority);
	if (cred->pcsc)
		f=hgvsprintf(f, "\tpcsc=%d\n", cred->pcsc);
	if (cred->realm)
		f=hgvsprintf(f, "\trealm=\"%s\"\n", cred->realm);
	if (cred->username)
		f=hgvsprintf(f, "\tusername=\"%s\"\n", cred->username);
	if (cred->password && cred->ext_password)
		f=hgvsprintf(f, "\tpassword=ext:%s\n", cred->password);
	else if (cred->password)
		f=hgvsprintf(f, "\tpassword=\"%s\"\n", cred->password);
	if (cred->ca_cert)
		f=hgvsprintf(f, "\tca_cert=\"%s\"\n", cred->ca_cert);
	if (cred->client_cert)
		f=hgvsprintf(f, "\tclient_cert=\"%s\"\n", cred->client_cert);
	if (cred->private_key)
		f=hgvsprintf(f, "\tprivate_key=\"%s\"\n", cred->private_key);
	if (cred->private_key_passwd)
		f=hgvsprintf(f, "\tprivate_key_passwd=\"%s\"\n",
			cred->private_key_passwd);
	if (cred->imsi)
		f=hgvsprintf(f, "\timsi=\"%s\"\n", cred->imsi);
	if (cred->milenage)
		f=hgvsprintf(f, "\tmilenage=\"%s\"\n", cred->milenage);
	for (i = 0; i < cred->num_domain; i++)
		f=hgvsprintf(f, "\tdomain=\"%s\"\n", cred->domain[i]);
	if (cred->domain_suffix_match)
		f=hgvsprintf(f, "\tdomain_suffix_match=\"%s\"\n",
			cred->domain_suffix_match);
	if (cred->roaming_consortium_len) {
		f=hgvsprintf(f, "\troaming_consortium=");
		for (i = 0; i < cred->roaming_consortium_len; i++)
			f=hgvsprintf(f, "%02x", cred->roaming_consortium[i]);
		f=hgvsprintf(f, "\n");
	}
	if (cred->eap_method) {
		const char *name;
		name = eap_get_name(cred->eap_method[0].vendor,
				    cred->eap_method[0].method);
		if (name)
			f=hgvsprintf(f, "\teap=%s\n", name);
	}
	if (cred->phase1)
		f=hgvsprintf(f, "\tphase1=\"%s\"\n", cred->phase1);
	if (cred->phase2)
		f=hgvsprintf(f, "\tphase2=\"%s\"\n", cred->phase2);
	if (cred->excluded_ssid) {
		size_t j;
		for (i = 0; i < cred->num_excluded_ssid; i++) {
			struct excluded_ssid *e = &cred->excluded_ssid[i];
			f=hgvsprintf(f, "\texcluded_ssid=");
			for (j = 0; j < e->ssid_len; j++)
				f=hgvsprintf(f, "%02x", e->ssid[j]);
			f=hgvsprintf(f, "\n");
		}
	}
	if (cred->roaming_partner) {
		for (i = 0; i < cred->num_roaming_partner; i++) {
			struct roaming_partner *p = &cred->roaming_partner[i];
			f=hgvsprintf(f, "\troaming_partner=\"%s,%d,%u,%s\"\n",
				p->fqdn, p->exact_match, p->priority,
				p->country);
		}
	}
	if (cred->update_identifier)
		f=hgvsprintf(f, "\tupdate_identifier=%d\n", cred->update_identifier);

	if (cred->provisioning_sp)
		f=hgvsprintf(f, "\tprovisioning_sp=\"%s\"\n", cred->provisioning_sp);
	if (cred->sp_priority)
		f=hgvsprintf(f, "\tsp_priority=%d\n", cred->sp_priority);

	if (cred->min_dl_bandwidth_home)
		f=hgvsprintf(f, "\tmin_dl_bandwidth_home=%u\n",
			cred->min_dl_bandwidth_home);
	if (cred->min_ul_bandwidth_home)
		f=hgvsprintf(f, "\tmin_ul_bandwidth_home=%u\n",
			cred->min_ul_bandwidth_home);
	if (cred->min_dl_bandwidth_roaming)
		f=hgvsprintf(f, "\tmin_dl_bandwidth_roaming=%u\n",
			cred->min_dl_bandwidth_roaming);
	if (cred->min_ul_bandwidth_roaming)
		f=hgvsprintf(f, "\tmin_ul_bandwidth_roaming=%u\n",
			cred->min_ul_bandwidth_roaming);

	if (cred->max_bss_load)
		f=hgvsprintf(f, "\tmax_bss_load=%u\n",
			cred->max_bss_load);

	if (cred->ocsp)
		f=hgvsprintf(f, "\tocsp=%d\n", cred->ocsp);

	if (cred->num_req_conn_capab) {
		for (i = 0; i < cred->num_req_conn_capab; i++) {
			int *ports;

			f=hgvsprintf(f, "\treq_conn_capab=%u",
				cred->req_conn_capab_proto[i]);
			ports = cred->req_conn_capab_port[i];
			if (ports) {
				int j;
				for (j = 0; ports[j] != -1; j++) {
					f=hgvsprintf(f, "%s%d", j > 0 ? "," : ":",
						ports[j]);
				}
			}
			f=hgvsprintf(f, "\n");
		}
	}

	if (cred->required_roaming_consortium_len) {
		f=hgvsprintf(f, "\trequired_roaming_consortium=");
		for (i = 0; i < cred->required_roaming_consortium_len; i++)
			f=hgvsprintf(f, "%02x",
				cred->required_roaming_consortium[i]);
		f=hgvsprintf(f, "\n");
	}

	if (cred->num_roaming_consortiums) {
		size_t j;

		f=hgvsprintf(f, "\troaming_consortiums=\"");
		for (i = 0; i < cred->num_roaming_consortiums; i++) {
			if (i > 0)
				f=hgvsprintf(f, ",");
			for (j = 0; j < cred->roaming_consortiums_len[i]; j++)
				f=hgvsprintf(f, "%02x",
					cred->roaming_consortiums[i][j]);
		}
		f=hgvsprintf(f, "\"\n");
	}

	if (cred->sim_num != DEFAULT_USER_SELECTED_SIM)
		f=hgvsprintf(f, "\tsim_num=%d\n", cred->sim_num);
    return f;
}


#ifndef CONFIG_NO_CONFIG_BLOBS
static char * wpa_config_write_blob(char *f, struct wpa_config_blob *blob) /*porting*/
{
	char *encoded;

	encoded = base64_encode(blob->data, blob->len, NULL);
	if (encoded == NULL)
		return f;

	f=hgvsprintf(f, "\nblob-base64-%s={\n%s}\n", blob->name, encoded);
	os_free(encoded);
    return f;
}
#endif /* CONFIG_NO_CONFIG_BLOBS */


static char * write_global_bin(char *f, const char *field,  /*porting*/
			     const struct wpabuf *val)
{
	size_t i;
	const u8 *pos;

	if (val == NULL)
		return f;

	f=hgvsprintf(f, "%s=", field);
	pos = wpabuf_head(val);
	for (i = 0; i < wpabuf_len(val); i++)
		f=hgvsprintf(f, "%02X", *pos++);
	f=hgvsprintf(f, "\n");
    return f;
}


static char * wpa_config_write_global(char *f, struct wpa_config *config)  /*porting*/
{
#ifdef CONFIG_CTRL_IFACE
	if (config->ctrl_interface)
		f=hgvsprintf(f, "ctrl_interface=%s\n", config->ctrl_interface);
	if (config->ctrl_interface_group)
		f=hgvsprintf(f, "ctrl_interface_group=%s\n",
			config->ctrl_interface_group);
#endif /* CONFIG_CTRL_IFACE */
	if (config->eapol_version != DEFAULT_EAPOL_VERSION)
		f=hgvsprintf(f, "eapol_version=%d\n", config->eapol_version);
	if (config->ap_scan != DEFAULT_AP_SCAN)
		f=hgvsprintf(f, "ap_scan=%d\n", config->ap_scan);
	if (config->disable_scan_offload)
		f=hgvsprintf(f, "disable_scan_offload=%d\n",
			config->disable_scan_offload);
	if (config->fast_reauth != DEFAULT_FAST_REAUTH)
		f=hgvsprintf(f, "fast_reauth=%d\n", config->fast_reauth);
	if (config->opensc_engine_path)
		f=hgvsprintf(f, "opensc_engine_path=%s\n",
			config->opensc_engine_path);
	if (config->pkcs11_engine_path)
		f=hgvsprintf(f, "pkcs11_engine_path=%s\n",
			config->pkcs11_engine_path);
	if (config->pkcs11_module_path)
		f=hgvsprintf(f, "pkcs11_module_path=%s\n",
			config->pkcs11_module_path);
	if (config->openssl_ciphers)
		f=hgvsprintf(f, "openssl_ciphers=%s\n", config->openssl_ciphers);
	if (config->pcsc_reader)
		f=hgvsprintf(f, "pcsc_reader=%s\n", config->pcsc_reader);
	if (config->pcsc_pin)
		f=hgvsprintf(f, "pcsc_pin=%s\n", config->pcsc_pin);
	if (config->driver_param)
		f=hgvsprintf(f, "driver_param=%s\n", config->driver_param);
	if (config->dot11RSNAConfigPMKLifetime)
		f=hgvsprintf(f, "dot11RSNAConfigPMKLifetime=%u\n",
			config->dot11RSNAConfigPMKLifetime);
	if (config->dot11RSNAConfigPMKReauthThreshold)
		f=hgvsprintf(f, "dot11RSNAConfigPMKReauthThreshold=%u\n",
			config->dot11RSNAConfigPMKReauthThreshold);
	if (config->dot11RSNAConfigSATimeout)
		f=hgvsprintf(f, "dot11RSNAConfigSATimeout=%u\n",
			config->dot11RSNAConfigSATimeout);
	if (config->update_config)
		f=hgvsprintf(f, "update_config=%d\n", config->update_config);
#ifdef CONFIG_WPS
	if (!is_nil_uuid(config->uuid)) {
		char buf[40];
		uuid_bin2str(config->uuid, buf, sizeof(buf));
		f=hgvsprintf(f, "uuid=%s\n", buf);
	}
	if (config->auto_uuid)
		f=hgvsprintf(f, "auto_uuid=%d\n", config->auto_uuid);
	if (config->device_name)
		f=hgvsprintf(f, "device_name=%s\n", config->device_name);
	if (config->manufacturer)
		f=hgvsprintf(f, "manufacturer=%s\n", config->manufacturer);
	if (config->model_name)
		f=hgvsprintf(f, "model_name=%s\n", config->model_name);
	if (config->model_number)
		f=hgvsprintf(f, "model_number=%s\n", config->model_number);
	if (config->serial_number)
		f=hgvsprintf(f, "serial_number=%s\n", config->serial_number);
	{
		char _buf[WPS_DEV_TYPE_BUFSIZE], *buf;
		buf = wps_dev_type_bin2str(config->device_type,
					   _buf, sizeof(_buf));
		if (os_strcmp(buf, "0-00000000-0") != 0)
			f=hgvsprintf(f, "device_type=%s\n", buf);
	}
	if (WPA_GET_BE32(config->os_version))
		f=hgvsprintf(f, "os_version=%08x\n",
			WPA_GET_BE32(config->os_version));
	if (config->config_methods)
		f=hgvsprintf(f, "config_methods=%s\n", config->config_methods);
	if (config->wps_cred_processing)
		f=hgvsprintf(f, "wps_cred_processing=%d\n",
			config->wps_cred_processing);
	if (config->wps_cred_add_sae)
		f=hgvsprintf(f, "wps_cred_add_sae=%d\n",
			config->wps_cred_add_sae);
	if (config->wps_vendor_ext_m1) {
		int i, len = wpabuf_len(config->wps_vendor_ext_m1);
		const u8 *p = wpabuf_head_u8(config->wps_vendor_ext_m1);
		if (len > 0) {
			f=hgvsprintf(f, "wps_vendor_ext_m1=");
			for (i = 0; i < len; i++)
				f=hgvsprintf(f, "%02x", *p++);
			f=hgvsprintf(f, "\n");
		}
	}
#endif /* CONFIG_WPS */
#ifdef CONFIG_P2P
	{
		int i;
		char _buf[WPS_DEV_TYPE_BUFSIZE], *buf;

		for (i = 0; i < config->num_sec_device_types; i++) {
			buf = wps_dev_type_bin2str(config->sec_device_type[i],
						   _buf, sizeof(_buf));
			if (buf)
				f=hgvsprintf(f, "sec_device_type=%s\n", buf);
		}
	}
	if (config->p2p_listen_reg_class)
		f=hgvsprintf(f, "p2p_listen_reg_class=%d\n",
			config->p2p_listen_reg_class);
	if (config->p2p_listen_channel)
		f=hgvsprintf(f, "p2p_listen_channel=%d\n",
			config->p2p_listen_channel);
	if (config->p2p_oper_reg_class)
		f=hgvsprintf(f, "p2p_oper_reg_class=%d\n",
			config->p2p_oper_reg_class);
	if (config->p2p_oper_channel)
		f=hgvsprintf(f, "p2p_oper_channel=%d\n", config->p2p_oper_channel);
	if (config->p2p_go_intent != DEFAULT_P2P_GO_INTENT)
		f=hgvsprintf(f, "p2p_go_intent=%d\n", config->p2p_go_intent);
	if (config->p2p_ssid_postfix)
		f=hgvsprintf(f, "p2p_ssid_postfix=%s\n", config->p2p_ssid_postfix);
	if (config->persistent_reconnect)
		f=hgvsprintf(f, "persistent_reconnect=%d\n",
			config->persistent_reconnect);
	if (config->p2p_intra_bss != DEFAULT_P2P_INTRA_BSS)
		f=hgvsprintf(f, "p2p_intra_bss=%d\n", config->p2p_intra_bss);
	if (config->p2p_group_idle)
		f=hgvsprintf(f, "p2p_group_idle=%d\n", config->p2p_group_idle);
	if (config->p2p_passphrase_len)
		f=hgvsprintf(f, "p2p_passphrase_len=%u\n",
			config->p2p_passphrase_len);
	if (config->p2p_pref_chan) {
		unsigned int i;
		f=hgvsprintf(f, "p2p_pref_chan=");
		for (i = 0; i < config->num_p2p_pref_chan; i++) {
			f=hgvsprintf(f, "%s%u:%u", i > 0 ? "," : "",
				config->p2p_pref_chan[i].op_class,
				config->p2p_pref_chan[i].chan);
		}
		f=hgvsprintf(f, "\n");
	}
	if (config->p2p_no_go_freq.num) {
		char *val = freq_range_list_str(&config->p2p_no_go_freq);
		if (val) {
			f=hgvsprintf(f, "p2p_no_go_freq=%s\n", val);
			os_free(val);
		}
	}
	if (config->p2p_add_cli_chan)
		f=hgvsprintf(f, "p2p_add_cli_chan=%d\n", config->p2p_add_cli_chan);
	if (config->p2p_optimize_listen_chan !=
	    DEFAULT_P2P_OPTIMIZE_LISTEN_CHAN)
		f=hgvsprintf(f, "p2p_optimize_listen_chan=%d\n",
			config->p2p_optimize_listen_chan);
	if (config->p2p_go_ht40)
		f=hgvsprintf(f, "p2p_go_ht40=%d\n", config->p2p_go_ht40);
	if (config->p2p_go_vht)
		f=hgvsprintf(f, "p2p_go_vht=%d\n", config->p2p_go_vht);
	if (config->p2p_go_he)
		f=hgvsprintf(f, "p2p_go_he=%d\n", config->p2p_go_he);
	if (config->p2p_go_edmg)
		f=hgvsprintf(f, "p2p_go_edmg=%d\n", config->p2p_go_edmg);
	if (config->p2p_go_ctwindow != DEFAULT_P2P_GO_CTWINDOW)
		f=hgvsprintf(f, "p2p_go_ctwindow=%d\n", config->p2p_go_ctwindow);
	if (config->p2p_disabled)
		f=hgvsprintf(f, "p2p_disabled=%d\n", config->p2p_disabled);
	if (config->p2p_no_group_iface)
		f=hgvsprintf(f, "p2p_no_group_iface=%d\n",
			config->p2p_no_group_iface);
	if (config->p2p_ignore_shared_freq)
		f=hgvsprintf(f, "p2p_ignore_shared_freq=%d\n",
			config->p2p_ignore_shared_freq);
	if (config->p2p_cli_probe)
		f=hgvsprintf(f, "p2p_cli_probe=%d\n", config->p2p_cli_probe);
	if (config->p2p_go_freq_change_policy != DEFAULT_P2P_GO_FREQ_MOVE)
		f=hgvsprintf(f, "p2p_go_freq_change_policy=%u\n",
			config->p2p_go_freq_change_policy);

	if (config->p2p_6ghz_disable)
		f=hgvsprintf(f, "p2p_6ghz_disable=%d\n", config->p2p_6ghz_disable);

	if (WPA_GET_BE32(config->ip_addr_go))
		f=hgvsprintf(f, "ip_addr_go=%u.%u.%u.%u\n",
			config->ip_addr_go[0], config->ip_addr_go[1],
			config->ip_addr_go[2], config->ip_addr_go[3]);
	if (WPA_GET_BE32(config->ip_addr_mask))
		f=hgvsprintf(f, "ip_addr_mask=%u.%u.%u.%u\n",
			config->ip_addr_mask[0], config->ip_addr_mask[1],
			config->ip_addr_mask[2], config->ip_addr_mask[3]);
	if (WPA_GET_BE32(config->ip_addr_start))
		f=hgvsprintf(f, "ip_addr_start=%u.%u.%u.%u\n",
			config->ip_addr_start[0], config->ip_addr_start[1],
			config->ip_addr_start[2], config->ip_addr_start[3]);
	if (WPA_GET_BE32(config->ip_addr_end))
		f=hgvsprintf(f, "ip_addr_end=%u.%u.%u.%u\n",
			config->ip_addr_end[0], config->ip_addr_end[1],
			config->ip_addr_end[2], config->ip_addr_end[3]);
#endif /* CONFIG_P2P */
	if (config->country[0] && config->country[1]) {
		f=hgvsprintf(f, "country=%c%c\n",
			config->country[0], config->country[1]);
	}
	if (config->bss_max_count != DEFAULT_BSS_MAX_COUNT)
		f=hgvsprintf(f, "bss_max_count=%u\n", config->bss_max_count);
	if (config->bss_expiration_age != DEFAULT_BSS_EXPIRATION_AGE)
		f=hgvsprintf(f, "bss_expiration_age=%u\n",
			config->bss_expiration_age);
	if (config->bss_expiration_scan_count !=
	    DEFAULT_BSS_EXPIRATION_SCAN_COUNT)
		f=hgvsprintf(f, "bss_expiration_scan_count=%u\n",
			config->bss_expiration_scan_count);
	if (config->filter_ssids)
		f=hgvsprintf(f, "filter_ssids=%d\n", config->filter_ssids);
	if (config->filter_rssi)
		f=hgvsprintf(f, "filter_rssi=%d\n", config->filter_rssi);
	if (config->max_num_sta != DEFAULT_MAX_NUM_STA)
		f=hgvsprintf(f, "max_num_sta=%u\n", config->max_num_sta);
	if (config->ap_isolate != DEFAULT_AP_ISOLATE)
		f=hgvsprintf(f, "ap_isolate=%u\n", config->ap_isolate);
	if (config->disassoc_low_ack)
		f=hgvsprintf(f, "disassoc_low_ack=%d\n", config->disassoc_low_ack);
#ifdef CONFIG_HS20
	if (config->hs20)
		f=hgvsprintf(f, "hs20=1\n");
#endif /* CONFIG_HS20 */
#ifdef CONFIG_INTERWORKING
	if (config->interworking)
		f=hgvsprintf(f, "interworking=%d\n", config->interworking);
	if (!is_zero_ether_addr(config->hessid))
		f=hgvsprintf(f, "hessid=" MACSTR "\n", MAC2STR(config->hessid));
	if (config->access_network_type != DEFAULT_ACCESS_NETWORK_TYPE)
		f=hgvsprintf(f, "access_network_type=%d\n",
			config->access_network_type);
	if (config->go_interworking)
		f=hgvsprintf(f, "go_interworking=%d\n", config->go_interworking);
	if (config->go_access_network_type)
		f=hgvsprintf(f, "go_access_network_type=%d\n",
			config->go_access_network_type);
	if (config->go_internet)
		f=hgvsprintf(f, "go_internet=%d\n", config->go_internet);
	if (config->go_venue_group)
		f=hgvsprintf(f, "go_venue_group=%d\n", config->go_venue_group);
	if (config->go_venue_type)
		f=hgvsprintf(f, "go_venue_type=%d\n", config->go_venue_type);
#endif /* CONFIG_INTERWORKING */
	if (config->pbc_in_m1)
		f=hgvsprintf(f, "pbc_in_m1=%d\n", config->pbc_in_m1);
	if (config->wps_nfc_pw_from_config) {
		if (config->wps_nfc_dev_pw_id)
			f=hgvsprintf(f, "wps_nfc_dev_pw_id=%d\n",
				config->wps_nfc_dev_pw_id);
		write_global_bin(f, "wps_nfc_dh_pubkey",
				 config->wps_nfc_dh_pubkey);
		write_global_bin(f, "wps_nfc_dh_privkey",
				 config->wps_nfc_dh_privkey);
		write_global_bin(f, "wps_nfc_dev_pw", config->wps_nfc_dev_pw);
	}

	if (config->ext_password_backend)
		f=hgvsprintf(f, "ext_password_backend=%s\n",
			config->ext_password_backend);
	if (config->p2p_go_max_inactivity != DEFAULT_P2P_GO_MAX_INACTIVITY)
		f=hgvsprintf(f, "p2p_go_max_inactivity=%d\n",
			config->p2p_go_max_inactivity);
	if (config->auto_interworking)
		f=hgvsprintf(f, "auto_interworking=%d\n",
			config->auto_interworking);
	if (config->okc)
		f=hgvsprintf(f, "okc=%d\n", config->okc);
	if (config->pmf)
		f=hgvsprintf(f, "pmf=%d\n", config->pmf);
	if (config->dtim_period)
		f=hgvsprintf(f, "dtim_period=%d\n", config->dtim_period);
	if (config->beacon_int)
		f=hgvsprintf(f, "beacon_int=%d\n", config->beacon_int);

	if (config->sae_groups) {
		int i;
		f=hgvsprintf(f, "sae_groups=");
		for (i = 0; config->sae_groups[i] > 0; i++) {
			f=hgvsprintf(f, "%s%d", i > 0 ? " " : "",
				config->sae_groups[i]);
		}
		f=hgvsprintf(f, "\n");
	}

	if (config->sae_pwe)
		f=hgvsprintf(f, "sae_pwe=%d\n", config->sae_pwe);

	if (config->sae_pmkid_in_assoc)
		f=hgvsprintf(f, "sae_pmkid_in_assoc=%d\n",
			config->sae_pmkid_in_assoc);

	if (config->ap_vendor_elements) {
		int i, len = wpabuf_len(config->ap_vendor_elements);
		const u8 *p = wpabuf_head_u8(config->ap_vendor_elements);
		if (len > 0) {
			f=hgvsprintf(f, "ap_vendor_elements=");
			for (i = 0; i < len; i++)
				f=hgvsprintf(f, "%02x", *p++);
			f=hgvsprintf(f, "\n");
		}
	}

	if (config->ignore_old_scan_res)
		f=hgvsprintf(f, "ignore_old_scan_res=%d\n",
			config->ignore_old_scan_res);

	if (config->freq_list && config->freq_list[0]) {
		int i;
		f=hgvsprintf(f, "freq_list=");
		for (i = 0; config->freq_list[i]; i++) {
			f=hgvsprintf(f, "%s%d", i > 0 ? " " : "",
				config->freq_list[i]);
		}
		f=hgvsprintf(f, "\n");
	}
	if (config->initial_freq_list && config->initial_freq_list[0]) {
		int i;
		f=hgvsprintf(f, "initial_freq_list=");
		for (i = 0; config->initial_freq_list[i]; i++) {
			f=hgvsprintf(f, "%s%d", i > 0 ? " " : "",
				config->initial_freq_list[i]);
		}
		f=hgvsprintf(f, "\n");
	}
	if (config->scan_cur_freq != DEFAULT_SCAN_CUR_FREQ)
		f=hgvsprintf(f, "scan_cur_freq=%d\n", config->scan_cur_freq);

	if (config->scan_res_valid_for_connect !=
	    DEFAULT_SCAN_RES_VALID_FOR_CONNECT)
		f=hgvsprintf(f, "scan_res_valid_for_connect=%d\n",
			config->scan_res_valid_for_connect);

	if (config->sched_scan_interval)
		f=hgvsprintf(f, "sched_scan_interval=%u\n",
			config->sched_scan_interval);

	if (config->sched_scan_start_delay)
		f=hgvsprintf(f, "sched_scan_start_delay=%u\n",
			config->sched_scan_start_delay);

	if (config->external_sim)
		f=hgvsprintf(f, "external_sim=%d\n", config->external_sim);

	if (config->tdls_external_control)
		f=hgvsprintf(f, "tdls_external_control=%d\n",
			config->tdls_external_control);

	if (config->wowlan_triggers)
		f=hgvsprintf(f, "wowlan_triggers=%s\n",
			config->wowlan_triggers);

	if (config->bgscan)
		f=hgvsprintf(f, "bgscan=\"%s\"\n", config->bgscan);

	if (config->autoscan)
		f=hgvsprintf(f, "autoscan=%s\n", config->autoscan);

	if (config->p2p_search_delay != DEFAULT_P2P_SEARCH_DELAY)
		f=hgvsprintf(f, "p2p_search_delay=%u\n",
			config->p2p_search_delay);

	if (config->mac_addr)
		f=hgvsprintf(f, "mac_addr=%d\n", config->mac_addr);

	if (config->rand_addr_lifetime != DEFAULT_RAND_ADDR_LIFETIME)
		f=hgvsprintf(f, "rand_addr_lifetime=%u\n",
			config->rand_addr_lifetime);

	if (config->preassoc_mac_addr)
		f=hgvsprintf(f, "preassoc_mac_addr=%d\n", config->preassoc_mac_addr);

	if (config->key_mgmt_offload != DEFAULT_KEY_MGMT_OFFLOAD)
		f=hgvsprintf(f, "key_mgmt_offload=%d\n", config->key_mgmt_offload);

	if (config->user_mpm != DEFAULT_USER_MPM)
		f=hgvsprintf(f, "user_mpm=%d\n", config->user_mpm);

	if (config->max_peer_links != DEFAULT_MAX_PEER_LINKS)
		f=hgvsprintf(f, "max_peer_links=%d\n", config->max_peer_links);

	if (config->cert_in_cb != DEFAULT_CERT_IN_CB)
		f=hgvsprintf(f, "cert_in_cb=%d\n", config->cert_in_cb);

	if (config->mesh_max_inactivity != DEFAULT_MESH_MAX_INACTIVITY)
		f=hgvsprintf(f, "mesh_max_inactivity=%d\n",
			config->mesh_max_inactivity);

	if (config->dot11RSNASAERetransPeriod !=
	    DEFAULT_DOT11_RSNA_SAE_RETRANS_PERIOD)
		f=hgvsprintf(f, "dot11RSNASAERetransPeriod=%d\n",
			config->dot11RSNASAERetransPeriod);

	if (config->passive_scan)
		f=hgvsprintf(f, "passive_scan=%d\n", config->passive_scan);

	if (config->reassoc_same_bss_optim)
		f=hgvsprintf(f, "reassoc_same_bss_optim=%d\n",
			config->reassoc_same_bss_optim);

	if (config->wps_priority)
		f=hgvsprintf(f, "wps_priority=%d\n", config->wps_priority);

	if (config->wpa_rsc_relaxation != DEFAULT_WPA_RSC_RELAXATION)
		f=hgvsprintf(f, "wpa_rsc_relaxation=%d\n",
			config->wpa_rsc_relaxation);

	if (config->sched_scan_plans)
		f=hgvsprintf(f, "sched_scan_plans=%s\n", config->sched_scan_plans);

#ifdef CONFIG_MBO
	if (config->non_pref_chan)
		f=hgvsprintf(f, "non_pref_chan=%s\n", config->non_pref_chan);
	if (config->mbo_cell_capa != DEFAULT_MBO_CELL_CAPA)
		f=hgvsprintf(f, "mbo_cell_capa=%u\n", config->mbo_cell_capa);
	if (config->disassoc_imminent_rssi_threshold !=
	    DEFAULT_DISASSOC_IMMINENT_RSSI_THRESHOLD)
		f=hgvsprintf(f, "disassoc_imminent_rssi_threshold=%d\n",
			config->disassoc_imminent_rssi_threshold);
	if (config->oce != DEFAULT_OCE_SUPPORT)
		f=hgvsprintf(f, "oce=%u\n", config->oce);
#endif /* CONFIG_MBO */

	if (config->gas_address3)
		f=hgvsprintf(f, "gas_address3=%d\n", config->gas_address3);

	if (config->ftm_responder)
		f=hgvsprintf(f, "ftm_responder=%d\n", config->ftm_responder);
	if (config->ftm_initiator)
		f=hgvsprintf(f, "ftm_initiator=%d\n", config->ftm_initiator);

	if (config->osu_dir)
		f=hgvsprintf(f, "osu_dir=%s\n", config->osu_dir);

	if (config->fst_group_id)
		f=hgvsprintf(f, "fst_group_id=%s\n", config->fst_group_id);
	if (config->fst_priority)
		f=hgvsprintf(f, "fst_priority=%d\n", config->fst_priority);
	if (config->fst_llt)
		f=hgvsprintf(f, "fst_llt=%d\n", config->fst_llt);

	if (config->gas_rand_addr_lifetime != DEFAULT_RAND_ADDR_LIFETIME)
		f=hgvsprintf(f, "gas_rand_addr_lifetime=%u\n",
			config->gas_rand_addr_lifetime);
	if (config->gas_rand_mac_addr)
		f=hgvsprintf(f, "gas_rand_mac_addr=%d\n", config->gas_rand_mac_addr);
	if (config->dpp_config_processing)
		f=hgvsprintf(f, "dpp_config_processing=%d\n",
			config->dpp_config_processing);
	if (config->coloc_intf_reporting)
		f=hgvsprintf(f, "coloc_intf_reporting=%d\n",
			config->coloc_intf_reporting);
	if (config->p2p_device_random_mac_addr)
		f=hgvsprintf(f, "p2p_device_random_mac_addr=%d\n",
			config->p2p_device_random_mac_addr);
	if (!is_zero_ether_addr(config->p2p_device_persistent_mac_addr))
		f=hgvsprintf(f, "p2p_device_persistent_mac_addr=" MACSTR "\n",
			MAC2STR(config->p2p_device_persistent_mac_addr));
	if (config->p2p_interface_random_mac_addr)
		f=hgvsprintf(f, "p2p_interface_random_mac_addr=%d\n",
			config->p2p_interface_random_mac_addr);
	if (config->disable_btm)
		f=hgvsprintf(f, "disable_btm=1\n");
	if (config->extended_key_id != DEFAULT_EXTENDED_KEY_ID)
		f=hgvsprintf(f, "extended_key_id=%d\n",
			config->extended_key_id);
	if (config->wowlan_disconnect_on_deinit)
		f=hgvsprintf(f, "wowlan_disconnect_on_deinit=%d\n",
			config->wowlan_disconnect_on_deinit);
    return f;
}

#endif /* CONFIG_NO_CONFIG_WRITE */


int wpa_config_write(const char *name, struct wpa_config *config)
{
#ifndef CONFIG_NO_CONFIG_WRITE
	struct wpa_ssid *ssid;
	struct wpa_cred *cred;
#ifndef CONFIG_NO_CONFIG_BLOBS
	struct wpa_config_blob *blob;
#endif /* CONFIG_NO_CONFIG_BLOBS */
    wpa_config_buff = os_malloc(CONFIG_BUFF_SIZE); /*porting*/
    ASSERT(wpa_config_buff);
    char *f = wpa_config_buff;

    f[CONFIG_BUFF_SIZE-1] = 0;    

	wpa_printf(MSG_DEBUG, "Writing configuration file '%s'", name);

	f = wpa_config_write_global(f, config);

	for (cred = config->cred; cred; cred = cred->next) {
		if (cred->temporary)
			continue;
		f=hgvsprintf(f, "\ncred={\n");
		wpa_config_write_cred(f, cred);
		f=hgvsprintf(f, "}\n");
	}

	for (ssid = config->ssid; ssid; ssid = ssid->next) {
		if (ssid->key_mgmt == WPA_KEY_MGMT_WPS || ssid->temporary)
			continue; /* do not save temporary networks */
		if (wpa_key_mgmt_wpa_psk_no_sae(ssid->key_mgmt) &&
		    !ssid->psk_set && !ssid->passphrase)
			continue; /* do not save invalid network */
		if (wpa_key_mgmt_sae(ssid->key_mgmt) &&
		    !ssid->passphrase && !ssid->sae_password)
			continue; /* do not save invalid network */
		f=hgvsprintf(f, "\nnetwork={\n");
		wpa_config_write_network(f, ssid);
		f=hgvsprintf(f, "}\n");
	}

#ifndef CONFIG_NO_CONFIG_BLOBS
	for (blob = config->blobs; blob; blob = blob->next) {
		char *f1 = wpa_config_write_blob(f, blob);
		if (f1 == f)
			break;
        f = f1;
	}
#endif /* CONFIG_NO_CONFIG_BLOBS */

    umac_config_write(name, wpa_config_buff, f-wpa_config_buff);
    os_free(wpa_config_buff);
    wpa_config_buff = NULL;
	wpa_printf(MSG_DEBUG, "Configuration file written successfully");
	return 0;
#else /* CONFIG_NO_CONFIG_WRITE */
	return -1;
#endif /* CONFIG_NO_CONFIG_WRITE */
}
