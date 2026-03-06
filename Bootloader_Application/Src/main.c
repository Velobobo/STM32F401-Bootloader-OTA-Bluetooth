#include <stdint.h>
#include <stdio.h>
#include "stm32f4xx.h"

#include "uart2.h"
#include "systick.h"
#include "iwdg.h"
#include "metadata.h"
/*
 *  Sector 0-1 bootloader 32kb
 *  Sector 2 metadata 16kb
 *  sector 3-5 Slot A 205kb
 *  sector 6-7 Slot B 256 kb
 *
 *  MAX FIRMWARE SIZE = 205KB
 */
void confirm_boot(void) {
    const volatile BootMetadata_t *meta = read_metadata();
    if (meta->pending == 1) {
        // Trial was successful
        // Update active_slot to the one we are currently running in
        // Set pending to 0 , Reset boot_tries to 0
        uint8_t current_slot = (meta->active_slot == 0) ? 1 : 0;
        Write_Metadata_To_Flash(current_slot, 0,0);
        printf("Boot Confirmed! Slot %d is now Active.\r\n", current_slot);
    }
}

int main(void)
{
	SCB->VTOR = APP_START_ADDR;
	__enable_irq();

	RCC->AHB1ENR |= (1U << 0);
	GPIOA->MODER &= ~(3U << (5 * 2)); // Clear bits
	GPIOA->MODER |=  (1U << (5 * 2)); // Set to 01
	GPIOA->ODR |= (1U << 5);

	uart2_rxtx_init();
	printf("Jumped to FIRMWARE v1.0\r\n");
	confirm_boot();
	while(1){
		IWDG_Feed();
		systickDelayMs_polling(100);
		GPIOA->ODR ^= (1U << 5);

	}


}

