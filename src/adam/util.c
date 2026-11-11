/*
  Platform specific utilities - Coleco Adam

  The VDP vblank is wired to NMI on the Adam; the CRT's NMI handler calls
  jiffyTick() (installed via add_raster_int in initGraphics), which drives
  getTime()/waitvsync().
*/

#include "../misc.h"
#include <eos.h>

// WRITE DEVICE SLOTS expects all 8 firmware slots, though the Adam shows 4
#define FUJI_HOST_SLOT_COUNT 8
#define FUJI_DEVICE_SLOT_COUNT 8

static HostSlot host_slots[FUJI_HOST_SLOT_COUNT];
static DeviceSlot device_slots[FUJI_DEVICE_SLOT_COUNT];

volatile uint16_t jiffyCount = 0;
volatile uint8_t vsyncFlag = 0;
volatile uint8_t nmiSeen = 0;

static uint16_t timerBase = 0;
static uint8_t seeded = 0;

void jiffyTick(void)
{
    // jiffyCount advances in waitvsync() (which counts frames via either
    // this flag or the polled VDP status bit) - don't double count here.
    vsyncFlag = 1;
    nmiSeen = 1;
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

static bool sameHost(const char *a, const char *b)
{
    while (*a && *b)
    {
        if ((*a | 0x20) != (*b | 0x20))
            return false;
        a++;
        b++;
    }
    return *a == *b;
}

static void mountLobby(void)
{
    static uint8_t i, slot;
    static const char host[] = "tnfs.fujinet.online";
    static const char filename[] = "ADAM/lobby.ddp";

    fuji_get_host_slots(host_slots, FUJI_HOST_SLOT_COUNT);

    // Reuse a slot that already has the host, else take the last one
    slot = FUJI_HOST_SLOT_COUNT;
    for (i = 0; i < FUJI_HOST_SLOT_COUNT; i++)
    {
        if (sameHost(host, (char *)host_slots[i]))
        {
            slot = i;
            break;
        }
    }

    if (slot == FUJI_HOST_SLOT_COUNT)
    {
        slot = FUJI_HOST_SLOT_COUNT - 1;
        strcpy((char *)host_slots[slot], host);
        fuji_put_host_slots(host_slots, FUJI_HOST_SLOT_COUNT);
    }

    fuji_mount_host_slot(slot);

    device_slots[0].hostSlot = slot;
    device_slots[0].mode = 0;
    strcpy((char *)device_slots[0].file, filename);
    fuji_put_device_slots(device_slots, FUJI_DEVICE_SLOT_COUNT);
    fuji_set_device_filename(0, slot, 0, (char *)filename);
    fuji_mount_disk_image(0, 1);
}

void quit()
{
    resetGraphics();
    mountLobby();

    // As CONFIG does: no CONFIG at boot, then restart EOS on the mounted image
    fuji_set_boot_config(0);
    eos_init();
}
