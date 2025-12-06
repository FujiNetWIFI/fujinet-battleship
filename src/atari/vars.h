#ifndef KEYMAP_H
#define KEYMAP_H

// Screen dimensions for platform

#define WIDTH 40
#define HEIGHT 26

// Other platform specific constnats

#define ROLL_SOUND_MOD 4      // How often to play roll sound
#define ROLL_FRAMES 31        // How many roll frames to play
#define SCORE_CURSOR_ALT 0x80 // Alternate score cursor color
#define BOTTOM_HEIGHT 4       // How high the bottom panel is
#define SCORES_X 11           // X start of scoreboard
#define GAMEOVER_PROMPT_Y HEIGHT - 3
#define QUERY_SUFFIX "" // No extra params for Atari
#define ROLL_X WIDTH - 25
#define TIMER_X 12
#define TIMER_NUM_OFFSET_X 0
#define TIMER_NUM_OFFSET_Y 0

// Icons
#define ICON_TEXT_CURSOR 0x3A
#define ICON_MARK 0x2B
#define ICON_MARK_ALT 0
#define ICON_PLAYER 0x2A
#define ICON_SPEC 0xDC
#define ICON_CURSOR 0xBE
#define ICON_CURSOR_ALT 0xBF
#define ICON_CURSOR_BLIP 0x3E

/**
 * Platform specific key map for common input
 */

#define KEY_LEFT_ARROW CH_CURS_LEFT
#define KEY_LEFT_ARROW_2 43 // +
#define KEY_LEFT_ARROW_3 60 // <

#define KEY_RIGHT_ARROW CH_CURS_RIGHT
#define KEY_RIGHT_ARROW_2 42 // *
#define KEY_RIGHT_ARROW_3 62 // >

#define KEY_UP_ARROW CH_CURS_UP
#define KEY_UP_ARROW_2 45 // -
#define KEY_UP_ARROW_3 2  // DUMMY

#define KEY_DOWN_ARROW CH_CURS_DOWN
#define KEY_DOWN_ARROW_2 61 // =
#define KEY_DOWN_ARROW_3 3  // DUMMY

// #define KEY_RETURN       0x0D
#define KEY_ESCAPE CH_ESC
#define KEY_ESCAPE_ALT 1

#define KEY_SPACEBAR 0x20 // cc65's atari.h KEY_SPACE is wrong
#define KEY_BACKSPACE CH_DEL

#undef KEY_RETURN
#define KEY_RETURN CH_ENTER

#endif /* KEYMAP_H */