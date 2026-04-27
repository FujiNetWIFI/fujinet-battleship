/*
  Graphics functionality
*/
#include <stdlib.h>
#include <stdio.h>
#include <peekpoke.h>
#include <string.h>
#include "../platform-specific/graphics.h"
#include "../misc.h"
#include "hires.h"
#include "text.h"
#include "vars.h"
#include "graphicsInternals.h"
#include "graphicsBoard.h"
#include "graphicsFieldCursor.h"
#include "graphicsText.h"

extern unsigned char charset[];

unsigned char colorMode = 0;

uint8_t fieldX = 0;
uint16_t currentPlayerCount = 0;

// Drawer border font data (8 bytes × 8 types = 64 bytes)
// Each character is 7 pixels wide × 8 lines high
// Pattern definitions:
// - Top half black / bottom half blue: for horizontal borders
// - Right half black / left half blue: for vertical borders (left side)
// - Left half blue / right half black: for vertical borders (right side)
const uint8_t drawerBorderFont[8][8] = {
    // 0: Top half black / bottom half blue (EVEN)
    {0x00, 0x00, 0x00, 0x00, 0xd5, 0xd5, 0xd5, 0xd5},
    // 1: Top half black / bottom half blue (ODD)
    {0x00, 0x00, 0x00, 0x00, 0xaa, 0xAA, 0xAA, 0xAA},
    // 2: Top half blue / bottom half black (EVEN)
    {0xd5, 0xd5, 0xd5, 0xd5, 0x00, 0x00, 0x00, 0x00},
    // 3: Top half blue / bottom half black (ODD)
    {0xAA, 0xAA, 0xAA, 0xaa, 0x00, 0x00, 0x00, 0x00},
    // 4: Right half black / left half blue (EVEN)
    {0x8A, 0x8A, 0x8A, 0x8A, 0x8A, 0x8A, 0x8A, 0x8A},
    // 5: Right half black / left half blue (ODD)
    {0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0},
    // 6: Left half blue / right half black (EVEN)
    {0xD0, 0xD0, 0xD0, 0xD0, 0xD0, 0xD0, 0xD0, 0xD0},
    // 7: Left half blue / right half black (ODD)
    {0xA8, 0xA8, 0xA8, 0xA8, 0xA8, 0xA8, 0xA8, 0xA8}
};

// Green line font data for player name underline (1 byte per pattern)
const uint8_t greenLineFont[2] = {
    0x2a,  // EVEN: green pattern
    0x55   // ODD: green pattern
};
// Orange line font data for active player name underline (1 byte per pattern)
const uint8_t orangeLineFont[2] = {
    0xaa,  // EVEN: orange pattern
    0xd5   // ODD: orange pattern
};

uint16_t quadrantOffset[] = {
    98 * 40 + 5,     // = 3925  (Player 0: bottom-left, x=5, y=98)
    10 * 40 + 5,     // = 405   (Player 1: top-left, x=5, y=10)
    10 * 40 + 17,    // = 417   (Player 2: top-right, x=17, y=10)
    98 * 40 + 17     // = 3937  (Player 3: bottom-right, x=17, y=98)
};

uint16_t legendShipOffset[] = {2, 1, 0, 256U * 5, 256U * 6 + 1};

void resetGraphics() {
    ;
}

void initGraphics() {
    int type;

    hires_Init();
    hires_Clear();
    type = identify();
    if (type == -1) {
        // printf("error\n");
        // exit(1);
    }
    setVsyncProc(type);
}

bool saveScreenBuffer() {
    return false;
}

void restoreScreenBuffer() {
    ;
}

void drawEndgameMessage(const char *message) {
    uint8_t x;
    uint8_t len = (uint8_t)strlen(message);
    x = (WIDTH - len) / 2;

    // Clear a wider area to ensure "YOU" and other player names are removed
    // Clear from y=170 (approximately 4 lines above message) to y=192 (bottom of screen)
    // This covers the area where player names are drawn (y=178) and the message line (y=183)
    hires_Mask(0, 170, WIDTH, 22, HIRES_MASK_CLEAR_MAIN);

    drawTextAt(x, HEIGHT * 8 - 9, message);
}

