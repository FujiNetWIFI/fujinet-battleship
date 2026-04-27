#include <string.h>
#include "../platform-specific/graphics.h"
#include "hires.h"
#include "text.h"
#include "vars.h"

void drawTextAt(unsigned char x, unsigned char y, const char *s) {
    static unsigned char c;
    while (*s) {
        c = *s++;
        if (c >= 97 && c <= 122) {
            c = c - 32;
        }
        hires_putc(x++, y, ROP_CPY, c);
    }
}

void drawTextAltAt(uint8_t x, uint8_t y, const char *s) {
    if (y == HEIGHT - 1) {
        uint8_t len = (uint8_t)strlen(s);

        if (x >= WIDTH - 5) {
            drawTextAt(x, y * 8, s);
        } else {
            if (x > 0) {
                drawSpace(0, y, x);
            }
            drawTextAt(x, y * 8, s);
            if (x + len < WIDTH) {
                drawSpace(x + len, y, WIDTH - x - len);
            }
        }
    } else {
        drawTextAt(x, y * 8 - 4, s);
    }
}
