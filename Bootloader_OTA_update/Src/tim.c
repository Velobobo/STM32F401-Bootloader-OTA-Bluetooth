#include "stm32f4xx.h"
#include "tim.h"


void tim2_1hz_init(void){
	RCC->APB1ENR|=1<<0; // enable clock access
	// set prescaler value
	TIM2->PSC= 1600-1 ; // 16000000/1600=10000
	// set auto reload register
	TIM2->ARR=10000-1; // 10000/10000 = 1
	TIM2->CNT=0; //clear
	TIM2->CR1= 1<<0; // enable timer
}

void tim2_1hz_interrupt_init(void){
	__disable_irq();
	RCC->APB1ENR|=1<<0; // enable clock access
	// set prescaler value
	TIM2->PSC= 1600-1 ; // 16000000/1600=10000
	// set auto reload register
	TIM2->ARR=10000-1; // 10000/10000 = 1
	TIM2->CNT=0; //clear
	TIM2->CR1|= 1<<0; // enable timer
	TIM2->DIER|=1<<0; // enable interrupt
	NVIC_EnableIRQ(TIM2_IRQn);
	__enable_irq();
}
