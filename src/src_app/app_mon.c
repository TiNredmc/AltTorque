#include "app_controlsys.h"

/////////// Fault monitoring ///////////
uint8_t vmon_fsm = 0;

uint8_t vbus_hysteresis = 0;
uint8_t vreg_hysteresis = 0;
uint8_t mfault_flag 		= 0;

volatile uint8_t recovery_delay = 0;

// Monitor VBus and Vreg
void app_mon_vmonRunner(){

	if(vbus_hysteresis == 0){
		// Detect VBus undervoltage
		if(app_controlsys_getVBus() < VBUS_ADC_VUVLO){
			
			vbus_hysteresis = 1;
		}
	}else{
		// Recover VBus from undervoltage
		if(app_controlsys_getVBus() > VBUS_ADC_VNORMAL){
			vbus_hysteresis = 0;
		}
	}
	
	if(vreg_hysteresis == 0){
		// Detect Vreg underfoltage
		if(app_controlsys_getVReg() < VREG_ADC_VUVLO){
			vreg_hysteresis = 1;
		}
	}else{
		// Recover VReg from undervoltage
		if(app_controlsys_getVReg() > VBUS_ADC_VNORMAL){
			vreg_hysteresis = 0;
		}
	}
	
	// Detect motor drive OCP OTP and/or OVP
	if(!(GPIO_ISTAT(GPIOA) & (1 << nFAULT_Pin))){
		mfault_flag = 1;
	}else{
		mfault_flag = 0;
	}
	
	switch(vmon_fsm){
		case 0:// Normal operation
		{
			// Check for Drive fault
			// Overcurrent
			// Over temp
			// Over voltage
			if(mfault_flag == 1){
				// Lock the motor driver 
				GPIO_OCTL(GPIOA) &= ~(1 << nSLEEP_Pin);
				vmon_fsm = 1;
			}
			
			// Check for Vbus undervoltage
			if(vbus_hysteresis == 1){
				// Lock the motor driver 
				GPIO_OCTL(GPIOA) &= ~(1 << nSLEEP_Pin);
				vmon_fsm = 1;
			}
			
			// Check for Vreg undervoltage
			if(vreg_hysteresis == 1){
				vmon_fsm = 1;
			}
			
			GPIO_OCTL(GPIOA) |= (1 << nSLEEP_Pin);
			
		}
		break;
		
		case 1:// Recovery from fault
		{
			if(
				(vbus_hysteresis == 0) &&	
				(vreg_hysteresis == 0) &&
				(mfault_flag == 0		)
			){
				recovery_delay++;
			}else{
				recovery_delay = 0;// reset delay when fault came back again
			}
			
			if(recovery_delay > RECOVERY_PERIOD){
				// Unlock the motor driver
				GPIO_OCTL(GPIOA) |= (1 << nSLEEP_Pin);
				vmon_fsm = 0;
			}
			
		}
		break;
	}
	
}

uint8_t app_mon_faultDetected(){
	return vmon_fsm ? 1 : 0;
}