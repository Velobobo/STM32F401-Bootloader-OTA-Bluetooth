/*
 * flash_if.h
 *
 *  Created on: Mar 2, 2026
 *      Author: parth
 */

#ifndef FLASH_IF_H_
#define FLASH_IF_H_

void Flash_Direct_Erase_Sector(uint8_t type);
void Flash_Direct_Write(uint32_t address, uint32_t *data, uint32_t word_count);

#endif /* FLASH_IF_H_ */
