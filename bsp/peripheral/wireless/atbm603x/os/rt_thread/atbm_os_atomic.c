/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/


#include "atbm_hal.h"
#include "atbm_os_atomic.h"
#include "rthw.h"

static unsigned int __atbm_local_irq_save(atbm_void)
{
	return rt_hw_interrupt_disable();
}

static atbm_void __atbm_local_irq_restore(unsigned int cpu_sr)
{
    rt_hw_interrupt_enable(cpu_sr);
}

unsigned int atbm_local_irq_save(atbm_void)
{
	return __atbm_local_irq_save();
}

atbm_void atbm_local_irq_restore(unsigned int cpu_sr)
{
	cpu_sr = cpu_sr;
	__atbm_local_irq_restore(cpu_sr);
}

 atbm_void atbm_atomic_set(atbm_atomic_t *at, int val)
{
	ATBM_OS_CPU_SR cpu_sr = __atbm_local_irq_save();
	at->val = val;
	__atbm_local_irq_restore(cpu_sr);
}

 int atbm_atomic_read(atbm_atomic_t *at)
{
	int val = 0;
	ATBM_OS_CPU_SR cpu_sr = __atbm_local_irq_save();
	
	val = at->val;
	__atbm_local_irq_restore(cpu_sr);

	return val;
}
  int atbm_atomic_add_return(int val,atbm_atomic_t *at)
{

	ATBM_OS_CPU_SR cpu_sr = __atbm_local_irq_save();
	at->val += val;
	__atbm_local_irq_restore(cpu_sr);
	
	return  at->val;	
}

int atbm_atomic_xchg(atbm_atomic_t * v, int val)
{
	int tmp = 0;
	ATBM_OS_CPU_SR cpu_sr = __atbm_local_irq_save();
	tmp = v->val;
	v->val = val;
	__atbm_local_irq_restore(cpu_sr);
	return tmp;
}


 
 
 int atbm_set_bit(int nr,atbm_uint32* addr)
 {
	 int mask,retval;
	 
	 ATBM_OS_CPU_SR cpu_sr = __atbm_local_irq_save();
	 addr += nr >>5;
	 mask = 1<<(nr & 0x1f);
	 
	 retval = (mask & *addr) != 0;
	 *addr |= mask;
	 __atbm_local_irq_restore(cpu_sr);
	 
	 return  retval;
	 
 }
 
 int atbm_clear_bit(int nr,atbm_uint32 * addr)
 {
	 int mask,retval;
	 ATBM_OS_CPU_SR cpu_sr = __atbm_local_irq_save();
	 
	 addr += nr >>5;
	 mask = 1<<(nr & 0x1f);
	 
	 retval = (mask & *addr) != 0;
	 *addr &= ~mask;
	 __atbm_local_irq_restore(cpu_sr);
	 
	 return  retval;
	 
 }
 
 
 int atbm_test_bit(int nr,atbm_uint32 * addr)
 {
 
	 int mask;
	 
	 addr += nr >>5;
	 mask = 1<<(nr & 0x1f);
	 
	 return  ((mask & *addr) != 0);
	 
 }
 int atbm_find_first_zero_bit(atbm_uint32 * addr,int size)
 {
	 int i =0;
	 for (i = 0; i <size; i++) {
		 if(atbm_test_bit(i,addr) ==0)
			 return i;
	 }
	 return -1;
 }

