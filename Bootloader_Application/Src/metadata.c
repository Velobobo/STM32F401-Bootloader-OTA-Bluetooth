#include "metadata.h"
#include "flash_if.h"

#define MAGIC_BYTES 0xDEADBEEF
#define METADATA_ADDR 0x08008000

const volatile BootMetadata_t *read_metadata(void) {
	const volatile BootMetadata_t *meta = (const volatile BootMetadata_t *)METADATA_ADDR;
	if(meta->magic==MAGIC_BYTES){
		return meta;
	}
	// else metadata corrupted
	return 0;
}

void Write_Metadata_To_Flash(uint8_t active, uint8_t pending,uint8_t boot_tries) {

    BootMetadata_t meta = {
        .magic = MAGIC_BYTES,
        .active_slot = active,
		.pending= pending,
		.boot_tries= boot_tries,
		.reserved=0xFF
    };

    Flash_Direct_Erase_Sector(2); // clear metadata sector
    uint32_t word_count = sizeof(BootMetadata_t) / 4;
    Flash_Direct_Write(METADATA_ADDR, (uint32_t*)&meta, word_count);
}

