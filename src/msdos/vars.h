#ifndef KEYMAP_H
#define KEYMAP_H

/* Macros that evaluate the return code of joy_read */
#define JOY_UP(v) ((v) & 1)
#define JOY_DOWN(v) ((v) & 2)
#define JOY_LEFT(v) ((v) & 4)
#define JOY_RIGHT(v) ((v) & 8)
#define JOY_BTN_1(v) ((v) & 0) /* Universally available */
#define JOY_BTN_2(v) ((v) & 0) /* Second button if available */


// Screen dimensions for platform

#define WIDTH 40
#define HEIGHT 25

// Other platform specific constnats

#define GAMEOVER_PROMPT_Y HEIGHT - 2

// Icons
#define ICON_TEXT_CURSOR 0x3A
#define ICON_PLAYER 0x2A
#define ICON_MARK 0x2B
#define ICON_MARK_ALT 0x19
#define ICON_ACTIVE_PLAYER 0x05

/**
 * Platform specific key map for common input
 */

#define KEY_LEFT_ARROW      0x4B00
#define KEY_LEFT_ARROW_2    0x4B01
#define KEY_LEFT_ARROW_3    0x4B02

#define KEY_RIGHT_ARROW     0x4D00
#define KEY_RIGHT_ARROW_2   0x4D01
#define KEY_RIGHT_ARROW_3   0x4D02

#define KEY_UP_ARROW        0x4800
#define KEY_UP_ARROW_2      0x4801
#define KEY_UP_ARROW_3      0x4802

#define KEY_DOWN_ARROW      0x5000
#define KEY_DOWN_ARROW_2    0x5001
#define KEY_DOWN_ARROW_3    0x5002

#define KEY_RETURN 0x0D

#define KEY_ESCAPE 0x03
#define KEY_ESCAPE_ALT 0x1B

#define KEY_SPACEBAR 0x20
#define KEY_BACKSPACE 0x08

#define CHAR_CURSOR 0x9F

#endif /* KEYMAP_H */
