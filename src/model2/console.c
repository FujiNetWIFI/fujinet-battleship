#ifdef __TRS80M2__

/*
  buf is what is drawn, scr what the screen shows; flush() sends only the
  cells that differ, through the BIOS's CONOUT (the BDOS console calls expand
  tabs and act on ^S and ^C). A whole screen takes about half a second.

  P&T's control codes: ESC Y row+32 col+32 places the cursor, ^L clears and
  homes, ^A erases to the end of the line, ^N/^O turn reverse on and off,
  ^Q/^T graphics, in which graphics character n is sent as 40h+n.

  src/h89/console.c sends each cell as it is set; here they wait for flush().
*/

#include <string.h>
#include "console.h"

#define CTL_CLEAR_EOL 0x01
#define CTL_CLEAR     0x0C
#define CTL_REV_ON    0x0E
#define CTL_REV_OFF   0x0F
#define CTL_GFX_ON    0x11
#define CTL_GFX_OFF   0x14
#define CTL_CURS_OFF  0x19
#define CTL_CURS_ON   0x1A
#define CTL_ESC       0x1B

#define CUR_UNKNOWN 0xFF

// Re-send up to this many unchanged cells rather than address past them
#define GAP_MAX 3

// Erase the rest of a row once at least this many cells there would
// otherwise have to be sent as spaces
#define EOL_MIN 4

// Clear the screen once at least this many cells are to be blanked
#define CLEAR_MIN 600

// Printing the bottom right cell scrolls the screen, so it goes straight
// into video memory
#define CORNER (SCR_W * SCR_H - 1)

static uint8_t buf[SCR_W * SCR_H];
static uint8_t scr[SCR_W * SCR_H];
static uint8_t saved[SCR_W * SCR_H];
static uint8_t dirty[SCR_H];
static uint8_t changed[SCR_H];

// sccz80 multiplies through a library call
static const uint16_t rowOff[SCR_H] = {
    0 * SCR_W, 1 * SCR_W, 2 * SCR_W, 3 * SCR_W, 4 * SCR_W,
    5 * SCR_W, 6 * SCR_W, 7 * SCR_W, 8 * SCR_W, 9 * SCR_W,
    10 * SCR_W, 11 * SCR_W, 12 * SCR_W, 13 * SCR_W, 14 * SCR_W,
    15 * SCR_W, 16 * SCR_W, 17 * SCR_W, 18 * SCR_W, 19 * SCR_W,
    20 * SCR_W, 21 * SCR_W, 22 * SCR_W, 23 * SCR_W,
};

// The driver's cursor and modes; CUR_UNKNOWN makes the next cell set them
static uint8_t curX, curY, curRev, curGfx;

/*
 * BIOS and P&T special functions. P&T's BIOS keeps IX, IY and the
 * alternate registers.
 */

// (0001) points at WBOOT, base+3: CONST is +3 from there, CONIN +6, CONOUT +9
void conOut(uint8_t c) __z88dk_fastcall __naked
{
#asm
    EXTERN  l_dcal
    ld      c,l
    ld      hl,(1)
    ld      de,9
    add     hl,de
    jp      l_dcal      ; CONOUT
#endasm
}

int conIn(void) __naked
{
#asm
    EXTERN  l_dcal
    ld      hl,(1)
    ld      de,3
    add     hl,de
    call    l_dcal      ; CONST: A = FF when a key is waiting
    or      a
    ld      hl,-1
    ret     z
    ld      hl,(1)
    ld      de,6
    add     hl,de
    call    l_dcal      ; CONIN
    ld      l,a
    ld      h,0
    ret
#endasm
}

// Special function 14: DE = low word, HL = high word
uint32_t ticks(void) __naked
{
#asm
    ld      b,14
    call    40h
    ex      de,hl
    ret
#endasm
}

static void special(uint8_t n) __z88dk_fastcall __naked
{
#asm
    ld      b,l
    jp      40h
#endasm
}

// Video memory is at F800h between special functions 21 and 22, with no
// system calls in between
static void vramPut(uint16_t off, uint8_t c)
{
    special(21);
    ((volatile uint8_t *)0xF800)[off] = c;
    special(22);
}

/*
 * Sending
 */

static void forget(void)
{
    curX = curY = curRev = curGfx = CUR_UNKNOWN;
}

static void moveTo(uint8_t x, uint8_t y)
{
    if (x != curX || y != curY)
    {
        conOut(CTL_ESC);
        conOut('Y');
        conOut(32 + y);
        conOut(32 + x);
        curX = x;
        curY = y;
    }
}

static uint8_t isGfx(uint8_t code)
{
    return (code & 0x7F) < 0x20;
}

static void revOff(void)
{
    if (curRev)
    {
        curRev = 0;
        conOut(CTL_REV_OFF);
    }
}

static void sendCell(uint8_t code)
{
    static uint8_t g;

    if ((code & REV) != curRev)
    {
        curRev = code & REV;
        conOut(curRev ? CTL_REV_ON : CTL_REV_OFF);
    }

    g = isGfx(code);
    if (g != curGfx)
    {
        curGfx = g;
        conOut(g ? CTL_GFX_ON : CTL_GFX_OFF);
    }

    code &= 0x7F;
    conOut(g ? 0x40 + code : code);

    // The driver wraps past the last column
    curX = (curX == SCR_W - 1) ? CUR_UNKNOWN : curX + 1;
}

