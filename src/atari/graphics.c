/*
  Graphics functionality
*/

#include <peekpoke.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <atari.h>
#include "../platform-specific/graphics.h"
#include "vars.h"

extern unsigned char charset[];
// I'm using the space XL/XE allocates to BASIC for player/missles, charset, and screen buffer/backup
// On graphics initialization, I turn off BASIC, freeing up this space.

// There are two startup scenarios for XL/XE that must be considered:
// 1. If BASIC is disabled, the cc65 Stack will grow down from $BC1F, so to leave 512 bytes for stack,
//    I'm allowing $A000-$BA1F for charset, screen/buf, and pmgraphics.

// 2. If BASIC is enabled, the stack grows down from $9C1F. So, again allowing for 512 byte stack,
//    $9A1F is as high as we want to allow the program to reach without needing custom cc65 configs/segments

// Bottom line, program+vars needs to stay below $9A1F

#define CHARSET_LOC 0xB000
#define SCREEN_LOC ((uint8_t *)0xB400)
#define SCREEN_BAK 0xAB00
#define PM_BASE 0xA000

#define xypos(x, y) (SCREEN_LOC + x + (y) * WIDTH)

#define TILE_SEA 0x38
#define TILE_MISS 0xE1
#define TILE_HIT 0x39
#define TILE_HIT2 0x1B
#define TILE_HIT_LEGOND 0x1C

static uint8_t colorMode = 0, oldChbas = 0, colIndex = 0, fieldX = 0, playerCount, box_color = 0xff;
static bool inGameCharSet = false;
static uint16_t lastCursor[] = {0, PM_BASE + 1024, PM_BASE + 1024, PM_BASE + 1024};

static uint16_t quadrant_offset[] = {
    WIDTH * 14 + 8,
    WIDTH * 2 + 8,
    WIDTH * 2 + 21,
    WIDTH * 14 + 21};
uint8_t legendShipOffset[] = {2, 1, 0, 40 * 5, 40 * 6 + 1};

// 26 lines
void DisplayList =
    {
        DL_BLK8, DL_BLK8,                                                     // 2 Blanks Lines
        DL_LMS(DL_CHR40x8x4), SCREEN_LOC,                                     // 1 Line
        DL_CHR40x8x4, DL_CHR40x8x4, DL_CHR40x8x4, DL_CHR40x8x4, DL_CHR40x8x4, // 5 Lines
        DL_CHR40x8x4, DL_CHR40x8x4, DL_CHR40x8x4, DL_CHR40x8x4, DL_CHR40x8x4, // 5 Lines
        DL_CHR40x8x4, DL_CHR40x8x4, DL_CHR40x8x4, DL_CHR40x8x4, DL_CHR40x8x4, // 5 Lines
        DL_CHR40x8x4, DL_CHR40x8x4, DL_CHR40x8x4, DL_CHR40x8x4, DL_CHR40x8x4, // 5 Lines
        DL_CHR40x8x4, DL_CHR40x8x4, DL_CHR40x8x4, DL_CHR40x8x4, DL_CHR40x8x4, // 5 Lines
        DL_JVB                                                                /* JVB Destination will be updated at the real display list location*/
};

// Color scheme:
// Player/Missle: Target cursor, UNUSED, UNUSED, UNUSED,
// White, Blue, Red, Green, Black background
const unsigned char colors[] = {
    0xff, 0xff, 0xff, 0xff, 0x00, 0x0F, 0x84, 0x46, 0xC8, 0x02, // NTSC
    0xff, 0xff, 0xff, 0xff, 0x00, 0x0F, 0x74, 0x26, 0xB8, 0x02  // PAL TBD
};

const uint8_t cursor_pmg[] = {
    0b11110000,
    0b10010000,
    0b10010000,
    0b00000000,
    0b00000000,
    0b10010000,
    0b10010000,
    0b11110000};

// Defined in this file
void drawTextAdd(uint8_t *dest, const char *s, uint8_t add);
void drawShipInternal(uint8_t *dest, uint8_t size, uint8_t delta);

unsigned char cycleNextColor()
{
    return 0;
}

