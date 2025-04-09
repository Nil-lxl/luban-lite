#include <rtthread.h>
#include <aic_osal.h>
#include <rtconfig.h>

#include "ptpd.h"
#include "lwip/netif.h"


extern void ptpd_update_timer(void);

void ieee1588_init(void)
{
    /* wait for netif_default OK */
    while (netif_default == NULL)
    	aicos_msleep(100);

    ptpd_init();

    while (1) {
    	if (netif_is_link_up(netif_default)) {
    		ptpd_task();
    		ptpd_update_timer();
    	}

        aicos_msleep(1);
    }
}

static void ptpd_thread_creat(void)
{
    aicos_thread_create("ptpd thread", LPKG_PTPD_THREAD_STACK_SIZE, LPKG_PTPD_THREAD_PRI, ieee1588_init, NULL);
}

INIT_LATE_APP_EXPORT(ptpd_thread_creat);
