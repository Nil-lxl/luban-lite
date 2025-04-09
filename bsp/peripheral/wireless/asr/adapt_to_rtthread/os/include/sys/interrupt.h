#ifndef _ASR_SYS_INTERRUPT_H_
#define _ASR_SYS_INTERRUPT_H_

int asr_arch_irq_save(void);
int asr_arch_irq_restore(int flags);

void asr_arch_irq_enable(void);
void asr_arch_irq_disable(void);

#define asr_irq_save asr_arch_irq_save
#define asr_irq_restore asr_arch_irq_restore

#endif /* _SYS_INTERRUPT_H_ */
