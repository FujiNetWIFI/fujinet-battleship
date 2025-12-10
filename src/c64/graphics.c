/*
  Graphics functionality for Commodore 64
*/

#include <peekpoke.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#include "../platform-specific/graphics.h"
#include "vars.h"

extern unsigned char charset[];
extern void irqVsyncWait(void);

#define SCREEN_LOC ((uint8_t *)0xCC00)
#define COLOR_LOC ((uint8_t *)0xD800)
#define CHARSET_LOC 0xC000

#define xypos(x, y) (SCREEN_LOC + x + (y) * WIDTH)
#define colorpos(x, y) (COLOR_LOC + x + (y) * WIDTH)

// These started at 0x00
#define TOP_BORDER_START 0x25
#define TOP_BORDER_END 0x26
#define TILE_BORDER1_TOP_L 0x28
#define TILE_BORDER1_TOP_R 0x29
#define TILE_BORDER1_TOP 0x47 //(0xD2 + MAGIC_SHIFT2)
#define TILE_BORDER1_BOTTOM_L 0x2a
#define TILE_BORDER1_BOTTOM_R 0x2b
#define TILE_BORDER1_BOTTOM 0x49
#define TILE_LINE_H 0x5F
// These seem to be at 40
#define TILE_BOTTOM_BORDER_START 0x40
#define TILE_BOTTOM_BORDER_END 0x41
#define TILE_TOP_BORDER 0x46
#define TILE_BOTTOM_BORDER 0x48

#define TILE_HIT 0x59 // 0x39
#define TILE_HIT2 0x3b // lub 9b, było 0x1B

#define TILE_HIT_LEGOND 0x3c // ???(0x2D + MAGIC_SHIFT3)   // -

#define TILE_ACTIVE_INDICATOR 0x1b

// ????

#define TILE_SHIP_LEFT 0x52
#define TILE_SHIP_HULL_H 0x53
#define TILE_SHIP_RIGHT 0x54
#define TILE_SHIP_BOTTOM 0x55
#define TILE_SHIP_HULL_V 0x56
#define TILE_SHIP_TOP 0x57

#define TILE_TOP_LEFT_CORNET 0x5b // było 3b
#define TILE_TOP_RIGHT_CORNET 0x5c
#define TILE_BOTTOM_LEFT_CORNET 0x5d
#define TILE_BOTTOM_RIGHT_CORNET 0x5e

#define MAGIC_SHIFT4 32
#define TILE_CLOCK 0x3d
#define TILE_MISS (0x58 + MAGIC_SHIFT4)         // X
#define TILE_CURSOR (0x5B + MAGIC_SHIFT4)
// #define TILE_NAME_START (0x5C + MAGIC_SHIFT4)
// #define TILE_NAME_END (0x5D + MAGIC_SHIFT4)
// #define TILE_LABEL_START (0x5E + MAGIC_SHIFT4)
// #define TILE_LABEL_END (0x5F + MAGIC_SHIFT4)
#define TILE_NAME_START (0x1C)
#define TILE_NAME_END (0x1D)
#define TILE_LABEL_START (0x1E)
#define TILE_LABEL_END (0x1F)

#define MAGIC_SHIFT2 -64
#define TILE_SEA 0          // Space
#define TILE_LABEL (0x60 + MAGIC_SHIFT2)
#define TILE_VERTICAL_LINE (0x7C + MAGIC_SHIFT2)


// Color definitions (0-15 for C64)
#define COLOR_BORDER 0
#define COLOR_BG 0
#define COLOR_CURSOR 1     // White
#define COLOR_SHIP 5       // Green
#define COLOR_HIT 2        // Red
#define COLOR_MISS 4       // Purple
#define COLOR_TEXT 2       // Red
#define COLOR_TEXT_ALT 5   // Green

#define VIC_MEMORY_SETUP_REGISTER 0xD018
#define CIA2_VIDEO_BANK_REGISTER 0xDD00

static uint8_t fieldX = 0, playerCount = 0;

// State for VIC bank switching to use RAM charset at CHARSET_LOC ($1000)
static uint8_t _saved_d018 = 0;
static uint8_t _saved_dd00 = 0;
static uint8_t _saved_d016 = 0;

