/*
  Graphics functionality
*/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <dos.h>
#include <i86.h>
#include <string.h>
#include "../platform-specific/graphics.h"
#include "../platform-specific/sound.h"
#include "../misc.h"
#include "charset.h"

#define VIDEO_RAM_ADDR ((unsigned char far *)0xB8000000UL)
#define VIDEO_LINE_BYTES 80
#define VIDEO_ODD_OFFSET 0x2000

uint8_t far *video = VIDEO_RAM_ADDR;
extern uint8_t charset[];
int oldmode=0;
uint8_t mask=0;

#define OFFSET_Y 2

#define ROP_CPY 0xff

// Mode 4
#define ROP_BLUE 0b10101010
#define ROP_YELLOW 0b01010101
#define BOX_SIDE 0b111100

#define COLOR_MODE_COCO3_RGB 1
#define COLOR_MODE_COCO3_COMPOSITE 2

#define LEGEND_X 24

// Defined in this file
void drawTextAltAt(uint8_t x, uint8_t y, const char *s);
void drawTextAt(uint8_t x, uint8_t y, const char *s);
void drawShipInternal(uint8_t *dest, uint8_t size, uint8_t delta);

extern char lastKey;
extern uint8_t background;
static uint8_t fieldX = 0;
int8_t highlightX = -1;
bool inBorderScreen = false;
uint8_t box_color = 0xff;
uint16_t quadrant_offset[] = {
    256U * 12 + 5 + 64,
    256U * 1 + 5 + 64,
    256U * 1 + 17 + 64,
    256U * 12 + 17 + 64};

uint16_t legendShipOffset[] = {2, 1, 0, 256U * 5, 256U * 6 + 1};

/**
 * @brief plot a 8x8 2bpp tile to screen at column x, row y
 * @param tile ptr to 2bpp tile data * 8
 * @param x Column 0-39
 * @param y Row 0-24
 */
void plot_tile(const unsigned char *tile, unsigned char x, unsigned char y)
{
    unsigned char i=0;

    if (y<25)
        y <<= 3; // Convert row to line

    x <<= 1; // Convert column to video ram offset

    for (i=0;i<8;i++)
    {
        unsigned char r = y + i;
        unsigned char rh = r >> 1; // Because CGA is interleaved odd/even.
        unsigned short ro = rh * VIDEO_LINE_BYTES + x;

        // If row is odd, go into second bank.
        if (r & 1)
            ro += VIDEO_ODD_OFFSET;

        // Put tile data into video RAM.
        video[ro] = tile[i*2];
        video[ro+1] = tile[i*2+1];
    }
}

/**
 * @brief Clear screen to given color index
 * @param c Color to set in all spots
 */
void clear(unsigned char c)
{
    unsigned char b = ((c & 0x03) | ((c & 0x03) << 2) | ((c & 0x03) << 4) | ((c & 0x03) << 6));

    _fmemset(&video[0x0000], b, 8000-640);
    _fmemset(&video[0x2000], b, 8000-640);
}


void updateColors()
{
}

uint8_t cycleNextColor()
{
    return 0;
}

void initGraphics()
{
    union REGS r;
    uint8_t *c;
    uint16_t i;

    // Get old mode
    r.h.ah = 0x0f;
    int86(0x10,&r,&r);

    oldmode=r.h.al;

    // Set graphics mode
    r.h.ah = 0x00;
    r.h.al = 0x04; // 320x200x4
    int86(0x10,&r,&r);

    // Remap palette colors, if possible
    r.h.ah = 0x10;
    r.h.al = 0x00;
    r.h.bl = 1;             // Color 1
    r.h.bh = 10;            // to light green.
    int86(0x10,&r,&r);

    r.h.ah = 0x10;
    r.h.al = 0x00;
    r.h.bl = 2;             // Color 2
    r.h.bh = 4;             // To dark red
    int86(0x10,&r,&r);

    // Create a second set of white on black text for status bar
    // It could also be achieved via a mask in exchange for more memory
    c=&ascii[0][0];
    for(i=0;i<sizeof(ascii)/2;i++) {
        // Set  ascii characters to white on black
        *(c+1536)= (*c^0x55)*3;
        c++;
    }
}

bool saveScreenBuffer()
{
    return false;
}

void restoreScreenBuffer()
{
    // No-op on CoCo
}

