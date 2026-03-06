/*
 * iwdg.h
 *
 *  Created on: Mar 6, 2026
 *      Author: parth
 */

#ifndef IWDG_H_
#define IWDG_H_
#include "stm32f4xx.h"

void IWDG_Init(uint16_t ms);
void IWDG_Feed(void);

#endif /* IWDG_H_ */
