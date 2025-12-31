#ifndef HIRES_H
#define HIRES_H
#include "vars.h"
#include <stdint.h>

void hires_putc(uint8_t x, uint8_t y, ROP_TYPE rop, uint8_t c);
void hires_putcc(uint8_t x, uint8_t y, ROP_TYPE rop, unsigned cc);
void hires_Mask(uint8_t x, uint8_t y, uint8_t xlen, uint8_t ylen, uint8_t c);
void hires_Draw(uint8_t x, uint8_t y, uint8_t xlen, uint8_t ylen, ROP_TYPE rop, uint8_t *src);

#ifdef COCO3
// Defines to switch to the particular MMU task
#define task0()              \
    do                       \
    {                        \
        asm("clr", "$FF91"); \
    } while (0) /* normal mode (to run main program) */
#define task1()              \
    do                       \
    {                        \
        asm("ldb", "#1");    \
        asm("stb", "$FF91"); \
    } while (0) /* to access graphics */

#define BEGIN_GFX        \
    disableInterrupts(); \
    task1();

#define END_GFX \
    task0();    \
    enableInterrupts();

#else
#define BEGIN_GFX
#define END_GFX
#endif

#endif /* HIRES_H */