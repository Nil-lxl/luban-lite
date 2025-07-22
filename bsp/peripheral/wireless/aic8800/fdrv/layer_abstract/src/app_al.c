/*
 * Copyright (C) 2018-2024 AICSemi Ltd.
 *
 * All Rights Reserved
 */

#include "app_al.h"
#ifdef CONFIG_APP_IPERF
#include "iperf.h"
#include"aic_plat_net.h"

int aic_iperf_parse_argv(int argc, char *argv[])
{
    return iperf_parse_argv(argc, argv);
}

int aic_iperf_handle_start(struct netif *nif, int handle)
{
    return iperf_handle_start(nif, handle);
}

int aic_iperf_handle_free(int handle)
{
    return iperf_handle_free(handle);
}

int aic_get_iperf_handle_max(void)
{
    return IPERF_ARG_HANDLE_MAX;
}
#endif