void initGraphics()
{
    // Check if BASIC is enabled
    if (PEEKW(0x2E5) < 0xBC1F && !(PEEK(0xD301) & 2))
    {
        waitvsync();

        // Disable BASIC by setting bit 1 of 0xD301
        POKE(0xD301, PEEK(0xD301) | 2);

        // Disable BASIC at shadow location
        POKE(0x3F8, 1);

        // Clear pmg memory
        memset(PM_BASE, 0, 0x800);
    }

    resetScreen();

    // Set the displaylist end JVB instruction to point to the start of the display list
    POKEW(PEEKW(0x230) + 31, OS.sdlst);

    // Overwrite current Display list with custom
    waitvsync();
    memcpy(OS.sdlst, &DisplayList, sizeof(DisplayList));

    // Load custom charset
    memcpy((void *)CHARSET_LOC, &charset, 1024);
    oldChbas = OS.chbas;
    OS.chbas = CHARSET_LOC / 256;

    // Initialize player+missle graphics, single line
    OS.sdmctl = OS.sdmctl | (12 + 16);

    OS.gprior = 1; // PM graphics above playfield

    POKE(0xD407, PM_BASE / 256);

    // PM
    // Turn on P+M
    POKE(0xD01D, 3);

    // PAL colors
    if (PEEK(0xD014) == 1)
    {
        colIndex = 10;
    }

    // Set color palette
    memcpy(&OS.pcolr0, &colors[colIndex + 1], 9);
}

bool saveScreenBuffer()
{
    return false;
    // memcpy(SCREEN_BAK, SCREEN_LOC, WIDTH * HEIGHT);
}

void restoreScreenBuffer()
{
    // waitvsync();
    // memcpy(SCREEN_LOC, SCREEN_BAK, WIDTH * HEIGHT);
}

void drawText(unsigned char x, unsigned char y, const char *s)
{
    static unsigned char c;
    static unsigned char *pos;

    pos = xypos(x, y);

    while (c = *s++)
    {

        if (c > 90 || (c < 65 && c > 32))
            c -= 32;
        else if (c == 0x20)
            c = 0x40;
        *pos++ = c;
    }
}

void drawTextAlt(unsigned char x, unsigned char y, const char *s)
{
    static uint8_t c;
    static uint8_t *pos;

    pos = xypos(x, y);

    while (c = *s++)
    {
        if (c > 90 || (c < 65 && c >= 32))
        {
            // Use alternate numbers if showing the clock
            if (inGameCharSet && y == HEIGHT - 1 && c >= 0x30 && c <= 0x39)
                c += 0x60;
            c += 96;
        }

        *pos++ = c;
    }
}

void resetScreen()
{
    waitvsync();
    memset((void *)SCREEN_LOC, 0, WIDTH * HEIGHT);
    if (inGameCharSet)
    {
        // Restore normal charset
        memcpy((void *)CHARSET_LOC, &charset, 1024);
        inGameCharSet = false;

        // Clear any cursors
        memset(PM_BASE + 1024, 0, 768);
    }
}

void drawIcon(unsigned char x, unsigned char y, unsigned char icon)
{
    POKE(xypos(x, y), icon);
}

void drawBlank(unsigned char x, unsigned char y)
{
    POKE(xypos(x, y), 0);
}

void drawSpace(unsigned char x, unsigned char y, unsigned char w)
{
    memset(xypos(x, y), 0, w);
}

void drawClock()
{
    POKE(xypos(WIDTH - 1, HEIGHT - 1), 0x1D);
}

void drawConnectionIcon(bool show)
{
    POKEW(xypos(0, HEIGHT - 1), show ? 0x1f1e : 0);
}

