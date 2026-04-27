/**
 * @brief Hires graphics functions
 * @author Oliver Schmidt
 */

#ifndef HIRES_H
#define HIRES_H

#define ROP_CONST(val)          0xA980|(val)
#define ROP_BLACK               0xA980
#define ROP_WHITE               0xA9FF
//#define ROP_X                   0x4900
//#define ROP_XOR(val)            ROP_X|(val)
#define ROP_CPY                 0x4980
#define ROP_INV                 0x49FF
#define ROP_A                   0x2900
#define ROP_O                   0x0900
#define ROP_CPY_NOFLIP          0x0900  //Green/Violet              
#define ROP_AND(val)            ROP_A|(val)
#define ROP_OR(val)             ROP_O|(val)

/** hires_Mask: bulk clear / blue-black fill (resetScreen, endgame message area) */
#define HIRES_MASK_CLEAR_MAIN   0xA900

void hires_Init(void);
void hires_Done(void);
void hires_Text(char flag);
void hires_Draw(char xpos,  char ypos,
                char xsize,   char ysize,
                unsigned rop, char *src);
void hires_Mask(char xpos,    char ypos,
                char xsize,   char ysize,
                unsigned rop);
void hires_Clear(void);

#endif /* HIRES_H */
