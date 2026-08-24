/*
  Graphics functionality - Coleco Adam / TMS9918A

  The VDP runs in Graphics II (mode 2) but is driven as a 32x24 character
  map: the name table at 0x1800 holds one glyph index per cell, and the
  pattern/color tables carry the charset (identical copies in all three
  screen thirds), giving per-glyph per-row colors at 1-byte-per-cell draw
  cost. A 768-byte RAM shadow of the name table backs saveScreenBuffer()
  and lets draw functions batch their VRAM writes.

  Layout and glyph indices follow the CoCo 1/2 client (src/coco/graphics.c);
  the charset is generated from support/coco/charset.fnt by
  support/adam/make_charset.py.
*/

#include "../misc.h"
#include <video/tms99x8.h>
#include <interrupt.h>

extern const uint8_t charsetPatterns[];
extern const uint8_t charsetColors[];
extern const uint8_t altPatterns[];
extern const uint8_t altColors[];

// From util.c - 60Hz NMI hook
extern void jiffyTick(void);
extern volatile uint8_t vsyncFlag;
extern volatile uint16_t jiffyCount;

#define NT_BASE 0x1800
#define CT_BASE 0x2000

#define CHARSET_GLYPHS 0x74
#define ALT_GLYPH_BASE 0xA0
#define ALT_GLYPH_COUNT 0x3B // copies of 0x20-0x5A

#define TILE_BLANK 0x20
#define TILE_SEA 0x18
#define TILE_HIT 0x19
#define TILE_MISS 0x1A
#define TILE_HIT2 0x1B
#define TILE_HIT_LEGEND 0x1C
#define TILE_CLOCK 0x1D
#define TILE_CONN_1 0x1E
#define TILE_CONN_2 0x1F
#define TILE_ATTACK_ANIM 0x63 // 6 frames
#define TILE_WATER_BASE 0x6B  // 6 water sparkle variants
#define TILE_RULE 0x71
#define TILE_BADGE_FILL 0x72
#define TILE_BAND 0x73
#define TILE_INACTIVE_MARK 0x62

// Board chrome (CoCo3 glyph vocabulary - the charset-16.png tile indices)
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
#define CHAR_BULLET 0x5B
#define FIELDX_1V1 5

static uint8_t screen[768];
static uint8_t screenBak[768];

static uint8_t fieldX = 0, playerCount = 0;

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

// Defined in this file
void drawShipInternal(uint16_t off, uint8_t size, uint8_t delta);

/// @brief Water sparkle variant for a field cell (CoCo3 formula)
static uint8_t waterTile(uint8_t quadrant, uint8_t x, uint8_t y)
{
    return TILE_WATER_BASE + ((quadrant + (((uint8_t)(y << 3)) % 5) + x) % 6);
}

/// @brief Copy a shadow-buffer range to the name table.
/// count 0 must never reach LDIRVM (it decrements before testing and would
/// spray 64K over VRAM); out-of-range writes would trash the sprite/color
/// tables that live above the name table.
static void blit(uint16_t off, uint16_t count)
{
    if (off >= 768)
        return;
    if (count > 768 - off)
        count = 768 - off;
    if (count == 0)
        return;
    vdp_vwrite((void *)(screen + off), NT_BASE + off, count);
}

/// @brief Set a single cell in shadow + VRAM
static void cell(uint16_t off, uint8_t c)
{
    if (off >= 768)
        return;
    screen[off] = c;
    vdp_vpoke(NT_BASE + off, c);
}

uint8_t cycleNextColor()
{
    return 0;
}

