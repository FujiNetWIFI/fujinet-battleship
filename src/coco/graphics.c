/*
  Graphics functionality
*/

#include "hires.h"
#include <peekpoke.h>
#include <string.h>
#include "../platform-specific/graphics.h"
#include "../platform-specific/sound.h"
#include <coco.h>

extern uint8_t charset[];
uint8_t playerCount;
#ifdef COCO3

// The 32k of the screen buffer is stored in MMU blocks 52-55
// Block 52 points to absolute address $68000.
// The value to put in $FF9D to show this screen is $68000 / 8 = $D000.
// Since 52 is the 5th MMU block, the local address is $8000.
// This allows the program to occupy up to the normal 32K limit.
//
// Task 1 is swapped in whenever drawing graphics, and swapped out so
// normal IO/FujiNet operations can occur.
static const byte task1MMUBlocks[8] =
    {
        56,
        57,
        58,
        59, // Default blocks (DO NOT CHANGE)
        52,
        53,
        54,
        55, // Graphics blocks
};

byte palette[] =
    {
        // RGB
        0,  // Black
        7,  // Dark Gray
        56, // Light Gray
        63, // White
        28, // Teal
        1,  // Dark blue
        9,  // Sea blue
        11, // Light blue
        25, // Foam
        27, // Light foam
        4,  // Dark red
        36, // Red
        38, // Red Orange
        52, // Orange
        54, // Yellow
        63, // WHITE FOR NOW -  2,  // Dark green

        // Composite
        0,  // Black
        16, // Dark Gray
        32, // Light Gray
        48, // White
        30, // Teal
        13, // Dark blue
        12, // Sea blue
        28, // Light blue
        44, // Foam
        62, // Light foam
        7,  // Dark red
        23, // Red
        22, // Red Orange
        21, // Orange
        36, // Yellow
        48  // WHITE FOR NOW - 15  // Dark green
};

byte paletteBackup[16];
#endif

#define OFFSET_Y 2

#ifdef COCO3
#define CHAR_SIZE 32
#define ROP_CPY 0xffff
#define ROP_ALT 0x8888
#define ROP_LINE 0xCC
#define ROP_BLUE 0x7777
#define ROP_ACTIVE 0xffff
#define ROP_INACTIVE 0x7777
#define BAK_ACTIVE 0xcccc
#define BAK_INACTIVE 0x4444
#define ROP_YELLOW 0xCCCC
#define CHAR_OUTSIDE_EDGE_LEFT 64
#define CHAR_OUTSIDE_EDGE_RIGHT 63
#define CHAR_INSIDE_EDGE_LEFT 96
#define CHAR_INSIDE_EDGE_RIGHT 97
#define CHAR_DRAWER_CROSS_SECTION_LEFT_UPPER 2
#define CHAR_DRAWER_CROSS_SECTION_LEFT_LOWER 4
#define CHAR_DRAWER_CROSS_SECTION_RIGHT_UPPER 34
#define CHAR_DRAWER_CROSS_SECTION_RIGHT_LOWER 36
#define CHAR_FAR_EDGE_UPPER_CORNER_LEFT 15
#define CHAR_FAR_EDGE_UPPER_CORNER_RIGHT 17
#define CHAR_FAR_EDGE_UPPER 16
#define CHAR_FAR_EDGE_LOWER_CORNER_LEFT 12
#define CHAR_FAR_EDGE_LOWER_CORNER_RIGHT 14
#define CHAR_FAR_EDGE_LOWER 13
#define CHAR_DRAWER_CORNER_RIGHT 35
#define CHAR_DRAWER_CORNER_LEFT 1
#define CHAR_DRAWER_EDGE_TOP 40
#define CHAR_DRAWER_EDGE_BOTTOM 41
#define CHAR_DRAWER_EDGE_LEFT 5
#define CHAR_DRAWER_EDGE_RIGHT 38
#define CHAR_BULLET 91
#define FIELDX_1V1 7
#else
#define CHAR_SIZE 8
#define ROP_CPY 0xff
#define ROP_ALT 0b10101010
#define ROP_LINE 0b10101010
#define ROP_BLUE 0b10101010
#define ROP_YELLOW 0b01010101
#define BAK_ACTIVE 0b01010101
#define BAK_INACTIVE 0b01010101
#define CHAR_OUTSIDE_EDGE_LEFT 0x23
#define CHAR_OUTSIDE_EDGE_RIGHT 0x22
#define CHAR_INSIDE_EDGE_LEFT 0x01
#define CHAR_INSIDE_EDGE_RIGHT 0x04
#define CHAR_DRAWER_CROSS_SECTION_LEFT_UPPER 0x24
#define CHAR_DRAWER_CROSS_SECTION_LEFT_LOWER CHAR_DRAWER_CROSS_SECTION_LEFT_UPPER
#define CHAR_DRAWER_CROSS_SECTION_RIGHT_UPPER 0x25
#define CHAR_DRAWER_CROSS_SECTION_RIGHT_LOWER CHAR_DRAWER_CROSS_SECTION_RIGHT_UPPER
#define CHAR_FAR_EDGE_UPPER_CORNER_LEFT 0x02
#define CHAR_FAR_EDGE_UPPER_CORNER_RIGHT 0x03
#define CHAR_FAR_EDGE_UPPER 0x29
#define CHAR_FAR_EDGE_LOWER_CORNER_LEFT CHAR_FAR_EDGE_UPPER_CORNER_LEFT
#define CHAR_FAR_EDGE_LOWER_CORNER_RIGHT CHAR_FAR_EDGE_UPPER_CORNER_RIGHT
#define CHAR_FAR_EDGE_LOWER CHAR_FAR_EDGE_UPPER
#define CHAR_DRAWER_CORNER_RIGHT 0xD
#define CHAR_DRAWER_CORNER_LEFT 0xC
#define CHAR_DRAWER_EDGE_TOP 0x11
#define CHAR_DRAWER_EDGE_BOTTOM CHAR_DRAWER_EDGE_TOP
#define CHAR_DRAWER_EDGE_LEFT 0x10
#define CHAR_DRAWER_EDGE_RIGHT CHAR_DRAWER_EDGE_LEFT
#define CHAR_BULLET 0x05
#define FIELDX_1V1 5
#endif

