#ifndef ADC_H
#define ADC_H

#include "gd32f1x0.h"
#include "gd32f1x0_adc.h"
#include "gd32f1x0_dma.h"

#include "app_gpio.h"

void adc_init(uint32_t *data_ptr);
void adc_softTrigger();
uint8_t adc_getDataAvaible();

#endif