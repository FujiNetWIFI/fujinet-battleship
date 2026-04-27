#ifndef GRAPHICS_INTERNALS_H
#define GRAPHICS_INTERNALS_H

#include <stdint.h>

/* identify.s */
int identify(void);

/* waitvsync.s */
void setVsyncProc(int type);

/* graphics.c — used by drawShip / drawLegendShip only */
void drawShipInternal(unsigned char x, unsigned char y, uint8_t size, uint8_t orientation);

#endif
