#include <stdlib.h>
#include <stdint.h>
#include "vars.h"
#include "hires.h"
#include "graphicsBoard.h"

extern const uint8_t drawerBorderFont[8][8];
extern uint16_t quadrantOffset[];
extern uint8_t fieldX;
extern uint16_t currentPlayerCount;
extern const uint8_t greenLineFont[];
extern const uint8_t orangeLineFont[];

void drawBoardPlayer(uint8_t player, uint8_t playerCount) {
    uint8_t x, y, ix, ox, left = 1, fy, eh, edgeSkip;
    uint8_t drawX;  // x start position of drawer
    uint8_t gx;
    uint16_t pos;

    pos = fieldX + quadrantOffset[player];
    x = (uint8_t)(pos % WIDTH);
    y = (uint8_t)(pos / WIDTH);

    // right and left drawers
    if (player > 1 || playerCount == 2 && player > 0) {
        ox = x - 1;
        ix = x + 10;
        left = 0;
        drawX = ix + 1; 
    } else {
        ix = x - 1;
        ox = x + 10;
        drawX = ix - 3;
    }
    if (player == 1 || player == 2) {
        if (y - 9 < 0 || y - 9 >= 192) return;  // or skip
        if (y + 80 >= 192) return;  // or skip
        if (y + 82 >= 192) return;  // or skip
        fy = y + 80;
    } else {
        fy = y - 8;
    }
    // Blue gamefield
    for (gx=0; gx < 10; gx++) {
        hires_Mask(x+gx, y, 1, 80, ROP_OR(((x+gx) % 2) ? ODD_BLUE : EVEN_BLUE));
    }
    edgeSkip = 0;
    if (playerCount == 1) {
        fy += 5;
        edgeSkip = 4;
    }
    // Far edge
    if (player || edgeSkip) {
        if (player != 2 && !edgeSkip)
            eh = 8;
        else
            eh = 3;
    }
    // Fill in the drawer
    for (gx=0; gx < 3; gx++) {
        hires_Mask(drawX+gx, y+8, 1, 64, ROP_OR(((drawX+gx) % 2) ? ODD_BLUE : EVEN_BLUE));
    }
    // Draw drawer borders
    // Top border (3 characters wide, starting at y, above drawer)
    drawDrawerBorder(drawX, y, 0, 3);
    // Bottom border (3 characters wide, at y+8+64)
    drawDrawerBorder(drawX, y + 8 + 64, 1, 3);
    
    // Vertical borders (9 lines high to cover y+4 to y+4+64)
    if (player > 1 || playerCount == 2 && player > 0) {
        // Right drawer: right border (outer side, away from game field)
        for (gx = 0; gx < 9; gx++) { 
            drawDrawerBorder(drawX + 3, y + 4 + gx * 8, 2, 1);
        }
    } else {
        // Left drawer: left border (outer side, away from game field)
        for (gx = 0; gx < 9; gx++) {
            drawDrawerBorder(drawX - 1, y + 4 + gx * 8, 3, 1);
        }
    }
    // Draw white outline outside drawer borders
    if (player > 1 || playerCount == 2 && player > 0) {
        // Right drawer
        // Top white line (1 pixel thick, above drawer top border, 4 columns wide)
        hires_Mask(drawX, y + 3, 4, 1, ROP_WHITE);
        
        // Bottom white line (1 pixel thick, below drawer bottom border, 4 columns wide)
        hires_Mask(drawX, y + 8 + 68, 4, 1, ROP_WHITE);
        
        // Right side white line (leftmost bit of column = screen right)
        // Use ROP_OR to preserve drawer border drawing
        for (gx = 0; gx < 9; gx++) {
            hires_Mask(drawX + 3, y + 4 + gx * 8, 1, 8, ROP_OR(V_LINE_RIGHT));
        }
    } else {
        // Left drawer
        hires_Mask(drawX - 1, y + 3, 4, 1, ROP_WHITE);
        
        // Bottom white line (1 pixel thick, below drawer bottom border, 4 columns wide)
        hires_Mask(drawX - 1, y + 8 + 68, 4, 1, ROP_WHITE);
        // Left side white line (rightmost bit of column = screen left)
        for (gx = 0; gx < 9; gx++) {
            hires_Mask(drawX - 1, y + 4 + gx * 8, 1, 8, ROP_OR(V_LINE_LEFT));
        }
    }
    
    // Draw white lines inside game field
    if (player == 1 || player == 2) {
        // Top players: draw at bottom of game field
        hires_Mask(x, y + 80, 10, 1, ROP_WHITE);
    } else {
        // Bottom players: draw at top of game field
        hires_Mask(x, y - 1, 10, 1, ROP_WHITE);
    }
    
    // Vertical line (opposite side of drawer)
    if (player > 1 || playerCount == 2 && player > 0) {
        // Right players: draw at left edge of game field
        if (player == 1 || player == 2) {
            // Top players: extend line 1 line down at the end
            for (gx = 0; gx < 10; gx++) {
                hires_Mask(x - 1, y + gx * 8, 1, 8, ROP_OR(V_LINE_RIGHT));
            }
            hires_Mask(x - 1, y + 80, 1, 1, ROP_OR(V_LINE_RIGHT));
        } else {
            // Bottom players: start 1 line up
            hires_Mask(x - 1, y - 1, 1, 1, ROP_OR(V_LINE_RIGHT));
            for (gx = 0; gx < 10; gx++) {
                hires_Mask(x - 1, y + gx * 8, 1, 8, ROP_OR(V_LINE_RIGHT));
            }
        }
    } else {
        // Left players: draw at right edge of game field
        if (player == 1 || player == 2) {
            // Top players: extend line 1 line down at the end
            for (gx = 0; gx < 10; gx++) {
                hires_Mask(x + 10, y + gx * 8, 1, 8, ROP_OR(V_LINE_LEFT));
            }
            // Add 1 line at the bottom
            hires_Mask(x + 10, y + 80, 1, 1, ROP_OR(V_LINE_LEFT));
        } else {
            // Bottom players: start 1 line up
            hires_Mask(x + 10, y - 1, 1, 1, ROP_OR(V_LINE_LEFT));
            for (gx = 0; gx < 10; gx++) {
                hires_Mask(x + 10, y + gx * 8, 1, 8, ROP_OR(V_LINE_LEFT));
            }
        }
    }
}

