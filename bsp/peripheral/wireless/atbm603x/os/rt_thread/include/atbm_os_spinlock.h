#ifndef ATBM_OS_SPINLOCK_H
#define ATBM_OS_SPINLOCK_H
#include "atbm_type.h"
#include "rthw.h"

typedef int atbm_spinlock_t;
/*spin lock*/
#define atbm_spin_lock_init(x)
#define atbm_spin_lock(x)                   do{/*iot_printf("spin_lock\n");*/rt_enter_critical();}while(0)
#define atbm_spin_unlock(x)                 do{/*iot_printf("spin_unlock\n");*/rt_exit_critical();}while(0)
#define atbm_spin_lock_irqsave(x,f)         do{/*iot_printf("spin_lock_irqsave\n");*/rt_enter_critical();}while(0)
#define atbm_spin_unlock_irqrestore(x,f)    do{/*iot_printf("spin_unlock_irqsave\n");*/rt_exit_critical();}while(0)
#define atbm_spin_lock_bh(x)                do{/*iot_printf("spin_lock_bh\n");*/rt_enter_critical();}while(0)
#define atbm_spin_unlock_bh(x)              do{/*iot_printf("spin_unlock_bh\n");*/rt_exit_critical();}while(0)

#endif /* ATBM_OS_SPINLOCK_H */