void waitvsync()
{
    // The VDP F flag (status bit 7) sets on every vblank regardless of the
    // interrupt enable, and reading status clears it - so polling works even
    // where the VDP interrupt is not wired through to the Z80 NMI (as in the
    // fujinet-go-adam emulator core). The flag is sticky, so a stale F from
    // an earlier vblank must be consumed FIRST or this returns immediately
    // and the whole game free-runs at full Z80 speed. When the NMI does fire
    // (real hardware), the CRT handler consumes the status read and the
    // installed tick sets vsyncFlag instead - both paths pace to one frame.
    vsyncFlag = 0;
    vdp_get_status(0); // clear any stale frame flag
    while (!vsyncFlag)
    {
        if (vdp_get_status(0) & 0x80)
            break;
    }
    ++jiffyCount;
}

void initGraphics()
{
    static uint16_t base;
    static uint8_t t;

    vdp_color(VDP_INK_WHITE, VDP_INK_BLACK, VDP_INK_BLACK);
    vdp_set_mode(mode_2);

    // Load charset patterns + colors into all three screen thirds
    for (t = 0; t < 3; t++)
    {
        base = (uint16_t)t << 11;
        vdp_vwrite((void *)charsetPatterns, base, CHARSET_GLYPHS * 8);
        vdp_vwrite((void *)charsetColors, CT_BASE + base, CHARSET_GLYPHS * 8);
        vdp_vwrite((void *)altPatterns, base + ALT_GLYPH_BASE * 8, ALT_GLYPH_COUNT * 8);
        vdp_vwrite((void *)altColors, CT_BASE + base + ALT_GLYPH_BASE * 8, ALT_GLYPH_COUNT * 8);
    }

    resetScreen();

    // 60Hz tick for getTime()/waitvsync()
    add_raster_int(jiffyTick);
}

void resetGraphics()
{
}

void resetScreen()
{
    memset(screen, TILE_BLANK, sizeof(screen));
    vdp_vfill(NT_BASE, TILE_BLANK, 768);
}

bool saveScreenBuffer()
{
    memcpy(screenBak, screen, sizeof(screen));
    return true;
}

void restoreScreenBuffer()
{
    memcpy(screen, screenBak, sizeof(screen));
    blit(0, 768);
}

void drawText(uint8_t x, uint8_t y, const char *s)
{
    static uint16_t off;
    static uint8_t c, n;

    if (y >= HEIGHT)
        y = HEIGHT - 1;
    if (x >= WIDTH) // centered text wider than the screen (33-char prompts)
        x = 0;

    off = xyoff(x, y);
    n = 0;
    while ((c = *s++) && x + n < WIDTH)
    {
        if (c >= 97 && c <= 122)
            c -= 32;
        screen[off + n++] = c;
    }
    blit(off, n);
}

void drawTextAlt(uint8_t x, uint8_t y, const char *s)
{
    static uint16_t off;
    static uint8_t c, n;

    if (y >= HEIGHT)
        y = HEIGHT - 1;
    if (x >= WIDTH) // centered text wider than the screen (33-char prompts)
        x = 0;

    off = xyoff(x, y);
    n = 0;
    while ((c = *s++) && x + n < WIDTH)
    {
        if (c < 65 || c > 90)
        {
            // Alternate color for anything not originally a capital letter
            if (c >= 97 && c <= 122)
                c -= 32;
            if (c >= 0x20 && c <= 0x5A)
                c += 0x80;
        }
        screen[off + n++] = c;
    }
    blit(off, n);
}

void drawIcon(uint8_t x, uint8_t y, uint8_t icon)
{
    cell(xyoff(x, y), icon);
}

void drawBlank(uint8_t x, uint8_t y)
{
    cell(xyoff(x, y), TILE_BLANK);
}

void drawSpace(uint8_t x, uint8_t y, uint8_t w)
{
    static uint16_t off;
    if (y >= HEIGHT)
        y = HEIGHT - 1;
    off = xyoff(x, y);
    memset(screen + off, TILE_BLANK, w);
    blit(off, w);
}

void drawLine(uint8_t x, uint8_t y, uint8_t w)
{
    static uint16_t off;
    if (y >= HEIGHT)
        y = HEIGHT - 1;
    off = xyoff(x, y);
    memset(screen + off, TILE_RULE, w);
    blit(off, w);
}

