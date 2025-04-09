#include <stdio.h>
#include "stddef.h"
#define __INLINE inline

#define  atbm_random()   rand()

#define  rcu_read_lock()
#define  rcu_read_unlock()
#define  TargetUsb_lmac_start()
#define synchronize_rcu()

atbm_uint32 atbm_os_random();
#define ZEROSIZE 0


