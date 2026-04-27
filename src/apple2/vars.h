#ifndef KEYMAP_H
#define KEYMAP_H

#include <joystick.h>

// Screen dimensions for platform
// from coco ver.

#define WIDTH 40
#define HEIGHT 24

// Other platform specific constnats
#define GAMEOVER_PROMPT_Y HEIGHT - 2
#define LOGO_Y 2

// Hires / board drawing (graphics.c, graphicsBoard.c)

// Checkerboard fill for gamefield (hires_Mask ROP_CONST)
#define EVEN_BLUE  0b11010101  // Pattern for even x coordinates
#define ODD_BLUE   0b10101010  // Pattern for odd x coordinates

// White outline OR-masks (single-column hires_Mask)
#define V_LINE_LEFT   0x07
#define V_LINE_RIGHT  0xf0

#undef ESCAPE
#define ESCAPE "BREAK"
#undef ESC
#define ESC "BRK"

// Icons
#define ICON_TEXT_CURSOR 0x3a
#define ICON_PLAYER 0x2a
#define ICON_MARK 0x2b
#define ICON_MARK_ALT 0x20
#define ICON_ACTIVE_PLAYER 0x05
#define ICON_ACTIVE_PLAYER_RIGHT 0x07
#define ICON_CONNECTION_EVEN 0x1e
#define ICON_CONNECTION_ODD 0x1f
#define ICON_CLOCK 0x11
#define ICON_BLANK 0x20

// Box outline (drawBox) — hires charset cell indices
#define CHAR_BOX_TOP_LEFT      0x3b
#define CHAR_BOX_TOP_RIGHT     0x3c
#define CHAR_BOX_BOTTOM_LEFT   0x3d
#define CHAR_BOX_BOTTOM_RIGHT  0x3e
#define CHAR_BOX_SIDE          0x3f
#define CHAR_BOX_HORIZ         0x40

// Ship hull segments (drawShipInternal) — hires charset
#define SHIP_HULL_STERN_HORIZ  0x14  // first column, horizontal ship
#define SHIP_HULL_BOW_HORIZ     0x12  // last column, horizontal ship
#define SHIP_HULL_MID_HORIZ     0x13
#define SHIP_HULL_BOW_VERT      0x17  // top segment, vertical ship (bow up)
#define SHIP_HULL_STERN_VERT    0x15  // bottom segment, vertical ship
#define SHIP_HULL_MID_VERT      0x16

/* drawShip / drawShipInternal: ship axis (pos below 100 = horizontal, else vertical) */
#define SHIP_ORIENT_HORIZONTAL  0
#define SHIP_ORIENT_VERTICAL    1

// Normal display (blink = 0)
#define EMPTY_NORMAL_EVEN    0x18  // normal even
#define EMPTY_NORMAL_ODD     0x5b  // normal odd 
#define HIT_NORMAL_EVEN      0x19  // hit normal even
#define HIT_NORMAL_ODD       0x60  // hit normal odd
#define MISS_NORMAL_EVEN     0x1a  // miss normal even
#define MISS_NORMAL_ODD      0x29  // miss normal odd

// Hit blink
#define HIT2_EVEN            0x1b  // hit blink even 
#define HIT2_ODD             0x06  // hit blink odd  

// Hit legend
#define HIT_LEGEND_EVEN      0x1c  // hit legend even
#define HIT_LEGEND_ODD       0x1d  // hit legend odd

// Cursor display (blink > 0) - empty cell
#define EMPTY_CURSOR_BLINK1_EVEN  0x23  // empty cursor blink1 even
#define EMPTY_CURSOR_BLINK1_ODD   0x61  // empty cursor blink1 odd
#define EMPTY_CURSOR_BLINK2_EVEN  0x24  //  empty cursor blink2 even
#define EMPTY_CURSOR_BLINK2_ODD   0x62  //  empty cursor blink2 odd

// Cursor display (blink > 0) - hit
#define HIT_CURSOR_BLINK1_EVEN    0x25  // hit, cursor blink1 (even x coordinate)
#define HIT_CURSOR_BLINK1_ODD    0x63  // hit, cursor blink1 (odd x coordinate)
#define HIT_CURSOR_BLINK2_EVEN    0x26  // hit, cursor blink2 (even x coordinate)
#define HIT_CURSOR_BLINK2_ODD    0x64  // hit, cursor blink2 (odd x coordinate)

// Cursor display (blink > 0) - miss
#define MISS_CURSOR_BLINK1_EVEN   0x27  // miss, cursor blink1 (even x coordinate)
#define MISS_CURSOR_BLINK1_ODD    0x65  // miss, cursor blink1 (odd x coordinate)
#define MISS_CURSOR_BLINK2_EVEN   0x28  // miss, cursor blink2 (even x coordinate)
#define MISS_CURSOR_BLINK2_ODD    0x66  // miss, cursor blink2 (odd x coordinate)

#define ATTACK_ANIM_START_EVEN  0x67  // attack animation start even
#define ATTACK_ANIM_START_ODD   0x6D  // attack animation start odd

/**
 * Platform specific key map for common input
 */

#define KEY_LEFT_ARROW 0x08
#define KEY_LEFT_ARROW_2 0xF1
#define KEY_LEFT_ARROW_3 0xF2 // ,

#define KEY_RIGHT_ARROW 0x15
#define KEY_RIGHT_ARROW_2 0xF3
#define KEY_RIGHT_ARROW_3 0xF4 // .

#define KEY_UP_ARROW 0x0b
#define KEY_UP_ARROW_2 0xF5
#define KEY_UP_ARROW_3 0xF6 // -

#define KEY_DOWN_ARROW 0x0a
#define KEY_DOWN_ARROW_2 0xF7
#define KEY_DOWN_ARROW_3 0xF9 // =

#define KEY_RETURN 0x0D

#define KEY_ESCAPE 0x03
#define KEY_ESCAPE_ALT 0x1B

#define KEY_SPACEBAR 0x20
#define KEY_BACKSPACE 0x7F

#define CHAR_CURSOR 0x9F


#endif /* KEYMAP_H */