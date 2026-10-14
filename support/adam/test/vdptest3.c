/* Test 3: vdp_vwrite + vdp_vpoke, no NMI hook.
   Expected: top half = checkerboard glyphs, bottom half = solid glyphs,
   border dark blue. Garbage means vdp_vwrite is broken. */
#include <video/tms99x8.h>

static unsigned char glyphs[32]; /* glyph 0: checker, glyph 1: solid */
static unsigned char colors[32];

void main(void)
{
    unsigned int i, t, base;

    for (i = 0; i < 8; i++)
    {
        glyphs[i] = (i & 1) ? 0xAA : 0x55; /* glyph 0 checker */
        glyphs[8 + i] = 0xFF;              /* glyph 1 solid */
        colors[i] = 0xF1;                  /* white on black */
        colors[8 + i] = 0x61;              /* dark red on black */
    }

    vdp_color(15, 1, 4);
    vdp_set_mode(mode_2);

    for (t = 0; t < 3; t++)
    {
        base = t << 11;
        vdp_vwrite(glyphs, base, 16);
        vdp_vwrite(colors, 0x2000 + base, 16);
    }

    for (i = 0; i < 384; i++)
        vdp_vpoke(0x1800 + i, 0);
    vdp_vfill(0x1800 + 384, 1, 384);

    for (;;)
        i++;
}
