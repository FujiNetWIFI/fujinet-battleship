#ifdef BUILD_SMS

/**
 * @brief   Utility Functions - Sega Master System
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "vars.h"
#include "../platform-specific/graphics.h"
#include "../platform-specific/util.h"

// Frames, counted by crt0's frame interrupt handler (enabled by
// initGraphics()); the game timer is frames since the last resetTimer().
extern volatile unsigned int timer;

static unsigned int timerBase;
static bool seeded;

// One LD HL,(nn): the interrupt cannot land between the two bytes.
static unsigned int frames(void)
{
    return timer;
}

void waitvsync(void)
{
    unsigned char t = (unsigned char)frames();

    while ((unsigned char)frames() == t)
        ;
}

void resetTimer(void)
{
    timerBase = frames();
}

uint16_t getTime(void)
{
    return (uint16_t)(frames() - timerBase);
}

uint8_t getJiffiesPerSecond()
{
    return 60;
}

uint8_t getRandomNumber(uint8_t maxExclusive)
{
    if (!seeded)
    {
        // Seeded from the frame count at the first request, which arrives
        // only after some human-paced input
        seeded = true;
        srand(frames());
    }
    return (uint8_t)(rand() % maxExclusive);
}

void housekeeping()
{
}

// quit() is in fujinet.c, with the rest of the cartridge calls.

#endif /* BUILD_SMS */
