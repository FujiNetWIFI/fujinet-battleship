#ifdef BUILD_SMS

/*
  Graphics functionality - Sega Master System VDP, Mode 4

  The art is the CoCo 3 client's 16-colour sheet, brought across tile for tile
  by src/sms/mktiles.py: Mode 4 is 4bpp as well, and the CoCo 3's 6-bit RGB
  is exactly the Master System's CRAM, so nothing is reduced. The layout is
  src/adam/graphics.c's -- the CoCo 3's vocabulary on the same 32x24 grid --
  and the tile numbers are the sheet's own.

  What the CoCo 3 did with raster ops, palette 1 does here per cell (name
  table bit 11): it is palette 0 with the chrome's warm colours dimmed the
  way ROP_INACTIVE masks them (index & 7, so red-orange becomes teal), and
  with the text colour turned to foam, ROP_ALT's. Inactive boards, inactive
  name plates and alternate text are therefore the same tiles in palette 1.
  The fields, ships and drawers are always palette 0, so a hit stays red.

  Text is the MS-DOS font in its own tile range, so the sheet's art keeps the
  ASCII slots and nothing is sacrificed to punctuation. Names sit on the
  badges in a font of their own, white on the badge colour, with the badge's
  white outline row baked in above (upper boards) or below (lower boards).

  A RAM shadow of the name table backs saveScreenBuffer() and lets the board
  be composed in RAM and pushed in one stream (vdp.asm's vdp_blit, paced for
  the display being on).
*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "vars.h"
#include "tiles.h"
#include "../misc.h"

#define NAME_TABLE  0x3800
#define SAT         0x3F00
#define VRAM_WRITE  0x4000
#define CRAM_WRITE  0xC000

#define CELLS       (WIDTH * HEIGHT)

/* Name table entry bits above the tile number */
#define PAL1        0x0800

extern void vdp_addr(unsigned int cmd) __z88dk_fastcall;    /* vdp.asm */
extern void vdp_word(unsigned int w) __z88dk_fastcall;
extern void vdp_byte(unsigned char b) __z88dk_fastcall;
extern void vdp_blit(const unsigned int *cells) __z88dk_fastcall;
extern void vdp_runs(const unsigned char *runs) __z88dk_fastcall;
extern unsigned int blitCount;

/* The VDP data port, for the font expansion (display off, so no pacing) */
__sfr __at 0xBE vdpData;

// Board chrome (CoCo 3 sheet indices, as src/adam/graphics.c names them)
#define CHAR_OUTSIDE_EDGE_LEFT 0x40
#define CHAR_OUTSIDE_EDGE_RIGHT 0x3F
#define CHAR_INSIDE_EDGE_LEFT 0x60
#define CHAR_INSIDE_EDGE_RIGHT 0x61
#define CHAR_CROSS_LEFT_UPPER 0x02
#define CHAR_CROSS_LEFT_LOWER 0x04
#define CHAR_CROSS_RIGHT_UPPER 0x22
#define CHAR_CROSS_RIGHT_LOWER 0x24
// "UPPER" set = field sits below the edge; "LOWER" set = field above it
#define CHAR_FAR_EDGE_UPPER 0x10 // corners 0x0F / 0x11
#define CHAR_FAR_EDGE_LOWER 0x0D // corners 0x0C / 0x0E
#define CHAR_DRAWER_CORNER_LEFT 0x01  // +2 = bottom corner
#define CHAR_DRAWER_CORNER_RIGHT 0x23 // +2 = bottom corner
#define CHAR_DRAWER_EDGE_TOP 0x28
#define CHAR_DRAWER_EDGE_BOTTOM 0x29
#define CHAR_DRAWER_EDGE_LEFT 0x05
#define CHAR_DRAWER_EDGE_RIGHT 0x26
#define CHAR_BADGE_UPPER_LEFT 0x5C // upper boards' badge; 0x5D right
#define CHAR_BADGE_LOWER_LEFT 0x5E // lower boards' badge; 0x5F right
#define CHAR_BULLET 0x5B
#define CHAR_SHIP_H 0x12 // left end; 0x13 middle, 0x14 right end
#define CHAR_SHIP_V 0x17 // top end; 0x16 middle, 0x15 bottom end
#define CHAR_CURSOR 0x05 // + 2 * cell + blink: 0x06-0x0B
#define CHAR_BOX 0x3B    // corners 0x3B-0x3E
#define FIELDX_1V1 5