void drawBox(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    cell(xyoff(x, y), 0x3B);
    cell(xyoff(x + w + 1, y), 0x3C);
    cell(xyoff(x, y + h + 1), 0x3D);
    cell(xyoff(x + w + 1, y + h + 1), 0x3E);
}

void drawClock()
{
    cell(xyoff(WIDTH - 1, HEIGHT - 1), TILE_CLOCK);
}

void drawConnectionIcon(bool show)
{
    cell(xyoff(0, HEIGHT - 1), show ? TILE_CONN_1 : TILE_BLANK);
    cell(xyoff(1, HEIGHT - 1), show ? TILE_CONN_2 : TILE_BLANK);
}

void drawPlayerName(uint8_t i, const char *name, bool active)
{
    static uint8_t x, y, by, fy, ix, ox, drawX, drawEdge, r, n, c, left, fe;
    static uint16_t off;

    x = quadrantX[i] + fieldX;
    y = quadrantY[i];
    left = !(i > 1 || (playerCount == 2 && i > 0));

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

    if (i == 1 || i == 2)
    {
        // Upper boards: badge above field, far edge below (field above edge)
        by = y - 1;
        fy = y + 10;
        fe = CHAR_FAR_EDGE_LOWER;
        screen[xyoff(x - 1, by)] = 0x5C;
        screen[xyoff(x + 10, by)] = 0x5D;
    }
    else
    {
        // Lower boards: badge below field, far edge above (field below edge)
        by = y + 10;
        fy = y - 1;
        fe = CHAR_FAR_EDGE_UPPER;
        screen[xyoff(x - 1, by)] = 0x5E;
        screen[xyoff(x + 10, by)] = 0x5F;
    }

    // Outside edge
    for (r = 0; r < 10; r++)
        screen[xyoff(ox, y + r)] = left ? CHAR_OUTSIDE_EDGE_LEFT : CHAR_OUTSIDE_EDGE_RIGHT;

    // Inner edge (adjacent to ships drawer) with drawer cross-sections
    screen[xyoff(ix, y)] = left ? CHAR_CROSS_LEFT_UPPER : CHAR_CROSS_RIGHT_UPPER;
    screen[xyoff(ix, y + 9)] = left ? CHAR_CROSS_LEFT_LOWER : CHAR_CROSS_RIGHT_LOWER;
    for (r = 1; r < 9; r++)
        screen[xyoff(ix, y + r)] = left ? CHAR_INSIDE_EDGE_LEFT : CHAR_INSIDE_EDGE_RIGHT;

    // Far horizontal edge (divider row between the top and bottom boards).
    // fe glyphs hug the field side of the row; corner glyphs are fe-1/fe+1.
    screen[xyoff(x - 1, fy)] = fe - 1;
    screen[xyoff(x + 10, fy)] = fe + 1;
    memset(screen + xyoff(x, fy), fe, 10);

    // Ship drawer horizontal edges, vertical edge, corners
    memset(screen + xyoff(drawX, y), CHAR_DRAWER_EDGE_TOP, 3);
    memset(screen + xyoff(drawX, y + 9), CHAR_DRAWER_EDGE_BOTTOM, 3);
    for (r = 1; r < 9; r++)
        screen[xyoff(drawEdge, y + r)] = left ? CHAR_DRAWER_EDGE_LEFT : CHAR_DRAWER_EDGE_RIGHT;
    screen[xyoff(drawEdge, y)] = left ? CHAR_DRAWER_CORNER_LEFT : CHAR_DRAWER_CORNER_RIGHT;
    screen[xyoff(drawEdge, y + 9)] = (left ? CHAR_DRAWER_CORNER_LEFT : CHAR_DRAWER_CORNER_RIGHT) + 2;

    // Player name on the badge row
    off = xyoff(x, by);
    screen[off] = active ? CHAR_BULLET : TILE_INACTIVE_MARK;
    n = 1;
    while ((c = *name++) && n < 10)
    {
        if (c >= 97 && c <= 122)
            c -= 32;
        if (!active && c >= 0x20 && c <= 0x5A)
            c += 0x80; // alt color for inactive player names
        screen[off + n++] = c;
    }
    while (n < 10)
        screen[off + n++] = TILE_BADGE_FILL;

    // Push the whole 12-row slice of this board
    blit(xyoff(0, y - 1), 12 * WIDTH);
}

