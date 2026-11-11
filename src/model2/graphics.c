#ifdef __TRS80M2__

/*
  Graphics - TRS-80 Model II (80x24)

  src/h89/graphics.c with the Model II's characters: its heavy line set, the
  partial blocks (a cell's lower rows or left columns of dots) and reverse
  video. The layout is the H89's, every board cell two characters wide:

    row 0      name badges of the top boards
    rows 1-10  top fields
    row 11     divider shared by the top and bottom boards
    rows 12-21 bottom fields
    row 22     name badges of the bottom boards
    row 23     status: connection, timer, clock

  Fields are 20 columns wide, at column 18 (left boards) and 42 (right
  boards), or 30 when there are only one or two players. The fleet drawer
  (the ships-left legend) sits outside the field, three hull columns wide.

  Drawing goes into console.c's back buffer and goes to the screen at
  waitvsync(), when input is read, and before every FujiNet call.
*/

#include "../misc.h"
#include "console.h"

#define FIELD_W 20     // columns in a field (10 cells, two characters each)
#define SHIFT_1V1 12   // field shift right when the boards stack in one column
#define DRAWER_LEFT 7  // left drawer interior starts this far left of the field
#define DRAWER_RIGHT 21 // right drawer interior starts this far right of it

/**
 * @brief Top left of each player's field. Players run clockwise from the
 * bottom left.
 */
static const uint8_t quadrantX[4] = {18, 18, 42, 42};
static const uint8_t quadrantY[4] = {12, 1, 1, 12};

/**
 * @brief Ship positions in the fleet drawer (hull column, row from the field top)
 */
static const uint8_t legendShipOffset[10] = {
    2, 1,
    1, 1,
    0, 1,
    0, 6,
    1, 7,
};

static uint8_t fieldShift = 0, playerCount = 0;

/*
 * A field cell is a pair of screen cells: two cell codes, left and right.
 * (Flat arrays: sccz80 balks at nested initializers.)
 */
static const uint8_t WATER[2] = {' ', ' '};
static const uint8_t MISS[2] = {'(', ')'}; // a centered circle
static const uint8_t HIT[2] = {REV | '>', REV | '<'}; // a cross over both cells
static const uint8_t HIT2[2] = {'#', '#'};
static const uint8_t SUNK[2] = {'#', '#'};

// Horizontal hulls take the part of the row toward the middle of the screen
// -- the lower 6 rows of dots on the top boards, the upper 6 on the bottom
// ones (a reversed lower 4) -- so they never touch a name badge, and step
// down to 4 rows at the bow and stern.
static const uint8_t HULL_H_LOW[6] = {
    G_LOW_4, G_LOW_6, // bow
    G_LOW_6, G_LOW_6, // middle
    G_LOW_6, G_LOW_4, // stern
};
static const uint8_t HULL_H_HIGH[6] = {
    REV | G_LOW_6, REV | G_LOW_4,
    REV | G_LOW_4, REV | G_LOW_4,
    REV | G_LOW_4, REV | G_LOW_6,
};

// Vertical hulls are centered on the two columns: 12 dots wide, 8 at the bow
// and stern
static const uint8_t HULL_V_END[2] = {REV | G_LEFT_4, G_LEFT_4};
static const uint8_t HULL_V_MID[2] = {REV | G_LEFT_2, G_LEFT_6};

// The attack, frames 10-15: closing in, the strike, the flash
static const uint8_t ATTACK[12] = {
    '>', '<',
    '\\', '/',
    '*', '*',
    REV | '*', REV | '*',
    '#', '#',
    REV | ' ', REV | ' ',
};

static void putPair(uint8_t x, uint8_t y, const uint8_t *p)
{
    put(x, y, p[0]);
    put(x + 1, y, p[1]);
}

static uint8_t fieldLeft(uint8_t q)
{
    return quadrantX[q] + fieldShift;
}

/// @brief Does this player's drawer sit right of the field?
static bool drawerRight(uint8_t q)
{
    return q > 1 || (playerCount < 3 && q > 0);
}

/// @brief Is there a board above or below this one, sharing its divider?
static bool hasPartner(uint8_t q)
{
    static const uint8_t partner[4] = {1, 0, 3, 2};

    return partner[q] < playerCount;
}

uint8_t cycleNextColor()
{
    return 0;
}

void initGraphics()
{
    termInit();
}

void resetGraphics()
{
    termExit();
}

void resetScreen()
{
    termClear();
}

bool saveScreenBuffer()
{
    termSave();
    return true;
}

void restoreScreenBuffer()
{
    termRestore();
}

/// @brief Text, as is; with alt, capitals not followed by a lower case letter
/// go in reverse video
static void text(uint8_t x, uint8_t y, const char *s, bool alt)
{
    static uint8_t c;

    if (y >= HEIGHT)
        y = HEIGHT - 1;
    if (x >= WIDTH) // centered text wider than the screen
        x = 0;

    while ((c = *s++) && x < WIDTH)
    {
        if (c < 0x20 || c > 0x7E)
            c = '?';
        else if (alt && c >= 'A' && c <= 'Z' && !(*s >= 'a' && *s <= 'z'))
            c |= REV;
        put(x++, y, c);
    }
}

