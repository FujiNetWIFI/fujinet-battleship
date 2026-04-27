#ifndef GRAPHICS_BOARD_H
#define GRAPHICS_BOARD_H

#include <stdbool.h>
#include <stdint.h>

void drawBoardPlayer(uint8_t player, uint8_t playerCount);
void drawDrawerBorder(uint8_t x, uint8_t y, uint8_t type, uint8_t length);
void drawPlayerBorders(uint8_t player, bool active);

#endif
