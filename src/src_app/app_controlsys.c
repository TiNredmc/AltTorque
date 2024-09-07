#include "app_controlsys.h"

// Private variable

/////////// Control system /////////////
SFIXPOINT32 p_term = 0;
SFIXPOINT32 i_term = 0;
SFIXPOINT32 d_term = 0;

/////////// Fault monitoring ///////////
uint8_t vmon_fsm = 0;

uint8_t vbus_hysteresis = 0;
uint8_t vreg_hysteresis = 0;
uint8_t mfault_flag 		= 0;

volatile uint8_t recovery_delay = 0;

uint8_t first_read = 0;

// Private typedef
adcD_type_t adc_d_t;
pid_type_t 	pid_t;

// Private prototype
void app_controlsys_commandMotor(int16_t speed);

void app_controlsys_init(){
	pwm_init();
	pwm_setDuty(0);
	
	adc_init((uint32_t *)&adc_d_t);
	adc_softTrigger();// Initial measurement
}

void app_controlsys_config(
	int32_t new_Kp,
	int32_t new_Ki,
	int32_t new_Kd
	){
		
	if(pid_t.Kp != new_Kp)
		pid_t.Kp = (SFIXPOINT16)new_Kp;
	
	if(pid_t.Ki != new_Ki)
		pid_t.Ki = (SFIXPOINT16)new_Ki;
	
	if(pid_t.Kd != new_Kd)
		pid_t.Kd = (SFIXPOINT16)new_Kd;
		
}

uint16_t app_controlsys_getVBus(){
	return adc_d_t.VbusSense;
}

uint16_t app_controlsys_getVReg(){
	return adc_d_t.V3v3Sense;
}

uint16_t app_controlsys_getImotor(){
	return adc_d_t.ImotorSense;
}

// ADC reading to degree w/ fixed point math
uint8_t app_controlsys_getPotDegree(){
	UFIXPOINT32 ADC_FXP = 0;
	UFIXPOINT32 F1=0, F2=0, F3=0, F4=0;
	UFIXPOINT32 FXP_sum = 0;
	
	// Safety net
	if(adc_d_t.VpotSense > 4095)
		adc_d_t.VpotSense = 4095;
	
	// 4095 * 239/4095 -> Convert ADC reading to Potentiometer degree
	// 4095 * 0.0583638 -> Fraction to decimals
	// Now, approximate the 0.0583638 with fixed point math
	// The closed is 1/32 + 1/64 + 1/128 + 1/256 or around 0.05859375
	// ~ 4095 * (1/32 + 1/64 + 1/128 + 1/256)
	// Convert the ADC reading into UQ16.16 fixed point format
	// then do the magic dividing by right shifting
	// then sum all of each part
	// and later truncate it from UQ16.16 to U16
	// But the max number is 239 which is < 255
	// so uint8_t is allowed as long as input < 4096.
	
	ADC_FXP = // Convert Uint16 to UQ16.16
		(UFIXPOINT32)adc_d_t.VpotSense << 16;
	F1 = ADC_FXP >> 5; 
	F2 = ADC_FXP >> 6;
	F3 = ADC_FXP >> 7;
	F4 = ADC_FXP >> 8;
	
	FXP_sum = F1 + F2 + F3 + F4;
	
	return (uint8_t)(FXP_sum >> 16);
}

// Degree to ADC reading w/ fixed point math
void app_controlsys_updateSP(uint8_t degree){
	if(degree > 239)// safety net
		degree = 239;
	
	// degree to ADC reading
	pid_t.setpoint = 
		(SFIXPOINT16)(
			(UFIXPOINT16)(degree * 17) + // Integer part
			(degree >> 3)							// Fractional part
		);
	i_term = 0; // reset integrator
}

void app_controlsys_runner(){
	if(!adc_getDataAvaible())
		return;
	
	if(!first_read){
		first_read = 1;
		pid_t.setpoint = (int16_t)adc_d_t.VpotSense;
	}
	
	// Calculate SP - PV
	pid_t.angle_error = 
		pid_t.setpoint	-
		(SFIXPOINT16)adc_d_t.VpotSense;
	
	// Scale angle error to -1 to (almost) 1 
	// Q1.15
	pid_t.angle_error = pid_t.angle_error << 3;
	
	// Q1.15 * Q1.15 -> Q2.30
	p_term = pid_t.angle_error * pid_t.Kp;
	
	i_term += (pid_t.angle_error * pid_t.Ki);
	
	if(i_term > (INTG_LIMIT *(1 << 13)))
		i_term = (INTG_LIMIT *(1 << 13));
	if(i_term < (-INTG_LIMIT *(1 << 13)))
		i_term = (-INTG_LIMIT *(1 << 13));
	
	d_term = 
		(pid_t.angle_error - pid_t.prevError) * pid_t.Kd;
	
	pid_t.prevError = pid_t.angle_error;
	
	pid_t.output = p_term + i_term + d_term;
	
	// Scale output from Q2.30 down to Q1.9	
	app_controlsys_commandMotor(
		(int16_t)(pid_t.output >> (8+5))
	);
	
	adc_softTrigger();// Trigger next ADC conversion
}

// Monitor VBus and Vreg
void app_controlsys_vmonRunner(){

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

uint8_t app_controlsys_faultDetected(){
	return vmon_fsm ? 1 : 0;
}
	

void app_controlsys_commandMotor(int16_t speed){
	if(speed < -1023)
		speed = -1023;
	if(speed > 1023)
		speed = 1023;
	
	GPIO_OCTL(GPIOA) &= ~(1 << BRAKE_Pin);
	
	if(speed > 0){
		GPIO_OCTL(GPIOA) |= (1 << PHASE_Pin);
		pwm_setDuty(speed);
	}else if (speed < 0){
		GPIO_OCTL(GPIOA) &= ~(1 << PHASE_Pin);
		pwm_setDuty(-speed);
	}else{
		GPIO_OCTL(GPIOA) |= (1 << BRAKE_Pin);
		pwm_setDuty(0);
	}
	
}