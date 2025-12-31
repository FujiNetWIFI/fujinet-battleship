//#ifdef __APPLE2__

/*
  Graphics functionality
*/
#include <stdlib.h>
#include <stdio.h>
#include "hires.h"
#include "text.h"
#include<peekpoke.h>
#include<string.h>
#include "../platform-specific/graphics.h"
#include "../misc.h"
#include "vars.h"

#define BOTTOM 175
#define RED_VAL_1 ROP_OR(0b11010100)
#define RED_VAL_2 ROP_OR(0b10101010)
// Added at the top of apple2/graphics.c
#define ROP_BLUE   ROP_OR(0b11010101)  
#define ROP_YELLOW ROP_OR(0b10101010)  // Phase-inverted hatching pattern
// Background pattern constants for checkerboard pattern
#define EVEN_BLUE  0b11010101  // Pattern for even x coordinates
#define ODD_BLUE   0b10101010  // Pattern for odd x coordinates
#define ROP_WRITE_RIGHT_HALF ROP_CONST(0b11110000) 
#define ROP_WRITE_LEFT_HALF ROP_CONST(0b1000011) 
#define SCREEN_WIDTH 40

int identify(void); 
void setVsyncProc(int type); 
unsigned char colorMode=0;
// Defined in this file
void drawTextAltAt(uint8_t x, uint8_t y, const char *s);
void drawTextAt(uint8_t x, uint8_t y, const char *s);
void drawShipInternal(unsigned char x, unsigned char y, uint8_t size, uint8_t delta);

static uint8_t fieldX = 0;
uint16_t quadrant_offset[] = {
  98 * 40 + 5,    // = 3925  (Player 0: bottom-left, x=5, y=98)
  10 * 40 + 5,    // = 405   (Player 1: top-left, x=5, y=10)
  10 * 40 + 17,   // = 417   (Player 2: top-right, x=17, y=10)
  98 * 40 + 17};  // = 3937  (Player 3: bottom-right, x=17, y=98)

 uint16_t legendShipOffset[] = {2, 1, 0, 256U * 5, 256U * 6 + 1}; 
extern unsigned char charset[];
#define OFFSET_Y 2
static uint16_t currentPlayerCount = 0;

// Drawer border font data (8 bytes × 8 types = 64 bytes)
// Each character is 7 pixels wide × 8 lines high
// Pattern definitions:
// - Top half black / bottom half blue: for horizontal borders
// - Right half black / left half blue: for vertical borders (left side)
// - Left half blue / right half black: for vertical borders (right side)
static const uint8_t drawerBorderFont[8][8] = {
    // 0: Top half black / bottom half blue (EVEN)
    {0x00, 0x00, 0x00, 0x00, 0xd5, 0xd5, 0xd5, 0xd5},
    // 1: Top half black / bottom half blue (ODD)
    {0x00, 0x00, 0x00, 0x00, 0xaa, 0xAA, 0xAA, 0xAA},
    // 2: Top half blue / bottom half black (EVEN)
    {0xd5, 0xd5, 0xd5, 0xd5, 0x00, 0x00, 0x00, 0x00},
    // 3: Top half blue / bottom half black (ODD)
    {0xAA, 0xAA, 0xAA, 0xaa, 0x00, 0x00, 0x00, 0x00},
    // 4: Right half black / left half blue (EVEN)
    {0x8A, 0x8A, 0x8A, 0x8A, 0x8A, 0x8A, 0x8A, 0x8A},
    // 5: Right half black / left half blue (ODD)
    {0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0},
    // 6: Left half blue / right half black (EVEN)
    {0xD0, 0xD0, 0xD0, 0xD0, 0xD0, 0xD0, 0xD0, 0xD0},
    // 7: Left half blue / right half black (ODD)
    {0xA8, 0xA8, 0xA8, 0xA8, 0xA8, 0xA8, 0xA8, 0xA8}
};
#define V_LINE_LEFT   0x07
#define V_LINE_RIGHT  0xf0

// Green line font data for active player name underline (1 byte per pattern)
static const uint8_t greenLineFont[2] = {
    0x2a,  // EVEN: green pattern
    0x55   // ODD: green pattern
};

