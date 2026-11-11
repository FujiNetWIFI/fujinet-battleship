#include <peekpoke.h>
#include <stdlib.h>
#include <atari.h>

#include "../misc.h"
#include <fujinet-fuji.h>
#include "../platform-specific/graphics.h"

void resetTimer()
{
    POKEW(0x13, 0);
}

uint16_t getTime()
{
    return PEEK(0x13) * 256 + PEEK(0x14);
}

void quit()
{
    resetScreen(false);
    resetGraphics();
    fuji_set_boot_config(1);
    exit(0);
}

void housekeeping()
{
    // Clear attract mode
    OS.atract = 0;
}

uint8_t getJiffiesPerSecond()
{
    return PEEK(0xD014) == 1 ? 50 : 60;
}

uint8_t getRandomNumber(uint8_t maxExclusive)
{
    return (*(uint8_t *)0xD20A) % maxExclusive;
}