void drawPlayerName(uint8_t player, const char *name, bool active)
{
    uint8_t x, y;
    uint16_t pos = fieldX + quadrant_offset[player];

    x = (uint8_t)(pos % 32 + 1);
    y = (uint8_t)(pos / 32 - 9);

    if (player == 0 || player == 3)
    {
        y += 89;
    }

    // background = ROP_YELLOW;

    if (active)
    {
        // hires_putc(x - 1, y, ROP_CPY, 0x05); // Arrow
        drawTextAt(x, y, name);
    }
    else
    {
        // hires_putc(x - 1, y, ROP_CPY, 0x62); // Blank bkg
        drawTextAltAt(x, y, name);
    }
}

void drawText(uint8_t x, uint8_t y, const char *s)
{
    signed char c=0;

    while (c = *s++)
    {
        c -= 32;
        if (c<0 || c>96)
            continue;

        if (x>39)
        {
            x=0;
            y++;
        }

        plot_tile(&ascii[c+mask],x++,y);
    }
}

void drawTextAt(uint8_t x, uint8_t y, const char *s)
{
    drawText(x,y,s);
}

void drawTextAlt(uint8_t x, uint8_t y, const char *s)
{
    drawText(x,y,s);
}

void drawTextAltAt(uint8_t x, uint8_t y, const char *s)
{
    drawText(x,y,s);
}

void resetScreen()
{
    clear(1);
}

void drawLegendShip(uint8_t player, uint8_t index, uint8_t size, uint8_t status)
{
    uint16_t dest = fieldX + quadrant_offset[player] + legendShipOffset[index];

    if (player > 1 || (player > 0 && fieldX > 0))
    {
        dest += 256 + 11;
    }
    else
    {
        dest += 256 - 4;
    }

    if (status)
    {
        // Draw normal ship.
        // drawShipInternal((uint8_t *)SCREEN + dest, size, 1);
    }
    else
    {
        // Draw shot ship
        // hires_Draw((uint8_t)(dest % 32), (uint8_t)(dest / 32), 1, size * 8, ROP_CPY, &charset[(uint16_t)0x1c << 3]);
    }
}

void drawGamefieldCursor(uint8_t quadrant, uint8_t x, uint8_t y, uint8_t *gamefield, uint8_t blink)
{

}

// Updates the gamefield display at attackPos
void drawGamefieldUpdate(uint8_t quadrant, uint8_t *gamefield, uint8_t attackPos, uint8_t blink)
{
}

void drawGamefield(uint8_t quadrant, uint8_t *field)
{
}

void drawShipInternal(uint8_t *dest, uint8_t size, uint8_t delta)
{
}

void drawShip(uint8_t size, uint8_t pos, bool hide)
{
}

void drawIcon(uint8_t x, uint8_t y, uint8_t icon)
{
}

void drawClock()
{
}

void drawConnectionIcon(bool show)
{
}

void drawSpace(uint8_t x, uint8_t y, uint8_t w)
{
}

void drawBoard(uint8_t playerCount)
{
}

void drawLine(uint8_t x, uint8_t y, uint8_t w)
{
    uint16_t o = 0;
    x <<=1;
    w <<= 1;
    o = y * 4 * VIDEO_LINE_BYTES + x;

    // Adjust for bottom move underlines
    if (y==24)
        o+=2*VIDEO_LINE_BYTES;

    _fmemset(&video[o], 0xAA, w);
    _fmemset(&video[o+0x2000], 0xAA, w);
}

void drawBox(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    uint8_t i;

    plot_tile(&pot_border[0],x,y);
    plot_tile(&pot_border[2],x+w+1,y);
    for (i=1;i<=w;i++) {
        plot_tile(&pot_border[1],x+i,y);
        plot_tile(&pot_border[1],x+i,y+h+1);
    }

    for (i=1;i<=h;i++) {
        plot_tile(&pot_border[5],x,y+i);
        plot_tile(&pot_border[5],x+w+1,y+i);
    }

    plot_tile(&pot_border[3],x,y+h+1);
    plot_tile(&pot_border[4],x+w+1,y+h+1);
}

void resetGraphics()
{
}

void waitvsync()
{
    // Wait until we are in vsync
    while (! (inp(0x3DA) & 0x08));
    while (inp(0x3DA) & 0x08);
}

void drawBlank(uint8_t x, uint8_t y)
{
    plot_tile(ascii[0x20+mask],x,y);
}
