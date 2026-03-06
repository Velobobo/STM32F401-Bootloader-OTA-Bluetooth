/*
 * uart6.h
 *
 *  Created on: Feb 10, 2026
 *      Author: parth
 */

#ifndef UART6_H_
#define UART6_H_
#include <stdint.h>
#include "stm32f4xx.h"

void uart6_rxtx_init(void);
void uart6_print(const char* str);
char uart6_read(void);
void uart6_write(int ch);

#endif /* UART6_H_ */
