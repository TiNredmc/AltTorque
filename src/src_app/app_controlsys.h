#ifndef APP_CONTROL_H
#define APP_CONTROL_H

#include "gd32f1x0.h"
#include "gd32f1x0_gpio.h"
#include "app_gpio.h"

#include "timer_pwm.h"
#include "adc.h"

// VBus sense seems to have x1/4 voltage divider
#define VBUS_ADC_VUVLO		1407 // ~5V (1.25V at ADC)
#define VBUS_ADC_VNORMAL	1688 // ~6V (1.5V at ADC)

// VReg sense seems to have x1/2 voltage divider
#define VREG_ADC_VUVLO		1288 // ~2.5V (1.25V at ADC)
#define VREG_ADC_VNORMAL	1700 // ~3.3V (1.6V at ADC)

// fault recovery delay 
// t is second = rate * 50
#define RECOVERY_PERIOD		50 // 1 second delay

// Integral limit
#define INTG_LIMIT				300

// Fixed point math
// using int16 with the format of Q1.14
// 1 sign bit, 1 interger bit and 14 fraction bit
// using int32 with the format of Q3.28

typedef uint16_t 	UFIXPOINT16;
typedef uint32_t	UFIXPOINT32;

typedef int16_t 	SFIXPOINT16;
typedef int32_t 	SFIXPOINT32;

typedef struct  __attribute__((packed)){
	uint16_t V3v3Sense;
	uint16_t VbusSense;
	uint16_t ImotorSense;
	uint16_t VpotSense;
}adcD_type_t;

typedef struct{
	int16_t setpoint;
	SFIXPOINT16 angle_error;

	SFIXPOINT16 Kp;
	SFIXPOINT16 Ki;
	SFIXPOINT16 Kd;
	
	SFIXPOINT16 prevError;
	SFIXPOINT32 output;
}pid_type_t;

void app_controlsys_init();
void app_controlsys_config(
	int32_t new_Kp,
	int32_t new_Ki,
	int32_t new_Kd
	);

uint8_t app_controlsys_getPotDegree();
uint16_t app_controlsys_getVBus();
uint16_t app_controlsys_getVReg();
uint16_t app_controlsys_getImotor();

void app_controlsys_updateSP(uint8_t degree);
void app_controlsys_runner();
void app_controlsys_vmonRunner();
uint8_t app_controlsys_faultDetected();

void app_controlsys_commandMotor(int16_t speed);

#endif