#define TEXT(c) (TEXT_BASE - 0x20 + (c))

static unsigned int screen[CELLS];
static unsigned int screenBak[CELLS];

static uint8_t fieldX = 0, playerCount = 0;

/// @brief which players' boards are drawn active (bit per player)
static uint8_t activeMask;

/**
 * @brief Top left of each playfield quadrant (char cells)
 */
static const uint8_t quadrantX[4] = {5, 5, 17, 17};
static const uint8_t quadrantY[4] = {12, 1, 1, 12};

/**
 * @brief offset of ships within legend/drawer (x, y in cells)
 */
static const uint8_t legendShipOffset[5][2] = {
    {2, 1},
    {1, 1},
    {0, 1},
    {0, 6},
    {1, 7},
};

#define xyoff(x, y) ((uint16_t)(y)*WIDTH + (x))

static void vdp_reg(unsigned char r, unsigned char v)
{
    vdp_addr(((unsigned int)(0x80 | r) << 8) | v);
}

/// @brief Water sparkle variant for a field cell (the CoCo 3's formula)
static unsigned int waterTile(uint8_t quadrant, uint8_t x, uint8_t y)
{
    return T_WATER + ((quadrant + (((uint8_t)(y << 3)) % 5) + x) % 6);
}

/// @brief Copy a shadow range to the name table
static void blit(uint16_t off, uint16_t count)
{
    if (off >= CELLS)
        return;
    if (count > CELLS - off)
        count = CELLS - off;
    if (count == 0)
        return;
    vdp_addr(VRAM_WRITE | NAME_TABLE | (off << 1));
    blitCount = count;
    vdp_blit(screen + off);
}

/// @brief Set a single cell in shadow + VRAM
static void cell(uint16_t off, unsigned int c)
{
    if (off >= CELLS)
        return;
    screen[off] = c;
    vdp_addr(VRAM_WRITE | NAME_TABLE | (off << 1));
    vdp_word(c);
}

/// @brief Fill a run of shadow cells (no VRAM write)
static void fill(uint16_t off, unsigned int c, uint8_t n)
{
    while (n--)
        screen[off++] = c;
}

#define STRIP_NONE 0
#define STRIP_TOP 1
#define STRIP_BOTTOM 2

/**
 * @brief Expand count glyphs of the 1bpp font into 4bpp tiles from tile,
 * in colours fg on bg. A strip row is solid fg: the name plates' white
 * outline, above the glyph (which then moves down a row) or below it.
 */
static void loadGlyphs(unsigned int tile, uint8_t count, uint8_t fg, uint8_t bg, uint8_t strip)
{
    const unsigned char *src = sms_font;
    uint8_t r, p, bits, b;

    vdp_addr(VRAM_WRITE | (tile << 5));
    while (count--)
    {
        for (r = 0; r < 8; r++)
        {
            if (strip == STRIP_TOP)
                bits = r ? src[r - 1] : 0xFF;
            else if (strip == STRIP_BOTTOM && r == 7)
                bits = 0xFF;
            else
                bits = src[r];

            for (p = 1; p != 0x10; p <<= 1)
            {
                b = 0;
                if (fg & p)
                    b = bits;
                if (bg & p)
                    b |= (uint8_t)~bits;
                vdpData = b;
            }
        }
        src += 8;
    }
}

uint8_t cycleNextColor()
{
    return 0;
}

