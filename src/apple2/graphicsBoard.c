#include <stdlib.h>
#include <stdint.h>
#include "vars.h"
#include "hires.h"
#include "graphicsBoard.h"

extern const uint8_t drawerBorderFont[8][8];
extern uint16_t quadrantOffset[];

/* index of drawerBorderFont */
#define DRAWER_FONT_TOP           0
#define DRAWER_FONT_BOTTOM        2
#define DRAWER_FONT_VERT_OUT_R    4
#define DRAWER_FONT_VERT_OUT_L    6
static void blitDrawerBorderTile(uint8_t x, uint8_t y, uint8_t fontBaseEven)
{
    uint8_t idx = fontBaseEven + (x & 1);
    hires_Draw(x, y, 1, FIELD_CELL_PX, ROP_CPY_NOFLIP,
               (char *)&drawerBorderFont[idx][0]);
}

static void drawDrawerBorderTop(uint8_t x, uint8_t y, uint8_t length) {
    uint8_t i;
    for (i = 0; i < length; i++) {
        blitDrawerBorderTile(x + i, y, DRAWER_FONT_TOP);
    }
}

static void drawDrawerBorderBottom(uint8_t x, uint8_t y, uint8_t length) {
    uint8_t i;

    for (i = 0; i < length; i++) {
        blitDrawerBorderTile(x + i, y, DRAWER_FONT_BOTTOM);
    }
}

static void drawDrawerOuterVertSegment(uint8_t x, uint8_t y,
                                       uint8_t fontIdx, uint8_t vLineMask) {
    hires_Draw(x, y, 1, FIELD_CELL_PX, ROP_CPY_NOFLIP,
                                            (char *)&drawerBorderFont[fontIdx][0]);
    hires_Mask(x, y, 1, FIELD_CELL_PX, ROP_OR(vLineMask));
}

static void drawDrawer(uint8_t drawX, uint8_t y, uint8_t rightDrawer) {
    uint8_t gx;
    uint8_t outerCol;
    uint8_t fontBase;
    uint8_t vMask;

    for (gx = 0; gx < DRAWER_COLS; gx++) {
        hires_Mask(drawX + gx, y + DRAWER_BODY_Y, 1, DRAWER_BODY_HEIGHT,
                   ROP_OR(((drawX + gx) % 2) ? ODD_BLUE : EVEN_BLUE));
    }
    drawDrawerBorderTop(drawX, y, DRAWER_COLS);
    drawDrawerBorderBottom(drawX, y + DRAWER_BOTTOM_Y, DRAWER_COLS);
    if (rightDrawer) {
        outerCol = drawX + 3;
        fontBase = DRAWER_FONT_VERT_OUT_R;
        vMask = V_LINE_RIGHT;
    } else {
        outerCol = drawX - 1;
        fontBase = DRAWER_FONT_VERT_OUT_L;
        vMask = V_LINE_LEFT;
    }
    for (gx = 0; gx < DRAWER_OUTER_VERT_SEGS; gx++) {
        uint8_t rowY = y + DRAWER_OUTER_VERT_Y + gx * FIELD_CELL_PX;
        drawDrawerOuterVertSegment(outerCol, rowY, fontBase, vMask);
    }
}


static void drawSeamCol(uint8_t ix, uint8_t y, uint8_t leftDrawer, uint8_t topPlayer) {
    uint8_t playerSideMask = leftDrawer ? V_LINE_RIGHT : V_LINE_SEAM_LEFT;
    uint8_t blueFill = (ix % 2) ? ODD_BLUE : EVEN_BLUE;

    hires_Mask(ix, y, 1, DRAWER_IND_TOP_Y, ROP_CONST(playerSideMask));
    hires_Mask(ix, y + 4, 1, 4, ROP_CONST(blueFill));
    hires_Mask(ix, y + 4, 1, 4, ROP_OR(playerSideMask));

    /* Drawer body zone: player side white, drawer side blue. */
    hires_Mask(ix, y + DRAWER_BODY_Y, 1, DRAWER_BODY_HEIGHT, ROP_CONST(blueFill));
    hires_Mask(ix, y + DRAWER_BODY_Y, 1, DRAWER_BODY_HEIGHT, ROP_OR(playerSideMask));

    hires_Mask(ix, y + 72, 1, 4, ROP_CONST(blueFill));
    hires_Mask(ix, y + 72, 1, 4, ROP_OR(playerSideMask));
    hires_Mask(ix, y + DRAWER_IND_BOT_Y + 1, 1, 3, ROP_CONST(playerSideMask));

    if (topPlayer) {
        if (!leftDrawer) {
            hires_Mask(ix, y - FIELD_BORDER_TOP_OFFSET, 1, 1,
                       ROP_CONST(V_LINE_SEAM_LEFT));
        } else {
            hires_Mask(ix, y - FIELD_BORDER_TOP_OFFSET, 1, 1,
                       ROP_OR(playerSideMask));
        }
        if (!leftDrawer) {
            hires_Mask(ix, y + FIELD_BORDER_BOTTOM_Y, 1, 1,
                       ROP_CONST(V_LINE_SEAM_LEFT));
        } else {
            hires_Mask(ix, y + FIELD_BORDER_BOTTOM_Y, 1, 1,
                       ROP_OR(playerSideMask));
        }
    } else {
        if (!leftDrawer) {
            hires_Mask(ix, y - FIELD_BORDER_TOP_OFFSET, 1, 1,
                       ROP_CONST(V_LINE_SEAM_LEFT));
        } else {
            hires_Mask(ix, y - FIELD_BORDER_TOP_OFFSET, 1, 1,
                       ROP_OR(playerSideMask));
        }
    }
}

