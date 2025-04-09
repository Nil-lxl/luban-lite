/*
 * Copyright (C) 2018-2024 AICSemi Ltd.
 *
 * All Rights Reserved
 */

#ifndef _APP_AL_H_
#define _APP_AL_H_

#include "aic_plat_types.h"

int aic_iperf_parse_argv(int argc, char *argv[]);
int aic_iperf_handle_start(struct netif *nif, int handle);
int aic_iperf_handle_free(int handle);
int aic_get_iperf_handle_max(void);

#endif /* _APP_AL_H_ */

