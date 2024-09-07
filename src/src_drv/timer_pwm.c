#include "timer_pwm.h"

void pwm_init(){
	
	RCU_APB2EN |= (1 << 11);// Enable TIM0 clock

	// Sysclock is 48MHz
	// PWM resolution is 10 bit (0-1023)
	// Required frequency ~24Khz PWM
	// (48e6 / 1024) / 2 = ~23.4 kHz
	TIMER_PSC(TIMER0) = 1;// Sysclock/2 -> TIM clock
	
	TIMER_CAR(TIMER0) = 1023;// ARR at 2^10 - 1
	
	TIMER_SWEVG(TIMER0) |= 1;// Enable update generation
	
	TIMER_CHCTL0(TIMER0) |=
		(1 << 3)						|	// Enable compare Shadown register 
		(6 << 4)						;	// CH0 as PWM0 mode
	
	TIMER_CHCTL2(TIMER0) |=
		(1 << 0)						;	// Enable CH0
	
	TIMER_CCHP(TIMER0) |= (1 << 14);// Enable Primary output
	
	TIMER_CTL0(TIMER0) |= 1;// Enable Timer
	
}

void pwm_setDuty(uint16_t dtcycl){
	if(dtcycl > 1023)
		dtcycl = 1023;
	
	TIMER_CH0CV(TIMER0) = dtcycl;
}