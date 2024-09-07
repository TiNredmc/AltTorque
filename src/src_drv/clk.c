#include "clk.h"

void clk_init(){
	// use 24MHz Xtal
	
	// Enable HXTAL
	RCU_CTL0 |= RCU_CTL0_HXTALEN;
	// Wait until oscillator is up
	while(!(RCU_CTL0 & RCU_CTL0_HXTALSTB));
	
	// Enable and configure the PLL
	
	RCU_CFG0 |= 
		RCU_CFG0_PLLSEL 	; // Select HXTAL as PLL source
		
	// PLL is normally multiply the freq by 2
	
	// Enable PLL
	RCU_CTL0 |= RCU_CTL0_PLLEN;
	// Wait until PLL is stable
	while(!(RCU_CTL0 & RCU_CTL0_PLLSTB));
	
	// Switch system clock to PLL
	RCU_CFG0 |= 2;// Select PLL as system clock
	// Wait until clock switch to PLL
	while(!(RCU_CFG0 & (2 << 2)));
	
	RCU_CTL1 |= 1;// Enable IRC14M for ADC
	// Wait until ADC clock is stable
	while(!(RCU_CTL1 & (1 << 1)));
}