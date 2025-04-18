#ifndef COMMON_H
#define COMMON_H

/* Define platform specific integer types */
#include <uwifi_kernel.h>

#ifndef WPA_TYPES_DEFINED
//#include <asr_arch.h>
#ifdef CONFIG_USE_INTTYPES_H
#include <inttypes.h>
#else
#include <stdint.h>
#endif
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
#ifndef JL_SDK
typedef int8_t s8;
#endif
#define WPA_TYPES_DEFINED
#endif /* !WPA_TYPES_DEFINED */


/* Define platform specific byte swapping macros */
#define TEST_FAIL() 0


#ifndef WPA_BYTE_SWAP_DEFINED

#ifndef __BYTE_ORDER
#ifndef __LITTLE_ENDIAN
#ifndef __BIG_ENDIAN
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN 4321
#if defined(sparc)
#define __BYTE_ORDER __BIG_ENDIAN
#endif
#endif /* __BIG_ENDIAN */
#endif /* __LITTLE_ENDIAN */
#endif /* __BYTE_ORDER */

//#if __BYTE_ORDER == __LITTLE_ENDIAN
#define le_to_host16(n) ((__force u16) (le16) (n))
#define host_to_le16(n) ((__force le16) (u16) (n))
#define be_to_host16(n) bswap_16((__force u16) (be16) (n))
#define host_to_be16(n) ((__force be16) bswap_16((n)))
#define le_to_host32(n) ((__force u32) (le32) (n))
#define host_to_le32(n) ((__force le32) (u32) (n))
#define be_to_host32(n) bswap_32((__force u32) (be32) (n))
#define host_to_be32(n) ((__force be32) bswap_32((n)))


#define WPA_BYTE_SWAP_DEFINED
#endif /* !WPA_BYTE_SWAP_DEFINED */


/* Macros for handling unaligned memory accesses */

static inline u16 WPA_GET_BE16(const u8 *a)
{
    return (a[0] << 8) | a[1];
}

static inline void WPA_PUT_BE16(u8 *a, u16 val)
{
    a[0] = val >> 8;
    a[1] = val & 0xff;
}

static inline u16 WPA_GET_LE16(const u8 *a)
{
    return (a[1] << 8) | a[0];
}

static inline void WPA_PUT_LE16(u8 *a, u16 val)
{
    a[1] = val >> 8;
    a[0] = val & 0xff;
}

static inline u32 WPA_GET_BE24(const u8 *a)
{
    return (a[0] << 16) | (a[1] << 8) | a[2];
}

static inline void WPA_PUT_BE24(u8 *a, u32 val)
{
    a[0] = (val >> 16) & 0xff;
    a[1] = (val >> 8) & 0xff;
    a[2] = val & 0xff;
}

static inline u32 WPA_GET_BE32(const u8 *a)
{
    return ((u32) a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3];
}

static inline void WPA_PUT_BE32(u8 *a, u32 val)
{
    a[0] = (val >> 24) & 0xff;
    a[1] = (val >> 16) & 0xff;
    a[2] = (val >> 8) & 0xff;
    a[3] = val & 0xff;
}

static inline u32 WPA_GET_LE32(const u8 *a)
{
    return ((u32) a[3] << 24) | (a[2] << 16) | (a[1] << 8) | a[0];
}

static inline void WPA_PUT_LE32(u8 *a, u32 val)
{
    a[3] = (val >> 24) & 0xff;
    a[2] = (val >> 16) & 0xff;
    a[1] = (val >> 8) & 0xff;
    a[0] = val & 0xff;
}

static inline u64 WPA_GET_BE64(const u8 *a)
{
    return (((u64) a[0]) << 56) | (((u64) a[1]) << 48) |
        (((u64) a[2]) << 40) | (((u64) a[3]) << 32) |
        (((u64) a[4]) << 24) | (((u64) a[5]) << 16) |
        (((u64) a[6]) << 8) | ((u64) a[7]);
}

static inline void WPA_PUT_BE64(u8 *a, u64 val)
{
    a[0] = val >> 56;
    a[1] = val >> 48;
    a[2] = val >> 40;
    a[3] = val >> 32;
    a[4] = val >> 24;
    a[5] = val >> 16;
    a[6] = val >> 8;
    a[7] = val & 0xff;
}

static inline u64 WPA_GET_LE64(const u8 *a)
{
    return (((u64) a[7]) << 56) | (((u64) a[6]) << 48) |
        (((u64) a[5]) << 40) | (((u64) a[4]) << 32) |
        (((u64) a[3]) << 24) | (((u64) a[2]) << 16) |
        (((u64) a[1]) << 8) | ((u64) a[0]);
}