void initGraphics()
{
    uint8_t i;

    vdp_reg(1, 0x80); // display and frame interrupt off
    vdp_reg(7, 0x00); // border: palette 1 colour 0, black

    vdp_addr(CRAM_WRITE);
    for (i = 0; i < 32; i++)
        vdp_byte(sms_palette[i]);

    vdp_runs(sms_tiles);
    loadGlyphs(TEXT_BASE, TEXT_GLYPHS, C_TEXT, C_BLACK, STRIP_NONE);
    loadGlyphs(NAME_BOT_BASE, NAME_GLYPHS, C_WHITE, C_BADGE, STRIP_BOTTOM);
    loadGlyphs(NAME_TOP_BASE, NAME_GLYPHS, C_WHITE, C_BADGE, STRIP_TOP);

    // No sprites: a Y of $D0 ends the list at the first entry.
    vdp_addr(VRAM_WRITE | SAT);
    vdp_byte(0xD0);

    resetScreen();

    // Display on, frame interrupt on: crt0's handler counts frames in timer,
    // which waitvsync() and getTime() read (src/sms/util.c).
    vdp_reg(1, 0xE0);
}

void resetGraphics()
{
}

void resetScreen()
{
    uint16_t i;

    for (i = 0; i < CELLS; i++)
        screen[i] = T_BLANK;
    blit(0, CELLS);
}

bool saveScreenBuffer()
{
    memcpy(screenBak, screen, sizeof(screen));
    return true;
}

void restoreScreenBuffer()
{
    memcpy(screen, screenBak, sizeof(screen));
    blit(0, CELLS);
}

/// @brief Text tile for a character; anything outside the font is '?'
static unsigned int textTile(uint8_t c)
{
    if (c < 0x20 || c > 0x7F)
        c = '?';
    return TEXT(c);
}

/// @brief Draw text, capitals in palette 1 (foam) when alt is set
static void putText(uint8_t x, uint8_t y, const char *s, bool alt)
{
    uint16_t off;
    uint8_t c, n;

    if (y >= HEIGHT)
        y = HEIGHT - 1;
    if (x >= WIDTH) // centered text wider than the screen (34-char prompts)
        x = 0;

    off = xyoff(x, y);
    n = 0;
    while ((c = (uint8_t)*s++) && x + n < WIDTH)
    {
        screen[off + n++] = textTile(c) | (alt && c >= 'A' && c <= 'Z' ? PAL1 : 0);
    }
    blit(off, n);
}

void drawText(uint8_t x, uint8_t y, const char *s)
{
    putText(x, y, s, false);
}

void drawTextAlt(uint8_t x, uint8_t y, const char *s)
{
    putText(x, y, s, true);
}

void drawIcon(uint8_t x, uint8_t y, uint8_t icon)
{
    cell(xyoff(x, y), icon);
}

void drawBlank(uint8_t x, uint8_t y)
{
    cell(xyoff(x, y), T_BLANK);
}

void drawSpace(uint8_t x, uint8_t y, uint8_t w)
{
    uint16_t off;

    if (y >= HEIGHT)
        y = HEIGHT - 1;
    if (x >= WIDTH)
        return;
    if (w > WIDTH - x)
        w = WIDTH - x;
    off = xyoff(x, y);
    fill(off, T_BLANK, w);
    blit(off, w);
}

void drawLine(uint8_t x, uint8_t y, uint8_t w)
{
    uint16_t off;

    if (y >= HEIGHT)
        y = HEIGHT - 1;
    if (x >= WIDTH)
        return;
    if (w > WIDTH - x)
        w = WIDTH - x;
    off = xyoff(x, y);
    fill(off, T_RULE, w);
    blit(off, w);
}

void drawBox(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    cell(xyoff(x, y), CHAR_BOX);
    cell(xyoff(x + w + 1, y), CHAR_BOX + 1);
    cell(xyoff(x, y + h + 1), CHAR_BOX + 2);
    cell(xyoff(x + w + 1, y + h + 1), CHAR_BOX + 3);
}

void drawClock()
{
    cell(xyoff(WIDTH - 1, HEIGHT - 1), T_CLOCK);
}

void drawConnectionIcon(bool show)
{
    cell(xyoff(0, HEIGHT - 1), show ? T_CONN_1 : T_BLANK);
    cell(xyoff(1, HEIGHT - 1), show ? T_CONN_2 : T_BLANK);
}

