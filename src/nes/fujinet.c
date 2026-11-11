#ifdef BUILD_NES

/**
 * @brief   FujiNet calls - NES (CUSTOM_FUJINET_CALLS, and the lobby boot)
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 *
 * Everything that talks to the cartridge through fujinet-lib is in this one
 * file: it includes the library's headers and, of the game's, only vars.h;
 * the shared code reaches the cartridge through the CUSTOM_FUJINET_CALLS
 * hooks instead.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fujinet-fuji.h>
#include <fujinet-network.h>
#include "vars.h"

// graphics.c - declared here rather than through ../platform-specific, which
// would bring in the game's headers
void drawText(uint8_t x, uint8_t y, const char *s);
void drawSpace(uint8_t x, uint8_t y, uint8_t w);

// READ HOST SLOTS returns all 8 slots.
#define FUJI_HOST_SLOT_COUNT 8
#define LOBBY_DEVICE_SLOT 0
#define LOBBY_MODE_READ 1

int16_t custom_network_call(char *url, uint8_t *buffer, uint16_t max_len)
{
    int16_t read;

    if (network_open(url, OPEN_MODE_HTTP_GET, OPEN_TRANS_NONE) != FN_ERR_OK)
        return -1;

    read = network_read(url, buffer, max_len);
    network_close(url);
    return read;
}

uint16_t custom_read_appkey(uint16_t creator_id, uint8_t app_id, uint8_t key_id, char *destination)
{
    uint16_t read = 0;

    fuji_set_appkey_details(creator_id, app_id, DEFAULT);
    if (!fuji_read_appkey(key_id, &read, (uint8_t *)destination))
        read = 0;
    return read;
}

void custom_write_appkey(uint16_t creator_id, uint8_t app_id, uint8_t key_id, uint16_t count, char *data)
{
    fuji_set_appkey_details(creator_id, app_id, DEFAULT);
    fuji_write_appkey(key_id, count, (uint8_t *)data);
}

static void quitStatus(const char *s)
{
    drawSpace(0, HEIGHT - 1, WIDTH);
    drawText((WIDTH - (unsigned char)strlen(s)) / 2, HEIGHT - 1, s);
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

/*
  SET_DEVICE_FULLPATH is a fixed 256-byte payload -- a short one is rejected on
  the ESP32 side -- so the path is padded here rather than at the call site.
*/
static const char lobbyPath[MAX_FILENAME_LEN] = "nes/lobby.nes";
static const char lobbyHost[] = "ec.tnfs.io";
static HostSlot slots[FUJI_HOST_SLOT_COUNT];

static uint8_t findLobbyHost(void)
{
    uint8_t i;

    if (!fuji_get_host_slots(slots, FUJI_HOST_SLOT_COUNT))
        return FUJI_HOST_SLOT_COUNT;

    for (i = 0; i < FUJI_HOST_SLOT_COUNT; i++)
        if (sameHost(lobbyHost, (const char *)slots[i]))
            return i;

    return FUJI_HOST_SLOT_COUNT;
}

/*
  The slot is used as found, never created: a player who reached this client
  through the FujiNet Lobby already has the host, and anyone else is told to
  add it in CONFIG.
*/
static bool mountLobby(uint8_t slot)
{
    if (!fuji_mount_host_slot(slot))
        return false;
    if (!fuji_set_device_filename(LOBBY_MODE_READ, slot, LOBBY_DEVICE_SLOT,
                                  (char *)lobbyPath))
        return false;
    // Returns once the whole image is on the cart.
    return fuji_mount_disk_image(LOBBY_DEVICE_SLOT, LOBBY_MODE_READ) != 0;
}

void quit(void)
{
    uint8_t romState, slot, pct = 0xFF;
    char pctText[5];
    FujiUserRomProgress progress;

    quitStatus("LOADING LOBBY...");

    if (fuji_user_rom_status(&progress) == FUJI_USER_ROM_NO_CART)
    {
        quitStatus("NO FUJINET CARTRIDGE");
        return;
    }

    slot = findLobbyHost();
    if (slot == FUJI_HOST_SLOT_COUNT)
    {
        quitStatus("ADD EC.TNFS.IO IN CONFIG");
        return;
    }

    if (!mountLobby(slot))
    {
        quitStatus("LOBBY NOT AVAILABLE");
        return;
    }

    // Watch the cartridge's own progress counter rather than guessing a delay.
    while ((romState = fuji_user_rom_status(&progress)) == FUJI_USER_ROM_LOADING)
    {
        if (progress.percent != pct)
        {
            pct = progress.percent;
            itoa(pct, pctText, 10);
            strcat(pctText, "%");
            drawText(18, HEIGHT - 1, pctText);
        }
    }

    if (romState != FUJI_USER_ROM_READY)
    {
        quitStatus("LOBBY LOAD FAILED");
        return;
    }

    // Stop the firmware serving CONFIG at boot, then hand the console to the
    // cartridge's loader ROM. It does not return.
    fuji_set_boot_config(0);
    fuji_enable_user_rom();
}

#endif /* BUILD_NES */
