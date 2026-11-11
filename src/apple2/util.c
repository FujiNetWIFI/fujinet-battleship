/*
  Apple II Utilities
*/

#include <stdint.h>
#include <fujinet-fuji.h>
#include "../platform-specific/util.h"

void hires_Done(void);

// External counter defined in waitvsync.s
extern uint16_t vsyncCounter;

void resetTimer() {
    vsyncCounter = 0;
}

uint16_t getTime() {
    return vsyncCounter;
}

void quit() {
    hires_Done();
    fuji_set_boot_mode(FUJI_BOOT_MODE_LOBBY);
    // ROM in, then spoil the reset vector's checksum so the reset is a cold boot
    __asm__ ("bit $c082");
    __asm__ ("dec $03f4");
    __asm__ ("jmp ($fffc)");
}

void housekeeping() {
    ;
}

uint8_t getJiffiesPerSecond() {
    return 60;
}

uint8_t getRandomNumber(uint8_t maxExclusive) {
    // Simple pseudo-random - not secure but works for prototype
    static uint16_t seed = 1;
    seed = seed * 1103515245 + 12345;
    return (uint8_t)(seed % maxExclusive);
}