/// @brief Can the cells from the cursor up to x be re-sent with no mode
/// change? They already show on screen, so this only moves the cursor.
static bool cheapGap(const uint8_t *row, uint8_t x)
{
    static uint8_t cx;

    if (curX == CUR_UNKNOWN || curX >= x || x - curX > GAP_MAX)
        return false;

    for (cx = curX; cx < x; cx++)
        if ((row[cx] & REV) != curRev || isGfx(row[cx]) != curGfx)
            return false;
    return true;
}

static void clearScreen(void)
{
    revOff();
    conOut(CTL_CLEAR);
    conOut(CTL_GFX_OFF);
    curGfx = 0;
    curX = curY = 0;
    memset(scr, ' ', sizeof(scr));
}

static void flushRow(uint8_t y)
{
    static uint8_t *b, *s;
    static uint8_t x, last, stale, end;

    b = buf + rowOff[y];
    s = scr + rowOff[y];

    end = SCR_W;
    if (y == SCR_H - 1)
    {
        end = SCR_W - 1;
        if (b[end] != s[end])
        {
            vramPut(CORNER, b[end]);
            s[end] = b[end];
        }
    }

    // The row's last non-blank cell, and how many cells past it still show
    // something. ^A is not used on the bottom row, where it would also erase
    // the corner cell.
    last = end;
    while (last && b[last - 1] == ' ')
        last--;
    stale = 0;
    for (x = last; x < end; x++)
        if (s[x] != ' ')
            stale++;
    if (stale < EOL_MIN || end < SCR_W)
        last = end;

    for (x = 0; x < last; x++)
    {
        if (b[x] == s[x])
            continue;

        if (y == curY && cheapGap(b, x))
        {
            while (curX < x)
                sendCell(b[curX]);
        }
        else
            moveTo(x, y);

        sendCell(b[x]);
        s[x] = b[x];
    }

    if (last < end)
    {
        moveTo(last, y);
        revOff();
        conOut(CTL_CLEAR_EOL);
        memset(s + last, ' ', SCR_W - last);
    }
}

/// @brief Cells of the changed rows that are to go blank
static uint16_t blanksToSend(void)
{
    static uint8_t y, n;
    static uint8_t *b, *s;
    static uint16_t blanks;

    blanks = 0;
    for (y = 0; y < SCR_H; y++)
    {
        if (!changed[y])
            continue;
        b = buf + rowOff[y];
        s = scr + rowOff[y];
        for (n = SCR_W; n; n--)
        {
            if (*b == ' ' && *s != ' ')
                blanks++;
            b++;
            s++;
        }
    }
    return blanks;
}

void flush(void)
{
    static uint8_t y, rows;

    // Most dirty rows were redrawn the same
    rows = 0;
    for (y = 0; y < SCR_H; y++)
    {
        changed[y] = dirty[y] && memcmp(buf + rowOff[y], scr + rowOff[y], SCR_W);
        dirty[y] = 0;
        rows += changed[y];
    }
    if (!rows)
        return;

    if (rows >= SCR_H / 2 && blanksToSend() >= CLEAR_MIN)
    {
        clearScreen();
        for (y = 0; y < SCR_H; y++)
            changed[y] = memcmp(buf + rowOff[y], scr + rowOff[y], SCR_W) != 0;
    }

    for (y = 0; y < SCR_H; y++)
        if (changed[y])
            flushRow(y);
}

/*
 * Screen
 */

void termInit(void)
{
    conOut(CTL_GFX_OFF);
    conOut(CTL_REV_OFF);
    conOut(CTL_CURS_OFF);
    forget();
    curRev = curGfx = 0;
    clearScreen();
    memset(buf, ' ', sizeof(buf));
    memset(dirty, 0, sizeof(dirty));
}

void termExit(void)
{
    conOut(CTL_GFX_OFF);
    conOut(CTL_REV_OFF);
    conOut(CTL_CLEAR);
    conOut(CTL_CURS_ON);
}

void put(uint8_t x, uint8_t y, uint8_t code)
{
    if (x >= SCR_W || y >= SCR_H)
        return;

    buf[rowOff[y] + x] = code;
    dirty[y] = 1;
}

void fill(uint8_t x, uint8_t y, uint8_t w, uint8_t code)
{
    if (x >= SCR_W || y >= SCR_H)
        return;
    if (w > SCR_W - x)
        w = SCR_W - x;

    memset(buf + rowOff[y] + x, code, w);
    dirty[y] = 1;
}

void termClear(void)
{
    memset(buf, ' ', sizeof(buf));
    memset(dirty, 1, sizeof(dirty));
}

void termSave(void)
{
    memcpy(saved, buf, sizeof(buf));
}

void termRestore(void)
{
    // Every row, as the shared code may have drawn anywhere since
    memcpy(buf, saved, sizeof(buf));
    memset(dirty, 1, sizeof(dirty));
}

#endif /* __TRS80M2__ */
