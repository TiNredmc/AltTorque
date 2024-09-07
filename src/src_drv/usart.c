#include "usart.h"


// Private Variables
volatile uint32_t usart_stat = 0;
volatile uint32_t rx_idx = 0;
volatile uint32_t rx_max_len = 0;

// Private pointers
uint8_t *rx_ptr;

void USART0_IRQHandler(void){
	usart_stat = USART_STAT(USART0);

	// check for RX not empty 
	if(usart_stat & (1 << 5)){
		if(rx_ptr == 0)
			goto iexit;
		
		*(rx_ptr + rx_idx) = USART_RDATA(USART0);
	
		if(rx_idx < rx_max_len)
			rx_idx++;
		else
			rx_idx = 0;
		
iexit:
		USART_CMD(USART0) = 1 << 3;// Clear RBNE flag
	}
	
	
}

void usart_init(uint8_t com_speed){
	RCU_APB2EN |= (1 << 14);// Enable USART0 clock
	
	USART_CTL0(USART0) |=
		(1 << 15)				|	// Oversampling by 8
		(1 <<  5)				| // RXNE interrupt
		(1 << 3)				| // Enable transmitter rightaway
		(1 <<  2)				; // Enable receiver rightaway
	
	USART_CTL2(USART0) |=
		(1 << 3)				; // Enable Half-duplex mode

	switch(com_speed){
		case SLOW:// 38400 baud
		{
			USART_BAUD(USART0) = 0x09C0;
		}
		break;
		
		case MEDM:// 115200 baud
		{
			USART_BAUD(USART0) = 0x0340;
		}
		break;
		
		case FAST:// 230400 baud
		{
			USART_BAUD(USART0) = 0x01A0;
		}
		break;

		default:// Default to slowest speed
		{
			USART_BAUD(USART0) = 0x09C0;
		}
		break;
	}

	USART_CTL0(USART0) |= (1 << 0);// Enable USART0
	
	NVIC_SetPriority(USART0_IRQn, 3);
	NVIC_EnableIRQ(USART0_IRQn);
}

void usart_setRxPtr(uint8_t *rxSetPtr){
	if(rxSetPtr == 0)
		return;

	rx_ptr = rxSetPtr;
}

void usart_setRxLen(uint32_t rxLength){
	rx_max_len = rxLength;
}

uint32_t usart_getRxIdx(){
	return rx_idx;
}

void usart_resetRxIdx(){
	rx_idx = 0;
}

void usart_enableTx(){
	USART_CTL0(USART0) &= ~(1 << 2);// Disable RX
}

void usart_disableTx(){
	USART_CTL0(USART0) |= (1 << 2);// Enable RX
}


// Check if the TX phy is ready to use
uint8_t usart_getTXPhyReady(){
	return (USART_STAT(USART0) & (1 << 21)) ? 1 : 0;
}

// Check if TX buffer is empty
uint8_t usart_getTxEmpty(){
	return (USART_STAT(USART0) & (1 << 7)) ? 1 : 0;
}

// Check if TX is completed
uint8_t usart_getTxComplete(){
	return (USART_STAT(USART0) & (1 << 6)) ? 1 : 0;
}

void usart_write(uint8_t w_data){
	USART_TDATA(USART0) = w_data;
}