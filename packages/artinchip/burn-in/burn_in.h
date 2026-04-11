/*
* Copyright (C) 2020-2023 ArtInChip Technology Co. Ltd
*
*  author: <jun.ma@artinchip.com>
*  Desc: burn_in
*/

#ifndef _BURN_IN_H_
#define _BURN_IN_H_

#define BURN_DEBUG

#ifdef BURN_DEBUG
    #define BURN_PRINT_DGB(fmt,args...) printf("\033[40;32m" "[%s:%s:%d]" fmt "\033[0m \n" ,__FILE__,__FUNCTION__,__LINE__,##args)
    #define BURN_PRINT_ERR(fmt,args...) printf("\033[40;31m" "[%s:%s:%d]" fmt "\033[0m \n" ,__FILE__,__FUNCTION__,__LINE__,##args)
#else
    #define BURN_PRINT_DGB(...)
    #define BURN_PRINT_ERR(...)
#endif

#ifdef LPKG_BURN_IN_PLAYER_ENABLE
    int burn_in_player_test(void *arg);
#endif

#endif
