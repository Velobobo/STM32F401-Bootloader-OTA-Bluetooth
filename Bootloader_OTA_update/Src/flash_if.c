#include "stm32f4xx.h"

void Flash_Direct_Unlock(void) {
	FLASH->SR = FLASH_SR_EOP | FLASH_SR_WRPERR | FLASH_SR_PGAERR | FLASH_SR_PGPERR | FLASH_SR_PGSERR; // clearing all flash errors
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = 0x45670123;
        FLASH->KEYR = 0xCDEF89AB;
    }
}

void Flash_Direct_Lock(void) {
    FLASH->CR |= FLASH_CR_LOCK;
}

void Flash_Direct_Erase_Sector(uint8_t type) {
    Flash_Direct_Unlock();

    uint8_t start_sector, num_sectors;

    if (type == 0) {
        start_sector = 3; num_sectors = 3; // Sectors 3, 4, 5 SLOT A
    } else if (type == 1) {
        start_sector = 6; num_sectors = 2; // Sectors 6, 7 SLOT B
    } else if (type == 2) {
        start_sector = 2; num_sectors = 1; // Sector 2 only METADATA
    } else {
        Flash_Direct_Lock();
        return;
    }

    for (uint8_t i = 0; i < num_sectors; i++) {
        uint8_t current_sector = start_sector + i;

        while (FLASH->SR & FLASH_SR_BSY);

        FLASH->CR &= ~(FLASH_CR_SNB);
        FLASH->CR |= (current_sector << FLASH_CR_SNB_Pos) | FLASH_CR_SER;
        FLASH->CR |= FLASH_CR_STRT;

        while (FLASH->SR & FLASH_SR_BSY);
        FLASH->CR &= ~FLASH_CR_SER;
    }

    Flash_Direct_Lock();
}

void Flash_Direct_Write(uint32_t address, uint32_t *data, uint32_t word_count) {
    Flash_Direct_Unlock();

    for (uint32_t i = 0; i < word_count; i++) {
        while (FLASH->SR & FLASH_SR_BSY);

        // Set PSIZE and PG bit
        FLASH->CR &= ~(FLASH_CR_PSIZE);
        FLASH->CR |= (0x02 << FLASH_CR_PSIZE_Pos) | FLASH_CR_PG;

        *(__IO uint32_t*)address = data[i];

        while (FLASH->SR & FLASH_SR_BSY);
        __DSB();
        // Clear PG bit AFTER EVERY WORD
        FLASH->CR &= ~FLASH_CR_PG;

        address += 4;
    }
    Flash_Direct_Lock();
}




