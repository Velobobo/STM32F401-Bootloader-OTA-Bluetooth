#include "iwdg.h"

void IWDG_Init(uint16_t ms) {
    //Enable Write access to IWDG_PR and IWDG_RLR registers
    IWDG->KR = 0x5555;

    // Set Prescaler to 32 (LSI 32kHz / 32 = 1000 ticks per second)
    // 0x03 corresponds to a divider of 32
    IWDG->PR = 0x03;

    // Set Reload value
    // Since 1 tick = 1ms, the value is simply the 'ms' parameter
    // Range is 0 to 4095 (Max ~4 seconds with this prescaler)
    IWDG->RLR = (ms & 0x0FFF);

    IWDG->KR = 0xAAAA; // Reload the counter
    IWDG->KR = 0xCCCC; // Start the Watchdog
}


void IWDG_Feed(void) {
    IWDG->KR = 0xAAAA;
}