// Helper function to draw drawer border
// type: 0=top black/bottom blue, 1=top blue/bottom black, 2=right black/left blue, 3=left blue/right black
// length: number of characters to draw
static void drawDrawerBorder(uint8_t x, uint8_t y, uint8_t type, uint8_t length)
{
    uint8_t fontIndex;
    uint8_t i;
    
    for (i = 0; i < length; i++) {
        uint8_t actualX = x + i;
        // Select font based on actual x coordinate (ODD/EVEN) and type
        if (type < 2) {
            // Horizontal borders (top/bottom)
            fontIndex = (actualX % 2) ? (type * 2 + 1) : (type * 2);
        } else {
          if (type == 2) {
            fontIndex = 4;
          }
          else {
            // Vertical borders (left/right)
            //fontIndex = (actualX % 2) ? ((type - 2) * 2 + 1 + 4) : ((type - 2) * 2 + 4);
            fontIndex = 6;
          }
        }
        hires_Draw(actualX, y, 1, 8, ROP_CPY_NOFLIP, (char*)&drawerBorderFont[fontIndex][0]);
    }
}

unsigned char cycleNextColor() {
  ++colorMode;
  if (colorMode>1)
    colorMode=0;
  return colorMode;
}



void drawTextAt(unsigned char x, unsigned char y, const char*s) {
  static unsigned char c;
  while(*s) {
    c=*s++;
    if (c>=97 && c<=122) c=c-32;
    hires_putc(x++,y,ROP_CPY,c);
  }  
}

void clearStatusBar() {
  //unsigned char i;
  hires_Mask(0,175,40,17,0xa900);
}

void drawStatusTimer() {
  hires_putcc(38,BOTTOM+5,ROP_CPY, 0x2829);
}

void drawText(unsigned char x, unsigned char y, const char* s) {
  drawTextAt(x,y * 8 - 4, s);  
}

// Call to clear the screen to an empty table
void resetScreen() { 
  hires_Mask(1,0,38,192,0xa900);  
}

void drawLine(unsigned char x, unsigned char y, unsigned char w) {
   hires_Mask(x,y*8-3,w,2, 0xa9ff); 
}
void hideLine(unsigned char x, unsigned char y, unsigned char w) {
   hires_Mask(x,y*8-3,w,2, 0xa900); 
}

void drawBox(unsigned char x, unsigned char y, unsigned char w, unsigned char h) {
  int i;

  // Treat y=0 as y=1 to prevent out-of-bounds access
  if (y == 0) {
    y = 1;
  }
  y=y*8-4;
  // Top Corners
  hires_putc(x,y,ROP_CPY, 0x3b);hires_putc(x+w+1,y,ROP_CPY, 0x3c);
  // Top/bottom lines
  for(i=x+w;i>x;--i) {
    hires_putc(i,y,ROP_CPY, 0x40);
    hires_putc(i,y+(h+1)*8,ROP_CPY, 0x40);
  }
  // Sides
  for(i=0;i<h;++i) {
    y+=8;
    hires_putc(x,y,ROP_CPY, 0x3f);
    hires_putc(x+w+1,y,ROP_CPY, 0x3f);
  }
  y+=8;
  // Bottom Corners
  hires_putc(x,y,ROP_CPY, 0x3d);hires_putc(x+w+1,y,ROP_CPY, 0x3e);
}

void resetGraphics() {
 
}

void initGraphics() {
  int type;
 // uint16_t c=0x900;
  bool use=false;
    
  hires_Init();
  hires_Clear();
  type = identify();
  if (type == -1) {
//    printf("error\n");
//    exit(1);
  }
  setVsyncProc(type);
}

void drawTextAltAt(uint8_t x, uint8_t y, const char *s) {
  if (y == HEIGHT - 1) {
    uint8_t len = (uint8_t)strlen(s);

    if (x >= WIDTH - 5) {
      drawTextAt(x, y * 8, s);
    } else {
      if (x > 0) {
        drawSpace(0, y, x);
    }
    drawTextAt(x, y * 8, s);
    if (x + len < WIDTH) {
        drawSpace(x + len, y, WIDTH - x - len);
      }
    }
  } else {
    drawTextAt(x, y * 8 - 4, s);
  }
}