unsigned char cycleNextColor() {
    ++colorMode;
    if (colorMode > 1) {
        colorMode = 0;
    }
    return colorMode;
}

void drawClock() {
    hires_putc(WIDTH - 1, (HEIGHT - 1) * 8, ROP_CPY, ICON_CLOCK);
}

void drawConnectionIcon(bool show) {
    hires_putc(0, (HEIGHT - 1) * 8, ROP_CPY, show ? ICON_CONNECTION_EVEN : ICON_BLANK);
    hires_putc(1, (HEIGHT - 1) * 8, ROP_CPY, show ? ICON_CONNECTION_ODD : ICON_BLANK);
}

void drawBlank(uint8_t x, uint8_t y) {
    hires_putc(x, y * 8 - 4, ROP_CPY, ICON_BLANK);
}

void drawSpace(uint8_t x, uint8_t y, uint8_t w) {
    uint8_t i;
    // If bottom line then y * 8, otherwise y * 8 - 4
    if (y == HEIGHT - 1) {
        y = y * 8;
    } else {
        y = y * 8 - 4;
    }
    for (i = 0; i < w; i++) {
        hires_putc(x + i, y, ROP_CPY, ICON_BLANK);
    }
}


void drawText(unsigned char x, unsigned char y, const char* s) {
    drawTextAt(x, y * 8 - 4, s);
}

// Call to clear the screen to an empty table
void resetScreen() {
    hires_Mask(1, 0, 38, 192, HIRES_MASK_CLEAR_MAIN);
}

void drawLine(unsigned char x, unsigned char y, unsigned char w) {
    hires_Mask(x, y * 8 - 3, w, 2, ROP_WHITE);
}

void drawBox(unsigned char x, unsigned char y, unsigned char w, unsigned char h) {
    int i;

    // Treat y=0 as y=1 to prevent out-of-bounds access
    if (y == 0) {
        y = 1;
    }
    y = y * 8 - 4;
    // Top Corners
    hires_putc(x, y, ROP_CPY, CHAR_BOX_TOP_LEFT);
    hires_putc(x + w + 1, y, ROP_CPY, CHAR_BOX_TOP_RIGHT);
    // Top/bottom lines
    for (i = x + w; i > x; --i) {
        hires_putc(i, y, ROP_CPY, CHAR_BOX_HORIZ);
        hires_putc(i, y + (h + 1) * 8, ROP_CPY, CHAR_BOX_HORIZ);
    }
    // Sides
    for (i = 0; i < h; ++i) {
        y += 8;
        hires_putc(x, y, ROP_CPY, CHAR_BOX_SIDE);
        hires_putc(x + w + 1, y, ROP_CPY, CHAR_BOX_SIDE);
    }
    y += 8;
    // Bottom Corners
    hires_putc(x, y, ROP_CPY, CHAR_BOX_BOTTOM_LEFT);
    hires_putc(x + w + 1, y, ROP_CPY, CHAR_BOX_BOTTOM_RIGHT);
}

void drawTextAlt(uint8_t x, uint8_t y, const char *s) {
    drawTextAltAt(x, y, s);
}

void drawIcon(uint8_t x, uint8_t y, uint8_t icon) {
    if (icon == ICON_PLAYER) {
        hires_putc(x, y * 8 - 4, ROP_CPY_NOFLIP, icon);
    } else {
        hires_putc(x, y * 8 - 4, ROP_CPY, icon);
    }
}

void drawShip(uint8_t quadrant, uint8_t size, uint8_t pos, bool hide) {
    uint8_t x, y, orientation = SHIP_ORIENT_HORIZONTAL;
    uint8_t gx;

    if (pos > 99) {
        orientation = SHIP_ORIENT_VERTICAL;
        pos -= 100;
    }

    x = (pos % 10) + fieldX + quadrantOffset[quadrant] % WIDTH;
    y = (pos / 10) * 8 + quadrantOffset[quadrant] / WIDTH + 1;

    if (hide) {
        if (orientation == SHIP_ORIENT_HORIZONTAL) {
            for (gx = 0; gx < size; gx++) {
                hires_Mask(x + gx, y, 1, 8, ROP_CONST(((x + gx) % 2) ? ODD_BLUE : EVEN_BLUE));
            }
        } else {
            hires_Mask(x, y, 1, size * 8, ROP_CONST((x % 2) ? ODD_BLUE : EVEN_BLUE));
        }
        return;
    }
    drawShipInternal(x, y, size, orientation);
}

