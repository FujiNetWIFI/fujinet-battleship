/*
  Graphics functionality
*/
#include <i86.h>
#include <dos.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <conio.h>
#include "vars.h"
#include "../platform-specific/graphics.h"
#include "../platform-specific/sound.h"
#include "../misc.h"

/* Based on the atari graphics.c */

/**
 * @brief extern pointing to character set arrays
 */
extern unsigned char charset[256][16];
extern unsigned char ascii[256][16];

/**
 * @brief pointer to the B800 video segment for CGA
 */
#define VIDEO_RAM_ADDR ((unsigned char far *)0xB8000000UL)
unsigned char far *video = VIDEO_RAM_ADDR;

/**
 * @brief stride size (# of bytes per line)
 */
#define VIDEO_LINE_BYTES 80

/**
 * @brief offset in video segment for odd lines
 */
#define VIDEO_ODD_OFFSET 0x2000

/**
 * @brief previous video mode
 */
unsigned char prevVideoMode;

/**
 * @brief Top left of each playfield quadrant
 */
static unsigned char quadrant_offset[4][2] =
    {
        {8,14}, // bottom left
        {8,2},  // Top left
        {21,2}, // top right
        {21,14} // bottom right
    };


/**
 * @brief offset of legends for each player
 */
uint8_t legendShipOffset[] = {2, 1, 0, 40 * 5, 40 * 6 + 1};

/**
 * @brief Horizontal Field offset (0-39)
 */
unsigned char fieldX = 0;

/**
 * @brief Number of active players (0-3)
 */
static unsigned char playerCount = 0;

/**
 * #brief not sure why this is here
 */
static unsigned char inGameCharSet = 0;

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
 * @brief plot char, in given color/inverse
 * @param x Horizontal position (0-39)
 * @param y Vertical position (0-24)
 * @param c Color (0-3)
 * @param i Inverse? (0-1)
 * @param s Pointer to null terminated string
 */