void drawTextAlt(uint8_t x, uint8_t y, const char *s)
{
  uint8_t len;

  if (y == HEIGHT - 1) {
    // Clear before and after text when drawing on the bottom line
    len = (uint8_t)strlen(s);
    if (x >= WIDTH - 5) {   
      drawTextAt(x, y * 8, s);
    } else {
      if (x > 0) {
        drawSpace(0, y, x);
      }
      drawTextAt(x, y * 8, s);
      if (x + len < WIDTH) {
        drawSpace(x + len, y, WIDTH - x - len);
      }
    }
  } else {
    drawTextAt(x, y * 8 - 4, s);
  }
}

void drawIcon(uint8_t x, uint8_t y, uint8_t icon)
{
  if (icon == ICON_PLAYER) {
    hires_putc(x, y * 8 - 4, ROP_CPY_NOFLIP, icon);
  }
  else 
  {
    hires_putc(x, y * 8 - 4, ROP_CPY, icon);
  }
}

void drawShip(uint8_t quadrant, uint8_t size, uint8_t pos, bool hide)
{
    uint8_t x, y, delta = 0;
    uint8_t gx;

    if (pos > 99)
    {
        delta = 1; // 1=vertical, 0=horizontal
        pos -= 100;
    }

    x = (pos % 10) + fieldX + quadrant_offset[quadrant] % WIDTH;
    y = (pos / 10) * 8 + quadrant_offset[quadrant] / WIDTH + 1;

    if (hide)
    {
        if (!delta)
        {
            // Horizontal: redraw background pattern for each x coordinate
            for (gx = 0; gx < size; gx++)
            {
                hires_Mask(x + gx, y, 1, 8, ROP_CONST(((x + gx) % 2) ? ODD_BLUE : EVEN_BLUE));
            }
        }
        else
        {
            hires_Mask(x, y, 1, size * 8, ROP_CONST((x % 2) ? ODD_BLUE : EVEN_BLUE));
        }
        return;
    }
    drawShipInternal(x, y, size, delta);
}

void drawLegendShip(uint8_t player, uint8_t index, uint8_t size, uint8_t status)
{
    uint8_t x, y;
    uint16_t pos;
    uint8_t drawerX;

    pos = fieldX + quadrant_offset[player];
    x = (uint8_t)(pos % WIDTH);
    y = (uint8_t)(pos / WIDTH);
    
    if (player > 1 || (currentPlayerCount == 2 && player > 0))
    {
        // right drawer
        drawerX = x + 11; // ix + 1, where ix = x + 10
    }
    else
    {
        // left drawer
        drawerX = x - 4;  // ix - 3, where ix = x - 1
    }
    
    if (index < 3)
    {
        // First 3 ships: arranged from top to bottom
        x = drawerX + legendShipOffset[index]; // Fine adjustment in x direction
        y = y + 8; // Starting y position in drawer (quadrant y + 8)
    }
    else
    {
        // Last 2 ships: placed lower
        x = drawerX + (legendShipOffset[index] % 256); // Fine adjustment in x direction
        y = y + (legendShipOffset[index] / 256) * 8 + 8; // y offset (256 = 1 line)
    }
    
    if (status)
    {
        drawShipInternal(x, y, size, 1); 
    }
    else
    {
      uint8_t i;
        // Ship sunk state: display blank character
        //hires_Draw(x, y, 1, size * 8, ROP_CPY, (char *)&charset[(uint16_t)(x %2 ? 0x1c : 0x1d) << 3]);
        for (i=0; i < size; i++) {
          hires_Draw(x, y + (i * 8), 1, 8,  ROP_CPY, (char *)&charset[(uint16_t)((x % 2) ? 0x1d : 0x1c) << 3]);
        }
    }
}