static uint16_t quadrant_offset[] = {
    WIDTH * 14 + 8,
    WIDTH * 2 + 8,
    WIDTH * 2 + 21,
    WIDTH * 14 + 21};

uint8_t legendShipOffset[] = {2, 1, 0, 40 * 5, 40 * 6 + 1};

// Defined in this file
void drawTextAdd(uint8_t *dest, const char *s, uint8_t add);
void drawShipInternal(uint8_t *dest, uint8_t size, uint8_t delta);

unsigned char toLowerCase(unsigned char c)
{
    if (c >= 64 && c <= 96)
    {
        // Use alternate numbers if showing the clock
        // if (y == HEIGHT - 1 && c >= 0x30 && c <= 0x39)
        //     c += 0x60;
        c -= 64;
    } else if (c >= 0x60 && c <= 0x7F) {
        // lowercase letters
        c -= 0x60;
    };
        
    return c;
};

unsigned char cycleNextColor()
{
    return 0;
};

void initGraphics()
{
    memcpy((void *)CHARSET_LOC, &charset, 2048);
    // Configure the C64's memory layout for custom character set:
    // 1. Set up RAM bank for character ROM access
    // CIA2 port A (56576) controls RAM bank selection
    // Mask with 252 (11111100) to select the correct bank 3 for $C000
    _saved_dd00 = PEEK(CIA2_VIDEO_BANK_REGISTER);    
    POKE(CIA2_VIDEO_BANK_REGISTER,PEEK(CIA2_VIDEO_BANK_REGISTER)&252);
    // 2. Configure VIC-II to use our custom character set
    // VIC_MEMORY_SETUP_REGISTER is the VIC-II control register for character ROM location
    // Setting to 32 tells VIC-II to use character ROM at $2000 (8192)
    // This points to our custom character set loaded at $C000
    // characters at offset 0, screen memory starts at $Cc00
    _saved_d018 = PEEK(VIC_MEMORY_SETUP_REGISTER);
    POKE(VIC_MEMORY_SETUP_REGISTER,0x30); 

    // Set border and background colors
    POKE(0xD020, COLOR_BORDER); // Border color
    POKE(0xD021, COLOR_BG);     // Background color
    POKE(0xD022, 1); // white
    POKE(0xD023, 6); // blue
    // Enable multicolor character mode (set bit 4 in VIC register $D016)
    _saved_d016 = PEEK(0xD016);
    POKE(0xD016, PEEK(0xD016) | 0x10);    
    // Clear screen memory
    memset(SCREEN_LOC, TILE_SEA, 1000);
    
    // Set all character colors to text color
    memset(COLOR_LOC, COLOR_TEXT, 1000);
}

void resetGraphics()
{
    // Reset to default colors
    POKE(0xD020, 254);
    POKE(0xD021, 246);
    // Restore previous memory mapping (if we changed it)
    POKE(VIC_MEMORY_SETUP_REGISTER, _saved_d018);
    POKE(CIA2_VIDEO_BANK_REGISTER, _saved_dd00);
    POKE(0xD016, _saved_d016);
    memset(SCREEN_LOC, TILE_SEA, 1000);
}

bool saveScreenBuffer()
{
    return false;
}

void restoreScreenBuffer()
{
}

void drawText(unsigned char x, unsigned char y, const char *s)
{
    uint8_t *pos = xypos(x, y);
    uint8_t *col_pos = colorpos(x, y);
    unsigned char c;

    while ((c = *s++))
    {
        c = toLowerCase(c);        
        *pos++ = c;
        *col_pos++ = COLOR_TEXT;
    }
}

void drawTextAlt(unsigned char x, unsigned char y, const char *s)
{
    static uint8_t c;
    static uint8_t *pos;
    uint8_t *col_pos = colorpos(x, y);
    static uint8_t color;

    pos = xypos(x, y);

    while (c = *s++)
    {
        if (c >= 65 && c <= 90)
        {
            color = COLOR_TEXT_ALT;  // Alternate color
        }
        else
        {
            color = COLOR_TEXT;
        }

        c = toLowerCase(c);

        *pos++ = c;
        *col_pos++ = color;
    }
}