void drawText(uint8_t x, uint8_t y, const char *s)
{
    text(x, y, s, false);
}

void drawTextAlt(uint8_t x, uint8_t y, const char *s)
{
    text(x, y, s, true);
}

void drawIcon(uint8_t x, uint8_t y, uint8_t icon)
{
    put(x, y, icon);
}

void drawBlank(uint8_t x, uint8_t y)
{
    put(x, y, ' ');
}

void drawSpace(uint8_t x, uint8_t y, uint8_t w)
{
    fill(x, y, w, ' ');
}

void drawLine(uint8_t x, uint8_t y, uint8_t w)
{
    fill(x, y, w, G_HLINE);
}

void drawBox(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    static uint8_t i;

    put(x, y, G_CORNER_UL);
    fill(x + 1, y, w, G_HLINE);
    put(x + w + 1, y, G_CORNER_UR);

    for (i = 1; i <= h; i++)
    {
        put(x, y + i, G_VLINE);
        put(x + w + 1, y + i, G_VLINE);
    }

    put(x, y + h + 1, G_CORNER_LL);
    fill(x + 1, y + h + 1, w, G_HLINE);
    put(x + w + 1, y + h + 1, G_CORNER_LR);
}

void drawClock()
{
    put(WIDTH - 1, HEIGHT - 1, '*');
}

void drawConnectionIcon(bool show)
{
    put(0, HEIGHT - 1, show ? REV | '<' : ' ');
    put(1, HEIGHT - 1, show ? REV | '>' : ' ');
}

void drawPlayerName(uint8_t i, const char *name, bool active)
{
    static uint8_t x, y, by, fy, ox, ix, drawX, drawEdge, r, n, c, fillCode, right;

    x = fieldLeft(i);
    y = quadrantY[i];
    right = drawerRight(i);

    if (right)
    {
        ox = x - 1;
        ix = x + FIELD_W;
        drawX = x + DRAWER_RIGHT;
        drawEdge = drawX + 6;
    }
    else
    {
        ix = x - 1;
        ox = x + FIELD_W;
        drawX = x - DRAWER_LEFT;
        drawEdge = drawX - 1;
    }

    if (i == 1 || i == 2)
    {
        // Top board: badge above the field, divider below it
        by = y - 1;
        fy = y + 10;
        put(x - 1, by, G_CORNER_UL);
        put(x + FIELD_W, by, G_CORNER_UR);
    }
    else
    {
        // Bottom board: divider above the field, badge below it
        by = y + 10;
        fy = y - 1;
        put(x - 1, by, G_CORNER_LL);
        put(x + FIELD_W, by, G_CORNER_LR);
    }

    // Field edges. The drawer's top and bottom edges meet the inner one.
    for (r = 0; r < 10; r++)
    {
        put(ox, y + r, G_VLINE);
        put(ix, y + r, (r == 0 || r == 9) ? (right ? G_TEE_RIGHT : G_TEE_LEFT) : G_VLINE);
    }

    // Divider: a T where a board sits on its other side too
    fill(x, fy, FIELD_W, G_HLINE);
    if (hasPartner(i))
    {
        put(x - 1, fy, G_TEE_RIGHT);
        put(x + FIELD_W, fy, G_TEE_LEFT);
    }
    else if (i == 1 || i == 2)
    {
        put(x - 1, fy, G_CORNER_LL);
        put(x + FIELD_W, fy, G_CORNER_LR);
    }
    else
    {
        put(x - 1, fy, G_CORNER_UL);
        put(x + FIELD_W, fy, G_CORNER_UR);
    }

    // Fleet drawer
    fill(drawX, y, 6, G_HLINE);
    fill(drawX, y + 9, 6, G_HLINE);
    put(drawEdge, y, right ? G_CORNER_UR : G_CORNER_UL);
    put(drawEdge, y + 9, right ? G_CORNER_LR : G_CORNER_LL);
    for (r = 1; r < 9; r++)
        put(drawEdge, y + r, G_VLINE);

    // Badge: "-- name ------" or, for the active player, the whole bar in
    // reverse video with a star
    fillCode = active ? REV | ' ' : G_HLINE;
    n = 0;
    put(x + n++, by, fillCode);
    if (*name)
    {
        put(x + n++, by, active ? REV | '*' : G_HLINE);
        put(x + n++, by, active ? REV | ' ' : ' ');
        while ((c = *name++) && n < FIELD_W - 2)
        {
            if (c < 0x20 || c > 0x7E)
                c = '?';
            put(x + n++, by, active ? REV | c : c);
        }
        put(x + n++, by, active ? REV | ' ' : ' ');
    }
    while (n < FIELD_W)
        put(x + n++, by, fillCode);
}

