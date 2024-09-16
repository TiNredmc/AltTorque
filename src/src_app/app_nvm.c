#include "app_nvm.h"

// Private typedef

// Constant data to be placed in flash with
// linker script
__IO const nvdata_type_t flash_nvdata_t __attribute__((section (".store_config"))) = {
	.SelfID = 0x01,		// Default ID is 1
	.SelfKp	= 1500,		// signed Q1.15 format, 16 bit data but kept in 32 bit storage
	.SelfKi	= 1,			// signed Q1.15 format, 16 bit data but kept in 32 bit storage
	.SelfKd = 0x0000 	// signed Q1.15 format, 16 bit data but kept in 32 bit storage
};

__IO nvdata_type_t temp_nvdata_t;

void app_nvm_init(){
	temp_nvdata_t.SelfID = flash_nvdata_t.SelfID;
	temp_nvdata_t.SelfKp = flash_nvdata_t.SelfKp;
	temp_nvdata_t.SelfKi = flash_nvdata_t.SelfKi;
	temp_nvdata_t.SelfKd = flash_nvdata_t.SelfKd;
}

uint8_t app_nvm_checkBlank(){
	if(
	(flash_nvdata_t.SelfID == 0xFFFFFFFF) ||	
	(flash_nvdata_t.SelfKp == 0xFFFFFFFF) ||
	(flash_nvdata_t.SelfKi == 0xFFFFFFFF) ||
	(flash_nvdata_t.SelfKd == 0xFFFFFFFF)
	)
		return 1;
	else
		return 0;
}

void app_nvm_unlockFlash(){
	// If locked, can be unlocked
	if(FMC_CTL & FMC_CTL_LK){
		// Unlock flash with key
		FMC_KEY = UNLOCK_KEY0;
		FMC_KEY = UNLOCK_KEY1;

		while(FMC_CTL & FMC_CTL_LK);
	}
	
}

void app_nvm_lockFlash(){
	if(FMC_CTL & FMC_CTL_LK)
		return;
	
	FMC_CTL |= FMC_CTL_LK;
}

uint8_t app_nvm_erasePage(){
	if(FMC_CTL & FMC_CTL_LK)
		return 1;
	
	// Wait until not busy
	while((FMC_STAT & FMC_STAT_BUSY));

	FMC_CTL |= FMC_CTL_PER;// Set page erase command
	
	FMC_ADDR = CONFIG_ADDR;// Erase page 63 
	
	FMC_CTL |= FMC_CTL_START;// Set start erase flag
	
	// Wait until not busy (erased)
	while((FMC_STAT & FMC_STAT_BUSY));
	
	// Wait until operation ended. 
	while(!(FMC_STAT & FMC_STAT_ENDF));
	
	// Clear end flag
	FMC_STAT |= FMC_STAT_ENDF;
	
	FMC_CTL &= ~FMC_CTL_PER;
	
	return 0;
}

uint8_t app_nvm_writeData(){
	if(FMC_CTL & FMC_CTL_LK)
		return 1;
	
	// Wait until not busy
	while((FMC_STAT & FMC_STAT_BUSY));
	
	FMC_CTL |= FMC_CTL_PG;// Set flash program command
	
	// Write from temp to flash
	{
		NVM_DATA->SelfID = temp_nvdata_t.SelfID;
		__ISB();
		// Detect for flash program error
		if(FMC_STAT & FMC_STAT_PGERR)
			goto bad_exit;
	}
	
	{
		NVM_DATA->SelfKp = temp_nvdata_t.SelfKp;
		__ISB();
		// Detect for flash program error
		if(FMC_STAT & FMC_STAT_PGERR)
			goto bad_exit;
	}
	
	{
		NVM_DATA->SelfKi = temp_nvdata_t.SelfKi;
		__ISB();
		// Detect for flash program error
		if(FMC_STAT & FMC_STAT_PGERR)
			goto bad_exit;
	}
	
	{
		NVM_DATA->SelfKd = temp_nvdata_t.SelfKd;	
		__ISB();
		// Detect for flash program error
		if(FMC_STAT & FMC_STAT_PGERR)
			goto bad_exit;
	}	
	
	// Wait until not busy
	while((FMC_STAT & FMC_STAT_BUSY));
	
	// Wait until operation ended. 
	while(!(FMC_STAT & FMC_STAT_ENDF));
	
	// Clear end flag
	FMC_STAT |= FMC_STAT_ENDF;
	
	FMC_CTL &= ~FMC_CTL_PG;

	return 0;
	
bad_exit:	
	FMC_STAT |= FMC_STAT_PGERR;// Clear flag
	app_nvm_lockFlash();// Safety lock
	return 1;
}

uint8_t app_nvm_verifyData(){
	if(NVM_DATA->SelfID != temp_nvdata_t.SelfID)
		return 1;
	
	if(NVM_DATA->SelfKp != temp_nvdata_t.SelfKp)
		return 1;
	
	if(NVM_DATA->SelfKi != temp_nvdata_t.SelfKi)
		return 1;
	
	if(NVM_DATA->SelfKd != temp_nvdata_t.SelfKd)
		return 1;
	
	return 0;	
}

uint8_t app_nvm_updateData(){
	uint8_t ret = 0;
	// Entering critical section
	__disable_irq();
	app_nvm_unlockFlash();
	
	if(app_nvm_erasePage()){
		ret = 1;
	}else if (app_nvm_writeData()){
		ret =  2;
	}else if (app_nvm_verifyData()){
		ret = 3;
	}
	
	app_nvm_lockFlash();
	__enable_irq();
	return ret;
}

void app_nvm_setSelfID(uint8_t ID){
	if(NVM_DATA->SelfID != ID)
		temp_nvdata_t.SelfID = ID;
}

void app_nvm_setSelfKp(int32_t Kp){
	if(NVM_DATA->SelfKp != Kp)
		temp_nvdata_t.SelfKp = (uint32_t)Kp;
}

void app_nvm_setSelfKi(int32_t Ki){
	if(NVM_DATA->SelfKi != Ki)
		temp_nvdata_t.SelfKi = (uint32_t)Ki;
}

void app_nvm_setSelfKd(int32_t Kd){
	if(NVM_DATA->SelfKd != Kd)
		temp_nvdata_t.SelfKd = (uint32_t)Kd;
}

uint8_t app_nvm_getSelfID(){
	return (uint8_t)NVM_DATA->SelfID;
}

int32_t app_nvm_getSelfKp(){
	return (int32_t)NVM_DATA->SelfKp;
}

int32_t app_nvm_getSelfKi(){
	return (int32_t)NVM_DATA->SelfKi;
}

int32_t app_nvm_getSelfKd(){
	return (int32_t)NVM_DATA->SelfKd;
}