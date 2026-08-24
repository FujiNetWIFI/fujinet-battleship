/*
  Platform specific utilities - Coleco Adam

  The VDP vblank is wired to NMI on the Adam; the CRT's NMI handler calls
  jiffyTick() (installed via add_raster_int in initGraphics), which drives
  getTime()/waitvsync().
*/

#include "../misc.h"
#include <eos.h>

volatile uint16_t jiffyCount = 0;
volatile uint8_t vsyncFlag = 0;

static uint16_t timerBase = 0;
static uint8_t seeded = 0;

void jiffyTick(void)
{
    // jiffyCount advances in waitvsync() (which counts frames via either
    // this flag or the polled VDP status bit) - don't double count here.
    vsyncFlag = 1;
}

void resetTimer()
{
    timerBase = jiffyCount;
}

uint16_t getTime()
{
    return jiffyCount - timerBase;
}

uint8_t getJiffiesPerSecond()
{
    return 60;
}

uint8_t getRandomNumber(uint8_t maxExclusive)
{
    if (!seeded)
    {
        seeded = 1;
        srand(jiffyCount);
    }
    return (uint8_t)(rand() % maxExclusive);
}

void housekeeping()
{
    // Not needed on Adam
}

void quit()
{
    resetGraphics();
    eos_exit_to_smartwriter();
}
