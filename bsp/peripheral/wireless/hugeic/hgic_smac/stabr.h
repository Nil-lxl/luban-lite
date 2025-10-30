
#include <asm/unaligned.h>
#include <linux/list.h>
#include <linux/netdevice.h>
#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/gfp.h>

#ifndef __RTOS__
#include <linux/compiler.h>
#endif

#ifndef HGIC_STABR_H_
#define HGIC_STABR_H_

//#define STABR_PRINT(fmt, ...) printk("%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
//#define STABR_PRINT(...)


#define STABR_ENTRY_LIFETIME (10*60*HZ)

enum stabr_entry_flag{
    stabr_entry_flag_ucast = BIT(0),
};

struct stabr_table_entry {
    struct list_head list;
    u32    id;
    unsigned long lifetime;
    u16    flag;
    char   mac[ETH_ALEN];
};

struct stabr_table {
    struct list_head list[16];
    spinlock_t lock;
    u16 entry_cnt, max_cnt;
};

struct stabr_protocol {
    u16 protocol;
    void (*rx_handler)(struct sk_buff *skb);
    struct sk_buff *(*tx_handler)(struct sk_buff *skb);
    struct stabr_table table;
};

extern int hgic_stabr_init(void);
extern int hgic_stabr_release(void);
extern void hgic_stabr_attach(struct net_device *dev);
extern struct stabr_table_entry *hgic_stabr_find_entry(u32 ip);
extern struct stabr_table_entry *hgic_stabr_table_update(u32 id, char *fmac, u16 flag);
extern void hgic_stabr_status(void);
#endif

