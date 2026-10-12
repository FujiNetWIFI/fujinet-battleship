#ifdef __H89__

/**
 * @brief   Input - Heathkit H89 (H19 keyboard)
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 *
 * The H19 sends its keypad and function keys as escape sequences: with the
 * keypad shifted (termInit sends ESC t) the arrows are ESC A-D, f1-f5 are
 * ESC S-W, and BLUE/RED/GRAY are ESC P-R. Those arrive back to back at 9600
 * baud, so an ESC with nothing behind it within a few ticks is the ESC key
 * itself. Both OSes buffer the keyboard on its interrupt (HDOS's console
 * driver; the CP/M BIOS's 40-byte RST 3 buffer), so nothing is lost between
 * polls.
 */

#include "../misc.h"
#include "console.h"

#define ESC_WAIT_TICKS 10 // 20 ms

// Decoded keys not yet returned
static uint8_t pending[4];
static uint8_t pendingCount;

static void push(uint8_t k)
{
    if (pendingCount < sizeof(pending))
        pending[pendingCount++] = k;
}

/// @brief Plain keys: DELETE erases like BACKSPACE, LINE FEED enters
static uint8_t plain(uint8_t c)
{
    if (c == 0x7F)
        return KEY_BACKSPACE;
    if (c == 0x0A)
        return KEY_RETURN;
    return c;
}

/// @brief Wait briefly for the byte after an ESC; -1 if none comes
static int escFollower(void)
{
    static uint16_t t;
    static int c;

    t = TICKS;
    do
    {
        c = conIn();
        if (c >= 0)
            return c;
    } while ((uint16_t)(TICKS - t) < ESC_WAIT_TICKS);

    return -1;
}

static void decode(uint8_t c)
{
    static int n;

    if (c != KEY_ESCAPE)
    {
        push(plain(c));
        return;
    }

    n = escFollower();
    switch (n)
    {
    case 'A':
        push(KEY_UP_ARROW);
        break;
    case 'B':
        push(KEY_DOWN_ARROW);
        break;
    case 'C':
        push(KEY_RIGHT_ARROW);
        break;
    case 'D':
        push(KEY_LEFT_ARROW);
        break;
    case 'P': // BLUE
    case 'Q': // RED
    case 'R': // GRAY
    case 'S': // f1
    case 'T': // f2
    case 'U': // f3
    case 'V': // f4
    case 'W': // f5
        // The function keys open the menu too
        push(KEY_ESCAPE);
        break;
    case 'H': // HOME
    case 'J': // ERASE
    case 'E': // SHIFT ERASE
    case 'L': // IL
    case 'M': // DL
    case 'N': // DC
    case '@': // IC
        // The keypad's other keys do nothing here
        break;
    default:
        // A lone ESC, or ESC then an ordinary key
        push(KEY_ESCAPE);
        if (n >= 0 && n != KEY_ESCAPE)
            push(plain((uint8_t)n));
        else if (n == KEY_ESCAPE)
            push(KEY_ESCAPE);
        break;
    }
}

unsigned char kbhit(void)
{
    static int c;

    while (!pendingCount)
    {
        c = conIn();
        if (c < 0)
            return 0;
        decode((uint8_t)c);
    }
    return 1;
}

unsigned char cgetc(void)
{
    static uint8_t k;

    while (!kbhit())
        ;

    k = pending[0];
    pendingCount--;
    memmove(pending, pending + 1, pendingCount);
    return k;
}

uint8_t readJoystick(void)
{
    // Keyboard only: the arrows and RETURN/SPACE come through cgetc()
    return 0;
}

#endif /* __H89__ */
