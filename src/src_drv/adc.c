#include "adc.h"

// Private pointers
uint32_t *adc_buffer;

// Private variables
volatile uint32_t dma_stat = 0;

volatile uint8_t adc_data_available = 0;

void DMA_Channel0_IRQHandler(void){
	dma_stat = DMA_INTF;

	// Check for global interrupt of CH0
	if(dma_stat & (1 << 0)){
		// Check for full transfer
		if(dma_stat & (1 << 1)){
			adc_data_available = 1;
		}	
	}
	
	DMA_INTC = 0x00000001;// Clear all CH0 flag

}

void adc_init(uint32_t *data_ptr){
	uint32_t cycle_delay = 0x1000000;
	
	if(data_ptr == 0)
		return;
	
	adc_buffer = data_ptr;
	
	RCU_AHBEN 	|= (1 << 0);// Enable DMA clock
	RCU_APB2EN 	|= (1 << 9);// Enable ADC clock
		
	ADC_CTL0 |= (1 << 8);// Enable scan mode
	
	ADC_CTL1 |= 
		(1 << 20)		| // Enable external trigger
		(7 << 17)		| // Use software trigger (for now)
		(1 << 8)		; // Enable DMA request
	
	// Setup ADC conversion scan sequence
	ADC_RSQ0 = ((4-1) << 20);// Set scan length to 4
	
	ADC_RSQ2 =
		(VRegSen_Pin << 0)		| // V3v3 sense
		(VBusSen_Pin << 5)		|	// VBus sense
		(VISen_Pin 	 << 10)		|	// Motor current sense
		(VPotSen_Pin << 15)		;	// Position pot sense
	
	// Set all channel sampling time to 239.5 cycles
	// total sample time is (239.5 + 12.5) -> 252 cycles
	// which is 252/14Mhz = 18us
	ADC_SAMPT1 =
		(7 << (VRegSen_Pin * 3))		|	
		(7 << (VBusSen_Pin * 3))		|
		(7 << (VISen_Pin * 3))			|
		(7 << (VPotSen_Pin * 3))		;
		
	ADC_CTL1 |= (1 << 0);// Turn ADC on
	while(cycle_delay--);
		
	ADC_CTL1 |= (1 << 3);// Reset calibration 
	while((ADC_CTL1 & (1 << 3)));// wait until cal reset
		
		
	ADC_CTL1 |= (1 << 2);// Start calibration routine
	while((ADC_CTL1 & (1 << 2)));// wait until cal routine finished
	
	// DMA configuration
	
	DMA_CH0PADDR	= (uint32_t)(&ADC_RDATA);
	DMA_CH0MADDR 	= (uint32_t)((uint32_t *)adc_buffer);
	
	DMA_CH0CNT		= 4;// 4 channels -> 4 data count
	
	DMA_CH0CTL |= 
		(3 << 12)		| // Ultra high priority
		(1 << 10)		| // 16 bit data buffer width
		(1 << 8)		|	// 16 bit peripheral data width
		(1 << 7)		|	// Memory incremental mode
		(1 << 5)		| // Circular mode
		(1 << 1)		; // Channel full transfer interrupt
	
	DMA_CH0CTL |= 1;// Enable DMA Channel 0
	
	NVIC_SetPriority(DMA_Channel0_IRQn, 1);
	NVIC_EnableIRQ(DMA_Channel0_IRQn);	
}

void adc_softTrigger(){
	ADC_CTL1 |= (1 << 22);
}

uint8_t adc_getDataAvaible(){
	if(adc_data_available == 0)
		return 0;
	
	adc_data_available = 0;
	
	return 1;
}