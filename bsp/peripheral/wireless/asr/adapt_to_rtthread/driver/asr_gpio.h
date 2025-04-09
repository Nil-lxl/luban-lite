#ifndef _ASR_GPIO_H_
#define _ASR_GPIO_H_

#include "asr_types.h"
#include "asr_rtos_api.h"

int asr_gpio_init(void);
void asr_wlan_irq_disable(void);
void asr_wlan_irq_enable(void);
void asr_wlan_irq_enable_clr(void);
void asr_wlan_irq_subscribe(void *func);
void asr_wlan_irq_unsubscribe(void);
int  asr_wlan_gpio_power(int on);
void asr_wlan_set_gpio_reset_pin(int on);

#endif /* _ASR_GPIO_H_ */
