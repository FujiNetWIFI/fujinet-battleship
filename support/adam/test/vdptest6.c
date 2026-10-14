/* Test 6: vdptest5 + fujinet-lib appkey reads (the game's loadPrefs pattern)
   before AND after VDP init, linked against fujinet-adam.lib + smartkeys
   like the real game. If VRAM corrupts, the fujinet/appkey path is guilty. */
#include <video/tms99x8.h>
#include <string.h>
#include <stdint.h>
#include <eos.h>
#include "../../../src/fujinet-fuji.h"

extern const uint8_t charsetPatterns[];
extern const uint8_t charsetColors[];
extern const uint8_t altPatterns[];
extern const uint8_t altColors[];

static GameControllerData cont;
static char tempBuffer[128];

static uint16_t readKey(uint16_t creator, uint8_t app, uint8_t key)
{
    uint16_t read = 0;
    fuji_set_appkey_details(creator, app, DEFAULT);
    if (!fuji_read_appkey(key, &read, (uint8_t *)tempBuffer))
        read = 0;
    tempBuffer[read] = 0;
    return read;
}

void main(void)
{
    static uint8_t t, c, started, key;
    static uint16_t base, off, n, frame;
    static const char *s = "FUJI APPKEY + VDP TEST";

    readKey(0xE41C, 5, 0); /* like loadPrefs, before initGraphics */

    vdp_color(15, 1, 1);
    vdp_set_mode(mode_2);

    for (t = 0; t < 3; t++)
    {
        base = (uint16_t)t << 11;
        vdp_vwrite((void *)charsetPatterns, base, 106 * 8);
        vdp_vwrite((void *)charsetColors, 0x2000 + base, 106 * 8);
        vdp_vwrite((void *)altPatterns, base + 0xA0 * 8, 59 * 8);
        vdp_vwrite((void *)altColors, 0x2000 + base + 0xA0 * 8, 59 * 8);
    }

    vdp_vfill(0x1800, 0x20, 768);

    off = 32 * 2 + 2;
    n = 0;
    while ((c = *s++))
    {
        if (c >= 97 && c <= 122)
            c -= 32;
        vdp_vpoke(0x1800 + off + n, c);
        n++;
    }

    readKey(1, 1, 0); /* like the username read, after initGraphics */

    started = 0;
    frame = 0;
    for (;;)
    {
        while (!(vdp_get_status(0) & 0x80))
            ;
        ++frame;

        if (!started)
        {
            eos_start_read_keyboard();
            started = 1;
        }
        key = eos_end_read_keyboard();
        if (key > 1)
        {
            eos_start_read_keyboard();
            vdp_vpoke(0x1800 + 32 * 8 + 2, key);
        }

        eos_read_game_controller(0x03, &cont);

        vdp_vpoke(0x1800 + 32 * 5 + 2, ((frame >> 4) & 1) ? 0x3A : 0x20);
        vdp_vpoke(0x1800 + 32 * 5 + 6, 0x30 + ((frame >> 6) % 10));
    }
}