void drawShipInternal(unsigned char x, unsigned char y, uint8_t size, uint8_t delta)
{
    uint8_t i;
    uint8_t c;
    
    if (delta == 0)
    {
        // Horizontal (delta = 0)
        // bow: 0x12, center: 0x13, stern: 0x14
        for (i = 0; i < size; i++)
        {
            if (i == 0)
            {
                c = 0x14; // stern
            }
            else if (i == size - 1)
            {
                c = 0x12; // bow
            }
            else
            {
                c = 0x13; // center
            }
            hires_putc(x + i, y, ROP_CPY, c);
        }
    }
    else
    {
        // Vertical (delta = 1)
        // bow: 0x17, center: 0x16, stern: 0x15
        for (i = 0; i < size; i++)
        {
            if (i == 0)
            {
                c = 0x17; // bow (upward)
            }
            else if (i == size - 1)
            {
                c = 0x15; // stern（downward）
            }
            else
            {
                c = 0x16; // center
            }
            hires_putc(x, y + (i * 8), ROP_CPY, c);
        }
    }
}

void drawPlayerName(uint8_t player, const char *name, bool active)
{
  uint16_t pos; 
  uint8_t x;
  uint8_t y;
  uint8_t nameLen = strlen(name);
  uint8_t gx;
  uint8_t lineY;

  
    pos = fieldX + quadrant_offset[player];
    x = (uint8_t)(pos % 40 + 1);  // Changed from 32 to 40
    y = (uint8_t)(pos / 40 - 9);
  if (player == 0 || player == 3)
  {
      y += 89;
  }

  lineY = y + 8;

  if (active)
  {
      // Draw marker and text
      hires_putc(x - 1, y, ROP_CPY_NOFLIP, ICON_ACTIVE_PLAYER);
      drawTextAt(x, y, name);
      
      // Draw green horizontal line at bottom of player name row (10 columns, 1 pixel thick)
      // Use hires_Draw with ROP_CPY_NOFLIP to get green/violet palette
      // Draw only 1 line (ysize=1) at lineY position
      for (gx = 0; gx < 10; gx++) {
          uint8_t actualX = x - 1 + gx;  // Start from x - 1 to cover 10 columns including icon
          uint8_t fontIndex = (actualX % 2);  // 0 for EVEN, 1 for ODD
          hires_Draw(actualX, lineY, 1, 1, ROP_CPY_NOFLIP, (char*)&greenLineFont[fontIndex]);
      }
  }
  else
  {
      hires_putc(x - 1, y, ROP_CPY, 0x20);
      drawTextAt(x, y, name);  // Draw player name
      
      // Erase green line by drawing black (10 columns, 1 pixel thick)
      for (gx = 0; gx < 10; gx++) {
          uint8_t actualX = x - 1 + gx;
          hires_Mask(actualX, lineY, 1, 1, ROP_BLACK);
      }
  }
}

uint8_t *srcBlank = &charset[(uint16_t)0x18 << 3];
uint8_t *srcHit = &charset[(uint16_t)0x19 << 3];
uint8_t *srcMiss = &charset[(uint16_t)0x1A << 3];
uint8_t *srcHit2 = &charset[(uint16_t)0x1B << 3];
uint8_t *srcHitLegend = &charset[(uint16_t)0x1C << 3];

void drawGamefield(uint8_t quadrant, uint8_t *field)
{
  uint16_t pos;
  uint8_t baseX;
  uint8_t baseY;
  uint8_t i, x, y;
  uint8_t charCode;
  uint8_t actualX;

  pos = fieldX + quadrant_offset[quadrant];
  baseX = (uint8_t)(pos % 40);  // WIDTH = 40
  baseY = (uint8_t)(pos / 40);  // WIDTH = 40

  for (i = 0; i < 100; i++)
  {
      if (field[i])
      {
          x = i % 10;
          y = i / 10;
          actualX = baseX + x;
          
          if (field[i] == FIELD_ATTACK)
          {
              // If actual x coordinate is even then EVEN, if odd then ODD
              charCode = (actualX % 2) ? HIT_NORMAL_ODD : HIT_NORMAL_EVEN;
          }
          else
          {
              charCode = (actualX % 2) ? MISS_NORMAL_ODD : MISS_NORMAL_EVEN;
          }
          hires_putc(actualX, baseY + y * 8, ROP_CPY, charCode);
      }
  }
}