void drawBoard(uint8_t currentPlayerCount)
{
    static uint8_t i, r, x, y, drawX;

    playerCount = currentPlayerCount;
    fieldShift = playerCount > 2 ? 0 : SHIFT_1V1;

    for (i = 0; i < playerCount; i++)
    {
        // Frame, drawer and an empty badge
        drawPlayerName(i, "", false);

        // Water
        x = fieldLeft(i);
        y = quadrantY[i];
        drawX = drawerRight(i) ? x + DRAWER_RIGHT : x - DRAWER_LEFT;
        for (r = 0; r < 10; r++)
            fill(x, y + r, FIELD_W, ' ');
        for (r = 1; r < 9; r++)
            fill(drawX, y + r, 6, ' ');
    }
}

/// @brief A ship's hull from its bow at x,y on quadrant's board
static void drawShipInternal(uint8_t quadrant, uint8_t x, uint8_t y, uint8_t size, uint8_t delta)
{
    static uint8_t i;
    static const uint8_t *p;

    for (i = 0; i < size; i++)
    {
        if (delta)
        {
            p = (i == 0 || i == size - 1) ? HULL_V_END : HULL_V_MID;
            putPair(x, y++, p);
        }
        else
        {
            p = (quadrant == 1 || quadrant == 2) ? HULL_H_LOW : HULL_H_HIGH;
            if (i == size - 1)
                p += 4;
            else if (i)
                p += 2;
            putPair(x, y, p);
            x += 2;
        }
    }
}

void drawShip(uint8_t quadrant, uint8_t size, uint8_t pos, bool hide)
{
    static uint8_t i, delta, x, y;

    delta = 0;
    if (pos > 99)
    {
        delta = 1; // vertical
        pos -= 100;
    }

    x = fieldLeft(quadrant) + (pos % 10) * 2;
    y = quadrantY[quadrant] + pos / 10;

    if (hide)
    {
        for (i = 0; i < size; i++)
        {
            putPair(x, y, WATER);
            if (delta)
                y++;
            else
                x += 2;
        }
        return;
    }

    drawShipInternal(quadrant, x, y, size, delta);
}

void drawLegendShip(uint8_t player, uint8_t index, uint8_t size, uint8_t status)
{
    static uint8_t i, x, y;

    x = fieldLeft(player) + legendShipOffset[index * 2] * 2;
    y = quadrantY[player] + legendShipOffset[index * 2 + 1];

    if (drawerRight(player))
        x += DRAWER_RIGHT;
    else
        x -= DRAWER_LEFT;

    if (status)
    {
        drawShipInternal(player, x, y, size, 1);
    }
    else
    {
        for (i = 0; i < size; i++)
            putPair(x, y + i, SUNK);
    }
}

void drawGamefield(uint8_t quadrant, uint8_t *field)
{
    static uint8_t x, y, cx;

    y = quadrantY[quadrant];
    for (cx = 0; cx < 100; cx++)
    {
        if (*field)
        {
            x = fieldLeft(quadrant) + (cx % 10) * 2;
            putPair(x, y + cx / 10, *field == FIELD_ATTACK ? HIT : MISS);
        }
        field++;
    }
}

void drawGamefieldUpdate(uint8_t quadrant, uint8_t *gamefield, uint8_t attackPos, uint8_t anim)
{
    static uint8_t c, x, y;

    x = fieldLeft(quadrant) + (attackPos % 10) * 2;
    y = quadrantY[quadrant] + attackPos / 10;

    if (anim > 9)
    {
        putPair(x, y, ATTACK + (anim - 10) * 2);
        return;
    }

    c = gamefield[attackPos];
    if (c == FIELD_ATTACK)
        putPair(x, y, anim ? HIT2 : HIT);
    else if (c == FIELD_MISS)
        putPair(x, y, MISS);
}

void drawGamefieldCursor(uint8_t quadrant, uint8_t x, uint8_t y, uint8_t *gamefield, uint8_t blink)
{
    static uint8_t c, sx, sy;
    static const uint8_t *p;

    sx = fieldLeft(quadrant) + x * 2;
    sy = quadrantY[quadrant] + y;

    c = gamefield[y * 10 + x];
    p = c == FIELD_ATTACK ? HIT : c == FIELD_MISS ? MISS : WATER;

    if (!blink)
    {
        putPair(sx, sy, p);
    }
    else if (blink == 2 && !c)
    {
        // Open water pulses: block, box, nothing
        put(sx, sy, '[');
        put(sx + 1, sy, ']');
    }
    else
    {
        // The cell turned over
        put(sx, sy, p[0] ^ REV);
        put(sx + 1, sy, p[1] ^ REV);
    }
}

void drawEndgameMessage(const char *message)
{
    static uint8_t x;

    x = (WIDTH - (uint8_t)strlen(message)) / 2;

    fill(0, HEIGHT - 2, WIDTH, G_HLINE);
    fill(0, HEIGHT - 1, WIDTH, ' ');
    drawText(x, HEIGHT - 1, message);
    // The server's prompts are all lower case
    if (message[0] >= 'a' && message[0] <= 'z')
        put(x, HEIGHT - 1, message[0] - 32);
}

#endif /* __TRS80M2__ */
