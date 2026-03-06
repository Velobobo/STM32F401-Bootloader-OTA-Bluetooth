/*
 * crc.h
 *
 *  Created on: Mar 3, 2026
 *      Author: parth
 */

#ifndef CRC_H_
#define CRC_H_

#include "stm32f4xx.h"

void CRC_Init(void);
uint32_t CRC_Calculate(uint32_t *data, uint32_t word_count);
uint32_t crc32_update(uint32_t crc, uint8_t data);

#endif /* CRC_H_ */