void drawGamefieldUpdate(uint8_t quadrant, uint8_t *gamefield, uint8_t attackPos, uint8_t anim)
{
    uint16_t pos;
    uint8_t baseX;
    uint8_t baseY;
    uint8_t x;
    uint8_t y;
    uint8_t c;
    uint8_t charCode;

    pos = fieldX + quadrant_offset[quadrant];
    baseX = (uint8_t)(pos % 40);  // WIDTH = 40
    baseY = (uint8_t)(pos / 40);  // WIDTH = 40
    x = attackPos % 10;
    y = attackPos / 10;
    c = gamefield[attackPos];

    // Animate attack
    if (anim > 9)
    {
        // Draw attack frame
        charCode = anim + ((baseX + x) % 2 ? (ATTACK_ANIM_START_ODD-10) : (ATTACK_ANIM_START_EVEN-10));
    }
    else if (c == FIELD_ATTACK)
    {
        // Determine if actual x coordinate is even or odd
        if (anim)
        {
            // Blink animation
            charCode = ((baseX + x) % 2) ? HIT2_ODD : HIT2_EVEN;
        }
        else
        {
            // Normal display
            charCode = ((baseX + x) % 2) ? HIT_NORMAL_ODD : HIT_NORMAL_EVEN;
        }
    }
    else if (c == FIELD_MISS)
    {
        //charCode = 0x1A;  // srcMiss
        charCode = ((baseX + x) % 2) ? MISS_NORMAL_ODD : MISS_NORMAL_EVEN;
    }
    //   else if (c == 0)
    //   {
    //     charCode = ((baseX + x) % 2) ? EMPTY_NORMAL_ODD : EMPTY_NORMAL_EVEN;
    //       //return;  debug: need to write back even for empty cells
    //   }
    else {
        return;
    }

    hires_putc(baseX + x, baseY + y * 8, ROP_CPY, charCode);
}

void drawGamefieldCursor(uint8_t quadrant, uint8_t x, uint8_t y, uint8_t *gamefield, uint8_t blink)
{
  uint16_t pos;
  uint8_t baseX;
  uint8_t baseY;
  uint8_t c;
  uint8_t charCode;

    pos = fieldX + quadrant_offset[quadrant];
    baseX = (uint8_t)(pos % 40);  // WIDTH = 40
    baseY = (uint8_t)(pos / 40);  // WIDTH = 40
    c = gamefield[y * 10 + x];

    if (blink)
    {
        // Cursor display: character codes for blink animation
        if ((baseX + x) % 2) {
          // For odd x coordinates
          if (c == 0) {
              charCode = blink == 1 ? EMPTY_CURSOR_BLINK1_ODD : EMPTY_CURSOR_BLINK2_ODD;
          } else if (c == 1) {
              charCode = blink == 1 ? HIT_CURSOR_BLINK1_ODD : HIT_CURSOR_BLINK2_ODD;
          } else { // c == 2
              charCode = blink == 1 ? MISS_CURSOR_BLINK1_ODD : MISS_CURSOR_BLINK2_ODD;
          }
      } else {
          // For even x coordinates
          if (c == 0) {
              charCode = blink == 1 ? EMPTY_CURSOR_BLINK1_EVEN : EMPTY_CURSOR_BLINK2_EVEN;
          } else if (c == 1) {
              charCode = blink == 1 ? HIT_CURSOR_BLINK1_EVEN : HIT_CURSOR_BLINK2_EVEN;
          } else { // c == 2
              charCode = blink == 1 ? MISS_CURSOR_BLINK1_EVEN : MISS_CURSOR_BLINK2_EVEN;
          }
      }
    }
    else
    {
        // Normal display: character code according to cell state
        if ((baseX + x) % 2) {
          if (c == 0) {
              charCode = EMPTY_NORMAL_ODD;
          } else if (c == 1) {
              charCode = HIT_NORMAL_ODD;
          } else { // c == 2
              charCode = MISS_NORMAL_ODD;  // 0x29
          }
        } else {
          if (c == 0) {
              charCode = EMPTY_NORMAL_EVEN;
          } else if (c == 1) {
              charCode = HIT_NORMAL_EVEN;
          } else { // c == 2
              charCode = MISS_NORMAL_EVEN;  // 0x1a
          }
        }
    }
    hires_putc(baseX + x, baseY + y * 8, ROP_CPY, charCode);
  }

void drawClock()
{
  hires_putc(WIDTH - 1, (HEIGHT - 1) * 8, ROP_CPY, ICON_CLOCK);
}

