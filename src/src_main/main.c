// AltTorque. 
// The alternative open-source firmware for the UBTECH U002k7V4-DSRA-S (2kg) servo motor
// Coded by TinLethax 8/21/2024 

#include "main.h"

void HardFault_Handler(void){
	
}

int main(){
	app_servo_Init();
	
	
	while(1){
		app_servo_runner();
	}

}