// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ARTINCHIP_ETHERNETIF_H__
#define __ARTINCHIP_ETHERNETIF_H__

#include "lwip/err.h"
#include "lwip/netif.h"

err_t ethernetif_init(struct netif *netif);
err_t ethernetif_input(struct netif *netif);
void ethernetif_input_poll(struct netif *netif);

#endif