static inline void WPA_PUT_LE64(u8 *a, u64 val)
{
    a[7] = val >> 56;
    a[6] = val >> 48;
    a[5] = val >> 40;
    a[4] = val >> 32;
    a[3] = val >> 24;
    a[2] = val >> 16;
    a[1] = val >> 8;
    a[0] = val & 0xff;
}


#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif
#ifndef ETH_HLEN
#define ETH_HLEN 14
#endif

#if 0
#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif
#endif

#ifndef ETH_P_ALL
#define ETH_P_ALL 0x0003
#endif
#ifndef ETH_P_IP
#define ETH_P_IP 0x0800
#endif
#ifndef ETH_P_80211_ENCAP
#define ETH_P_80211_ENCAP 0x890d /* TDLS comes under this category */
#endif
#ifndef ETH_P_PAE
#define ETH_P_PAE 0x888E /* Port Access Entity (IEEE 802.1X) */
#endif /* ETH_P_PAE */
#ifndef ETH_P_EAPOL
#define ETH_P_EAPOL ETH_P_PAE
#endif /* ETH_P_EAPOL */
#ifndef ETH_P_RSN_PREAUTH
#define ETH_P_RSN_PREAUTH 0x88c7
#endif /* ETH_P_RSN_PREAUTH */
#ifndef ETH_P_RRB
#define ETH_P_RRB 0x890D
#endif /* ETH_P_RRB */
#ifndef ETH_P_OUI
#define ETH_P_OUI 0x88B7
#endif /* ETH_P_OUI */


#ifdef __GNUC__
#define PRINTF_FORMAT(a,b) __attribute__ ((format (printf, (a), (b))))
#define STRUCT_PACKED __attribute__ ((packed))
#else
#define PRINTF_FORMAT(a,b)
#define STRUCT_PACKED
#endif

#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"

/*
 * Compact form for string representation of MAC address
 * To be used, e.g., for constructing dbus paths for P2P Devices
 */
#define COMPACT_MACSTR "%02x%02x%02x%02x%02x%02x"
#endif

#ifndef BIT
#define BIT(x) (1U << (x))
#endif

/*
 * Definitions for sparse validation
 * (http://kernel.org/pub/linux/kernel/people/josh/sparse/)
 */
#ifdef __CHECKER__
#define __force __attribute__((force))
#undef __bitwise
#define __bitwise __attribute__((bitwise))
#else
#define __force
#undef __bitwise
#define __bitwise
#endif

typedef u16 __bitwise be16;
typedef u16 __bitwise le16;
typedef u32 __bitwise be32;
typedef u32 __bitwise le32;
typedef u64 __bitwise be64;
typedef u64 __bitwise le64;

#ifndef __must_check
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define __must_check __attribute__((__warn_unused_result__))
#else
#define __must_check
#endif /* __GNUC__ */
#endif /* __must_check */

#ifndef __maybe_unused
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define __maybe_unused __attribute__((unused))
#else
#define __maybe_unused
#endif /* __GNUC__ */
#endif /* __must_check */

#define SSID_MAX_LEN 32

struct wpa_ssid_value {
    u8 ssid[SSID_MAX_LEN];
    size_t ssid_len;
};

int hwaddr_aton(const char *txt, u8 *addr);
int hwaddr_masked_aton(const char *txt, u8 *addr, u8 *mask, u8 maskable);
int hwaddr_compact_aton(const char *txt, u8 *addr);
int hwaddr_aton2(const char *txt, u8 *addr);
//int hex2byte(const char *hex);
//int hexstr2bin(const char *hex, u8 *buf, size_t len);
void inc_byte_array(u8 *counter, size_t len);
void buf_shift_right(u8 *buf, size_t len, size_t bits);
void wpa_get_ntp_timestamp(u8 *buf);
int wpa_scnprintf(char *buf, size_t size, const char *fmt, ...);
int wpa_snprintf_hex_sep(char *buf, size_t buf_size, const u8 *data, size_t len,
             char sep);
int wpa_snprintf_hex(char *buf, size_t buf_size, const u8 *data, size_t len);
int wpa_snprintf_hex_uppercase(char *buf, size_t buf_size, const u8 *data,
                   size_t len);

int hwaddr_mask_txt(char *buf, size_t len, const u8 *addr, const u8 *mask);
int ssid_parse(const char *buf, struct wpa_ssid_value *ssid);

#ifdef CONFIG_NATIVE_WINDOWS
void wpa_unicode2ascii_inplace(TCHAR *str);
TCHAR * wpa_strdup_tchar(const char *str);
#else /* CONFIG_NATIVE_WINDOWS */
#define wpa_unicode2ascii_inplace(s) do { } while (0)
#define wpa_strdup_tchar(s) strdup((s))
#endif /* CONFIG_NATIVE_WINDOWS */

