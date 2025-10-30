/*
* Copyright (C) 2020-2023 ArtInChip Technology Co. Ltd
*
*  author: <jun.ma@artinchip.com>
*  Desc: burn_in
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <rthw.h>
#include <rtthread.h>
#include <shell.h>
#include "msh.h"
#include "aic_osal.h"
#include "burn_in.h"

static void auto_burn_in_test(void *arg)
{
    // wait some time for resource
    usleep(5*1000*1000);
#ifdef LPKG_BURN_IN_PLAYER_ENABLE
    burn_in_player_test(NULL);
#endif
}

int auto_burn_in(void)
{
    aicos_thread_t thid = NULL;
    thid = aicos_thread_create("auto_burn_in_test", 8192, 2, auto_burn_in_test, NULL);
    if (thid == NULL) {
        BURN_PRINT_ERR("Failed to create thread\n");
        return -1;
    }
    return 0;
}

INIT_APP_EXPORT(auto_burn_in);
