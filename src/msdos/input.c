#include <dos.h>
#include "vars.h"
#include <stdint.h>

int cgetc(void)
{
    union REGS r;
    unsigned int k=0;

    // Check keyboard buffer, 0 = nothing.
    r.h.ah = 0x01;
    int86(0x16,&r,&r);

    // Nothing found, return 0.
    if (!r.x.ax)
        return 0;

    // Get key.
    r.h.ah = 0x00;
    int86(0x16,&r,&r);
    k=r.x.ax;

    switch(k)
    {
    case KEY_LEFT_ARROW:
    case KEY_RIGHT_ARROW:
    case KEY_UP_ARROW:
    case KEY_DOWN_ARROW:
        return r.x.ax;
    default:
        return r.h.al;
    }

    return r.h.al;
}

uint8_t readJoystick()
{
    // byte*  joy = *readJoystickPositions();
    // return joy[0];
    return 0;
}
