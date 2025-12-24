/*
  Apple II Utilities - Prototype version
*/

#include <stdint.h>
#include "../platform-specific/util.h"
void hires_Done(void);
void prodosQuit(void);

// External counter defined in waitvsync.s
extern uint16_t vsyncCounter;

void resetTimer()
{
    vsyncCounter = 0;
}

uint16_t getTime()
{
    return vsyncCounter;
}

void quit()
{
    hires_Done();
    prodosQuit();
}

void housekeeping()
{
    // Not implemented in prototype
}

uint8_t getJiffiesPerSecond()
{
    return 60; // Assume 60Hz for prototype
}

uint8_t getRandomNumber(uint8_t maxExclusive)
{
    // Simple pseudo-random - not secure but works for prototype
    static uint16_t seed = 1;
    seed = seed * 1103515245 + 12345;
    return (uint8_t)(seed % maxExclusive);
}