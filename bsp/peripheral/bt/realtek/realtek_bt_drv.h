/*
 * Copyright (C) 2015 Endless Mobile, Inc.
 *
 * Author: Thomas_li <thomas_li@realsil.com.cn>
 */
#include <aic_common.h>

#define RTL_FRAG_LEN 252

#define HCI_UART 3

#define RTL_EPATCH_SIGNATURE "Realtech"
#define RTL_ROM_LMP_3499     0x3499
#define RTL_ROM_LMP_8723A    0x1200
#define RTL_ROM_LMP_8723B    0x8723
#define RTL_ROM_LMP_8723D    0x8873
#define RTL_ROM_LMP_8821A    0x8821
#define RTL_ROM_LMP_8761A    0x8761
#define RTL_ROM_LMP_8822B    0x8822
#define RTL_CONFIG_MAGIC     0x8723ab55

#define RTL_FLAG_LMPSUBV (1 << 0)
#define RTL_FLAG_HCIREV  (1 << 1)
#define RTL_FLAG_HCIVER  (1 << 2)
#define RTL_FLAG_HCIBUS  (1 << 3)

#define get_unaligned(p)                                        \
({                                                              \
    struct packed_dummy_struct {                                \
        typeof(*(p)) __val;                                     \
    } __attribute__((packed)) *__ptr = (void *) (p);            \
                                                                \
    __ptr->__val;                                               \
})
#define get_unaligned_le16(p)   le16_to_cpu(get_unaligned((u16 *)(p)))
#define get_unaligned_le32(p)   le32_to_cpu(get_unaligned((u32 *)(p)))

struct realtek_bt_patch {
	char     *name;
	uint8_t  *data;
	uint32_t len;
};

struct realtek_bt_module {
    u16 flags;
    u8 bus;
    u8 hci_ver;
    u16 hci_rev;
    u16 lmp_subver;
    bool config_needed;
    bool has_rom_version;
    struct realtek_bt_patch fw;
    struct realtek_bt_patch cfg;
};

struct realtek_bt_device {
    struct realtek_bt_module *module;
    u8 rom_version;
};

struct realtek_bt_dowmload_cmd {
    u8 index;
    u8 data[RTL_FRAG_LEN];
};

struct realtek_bt_dowmload_response {
    u8 index;
};

struct realtek_bt_rom_version_evt {
    u8 version;
};

struct realtek_bt_vendor_config_entry {
    u16 offset;
    u8 len;
    u8 data[];
};

struct realtek_bt_vendor_config {
    u32 signature;
    u16 total_len;
    struct realtek_bt_vendor_config_entry entry[];
};

#define HCI_OP_RESET              0x0c03
#define HCI_OP_READ_LOCAL_VERSION 0x1001

int realtek_bt_pin_init(void);
void realtek_bt_init(void);
int realtek_bt_shutdown(void);
