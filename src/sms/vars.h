#ifdef BUILD_SMS

#ifndef KEYMAP_H
#define KEYMAP_H

#include "joystick.h"

// Screen dimensions for platform. Mode 4 shows exactly 32x24 tiles, the grid
// the Adam, ColecoVision and NES ports share.
#define WIDTH 32
#define HEIGHT 24

#define GAMEOVER_PROMPT_Y HEIGHT - 2

// Icons - CoCo 3 sheet indices (src/sms/mktiles.py keeps the sheet's numbering)
#define ICON_TEXT_CURSOR 0x3A
#define ICON_PLAYER 0x2A
#define ICON_MARK 0x2B
#define ICON_MARK_ALT 0x20
#define ICON_ACTIVE_PLAYER 0x5B

/* readJoystick() (src/sms/input.c) returns the d-pad only, in joystick.h's
   bits. The buttons never appear in the joystick byte: input.c reports them as
   key codes behind kbhit()/cgetc(), with one edge detector, the way the NES
   and ColecoVision ports report theirs. Button 1 is KEY_RETURN, which the
   shared input maps to the trigger. */
#undef JOY_BTN_1
#undef JOY_BTN_2
#define JOY_BTN_1(v) ((v) & 0)
#define JOY_BTN_2(v) ((v) & 0)

/**
 * Platform specific key map for common input
 */

// There is no keyboard: a d-pad, buttons 1 and 2, and Pause on the console.
// Direction comes from the d-pad, never from a key, so the arrow codes are
// deliberately unreachable placeholders -- they exist because
// readCommonInput() switches on all twelve.
#define KEY_LEFT_ARROW      0xF1
#define KEY_LEFT_ARROW_2    0xF2
#define KEY_LEFT_ARROW_3    0xF3

#define KEY_RIGHT_ARROW     0xF4
#define KEY_RIGHT_ARROW_2   0xF5
#define KEY_RIGHT_ARROW_3   0xF6

#define KEY_UP_ARROW        0xF7
#define KEY_UP_ARROW_2      0xF8
#define KEY_UP_ARROW_3      0xF9

#define KEY_DOWN_ARROW      0xFA
#define KEY_DOWN_ARROW_2    0xFB
#define KEY_DOWN_ARROW_3    0xFC

#define KEY_RETURN 0x0D
#define KEY_ESCAPE 0x1B
#define KEY_ESCAPE_ALT 0x03
#define KEY_SPACEBAR 0x20
#define KEY_BACKSPACE 0x08

// What the shared screens call the escape key
#define ESCAPE "PAUSE"
#define ESC "PAUSE"

// Countdown can reach two digits
#define TIMER_WIDTH 2

// Name entry happens on src/sms/osk.c's on-screen keyboard, not
// inputFieldCycle() -- there are no letter keys to type into it.
#define USE_PLATFORM_NAME_ENTRY 1

// The buttons stand in for the letter shortcuts the other ports type, with
// button 2 held as a shift the way the NES holds SELECT. src/sms/input.c maps
// them; keep the two in step:
//   1            RETURN   join / fire / place / ready
//   2            'r'      refresh the table list, rotate a ship (on release)
//   Pause        ESCAPE   the in-game menu
//   2+1          'q'      quit (the 5 Card Stud SMS chord)
//   2+Up         'h'      how to play
//   2+Down       's'      sound on/off
//   2+Left/Right 'n'      change name
#define TABLE_STATUS_TEXT "2:Refresh 2+Up:Help 2+Lt:Name"
#define MENU_QUIT_TEXT "  2+1: quit game"
#define MENU_HELP_TEXT "  2+UP: how to play"
#define MENU_SOUND_ON_TEXT "  2+DN: sound ON "
#define MENU_SOUND_OFF_TEXT "  2+DN: sound OFF"
#define ROTATE_PROMPT_TEXT "press 2 to rotate"
#define READY_PROMPT_TEXT "press 1 when ready"
#define MENU_CLOSE_TEXT "press 1 to close"

#endif /* KEYMAP_H */

#endif /* BUILD_SMS */
