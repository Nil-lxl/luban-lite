#ifndef ATBM_OS_SKBUFF_H
#define ATBM_OS_SKBUFF_H
#include "atbm_type.h"
typedef struct netif ATBM_NETIF;

struct atbm_net_device * atbm_alloc_netdev(atbm_int32 size);
atbm_void atbm_skbbuffer_init();
atbm_void * netdev_drv_priv(struct atbm_net_device *ndev);
int  atbm_register_netdevice(struct atbm_net_device *netdev);

#endif /* ATBM_OS_SKBUFF_H */

