/*
  Graphics functionality
*/
#include <i86.h>
#include <dos.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <conio.h>
#include "../platform-specific/graphics.h"
#include "../platform-specific/sound.h"
#include "../misc.h"

/**
 * @brief start offset
 */
#define OFFSET_Y 2

/**
 * @brief Field offset (current player)
 */
static uint8_t fieldX = 0;

/**
 * @brief external reference to character set
 */
extern uint8_t charset[256][16];

/**
 * @brief Old graphics mode
 */
unsigned char oldMode=0;

/**
 * @brief Number of bytes in a single line of CGA graphics
 */
#define VIDEO_LINE_BYTES 80

/**
 * @brief 0xB800 segment offset for odd lines
 */
#define VIDEO_ODD_OFFSET 0x2000

/**
 * @brief Address of CGA video RAM
 */
#define VIDEO_RAM_ADDR ((unsigned char far *)0xB8000000UL)

/**
 * @brief Pointer to video RAM
 */
unsigned char far *video = VIDEO_RAM_ADDR;

/**
 * @brief Quadrant offsets
 */
uint16_t quadrant_offset[] = {
    256U * 12 + 5 + 64,
    256U * 1 + 5 + 64,
    256U * 1 + 17 + 64,
    256U * 12 + 17 + 64};

/**
 * @brief pointers to grid charset elements
 */
uint8_t *srcBlank = &charset[0x18];
uint8_t *srcHit = &charset[0x19];
uint8_t *srcMiss = &charset[0x1A];
uint8_t *srcHit2 = &charset[0x1B];
uint8_t *srcHitLegend = &charset[0x1C << 3];

/**
 * @brief Plot a single 8x8 tile at requested coordinates
 * @param tile Pointer to tile data, expected to be 16 bytes in length
 * @param x horizontal position (0-39)
 * @param y vertical position (0-199)
 * @param c color (0-3)
 */
void plot_tile(const unsigned char *tile, unsigned char x, unsigned char y, unsigned char c)
{
    unsigned char i=0;
    unsigned char m=0;

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

        // Set mask for given color
        switch(c)
        {
        case 0:
            m = 0x00;
            break;
        case 1:
            m = 0x55;
            break;
        case 2:
            m = 0xAA;
            break;
        case 3:
            m = 0xFF;
            break;
        }

        // Put tile data into video RAM.
        video[ro] = tile[i*2];
        video[ro+1] = tile[i*2+1];
        video[ro] &= m;
        video[ro+1] &= m;
    }
}

/**
 * @brief return row offset given row
 * @param y Row (0-199)
 * @return address offset in b800
 */
static inline uint16_t row_offset(int y)
{
    /* Even lines start at 0x0000, odd lines at 0x2000 */
    int bank_offset = (y & 1) ? 0x2000 : 0x0000;
    int row_in_bank = (y >> 1) * VIDEO_LINE_BYTES;
    return bank_offset + row_in_bank;
}

/**
 * @brief Fill rectangle
 * @param x left rectangle bound (0-319)
 * @param y top rectangle bound (0-199)
 * @param w width of rectangle (0-319)
 * @param h height of rectangle (0-199)
 * @param c color (0-3)
 */
void rectFill(int x, int y, int w, int h, uint8_t color)
{
    uint8_t full_byte =
        (color << 6) |
        (color << 4) |
        (color << 2) |
        (color);

    int row=0, col=0;

    for (row = y; row < y + h; row++)
    {
        uint16_t line_off = row_offset(row);
        uint8_t far *line = &video[line_off];
        int start_byte    = x >> 2;
        int end_byte      = (x+w - 1) >> 2;
        int start_pixel   = x & 3;
        int end_pixel     = (x+w-1) & 3;

        if (start_byte == end_byte)
        {
            uint8_t mask = 0xFF;
            mask &= (0xFF >> (start_pixel * 2));
            mask &= (0xFF << ((3 - end_pixel) * 2));

            line[start_byte] =
                (line[start_byte] & ~mask) |
                (full_byte & mask);

            continue;
        }

        {
            uint8_t mask = (0xFF >> (start_pixel * 2));
            uint8_t old  = line[start_byte];
            line[start_byte] = (old * ~mask) | (full_byte & mask);
        }

        for (col = start_byte + 1; col < end_byte; col++)
        {
            line[col] = full_byte;
        }

        {
            uint8_t mask = (0xFF << ((3 - end_pixel) * 2));
            uint8_t old  = line[end_byte];
            line[end_byte] = (old & ~mask) | (full_byte & mask);
        }
    }
}

/**
 * @brief Initialize Graphics mode
 */