void resetScreen()
{
    memset(SCREEN_LOC, TILE_SEA, 1000);
    memset(COLOR_LOC, COLOR_TEXT, 1000);
}

void drawIcon(unsigned char x, unsigned char y, unsigned char icon)
{
    *xypos(x, y) = icon;
    *colorpos(x, y) = COLOR_CURSOR;
}

void drawBlank(unsigned char x, unsigned char y)
{
    *xypos(x, y) = TILE_SEA;
    *colorpos(x, y) = COLOR_BG;
}

void drawSpace(unsigned char x, unsigned char y, unsigned char w)
{
    memset(xypos(x, y), TILE_SEA, w);
    memset(colorpos(x, y), COLOR_BG, w);
}

void drawClock()
{
    POKE(xypos(WIDTH - 1, HEIGHT - 1), TILE_CLOCK);
    *colorpos(WIDTH - 1, HEIGHT - 1) = COLOR_CURSOR;
}

void drawConnectionIcon(bool show)
{
    POKEW(xypos(0, HEIGHT - 1), show ? 0x3f3e : 0);
    *colorpos(0, HEIGHT - 1) = COLOR_CURSOR;
}


void drawTextAdd(uint8_t *dest, const char *s, uint8_t add)
{
    uint8_t *col_dest = COLOR_LOC + (dest - SCREEN_LOC);
    char c;

    while ((c = *s++))
    {
        c = toLowerCase(c);
        
        *dest++ = c;
        *col_dest++ = add ? COLOR_HIT : COLOR_TEXT;
    }
}

void drawPlayerName(uint8_t player, const char *name, bool active)
{
    static uint8_t i, add;
    uint8_t *dest = SCREEN_LOC + fieldX + quadrant_offset[player] - WIDTH - 1;
    add = active ? 0 : 128;

    // Draw top and bottom borders and name label
    if (player == 0 || player == 3)
    {
        // Bottom player boards

        // Thin horizontal border
        dest[0] = TILE_BORDER1_TOP_L + add;
        dest[11] = TILE_BORDER1_TOP_R + add;
        memset(dest + 1, 0x27 + add, 10);

        // Name Label
        dest[WIDTH * 11] = 0x5E + add;
        dest[WIDTH * 11 + 11] = 0x5F + add;
        memset(WIDTH * 11 + 1 + dest, 0x60 + add, 10);
        drawTextAdd(dest + WIDTH * 11 + 2, name, add);

        // Active indicator
        if (active)
        {
            dest[WIDTH * 11 + 1] = TILE_ACTIVE_INDICATOR;
        }

        // Bottom border below name label
        dest[WIDTH * 12] = TILE_BOTTOM_BORDER_START + add;
        dest[WIDTH * 12 + 11] = TILE_BOTTOM_BORDER_END + add;
        memset(WIDTH * 12 + 1 + dest, TILE_BOTTOM_BORDER + add, 10);
    }
    else
    {
        // Top player boards

        // Top border above name label
        *(dest - WIDTH) = 0x5;
        *(dest - WIDTH + 11) = 0x6;
        memset(dest - WIDTH + 1, 0x26, 10);

        // Name Label
        dest[0] = 0x5C + add;
        dest[11] = 0x5D + add;
        memset(dest + 1, 0x60 + add, 10);
        drawTextAdd(dest + 2, name, add);

        // Active indicator
        if (active)
        {
            dest[1] = TILE_ACTIVE_INDICATOR;
        }

        // Thin horizontal border
        dest[WIDTH * 11] = TILE_BORDER1_BOTTOM_L + add;
        dest[WIDTH * 11 + 11] = TILE_BORDER1_BOTTOM_L + add;
        memset(WIDTH * 11 + 1 + dest, TILE_BORDER1_BOTTOM + add, 10);
    }

    // Draw left/right borders and drawers
    if (player > 1 || playerCount == 2 && player > 0)
    {
        // Right drawer
        dest += WIDTH;
        memset(dest + 12, 0x31 + add, 3);
        dest[11] = 0x25 + add;
        dest[15] = 0x2D + add;
        dest[0] = 0x22 + add; // Left edge

        for (i = 0; i < 8; i++)
        {
            dest += WIDTH;
            dest[11] = 0x03 + add;
            dest[15] = 0x02 + add;
            dest[0] = 0x22 + add; // Left edge
        }

        dest += WIDTH;
        memset(dest + 12, 0x31 + add, 3);
        dest[11] = 0x25 + add;
        dest[15] = 0x2F + add;
        dest[0] = 0x22 + add; // Left edge
    }
    else
    {
        // Left drawer
        dest += WIDTH - 4;
        dest[0] = 0x2C + add;
        memset(dest + 1, 0x31 + add, 3);
        dest[4] = 0x24 + add;
        dest[15] = 0x23 + add; // Right edge

        for (i = 0; i < 8; i++)
        {
            dest += WIDTH;
            dest[0] = dest[4] = 0x02 + add;
            dest[15] = 0x23 + add; // Right edge
        }

        dest += WIDTH;
        dest[0] = 0x2E + add;
        memset(dest + 1, 0x31 + add, 3);
        dest[4] = 0x24 + add;
        dest[15] = 0x23 + add; // Right edge
    }
}

