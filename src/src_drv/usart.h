#ifndef USART_H
#define USART_H

#include "gd32f1x0.h"
#include "gd32f1x0_usart.h"


enum SERVO_COM_SPEED{
	SLOW = 0,		// 38400
	MEDM,				// 115200
	FAST				// 230400
};

void usart_init(uint8_t com_speed);

// RX stuffs
void usart_setRxPtr(uint8_t *rxSetPtr);
void usart_setRxLen(uint32_t rxLength);
uint32_t usart_getRxIdx();
void usart_resetRxIdx();

// TX stuffs
void usart_enableTx();
void usart_disableTx();
uint8_t usart_getTXPhyReady();
uint8_t usart_getTxEmpty();
uint8_t usart_getTxComplete();
void usart_write(uint8_t w_data);

#endif