#define BOX_SIDE 0b111100

#define COLOR_MODE_COCO3_RGB 1
#define COLOR_MODE_COCO3_COMPOSITE 2

#define LEGEND_X 24

// Defined in this file
void drawTextAltAt(uint8_t x, uint8_t y, const char *s);
void drawTextAt(uint8_t x, uint8_t y, const char *s);
void drawShipInternal(uint8_t x, uint8_t y, uint8_t size, uint8_t delta);

extern char lastKey;
extern ROP_TYPE background;
static uint8_t fieldX = 0;
uint8_t box_color = 0xff;

#ifdef COCO3
/**
 * @brief Top left of each playfield quadrant
 */
static unsigned char quadrant_offset_xy[4][2] =
    {
        {8, 13 * 8 + 2}, // bottom left
        {8, 1 * 8 + 2},  // Top left
        {21, 1 * 8 + 2}, // top right
        {21, 13 * 8 + 2} // bottom right
};
#else
/**
 * @brief Top left of each playfield quadrant
 */
static unsigned char quadrant_offset_xy[4][2] =
    {
        {5, 12 * 8 + 2}, // bottom left
        {5, 1 * 8 + 2},  // Top left
        {17, 1 * 8 + 2}, // top right
        {17, 12 * 8 + 2} // bottom right
};
#endif

/**
 * @brief offset of ships within legend/drawer
 */
uint8_t legendShipOffset[5][2] =
    {
        {2, 7},
        {1, 7},
        {0, 7},
        {0, 6 * 8 - 1},
        {1, 7 * 8 - 1},
};

/* Screen memory offset from the top/left of the tray of where to draw each ship:

0-4 below represent the starting offset of each ship, with # indicating the rest of the ship characters

       0 1 2 X offset
     . . . . .
   8 . 2 1 0 .
  16 . # # # .
  24 . # # # .
  32 .   # # .
   Y .     # .
     . 3     .
     . # 4   .
     . # #   .
     . . . . .
*/