void drawBoard(uint8_t currentPlayerCount)
{
    static uint8_t i, y;
    static uint8_t *dest;
    playerCount = currentPlayerCount;
    fieldX = playerCount > 2 ? 0 : 7;

    // if (playerCount > 1 && !inGameCharSet)
    // {
    //     // Invert 0-9 & A-Z characters for in-game charset
    //     inGameCharSet = true;

    //     dest = CHARSET_LOC + 0x01 * 8;
    //     while (dest < CHARSET_LOC + 0x5b * 8)
    //     {
    //         *dest = *dest ^ 0xff | 0b01010101;
    //         dest++;
    //         if (dest == CHARSET_LOC + 0x1A * 8)
    //             dest = CHARSET_LOC + 0x40 * 8;
    //         if (dest == CHARSET_LOC + 0x02 * 8)
    //             dest = CHARSET_LOC + 0x10 * 8;
    //     }
    // }

    for (i = 0; i < playerCount; i++)
    {
        dest = SCREEN_LOC + fieldX + quadrant_offset[i];

        // Draw player border
        drawPlayerName(i, "", false);

        // Blue gamefield
        for (y = 0; y < 10; y++)
        {
            memset(dest + y * WIDTH, TILE_SEA, 10);
        }

        // Blue drawer
        if (i > 1 || (i > 0 && fieldX > 0))
        {
            dest += WIDTH + 11;
        }
        else
        {
            dest += WIDTH - 4;
        }
        for (y = 0; y < 8; y++)
        {
            memset(dest, TILE_SEA, 3);
            dest += WIDTH;
        }
    }

    dest = xypos(16, 0);
}

void drawLine(unsigned char x, unsigned char y, unsigned char w)
{
    memset(xypos(x, y), TILE_LINE_H, w);
    memset(colorpos(x, y), 8, w);
}

void drawShipInternal(uint8_t *dest, uint8_t size, uint8_t delta)
{
    static uint8_t i;
    uint8_t c = TILE_SHIP_LEFT;
    if (delta)
        c = TILE_SHIP_TOP;

    for (i = 0; i < size; i++)
    {
        *dest = c;
        if (delta)
        {
            c = TILE_SHIP_HULL_V;
            if (i == size - 2)
                c = TILE_SHIP_BOTTOM;
            dest += WIDTH;
        }
        else
        {
            dest++;
            c = TILE_SHIP_HULL_H;
            if (i == size - 2)
                c = TILE_SHIP_RIGHT;
        }
    }
}

void drawShip(uint8_t size, uint8_t pos, bool hide)
{
    uint8_t i, delta = 0;
    uint8_t *dest;

    if (pos > 99)
    {
        delta = 1; // 1=vertical, 0=horizontal
        pos -= 100;
    }

    dest = xypos((pos % 10), (pos / 10)) + fieldX + quadrant_offset[0];

    if (hide)
    {
        if (!delta)
            memset(dest, 0x38, size);
        else
            for (i = 0; i < size; i++)
                *(dest + i * WIDTH) = 0x38;
        return;
    }

    drawShipInternal(dest, size, delta);
}