/**
 * @brief true when this player's far edge (row 11) is shared with the board
 * above/below it, which happens whenever that counterpart is drawn. The
 * CoCo 3 has a 25th row and draws the two edges a row apart; here they share
 * a row, and the generator's merged tiles carry both.
 */
static bool sharedBorderRow(uint8_t player)
{
    if (player == 0)
        return playerCount >= 2;
    if (player == 2)
        return playerCount == 4;
    return true; // 1 always has 0 below; 3 only exists in 4-player
}

/// @brief the board sharing row 11 with this one
static const uint8_t borderPartner[4] = {1, 0, 3, 2};

void drawPlayerName(uint8_t i, const char *name, bool active)
{
    // static, not stack: sccz80 (-O2 and -O3 alike) compiles `ix = x - 1`
    // between two stack chars as a load from x's offset + 2 -- it drops the
    // PUSH it had reckoned the offset against -- so the field's inner edge,
    // far edge and upper badge landed on garbage coordinates.
    static uint8_t x, y, by, fy, ix, ox, drawX, drawEdge, r, n, c, left, upper;
    static unsigned int pal, edgePal, fe, glyph;
    static uint16_t off;

    x = quadrantX[i] + fieldX;
    y = quadrantY[i];
    left = !(i > 1 || (playerCount == 2 && i > 0));
    upper = (i == 1 || i == 2);

    if (active)
        activeMask |= 1 << i;
    else
        activeMask &= ~(1 << i);

    // The CoCo 3 drew the chrome through ROP_INACTIVE for an idle player:
    // here that is palette 1. The drawer is always drawn plain.
    pal = active ? 0 : PAL1;

    if (left)
    {
        // Left ship drawer
        ix = x - 1;
        ox = x + 10;
        drawX = x - 4;
        drawEdge = x - 5;
    }
    else
    {
        // Right ship drawer
        ox = x - 1;
        ix = x + 10;
        drawX = x + 11;
        drawEdge = x + 14;
    }

    if (upper)
    {
        // Upper boards: badge above field, far edge below (field above edge)
        by = y - 1;
        fy = y + 10;
        fe = CHAR_FAR_EDGE_LOWER;
        screen[xyoff(x - 1, by)] = CHAR_BADGE_UPPER_LEFT | pal;
        screen[xyoff(x + 10, by)] = (CHAR_BADGE_UPPER_LEFT + 1) | pal;
    }
    else
    {
        // Lower boards: badge below field, far edge above (field below edge)
        by = y + 10;
        fy = y - 1;
        fe = CHAR_FAR_EDGE_UPPER;
        screen[xyoff(x - 1, by)] = CHAR_BADGE_LOWER_LEFT | pal;
        screen[xyoff(x + 10, by)] = (CHAR_BADGE_LOWER_LEFT + 1) | pal;
    }

    // Outside edge
    for (r = 0; r < 10; r++)
        screen[xyoff(ox, y + r)] = (left ? CHAR_OUTSIDE_EDGE_LEFT : CHAR_OUTSIDE_EDGE_RIGHT) | pal;

    // Inner edge (adjacent to ships drawer) with drawer cross-sections
    screen[xyoff(ix, y)] = (left ? CHAR_CROSS_LEFT_UPPER : CHAR_CROSS_RIGHT_UPPER) | pal;
    screen[xyoff(ix, y + 9)] = (left ? CHAR_CROSS_LEFT_LOWER : CHAR_CROSS_RIGHT_LOWER) | pal;
    for (r = 1; r < 9; r++)
        screen[xyoff(ix, y + r)] = (left ? CHAR_INSIDE_EDGE_LEFT : CHAR_INSIDE_EDGE_RIGHT) | pal;

    // Far horizontal edge (the row between the top and bottom boards). When
    // the other board of the column is drawn too, the row carries both edges
    // in one merged tile set, dimmed only when both boards are idle.
    if (sharedBorderRow(i))
    {
        edgePal = (activeMask & ((1 << i) | (1 << borderPartner[i]))) ? 0 : PAL1;
        screen[xyoff(x - 1, fy)] = T_JOIN_LEFT | edgePal;
        screen[xyoff(x + 10, fy)] = T_JOIN_RIGHT | edgePal;
        fill(xyoff(x, fy), T_JOIN | edgePal, 10);
    }
    else
    {
        // fe glyphs hug the field side of the row; corners are fe-1/fe+1
        screen[xyoff(x - 1, fy)] = (fe - 1) | pal;
        screen[xyoff(x + 10, fy)] = (fe + 1) | pal;
        fill(xyoff(x, fy), fe | pal, 10);
    }

    // Ship drawer horizontal edges, vertical edge, corners
    fill(xyoff(drawX, y), CHAR_DRAWER_EDGE_TOP, 3);
    fill(xyoff(drawX, y + 9), CHAR_DRAWER_EDGE_BOTTOM, 3);
    for (r = 1; r < 9; r++)
        screen[xyoff(drawEdge, y + r)] = left ? CHAR_DRAWER_EDGE_LEFT : CHAR_DRAWER_EDGE_RIGHT;
    screen[xyoff(drawEdge, y)] = left ? CHAR_DRAWER_CORNER_LEFT : CHAR_DRAWER_CORNER_RIGHT;
    screen[xyoff(drawEdge, y + 9)] = (left ? CHAR_DRAWER_CORNER_LEFT : CHAR_DRAWER_CORNER_RIGHT) + 2;

    // Player name on the badge row: the bullet marks the active player
    off = xyoff(x, by);
    screen[off] = active ? (upper ? T_BULLET_TOP : T_BULLET_BOT)
                         : (upper ? T_BADGE_TOP : T_BADGE_BOT) | PAL1;
    glyph = (upper ? NAME_TOP_BASE : NAME_BOT_BASE) - 0x20;
    n = 1;
    while ((c = (uint8_t)*name++) && n < 10)
    {
        if (c >= 'a' && c <= 'z')
            c -= 32;
        if (c < 0x20 || c > 0x5F)
            c = '?';
        screen[off + n++] = (glyph + c) | pal;
    }
    while (n < 10)
        screen[off + n++] = (upper ? T_BADGE_TOP : T_BADGE_BOT) | pal;

    // Push the whole 12-row slice of this board
    blit(xyoff(0, y - 1), 12 * WIDTH);
}

