#ifdef __H89__

/**
 * @brief   H19 terminal screen - Heathkit H89
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 *
 * The console is the H19 on the H89's 8250 at 9600 baud -- under a thousand
 * characters a second -- so the screen is a shadow of what the terminal
 * shows, and only cells that change are sent. Console bytes go through the
 * OS raw: HDOS's .SCOUT/.SCIN with the console driver in raw character mode,
 * or the CP/M BIOS's CONOUT/CONST/CONIN straight from the jump table (the
 * BDOS console calls expand, filter and hold back characters).
 */

#include <string.h>
#include "console.h"

#define ESC 0x1B
#define CUR_UNKNOWN 0xFF

static uint8_t scr[SCR_W * SCR_H];
static uint8_t scrBak[SCR_W * SCR_H];

// What the terminal is doing now: cursor, reverse video, graphics mode.
// CUR_UNKNOWN forces the next cell to set it.
static uint8_t curX, curY, curRev, curGfx;

/*
 * Raw console I/O
 */

#ifdef __HDOS__

// HDOS system calls: RST 38H, then the function byte. HDOS is 8080 code and
// leaves IX and IY alone.
void conOut(uint8_t c) __z88dk_fastcall __naked
{
#asm
    ld      a,l
    rst     38h
    defb    2           ; .SCOUT
    ret
#endasm
}

int conIn(void) __naked
{
#asm
    rst     38h
    defb    1           ; .SCIN - carry set when nothing is waiting
    ld      hl,-1
    ret     c
    ld      l,a
    ld      h,0
    ret
#endasm
}

// .CONSL: A = index of the console byte, B = the bits to set, C = the mask of
// bits to change
uint8_t h89ConslIndex, h89ConslBits, h89ConslMask;

static void consl(void) __naked
{
#asm
    ld      a,(_h89ConslBits)
    ld      b,a
    ld      a,(_h89ConslMask)
    ld      c,a
    ld      a,(_h89ConslIndex)
    rst     38h
    defb    6           ; .CONSL
    ret
#endasm
}

static void conslSet(uint8_t index, uint8_t bits, uint8_t mask)
{
    h89ConslIndex = index;
    h89ConslBits = bits;
    h89ConslMask = mask;
    consl();
}

// The five console cells (S.CSLMD..S.CONFL) live at 040.326A in HDOS 2.0's
// system values (HOSDEF: S.VAL at 040.277A)
#define S_CSLMD ((volatile uint8_t *)0x20D6)
#define I_CSLMD 0
#define I_CONTY 1
#define I_CUSOR 2
#define I_CONWI 3
#define I_CONFL 4

#define CSL_ECH 0x80 // suppress echo
#define CSL_RAW 0x04 // raw mode I/O (no CTL-C/S/O/P processing)
#define CSL_WRP 0x02 // wrap lines at width
#define CSL_CHR 0x01 // character mode
#define CTP_MLI 0x20 // map lower case to upper on input
#define CTP_MLO 0x10 // map lower case to upper on output
#define CTP_BKM 0x02 // map backspace to rubout on input
#define CO_FLG 0x01  // CTL-O (discard output) flag
#define CS_FLG 0x80  // CTL-S (hold output) flag

static uint8_t savedConsole[5];

static void osInit(void)
{
    memcpy(savedConsole, (void *)S_CSLMD, sizeof(savedConsole));

    // Raw character mode with no echo, no wrap and no width (HDOS counts
    // escape sequences as printing columns), and no case mapping: the
    // graphics characters are sent as lower case letters.
    conslSet(I_CSLMD, CSL_ECH | CSL_RAW | CSL_CHR, CSL_ECH | CSL_RAW | CSL_WRP | CSL_CHR);
    conslSet(I_CONTY, 0, CTP_MLI | CTP_MLO | CTP_BKM);
    conslSet(I_CONWI, 255, 0xFF);
    conslSet(I_CONFL, 0, CO_FLG | CS_FLG);
}

static void osExit(void)
{
    conslSet(I_CSLMD, savedConsole[I_CSLMD], 0xFF);
    conslSet(I_CONTY, savedConsole[I_CONTY], 0xFF);
    conslSet(I_CONWI, savedConsole[I_CONWI], 0xFF);
    conslSet(I_CUSOR, 0, 0xFF); // ESC z homed the cursor
}

#else /* CP/M */

