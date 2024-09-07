#include "app_com.h"

// Private typedef
servo_packet_t recv_packet_t;
#define COM_RX_LEN 	10
servo_packet_t send_packet_t;
#define COM_TX_LEN 	10

// Private variables
uint8_t servo_self_id = 0;

uint8_t app_com_main_fsm = 0;
uint8_t app_com_sync_fsm = 0;


uint8_t rx_buffer[32] = {0};

uint8_t app_com_tx_fsm = 0;
uint8_t tx_req_len = 0;
uint8_t tx_write_idx = 0;

uint8_t command_status = 0;

// private prototypes
void app_comHeaderSync();
void app_comProcRx();
void app_comTxRequest(uint8_t len);
uint8_t app_comTxPreply();


// Init communication thingy
void app_comInit(uint8_t self_id){
	usart_init(MEDM);
	usart_setRxPtr(
		rx_buffer
	);
	usart_setRxLen(32);
	
	servo_self_id = self_id;
}

// communication FSM
void app_comPoll(){

	switch(app_com_main_fsm){
		case COM_RECV:
		{
			app_comHeaderSync();
//			if(usart_getRxIdx() != COM_RX_LEN)
//				return;
//			
//			usart_resetRxIdx();
//			
//			memcpy(
//				(uint8_t *)&recv_packet_t,
//				rx_buffer,
//				COM_RX_LEN
//			);	
//			
//			app_comProcRx();
		}
		break;
		
		case COM_TRAN:
		{
			if(app_comTxPreply()){
				
				app_com_main_fsm = COM_RECV;
			}
		}
		break;
		
	}

}

uint8_t app_comCheckSum(servo_packet_t *ptr_t){
	return (
		ptr_t->ID						+
		ptr_t->Command 			+
		ptr_t->Argument8[0]	+
		ptr_t->Argument8[1]	+
		ptr_t->Argument8[2]	+
		ptr_t->Argument8[3]
	);
}

void app_comHeaderSync(){
	
	switch(app_com_sync_fsm){
		case 0:
		{
			if(usart_getRxIdx() != 0){
				if(rx_buffer[0] != 'R'){
					usart_resetRxIdx();
				}else{
					app_com_sync_fsm = 1;
				}
			}
		}
		break;
	
		case 1:
		{
			if(usart_getRxIdx() == 2){
				if(rx_buffer[1] != 'B'){
					usart_resetRxIdx();
					app_com_sync_fsm = 0;
				}else{
					app_com_sync_fsm = 2;
				}
			}
		}
		break;
		
		case 2:
		{
			if(usart_getRxIdx() == COM_RX_LEN){
				usart_resetRxIdx();
				
				memcpy(
					(uint8_t *)&recv_packet_t,
					rx_buffer,
					COM_RX_LEN
				);	
				
				app_comProcRx();
				app_com_sync_fsm = 0;
			}
			
			if(usart_getRxIdx() > COM_RX_LEN){
				usart_resetRxIdx();
				app_com_sync_fsm = 0;
			}
		}
		break;
	}
	
}

