#include "uart6.h"

// PA11 - TX , PA12 - RX

static uint16_t compute_uart_brr(uint32_t periphclk,uint32_t baudrate);
void uart6_write(int ch);


void uart6_print(const char* str){
	while(*str){
		uart6_write(*str++);
	}
}


void uart6_rxtx_init(void){
	__disable_irq();
	//clock access gpioA
	RCC->AHB1ENR|=1<<0;
	// PA11 , PA12 alternate function mode
	GPIOA->MODER&=~(3<<22); GPIOA->MODER|=2<<22;
	GPIOA->MODER&=~(3<<24); GPIOA->MODER|=2<<24;
	// AF-08
	GPIOA->AFR[1]&=~(15<<12);GPIOA->AFR[1]&=~(15<<16);
	GPIOA->AFR[1]|=8<<12;GPIOA->AFR[1]|=8<<16;
	//clock access uart6
	RCC->APB2ENR|=1<<5;

	// configure baudrate
	USART6->BRR = compute_uart_brr(16000000,115200);

	USART6->CR1 |= 1<<3 | 1<<2; //TX,RX enable
	// enable uart
	USART6->CR1 |= 1<<13;
	__enable_irq();

}

void uart6_rxtx_init_interrupt(void){
	__disable_irq();
	//clock access gpioA
	RCC->AHB1ENR|=1<<0;
	// PA11 , PA12 alternate function mode
	GPIOA->MODER&=~(3<<22); GPIOA->MODER|=2<<22;
	GPIOA->MODER&=~(3<<24); GPIOA->MODER|=2<<24;
	// AF-08
	GPIOA->AFR[1]&=~(15<<12);GPIOA->AFR[1]&=~(15<<16);
	GPIOA->AFR[1]|=8<<12;GPIOA->AFR[1]|=8<<16;
	//clock access uart6
	RCC->APB2ENR|=1<<5;

	// configure baudrate
	USART6->BRR = compute_uart_brr(16000000,115200);

	USART6->CR1 |= 1<<3 | 1<<2; //TX,RX enable
	USART6->CR1 |= 1<<5; //RX interrupt enable
	// enable uart
	USART6->CR1 |= 1<<13;
	NVIC_EnableIRQ(USART6_IRQn);
	__enable_irq();

}

static uint16_t compute_uart_brr(uint32_t periphclk,uint32_t baudrate){
	return (periphclk +(baudrate/2U))/baudrate;
}

char uart6_read(void){ // for polling method
	// make sure receive data register is ready
	while(!(USART6->SR & (1<<5)));
	return USART6->DR;
}

void uart6_write(int ch){
	// make sure transmit data register empty
	while(!(USART6->SR & (1<<7)));
	// write to transmit data register
	USART6->DR=ch & 0xFF;
}

