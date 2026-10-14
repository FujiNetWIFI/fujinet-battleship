/*
  Input handling - Coleco Adam

  Keyboard comes over AdamNet via EOS. z88dk's getk() is a dead stub on
  this target, so kbhit()/cgetc() are built on the asynchronous EOS
  keyboard read (eos_start_read_keyboard / eos_end_read_keyboard, which
  returns the key once it is > 1).
*/

#include "../misc.h"
#include <eos.h>

static GameControllerData cont;
static uint8_t pendingKey = 0;
static uint8_t kbdStarted = 0;

uint8_t kbhit(void)
{
    static uint8_t k;

    if (!kbdStarted)
    {
        eos_start_read_keyboard();
        kbdStarted = 1;
    }

    if (pendingKey)
        return 1;

    k = eos_end_read_keyboard();
    if (k > 1)
    {
        pendingKey = k;
        eos_start_read_keyboard();
        return 1;
    }
    return 0;
}

// Unsigned so the Adam's high-bit key codes (arrows 0xA0-0xA3, SmartKeys)
// zero-extend into input.key - sccz80 chars are signed
unsigned char cgetc(void)
{
    static uint8_t k;

    while (!kbhit())
        ;
    k = pendingKey;
    pendingKey = 0;
    return k;
}

uint8_t readJoystick(void)
{
    static uint8_t raw, value;

    eos_read_game_controller(0x03, &cont);

    // Raw AdamNet direction bits: 1=up 2=right 4=down 8=left
    raw = cont.joystick1 | cont.joystick2;

    value = 0;
    if (raw & 1)
        value |= 1; // up
    if (raw & 4)
        value |= 2; // down
    if (raw & 8)
        value |= 4; // left
    if (raw & 2)
        value |= 8; // right

    if (cont.joystick1_button_left || cont.joystick2_button_left)
        value |= 16;
    if (cont.joystick1_button_right || cont.joystick2_button_right)
        value |= 32;

    return value;
}
