#include <stdint.h>
#include <stdio.h>
#include "stm32f4xx.h"

#include "uart2.h" // for debug
#include "uart6.h" // for bluetooth
#include "systick.h"
#include "tim.h"
#include "flash_if.h"
#include "metadata.h"
#include "protocol.h"
#include "iwdg.h"

/*
 *  Sector 0-1 bootloader 32kb
 *  Sector 2 metadata 16kb
 *  sector 3-5 Slot A 205kb
 *  sector 6-7 Slot B 256 kb
 *
 *  MAX FIRMWARE SIZE = 205KB
 */
void gpioc_pc10_init(){
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
	GPIOC->MODER &= ~(3U << (10 * 2));
	GPIOC->PUPDR &= ~(3U << (10 * 2));
	GPIOC->PUPDR |=  (2U << (10 * 2)); // pull down

}

void jump_to_slot(int slot) {
    uint32_t app_address = (slot==0) ? 0x0800C000 : 0x08040000;
    uint32_t appStack=*(volatile uint32_t*)app_address;
    uint32_t reset_handler_address = *(volatile uint32_t*)(app_address + 4);
    void (*app_reset_handler)(void) = (void *)reset_handler_address;

    if((appStack>=0x20000000)&&(appStack<=0x20018000)){
    	 __disable_irq();

    	 SysTick->CTRL = 0;
    	 SysTick->LOAD = 0;
    	 SysTick->VAL  = 0;

    	 for(int i=0;i<8;i++){
    	     NVIC->ICER[i] = 0xFFFFFFFF;
    	     NVIC->ICPR[i] = 0xFFFFFFFF;
    	 }
    	printf("Reset handler: 0x%08lX\r\n", reset_handler_address);
    	printf("MSP: 0x%08lX\r\n", appStack);
    	//printf("Jumping to APP\n\r");
    	SCB->VTOR=app_address;
    	__set_MSP(appStack);
    	IWDG_Feed();
    	app_reset_handler();
    }

}

void Bootloader_Check_And_Jump(void) {
    const volatile BootMetadata_t *meta = read_metadata();
    uint8_t trial_slot;

    // Trigger Update Mode if PC10 is HIGH
    if (GPIOC->IDR & (1U << 10)) {
        printf("Entering Update Mode via Pin...\r\n");
        return;
    }
    IWDG_Init(4000); // enable watchdog only if not in update mode
    if (meta->pending == 1) {
        // We are in a Trial state. Attempt to boot the OPPOSITE slot.
        trial_slot = (meta->active_slot == 0) ? 1 : 0;

        if (meta->boot_tries < 3) {
            Write_Metadata_To_Flash(meta->active_slot,meta->pending, meta->boot_tries +1);
            printf("Trial Boot Slot %d (Attempt %d)\r\n", trial_slot, meta->boot_tries);
            IWDG_Feed(); // WDG feed cuz flash write consume large time
            jump_to_slot(trial_slot);
        } else {
            // Max tries exceeded , New firmware is bad
            // Reset pending and go back to the original safe active_slot.
        	Write_Metadata_To_Flash(meta->active_slot,0, 0);
            printf("Trial Failed. Rolling back to Slot %d\r\n", meta->active_slot);
            IWDG_Feed();
            jump_to_slot(meta->active_slot);
        }
    } else {
        // Standard Boot
        printf("Booting Active Slot %d\r\n", meta->active_slot);
        jump_to_slot(meta->active_slot);
    }


}

void Check_Watchdog_reset(void) {
    const volatile BootMetadata_t *meta = read_metadata();

    // Check if watchdog triggered the reset
    if (RCC->CSR & RCC_CSR_IWDGRSTF) {
    	RCC->CSR |= RCC_CSR_RMVF; // clear reset flags
        printf("System recovered from Watchdog Reset!\r\n");

        if (meta->pending == 0) {
            // Stable app crashed randomly ,  We just boot it again.
            printf("Stable app crashed , Restarting active slot %d...\r\n", meta->active_slot);
            jump_to_slot(meta->active_slot);
        }
    }

}

int main(void) {
	uint8_t slot;

	RCC->AHB1ENR |= (1U << 0);
	GPIOA->MODER &= ~(3U << (5 * 2)); // Clear bits
	GPIOA->MODER |=  (1U << (5 * 2)); // Set to 01
	GPIOA->ODR |= (1U << 5);

	gpioc_pc10_init(); // pin used to enter bootloader update mode , logic high to enter
	uart2_rxtx_init();

	Check_Watchdog_reset();

	Bootloader_Check_And_Jump();

	// UPDATION MODE STARTS HERE
	uart6_rxtx_init();
	systickDelayMs_polling(100);
	printf("Inside bootloader Update Mode\n\r");
	const volatile BootMetadata_t *meta = read_metadata();
	printf("Initial Metadata: magic %lX ,active %d ,pending %d ,boot_tries %d \r\n",meta->magic,meta->active_slot,meta->pending,meta->boot_tries);
	slot=comm_init();
	if(start_protocol()){
	    printf("Firmware crc verified , marking pending and trying to boot to new firmware\r\n");
	    Write_Metadata_To_Flash(meta->active_slot,1, 0);
	    NVIC_SystemReset();

	}
	else{
		printf("FIRMWARE CRC WRONG, rebooting \r\n");
		NVIC_SystemReset();
	}

	while(1){


	}


}
