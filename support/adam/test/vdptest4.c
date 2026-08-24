/* Test 4: exact replica of the game's initGraphics upload sequence using the
   real generated charset arrays, plus a text row and a game-tile row.
   Expected: black screen, "ADAM BATTLESHIP TEST" in white, a row of game
   tiles below. Garbage isolates the fault to this sequence/data. */
#include <video/tms99x8.h>
#include <string.h>
#include <stdint.h>

extern const uint8_t charsetPatterns[];
extern const uint8_t charsetColors[];
extern const uint8_t altPatterns[];
extern const uint8_t altColors[];

static uint8_t screen[768];

void main(void)
{
    static uint8_t t, i, c;
    static uint16_t base, off, n;
    static const char *s = "ADAM BATTLESHIP TEST 0123";

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
    memset(screen, 0x20, sizeof(screen));

    off = 32 * 2 + 2;
    n = 0;
    while ((c = *s++))
    {
        if (c >= 97 && c <= 122)
            c -= 32;
        screen[off + n++] = c;
    }
    vdp_vwrite(screen + off, 0x1800 + off, n);

    off = 32 * 5 + 2;
    for (i = 0; i < 16; i++)
        screen[off + i] = 0x10 + i;
    vdp_vwrite(screen + off, 0x1800 + off, 16);

    for (;;)
        t;
}
