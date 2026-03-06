#include "crc.h"
#include <stdint.h>
void CRC_Init(void) {
    // Enable CRC clock in AHB1ENR
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;

    // Reset the CRC unit (sets DR to 0xFFFFFFFF)
    CRC->CR |= CRC_CR_RESET;
}

/**
 * @brief Calculates the CRC32 of a memory block
 * @param data: Pointer to the start of the data (must be 4-byte aligned)
 * @param size: Number of 32-bit words (not bytes!)
 * @return Calculated 32-bit CRC
 */
uint32_t CRC_Calculate(uint32_t *data, uint32_t word_count) {
    // 1. Reset the CRC unit to initial state (0xFFFFFFFF)
    CRC->CR |= CRC_CR_RESET;
    // 2. Feed the data into the Data Register (DR)
    for (uint32_t i = 0; i < word_count; i++) {
        CRC->DR = data[i];
    }
    // 3. Return the final result
    return CRC->DR;
}


/**
 * @brief Updates a CRC32 value with a single byte.
 * @param previous_crc: The current CRC state (Initial value: 0xFFFFFFFF)
 * @param byte: The new data byte to process
 * @return The updated 32-bit CRC state

uint32_t crc32_update_byte(uint32_t previous_crc, uint8_t byte) {
    uint32_t crc = previous_crc ^ byte;

    for (uint8_t i = 0; i < 8; i++) {
        if (crc & 1) {
            crc = (crc >> 1) ^ 0xEDB88320;
        } else {
            crc >>= 1;
        }
    }

    return crc;
}
 */

//4-bit Lookup Table for CRC32 (Standard Polynomial: 0xEDB88320)
static const uint32_t crc32_nibble_table[16] = {
    0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC,
    0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
    0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C,
    0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C
};

/*
 * @brief Updates the CRC32 state using a 4-bit LUT (Nibble-wise)
 * @param crc: Current running CRC value
 * @param data: The byte to process
 * @return Updated CRC32 value
 */


uint32_t crc32_update(uint32_t crc, uint8_t data) {
    // Process the Low Nibble (first 4 bits)
    crc = (crc >> 4) ^ crc32_nibble_table[(crc ^ (data >> 0)) & 0x0F];

    // Process the High Nibble (next 4 bits)
    crc = (crc >> 4) ^ crc32_nibble_table[(crc ^ (data >> 4)) & 0x0F];

    return crc;
}