void initGraphics()
{
    union REGS r;
    uint8_t *c;
    uint16_t i;

    // Get old mode
    r.h.ah = 0x0f;
    int86(0x10,&r,&r);

    oldMode=r.h.al;

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
}

/**
 * @brief Put icon to screen
 * @param x Horizontal Coordinate (0-39)
 * @param y Vertical Coordinate (0-23)
 * @param icon Icon #
 */
void drawIcon(uint8_t x, uint8_t y, uint8_t icon)
{
    plot_tile(&charset[icon],x,y,3);
}

/**
 * @brief Clear the screen
 */
void resetScreen(void)
{
    _fmemset(&video[0x0000], 0x00, 8000);
    _fmemset(&video[0x2000], 0x00, 8000);
}

/**
 * @brief Draw line
 * @param x Horizontal position (0-39)
 * @param y Vertical position (0-24)
 * @param w Width (0-39)
 */
void drawLine(uint8_t x, uint8_t y, uint8_t w)
{
    int i=x;

    for (i=x;i<x+w;i++)
        plot_tile(&charset[0x18],i,y,3);
}

/**
 * @brief Draw Space
 * @param x Horizontal Position (0-39)
 * @param y Vertical Position (0-24)
 * @param w Width (0-39)
 */
void drawSpace(uint8_t x, uint8_t y, uint8_t w)
{
    int i=x;

    for (i=x;i<x+w;i++)
        plot_tile(&charset[0x20],i,y,3);
}

/**
 * @brief Draw text
 * @param x Horizontal Position (0-39)
 * @param y Vertical Position (0-24)
 * @param s Pointer to string to output.
 */
void drawText(uint8_t x, uint8_t y, const char *s)
{
    char c=0;

    while (c = *s++)
    {
        if (c >= 97 && c <= 122)
            c -= 32;
        plot_tile(&charset[c],x++,y,3);
    }
}

/**
 * @brief Draw text (alt)
 * @param x Horizontal Position (0-39)
 * @param y Vertical Position (0-24)
 * @param s Pointer to string to output.
 */
void drawTextAlt(uint8_t x, uint8_t y, const char *s)
{
    char c=0;

    while (c = *s++)
    {
        unsigned char co=3;

        if (c<65 || c > 90)
            co = 0x01;
        else
            co = 0x03;

        if (c >= 97 && c <= 122)
            c -= 32;

        plot_tile(&charset[c],x++,y,3);
    }
}

/**
 * @brief Wait for vertical sync
 */
void waitvsync()
{
    while (! (inp(0x3DA) & 0x08));
    while (inp(0x3DA) & 0x08);
}

/**
 * @brief draw ship
 * @param size Size of ship (2-5)
 * @param pos Ship position
 * @param hide Hide ship (bool)
 */
void drawShip(uint8_t size, uint8_t pos, bool hide)
{
    uint8_t x, y, i, j, delta = 0;
    uint8_t *src = NULL;

    if (pos > 99)
    {
        delta = 1; // 1=vertical, 0=horizontal
        pos -= 100;
    }

    x = (pos % 10) + fieldX + 5;
    y = ((pos / 10) + 12) * 8 + OFFSET_Y;

    if (hide) // Draw hidden
    {
        char *blank = &charset[0x20];

        if (!delta) // Horizontal hide
        {
            while (size--)
                plot_tile(blank, x++, y, 0x03);
        }
        else
        {
            while (size--)
                plot_tile(blank, x, y++, 0x03);
        }
    }
    else
    {
        // Draw normal

        if (!delta) // Horizontal
        {
            char t = 0x12;
            plot_tile(&charset[t++],x++,y,0x03);

            while (size--)
            {
                if (!size)
                    t++;

                plot_tile(&charset[t],x++,y,0x03);
            }
        }
        else
        {
            char t = 0x15;
            plot_tile(&charset[t++],x,y--,0x03);

            while(size--)
            {
                if (!size)
                    t++;

                plot_tile(&charset[t],x,y--,0x03);
            }
        }
    }
}

/**
 * @brief Draw the board, based on player count
 * @param playerCount Player count (0-3)
 */
