/* Test 5: vdptest4 + the game's per-frame EOS input polling
   (eos_start/end_read_keyboard + eos_read_game_controller) + a blinking
   cursor cell + status-poll vsync. If this corrupts VRAM, the EOS calls
   are interacting with the VDP path. */
#include <video/tms99x8.h>
#include <string.h>
#include <stdint.h>
#include <eos.h>

extern const uint8_t charsetPatterns[];
extern const uint8_t charsetColors[];
extern const uint8_t altPatterns[];
extern const uint8_t altColors[];

static GameControllerData cont;
static uint8_t screen[768];

void main(void)
{
    static uint8_t t, i, c, blink, started, key;
    static uint16_t base, off, n, frame;
    static const char *s = "EOS INPUT + VDP TEST";

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

    started = 0;
    frame = 0;
    for (;;)
    {
        /* status-poll vsync */
        while (!(vdp_get_status(0) & 0x80))
            ;
        ++frame;

        /* game-style input polling */
        if (!started)
        {
            eos_start_read_keyboard();
            started = 1;
        }
        key = eos_end_read_keyboard();
        if (key > 1)
        {
            eos_start_read_keyboard();
            /* echo the key */
            vdp_vpoke(0x1800 + 32 * 8 + 2, key);
        }

        eos_read_game_controller(0x03, &cont);

        /* blinking cursor cell */
        blink = (frame >> 4) & 1;
        vdp_vpoke(0x1800 + 32 * 5 + 2, blink ? 0x3A : 0x20);

        /* live frame counter digits */
        vdp_vpoke(0x1800 + 32 * 5 + 6, 0x30 + ((frame >> 6) % 10));
    }
}
