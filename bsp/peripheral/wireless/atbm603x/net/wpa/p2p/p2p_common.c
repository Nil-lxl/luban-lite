
#include "p2p_common.h"
#include "p2p_defs.h"
#include "p2p_list.h"
#include "atbm_hal.h"

#if CONFIG_P2P

void printf_encode(char *txt, atbm_size_t maxlen, const atbm_uint8 *data, atbm_size_t len)
{
	char *end = txt + maxlen;
	atbm_size_t i;

	for (i = 0; i < len; i++) {
		if (txt + 4 >= end)
			break;

		switch (data[i]) {
		case '\"':
			*txt++ = '\\';
			*txt++ = '\"';
			break;
		case '\\':
			*txt++ = '\\';
			*txt++ = '\\';
			break;
		case '\033':
			*txt++ = '\\';
			*txt++ = 'e';
			break;
		case '\n':
			*txt++ = '\\';
			*txt++ = 'n';
			break;
		case '\r':
			*txt++ = '\\';
			*txt++ = 'r';
			break;
		case '\t':
			*txt++ = '\\';
			*txt++ = 't';
			break;
		default:
			if (data[i] >= 32 && data[i] <= 126) {
				*txt++ = data[i];
			} else {
				txt += os_snprintf(txt, end - txt, "\\x%02x",
						   data[i]);
			}
			break;
		}
	}

	*txt = '\0';
}


const char * wpa_ssid_txt(const atbm_uint8 *ssid, atbm_size_t ssid_len)
{
	static char ssid_txt[32 * 4 + 1];

	if (ssid == NULL) {
		ssid_txt[0] = '\0';
		return ssid_txt;
	}

	printf_encode(ssid_txt, sizeof(ssid_txt), ssid, ssid_len);
	return ssid_txt;
}


atbm_size_t os_strlcpy(char *dest, const char *src, atbm_size_t siz)
{
	const char *s = src;
	atbm_size_t left = siz;

	if (left) {
		/* Copy string up to the maximum size of the dest buffer */
		while (--left != 0) {
			if ((*dest++ = *s++) == '\0')
				break;
		}
	}

	if (left == 0) {
		/* Not enough room for the string; force NUL-termination */
		if (siz != 0)
			*dest = '\0';
		while (*s++)
			; /* determine total src string length */
	}

	return s - src - 1;
}

void *os_realloc_array(void *ptr, atbm_size_t nmemb, atbm_size_t size)
{
	if (size && nmemb > (~(atbm_size_t) 0) / size)
		return NULL;

	return (void *)atbm_kzalloc(nmemb * size, GFP_KERNEL);
}

void * os_memdup(const void *src, atbm_size_t len)
{
	void *r = (void *)atbm_kmalloc(len, GFP_KERNEL);

	if (r && src)
		atbm_memcpy(r, src, len);
	return r;
}

void os_sleep(os_time_t sec, os_time_t usec)
{
	if (sec)
		atbm_SleepMs(sec);
	if (usec)
		atbm_SleepMs(usec/1000);
}


int os_get_reltime(struct os_reltime *t)
{
	t->sec = 0;
	t->usec = 0;
	return 0;
}


struct wpabuf * ieee802_11_vendor_ie_concat(const atbm_uint8 *ies, atbm_size_t ies_len,
					    atbm_uint32 oui_type)
{
	struct wpabuf *buf;
	const struct element *elem, *found = NULL;

	for_each_element_id(elem, ATBM_WLAN_EID_VENDOR_SPECIFIC, ies, ies_len) {
		if (elem->datalen >= 4 &&
		    ATBM_WPA_GET_BE32(elem->data) == oui_type) {
			found = elem;
			break;
		}
	}

	if (!found)
		return NULL; /* No specified vendor IE found */

	buf = wpabuf_alloc(ies_len);
	if (buf == NULL)
		return NULL;

	/*
	 * There may be multiple vendor IEs in the message, so need to
	 * concatenate their data fields.
	 */
	for_each_element_id(elem, ATBM_WLAN_EID_VENDOR_SPECIFIC, ies, ies_len) {
		if (elem->datalen >= 4 && ATBM_WPA_GET_BE32(elem->data) == oui_type)
			wpabuf_put_data(buf, elem->data + 4, elem->datalen - 4);
	}

	return buf;
}

int is_ctrl_char(char c)
{
	return c > 0 && c < 32;
}


int supp_rates_11b_only(struct atbmwifi_ieee802_11_elems *elems)
{
	int num_11b = 0, num_others = 0;
	int i;

	if (elems->supp_rates == NULL && elems->ext_supp_rates == NULL)
		return 0;

	for (i = 0; elems->supp_rates && i < elems->supp_rates_len; i++) {
		if (is_11b(elems->supp_rates[i]))
			num_11b++;
		else
			num_others++;
	}

	for (i = 0; elems->ext_supp_rates && i < elems->ext_supp_rates_len;
	     i++) {
		if (is_11b(elems->ext_supp_rates[i]))
			num_11b++;
		else
			num_others++;
	}

	return num_11b > 0 && num_others == 0;
}

int freq_range_list_includes(const struct wpa_freq_range_list *list,
			     unsigned int freq)
{
	unsigned int i;

	if (list == NULL)
		return 0;

	for (i = 0; i < list->num; i++) {
		if (freq >= list->range[i].min && freq <= list->range[i].max)
			return 1;
	}

	return 0;
}

atbm_size_t utf8_escape(const char *inp, atbm_size_t in_size,
		   char *outp, atbm_size_t out_size)
{
	atbm_size_t res_size = 0;

	if (!inp || !outp)
		return 0;

	/* inp may or may not be NUL terminated, but must be if 0 size
	 * is specified */
	if (!in_size)
		in_size = os_strlen(inp);

	while (in_size) {
		in_size--;
		if (res_size++ >= out_size)
			return 0;

		switch (*inp) {
		case '\\':
		case '\'':
			if (res_size++ >= out_size)
				return 0;
			*outp++ = '\\';
			/* fall through */

		default:
			*outp++ = *inp++;
			break;
		}
	}

	/* NUL terminate if space allows */
	if (res_size < out_size)
		*outp = '\0';

	return res_size;
}

#endif

