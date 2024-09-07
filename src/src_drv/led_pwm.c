#include "led_pwm.h"

void ledpwm_init(){
	RCU_APB1EN |= (1 << 8);// Enable TIM13 clock

	// Sysclock is 48MHz
	// PWM resolution is 4 bit (0-15)
	// Required frequency 240hz PWM
	// (48e6 / 16) / 240 = ~23.4 kHz
	TIMER_PSC(TIMER13) = 12500 - 1;// (Sysclock/16) / 240 -> TIM clock
	
	TIMER_CAR(TIMER13) = 16 - 1;// ARR at 2^4 - 1
	
	TIMER_SWEVG(TIMER13) |= 1;// Enable update generation
	
	TIMER_CHCTL0(TIMER13) |=
		(1 << 3)						|	// Enable compare Shadown register 
		(7 << 4)						;	// CH0 as PWM1 mode
	
	TIMER_CHCTL2(TIMER13) |=
		(1 << 0)						;	// Enable CH0
	
	TIMER_CTL0(TIMER13) |= 1;// Enable Timer
	
}

void ledpwm_setDuty(uint16_t dtcycl){
	if(dtcycl > 15)
		dtcycl = 15;
	
	TIMER_CH0CV(TIMER13) = dtcycl;
}