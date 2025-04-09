/*
 * WPA Supplicant - RSN PMKSA cache
 * Copyright (c) 2004-2009, 2011-2015, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

//#include "includes.h"

#include "atbm_hal.h"

static const int pmksa_cache_max_entries = 32;

struct rsn_pmksa_cache {
	struct rsn_pmksa_cache_entry *pmksa; /* PMKSA cache */
	int pmksa_count; /* number of entries in PMKSA cache */
	struct atbmwifi_wpa_sm *sm; /* TODO: get rid of this reference(?) */
	unsigned int dot11RSNAConfigPMKLifetime;

	void (*free_cb)(struct rsn_pmksa_cache_entry *entry, void *ctx,
			enum pmksa_free_reason reason);
	void *ctx;
};


static void pmksa_cache_set_expiration(struct rsn_pmksa_cache *pmksa);


static void _pmksa_cache_free_entry(struct rsn_pmksa_cache_entry *entry)
{
	bin_clear_free(entry, sizeof(*entry));
}


static void pmksa_cache_free_entry(struct rsn_pmksa_cache *pmksa,
				   struct rsn_pmksa_cache_entry *entry,
				   enum pmksa_free_reason reason)
{
	pmksa->pmksa_count--;
	pmksa->free_cb(entry, pmksa->ctx, reason);
	_pmksa_cache_free_entry(entry);
}


static void pmksa_cache_expire(void *eloop_ctx, void *timeout_ctx)
{
	struct rsn_pmksa_cache *pmksa = eloop_ctx;
	atbm_uint32 now;

	now = atbm_GetOsTime();
	while (pmksa->pmksa && pmksa->pmksa->expiration <= now) {
		struct rsn_pmksa_cache_entry *entry = pmksa->pmksa;
		pmksa->pmksa = entry->next;
		wpa_printf(MSG_INFO, "RSN: expired PMKSA cache entry for "
			   MACSTR, MAC2STR(entry->aa));
		pmksa_cache_free_entry(pmksa, entry, PMKSA_EXPIRE);
	}

	pmksa_cache_set_expiration(pmksa);
}

static void pmksa_cache_set_expiration(struct rsn_pmksa_cache *pmksa)
{
	int msec;
	struct rsn_pmksa_cache_entry *entry;
	atbm_uint32 now;

	atbmwifi_eloop_cancel_timeout(pmksa_cache_expire, pmksa, ATBM_NULL);
	if (pmksa->pmksa == ATBM_NULL)
		return;
	now = atbm_GetOsTime();
	msec = pmksa->pmksa->expiration - now;
	if (msec < 0)
		msec = 0;

	atbmwifi_eloop_register_timeout(1, msec, pmksa_cache_expire, pmksa, ATBM_NULL);

	//entry = pmksa->sm->cur_pmksa ? pmksa->sm->cur_pmksa :
	//pmksa_cache_get(pmksa, pmksa->sm->bssid, ATBM_NULL, ATBM_NULL, 0);
}


/**
 * pmksa_cache_add - Add a PMKSA cache entry
 * @pmksa: Pointer to PMKSA cache data from pmksa_cache_init()
 * @pmk: The new pairwise master key
 * @pmk_len: PMK length in bytes, usually PMK_LEN (32)
 * @pmkid: Calculated PMKID
 * @kck: Key confirmation key or %ATBM_NULL if not yet derived
 * @kck_len: KCK length in bytes
 * @aa: Authenticator address
 * @spa: Supplicant address
 * @network_ctx: Network configuration context for this PMK
 * @akmp: WPA_KEY_MGMT_* used in key derivation
 * @cache_id: Pointer to FILS Cache Identifier or %ATBM_NULL if not advertised
 * Returns: Pointer to the added PMKSA cache entry or %ATBM_NULL on error
 *
 * This function create a PMKSA entry for a new PMK and adds it to the PMKSA
 * cache. If an old entry is already in the cache for the same Authenticator,
 * this entry will be replaced with the new entry. PMKID will be calculated
 * based on the PMK and the driver interface is notified of the new PMKID.
 */
