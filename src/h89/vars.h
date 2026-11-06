#ifdef __H89__

#ifndef KEYMAP_H
#define KEYMAP_H

// Screen dimensions for platform: the H19 terminal's 80x24 (its 25th line is
// left alone). Every board cell is two characters wide (src/h89/graphics.c).
#define WIDTH 80
#define HEIGHT 24

#define GAMEOVER_PROMPT_Y HEIGHT - 2

/*
  Icons are screen cell codes (src/h89/console.h): bit 7 is reverse video and
  the low seven bits are the H19's own character ROM index -- 0x00-0x1F the
  graphics characters, 0x7F the graphics bullet, 0x20-0x7E ASCII.
*/
#define ICON_TEXT_CURSOR 0xA0 // reverse space: a block cursor
#define ICON_PLAYER 0x7F      // bullet
#define ICON_MARK 0x08        // right arrow
#define ICON_MARK_ALT 0x20
#define ICON_ACTIVE_PLAYER 0x7F

/**
 * Platform specific key map for common input (H19 keyboard)
 */

// src/h89/input.c turns the keypad's ESC A-D (keypad shifted mode, set by
// termInit) into 0xA0-0xA3. The _2 codes are unreachable placeholders --
// readCommonInput() switches on all twelve -- and the _3 keys are the Adam's
// spares for a keyboard without the keypad.
#define KEY_UP_ARROW        0xA0
#define KEY_UP_ARROW_2      0xF1
#define KEY_UP_ARROW_3      0x2D // -

#define KEY_DOWN_ARROW      0xA1
#define KEY_DOWN_ARROW_2    0xF2
#define KEY_DOWN_ARROW_3    0x3D // =

#define KEY_RIGHT_ARROW     0xA2
#define KEY_RIGHT_ARROW_2   0xF3
#define KEY_RIGHT_ARROW_3   0x2E // .

#define KEY_LEFT_ARROW      0xA3
#define KEY_LEFT_ARROW_2    0xF4
#define KEY_LEFT_ARROW_3    0x2C // ,

#define KEY_RETURN 0x0D
#define KEY_ESCAPE 0x1B
#define KEY_ESCAPE_ALT 0x03 // CTRL-C
#define KEY_SPACEBAR 0x20
#define KEY_BACKSPACE 0x08

/* Keyboard only: readJoystick() always returns 0 */
#define JOY_UP(v) ((v) & 1)
#define JOY_DOWN(v) ((v) & 2)
#define JOY_LEFT(v) ((v) & 4)
#define JOY_RIGHT(v) ((v) & 8)
#define JOY_BTN_1(v) ((v) & 0)
#define JOY_BTN_2(v) ((v) & 0)

// Countdown can reach two digits
#define TIMER_WIDTH 2

// What the shared screens call the keys: the H19's is labelled ESC, and its
// f1-f5 and colour keys open the menu too (src/h89/input.c)
#define ESCAPE "ESC"
#define ESC "ESC"
#define READY_PROMPT_TEXT "press SPACE when ready"
#define MENU_CLOSE_TEXT "press SPACE to close"

#endif /* KEYMAP_H */

#endif /* __H89__ */
