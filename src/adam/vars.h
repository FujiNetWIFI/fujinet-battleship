#ifndef KEYMAP_H
#define KEYMAP_H

// Screen dimensions for platform (TMS9918 mode 2 used as a 32x24 char map)

#define WIDTH 32
#define HEIGHT 24

// Other platform specific constants

#define GAMEOVER_PROMPT_Y HEIGHT - 2

// Icons (indices into the CoCo3-derived charset, see support/adam/make_charset.py)
#define ICON_TEXT_CURSOR 0x3A
#define ICON_PLAYER 0x2A
#define ICON_MARK 0x2B
#define ICON_MARK_ALT 0x20
#define ICON_ACTIVE_PLAYER 0x5B

/**
 * Platform specific key map for common input (Adam keyboard via EOS)
 */

#define KEY_LEFT_ARROW 0xA3
#define KEY_LEFT_ARROW_2 0x9D
#define KEY_LEFT_ARROW_3 0x2C // ,

#define KEY_RIGHT_ARROW 0xA1
#define KEY_RIGHT_ARROW_2 0x1D
#define KEY_RIGHT_ARROW_3 0x2E // .

#define KEY_UP_ARROW 0xA0
#define KEY_UP_ARROW_2 0x91
#define KEY_UP_ARROW_3 0x2D // -

#define KEY_DOWN_ARROW 0xA2
#define KEY_DOWN_ARROW_2 0x11
#define KEY_DOWN_ARROW_3 0x3D // =

#define KEY_RETURN 0x0D

#define KEY_ESCAPE 0x1B
#define KEY_ESCAPE_ALT 0x03

#define KEY_SPACEBAR 0x20
#define KEY_BACKSPACE 0x08

/* Macros that evaluate the return code of readJoystick */
#define JOY_UP(v) ((v) & 1)
#define JOY_DOWN(v) ((v) & 2)
#define JOY_LEFT(v) ((v) & 4)
#define JOY_RIGHT(v) ((v) & 8)
#define JOY_BTN_1(v) ((v) & 16) /* Universally available */
#define JOY_BTN_2(v) ((v) & 32) /* Second button if available */

#endif /* KEYMAP_H */