// The BIOS jump table: (0001) points at its WBOOT entry, base+3. CONST is at
// +3 from there, CONIN +6, CONOUT +9. The BIOS may use any register, so IX
// and IY are kept for the C side.
void conOut(uint8_t c) __z88dk_fastcall __naked
{
#asm
    EXTERN  l_dcal
    push    ix
    push    iy
    ld      c,l
    ld      hl,(1)
    ld      de,9
    add     hl,de
    call    l_dcal      ; CONOUT
    pop     iy
    pop     ix
    ret
#endasm
}

int conIn(void) __naked
{
#asm
    EXTERN  l_dcal
    push    ix
    push    iy
    ld      hl,(1)
    ld      de,3
    add     hl,de
    call    l_dcal      ; CONST: A = FF when a character is waiting
    or      a
    ld      hl,-1
    jr      z,conIn_done
    ld      hl,(1)
    ld      de,6
    add     hl,de
    call    l_dcal      ; CONIN
    ld      l,a
    ld      h,0
conIn_done:
    pop     iy
    pop     ix
    ret
#endasm
}

static void osInit(void)
{
}

static void osExit(void)
{
}

#endif /* __HDOS__ */

static void esc(uint8_t c)
{
    conOut(ESC);
    conOut(c);
}

static void forget(void)
{
    curX = curY = curRev = curGfx = CUR_UNKNOWN;
}

/*
 * Screen
 */

void termInit(void)
{
    osInit();

    esc('G');      // graphics mode off
    esc('q');      // reverse video off
    esc('x');      // cursor off
    conOut('5');
    esc('w');      // discard (no wrap) at the end of a line
    esc('t');      // keypad shifted: the unshifted keypad sends ESC A-D
    forget();
    termClear();
}

void termExit(void)
{
    static uint16_t t;

    // Back to the power-up configuration the DIP switches set: modes,
    // cursor, keypad, and a clear screen. The terminal needs a moment.
    esc('z');
    t = TICKS;
    while ((uint16_t)(TICKS - t) < 100)
        ;
    osExit();
}

void termClear(void)
{
    esc('E');
    memset(scr, ' ', sizeof(scr));
    curX = curY = 0;
}

void termBell(void)
{
    conOut(0x07);
}

/// @brief Send a cell to the terminal at x,y
static void emit(uint8_t x, uint8_t y, uint8_t code)
{
    static uint8_t g;

    if (x != curX || y != curY)
    {
        esc('Y');
        conOut(32 + y);
        conOut(32 + x);
    }

    if ((code & REV) != curRev)
    {
        curRev = code & REV;
        esc(curRev ? 'p' : 'q');
    }

    g = code & 0x7F;
    if (g < 0x20 || g == 0x7F)
    {
        // Graphics characters: ROM 0x00-0x1E are ` through ~, 0x1F is _,
        // the bullet is ^
        if (curGfx != 1)
        {
            curGfx = 1;
            esc('F');
        }
        if (g == 0x7F)
            g = '^';
        else if (g == 0x1F)
            g = '_';
        else
            g += 0x60;
    }
    else if (g >= 0x5E)
    {
        // Graphics mode only remaps 0x5E-0x7E: everything below prints the
        // same either way, so the mode is left alone for it
        if (curGfx != 0)
        {
            curGfx = 0;
            esc('G');
        }
    }

    conOut(g);

    // In discard-at-end-of-line mode the cursor stays on the last column
    curY = y;
    curX = (x == SCR_W - 1) ? CUR_UNKNOWN : x + 1;
}

void put(uint8_t x, uint8_t y, uint8_t code)
{
    static uint16_t off;

    if (x >= SCR_W || y >= SCR_H)
        return;

    off = (uint16_t)y * SCR_W + x;
    if (scr[off] == code)
        return;

    scr[off] = code;
    emit(x, y, code);
}

void fill(uint8_t x, uint8_t y, uint8_t w, uint8_t code)
{
    while (w--)
        put(x++, y, code);
}

void termSave(void)
{
    memcpy(scrBak, scr, sizeof(scr));
}

void termRestore(void)
{
    static uint8_t x, y;
    static uint8_t *p;

    p = scrBak;
    for (y = 0; y < SCR_H; y++)
        for (x = 0; x < SCR_W; x++)
            put(x, y, *p++);
}

#endif /* __H89__ */
