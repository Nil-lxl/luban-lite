
#ifndef P2P_COMMMON_H
#define P2P_COMMMON_H
#include "atbm_hal.h"

#include "stdlib.h"
#include "p2p_defs.h"
#include "p2p_debug.h"
#include "wpabuf.h"

struct os_reltime {
	os_time_t sec;
	os_time_t usec;
};
struct element {
	atbm_uint8 id;
	atbm_uint8 datalen;
	atbm_uint8 data[];
}atbm_packed;


#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"

#define os_snprintf snprintf
#define os_strlen strlen
#define os_memmove memmove
#define os_strncmp strncmp
#define os_strdup(s) strdup(s)
#define os_realloc realloc


typedef enum { ParseOK = 0, ParseUnknown = 1, ParseFailed = -1 } ParseRes;


/* element iteration helpers */
#define for_each_element(_elem, _data, _datalen)			\
	for (_elem = (const struct element *) (_data);			\
	     (const atbm_uint8 *) (_data) + (_datalen) - (const atbm_uint8 *) _elem >=	\
		(int) sizeof(*_elem) &&					\
	     (const atbm_uint8 *) (_data) + (_datalen) - (const atbm_uint8 *) _elem >=	\
		(int) sizeof(*_elem) + _elem->datalen;			\
	     _elem = (const struct element *) (_elem->data + _elem->datalen))

#define for_each_element_id(element, _id, data, datalen)		\
	for_each_element(element, data, datalen)			\
		if (element->id == (_id))

#define for_each_element_extid(element, extid, _data, _datalen)		\
	for_each_element(element, _data, _datalen)			\
		if (element->id == WLAN_EID_EXTENSION &&		\
		    element->datalen > 0 &&				\
		    element->data[0] == (extid))

#define for_each_subelement(sub, element)				\
	for_each_element(sub, (element)->data, (element)->datalen)

#define for_each_subelement_id(sub, id, element)			\
	for_each_element_id(sub, id, (element)->data, (element)->datalen)

#define for_each_subelement_extid(sub, extid, element)			\
	for_each_element_extid(sub, extid, (element)->data, (element)->datalen)


static inline int os_reltime_before(struct os_reltime *a,
				    struct os_reltime *b)
{
	return 0;
}


static inline int os_reltime_expired(struct os_reltime *now,
				     struct os_reltime *ts,
				     os_time_t timeout_secs)
{
	return 0;
}


static inline void *os_calloc(atbm_size_t nmemb, atbm_size_t size)
{
	if (size && nmemb > (~(atbm_size_t) 0) / size)
		return NULL;
	return (void *)atbm_kzalloc(nmemb * size, GFP_KERNEL);
}

static inline int os_snprintf_error(atbm_size_t size, int res)
{
	return res < 0 || (unsigned int) res >= size;
}



const char * wpa_ssid_txt(const atbm_uint8 *ssid, atbm_size_t ssid_len);

atbm_size_t os_strlcpy(char *dest, const char *src, atbm_size_t siz);
void * os_memdup(const void *src, atbm_size_t len);
void *os_realloc_array(void *ptr, atbm_size_t nmemb, atbm_size_t size);

struct wpabuf * ieee802_11_vendor_ie_concat(const atbm_uint8 *ies, atbm_size_t ies_len,
					    atbm_uint32 oui_type);
int is_ctrl_char(char c);
int supp_rates_11b_only(struct atbmwifi_ieee802_11_elems *elems);
int freq_range_list_includes(const struct wpa_freq_range_list *list,
			     unsigned int freq);
atbm_size_t utf8_escape(const char *inp, atbm_size_t in_size,
		   char *outp, atbm_size_t out_size);



#endif