void drawLegendShip(uint8_t player, uint8_t index, uint8_t size, uint8_t status) {
    uint8_t x, y;
    uint16_t pos;
    uint8_t drawerX;

    pos = fieldX + quadrantOffset[player];
    x = (uint8_t)(pos % WIDTH);
    y = (uint8_t)(pos / WIDTH);
    
    if (player > 1 || (currentPlayerCount == 2 && player > 0)) {
        // right drawer
        drawerX = x + 11; // ix + 1, where ix = x + 10
    } else {
        // left drawer
        drawerX = x - 4;  // ix - 3, where ix = x - 1
    }
    
    if (index < 3) {
        // First 3 ships: arranged from top to bottom
        x = drawerX + legendShipOffset[index]; // Fine adjustment in x direction
        y = y + 8; // Starting y position in drawer (quadrant y + 8)
    } else {
        // Last 2 ships: placed lower
        x = drawerX + (legendShipOffset[index] % 256); // Fine adjustment in x direction
        y = y + (legendShipOffset[index] / 256) * 8 + 8; // y offset (256 = 1 line)
    }
    
    if (status) {
        drawShipInternal(x, y, size, SHIP_ORIENT_VERTICAL);
    } else {
        uint8_t i;
        for (i = 0; i < size; i++) {
            hires_Draw(x, y + (i * 8), 1, 8, ROP_CPY,
                       (char *)&charset[(uint16_t)((x % 2) ? HIT_LEGEND_ODD : HIT_LEGEND_EVEN) << 3]);
        }
    }
}

void drawShipInternal(unsigned char x, unsigned char y, uint8_t size, uint8_t orientation) {
    uint8_t i;
    uint8_t c;
    
    if (orientation == SHIP_ORIENT_HORIZONTAL) {
        for (i = 0; i < size; i++) {
            if (i == 0) {
                c = SHIP_HULL_STERN_HORIZ;
            } else if (i == size - 1) {
                c = SHIP_HULL_BOW_HORIZ;
            } else {
                c = SHIP_HULL_MID_HORIZ;
            }
            hires_putc(x + i, y, ROP_CPY, c);
        }
    } else {
        for (i = 0; i < size; i++) {
            if (i == 0) {
                c = SHIP_HULL_BOW_VERT;
            } else if (i == size - 1) {
                c = SHIP_HULL_STERN_VERT;
            } else {
                c = SHIP_HULL_MID_VERT;
            }
            hires_putc(x, y + (i * 8), ROP_CPY, c);
        }
    }
}

void drawPlayerName(uint8_t player, const char *name, bool active) {
    uint16_t pos;
    uint8_t x;
    uint8_t y;
    uint8_t gx;
    uint8_t lineY;

    pos = fieldX + quadrantOffset[player];
    x = (uint8_t)(pos % WIDTH + 1);
    y = (uint8_t)(pos / WIDTH - 9);
    if (player == 0 || player == 3) {
        y += 89;
    }
    lineY = y + 8;

    drawTextAt(x, y, name);
    if (active) {
        // Draw marker
        hires_putc(x - 1, y, ROP_CPY, ICON_ACTIVE_PLAYER);
        hires_putc(x + 8, y, ROP_CPY, ICON_ACTIVE_PLAYER_RIGHT);

        // Draw orange horizontal line at bottom of player name row (10 columns, 1 pixel thick)
        for (gx = 0; gx < 10; gx++) {
            uint8_t actualX = x - 1 + gx;      // Start from x - 1 to cover 10 columns including icon
            uint8_t fontIndex = (actualX % 2); // 0 for EVEN, 1 for ODD
            hires_Draw(actualX, lineY, 1, 1, ROP_CPY_NOFLIP, (char *)&orangeLineFont[fontIndex]);
        }
    } else {
        hires_putc(x - 1, y, ROP_CPY, ICON_BLANK);
        hires_putc(x + 8, y, ROP_CPY, ICON_BLANK);
        // Erase orange line by drawing green (10 columns, 1 pixel thick)
        for (gx = 0; gx < 10; gx++) {
            uint8_t actualX = x - 1 + gx;
            uint8_t fontIndex = (actualX % 2); // 0 for EVEN, 1 for ODD
            hires_Draw(actualX, lineY, 1, 1, ROP_CPY_NOFLIP, (char *)&greenLineFont[fontIndex]);
            // hires_Mask(actualX, lineY, 1, 1, ROP_BLACK);
        }
    }
    drawPlayerBorders(player, active);
}