void drawBoard(uint8_t currentPlayerCount)
{
    uint8_t i, r, x, y, drawX, cx;

    playerCount = currentPlayerCount;
    fieldX = playerCount > 2 ? 0 : FIELDX_1V1;
    activeMask = 0;

    for (i = 0; i < playerCount; i++)
    {
        x = quadrantX[i] + fieldX;
        y = quadrantY[i];

        // Draw player border
        drawPlayerName(i, "", false);

        if (i > 1 || (playerCount == 2 && i > 0))
            drawX = x + 11;
        else
            drawX = x - 4;

        // Water with sparkle variants
        for (r = 0; r < 10; r++)
            for (cx = 0; cx < 10; cx++)
                screen[xyoff(x + cx, y + r)] = waterTile(i, cx, r);

        // Flat sea in the ship drawer
        for (r = 1; r < 9; r++)
            fill(xyoff(drawX, y + r), T_SEA, 3);
    }

    blit(0, CELLS);
}

static void drawShipInternal(uint16_t off, uint8_t size, uint8_t delta)
{
    uint8_t i, c;

    c = delta ? CHAR_SHIP_V : CHAR_SHIP_H;
    for (i = 0; i < size; i++)
    {
        cell(off, c);
        if (delta)
        {
            off += WIDTH;
            c = (i == size - 2) ? CHAR_SHIP_V - 2 : CHAR_SHIP_V - 1;
        }
        else
        {
            off++;
            c = (i == size - 2) ? CHAR_SHIP_H + 2 : CHAR_SHIP_H + 1;
        }
    }
}

