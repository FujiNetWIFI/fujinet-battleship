

#include <string.h>
#include <stdint.h>
#include <conio.h>
#include "../platform-specific/sound.h"
#include "hires.h"
#include "vars.h"

ROP_TYPE background = 0;
extern uint8_t charset[];

/*-----------------------------------------------------------------------*/
void hires_putc(uint8_t x, uint8_t y, ROP_TYPE rop, uint8_t c)
{
    hires_Draw(x, y, 1, 8, rop, &charset[(uint16_t)c CHAR_SHIFT]);
}

/*-----------------------------------------------------------------------*/
void hires_putcc(uint8_t x, uint8_t y, ROP_TYPE rop, uint16_t cc)
{
    hires_putc(x, y, rop, (uint8_t)(cc >> 8));
    hires_putc(++x, y, rop, (uint8_t)cc);
}

void hires_Mask(uint8_t x, uint8_t y, uint8_t xlen, uint8_t ylen, uint8_t c)
{
#ifdef COCO3
    uint8_t *pos = (uint8_t *)SCREEN + (uint16_t)y * (WIDTH * 4) + x * 4;
    ylen++;
    BEGIN_GFX
    while (--ylen)
    {
        memset(pos, c, xlen * 4);
        pos += WIDTH * 4;
    }
    END_GFX
#else
    uint8_t *pos = (uint8_t *)SCREEN + (uint16_t)y * WIDTH + x;
    ylen++;
    while (--ylen)
    {
        memset(pos, c, xlen);
        pos += WIDTH;
    }
#endif
}

void hires_Draw(uint8_t x, uint8_t y, uint8_t xlen, uint8_t ylen, ROP_TYPE rop, uint8_t *src)
{
#ifdef COCO3
    uint16_t *dest = (uint16_t *)SCREEN + (uint16_t)y * (WIDTH * 2) + x * 2;
    uint16_t *src16 = (uint16_t *)src;
    uint8_t c, j;

    BEGIN_GFX
    // src16 = (uint16_t *)&charset[32];

    if (background)
    {
        for (c = 0; c < ylen; ++c)
        {
            for (j = 0; j < xlen; j++)
            {
                *dest++ = *src16 & rop | background;
                *dest++ = *(src16 + 1) & rop | background;
            }
            dest += WIDTH * 2 - j * 2;
            src16 += 2;

            // Wrap around to beginning of source character
            if ((c & 7) == 7)
                src16 -= 16;
        }
    }
    else
    {
        for (c = 0; c < ylen; ++c)
        {
            for (j = 0; j < xlen; j++)
            {
                *dest++ = *src16 & rop;
                *dest++ = *(src16 + 1) & rop;
            }
            dest += WIDTH * 2 - j * 2;
            src16 += 2;

            // Wrap around to beginning of source character
            if ((c & 7) == 7)
                src16 -= 16;
        }
    }

    END_GFX
#else
    uint8_t *pos = (uint8_t *)SCREEN + (uint16_t)y * WIDTH + x;
    uint8_t c;
    if (background)
    {
        for (c = 0; c < ylen; ++c)
        {
            memset(pos, (background ^ *(src + (c % 8))) | (*(src + (c % 8)) & rop), xlen);
            pos += WIDTH;
        }
    }
    else
    {
        for (c = 0; c < ylen; ++c)
        {
            memset(pos, (*(src + (c % 8)) & rop), xlen);
            pos += WIDTH;
        }
    }
#endif
}

/*
src = (uint16_t *)&charset[c];
            *dest = *src++;
            *(dest + 1) = *src++;
            *(dest + 80) = *src++;
            *(dest + 81) = *src++;
            *(dest + 160) = *src++;
            *(dest + 161) = *src++;
            *(dest + 240) = *src++;
            *(dest + 241) = *src++;
            *(dest + 320) = *src++;
            *(dest + 321) = *src++;
            *(dest + 400) = *src++;
            *(dest + 401) = *src++;
            *(dest + 480) = *src++;
            *(dest + 481) = *src++;
            *(dest + 560) = *src++;
            *(dest + 561) = *src++;
            c += 32;
*/
