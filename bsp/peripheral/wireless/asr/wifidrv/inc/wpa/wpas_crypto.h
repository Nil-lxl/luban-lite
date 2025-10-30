#ifndef __WPAS_CRYPTO_H_
#define __WPAS_CRYPTO_H_

#include <stdint.h>
#include <stdbool.h>
#ifdef CONFIG_IEEE80211W
#include "uwifi_msg_rx.h"
#endif

#define ETHER_ADDRLEN                   6
#define PMK_EXPANSION_CONST             "Pairwise key expansion"
#define PMK_EXPANSION_CONST_SIZE        22
#define PMKID_NAME_CONST                "PMK Name"
#define PMKID_NAME_CONST_SIZE           8
#define GMK_EXPANSION_CONST             "Group key expansion"
#define GMK_EXPANSION_CONST_SIZE        19
#define RANDOM_EXPANSION_CONST          "Init Counter"
#define RANDOM_EXPANSION_CONST_SIZE     12
#define PTK_LEN_CCMP                    48

#define LargeIntegerOverflow(x) ((x.field.HighPart == 0xffffffff) && \
                                (x.field.LowPart == 0xffffffff))
#define LargeIntegerZero(x) memset(&x.charData, 0, 8);

#define Octet16IntegerOverflow(x) (LargeIntegerOverflow(x.field.HighPart) && \
                                   LargeIntegerOverflow(x.field.LowPart))
#define Octet16IntegerZero(x) memset(&x.charData, 0, 16);

#define SetNonce(ocDst, oc32Counter) set_eapol_key_iv(ocDst, oc32Counter)


int32_t password_hash (
    uint8_t *password,
    uint8_t *ssid,
    int16_t ssidlength,
    uint8_t *output);

void octet_to_large_int(OCTET_STRING f, LARGE_INTEGER * li);

int32_t replay_counter_not_larger(LARGE_INTEGER li1, OCTET_STRING f);

#ifdef CFG_SOFTAP_SUPPORT

int32_t replay_counter_equal(LARGE_INTEGER li1, OCTET_STRING f);

int32_t replay_counter_larger(LARGE_INTEGER li1, OCTET_STRING f);

void set_replay_counter(OCTET_STRING *f, uint32_t h, uint32_t l);

#endif //CFG_SOFTAP_SUPPORT

void inc_large_int(LARGE_INTEGER * x);

OCTET32_INTEGER *inc_oct32_int(OCTET32_INTEGER * x);

int32_t check_mic(OCTET_STRING EAPOLMsgRecvd, uint8_t *key, int32_t keylen);

void calc_mic(OCTET_STRING EAPOLMsgSend, int32_t algo, uint8_t *key, int32_t keylen);

void calc_gtk(uint8_t *addr, uint8_t *nonce,
                uint8_t *keyin, int32_t keyinlen,
                uint8_t *keyout, int32_t keyoutlen);

#ifdef CONFIG_IEEE80211W
void calc_ptk(uint8_t *addr1, uint8_t *addr2,
                uint8_t *nonce1, uint8_t *nonce2,
                uint8_t * keyin, int32_t keyinlen,
                uint8_t *keyout, int32_t keyoutlen, uint32_t keymgmt);
#else
void calc_ptk(uint8_t *addr1, uint8_t *addr2,
                uint8_t *nonce1, uint8_t *nonce2,
                uint8_t * keyin, int32_t keyinlen,
                uint8_t *keyout, int32_t keyoutlen);
#endif

int32_t dec_wpa2_key_data(WPA_STA_INFO* pStaInfo, uint8_t *key, int32_t *keylen, uint8_t *kek, int32_t keklen, uint8_t *kout);

int32_t dec_gtk(OCTET_STRING EAPOLMsgRecvd, uint8_t *kek, int32_t keklen, int32_t *keylen, uint8_t *kout);

void gen_nonce(uint8_t *nonce, uint8_t *addr);

#ifdef CFG_SOFTAP_SUPPORT
void integrity_check_fail(struct wpa_priv *priv);
#endif //CFG_SOFTAP_SUPPORT

#endif //__WPAS_CRYPTO_H_