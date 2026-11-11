#ifndef M2_CONSOLE_H
#define M2_CONSOLE_H

/*
  The Model II screen, drawn by cell codes into a back buffer and sent by
  difference through the P&T video driver (src/model2/console.c). A cell is
  one byte: bit 7 is reverse video, the rest a graphics character (00h-1Fh)
  or ASCII.
*/

#include <stdint.h>
#include <stdbool.h>

#define SCR_W 80
#define SCR_H 24

#define REV 0x80

// Model II graphics characters, from the Operation Manual's chart. A cell is
// 8 dots wide and 10 high.
#define G_CORNER_UL  0x00
#define G_CORNER_UR  0x01
#define G_CORNER_LR  0x02
#define G_CORNER_LL  0x03
#define G_TEE_DOWN   0x04
#define G_TEE_LEFT   0x05
#define G_TEE_UP     0x06
#define G_TEE_RIGHT  0x07
#define G_CROSS      0x0C
#define G_VLINE      0x14
#define G_HLINE      0x16
#define G_LOW_2      0x18 // the bottom 2 rows of dots
#define G_LOW_4      0x19
#define G_LOW_6      0x1A
#define G_LOW_8      0x1B
#define G_LEFT_2     0x1C // the left 2 columns of dots
#define G_LEFT_4     0x1D
#define G_LEFT_6     0x1E

void termInit(void);
void termExit(void);
void put(uint8_t x, uint8_t y, uint8_t code);
void fill(uint8_t x, uint8_t y, uint8_t w, uint8_t code);
void termClear(void);

/// @brief Send the screen whatever differs from the back buffer
void flush(void);

void termSave(void);

/// @brief Put back the copy termSave() took (sent at the next flush)
void termRestore(void);

/// @brief One byte straight to the BIOS console
void conOut(uint8_t c) __z88dk_fastcall;

/// @brief Raw console byte, or -1 when none is waiting
int conIn(void);

/// @brief P&T's clock, in 1/100 s since boot
uint32_t ticks(void);

#endif /* M2_CONSOLE_H */
