#ifdef __TRS80M2__

#ifndef KEYMAP_H
#define KEYMAP_H

// TRS-80 Model II under Pickles & Trout CP/M: 80x24. Every board cell is two
// characters wide (src/model2/graphics.c).
#define WIDTH 80
#define HEIGHT 24

#define GAMEOVER_PROMPT_Y HEIGHT - 2

// Icons: cell codes (src/model2/console.h), bit 7 reverse video
#define ICON_TEXT_CURSOR 0xA0 // reverse space
#define ICON_PLAYER '*'
#define ICON_MARK '>'
#define ICON_MARK_ALT ' '
#define ICON_ACTIVE_PLAYER '*'

// P&T CP/M key codes. F1 and F2 are spare up and down keys, BREAK is
// KEY_ESCAPE_ALT; the left and right _2 codes are unreachable placeholders.
#define KEY_LEFT_ARROW      0x1C
#define KEY_LEFT_ARROW_2    0xF1
#define KEY_LEFT_ARROW_3    0x2C // ,

#define KEY_RIGHT_ARROW     0x1D
#define KEY_RIGHT_ARROW_2   0xF2
#define KEY_RIGHT_ARROW_3   0x2E // .

#define KEY_UP_ARROW        0x1E
#define KEY_UP_ARROW_2      0x01 // F1
#define KEY_UP_ARROW_3      0x2D // -

#define KEY_DOWN_ARROW      0x1F
#define KEY_DOWN_ARROW_2    0x02 // F2
#define KEY_DOWN_ARROW_3    0x3D // =

#define KEY_RETURN 0x0D
#define KEY_ESCAPE 0x1B
#define KEY_ESCAPE_ALT 0x03 // BREAK
#define KEY_SPACEBAR 0x20
#define KEY_BACKSPACE 0x08

#define JOY_UP(v) ((v) & 0)
#define JOY_DOWN(v) ((v) & 0)
#define JOY_LEFT(v) ((v) & 0)
#define JOY_RIGHT(v) ((v) & 0)
#define JOY_BTN_1(v) ((v) & 0)
#define JOY_BTN_2(v) ((v) & 0)

// Countdown can reach two digits
#define TIMER_WIDTH 2

// Upper and lower case both show, so the shared screens use their TXT()
// mixed-case text and names keep their capitals. drawTextAlt() reverses a
// capital unless a lower case letter follows it.
#define MIXED_CASE

#define ESCAPE "ESC"
#define ESC "ESC"
#define READY_PROMPT_TEXT "Press SPACE when ready"
#define ROTATE_PROMPT_TEXT "Press R to rotate"
#define MENU_CLOSE_TEXT "Press SPACE to close"
#define TABLE_STATUS_TEXT "R)efresh   H)elp   N)ame   Q)uit"
#define MENU_QUIT_TEXT "  Q: Quit game"
#define MENU_HELP_TEXT "  H: How to play"

// The Model II has no speaker: the shared screens leave out the sound option
#define NO_SOUND

#endif /* KEYMAP_H */

#endif /* __TRS80M2__ */
