#ifndef H89_CONSOLE_H
#define H89_CONSOLE_H

/*
  H19 terminal screen for the H89 port.

  The screen is drawn by cell codes, one byte each: bit 7 is reverse video and
  the low seven bits are the H19's own character ROM index (444-29) --
  0x00-0x1F the graphics characters, 0x7F the graphics bullet, 0x20-0x7E
  ASCII -- the same form the terminal keeps its screen in. console.c keeps a
  shadow of what the terminal shows and sends only the cells that change, with
  as few cursor-address (ESC Y), reverse-video (ESC p/q) and graphics-mode
  (ESC F/G) changes as it can: the console runs at 9600 baud.
*/

#include <stdint.h>
#include <stdbool.h>

#define SCR_W 80
#define SCR_H 24

#define REV 0x80

// H19 graphics characters: ROM index, shown in graphics mode (ESC F) for the
// character in the comment
#define G_VLINE      0x00 // `  vertical line
#define G_HLINE      0x01 // a  horizontal line
#define G_CROSS      0x02 // b  cross
#define G_CORNER_UR  0x03 // c  upper right corner
#define G_CORNER_LR  0x04 // d  lower right corner
#define G_CORNER_LL  0x05 // e  lower left corner
#define G_CORNER_UL  0x06 // f  upper left corner
#define G_PLUSMINUS  0x07 // g
#define G_ARROW_R    0x08 // h  right arrow
#define G_CHECKER    0x09 // i  checkerboard
#define G_DIVIDE     0x0A // j
#define G_ARROW_D    0x0B // k  down arrow
#define G_QUAD_LR    0x0C // l  lower right quadrant
#define G_QUAD_LL    0x0D // m  lower left quadrant
#define G_QUAD_UL    0x0E // n  upper left quadrant
#define G_QUAD_UR    0x0F // o  upper right quadrant
#define G_HALF_TOP   0x10 // p  upper half
#define G_HALF_RIGHT 0x11 // q  right half
#define G_TRI_UL     0x12 // r  upper left triangle
#define G_TEE_DOWN   0x13 // s  T, stem down
#define G_TEE_LEFT   0x14 // t  T, stem left
#define G_TEE_UP     0x15 // u  T, stem up
#define G_TEE_RIGHT  0x16 // v  T, stem right
#define G_X          0x17 // w  diagonal cross
#define G_SLASH      0x18 // x  /
#define G_BACKSLASH  0x19 // y  backslash
#define G_BAR_TOP    0x1A // z  top line
#define G_BAR_BOTTOM 0x1B // {  bottom line
#define G_BAR_LEFT   0x1C // |  left bar
#define G_BAR_RIGHT  0x1D // }  right bar
#define G_PILCROW    0x1E // ~
#define G_TRI_UR     0x1F // _  upper right triangle
#define G_BULLET     0x7F // ^  bullet

/// @brief Set up the terminal (and HDOS's console driver) for the game
void termInit(void);

/// @brief Put the terminal (and HDOS's console driver) back as they were
void termExit(void);

/// @brief Clear the screen
void termClear(void);

/// @brief Set one cell, sending it only if it changed
void put(uint8_t x, uint8_t y, uint8_t code);

/// @brief Set a run of w cells to one code
void fill(uint8_t x, uint8_t y, uint8_t w, uint8_t code);

/// @brief Copy the screen aside / bring it back
void termSave(void);
void termRestore(void);

/// @brief Ring the terminal's bell
void termBell(void);

/// @brief Raw console byte, or -1 when none is waiting
int conIn(void);

/// @brief The H89's 2 ms clock tick counter (HDOS .TICCNT / the CP/M BIOS's)
#ifdef __HDOS__
#define TICKS (*(volatile uint16_t *)0x201B)
#else
#define TICKS (*(volatile uint16_t *)0x000B)
#endif

#endif /* H89_CONSOLE_H */
