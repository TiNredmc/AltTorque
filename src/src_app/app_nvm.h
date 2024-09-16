#ifndef APP_NVM_H
#define APP_NVM_H

#include "gd32f1x0_fmc.h"

#define CONFIG_ADDR 0x0800FC00

typedef struct{
	uint32_t SelfID;
	uint32_t SelfKp;
	uint32_t SelfKi;
	uint32_t SelfKd;
}nvdata_type_t;

#define NVM_DATA ((__IO nvdata_type_t*)CONFIG_ADDR)


void app_nvm_init();
uint8_t app_nvm_checkBlank();

uint8_t app_nvm_updateData();
void app_nvm_setSelfID(uint8_t ID);
void app_nvm_setSelfKp(int32_t Kp);
void app_nvm_setSelfKi(int32_t Ki);
void app_nvm_setSelfKd(int32_t Kd);

uint8_t app_nvm_getSelfID();
int32_t app_nvm_getSelfKp();
int32_t app_nvm_getSelfKi();
int32_t app_nvm_getSelfKd();

#endif