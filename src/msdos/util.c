#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <process.h>
#include <direct.h>
#include <stdio.h>
#include <i86.h>

#include <fujinet-fuji.h>

extern void resetGraphics(void);

#define FUJI_SIGNATURE     "FUJI"
#define FUJI_HOST_SLOT_COUNT 8
#define FUJI_DEVICE_SLOT_COUNT 8

static HostSlot host_slots[FUJI_HOST_SLOT_COUNT];
static DeviceSlot device_slots[FUJI_DEVICE_SLOT_COUNT];

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

typedef struct {
    uint8_t query;
    char    signature[4];
    uint8_t unit;
} fuji_ioctl_query;

static int find_slot_drive(int slot)
{
    int drive;
    union REGS regs;
    struct SREGS sregs;
    fuji_ioctl_query query;

    for (drive = 3; drive <= 26; drive++) {
        memset(&query, 0, sizeof(query));
        regs.h.ah = 0x44;
        regs.h.al = 0x04;
        regs.h.bl = (unsigned char)drive;
        regs.w.cx = sizeof(query);
        regs.x.dx = FP_OFF(&query);
        sregs.ds  = FP_SEG(&query);
        int86x(0x21, &regs, &regs, &sregs);
        if (!(regs.x.cflag & INTR_CF) &&
            memcmp(query.signature, FUJI_SIGNATURE, 4) == 0 &&
            query.unit == slot)
            return drive;
    }
    return -1;
}

void mount()
{
    static uint8_t i, slot;
    unsigned int total;
    int drive;
    char host[]     = "ec.tnfs.io";
    char filename[] = "msdos/lobby.img";

    fuji_get_host_slots((unsigned char *)host_slots, FUJI_HOST_SLOT_COUNT);

    slot = FUJI_HOST_SLOT_COUNT;
    for (i = 0; i < FUJI_HOST_SLOT_COUNT; i++) {
        if (stricmp(host, (char *)host_slots[i]) == 0) {
            slot = i;
            break;
        }
    }

    if (slot == FUJI_HOST_SLOT_COUNT) {
        slot = FUJI_HOST_SLOT_COUNT - 1;
        strcpy((char *)host_slots[slot], host);
        fuji_put_host_slots((unsigned char *)host_slots, FUJI_HOST_SLOT_COUNT);
    }

    fuji_mount_host_slot(slot);

    device_slots[0].hostSlot = slot;
    device_slots[0].mode = 0;
    strcpy((char *)device_slots[0].file, filename);
    fuji_put_device_slots(device_slots, FUJI_DEVICE_SLOT_COUNT);
    fuji_set_device_filename(0, slot, 0, filename);
    fuji_mount_disk_image(0, 1);

    drive = find_slot_drive(0);
    if (drive < 0)
        return;

    _dos_setdrive(drive, &total);
    chdir("\\");
    spawnlp(P_OVERLAY, "COMMAND.COM", "COMMAND.COM", "/C", "AUTOEXEC.BAT", NULL);
}

void quit()
{
    resetGraphics();
    printf("Loading Lobby...\n");
    mount();
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
