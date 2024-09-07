#ifndef LED_PWM_H
#define LED_PWM_H

#include "gd32f1x0.h"
#include "gd32f1x0_timer.h"

void ledpwm_init();
void ledpwm_setDuty(uint16_t dtcycl);

#endif