void drawConnectionIcon(bool show)
{
  hires_putc(0, (HEIGHT - 1) * 8, ROP_CPY, show ? ICON_CONNECTION_EVEN : ICON_BLANK);
  hires_putc(1, (HEIGHT - 1) * 8, ROP_CPY, show ? ICON_CONNECTION_ODD : ICON_BLANK);
}

void drawBlank(uint8_t x, uint8_t y)
{
  hires_putc(x, y * 8 - 4, ROP_CPY, 0x20);
}

void drawSpace(uint8_t x, uint8_t y, uint8_t w) {
  uint8_t i;
     // If bottom line then y * 8, otherwise y * 8 - 4
  if (y == HEIGHT - 1) {
      y = y * 8;
  } else {
      y = y * 8 - 4;
  }
  for (i = 0; i < w; i++) {
      hires_putc(x + i, y, ROP_CPY, 0x20);
  }
}

void drawBoard(uint8_t playerCount)
{
    uint8_t i, x, y, ix, ox, left = 1, fy, eh, edgeSkip;
    uint8_t drawX;  // x start position of drawer
    uint8_t gx;
    uint16_t pos;
    // Center layout
    fieldX = playerCount > 2 ? 4 : 10;
    currentPlayerCount = playerCount;
    for (i = 0; i < playerCount; i++)
    {
        pos = fieldX + quadrant_offset[i];
        x = (uint8_t)(pos % WIDTH);
        y = (uint8_t)(pos / WIDTH);

        // right and left drawers
        if (i > 1 || playerCount == 2 && i > 0)
        {
            ox = x - 1;
            ix = x + 10;
            left = 0;
            drawX = ix + 1; 
        }
        else
        {
            ix = x - 1;
            ox = x + 10;
            drawX = ix - 3;
        }
        if (i == 1 || i == 2)
        {
          if (y - 9 < 0 || y - 9 >= 192) return;  // or skip
          if (y + 80 >= 192) return;  // or skip
          if (y + 82 >= 192) return;  // or skip
            fy = y + 80;
        }
        else
        {
            fy = y - 8;
        }
        // Blue gamefield
        for (gx=0; gx < 10; gx++) {
          hires_Mask(x+gx, y, 1, 80, ((x+gx) % 2 ) ? ROP_YELLOW : ROP_BLUE);
        }
        edgeSkip = 0;
        if (playerCount == 1)
        {
            fy += 5;
            edgeSkip = 4;
        }
        // Far edge
        if (i || edgeSkip)
        {
            if (i != 2 && !edgeSkip)
                eh = 8;
            else
                eh = 3;
        }
        // Fill in the drawer
        for (gx=0; gx < 3; gx++) {
          hires_Mask(drawX+gx, y+8, 1, 64, ((drawX+gx) % 2 ) ? ROP_YELLOW : ROP_BLUE);
        }
        // Draw drawer borders
        // Top border (3 characters wide, starting at y, above drawer)
        drawDrawerBorder(drawX, y, 0, 3);
        
        // Bottom border (3 characters wide, at y+8+64)
        drawDrawerBorder(drawX, y + 8 + 64, 1, 3);
        
        // Vertical borders (9 lines high to cover y+4 to y+4+64)
        if (i > 1 || playerCount == 2 && i > 0) {
            // Right drawer: right border (outer side, away from game field)
            for (gx = 0; gx < 9; gx++) { 
                drawDrawerBorder(drawX + 3, y + 4 + gx * 8, 2, 1);
            }
        } else {
            // Left drawer: left border (outer side, away from game field)
            for (gx = 0; gx < 9; gx++) {
                drawDrawerBorder(drawX - 1, y + 4 + gx * 8, 3, 1);
            }
        }
        // Draw white outline outside drawer borders
        if (i > 1 || playerCount == 2 && i > 0) {
            // Right drawer
            // Top white line (1 pixel thick, above drawer top border, 4 columns wide)
            hires_Mask(drawX, y + 3, 4, 1, ROP_WHITE);
            
            // Bottom white line (1 pixel thick, below drawer bottom border, 4 columns wide)
            hires_Mask(drawX, y + 8 + 68, 4, 1, ROP_WHITE);
            
            // Right side white line (leftmost bit of column = screen right)
            // Use ROP_OR to preserve drawer border drawing
            for (gx = 0; gx < 9; gx++) {
                hires_Mask(drawX + 3, y + 4 + gx * 8, 1, 8, ROP_OR(V_LINE_RIGHT));
            }
        } else {
            // Left drawer
            // Top white line (1 pixel thick, above drawer top border, 4 columns wide)
            // x position: 4 pixels left (adjust by using drawX - 1 for partial column offset)
            hires_Mask(drawX - 1, y + 3, 4, 1, ROP_WHITE);
            
            // Bottom white line (1 pixel thick, below drawer bottom border, 4 columns wide)
            // x position: 4 pixels left
            hires_Mask(drawX - 1, y + 8 + 68, 4, 1, ROP_WHITE);
            
            // Left side white line (rightmost bit of column = screen left)
            // Use ROP_OR to preserve drawer border drawing
            for (gx = 0; gx < 9; gx++) {
                hires_Mask(drawX - 1, y + 4 + gx * 8, 1, 8, ROP_OR(V_LINE_LEFT));
            }
        }
        
        // Draw white lines inside game field
        // Horizontal line (opposite side of player name)
        if (i == 1 || i == 2) {
            // Top players: draw at bottom of game field
            hires_Mask(x, y + 80, 10, 1, ROP_WHITE);
        } else {
            // Bottom players: draw at top of game field
            hires_Mask(x, y - 1, 10, 1, ROP_WHITE);
        }
        
        // Vertical line (opposite side of drawer)
        // Use ROP_OR to preserve game field drawing
        if (i > 1 || playerCount == 2 && i > 0) {
            // Right players: draw at left edge of game field
            if (i == 1 || i == 2) {
                // Top players: extend line 1 line down at the end
                for (gx = 0; gx < 10; gx++) {
                    hires_Mask(x - 1, y + gx * 8, 1, 8, ROP_OR(V_LINE_RIGHT));
                }
                // Add 1 line at the bottom
                hires_Mask(x - 1, y + 80, 1, 1, ROP_OR(V_LINE_RIGHT));
            } else {
                // Bottom players: start 1 line up
                // Add 1 line at the top
                hires_Mask(x - 1, y - 1, 1, 1, ROP_OR(V_LINE_RIGHT));
                for (gx = 0; gx < 10; gx++) {
                    hires_Mask(x - 1, y + gx * 8, 1, 8, ROP_OR(V_LINE_RIGHT));
                }
            }
        } else {
            // Left players: draw at right edge of game field
            if (i == 1 || i == 2) {
                // Top players: extend line 1 line down at the end
                for (gx = 0; gx < 10; gx++) {
                    hires_Mask(x + 10, y + gx * 8, 1, 8, ROP_OR(V_LINE_LEFT));
                }
                // Add 1 line at the bottom
                hires_Mask(x + 10, y + 80, 1, 1, ROP_OR(V_LINE_LEFT));
            } else {
                // Bottom players: start 1 line up
                // Add 1 line at the top
                hires_Mask(x + 10, y - 1, 1, 1, ROP_OR(V_LINE_LEFT));
                for (gx = 0; gx < 10; gx++) {
                    hires_Mask(x + 10, y + gx * 8, 1, 8, ROP_OR(V_LINE_LEFT));
                }
            }
        }
    }
}

bool saveScreenBuffer()
{
    return false;
}

void restoreScreenBuffer()
{
    // Not implemented in prototype
}
void drawEndgameMessage(const char *message)
{
  uint8_t x;
  uint8_t len = (uint8_t)strlen(message);
  x = (WIDTH - len) / 2;

  // Clear a wider area to ensure "YOU" and other player names are removed
  // Clear from y=170 (approximately 4 lines above message) to y=192 (bottom of screen)
  // This covers the area where player names are drawn (y=178) and the message line (y=183)
  hires_Mask(0, 170, WIDTH, 22, 0xa900);
  
  drawTextAt(x, HEIGHT * 8 - 9, message);
}
/* 
void waitvsync() {
  static uint16_t i;
  // Aproximate a jiffy for the timer countdown
  for ( i=0;i<630;i++);
}
  */
//#endif /* __APPLE2__ */