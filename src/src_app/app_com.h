#ifndef APP_COM_H
#define APP_COM_H

#include <string.h>
#include "usart.h"

#include "app_controlsys.h"
#include "app_nvm.h"


typedef struct __attribute__((packed)){
	uint8_t Header[2];
	
	uint8_t ID;
	uint8_t Command;
	union{
		uint8_t 	Argument8[4];
		uint16_t 	Argument16[2];
		uint32_t 	Argument32;
	};
	
	uint8_t chksum;
	
	uint8_t Ender;
}servo_packet_t;

enum COM_FSM{
	COM_RECV = 0,
	COM_TRAN
};

void app_comInit(uint8_t self_id);
void app_comPoll();

#endif