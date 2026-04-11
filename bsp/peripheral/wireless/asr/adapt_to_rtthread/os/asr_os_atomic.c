/*************************************************************
 *   Copyright (C) 2022 All rights reserved.
 *   
 *   Descraption: TODO
 *                
 *   FileName  : asr_os_atomic.c
 *   Author    : ASR
 *   Create    : 2022-12-07
 *   Version   : V0.1.0
 *
**************************************************************/
#include "asr_rtos_api.h"
//#include "asm/atomic.h"
//#include "asm/irqflags.h"

#define BITS_PER_LONG	32
#define BIT(nr)		(1UL << (nr))
#define BIT_MASK(nr)	(1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)	((nr) / BITS_PER_LONG)


//#define _asr_atomic_lock_irqsave(l,f) do {f = arch_local_irq_save();} while (0)
//#define _asr_atomic_unlock_irqsave(l,f) do {arch_local_irq_restore(f);} while (0)


int asr_test_bit(int nr, asr_atomic_t* addr)
{
    return 1UL & (addr[BIT_WORD(nr)] >> (nr & (BITS_PER_LONG - 1)));
}


int asr_clear_bit(int nr, asr_atomic_t* addr)
{
    unsigned long mask = BIT_MASK(nr);
    unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
    unsigned long flags;

    //_asr_atomic_lock_irqsave(p, flags);
    *p &= ~mask;
    //_asr_atomic_unlock_irqsave(p, flags);
    return 0;
}

int asr_set_bit(int nr, asr_atomic_t* addr)
{
    unsigned long mask = BIT_MASK(nr);
    unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
    unsigned long flags;

    //_asr_atomic_lock_irqsave(p, flags);
    *p |= mask;
    //_asr_atomic_unlock_irqsave(p, flags);
    return 0;
}

int asr_test_and_set_bit(int nr, asr_atomic_t* addr)
{
    unsigned long mask = BIT_MASK(nr);
    unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
    unsigned long old;
    //unsigned long flags;

    //_asr_atomic_lock_irqsave(p, flags);
    old = *p;
    *p = old | mask;
    //_asr_atomic_unlock_irqsave(p, flags);
    return (old & mask) != 0;
}