struct rsn_pmksa_cache_entry *
pmksa_cache_add(struct rsn_pmksa_cache *pmksa, const atbm_uint8 *pmk, atbm_size_t pmk_len,
		const atbm_uint8 *pmkid, const atbm_uint8 *kck, atbm_size_t kck_len,
		const atbm_uint8 *aa, const atbm_uint8 *spa, void *network_ctx, int akmp,
		const atbm_uint8 *cache_id)
{
	struct rsn_pmksa_cache_entry *entry;
	atbm_uint32 now;

	if (pmk_len > ATBM_PMK_LEN_MAX)
		return ATBM_NULL;

	entry = atbm_kzalloc(sizeof(*entry), GFP_KERNEL);
	if (entry == ATBM_NULL)
		return ATBM_NULL;
	atbm_memcpy(entry->pmk, pmk, pmk_len);
	entry->pmk_len = pmk_len;
	if (pmkid)
		atbm_memcpy(entry->pmkid, pmkid, ATBM_PMKID_LEN);
	else
		return ATBM_NULL;
	now = atbm_GetOsTime();
	entry->expiration = now + pmksa->dot11RSNAConfigPMKLifetime;

	entry->akmp = akmp;

	atbm_memcpy(entry->aa, aa, ATBM_ETH_ALEN);
	entry->network_ctx = network_ctx;

	return pmksa_cache_add_entry(pmksa, entry);
}

struct rsn_pmksa_cache_entry *
pmksa_cache_add_entry(struct rsn_pmksa_cache *pmksa,
		      struct rsn_pmksa_cache_entry *entry)
{
	struct rsn_pmksa_cache_entry *pos, *prev;
	/* Replace an old entry for the same Authenticator (if found) with the
	 * new entry */
	pos = pmksa->pmksa;
	prev = ATBM_NULL;
	while (pos) {
		if (atbm_memcmp(entry->aa, pos->aa, ATBM_ETH_ALEN) == 0) {
			if (pos->pmk_len == entry->pmk_len &&
			    atbm_memcmp(pos->pmk, entry->pmk,
					    entry->pmk_len) == 0 &&
			    atbm_memcmp(pos->pmkid, entry->pmkid,
					    ATBM_PMKID_LEN) == 0) {
				wpa_printf(MSG_DEBUG, "WPA: reusing previous "
					   "PMKSA entry");
				atbm_kfree(entry);
				return pos;
			}
			if (prev == ATBM_NULL)
				pmksa->pmksa = pos->next;
			else
				prev->next = pos->next;

			/*
			 * If OKC is used, there may be other PMKSA cache
			 * entries based on the same PMK. These needs to be
			 * flushed so that a new entry can be created based on
			 * the new PMK. Only clear other entries if they have a
			 * matching PMK and this PMK has been used successfully
			 * with the current AP, i.e., if opportunistic flag has
			 * been cleared in wpa_supplicant_key_neg_complete().
			 */
			wpa_printf(MSG_DEBUG, "RSN: Replace PMKSA entry for "
				   "the current AP and any PMKSA cache entry "
				   "that was based on the old PMK");
			if (!pos->opportunistic)
				pmksa_cache_flush(pmksa, entry->network_ctx,
						  pos->pmk, pos->pmk_len);
			pmksa_cache_free_entry(pmksa, pos, PMKSA_REPLACE);
			break;
		}
		prev = pos;
		pos = pos->next;
	}

	if (pmksa->pmksa_count >= pmksa_cache_max_entries && pmksa->pmksa) {
		/* Remove the oldest entry to make room for the new entry */
		pos = pmksa->pmksa;

		if (pos == pmksa->sm->cur_pmksa) {
			/*
			 * Never remove the current PMKSA cache entry, since
			 * it's in use, and removing it triggers a needless
			 * deauthentication.
			 */
			pos = pos->next;
			pmksa->pmksa->next = pos ? pos->next : ATBM_NULL;
		} else
			pmksa->pmksa = pos->next;

		if (pos) {
			wpa_printf(MSG_DEBUG, "RSN: removed the oldest idle "
				   "PMKSA cache entry (for " MACSTR ") to "
				   "make room for new one",
				   MAC2STR(pos->aa));
			pmksa_cache_free_entry(pmksa, pos, PMKSA_FREE);
		}
	}

	/* Add the new entry; order by expiration time */
	pos = pmksa->pmksa;
	prev = ATBM_NULL;
	while (pos) {
		if (pos->expiration > entry->expiration)
			break;
		prev = pos;
		pos = pos->next;
	}
	if (prev == ATBM_NULL) {
		entry->next = pmksa->pmksa;
		pmksa->pmksa = entry;
		pmksa_cache_set_expiration(pmksa);
	} else {
		entry->next = prev->next;
		prev->next = entry;
	}
	pmksa->pmksa_count++;
	wpa_printf(MSG_DEBUG, "RSN: Added PMKSA cache entry for " MACSTR
		   " network_ctx=%p akmp=0x%x", MAC2STR(entry->aa),
		   entry->network_ctx, entry->akmp);

