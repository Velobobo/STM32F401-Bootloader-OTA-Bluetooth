#include <uart2.h>


static uint16_t compute_uart2_brr(uint32_t periphclk,uint32_t baudrate);
void uart2_write(int ch);


int __io_putchar(int ch){
	uart2_write(ch);
	return ch;
}

void uart2_rxtx_init(void){
	__disable_irq();
	//clock access gpioA
	RCC->AHB1ENR|=1<<0;
	// PA2 , PA3 alternate function mode
	GPIOA->MODER|=1<<5; GPIOA->MODER&=~(1<<4);
	GPIOA->MODER|=1<<7; GPIOA->MODER&=~(1<<6);
	// PA2 fucntion type to AF07 uart_tx , PA3 AF07 uart_rx
	GPIOA->AFR[0]|=0b111<<8;
	GPIOA->AFR[0]|=0b111<<12;
	//clock access uart
	RCC->APB1ENR|=1<<17;

	// configure baudrate
	USART2->BRR = compute_uart2_brr(16000000,115200);

	USART2->CR1 |= 1<<3 | 1<<2; //TX,RX enable
	__enable_irq();
	// enable uart
	USART2->CR1 |= 1<<13;
}

static uint16_t compute_uart2_brr(uint32_t periphclk,uint32_t baudrate){
	return (periphclk +(baudrate/2U))/baudrate;
}

char uart2_read(void){
	// make sure receive data register is ready
	while(!(USART2->SR & (1<<5)));
	return USART2->DR;
}

void uart2_write(int ch){
	// make sure transmit data register empty
	while(!(USART2->SR & (1<<7)));
	// write to transmit data register
	USART2->DR=ch & 0xFF;
}


void dma1_stream6_init(uint32_t src,uint32_t dst,uint32_t len){
	// stream 6 , channel 4 USART2_TX
	// enable clock access dma
	RCC->AHB1ENR|=1<<21;
	// disable dma1 stream 6
	DMA1_Stream6->CR &= ~(1<<0);
	// advisable to wait until stream it is disabled
	while(DMA1_Stream6->CR & (1<<0));
	// clear all interupt flag of stream 6
	DMA1->HIFCR |= (1<<16) | (1<<18) | (1<<19) | (1<<20) | (1<<21);
	// set the destination buffer
	DMA1_Stream6->PAR=dst;
	// set the source buffer
	DMA1_Stream6->M0AR=src;
	// set length
	DMA1_Stream6->NDTR=len;
	// select channel 4
	DMA1_Stream6->CR |= 1<<27;
	// enable memory increment
	DMA1_Stream6->CR|=1<<10;
	// select transfer direction
	DMA1_Stream6->CR|=1<<6; // memory to peripheral
	// enable transfer complete interrupt
	DMA1_Stream6->CR|=1<<4;
	// enable direct mode , disable fifo
	DMA1_Stream6->FCR=0;
	// enable USART2 TX DMA
	USART2->CR3|=1<<7;
	// enable dma1 stream 6
	DMA1_Stream6->CR |=1<<0;

	NVIC_EnableIRQ(DMA1_Stream6_IRQn);

}

