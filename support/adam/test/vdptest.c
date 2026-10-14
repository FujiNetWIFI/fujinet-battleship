/* Minimal TMS9918 sanity test for the Adam target.
   Expected display: dark-red border, entire screen fine vertical
   stripes (0xAA pattern) in white-on-black. */
#include <video/tms99x8.h>

void main(void)
{
    vdp_color(15, 1, 6); /* border dark red - proves register writes */
    vdp_set_mode(mode_2);
    vdp_vfill(0x0000, 0xAA, 0x1800); /* patterns: vertical stripes */
    vdp_vfill(0x2000, 0xF1, 0x1800); /* colors: white on black */
    for (;;)
        vdp_set_reg(7, 0x16); /* keep border cyan-on-red blink target visible */
}
