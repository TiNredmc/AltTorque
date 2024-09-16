#include "app_servo.h"

// Private variables
uint8_t app_servo_main_fsm = 0;

uint32_t controlsys_millis = 0;
uint32_t voltmonitor_millis = 0;
uint32_t communication_millis = 0;
uint32_t play_millis = 0;

uint8_t led_fsm = 0;

void app_servo_gpioInit(){
	RCU_AHBEN |=
		(1 << 17)		| // Enable GPIOA
		(1 << 18)		; // Enable GPIOB
	
	// PORT A
	
	GPIO_CTL(GPIOA) |=
		(3 << (VRegSen_Pin * 2))	|
		(3 << (VBusSen_Pin * 2))	|
		(3 << (VISen_Pin * 2))		|
		(1 << (PHASE_Pin * 2))		|
		(1 << (nSLEEP_Pin * 2))	 	|
		(1 << (BRAKE_Pin * 2))		| 
		(0 << (nFAULT_Pin * 2))	 	|
		(3 << (VPotSen_Pin * 2))	|
		(2 << (ENBL_Pin * 2))		 	|
		(2 << (USART_IO * 2))			;
	
	GPIO_OMODE(GPIOA) |= 
		(1 << USART_IO);// Open drain output PA9
	
	GPIO_AFSEL1(GPIOA) |= 
		(2 << (0 * 4)) 	| 	// (PA8) AF2 - TIM0 CH0
		(1 << (1 * 4))	;		// (PA9) AF1 - USART0 TX
	
	GPIO_OSPD(GPIOA) |= 
		(3 << (ENBL_Pin * 2))	|
		(3 << (USART_IO * 2))	;
	
	// PORT B

	GPIO_CTL(GPIOB) |=
		(3 << (0 * 2))			| // Test PB0 analog to search for VIsense
		(2 << (LED_Pin * 2));
	
	// Open drain output
	GPIO_OMODE(GPIOB) |=
		(1 << LED_Pin);
	
	// Using TIM13 CH0 for led control
	GPIO_AFSEL0(GPIOB) |=
		(0 << (LED_Pin * 4));
	
}

void app_servo_ledOn(){
	ledpwm_setDuty(15);
}

void app_servo_ledOff(){
	ledpwm_setDuty(0);
}

uint8_t blink_sequence = 0;

void app_servo_ledBreathingFast(){
	if(blink_sequence > 0x08){
		blink_sequence -= 4;
		if(blink_sequence == 0x10)
			blink_sequence = 0;
	}else{
		blink_sequence += 4;
		if(blink_sequence == 0x08)
			blink_sequence = 0x18;
	}
	
	ledpwm_setDuty(blink_sequence);
}

void app_servo_ledBreathingSlow(){
	if(blink_sequence > 0x08){
		blink_sequence--;
		if(blink_sequence == 0x10)
			blink_sequence = 0;
	}else{
		blink_sequence++;
		if(blink_sequence == 0x08)
			blink_sequence = 0x18;
	}
	ledpwm_setDuty(blink_sequence);
}


void app_servo_ledRunner(){
	// First priority, Fault indicator
	if(app_mon_faultDetected()){
		app_servo_ledBreathingFast();
		return;
	}
	
	app_servo_ledBreathingSlow();// Stand by LED
	
}

void app_servo_Init(){
	clk_init();
	systick_init(48000000UL, 10000U);// 100us
	app_servo_gpioInit();
	ledpwm_init();
	// Initialize control system
	
	app_controlsys_init();
	
	// Check for configuration data in flash
	if(!app_nvm_checkBlank()){
		app_nvm_init();
		// TODO : If Blank, do something
		app_controlsys_config(
			app_nvm_getSelfKp(),
			app_nvm_getSelfKi(),
			app_nvm_getSelfKd()
		);
	}
	
	// Initialize communication
	app_comInit(app_nvm_getSelfID());
}

uint8_t play_fsm = 0;

void app_servo_runner(){
	app_comPoll();
	
	// 250Hz control system
	if((millis() - controlsys_millis) > 40){
		controlsys_millis = millis();
		app_controlsys_runner();
	}
	
	// 50Hz Voltage monitoring
	if((millis() - voltmonitor_millis) > 200){
		voltmonitor_millis = millis();
		app_mon_vmonRunner();
		app_servo_ledRunner();		
	}
	
}