	return entry;
}

/**
 * pmksa_cache_flush - Flush PMKSA cache entries for a specific network
 * @pmksa: Pointer to PMKSA cache data from pmksa_cache_init()
 * @network_ctx: Network configuration context or %ATBM_NULL to flush all entries
 * @pmk: PMK to match for or %NYLL to match all PMKs
 * @pmk_len: PMK length
 */
void pmksa_cache_flush(struct rsn_pmksa_cache *pmksa, void *network_ctx,
		       const atbm_uint8 *pmk, atbm_size_t pmk_len)
{
	struct rsn_pmksa_cache_entry *entry, *prev = ATBM_NULL, *tmp;
	int removed = 0;

	entry = pmksa->pmksa;
	while (entry) {
		if ((entry->network_ctx == network_ctx ||
		     network_ctx == ATBM_NULL) &&
		    (pmk == ATBM_NULL ||
		     (pmk_len == entry->pmk_len &&
		      atbm_memcmp(pmk, entry->pmk, pmk_len) == 0))) {
			wpa_printf(MSG_DEBUG, "RSN: Flush PMKSA cache entry "
				   "for " MACSTR, MAC2STR(entry->aa));
			if (prev)
				prev->next = entry->next;
			else
				pmksa->pmksa = entry->next;
			tmp = entry;
			entry = entry->next;
			pmksa_cache_free_entry(pmksa, tmp, PMKSA_FREE);
			removed++;
		} else {
			prev = entry;
			entry = entry->next;
		}
	}
	if (removed)
		pmksa_cache_set_expiration(pmksa);
}


/**
 * pmksa_cache_deinit - Free all entries in PMKSA cache
 * @pmksa: Pointer to PMKSA cache data from pmksa_cache_init()
 */
void pmksa_cache_deinit(struct rsn_pmksa_cache *pmksa)
{
	struct rsn_pmksa_cache_entry *entry, *prev;

	if (pmksa == ATBM_NULL)
		return;

	entry = pmksa->pmksa;
	pmksa->pmksa = ATBM_NULL;
	while (entry) {
		prev = entry;
		entry = entry->next;
		atbm_kfree(prev);
	}
	pmksa_cache_set_expiration(pmksa);
	atbm_kfree(pmksa);
}


/**
 * pmksa_cache_get - Fetch a PMKSA cache entry
 * @pmksa: Pointer to PMKSA cache data from pmksa_cache_init()
 * @aa: Authenticator address or %ATBM_NULL to match any
 * @pmkid: PMKID or %ATBM_NULL to match any
 * @network_ctx: Network context or %ATBM_NULL to match any
 * @akmp: Specific AKMP to search for or 0 for any
 * Returns: Pointer to PMKSA cache entry or %ATBM_NULL if no match was found
 */
struct rsn_pmksa_cache_entry * pmksa_cache_get(struct rsn_pmksa_cache *pmksa,
					       const atbm_uint8 *aa, const atbm_uint8 *pmkid,
					       const void *network_ctx,
					       int akmp)
{
	struct rsn_pmksa_cache_entry *entry = pmksa->pmksa;
	while (entry) {
		if ((aa == ATBM_NULL || atbm_memcmp(entry->aa, aa, ATBM_ETH_ALEN) == 0) &&
		    (pmkid == ATBM_NULL ||
		     atbm_memcmp(entry->pmkid, pmkid, ATBM_PMKID_LEN) == 0) &&
		    (!akmp || akmp == entry->akmp) &&
		    (network_ctx == ATBM_NULL || network_ctx == entry->network_ctx))
			return entry;
		entry = entry->next;
	}
	return ATBM_NULL;
}


static struct rsn_pmksa_cache_entry *
pmksa_cache_clone_entry(struct rsn_pmksa_cache *pmksa,
			const struct rsn_pmksa_cache_entry *old_entry,
			const atbm_uint8 *aa)
{
	struct rsn_pmksa_cache_entry *new_entry;
	os_time_t old_expiration = old_entry->expiration;

	new_entry = pmksa_cache_add(pmksa, old_entry->pmk, old_entry->pmk_len,
				    ATBM_NULL, ATBM_NULL, 0,
				    aa, pmksa->sm->own_addr,
				    old_entry->network_ctx, old_entry->akmp,
				    old_entry->fils_cache_id_set ?
				    old_entry->fils_cache_id : ATBM_NULL);
	if (new_entry == ATBM_NULL)
		return ATBM_NULL;

	/* TODO: reorder entries based on expiration time? */
	new_entry->expiration = old_expiration;
	new_entry->opportunistic = 1;

	return new_entry;
}


