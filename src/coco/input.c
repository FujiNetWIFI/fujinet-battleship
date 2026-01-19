#include <coco.h>
#include "fujinet-fuji.h"

#define JOY_CENTER   31
#define JOY_HALF     16

#define JOY_LOW_TH   (JOY_CENTER - JOY_HALF)   /* 15 */
#define JOY_HIGH_TH  (JOY_CENTER + JOY_HALF)   /* 47 */

static char lastKey = 0;
static bool right_joy_selected = 0;
static bool left_joy_selected = 0;

uint8_t kbhit(void)
{
    return (char)(lastKey || (lastKey = inkey()) || (lastKey = inkey()));
}

char cgetc(void)
{
    char key = lastKey;

    lastKey = 0;

    while (!key)
    {
        key = (char)inkey();
    }

    return key;
}

byte readJoystick(void)
{
    byte value = 0;
    bool lbtn1, lbtn2, rbtn1, rbtn2;
    byte h, v;

    byte buttons = readJoystickButtons();   /* active-high */

    /* NOTE: As of right now, the enum in the coco.h */
    /* header file is incorrect for the button values. */
    lbtn1  = (buttons & 0x02 ) == 0;
    lbtn2  = (buttons & 0x08 ) == 0;
    rbtn1  = (buttons & 0x01 ) == 0;
    rbtn2  = (buttons & 0x04 ) == 0;

    // Don't read joystick positions until one of the buttons is pressed.
    if (lbtn1 || lbtn2)
    {
        left_joy_selected = true;
        right_joy_selected = false;
    }
    else if (rbtn1 || rbtn2 )
    {
        right_joy_selected = true;
        left_joy_selected = false;

    }   

    if (left_joy_selected || right_joy_selected)
    {
        const byte *joy = readJoystickPositions();

        // Toggle back and forth between left and right joystick
        // depending on which one's buttons were last pressed.
        if (left_joy_selected)
        {
            h = joy[JOYSTK_LEFT_HORIZ];
            v = joy[JOYSTK_LEFT_VERT];
            if (lbtn1)
                value |= 16; /* bit 4 = button 1 */
            if (lbtn2)
                value |= 32; /* bit 5 = button 2 */
        }
        else /* right_joy_selected */
        {
            h = joy[JOYSTK_RIGHT_HORIZ];
            v = joy[JOYSTK_RIGHT_VERT];
            
            if (rbtn1)
                value |= 16; /* bit 4 = button 1 */
            if (rbtn2)
                value |= 32; /* bit 5 = button 2 */
        }

        /* Direction bits: UP, DOWN, LEFT, RIGHT
           Vertical: 0 = UP, 63 = DOWN */
        if (v <= JOY_LOW_TH)
            value |= 1; /* up */
        if (v >= JOY_HIGH_TH)
            value |= 2; /* down */
        if (h <= JOY_LOW_TH)
            value |= 4; /* left */
        if (h >= JOY_HIGH_TH)
            value |= 8; /* right */

    }

    return value;
}