void drawLegendShip(uint8_t player, uint8_t index, uint8_t size, uint8_t status)
{
    static uint8_t i;
    uint8_t *dest = SCREEN_LOC + fieldX + quadrant_offset[player] + legendShipOffset[index];

    if (player > 1 || (player > 0 && fieldX > 0))
    {
        dest += WIDTH + 11;
    }
    else
    {
        dest += WIDTH - 4;
    }

    if (status)
    {
        drawShipInternal(dest, size, 1);
    }
    else
    {
        for (i = 0; i < size; i++)
            *(dest + i * WIDTH) = TILE_HIT_LEGOND;
    }
}

void drawGamefield(uint8_t quadrant, uint8_t *field)
{
    static uint8_t y, x;
    uint8_t *dest = SCREEN_LOC + quadrant_offset[quadrant] + fieldX;

    for (y = 0; y < 10; ++y)
    {
        for (x = 0; x < 10; ++x)
        {
            if (*field)
            {
                *dest = *field == 1 ? TILE_HIT : TILE_MISS;
            }
            field++;
            dest++;
        }

        dest += WIDTH - 10;
    }
}

static bool cursorVisible = false;
void drawGamefieldUpdate(uint8_t quadrant, uint8_t *gamefield, uint8_t attackPos, uint8_t blink)
{
    uint8_t *dest = SCREEN_LOC + quadrant_offset[quadrant] + fieldX + (uint16_t)(attackPos / 10) * WIDTH + (attackPos % 10);
    uint8_t c = gamefield[attackPos];

    if (cursorVisible)
    {
        cursorVisible = false;
        //memset(PM_BASE + 1024, 0, 768);
    }

    // Animate attack (only in empty sea cell)
    if (blink > 9 && (*dest == TILE_SEA || *dest > 226))
    {
        *dest = 217 + blink;
        return;
    }

    if (c == FIELD_ATTACK)
    {
        *dest = blink ? TILE_HIT2 : TILE_HIT;
    }
    else if (c == FIELD_MISS)
    {
        *dest = TILE_MISS;
    }
    else
    {
        return;
    }
}
void drawGamefieldCursor(uint8_t quadrant, uint8_t x, uint8_t y, uint8_t *gamefield, uint8_t blink)
{
    uint16_t pos = quadrant_offset[quadrant] + y * WIDTH + fieldX + x;
    // uint16_t lc = lastCursor[quadrant];

    // lastCursor[quadrant] = PM_BASE + 768 + 24 + quadrant * 256 + (pos / WIDTH) * 8;

    // memset(lc, 0, 8);
    // memcpy(lastCursor[quadrant], &cursor_pmg, 8);

    // POKE(0xd000 - 1 + quadrant, (pos % WIDTH) * 4 + 48); // horiz loc
    cursorVisible = true;

    (void)gamefield;
}

void drawEndgameMessage(const char *message)
{
    uint8_t i, x;
    i = (uint8_t)strlen(message);
    x = WIDTH / 2 - i / 2;

    memset(xypos(0, HEIGHT - 2), 0x62, WIDTH);
    memset(xypos(0, HEIGHT - 1), 0x40, x);
    memset(xypos(x + i, HEIGHT - 1), 0x40, WIDTH - x - i);
    drawText(x, HEIGHT - 1, message);
}

void drawBox(unsigned char x, unsigned char y, unsigned char w, unsigned char h)
{
    
    uint8_t *pos = xypos(x, y);
    uint8_t *col_pos = colorpos(x, y);
    // Draw corners
    *pos = TILE_TOP_LEFT_CORNET;
    *col_pos = 8;
    pos[w + 1] = TILE_TOP_RIGHT_CORNET;
    col_pos[w + 1] = 8;
    pos[h * WIDTH + WIDTH] = TILE_BOTTOM_LEFT_CORNET;
    col_pos[h * WIDTH + WIDTH] = 8;
    pos[w + h * WIDTH + WIDTH + 1] = TILE_BOTTOM_RIGHT_CORNET;
    col_pos[w + h * WIDTH + WIDTH + 1] = 8;
}

void waitvsync()
{
    //irqVsyncWait();
}