#ifdef COCO3
void updateColors()
{
    memcpy((void *)0xFFB0, &palette + 16 * (prefs.colorMode - 1), 16);
}

uint8_t cycleNextColor()
{
    if (!prefs.colorMode)
        return 0; // Coco3 is not enabled

    ++prefs.colorMode;
    if (prefs.colorMode > 2)
        prefs.colorMode = 1;

    updateColors();
    return prefs.colorMode;
}

void rgbOrComposite()
{
    while (!prefs.colorMode)
    {
        drawTextAltAt(10, 96, "r-RGB or c-COMPOSITE");
        switch (cgetc())
        {
        case 'R':
        case 'r':
            prefs.colorMode = COLOR_MODE_COCO3_RGB;
            break;
        case 'C':
        case 'c':
            prefs.colorMode = COLOR_MODE_COCO3_COMPOSITE;
            break;
        }
    }

    updateColors();
}

uint16_t oldGime;
#else
uint8_t cycleNextColor() {
    // Not implemented on CoCo 2
    return 0;
}
#endif

void initGraphics()
{
    uint16_t i;
    initCoCoSupport();

#ifdef COCO3

    // Fix endianness of charset - this COULD be done with an external tool
    for (i = 0; i < 32U * 107; i++)
    {
        charset[i] = ((charset[i] >> 4) & 0x0F) | ((charset[i] << 4) & 0xF0);
    }

    disableInterrupts();

    // Set up Task #1 memory block configuration
    memcpy(0xFFA8, task1MMUBlocks, sizeof(task1MMUBlocks));

    memcpy(paletteBackup, (void *)0xFFB0, 16); // Backup balette
    memcpy((void *)0xFFB0, &palette, 16);      // assumes RGB monitor

    asm { sync } // wait for v-sync to change graphics mode

    // Allow border color by switching to CoCo 3 graphics mode verison of PMODE 3:
    *(byte *)0xFF90 = 0x4C; // reset CoCo 2 compatible bit
    *(byte *)0xFF98 = 0x80; // graphics mode

    // GIME graphics mode register bits
    // .XX..... : Scan Lines : 0=192, 1=200, 3=225
    // ...XXX.. : Bytes/row  : 0=16, 1=20, 2=32, 3=40, 4=64, 5=80, 6=128, 7=160
    // ......XX : Pixels/byte: 0=8 (2 color), 1=4 (4 color), 2=2 (16 colors)

    *(byte *)0xFF99 = 0b00111110; // 320x200x16 colors - 40x25 characters

    *(byte *)0xFF9A = 0; // make border black

    // Tell GIME the location of the screen, which is mapped to 52 by MMU.
    oldGime = *(uint16_t *)0xFF9D;
    *(uint16_t *)0xFF9D = 0xD000; // 52 << 10;

    // Our first graphics command - this resets the screen
    resetScreen();

    rgbOrComposite();

#else
    pmode(3, SCREEN);
    pcls(0);
    screen(1, 0);
#endif

    //
}

bool saveScreenBuffer()
{
    // No room on CoCo 32K for second page
    return false;
}

void restoreScreenBuffer()
{
    // No-op on CoCo
}

void drawEndgameMessage(const char *message)
{
    uint8_t i, x;
    i = (uint8_t)strlen(message);
    x = (WIDTH - i) / 2;

    hires_Mask(0, HEIGHT * 8 - 10, WIDTH, 1, ROP_BLUE);
    hires_Mask(0, HEIGHT * 8 - 9, WIDTH, 9, ROP_YELLOW);

    background = ROP_YELLOW;
    drawTextAt(x, HEIGHT * 8 - 9, message);
    background = 0;
}

