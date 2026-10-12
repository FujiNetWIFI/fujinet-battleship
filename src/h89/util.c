#ifdef __H89__

/**
 * @brief   Utility Functions - Heathkit H89
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 *
 * The H89's clock interrupts every 2 ms, and both HDOS and the CP/M BIOS
 * count the ticks in a word (console.h's TICKS). A frame here is 10 ticks:
 * 50 a second.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "vars.h"
#include "console.h"
#include "../platform-specific/graphics.h"
#include "../platform-specific/util.h"

#define TICKS_PER_FRAME 10

static uint16_t lastFrame, timerBase;
static bool seeded;

void waitvsync(void)
{
    // At least a frame since the last one returned. Drawing over the 9600
    // baud console can take longer than that; the game just runs at the
    // pace of the terminal then.
    while ((uint16_t)(TICKS - lastFrame) < TICKS_PER_FRAME)
        ;
    lastFrame = TICKS;
}

void resetTimer(void)
{
    timerBase = TICKS;
}

uint16_t getTime(void)
{
    return (uint16_t)(TICKS - timerBase) / TICKS_PER_FRAME;
}

uint8_t getJiffiesPerSecond(void)
{
    return 500 / TICKS_PER_FRAME;
}

uint8_t getRandomNumber(uint8_t maxExclusive)
{
    if (!seeded)
    {
        // Seeded at the first request, which comes only after some
        // human-paced input
        seeded = true;
        srand(TICKS);
    }
    return (uint8_t)(rand() % maxExclusive);
}

void housekeeping(void)
{
}

void quit(void)
{
    // There is no H89 lobby to load: back to HDOS or CP/M, with the
    // terminal as it was
    resetGraphics();
    exit(0);
}

#endif /* __H89__ */