void drawDrawerBorder(uint8_t x, uint8_t y, uint8_t type, uint8_t length) {
    uint8_t fontIndex;
    uint8_t i;
    
    for (i = 0; i < length; i++) {
        uint8_t actualX = x + i;
        // Select font based on actual x coordinate (ODD/EVEN) and type
        if (type < 2) {
            // Horizontal borders (top/bottom)
            fontIndex = (actualX % 2) ? (type * 2 + 1) : (type * 2);
        } else {
            if (type == 2) {
                fontIndex = 4;
            } else {
                // Vertical borders (left/right)
                fontIndex = 6;
            }
        }
        hires_Draw(actualX, y, 1, 8, ROP_CPY_NOFLIP, (char*)&drawerBorderFont[fontIndex][0]);
    }
}

// Helper function to draw horizontal player borders (drawer and game field)
void drawPlayerBorders(uint8_t player, bool active) {
    uint8_t x, y, drawX, gx;
    uint16_t pos;
    uint8_t playerCount = currentPlayerCount;
    
    // Color patterns for borders
    const uint8_t *horizPattern = active ? orangeLineFont : greenLineFont;
    
    pos = fieldX + quadrantOffset[player];
    x = (uint8_t)(pos % WIDTH);
    y = (uint8_t)(pos / WIDTH);
    
    // Determine drawer position (same as drawBoard)
    if (player > 1 || playerCount == 2 && player > 0) {
        drawX = x + 11;  // Right drawer: ix + 1, where ix = x + 10
    } else {
        drawX = x - 4;   // Left drawer: ix - 3, where ix = x - 1
    }
    
    // Draw drawer horizontal borders (outside white lines)
    if (player > 1 || playerCount == 2 && player > 0) {
        // Right drawer
        // Top horizontal line (1 pixel thick, above drawer top border, 4 columns wide)
        for (gx = 0; gx < 4; gx++) {
            uint8_t actualX = drawX + gx;
            uint8_t fontIndex = (actualX % 2);
            hires_Draw(actualX, y + 3, 1, 1, ROP_CPY_NOFLIP, (char*)&horizPattern[fontIndex]);
        }
        // Bottom horizontal line (1 pixel thick, below drawer bottom border, 4 columns wide)
        for (gx = 0; gx < 4; gx++) {
            uint8_t actualX = drawX + gx;
            uint8_t fontIndex = (actualX % 2);
            hires_Draw(actualX, y + 8 + 68, 1, 1, ROP_CPY_NOFLIP, (char*)&horizPattern[fontIndex]);
        }
    } else {
        // Left drawer
        for (gx = 0; gx < 4; gx++) {
            uint8_t actualX = drawX - 1 + gx;
            uint8_t fontIndex = (actualX % 2);
            hires_Draw(actualX, y + 3, 1, 1, ROP_CPY_NOFLIP, (char*)&horizPattern[fontIndex]);
        }
        // Bottom horizontal line (1 pixel thick, below drawer bottom border, 4 columns wide)
        for (gx = 0; gx < 4; gx++) {
            uint8_t actualX = drawX - 1 + gx;
            uint8_t fontIndex = (actualX % 2);
            hires_Draw(actualX, y + 8 + 68, 1, 1, ROP_CPY_NOFLIP, (char*)&horizPattern[fontIndex]);
        }
    }
    // Draw game field horizontal border (opposite side of player name)
    if (player == 1 || player == 2) {
        // Top players: draw at bottom of game field
        for (gx = 0; gx < 10; gx++) {
            uint8_t actualX = x + gx;
            uint8_t fontIndex = (actualX % 2);
            hires_Draw(actualX, y + 80, 1, 1, ROP_CPY_NOFLIP, (char*)&horizPattern[fontIndex]);
        }
    } else {
        // Bottom players: draw at top of game field
        for (gx = 0; gx < 10; gx++) {
            uint8_t actualX = x + gx;
            uint8_t fontIndex = (actualX % 2);
            hires_Draw(actualX, y - 1, 1, 1, ROP_CPY_NOFLIP, (char*)&horizPattern[fontIndex]);
        }
    }
}