// Process RX data
void app_comProcRx(){
	
	app_com_main_fsm = COM_RECV;
	command_status = 0xFF;
	send_packet_t.Argument8[0] = 'F';
	send_packet_t.Argument8[1] = 'A';
	send_packet_t.Argument8[2] = 'I';
	send_packet_t.Argument8[3] = 'L';
	
	// "RBC" check
	// Check for "RB" in header
	if(recv_packet_t.Header[0] != 'R')
		return;
	if(recv_packet_t.Header[1] != 'B')
		return;
	// Check for 'C' in Ender
	if(recv_packet_t.Ender != 'C')
		return;
	
	// Check for ID
	if(recv_packet_t.ID != servo_self_id)
		return;
	
	// Check for checksum match
//	if(recv_packet_t.chksum != app_comCheckSum(&recv_packet_t))
//		goto com_rx_error;
	
	
	// Command decoder
	switch(recv_packet_t.Command){
		case 'N':// No operation command
		{
			command_status = 0x00;
			send_packet_t.Argument8[0] = 'N';
			send_packet_t.Argument8[1] = 'O';
			send_packet_t.Argument8[2] = 'P';
			send_packet_t.Argument8[3] = ' ';
		}
		break;
		
		case 'M':// Move absolute
		{
			app_controlsys_updateSP(recv_packet_t.Argument8[0]);
			
			command_status = 0x00;
			send_packet_t.Argument8[0] = 'D';
			send_packet_t.Argument8[1] = 'O';
			send_packet_t.Argument8[2] = 'N';
			send_packet_t.Argument8[3] = 'E';
		}	
		break;
		
		case 'm':// Move relative
		{
			app_controlsys_updateSP(
				recv_packet_t.Argument8[0] + 
				app_controlsys_getPotDegree()
			);
			
			command_status = 0x00;
			send_packet_t.Argument8[0] = 'D';
			send_packet_t.Argument8[1] = 'O';
			send_packet_t.Argument8[2] = 'N';
			send_packet_t.Argument8[3] = 'E';
		}
		break;
		
		case 'S':// Spin with certain RPM
		{
			command_status = 0x00;
			send_packet_t.Argument8[0] = 'D';
			send_packet_t.Argument8[1] = 'O';
			send_packet_t.Argument8[2] = 'N';
			send_packet_t.Argument8[3] = 'E';
		}
		break;
		
		case 'X':// Get current position
		{
			send_packet_t.Argument32 = app_controlsys_getPotDegree();
			
			command_status = 0x00;
		}
		break;
		
		case 'W':// Get current speed
		{
			send_packet_t.Argument32 = 1;
			
			command_status = 0x00;
		}
		break;
		
		case 'Q':// Update ID number
		{
			if(
				(recv_packet_t.Argument8[0] != 0xFF) ||
				(recv_packet_t.Argument8[0] != 0x00)
			){
				app_nvm_setSelfID(recv_packet_t.Argument8[0]);
				command_status = app_nvm_updateData();
				if(command_status == 0){
					servo_self_id = app_nvm_getSelfID();
					send_packet_t.Argument8[0] = 'D';
					send_packet_t.Argument8[1] = 'O';
					send_packet_t.Argument8[2] = 'N';
					send_packet_t.Argument8[3] = 'E';
					break;
				}
			}
									
			send_packet_t.Argument8[0] = 'F';
			send_packet_t.Argument8[1] = 'A';
			send_packet_t.Argument8[2] = 'I';
			send_packet_t.Argument8[3] = 'L';
		}
		break;
		
		case 'P':// Set Kp
		{
			app_nvm_setSelfKp(recv_packet_t.Argument32);
			command_status = app_nvm_updateData();
			if(command_status == 0){
				app_controlsys_config(
					app_nvm_getSelfKp(),
					app_nvm_getSelfKi(),
					app_nvm_getSelfKd()
				);
				send_packet_t.Argument8[0] = 'D';
				send_packet_t.Argument8[1] = 'O';
				send_packet_t.Argument8[2] = 'N';
				send_packet_t.Argument8[3] = 'E';
			}else{
				send_packet_t.Argument8[0] = 'F';
				send_packet_t.Argument8[1] = 'A';
				send_packet_t.Argument8[2] = 'I';
				send_packet_t.Argument8[3] = 'L';
			}
		}
		break;
		
		case 'I':// Set Ki
		{
			app_nvm_setSelfKi(recv_packet_t.Argument32);
			command_status = app_nvm_updateData();
			if(command_status == 0){
				app_controlsys_config(
					app_nvm_getSelfKp(),
					app_nvm_getSelfKi(),
					app_nvm_getSelfKd()
				);
				send_packet_t.Argument8[0] = 'D';
				send_packet_t.Argument8[1] = 'O';
				send_packet_t.Argument8[2] = 'N';
				send_packet_t.Argument8[3] = 'E';
			}else{
				send_packet_t.Argument8[0] = 'F';
				send_packet_t.Argument8[1] = 'A';
				send_packet_t.Argument8[2] = 'I';
				send_packet_t.Argument8[3] = 'L';
			}
		}
		break;
		
		case 'D':// Set Kd
		{
			app_nvm_setSelfKd(recv_packet_t.Argument32);
			command_status = app_nvm_updateData();
			if(command_status == 0){
				app_controlsys_config(
					app_nvm_getSelfKp(),
					app_nvm_getSelfKi(),
					app_nvm_getSelfKd()
				);
				send_packet_t.Argument8[0] = 'D';
				send_packet_t.Argument8[1] = 'O';
				send_packet_t.Argument8[2] = 'N';
				send_packet_t.Argument8[3] = 'E';
			}else{
				send_packet_t.Argument8[0] = 'F';
				send_packet_t.Argument8[1] = 'A';
				send_packet_t.Argument8[2] = 'I';
				send_packet_t.Argument8[3] = 'L';
			}
		}
		break;
		
		case 'p':// Ping test, reply with "PONG"
		{
		send_packet_t.Argument8[0] = 'P';
		send_packet_t.Argument8[1] = 'O';
		send_packet_t.Argument8[2] = 'N';
		send_packet_t.Argument8[3] = 'G';
		}
		break;
		
		
		default:// Unknow 
		{
			send_packet_t.Argument8[0] = 'U';
			send_packet_t.Argument8[1] = 'N';
			send_packet_t.Argument8[2] = 'K';
			send_packet_t.Argument8[3] = 'W';
		}
		break;
	}
	
	send_packet_t.Header[0] = 'A';
	send_packet_t.Header[1] = 'J';
	send_packet_t.Ender			= 'B';
	send_packet_t.ID				= servo_self_id;

	send_packet_t.chksum = 
		app_comCheckSum(&send_packet_t);

	app_comTxRequest(COM_TX_LEN);
	
	app_com_main_fsm = COM_TRAN;
}

void app_comTxRequest(uint8_t len){
	if(app_com_tx_fsm != 0)
		return;
	
	tx_req_len = len;
}

uint8_t app_comTxPreply(){
	uint8_t ret;
	
	switch(app_com_tx_fsm){
		case 0:// Check if we got write request
		{
			if(tx_req_len > 0){
				usart_enableTx();// Enable TX
				app_com_tx_fsm = 1;
				ret = 0;
			}
		}
		break;
		
		case 1:// Wait until TX phy is ready to use
		{
			if(usart_getTXPhyReady()){
				tx_write_idx = 0;
				app_com_tx_fsm = 2;
				ret = 0;
			}
		}
		break;
		
		case 2:// Write data
		{
			usart_write(
				*((uint8_t *)(
				&send_packet_t) + 
				tx_write_idx)
			);
			
			tx_write_idx++;
			
			app_com_tx_fsm = 3;
			ret = 0;
		}
		break;
		
		case 3:// Wait until Transfer completed
		{
			ret = 0;
			if(usart_getTxComplete()){
				tx_req_len--;
				
				if(tx_req_len == 0){
					usart_disableTx();
					app_com_tx_fsm = 0;
					ret = 1;
				}else{
					app_com_tx_fsm = 2;
				}
				
			}
		}
		break;
		
	}
	
	return ret;
}