void drawGamefield(uint8_t quadrant, uint8_t *field) {
    uint16_t pos;
    uint8_t baseX;
    uint8_t baseY;
    uint8_t i, x, y;
    uint8_t charCode;
    uint8_t actualX;

    pos = fieldX + quadrantOffset[quadrant];
    baseX = (uint8_t)(pos % WIDTH);  // WIDTH = 40
    baseY = (uint8_t)(pos / WIDTH);  // WIDTH = 40

    for (i = 0; i < 100; i++) {
        if (field[i]) {
            x = i % 10;
            y = i / 10;
            actualX = baseX + x;

            if (field[i] == FIELD_ATTACK) {
                // If actual x coordinate is even then EVEN, if odd then ODD
                charCode = (actualX % 2) ? HIT_NORMAL_ODD : HIT_NORMAL_EVEN;
            } else {
                charCode = (actualX % 2) ? MISS_NORMAL_ODD : MISS_NORMAL_EVEN;
            }
            hires_putc(actualX, baseY + y * 8, ROP_CPY, charCode);
        }
    }
}

void drawGamefieldUpdate(uint8_t quadrant, uint8_t *gamefield, uint8_t attackPos, uint8_t anim) {
    uint16_t pos;
    uint8_t baseX;
    uint8_t baseY;
    uint8_t x;
    uint8_t y;
    uint8_t c;
    uint8_t charCode;

    pos = fieldX + quadrantOffset[quadrant];
    baseX = (uint8_t)(pos % WIDTH);  // WIDTH = 40
    baseY = (uint8_t)(pos / WIDTH);  // WIDTH = 40
    x = attackPos % 10;
    y = attackPos / 10;
    c = gamefield[attackPos];

    // Animate attack
    if (anim > 9) {
        // Draw attack frame
        charCode = anim + ((baseX + x) % 2 ? (ATTACK_ANIM_START_ODD - 10) : (ATTACK_ANIM_START_EVEN - 10));
    } else if (c == FIELD_ATTACK) {
        // Determine if actual x coordinate is even or odd
        if (anim) {
            // Blink animation
            charCode = ((baseX + x) % 2) ? HIT2_ODD : HIT2_EVEN;
        } else {
            // Normal display
            charCode = ((baseX + x) % 2) ? HIT_NORMAL_ODD : HIT_NORMAL_EVEN;
        }
    } else if (c == FIELD_MISS) {
        charCode = ((baseX + x) % 2) ? MISS_NORMAL_ODD : MISS_NORMAL_EVEN;
    } else {
        return;
    }

    hires_putc(baseX + x, baseY + y * 8, ROP_CPY, charCode);
}

void drawGamefieldCursor(uint8_t quadrant, uint8_t x, uint8_t y, uint8_t *gamefield, uint8_t blink) {
    uint16_t pos;
    uint8_t baseX;
    uint8_t baseY;
    uint8_t c;
    uint8_t charCode;

    pos = fieldX + quadrantOffset[quadrant];
    baseX = (uint8_t)(pos % WIDTH);  // WIDTH = 40
    baseY = (uint8_t)(pos / WIDTH);  // WIDTH = 40
    c = gamefield[y * 10 + x];

    charCode = fieldCursorCode(baseX, x, c, blink);
    hires_putc(baseX + x, baseY + y * 8, ROP_CPY, charCode);
}

void drawBoard(uint8_t playerCount) {
    uint8_t i;
    fieldX = playerCount > 2 ? 4 : 10;
    currentPlayerCount = playerCount;

    for (i = 0; i < playerCount; i++) {
        drawBoardPlayer(i, playerCount);
    }
}