void printf_encode(char *txt, size_t maxlen, const u8 *data, size_t len);
size_t printf_decode(u8 *buf, size_t maxlen, const char *str);

const char * wpa_ssid_txt(const u8 *ssid, size_t ssid_len);

char * wpa_config_parse_string(const char *value, size_t *len);
int is_hex(const u8 *data, size_t len);
int has_ctrl_char(const u8 *data, size_t len);
int has_newline(const char *str);
size_t merge_byte_arrays(u8 *res, size_t res_len,
             const u8 *src1, size_t src1_len,
             const u8 *src2, size_t src2_len);
char * dup_binstr(const void *src, size_t len);

static inline int is_zero_ether_addr(const u8 *a)
{
    return !(a[0] | a[1] | a[2] | a[3] | a[4] | a[5]);
}

#if 0
static inline int is_broadcast_ether_addr(const u8 *a)
{
    return (a[0] & a[1] & a[2] & a[3] & a[4] & a[5]) == 0xff;
}


static inline int is_multicast_ether_addr(const u8 *a)
{
    return a[0] & 0x01;
}
#endif

#define broadcast_ether_addr (const u8 *) "\xff\xff\xff\xff\xff\xff"

//#include "wpa_debug.h"


struct wpa_freq_range_list {
    struct wpa_freq_range {
        unsigned int min;
        unsigned int max;
    } *range;
    unsigned int num;
};

int freq_range_list_parse(struct wpa_freq_range_list *res, const char *value);
int freq_range_list_includes(const struct wpa_freq_range_list *list,
                 unsigned int freq);
char * freq_range_list_str(const struct wpa_freq_range_list *list);

int int_array_len(const int *a);
void int_array_concat(int **res, const int *a);
void int_array_sort_unique(int *a);
void int_array_add_unique(int **res, int a);

//#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

void str_clear_free(char *str);
void bin_clear_free(void *bin, size_t len);

int random_mac_addr(u8 *addr);
int random_mac_addr_keep_oui(u8 *addr);

const char * cstr_token(const char *str, const char *delim, const char **last);
char * str_token(char *str, const char *delim, char **context);
size_t utf8_escape(const char *inp, size_t in_size,
           char *outp, size_t out_size);
size_t utf8_unescape(const char *inp, size_t in_size,
             char *outp, size_t out_size);
int is_ctrl_char(char c);

int str_starts(const char *str, const char *start);

u8 rssi_to_rcpi(int rssi);
char * get_param(const char *cmd, const char *param);

void forced_memzero(void *ptr, size_t len);

// merge from os_internal.h
int os_get_random(unsigned char *buf, size_t len);
void * os_malloc(size_t size);
void * os_calloc(size_t nmemb, size_t size);
void * os_memset(void *s, int c, size_t n);
void * os_zalloc(size_t size);
void * os_realloc(void *ptr, size_t size);
void os_free(void *ptr);
void * os_memcpy(void *dest, const void *src, size_t n);
void * os_memmove(void *dest, const void *src, size_t n);
int os_memcmp(const void *s1, const void *s2, size_t n);
size_t os_strlen(const char *s);
int os_strcasecmp(const char *s1, const char *s2);
int os_strncasecmp(const char *s1, const char *s2, size_t n);
char * os_strchr(const char *s, int c);
char * os_strrchr(const char *s, int c);
int os_strcmp(const char *s1, const char *s2);
int os_strncmp(const char *s1, const char *s2, size_t n);
size_t os_strlcpy(char *dest, const char *src, size_t siz);
int os_memcmp_const(const void *a, const void *b, size_t len);

/*
 * gcc 4.4 ends up generating strict-aliasing warnings about some very common
 * networking socket uses that do not really result in a real problem and
 * cannot be easily avoided with union-based type-punning due to struct
 * definitions including another struct in system header files. To avoid having
 * to fully disable strict-aliasing warnings, provide a mechanism to hide the
 * typecast from aliasing for now. A cleaner solution will hopefully be found
 * in the future to handle these cases.
 */
void * __hide_aliasing_typecast(void *foo);
#define aliasing_hide_typecast(a,t) (t *) __hide_aliasing_typecast((a))

#ifdef CONFIG_VALGRIND
#include <valgrind/memcheck.h>
#define WPA_MEM_DEFINED(ptr, len) VALGRIND_MAKE_MEM_DEFINED((ptr), (len))
#else /* CONFIG_VALGRIND */
#define WPA_MEM_DEFINED(ptr, len) do { } while (0)
#endif /* CONFIG_VALGRIND */

#endif /* COMMON_H */