void drawTextAdd(uint8_t *dest, const char *s, uint8_t add)
{
    char c;

    while ((c = *s++))
    {
        if (c >= 97 && c <= 122)
            c -= 32;
        else if (c == 0x20)
            c = 0x40;
        *dest++ = c + add;
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
        dest[0] = 0x08 + add;
        dest[11] = 0x09 + add;
        memset(dest + 1, 0x27 + add, 10);

        // Name Label
        dest[WIDTH * 11] = 0x5E + add;
        dest[WIDTH * 11 + 11] = 0x5F + add;
        memset(WIDTH * 11 + 1 + dest, 0x60 + add, 10);
        drawTextAdd(dest + WIDTH * 11 + 2, name, add);

        // Active indicator
        if (active)
        {
            dest[WIDTH * 11 + 1] = 0x5b;
        }

        // Bottom border below name label
        dest[WIDTH * 12] = 0x20 + add;
        dest[WIDTH * 12 + 11] = 0x21 + add;
        memset(WIDTH * 12 + 1 + dest, 0x28 + add, 10);
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
            dest[1] = 0x5b;
        }

        // Thin horizontal border
        dest[WIDTH * 11] = 0x0A + add;
        dest[WIDTH * 11 + 11] = 0x0B + add;
        memset(WIDTH * 11 + 1 + dest, 0x29 + add, 10);
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

    if (playerCount > 1 && !inGameCharSet)
    {
        // Invert 0-9 & A-Z characters for in-game charset
        inGameCharSet = true;

        dest = CHARSET_LOC + 0x01 * 8;
        while (dest < CHARSET_LOC + 0x5b * 8)
        {
            *dest = *dest ^ 0xff | 0b01010101;
            dest++;
            if (dest == CHARSET_LOC + 0x1A * 8)
                dest = CHARSET_LOC + 0x40 * 8;
            if (dest == CHARSET_LOC + 0x02 * 8)
                dest = CHARSET_LOC + 0x10 * 8;
        }
    }

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
    memset(xypos(x, y), 0x3F, w);
}

void drawShipInternal(uint8_t *dest, uint8_t size, uint8_t delta)
{
    static uint8_t i;
    uint8_t c = 0x32;
    uint8_t *src;
    if (delta)
        c = 0x37;

    for (i = 0; i < size; i++)
    {
        *dest = c;
        if (delta)
        {
            c = 0x36;
            if (i == size - 2)
                c = 0x35;
            dest += WIDTH;
        }
        else
        {
            dest++;
            c = 0x33;
            if (i == size - 2)
                c = 0x34;
        }
    }
}

void drawShip(uint8_t quadrant, uint8_t size, uint8_t pos, bool hide)
{
    uint8_t i, delta = 0;
    uint8_t *dest;

    if (pos > 99)
    {
        delta = 1; // 1=vertical, 0=horizontal
        pos -= 100;
    }

    dest = xypos((pos % 10), (pos / 10)) + fieldX + quadrant_offset[quadrant];

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
void drawGamefieldUpdate(uint8_t quadrant, uint8_t *gamefield, uint8_t attackPos, uint8_t anim)
{
    uint8_t *dest = SCREEN_LOC + quadrant_offset[quadrant] + fieldX + (uint16_t)(attackPos / 10) * WIDTH + (attackPos % 10);
    uint8_t c = gamefield[attackPos];

    if (cursorVisible)
    {
        cursorVisible = false;
        memset(PM_BASE + 1024, 0, 768);
    }

    // Animate attack only
    if (anim > 9)
    {
        *dest = 217 + anim;
        return;
    }

    if (c == FIELD_ATTACK)
    {
        *dest = anim ? TILE_HIT2 : TILE_HIT;
    }
    else if (c == FIELD_MISS)
    {
        *dest = TILE_MISS;
    }
}
void drawGamefieldCursor(uint8_t quadrant, uint8_t x, uint8_t y, uint8_t *gamefield, uint8_t blink)
{
    uint16_t pos = quadrant_offset[quadrant] + y * WIDTH + fieldX + x;
    uint16_t lc = lastCursor[quadrant];

    lastCursor[quadrant] = PM_BASE + 768 + 24 + quadrant * 256 + (pos / WIDTH) * 8;

    memset(lc, 0, 8);
    memcpy(lastCursor[quadrant], &cursor_pmg, 8);

    POKE(0xd000 - 1 + quadrant, (pos % WIDTH) * 4 + 48); // horiz loc
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
    // Draw corners
    *pos = 0x3b;
    pos[w + 1] = 0x3c;
    pos[h * WIDTH + WIDTH] = 0x3d;
    pos[w + h * WIDTH + WIDTH + 1] = 0x3e;
}

void resetGraphics()
{
    OS.color4 = 2;
    OS.chbas = oldChbas;
    waitvsync();
}