void plot_char(unsigned char x,
               unsigned char y,
               unsigned char color,
               unsigned char i,
               char c)
{
    unsigned char tile[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    unsigned char mask = 0xFF;

    // Optimization to just call plot_tile directly
    // If we're just doing white on color 0.
    if (i==0 && color == 3)
    {
        plot_tile(ascii[c], x, y);
        return;
    }

    if (i)
        i=0xFF;

    switch(color)
    {
    case 0:
        mask = 0x00;
        break;
    case 1:
        mask = 0x55;
        break;
    case 2:
        mask = 0xAA;
        break;
    case 3:
        mask = 0xFF;
    }

    // Yes, this is unrolled.
    tile[0]=charset[c][0] ^ i & mask;
    tile[1]=charset[c][1] ^ i & mask;
    tile[2]=charset[c][2] ^ i & mask;
    tile[3]=charset[c][3] ^ i & mask;
    tile[4]=charset[c][4] ^ i & mask;
    tile[5]=charset[c][5] ^ i & mask;
    tile[6]=charset[c][6] ^ i & mask;
    tile[7]=charset[c][7] ^ i & mask;
    tile[8]=charset[c][8] ^ i & mask;
    tile[9]=charset[c][9] ^ i & mask;
    tile[10]=charset[c][10] ^ i & mask;
    tile[11]=charset[c][11] ^ i & mask;
    tile[12]=charset[c][12] ^ i & mask;
    tile[13]=charset[c][13] ^ i & mask;
    tile[14]=charset[c][14] ^ i & mask;
    tile[15]=charset[c][15] ^ i & mask;
    tile[16]=charset[c][16] ^ i & mask;
}

/**
 * @brief plot name text inverse
 * @param x Horizontal position (0-39)
 * @param y Vertical position (0-24)
 * @param color Color to plot (0-3)
 * @param s Pointer to name string
 */
void plotName(unsigned char x, unsigned char y, unsigned char color, const char *s)
{
    char c=0;

    while (c = *s++)
    {
        plot_char(x++, y, color, 1, c);
    }
}


/**
 * @brief Clear screen to given color index
 */
void resetScreen(void)
{
    waitvsync();
    _fmemset(&video[0x0000], 0, 8000-640);
    _fmemset(&video[0x2000], 0, 8000-640);
    waitvsync();
}

/**
 * @brief cycle to next color palette
 */
unsigned char cycleNextColor()
{
    return 0;
}

/**
 * @brief Initialize Graphics mode
 * @verbose 320x200x2bpp (4 colors, CGA)
 */
void initGraphics()
{
    union REGS r;

    // Get old mode
    r.h.ah = 0x0f;
    int86(0x10,&r,&r);

    prevVideoMode=r.h.al;

    // Set graphics mode
    r.h.ah = 0x00;
    r.h.al = 0x04; // 320x200x4
    int86(0x10,&r,&r);
}

/**
 * @brief Store screen buffer into secondary buffer
 * @verbose not used.
 */
bool saveScreenBuffer()
{
    return false;
    // memcpy(SCREEN_BAK, SCREEN_LOC, WIDTH * HEIGHT);
}

/**
 * @brief Restore screen buffer from secondary buffer
 * @verbose not used.
 */
void restoreScreenBuffer()
{
    // waitvsync();
    // memcpy(SCREEN_LOC, SCREEN_BAK, WIDTH * HEIGHT);
}

/**
 * @brief Text output
 * @param x Column
 * @param y Row
 * @param s Text to output
 */
void drawText(unsigned char x, unsigned char y, const char *s)
{
    signed char c=0;

    while (c = *s++)
    {
        if (x>39)
        {
            x=0;
            y++;
        }

        plot_char(x++, y, 3, 0, c);
    }

}

void drawTextAlt(unsigned char x, unsigned char y, const char *s)
{
    signed char c=0;

    while (c = *s++)
    {
        if (c>90 || (c < 65 && c > 32))
        {
            if (inGameCharSet && y == HEIGHT - 1 && c >= 0x30 && c <= 0x39)
                plot_char(x++, y, 1, 0, c);
        }

        if (x>39)
        {
            x=0;
            y++;
        }

        plot_char(x++, y, 3, 0, c);
    }
}

/**
 * @brief Wait for vertical sync, no more than 1/60th of a second.
 */
void waitvsync()
{
    // Wait until we are in vsync
    while (! (inp(0x3DA) & 0x08));
    while (inp(0x3DA) & 0x08);
}

/**
 * @brief draw icon
 * @param x Horizontal position (0-39)
 * @param y Vertical position (0-24)
 * @param icon Character # (0-255)
 */
void drawIcon(unsigned char x, unsigned char y, unsigned char icon)
{
    plot_tile(&charset[icon], x, y);
}

/**
 * @brief draw blank
 * @param x Horizontal position (0-39)
 * @param y Vertical position (0-24)
 */
void drawBlank(unsigned char x, unsigned char y)
{
    plot_tile(&charset[0x00], x, y);
}

/**
 * @brief Draw a run of blanks
 * @param x Horizontal Position (0-39)
 * @param y Vertical position (0-24)
 * @param w # of blank characters (0-39)
 */
void drawSpace(unsigned char x, unsigned char y, unsigned char w)
{
    while (w--)
        drawBlank(x++,y);
}

/**
 * @brief Draw the clock icon
 */
void drawClock(void)
{
    drawIcon(WIDTH-1, HEIGHT-1, 0x1d);
}

/**
 * @brief Draw the network connection icon
 */
void drawConnectionIcon(bool show)
{
    if (show)
    {
        drawIcon(0, HEIGHT-1, 0x1E);
        drawIcon(1, HEIGHT-1, 0x1F);
    }
    else
    {
        drawIcon(0, HEIGHT-1, 0x00);
        drawIcon(0, HEIGHT-1, 0x00);
    }
}

/**
 * @brief Draw Player Name
 * @param player Player # (0-3)
 * @param name Pointer to player name
 * @param active Is player active?
 */
void drawPlayerName(unsigned char player, const char *name, bool active)
{
    uint8_t x   = quadrant_offset[player][0];
    uint8_t y   = quadrant_offset[player][1];
    uint8_t add = active ? 0x00 : 0x80;
    uint8_t i   = 0;

    x += fieldX;

    if (player == 0 || player == 3)
    {
        // Bottom player boards

        // Thin horizontal border
        drawIcon(x, y, 0x08 + add);
        drawIcon(x+1, y, 0x27 + add);
        drawIcon(x+2, y, 0x27 + add);
        drawIcon(x+3, y, 0x27 + add);
        drawIcon(x+4, y, 0x27 + add);
        drawIcon(x+5, y, 0x27 + add);
        drawIcon(x+6, y, 0x27 + add);
        drawIcon(x+7, y, 0x27 + add);
        drawIcon(x+8, y, 0x27 + add);
        drawIcon(x+9, y, 0x27 + add);
        drawIcon(x+10, y, 0x27 + add);
        drawIcon(x+11, y, 0x09 + add);

        // Name label
        drawIcon(x, y+11, 0x5E + add);
        drawIcon(x+1,y+11, 0x60 + add);
        drawIcon(x+2,y+11, 0x60 + add);
        drawIcon(x+3,y+11, 0x60 + add);
        drawIcon(x+4,y+11, 0x60 + add);
        drawIcon(x+5,y+11, 0x60 + add);
        drawIcon(x+6,y+11, 0x60 + add);
        drawIcon(x+7,y+11, 0x60 + add);
        drawIcon(x+8,y+11, 0x60 + add);
        drawIcon(x+9,y+11, 0x60 + add);
        drawIcon(x+10,y+11, 0x60 + add);
        drawIcon(x+11,y+11, 0x5F + add);
        plotName(x+1,y+11, active ? 1 : 2, name);

        // Active indicator
        if (active)
            drawIcon(x+1,y+11,0x5B);

        // Bottom border below name label
        drawIcon(x,y+12, 0x20 + add);
        drawIcon(x+1,y+12,0x28 + add);
        drawIcon(x+2,y+12,0x28 + add);
        drawIcon(x+3,y+12,0x28 + add);
        drawIcon(x+4,y+12,0x28 + add);
        drawIcon(x+5,y+12,0x28 + add);
        drawIcon(x+6,y+12,0x28 + add);
        drawIcon(x+7,y+12,0x28 + add);
        drawIcon(x+8,y+12,0x28 + add);
        drawIcon(x+9,y+12,0x28 + add);
        drawIcon(x+10,y+12,0x28 + add);
    }
    else
    {
        // Top player boards

        // top border ABOVE name label
        drawIcon(x, y-1, 0x05);
        drawIcon(x+1,y-1,0x26);
        drawIcon(x+2,y-1,0x26);
        drawIcon(x+3,y-1,0x26);
        drawIcon(x+4,y-1,0x26);
        drawIcon(x+5,y-1,0x26);
        drawIcon(x+6,y-1,0x26);
        drawIcon(x+7,y-1,0x26);
        drawIcon(x+8,y-1,0x26);
        drawIcon(x+9,y-1,0x26);
        drawIcon(x+10,y-1,0x26);
        drawIcon(x+11, y-1, 0x06);

        // Name label
        drawIcon(x, y, 0x5C + add);
        drawIcon(x+1, y, 0x60 + add);
        drawIcon(x+2, y, 0x60 + add);
        drawIcon(x+3, y, 0x60 + add);
        drawIcon(x+4, y, 0x60 + add);
        drawIcon(x+5, y, 0x60 + add);
        drawIcon(x+6, y, 0x60 + add);
        drawIcon(x+7, y, 0x60 + add);
        drawIcon(x+8, y, 0x60 + add);
        drawIcon(x+9, y, 0x60 + add);
        drawIcon(x+10, y, 0x60 + add);
        plotName(x+1, y, active ? 1 : 2, name);

        // Active indicator
        if (active)
            drawIcon(x+1, y, 0x5B);

        // Thin Horizontal Border
        drawIcon(x, y+11, 0x0A + add);
        drawIcon(x+1, y+11, 0x29 + add);
        drawIcon(x+2, y+11, 0x29 + add);
        drawIcon(x+3, y+11, 0x29 + add);
        drawIcon(x+4, y+11, 0x29 + add);
        drawIcon(x+5, y+11, 0x29 + add);
        drawIcon(x+6, y+11, 0x29 + add);
        drawIcon(x+7, y+11, 0x29 + add);
        drawIcon(x+8, y+11, 0x29 + add);
        drawIcon(x+9, y+11, 0x29 + add);
        drawIcon(x+10, y+11, 0x29 + add);
        drawIcon(x+11, y+11, 0x0B + add);
    }

    // Draw left/right borders and drawers
    if (player > 1 || playerCount == 2 && player > 0)
    {
        // Right drawer
        // top
        drawIcon(x+11,y+1,0x25 + add);
        drawIcon(x+12,y+1,0x31 + add);
        drawIcon(x+13,y+1,0x31 + add);
        drawIcon(x+14,y+1,0x31 + add);
        drawIcon(x+15,y+1,0x2D + add);
        drawIcon(x,y+1,0x22 + add);

        // Edges
        for (i=0;i<8;i++)
        {
            drawIcon(x+11, y+2+i, 0x03 + add);
            drawIcon(x+15, y+2+i, 0x02 + add);
            drawIcon(x, y+2+i, 0x22 + add);
        }

        // bottom
        drawIcon(x, y+10, 0x22 + add);
        drawIcon(x+1,y+10, 0x31 + add);
        drawIcon(x+2,y+10, 0x31 + add);
        drawIcon(x+3,y+10, 0x31 + add);
        drawIcon(x+11, y+10, 0x25 + add);
        drawIcon(x+15, y+10, 0x2F + add); // Left edge
    }
    else
    {
        // Left drawer
        drawIcon(x-4,y+1,0x2C+add);
        drawIcon(x-3,y+1,0x31+add);
        drawIcon(x-2,y+1,0x31+add);
        drawIcon(x-1,y+1,0x31+add);
        drawIcon(x,y+1,0x24+add);
        drawIcon(x+11,y+1,0x23 + add);

        // Edges
        for (i=0;i<8;i++)
        {
            drawIcon(x-4, y+2+i, 0x02 + add);
            drawIcon(x, y+2+i, 0x02 + add);
            drawIcon(x+11, y+2+i, 0x22 + add);
        }

        drawIcon(x-4,y+10,0x2E+add);
        drawIcon(x-3,y+10,0x31+add);
        drawIcon(x-2,y+10,0x31+add);
        drawIcon(x-1,y+10,0x31+add);
        drawIcon(x,y+10,0x24+add);
        drawIcon(x+11,y+10,0x23+add); // Right edge
    }
}

/**
 * @brief Draw the board
 * @param currentPlayerCount The current # of players
 */
void drawBoard(unsigned char currentPlayerCount)
{
    int i=0;

    playerCount = currentPlayerCount;

    fieldX = playerCount > 2 ? 0 : 7;

    for (i=0; i< playerCount; i++)
    {
        drawPlayerName(i, "", false);
    }
}

/**
 * @brief draw a horizontal line of w characters at x,y
 * @param x Horizontal position (0-39)
 * @param y Vertical position (0-24)
 * @param w Width (0-39)
 */
void drawLine(unsigned char x, unsigned char y, unsigned char w)
{
    while (w--)
        plot_tile(&charset[0x3F], x++, y);
}
