#include "stm32f4xx.h"
#include "systick.h"

#define SYSTICK_LOAD_VAL 16000
// 16Mhz sysclk , so for 1 ms delay , 16000 cycles
#define CTRL_COUNTFLAG (1U<<16)

void systickDelayMs_polling(int delay){
	SysTick->LOAD = SYSTICK_LOAD_VAL; // reload with number of clocks per millisecond
	SysTick->VAL=0; // clear systick current value register
	// enable systick and select internal clock source
	SysTick->CTRL|= (1<<0) | (1<<2) ;

	for(int i=0;i<delay;i++){
		// wait until COUNTFLAG is set
		while(!(SysTick->CTRL & CTRL_COUNTFLAG)){}
	}
	SysTick->CTRL=0; // stop systick
}


void systick_1hz_interrupt(void){
	SysTick->LOAD = 16000000-1; // reload with number of clocks per second
	SysTick->VAL=0; // clear systick current value register
	// enable systick and select internal clock source and enable systick interrupt
	SysTick->CTRL|= (1<<0) | (1<<1) | (1<<2);

}