void drawBoard(uint8_t currentPlayerCount)
{
    static uint8_t i, r, x, y, drawX, cx;

    playerCount = currentPlayerCount;
    fieldX = playerCount > 2 ? 0 : FIELDX_1V1;

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
            memset(screen + xyoff(drawX, y + r), TILE_SEA, 3);
    }

    blit(0, 768);
}

void drawShipInternal(uint16_t off, uint8_t size, uint8_t delta)
{
    static uint8_t i, c;

    c = delta ? 0x17 : 0x12;
    for (i = 0; i < size; i++)
    {
        cell(off, c);
        if (delta)
        {
            off += WIDTH;
            c = (i == size - 2) ? 0x15 : 0x16;
        }
        else
        {
            off++;
            c = (i == size - 2) ? 0x14 : 0x13;
        }
    }
}

void drawShip(uint8_t quadrant, uint8_t size, uint8_t pos, bool hide)
{
    static uint8_t i, delta, cx, cy;
    static uint16_t off;

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
    static uint8_t i, x, y;
    static uint16_t off;

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
        // Red splats
        for (i = 0; i < size; i++)
        {
            cell(off, TILE_HIT_LEGEND);
            off += WIDTH;
        }
    }
}

void drawGamefield(uint8_t quadrant, uint8_t *field)
{
    static uint8_t x, y;
    static uint16_t off;

    for (y = 0; y < 10; ++y)
    {
        off = xyoff(quadrantX[quadrant] + fieldX, quadrantY[quadrant] + y);
        for (x = 0; x < 10; ++x)
        {
            if (*field)
                screen[off + x] = (*field == 1) ? TILE_HIT : TILE_MISS;
            field++;
        }
        blit(off, 10);
    }
}

void drawGamefieldUpdate(uint8_t quadrant, uint8_t *gamefield, uint8_t attackPos, uint8_t anim)
{
    static uint8_t c;
    static uint16_t off;

    off = xyoff(quadrantX[quadrant] + fieldX + (attackPos % 10), quadrantY[quadrant] + (attackPos / 10));

    // Animate attack only
    if (anim > 9)
    {
        cell(off, TILE_ATTACK_ANIM + anim - 10);
        return;
    }

    c = gamefield[attackPos];
    if (c == FIELD_ATTACK)
        cell(off, anim ? TILE_HIT2 : TILE_HIT);
    else if (c == FIELD_MISS)
        cell(off, TILE_MISS);
}

void drawGamefieldCursor(uint8_t quadrant, uint8_t x, uint8_t y, uint8_t *gamefield, uint8_t blink)
{
    static uint8_t c;

    c = gamefield[y * 10 + x];
    if (blink)
        c = c * 2 + 5 + blink; // cursor glyphs 0x06-0x0B
    else if (!c)
        c = waterTile(quadrant, x, y); // restore the water pattern
    else
        c += TILE_SEA; // restore hit/miss

    cell(xyoff(quadrantX[quadrant] + fieldX + x, quadrantY[quadrant] + y), c);
}

void drawEndgameMessage(const char *message)
{
    static uint8_t i, x;

    i = (uint8_t)strlen(message);
    x = (WIDTH - i) / 2;

    memset(screen + xyoff(0, HEIGHT - 2), TILE_BAND, WIDTH);
    memset(screen + xyoff(0, HEIGHT - 1), TILE_BLANK, WIDTH);
    blit(xyoff(0, HEIGHT - 2), WIDTH * 2);
    drawText(x, HEIGHT - 1, message);
}
