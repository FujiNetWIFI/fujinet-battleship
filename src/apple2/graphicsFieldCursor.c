#include <stdint.h>
#include "vars.h"

uint8_t fieldCursorCode(uint8_t baseX, uint8_t x, uint8_t c, uint8_t blink) {
    uint8_t charCode;

    if (blink) {
        /* Cursor display: character codes for blink animation */
        if ((baseX + x) % 2) {
            /* Odd x coordinates */
            if (c == 0) {
                charCode = blink == 1 ? EMPTY_CURSOR_BLINK1_ODD : EMPTY_CURSOR_BLINK2_ODD;
            } else if (c == 1) {
                charCode = blink == 1 ? HIT_CURSOR_BLINK1_ODD : HIT_CURSOR_BLINK2_ODD;
            } else { /* c == 2 */
                charCode = blink == 1 ? MISS_CURSOR_BLINK1_ODD : MISS_CURSOR_BLINK2_ODD;
            }
        } else {
            /* Even x coordinates */
            if (c == 0) {
                charCode = blink == 1 ? EMPTY_CURSOR_BLINK1_EVEN : EMPTY_CURSOR_BLINK2_EVEN;
            } else if (c == 1) {
                charCode = blink == 1 ? HIT_CURSOR_BLINK1_EVEN : HIT_CURSOR_BLINK2_EVEN;
            } else { /* c == 2 */
                charCode = blink == 1 ? MISS_CURSOR_BLINK1_EVEN : MISS_CURSOR_BLINK2_EVEN;
            }
        }
    } else {
        /* Normal display: character code according to cell state */
        if ((baseX + x) % 2) {
            if (c == 0) {
                charCode = EMPTY_NORMAL_ODD;
            } else if (c == 1) {
                charCode = HIT_NORMAL_ODD;
            } else { /* c == 2 */
                charCode = MISS_NORMAL_ODD; /* 0x29 */
            }
        } else {
            if (c == 0) {
                charCode = EMPTY_NORMAL_EVEN;
            } else if (c == 1) {
                charCode = HIT_NORMAL_EVEN;
            } else { /* c == 2 */
                charCode = MISS_NORMAL_EVEN; /* 0x1a */
            }
        }
    }
    return charCode;
}
