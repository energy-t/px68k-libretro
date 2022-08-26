#ifndef _WINX68K_IRQ_H
#define _WINX68K_IRQ_H

#include <stdint.h>
#include "common.h"

void IRQH_Init(void);
uint32_t FASTCALL IRQH_DefaultVector(uint8_t irq);
void IRQH_IRQCallBack(uint8_t irq);
void IRQH_Int(uint8_t irq, uint32_t (*handler)(uint8_t));

#endif /* WINX68K_IRQ_H */
