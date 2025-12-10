#include <peekpoke.h>

// The hardware reports 0 for pressed bits, so we invert and map to our JOY_* macros.
unsigned char readJoystick()
{
    unsigned char reg;
    unsigned char value = 0;

    // Try port 1 ($DC00)
    reg = PEEK(0xDC00);
    if (reg != 0xFF) {
        if (!(reg & 1)) value |= 1;   // up
        if (!(reg & 2)) value |= 2;   // down
        if (!(reg & 4)) value |= 4;   // left
        if (!(reg & 8)) value |= 8;   // right
        if (!(reg & 16)) value |= 16; // fire/button
    }

    if (!value) {
        // Try port 2 ($DC01)
        reg = PEEK(0xDC01);
        if (reg != 0xFF) {
            if (!(reg & 1)) value |= 1;   // up
            if (!(reg & 2)) value |= 2;   // down
            if (!(reg & 4)) value |= 4;   // left
            if (!(reg & 8)) value |= 8;   // right
            if (!(reg & 16)) value |= 16; // fire/button
        }
    }

    return value;
}
