#ifndef APP_SERVO_H
#define APP_SERVO_H

#include "gd32f1x0.h"
#include "gd32f1x0_gpio.h"

#include "clk.h"
#include "Systick_delay.h"
#include "led_pwm.h"

#include "app_gpio.h"
#include "app_com.h"
#include "app_controlsys.h"
#include "app_nvm.h"


void app_servo_Init();
void app_servo_runner();

#endif