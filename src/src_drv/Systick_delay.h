#ifndef SYSTICK_DELAY_H
#define SYSTICK_DELAY_H

#include "gd32f1x0.h"

void systick_init(const uint32_t f_cpu, const uint32_t Hz);
uint32_t millis();
void delay_ms(uint32_t ms);

#endif