void drawShip(uint8_t quadrant, uint8_t size, uint8_t pos, bool hide)
{
    uint8_t i, delta, cx, cy;
    uint16_t off;

    delta = 0;
    if (pos > 99)
    {
        delta = 1; // 1=vertical, 0=horizontal
        pos -= 100;
    }

    cx = pos % 10;
    cy = pos / 10;
    off = xyoff(quadrantX[quadrant] + fieldX + cx, quadrantY[quadrant] + cy);

    if (hide)
    {
        // Restore the water pattern beneath the ship
        for (i = 0; i < size; i++)
        {
            cell(off, waterTile(quadrant, cx, cy));
            if (delta)
            {
                off += WIDTH;
                cy++;
            }
            else
            {
                off++;
                cx++;
            }
        }
        return;
    }

    drawShipInternal(off, size, delta);
}

void drawLegendShip(uint8_t player, uint8_t index, uint8_t size, uint8_t status)
{
    uint8_t i, x, y;
    uint16_t off;

    x = quadrantX[player] + legendShipOffset[index][0] + fieldX;
    y = quadrantY[player] + legendShipOffset[index][1];

    if (player > 1 || (player > 0 && fieldX > 0))
        x += 11;
    else
        x -= 4;

    off = xyoff(x, y);

    if (status)
    {
        drawShipInternal(off, size, 1);
    }
    else
    {
        // Burning wrecks
        for (i = 0; i < size; i++)
        {
            cell(off, T_HIT_LEGEND);
            off += WIDTH;
        }
    }
}

void drawGamefield(uint8_t quadrant, uint8_t *field)
{
    uint8_t x, y;
    uint16_t off;

    for (y = 0; y < 10; ++y)
    {
        off = xyoff(quadrantX[quadrant] + fieldX, quadrantY[quadrant] + y);
        for (x = 0; x < 10; ++x)
        {
            if (*field)
                screen[off + x] = (*field == FIELD_ATTACK) ? T_HIT : T_MISS;
            field++;
        }
        blit(off, 10);
    }
}

void drawGamefieldUpdate(uint8_t quadrant, uint8_t *gamefield, uint8_t attackPos, uint8_t anim)
{
    uint8_t c;
    uint16_t off;

    off = xyoff(quadrantX[quadrant] + fieldX + (attackPos % 10), quadrantY[quadrant] + (attackPos / 10));

    // Animate attack only
    if (anim > 9)
    {
        cell(off, T_ATTACK_ANIM + anim - 10);
        return;
    }

    c = gamefield[attackPos];
    if (c == FIELD_ATTACK)
        cell(off, anim ? T_HIT2 : T_HIT);
    else if (c == FIELD_MISS)
        cell(off, T_MISS);
}

void drawGamefieldCursor(uint8_t quadrant, uint8_t x, uint8_t y, uint8_t *gamefield, uint8_t blink)
{
    unsigned int c;

    c = gamefield[y * 10 + x];
    if (blink)
        c = CHAR_CURSOR + c * 2 + blink; // cursor glyphs 0x06-0x0B
    else if (!c)
        c = waterTile(quadrant, x, y); // restore the water pattern
    else
        c += T_SEA; // restore hit/miss

    cell(xyoff(quadrantX[quadrant] + fieldX + x, quadrantY[quadrant] + y), c);
}

/**
 * @brief The result, white on a badge-coloured band across the bottom row,
 * the way the CoCo 3 bands it - the lower boards' name plates stay visible.
 */
void drawEndgameMessage(const char *message)
{
    uint8_t c, n, x;
    uint16_t off;

    n = (uint8_t)strlen(message);
    if (n > WIDTH)
        n = WIDTH;
    x = (WIDTH - n) / 2;

    off = xyoff(0, HEIGHT - 1);
    fill(off, T_BADGE_TOP, WIDTH);
    off += x;
    while ((c = (uint8_t)*message++) && n--)
    {
        if (c >= 'a' && c <= 'z')
            c -= 32;
        if (c < 0x20 || c > 0x5F)
            c = '?';
        screen[off++] = NAME_TOP_BASE - 0x20 + c;
    }
    blit(xyoff(0, HEIGHT - 1), WIDTH);
}

#endif /* BUILD_SMS */
