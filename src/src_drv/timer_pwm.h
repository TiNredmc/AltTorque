#ifndef TIMER_H
#define TIMER_H

#include "gd32f1x0.h"
#include "gd32f1x0_timer.h"

void pwm_init();

void pwm_setDuty(uint16_t dtcycl);

#endif