#include <stdlib.h>
#include <stdint.h>
#include "../fujinet-fuji.h"
#include "../platform-specific/graphics.h"

#include "../fujinet-fuji.h"
#include "../fujinet-network.h"

#define FUJI_HOST_SLOT_COUNT 8
#define FUJI_DEVICE_SLOT_COUNT 8

HostSlot host_slots[FUJI_HOST_SLOT_COUNT];
DeviceSlot device_slots[FUJI_DEVICE_SLOT_COUNT];

static uint16_t timer;

void resetTimer()
{
    timer = 0;
}

uint16_t getTime()
{
    timer++;
    return timer;
}

void mount()
{
}

void quit()
{
    resetGraphics();
    exit(0);
}

void housekeeping()
{
    // Not needed on msdos
}

uint8_t getJiffiesPerSecond()
{
    return 60;
}

uint8_t getRandomNumber(uint8_t maxExclusive)
{
    return (uint8_t)rand()&0xff;
}
