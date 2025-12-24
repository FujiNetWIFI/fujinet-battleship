/*
  Apple II Joystick - Prototype version
*/

#include <stdint.h>
//#include "../include/joystick.h"

// Redefine macros for Apple II
#undef JOY_UP
#undef JOY_DOWN
#undef JOY_LEFT
#undef JOY_RIGHT
#undef JOY_BTN_1
#undef JOY_BTN_2

#define JOY_BTN_1_MASK 0x10
#define JOY_BTN_2_MASK 0x20

#define JOY_LEFT_MASK  0x01
#define JOY_RIGHT_MASK 0x02
#define JOY_UP_MASK    0x04
#define JOY_DOWN_MASK  0x08

#define JOY_LEFT(v)  ((v) & JOY_LEFT_MASK)
#define JOY_RIGHT(v) ((v) & JOY_RIGHT_MASK)
#define JOY_UP(v)    ((v) & JOY_UP_MASK)
#define JOY_DOWN(v)  ((v) & JOY_DOWN_MASK)
#define JOY_BTN_1(v) ((v) & JOY_BTN_1_MASK)
#define JOY_BTN_2(v) ((v) & JOY_BTN_2_MASK)

uint8_t readJoystick(void)
{
    // Not implemented in prototype - return 0 (no input)
    return 0;
}

