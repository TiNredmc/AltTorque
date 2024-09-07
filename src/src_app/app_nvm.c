#include "app_nvm.h"

// Private typedef

// Constant data to be placed in flash with
// linker script
volatile const nvdata_type_t flash_nvdata_t __attribute__((section (".store_config"))) = {
	.SelfID = 0x01,// Default ID is 1
	.SelfKp	= 800,// signed Q1.15 format, 16 bit data but kept in 32 bit storage
	.SelfKi	= 5,// signed Q1.15 format, 16 bit data but kept in 32 bit storage
	.SelfKd = 0x0000 // signed Q1.15 format, 16 bit data but kept in 32 bit storage
};

nvdata_type_t temp_nvdata_t;

void app_nvm_init(){


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
	if(FMC_CTL & (1 << 7)){
		// Unlock flash with key
		FMC_KEY = 0x45670123;
		FMC_KEY = 0xCDEF89AB;

		while(FMC_CTL & (1 << 7));
	}
	
}

void app_nvm_lockFlash(){
	if(FMC_CTL & (1 << 7))
		return;
	
	FMC_CTL |= (1 << 7);
}

uint8_t app_nvm_erasePage(){
	if(FMC_CTL & (1 << 7))
		return 1;
	
	// Wait until not busy
	while(!(FMC_BUSY & (1 << 0)));

	FMC_ADDR = CONFIG_ADDR;// Erase page 63 
	
	FMC_CTL |= (1 << 1);// Set page erase command
	
	FMC_CTL |= (1 << 6);// Set start erase flag
	
	// Wait until not busy (erased)
	while(!(FMC_BUSY & (1 << 0)));
	
	if(FMC_STAT & (1 << 5)){
		FMC_STAT |= (1 << 5);
		return 0;
	}
	
	return 1;
}

uint8_t app_nvm_writeData(){
	if(FMC_CTL & (1 << 7))
		return 1;
	
	// Wait until not busy
	while(!(FMC_BUSY & (1 << 0)));
	
	FMC_CTL |= (1 << 0);// Set flash program command
	
	// Write from temp to flash
	{
		NVM_DATA->SelfID = temp_nvdata_t.SelfID;
		// Detect for flash program error
		if(FMC_STAT & (1 << 2))
			goto bad_exit;
	}
	
	{
		NVM_DATA->SelfKp = temp_nvdata_t.SelfKp;
		// Detect for flash program error
		if(FMC_STAT & (1 << 2))
			goto bad_exit;
	}
	
	{
		NVM_DATA->SelfKi = temp_nvdata_t.SelfKi;
		// Detect for flash program error
		if(FMC_STAT & (1 << 2))
			goto bad_exit;
	}
	
	{
		NVM_DATA->SelfKd = temp_nvdata_t.SelfKd;	
		// Detect for flash program error
		if(FMC_STAT & (1 << 2))
			goto bad_exit;
	}	
	
	while(!(FMC_BUSY & (1 << 0)));

	return 0;
	
bad_exit:	
	FMC_STAT |= (1 << 2);// Clear flag
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
	app_nvm_unlockFlash();
	
	if(app_nvm_erasePage()){
		return 1;
	}else if (app_nvm_writeData()){
		return 2;
	}else if (app_nvm_verifyData()){
		return 3;
	}
	
	app_nvm_lockFlash();
	
	return 0;
}

void app_nvm_setSelfID(uint8_t ID){
	if(NVM_DATA->SelfID != temp_nvdata_t.SelfID)
		temp_nvdata_t.SelfID = ID;
}

void app_nvm_setSelfKp(int32_t Kp){
	if(NVM_DATA->SelfKp != temp_nvdata_t.SelfKp)
		temp_nvdata_t.SelfKp = (uint32_t)Kp;
}

void app_nvm_setSelfKi(int32_t Ki){
	if(NVM_DATA->SelfKi != temp_nvdata_t.SelfKi)
		temp_nvdata_t.SelfKi = (uint32_t)Ki;
}

void app_nvm_setSelfKd(int32_t Kd){
	if(NVM_DATA->SelfKd != temp_nvdata_t.SelfKd)
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