void drawPlayerName(uint8_t i, const char *name, bool active)
{
    uint8_t ix, ox, left = 1, fy, eh, drawEdge, drawEdgeChar, drawX, drawCorner, edgeSkip;
    uint8_t x = quadrant_offset_xy[i][0] + fieldX;
    uint8_t y = quadrant_offset_xy[i][1];
#ifdef COCO3
    uint16_t rop_active = active ? ROP_ACTIVE : ROP_INACTIVE;
#else
    uint8_t rop_active = ROP_CPY;
#endif
    x = quadrant_offset_xy[i][0] + fieldX;
    y = quadrant_offset_xy[i][1];

    // right and left drawers
    if (i > 1 || playerCount == 2 && i > 0)
    {
        // Right ship drawer
        ox = x - 1;
        ix = x + 10;
        left = 0;
        drawX = ix + 1;
        drawEdge = drawX + 3;
        drawCorner = CHAR_DRAWER_CORNER_RIGHT;
        drawEdgeChar = CHAR_DRAWER_EDGE_RIGHT;
    }
    else
    {
        // Left ship drawer
        ix = x - 1;
        ox = x + 10;
        drawX = ix - 3;
        drawEdge = drawX - 1;
        drawCorner = CHAR_DRAWER_CORNER_LEFT;
        drawEdgeChar = CHAR_DRAWER_EDGE_LEFT;
    }

    // Player field placements
    //
    // 1 | 2
    // --+--
    // 0 | 3

    if (i == 1 || i == 2)
    {
        // Upper name badges

        // Name badge corners
        hires_putc(x - 1, y - 8, rop_active, 0x5C);
        hires_putc(x + 10, y - 8, rop_active, 0x5D);

        // Name badge

#ifdef COCO3

        // Top Border
        hires_Mask(x, y - 9, 10, 1, 0x33); // White strip

        // Left & right corners pixels
        hires_Draw(x - 1, y - 9, 1, 1, rop_active, &charset[(uint16_t)106 CHAR_SHIFT]);
        hires_Draw(x + 10, y - 9, 1, 1, rop_active, &charset[(uint16_t)105 CHAR_SHIFT]);

#else

        // Border
        hires_Mask(x, y - 10, 10, 1, ROP_BLUE);
        hires_Mask(x - 1, y - 10, 1, 1, 0b00000010);
        hires_Mask(x + 10, y - 10, 1, 1, 0b10000000);
        hires_Mask(x - 1, y - 9, 1, 1, 0b001001);
        hires_Mask(x + 10, y - 9, 1, 1, 0b01100000);
#endif
        fy = y + 80;
    }
    else
    {
        // Lower name badges

        // Bottom Name badge corners
        hires_putc(x - 1, y + 80, rop_active, 0x5E);
        hires_putc(x + 10, y + 80, rop_active, 0x5F);

#ifdef COCO3
        // Name fill
        // hires_Mask(x, y + 80, 10, 8, active ? 0xCC : 0x44);

        // Bottom Border
        hires_Mask(x, y + 88, 10, 1, 0x33); // White strip

        // Left & right corner pixels
        hires_Draw(x - 1, y + 88, 1, 1, rop_active, &charset[(uint16_t)106 CHAR_SHIFT]);
        hires_Draw(x + 10, y + 88, 1, 1, rop_active, &charset[(uint16_t)105 CHAR_SHIFT]);
#else

        // Name fill
        // hires_Mask(x, y + 80, 10, 9, ROP_YELLOW);

        // Bottom Border
        hires_Mask(x - 1, y + 88, 1, 1, 0b001001);
        hires_Mask(x + 10, y + 88, 1, 1, 0b01100000);

        hires_Mask(x, y + 89, 10, 1, ROP_BLUE);
        hires_Mask(x - 1, y + 89, 1, 1, 0b00000010);
        hires_Mask(x + 10, y + 89, 1, 1, 0b10000000);
#endif

        fy = y - 8;
    }

    // Outside edge
    hires_Draw(ox, y, 1, 80, rop_active, &charset[(uint16_t)(left ? CHAR_OUTSIDE_EDGE_LEFT : CHAR_OUTSIDE_EDGE_RIGHT)CHAR_SHIFT]);

    // Inner edge (adjacent to ships drawer)
    hires_Draw(ix, y + 8, 1, 64, rop_active, &charset[(uint16_t)(left ? CHAR_INSIDE_EDGE_LEFT : CHAR_INSIDE_EDGE_RIGHT)CHAR_SHIFT]);

    // Inner edge + ship drawer
    hires_putc(ix, y, rop_active, left ? CHAR_DRAWER_CROSS_SECTION_LEFT_UPPER : CHAR_DRAWER_CROSS_SECTION_RIGHT_UPPER);
    hires_putc(ix, y + 72, rop_active, left ? CHAR_DRAWER_CROSS_SECTION_LEFT_LOWER : CHAR_DRAWER_CROSS_SECTION_RIGHT_LOWER);
    edgeSkip = 0;

#if COCO3

    // Far vertical edge
    if (1)
    {

        eh = 8;
#else
    if (playerCount == 1)
    {
        fy += 5;
        edgeSkip = 4;
    }

    if (i || edgeSkip)
    {
        if (i != 2 && !edgeSkip)
            eh = 8;
        else
            eh = 3;
#endif
        hires_Draw(x - 1, fy, 1, eh, rop_active, &charset[(uint16_t)(fy < y ? CHAR_FAR_EDGE_UPPER_CORNER_LEFT : CHAR_FAR_EDGE_LOWER_CORNER_LEFT) CHAR_SHIFT] + edgeSkip);
        hires_Draw(x + 10, fy, 1, eh, rop_active, &charset[(uint16_t)(fy < y ? CHAR_FAR_EDGE_UPPER_CORNER_RIGHT : CHAR_FAR_EDGE_LOWER_CORNER_RIGHT) CHAR_SHIFT] + edgeSkip);
        hires_Draw(x, fy, 10, eh, rop_active, &charset[(uint16_t)(fy < y ? CHAR_FAR_EDGE_UPPER : CHAR_FAR_EDGE_LOWER) CHAR_SHIFT] + edgeSkip);
    }

    // Ship drawer horizontal edges
    hires_Draw(drawX, y, 3, 8, ROP_CPY, &charset[(uint16_t)CHAR_DRAWER_EDGE_TOP CHAR_SHIFT]);
    hires_Draw(drawX, y + 72, 3, 8, ROP_CPY, &charset[(uint16_t)CHAR_DRAWER_EDGE_BOTTOM CHAR_SHIFT]);

    // Vertical edge
    hires_Draw(drawEdge, y + 8, 1, 64, ROP_CPY, &charset[(uint16_t)drawEdgeChar CHAR_SHIFT]);

    // Corners
    hires_putc(drawEdge, y, ROP_CPY, drawCorner);
    hires_putc(drawEdge, y + 72, ROP_CPY, drawCorner + 2);

// Player name
#if COCO3
    y -= 8;
#else
    y -= 9;
#endif
    if (i == 0 || i == 3)
    {

#if COCO3
        y += 88;
#else
        y += 89;
#endif
    }

    // Name fill
#ifdef COCO3
    hires_Mask(x + 1 + strlen(name), y, 9 - strlen(name), 8, active ? 0xCC : 0x44);
#else
    if (strlen(name) == 0)
        hires_Mask(x, y, 10, 9, ROP_YELLOW);
    else
        hires_Mask(x + 1 + strlen(name), y, 9 - strlen(name), 9, ROP_YELLOW);
#endif

    if (active)
    {
#ifndef COCO3
        background = BAK_ACTIVE;
#endif
        hires_putc(x, y, ROP_CPY, CHAR_BULLET);
        background = BAK_ACTIVE;
        drawTextAt(x + 1, y, name);
    }
    else
    {
#ifndef COCO3
        background = BAK_INACTIVE;
#endif
        hires_putc(x, y, ROP_CPY, 0x62);

#ifdef COCO3
        background = BAK_INACTIVE;
        drawTextAt(x + 1, y, name);
#else
        drawTextAltAt(x + 1, y, name);
#endif
    }
    background = 0;
}
void drawText(uint8_t x, uint8_t y, const char *s)
{
    y = y * 8 + OFFSET_Y;
    if (y > 184)
        y = 184;
    drawTextAt(x, y, s);
}
void drawTextAt(uint8_t x, uint8_t y, const char *s)
{
    char c;

    while ((c = *s++))
    {
        if (c >= 97 && c <= 122)
            c -= 32;
        hires_putc(x++, y, ROP_CPY, c);
    }
}
void drawTextAlt(uint8_t x, uint8_t y, const char *s)
{
    y = y * 8 + OFFSET_Y;

    if (y > (HEIGHT - 1) * 8)
        y = (HEIGHT - 1) * 8;

    drawTextAltAt(x, y, s);
}

void drawTextAltAt(uint8_t x, uint8_t y, const char *s)
{
    char c;
    ROP_TYPE rop;

    while ((c = *s++))
    {
        if (c < 65 || c > 90)
        {
            rop = ROP_ALT;
        }
        else
        {
            rop = ROP_CPY;
        }

        if (c >= 97 && c <= 122)
            c -= 32;
        hires_putc(x++, y, rop, c);
    }
}

void resetScreen()
{
    BEGIN_GFX
#ifdef COCO3
    memset16(SCREEN, 0, 16000U);
#else
    pcls(0);
#endif
    END_GFX
}

void drawLegendShip(uint8_t player, uint8_t index, uint8_t size, uint8_t status)
{
    uint8_t x = quadrant_offset_xy[player][0] + legendShipOffset[index][0] + fieldX;
    uint8_t y = quadrant_offset_xy[player][1] + legendShipOffset[index][1];

    if (player > 1 || (player > 0 && fieldX > 0))
    {
        x += 11;
    }
    else
    {
        x -= 4;
    }

    if (status)
    {
        drawShipInternal(x, y + 1, size, 1);
    }
    else
    {
        // Draw red splats
        hires_Draw(x, y + 1, 1, size * 8 - 1, ROP_CPY, &charset[(uint16_t)0x1c CHAR_SHIFT]);
    }
}

void drawGamefieldCursor(uint8_t quadrant, uint8_t x, uint8_t y, uint8_t *gamefield, uint8_t blink)
{
    uint8_t j, c = gamefield[y * 10 + x];

    if (blink)
    {
        c = c * 2 + 5 + blink;
    }
    else
    {
        c += 0x18;
    }
    hires_putc(quadrant_offset_xy[quadrant][0] + fieldX + x, quadrant_offset_xy[quadrant][1] + y * 8, ROP_CPY, c);
}

uint8_t *srcBlank = &charset[(uint16_t)0x18 CHAR_SHIFT];
uint8_t *srcHit = &charset[(uint16_t)0x19 CHAR_SHIFT];
uint8_t *srcMiss = &charset[(uint16_t)0x1A CHAR_SHIFT];
uint8_t *srcHit2 = &charset[(uint16_t)0x1B CHAR_SHIFT];
uint8_t *srcHitLegend = &charset[(uint16_t)0x1C CHAR_SHIFT];
uint8_t *srcAttackAnimStart = &charset[(uint16_t)0x63 CHAR_SHIFT];


void drawGamefieldUpdate(uint8_t quadrant, uint8_t *gamefield, uint8_t attackPos, uint8_t anim)
{
    uint8_t j, c = gamefield[attackPos];
    uint8_t *src;

    // Animate attack
    if (anim > 9)
    {
        src = srcAttackAnimStart + (anim - 10) * CHAR_SIZE;
    }
    else
    {

        if (c == FIELD_ATTACK)
        {
            src = anim ? srcHit2 : srcHit;
        }
        else if (c == FIELD_MISS)
        {
            src = srcMiss;
        }
        else
        {
            return;
        }
    }

    // Draw the updated cell
    hires_Draw(quadrant_offset_xy[quadrant][0] + fieldX + (attackPos % 10), quadrant_offset_xy[quadrant][1] + (attackPos / 10) * 8, 1, 8, ROP_CPY, src);
}

void drawGamefield(uint8_t quadrant, uint8_t *field)
{
    uint8_t y, x, j;

    for (y = 0; y < 10; ++y)
    {
        for (x = 0; x < 10; ++x)
        {
            if (*field)
            {
                hires_Draw(quadrant_offset_xy[quadrant][0] + fieldX + x, quadrant_offset_xy[quadrant][1] + y * 8, 1, 8, ROP_CPY, *field == 1 ? srcHit : srcMiss);
            }
            field++;
        }
    }
}

void drawShipInternal(uint8_t x, uint8_t y, uint8_t size, uint8_t delta)
{
    uint8_t i, c = 0x12;
    if (delta)
        c = 0x17;
    for (i = 0; i < size; i++)
    {
        hires_putc(x, y, ROP_CPY, c);

        if (delta)
        {
            y += 8;
            c = 0x16;
            if (i == size - 2)
                c = 0x15;
        }
        else
        {
            x++;
            c = 0x13;
            if (i == size - 2)
                c = 0x14;
        }
    }
}

void drawShip(uint8_t quadrant, uint8_t size, uint8_t pos, bool hide)
{
    uint8_t x, y, i, j, delta = 0;
    uint8_t *src;

    if (pos > 99)
    {
        delta = 1; // 1=vertical, 0=horizontal
        pos -= 100;
    }

    x = (pos % 10) + fieldX + quadrant_offset_xy[quadrant][0];
    y = ((pos / 10) + quadrant_offset_xy[quadrant][1] / 8) * 8 + OFFSET_Y;

    if (hide)
    {
        if (!delta)
            hires_Mask(x, y, size, 8, ROP_BLUE);
        else
            hires_Mask(x, y, 1, size * 8, ROP_BLUE);
        return;
    }

    // uint8_t *dest = (uint8_t *)SCREEN + (uint16_t)y * WIDTH + x;
    drawShipInternal(x, y, size, delta);
}

void drawIcon(uint8_t x, uint8_t y, uint8_t icon)
{
    hires_putc(x, y * 8 + OFFSET_Y, ROP_CPY, icon);
}

void drawClock()
{
    hires_putc(WIDTH - 1, HEIGHT * 8 - 8, ROP_CPY, 0x1D);
}

void drawConnectionIcon(bool show)
{
    hires_putcc(0, HEIGHT * 8 - 8, ROP_CPY, show ? 0x1e1f : 0x2020);
}

void drawSpace(uint8_t x, uint8_t y, uint8_t w)
{
    y = y * 8 + OFFSET_Y;
    if (y > 184)
        y = 184;
    hires_Mask(x, y, w, 8, 0);
}

void drawBoard(uint8_t currentPlayerCount)
{
    uint8_t i, x, y, ix, ox, left = 1, fy, eh, drawEdge, drawX, drawCorner, edgeSkip;

    // Center layout
    playerCount = currentPlayerCount;
    fieldX = playerCount > 2 ? 0 : FIELDX_1V1;

    for (i = 0; i < playerCount; i++)
    {
        x = quadrant_offset_xy[i][0] + fieldX;
        y = quadrant_offset_xy[i][1];

        // right and left drawers
        if (i > 1 || playerCount == 2 && i > 0)
        {
            drawX = x + 11;
        }
        else
        {
            drawX = x - 4;
        }

        // Draw player border
        drawPlayerName(i, "", false);

        // Blue gamefield
        hires_Mask(x, y, 10, 80, ROP_BLUE);

        // Fill in the drawer
        hires_Mask(drawX, y + 8, 3, 64, ROP_BLUE);
    }
}

void drawLine(uint8_t x, uint8_t y, uint8_t w)
{
    y = y * 8 + OFFSET_Y + 1;
    hires_Mask(x, y, w, 2, ROP_LINE);
}

void drawBox(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    y = y * 8 + 1 + OFFSET_Y;

    // Top Corners
    hires_putc(x, y, ROP_CPY, 0x3b);
    hires_putc(x + w + 1, y, ROP_CPY, 0x3c);

    y += 8 * (h - 1);
    // Bottom Corners
    hires_putc(x, y + 14, ROP_CPY, 0x3d);
    hires_putc(x + w + 1, y + 14, ROP_CPY, 0x3e);
}

void resetGraphics()
{

#ifdef COCO3
    memcpy((void *)0xFFB0, paletteBackup, 16); // assumes RGB monitor
    width(32);
#else
    screen(0, 0);
    cls(255);
#endif
}

void waitvsync()
{
    asm { sync }
}

void drawBlank(uint8_t x, uint8_t y)
{
    hires_putc(x, y * 8 + OFFSET_Y, ROP_CPY, 0x20);
}
