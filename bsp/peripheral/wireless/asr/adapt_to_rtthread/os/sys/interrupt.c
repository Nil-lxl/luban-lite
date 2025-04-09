//#include "typedef.h"
//#include "asm/irqflags.h"

int asr_arch_irq_save(void)
{
    return 0;
    //return arch_local_irq_save();
}

int asr_arch_irq_restore(int flags)
{
    //arch_local_irq_restore(flags);
    return 0;
}

void asr_arch_irq_disable(void)
{
}

void asr_arch_irq_enable(void)
{
}

#if 0
void asr_rtos_enter_critical( void )
{

}

void asr_rtos_exit_critical( void )
{

}
#endif