void drawBoard(uint8_t playerCount)
{
    uint8_t i, x, y, ix, ox, left = 1, fy, eh, drawEdge, drawX, drawCorner, edgeSkip;

    uint16_t pos;
    // Center layout
    fieldX = playerCount > 2 ? 0 : 6;

    for (i = 0; i < playerCount; i++)
    {
        pos = fieldX + quadrant_offset[i];
        x = (uint8_t)(pos % 32);
        y = (uint8_t)(pos / 32);

        // right and left drawers
        if (i > 1 || playerCount == 2 && i > 0)
        {
            ox = x - 1;
            ix = x + 10;
            left = 0;
            drawX = ix + 1;
            drawEdge = drawX + 3;
            drawCorner = 0xd;
        }
        else
        {
            ix = x - 1;
            ox = x + 10;
            drawX = ix - 3;
            drawEdge = drawX - 1;
            drawCorner = 0xc;
        }
        if (i == 1 || i == 2)
        {
            // Name badge corners
            plot_tile(&charset[0x5C], x-1, y, 3);
            plot_tile(&charset[0x5D], x+10, y, 3);

            // Name badge

            // Fill
            rectFill(x<<3,y-9,10*8,9,3);

            // Border
            rectFill(x<<3,y-10,10*8,1,1);
            rectFill(x<<3,y-10,1,1,2);
            rectFill(x<<3+10,y-10,1,1,2);
            rectFill(x<<3-1,y-9,1,1,1);
            rectFill(x<<3+10,y-9,1,1,3);

            fy = y + 80;
        }
        else
        {
            // Name badge corners
            plot_tile(&charset[0x5E],x-1,y+1,3);
            plot_tile(&charset[0x5F],x+10,y+1,3);

            // Name fill
            rectFill(x*8,y+80,10,9,3);

            // Border
            rectFill(x-1,y+88,1,1,1);
            rectFill(x+10,y+88,1,1,3);

            rectFill(x,y+89,10,1,1);
            rectFill(x-1,y+89,1,1,2);
            rectFill(x+10,y+89,1,1,2);

            fy = y - 8;
        }

        // Outside edge
        plot_tile(&charset[(left ? 0x23 : 0x22)], ox, y<<3, 0x03);

        // Inner edge (adjacent to ships drawer)
        plot_tile(&charset[(left ? 0x01 : 0x04)],ix,y+1,0x03);

        // Inner edge + ship drawer
        plot_tile(&charset[(left ? 0x24 : 0x25)], ix, y>>3, 0x03);
        plot_tile(&charset[(left ? 0x24 : 0x25)], ix, y>>3+9, 0x03);

        // Blue gamefield
        rectFill(x<<3, y, 10, 80, 0x01);

        edgeSkip = 0;

        if (playerCount == 1)
        {
            fy += 5;
            edgeSkip = 4;
        }

        // Far edge
        if (i || edgeSkip)
        {
            if (i != 2 && !edgeSkip)
                eh = 8;
            else
                eh = 3;

            plot_tile(&charset[0x02] + edgeSkip, x-1, fy, 0x03);
            plot_tile(&charset[0x03] + edgeSkip, x+10, fy, 0x03);
            plot_tile(&charset[0x29] + edgeSkip, x, fy, 0x03);
        }

        // Ship drawer edges
        plot_tile(&charset[0x11], drawX, y, 0x03);
        plot_tile(&charset[0x11], drawX, y+72, 0x03);
        plot_tile(&charset[0x10], drawEdge, y+8, 0x03);
        plot_tile(&charset[drawCorner], drawEdge, y, 0x03);
        plot_tile(&charset[drawCorner+2], drawEdge, y+72, 0x03);

        // Fill in the drawer
        rectFill(drawX, y+8, 3, 64, 0x01);
    }
}

/**
 * @brief Draw game field
 * @param quadrant (0-3)
 * @param pointer to field
 */
void drawGamefield(uint8_t quadrant, uint8_t *field)
{
    // These functions need to take into account CGA's interleaving.
    uint8_t *dest = (uint8_t *)video + quadrant_offset[quadrant] + fieldX;
    uint8_t y, x, j;
    uint8_t *src;

    for (y = 0; y < 10; ++y)
    {
        for (x = 0; x < 10; ++x)
        {
            if (*field)
            {
                src = *field == 1 ? srcHit : srcMiss;
                for (j = 0; j < 8; ++j)
                {
                    *dest = *src++;
                    dest += 32;
                }
                dest -= 256;
            }
            field++;
            dest++;
        }

        dest += 246;
    }
}

/**
 * @brief update game filed at attackPos
 * @param quadrant (0-3)
 * @param gamefield pointer to game field
 * @param attackPos attack position
 * @param whether to blink?
 */
void drawGamefieldUpdate(uint8_t quadrant, uint8_t *gamefield, uint8_t attackPos, uint8_t blink)
{
    uint8_t *src, *dest = (uint8_t *)video + quadrant_offset[quadrant] + fieldX + (uint16_t)(attackPos / 10) * 256 + (attackPos % 10);
    uint8_t j, c = gamefield[attackPos];

    if (c == FIELD_ATTACK)
    {
        src = blink ? srcHit2 : srcHit;
    }
    else if (c == FIELD_MISS)
    {
        src = srcMiss;
    }
    else
    {
        return;
    }

    for (j = 0; j < 8; ++j)
    {
        *dest = *src++;
        dest += 32;
    }
}
