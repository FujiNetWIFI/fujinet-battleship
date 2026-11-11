#ifdef __TRS80M2__

/*
  P&T's clock counts in 1/100 s (console.h's ticks()). waitvsync() paces
  frames at 60 a second, as the shared code expects of pause(): 5 ticks per
  3 frames, so 2, 2, then 1.
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <cpm.h>
#include <fujinet-fuji.h>
#include "console.h"
#include "../misc.h"
#include "../gamelogic.h"
#include "../platform-specific/graphics.h"
#include "../platform-specific/util.h"

// The firmware's WRITE HOST/DEVICE SLOTS expect all 8 slots
#define FUJI_HOST_SLOT_COUNT 8
#define FUJI_DEVICE_SLOT_COUNT 8

// Not fuji_set_boot_mode(): the Model II's FujiNet is the RS232 build, whose
// lobby is the MS-DOS one
#define LOBBY_HOST "fujinet.online"
#define LOBBY_FILE "MODEL2/lobby.imd"

// The FujiNet driver and its drives' DPHs live above $BFFF
#define DRIVER_BASE 0xC000
#define NO_DRIVE    0xFF

static uint16_t lastFrame, timerBase;
static uint8_t framePhase;
static bool seeded;

void waitvsync()
{
    static uint8_t len;

    // Send what was drawn, then wait out the rest of the frame
    flush();
    len = (framePhase == 2) ? 1 : 2;
    while ((uint16_t)((uint16_t)ticks() - lastFrame) < len)
        ;
    lastFrame = (uint16_t)ticks();
    framePhase = (framePhase == 2) ? 0 : framePhase + 1;
}

void resetTimer()
{
    timerBase = (uint16_t)ticks();
}

uint16_t getTime()
{
    // Wraps after 655 s, longer than any move clock
    return (uint16_t)ticks() - timerBase;
}

uint8_t getJiffiesPerSecond()
{
    return 100;
}

uint8_t getRandomNumber(uint8_t maxExclusive)
{
    if (!seeded)
    {
        // The first request comes only after some human-paced input
        seeded = true;
        srand((uint16_t)ticks());
    }
    return (uint8_t)(rand() % maxExclusive);
}

void housekeeping()
{
}

/*
 * Quitting to the lobby
 */

static HostSlot host_slots[FUJI_HOST_SLOT_COUNT];
static DeviceSlot device_slots[FUJI_DEVICE_SLOT_COUNT];
static uint8_t fcb[36];
static uint8_t dma[128];
static char program[9];
static char command[12];

static void print(const char *s)
{
    while (*s)
        conOut((uint8_t)*s++);
}

// BIOS SELDSK (WBOOT+24): the drive's DPH, or 0 if there is no such drive
static uint16_t seldsk(uint8_t drive) __z88dk_fastcall __naked
{
#asm
    EXTERN  l_dcal
    ld      c,l
    ld      hl,(1)
    ld      de,24
    add     hl,de
    ld      e,1         ; not a first select
    jp      l_dcal
#endasm
}

/* The CP/M drive for FujiNet slot 0. The driver maps its drives to slots in
   letter order, so it is the lowest drive the driver answers for. */
static uint8_t fujinet_drive(void)
{
    static uint8_t d;

    for (d = 0; d < 16; d++)
        if (seldsk(d) >= DRIVER_BASE)
            return d;
    return NO_DRIVE;
}

static void set_fcb(uint8_t drive, const char *name)
{
    memset(fcb, 0, sizeof(fcb));
    fcb[0] = drive;
    memcpy(fcb + 1, name, 11);
}

/* The .COM on drive to run: the one named like the image, else the first */
static bool find_program(uint8_t drive, const char *image)
{
    static const char *s;
    static uint8_t i, r, *e;
    static char want[9], name[9];

    s = strrchr(image, '/');
    s = s ? s + 1 : image;
    for (i = 0; i < 8 && s[i] && s[i] != '.'; i++)
        want[i] = (s[i] >= 'a' && s[i] <= 'z') ? s[i] - 32 : s[i];
    want[i] = 0;

    program[0] = 0;
    bdos(CPM_SDMA, (int)dma);
    set_fcb(drive + 1, "????????COM");
    for (r = (uint8_t)bdos(CPM_FFST, (int)fcb); r != 0xFF; r = (uint8_t)bdos(CPM_FNXT, 0))
    {
        e = dma + (r & 3) * 32 + 1;
        for (i = 0; i < 8 && (e[i] & 0x7F) != ' '; i++)
            name[i] = e[i] & 0x7F;
        name[i] = 0;
        if (!program[0] || !strcmp(name, want))
            strcpy(program, name);
    }
    return program[0] != 0;
}

/* CP/M runs the last line of A:$$$.SUB at the next warm boot */
static bool submit(const char *line)
{
    static uint8_t n;

    set_fcb(1, "$$$     SUB");
    bdos(CPM_DEL, (int)fcb);
    set_fcb(1, "$$$     SUB");
    if ((uint8_t)bdos(CPM_MAKE, (int)fcb) == 0xFF)
        return false;

    n = (uint8_t)strlen(line);
    memset(dma, 0, sizeof(dma));
    dma[0] = n;
    memcpy(dma + 1, line, n);
    bdos(CPM_SDMA, (int)dma);
    if (bdos(CPM_WRIT, (int)fcb) & 0xFF)
        return false;
    return (uint8_t)bdos(CPM_CLS, (int)fcb) != 0xFF;
}

static void mountLobby(void)
{
    static uint8_t i, slot;

    fuji_get_host_slots(host_slots, FUJI_HOST_SLOT_COUNT);

    // Use the host's slot if it has one, else take the last
    slot = FUJI_HOST_SLOT_COUNT;
    for (i = 0; i < FUJI_HOST_SLOT_COUNT; i++)
    {
        if (stricmp(LOBBY_HOST, (char *)host_slots[i]) == 0)
        {
            slot = i;
            break;
        }
    }

    if (slot == FUJI_HOST_SLOT_COUNT)
    {
        slot = FUJI_HOST_SLOT_COUNT - 1;
        strcpy((char *)host_slots[slot], LOBBY_HOST);
        fuji_put_host_slots(host_slots, FUJI_HOST_SLOT_COUNT);
    }

    fuji_mount_host_slot(slot);

    device_slots[0].hostSlot = slot;
    device_slots[0].mode = 0;
    strcpy((char *)device_slots[0].file, LOBBY_FILE);
    fuji_put_device_slots(device_slots, FUJI_DEVICE_SLOT_COUNT);
    fuji_set_device_filename(0, slot, 0, LOBBY_FILE);
    fuji_mount_disk_image(0, 1);
}

// Run the lobby through CP/M, the way it runs a game, or say where it is
void quit()
{
    static uint8_t drive;

    resetScreen();
    centerText(HEIGHT / 2, "Loading lobby...");
    flush();
    mountLobby();
    resetGraphics();

    drive = fujinet_drive();
    // Logs the new image in, and puts BDOS back on A: after the probe
    bdos(CPM_RDS, 0);

    if (drive != NO_DRIVE && find_program(drive, LOBBY_FILE))
    {
        command[0] = 'A' + drive;
        command[1] = ':';
        strcpy(command + 2, program);
        if (submit(command))
            exit(0);
    }

    // As CONFIG does when it can't boot
    print("FujiNet drives:\r\n  1 R/O " LOBBY_HOST ":" LOBBY_FILE "\r\n");
    exit(0);
}

#endif /* __TRS80M2__ */
