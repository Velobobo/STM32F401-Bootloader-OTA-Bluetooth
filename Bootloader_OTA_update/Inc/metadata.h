/*
 * metadata.h
 *
 *  Created on: Mar 2, 2026
 *      Author: parth
 */

#ifndef METADATA_H_
#define METADATA_H_
#include "stm32f4xx.h"

typedef struct {
    uint32_t magic;
    uint8_t active_slot;
    uint8_t pending;
    uint8_t boot_tries;
    uint8_t reserved;

} BootMetadata_t;


const volatile BootMetadata_t *read_metadata(void);
void Write_Metadata_To_Flash(uint8_t active, uint8_t pending,uint8_t boot_tries) ;


#endif /* METADATA_H_ */