/**
 * pmksa_cache_get_opportunistic - Try to get an opportunistic PMKSA entry
 * @pmksa: Pointer to PMKSA cache data from pmksa_cache_init()
 * @network_ctx: Network configuration context
 * @aa: Authenticator address for the new AP
 * @akmp: Specific AKMP to search for or 0 for any
 * Returns: Pointer to a new PMKSA cache entry or %ATBM_NULL if not available
 *
 * Try to create a new PMKSA cache entry opportunistically by guessing that the
 * new AP is sharing the same PMK as another AP that has the same SSID and has
 * already an entry in PMKSA cache.
 */
struct rsn_pmksa_cache_entry *
pmksa_cache_get_opportunistic(struct rsn_pmksa_cache *pmksa, void *network_ctx,
			      const atbm_uint8 *aa, int akmp)
{
	struct rsn_pmksa_cache_entry *entry = pmksa->pmksa;

	wpa_printf(MSG_DEBUG, "RSN: Consider " MACSTR " for OKC", MAC2STR(aa));
	if (network_ctx == ATBM_NULL)
		return ATBM_NULL;
	while (entry) {
		if (entry->network_ctx == network_ctx &&
		    (!akmp || entry->akmp == akmp)) {
			entry = pmksa_cache_clone_entry(pmksa, entry, aa);
			if (entry) {
				wpa_printf(MSG_DEBUG, "RSN: added "
					   "opportunistic PMKSA cache entry "
					   "for " MACSTR, MAC2STR(aa));
			}
			return entry;
		}
		entry = entry->next;
	}
	return ATBM_NULL;
}

/**
 * pmksa_cache_get_current - Get the current used PMKSA entry
 * @sm: Pointer to WPA state machine data from wpa_sm_init()
 * Returns: Pointer to the current PMKSA cache entry or %ATBM_NULL if not available
 */
struct rsn_pmksa_cache_entry * pmksa_cache_get_current(struct atbmwifi_wpa_sm *sm)
{
	if (sm == ATBM_NULL)
		return ATBM_NULL;
	return sm->cur_pmksa;
}


/**
 * pmksa_cache_clear_current - Clear the current PMKSA entry selection
 * @sm: Pointer to WPA state machine data from wpa_sm_init()
 */
void pmksa_cache_clear_current(struct atbmwifi_wpa_sm *sm)
{
	if (sm == ATBM_NULL)
		return;
	sm->cur_pmksa = ATBM_NULL;
}


/**
 * pmksa_cache_set_current - Set the current PMKSA entry selection
 * @sm: Pointer to WPA state machine data from wpa_sm_init()
 * @pmkid: PMKID for selecting PMKSA or %ATBM_NULL if not used
 * @bssid: BSSID for PMKSA or %ATBM_NULL if not used
 * @network_ctx: Network configuration context
 * @try_opportunistic: Whether to allow opportunistic PMKSA caching
 * @fils_cache_id: Pointer to FILS Cache Identifier or %ATBM_NULL if not used
 * Returns: 0 if PMKSA was found or -1 if no matching entry was found
 */
int pmksa_cache_set_current(struct atbmwifi_wpa_sm *sm, const atbm_uint8 *pmkid,
			    const atbm_uint8 *bssid, void *network_ctx,
			    int try_opportunistic, const atbm_uint8 *fils_cache_id,
			    int akmp)
{
	struct rsn_pmksa_cache *pmksa = sm->pmksa;
	wpa_printf(MSG_DEBUG, "RSN: PMKSA cache search - network_ctx=%p "
		   "try_opportunistic=%d akmp=0x%x",
		   network_ctx, try_opportunistic, akmp);
	if (pmkid)
		wpa_hexdump(MSG_DEBUG, "RSN: Search for PMKID",
			    pmkid, ATBM_PMKID_LEN);
	if (bssid)
		wpa_printf(MSG_DEBUG, "RSN: Search for BSSID " MACSTR,
			   MAC2STR(bssid));
	if (fils_cache_id)
		wpa_printf(MSG_DEBUG,
			   "RSN: Search for FILS Cache Identifier %02x%02x",
			   fils_cache_id[0], fils_cache_id[1]);

	sm->cur_pmksa = ATBM_NULL;
	if (pmkid)
		sm->cur_pmksa = pmksa_cache_get(pmksa, ATBM_NULL, pmkid,
						network_ctx, akmp);
	if (sm->cur_pmksa == ATBM_NULL && bssid)
		sm->cur_pmksa = pmksa_cache_get(pmksa, bssid, ATBM_NULL,
						network_ctx, akmp);
	if (sm->cur_pmksa == ATBM_NULL && try_opportunistic && bssid)
		sm->cur_pmksa = pmksa_cache_get_opportunistic(pmksa,
							      network_ctx,
							      bssid, akmp);
	if (sm->cur_pmksa) {
		wpa_hexdump(MSG_DEBUG, "RSN: PMKSA cache entry found - PMKID",
			    sm->cur_pmksa->pmkid, ATBM_PMKID_LEN);
		return 0;
	}
	wpa_printf(MSG_DEBUG, "RSN: No PMKSA cache entry found");
	return -1;
}

