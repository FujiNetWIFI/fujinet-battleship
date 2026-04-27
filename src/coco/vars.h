#ifndef KEYMAP_H
#define KEYMAP_H

// #define COCO3

// Screen/graphics defines
#ifdef COCO3
#define WIDTH 40
#define HEIGHT 25
#define CHAR_SHIFT << 5 // 4bpp characters
#define SCREEN 0x8000U
#define ROP_TYPE uint16_t
#define BPC 4 // Bytes per character
#else
#define WIDTH 32
#define HEIGHT 24
#define CHAR_SHIFT << 3 // 2bpp characters
#define SCREEN 0x6000U
#define ROP_TYPE uint8_t
#define BPC 1 // Bytes per character
#endif

// Other platform specific constnats

#define GAMEOVER_PROMPT_Y HEIGHT - 2

#undef ESCAPE
#define ESCAPE "BREAK"
#undef ESC
#define ESC "BRK"

// Icons
#define ICON_TEXT_CURSOR 0x3A
#define ICON_PLAYER 0x2A
#define ICON_MARK 0x2B
#define ICON_MARK_ALT 0x20
#define ICON_ACTIVE_PLAYER 0x05

/**
 * Platform specific key map for common input
 */

#define KEY_LEFT_ARROW 0x08
#define KEY_LEFT_ARROW_2 0xF1
#define KEY_LEFT_ARROW_3 0xF2 // ,

#define KEY_RIGHT_ARROW 0x09
#define KEY_RIGHT_ARROW_2 0xF3
#define KEY_RIGHT_ARROW_3 0xF4 // .

#define KEY_UP_ARROW 0x5E
#define KEY_UP_ARROW_2 0xF5
#define KEY_UP_ARROW_3 0xF6 // -

#define KEY_DOWN_ARROW 0x0A
#define KEY_DOWN_ARROW_2 0xF7
#define KEY_DOWN_ARROW_3 0xF9 // =

#define KEY_RETURN 0x0D

#define KEY_ESCAPE 0x03
#define KEY_ESCAPE_ALT 0x1B

#define KEY_SPACEBAR 0x20
#define KEY_BACKSPACE 0x7F

#define CHAR_CURSOR 0x9F

/* Macros that evaluate the return code of joy_read */
#define JOY_UP(v) ((v) & 1)
#define JOY_DOWN(v) ((v) & 2)
#define JOY_LEFT(v) ((v) & 4)
#define JOY_RIGHT(v) ((v) & 8)
#define JOY_BTN_1(v) ((v) & 16) /* Universally available */
#define JOY_BTN_2(v) ((v) & 32) /* Second button if available */

#endif /* KEYMAP_H */