void drawBoardPlayer(uint8_t player, uint8_t playerCount) {
    int     y;
    uint8_t x, ix;
    uint8_t drawX;  // x start position of drawer
    uint8_t gx;
    uint16_t pos;
    uint8_t topPlayer = (uint8_t)(player == 1 || player == 2);
    uint8_t rightDrawer = (uint8_t)(player > 1 || (playerCount == 2 && player > 0));
    uint8_t leftDrawer = !rightDrawer;
    uint8_t fieldX = (uint8_t)(playerCount > 2 ? 4 : 10);

    pos = fieldX + quadrantOffset[player];
    x = (uint8_t)(pos % WIDTH);
    y = (uint8_t)(pos / WIDTH);
    
    if (topPlayer) {
        if (y - 9 < 0 || y - 9 >= 192) return;
        if (y + FIELD_BORDER_BOTTOM_Y >= 192) return;
        if (y + 82 >= 192) return;
    }

    // Blue gamefield
    for (gx=0; gx < FIELD_COLS; gx++) {
        hires_Mask(x+gx, y, 1, FIELD_HEIGHT_PX, ROP_OR(((x+gx) % 2) ? ODD_BLUE : EVEN_BLUE));
    }
    
    // Draw white lines inside game field
    if (topPlayer) {
        hires_Mask(x, y + FIELD_BORDER_BOTTOM_Y, FIELD_COLS, 1, ROP_WHITE);
    } else {
        // Bottom players: draw at top of game field
        hires_Mask(x, y - FIELD_BORDER_TOP_OFFSET, FIELD_COLS, 1, ROP_WHITE);
    }
    
    // Vertical line (opposite side of drawer)
    if (rightDrawer) {
        if (topPlayer) {
            hires_Mask(x - 1, y - FIELD_BORDER_TOP_OFFSET, 1, 1, ROP_OR(V_LINE_RIGHT));
            for (gx = 0; gx < FIELD_COLS; gx++) {
                hires_Mask(x - 1, y + gx * FIELD_CELL_PX, 1, FIELD_CELL_PX, ROP_OR(V_LINE_RIGHT));
            }
            hires_Mask(x - 1, y + FIELD_BORDER_BOTTOM_Y, 1, 1, ROP_OR(V_LINE_RIGHT));
        } else {
            hires_Mask(x - 1, y - FIELD_BORDER_TOP_OFFSET, 1, 1, ROP_OR(V_LINE_RIGHT));
            for (gx = 0; gx < FIELD_COLS; gx++) {
                hires_Mask(x - 1, y + gx * FIELD_CELL_PX, 1, FIELD_CELL_PX, ROP_OR(V_LINE_RIGHT));
            }
        }
    } else {
        if (topPlayer) {
            hires_Mask(x + FIELD_COLS, y - FIELD_BORDER_TOP_OFFSET, 1, 1,ROP_OR(V_LINE_LEFT));
            for (gx = 0; gx < FIELD_COLS; gx++) {
                hires_Mask(x + FIELD_COLS, y + gx * FIELD_CELL_PX, 1, FIELD_CELL_PX, ROP_OR(V_LINE_LEFT));
            }
            hires_Mask(x + FIELD_COLS, y + FIELD_BORDER_BOTTOM_Y, 1, 1, ROP_OR(V_LINE_LEFT));
        } else {
            hires_Mask(x + FIELD_COLS, y - FIELD_BORDER_TOP_OFFSET, 1, 1, ROP_OR(V_LINE_LEFT));
            for (gx = 0; gx < FIELD_COLS; gx++) {
                hires_Mask(x + FIELD_COLS, y + gx * FIELD_CELL_PX, 1, FIELD_CELL_PX, ROP_OR(V_LINE_LEFT));
            }
        }
    }

    // right and left drawers
    if (rightDrawer) {
        ix = x + FIELD_COLS;
        drawX = ix + 1; 
    } else {
        ix = x - 1;
        drawX = ix - DRAWER_COLS;
    }
    drawSeamCol(ix, y, leftDrawer, topPlayer);
    drawDrawer(drawX, y, rightDrawer);
    /* Restore indicator white bands after top/bottom border blit. */
    hires_Mask(drawX - 1, y + DRAWER_IND_TOP_Y, DRAWER_FIXED_WHITE_WIDTH, 1, ROP_WHITE);
    hires_Mask(drawX - 1, y + DRAWER_IND_BOT_Y, DRAWER_FIXED_WHITE_WIDTH, 1, ROP_WHITE);
}