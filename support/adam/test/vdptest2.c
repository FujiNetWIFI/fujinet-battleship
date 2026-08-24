/* Test 2: NMI tick (add_raster_int) + waitvsync + per-frame VDP writes.
   Healthy: screen cycles patterns/colors smoothly, black border.
   NMI race: accumulating random garbage.
   Tick not firing: static stripes (hang at first waitvsync). */
#include <video/tms99x8.h>
#include <interrupt.h>

volatile unsigned int jiffy;
volatile unsigned char vs;

void tick(void)
{
    ++jiffy;
    vs = 1;
}

void main(void)
{
    vdp_color(15, 1, 1);
    vdp_set_mode(mode_2);
    vdp_vfill(0x0000, 0xAA, 0x1800);
    vdp_vfill(0x2000, 0xF1, 0x1800);
    add_raster_int(tick);

    for (;;)
    {
        vs = 0;
        while (!vs)
            ;
        vdp_vfill(0x1800, (unsigned char)jiffy, 768);
        vdp_vfill(0x2000 + ((jiffy & 7) << 8), (jiffy & 1) ? 0xF1 : 0x4B, 256);
    }
}