static inline int os_snprintf_error(atbm_size_t size, int res)
{
	return res < 0 || (unsigned int) res >= size;
}

/**
 * pmksa_cache_list - Dump text list of entries in PMKSA cache
 * @pmksa: Pointer to PMKSA cache data from pmksa_cache_init()
 * @buf: Buffer for the list
 * @len: Length of the buffer
 * Returns: number of bytes written to buffer
 *
 * This function is used to generate a text format representation of the
 * current PMKSA cache contents for the ctrl_iface PMKSA command.
 */
int pmksa_cache_list(struct rsn_pmksa_cache *pmksa, char *buf, atbm_size_t len)
{
	int i, ret;
	char *pos = buf;
	struct rsn_pmksa_cache_entry *entry;
	atbm_uint32 now;
	int cache_id_used = 0;

	for (entry = pmksa->pmksa; entry; entry = entry->next) {
		if (entry->fils_cache_id_set) {
			cache_id_used = 1;
			break;
		}
	}

	now = atbm_GetOsTime();
	ret = snprintf(pos, buf + len - pos,
			  "Index / AA / PMKID / expiration (in seconds) / "
			  "opportunistic%s\n",
			  cache_id_used ? " / FILS Cache Identifier" : "");
	if (os_snprintf_error(buf + len - pos, ret))
		return pos - buf;
	pos += ret;
	i = 0;
	entry = pmksa->pmksa;
	while (entry) {
		i++;
		ret = snprintf(pos, buf + len - pos, "%d " MACSTR " ",
				  i, MAC2STR(entry->aa));
		if (os_snprintf_error(buf + len - pos, ret))
			return pos - buf;
		pos += ret;
		pos += atbmwifi_wpa_snprintf_hex(pos, buf + len - pos, entry->pmkid,
					ATBM_PMKID_LEN);
		ret = snprintf(pos, buf + len - pos, " %d %d",
				  (int) (entry->expiration - now),
				  entry->opportunistic);
		if (os_snprintf_error(buf + len - pos, ret))
			return pos - buf;
		pos += ret;
		if (entry->fils_cache_id_set) {
			ret = snprintf(pos, buf + len - pos, " %02x%02x",
					  entry->fils_cache_id[0],
					  entry->fils_cache_id[1]);
			if (os_snprintf_error(buf + len - pos, ret))
				return pos - buf;
			pos += ret;
		}
		ret = snprintf(pos, buf + len - pos, "\n");
		if (os_snprintf_error(buf + len - pos, ret))
			return pos - buf;
		pos += ret;
		entry = entry->next;
	}
	return pos - buf;
}


struct rsn_pmksa_cache_entry * pmksa_cache_head(struct rsn_pmksa_cache *pmksa)
{
	return pmksa->pmksa;
}


/**
 * pmksa_cache_init - Initialize PMKSA cache
 * @free_cb: Callback function to be called when a PMKSA cache entry is freed
 * @ctx: Context pointer for free_cb function
 * @sm: Pointer to WPA state machine data from wpa_sm_init()
 * Returns: Pointer to PMKSA cache data or %ATBM_NULL on failure
 */
struct rsn_pmksa_cache *
pmksa_cache_init(void (*free_cb)(struct rsn_pmksa_cache_entry *entry,
				 void *ctx, enum pmksa_free_reason reason),
		 void *ctx, struct atbmwifi_wpa_sm *sm)
{
	struct rsn_pmksa_cache *pmksa;

	pmksa = atbm_kzalloc(sizeof(*pmksa), GFP_KERNEL);
	if (pmksa) {
		pmksa->free_cb = free_cb;
		pmksa->ctx = ctx;
		pmksa->dot11RSNAConfigPMKLifetime = 24*3600*1000; /*24h*/
		pmksa->sm = sm;
	}

	return pmksa;
}

