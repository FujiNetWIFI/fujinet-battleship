/*
  Apple II Sound - Prototype version
*/

#include <stdint.h>
#include <stdlib.h>
#include <apple2.h>
#include <string.h>
#include "../misc.h"
#include "../platform-specific/sound.h"
#define CLICK  __asm__ ("sta $c030")

uint16_t ii;

void tone(uint16_t period, uint8_t dur, uint8_t wait) {
  while (dur--) {
    for (ii=0; ii<period; ii++) ;
    CLICK;
  }

  while (wait--)
    for (ii=0; ii<40; ii++) ;
}

// Keeping this here in case I need it
// void toneFinder() {
//   clearCommonInput();
//   while (inputKey != KEY_RETURN || i<2) {
//     while (!kbhit());
//     inputKey = cgetc();
//     if (inputKey == KEY_DOWN_ARROW)
//       i-=1;
//     if (inputKey == KEY_UP_ARROW)
//       i+=1;
//       cprintf("%i ",i);
//     tone(i,50,0,0);
//   }
// }

void initSound() {
 
}

void soundJoinGame() {
  tone(34,40,115);
  tone(44,40,40);
  tone(28,40,0);
}

void soundMyTurn() {
  tone(34,40,50);
  tone(34,40,0);
}

void soundGameDone() {
  tone(83,20,20);
  tone(79,50,30);
  tone(65,20,20);
  tone(61,40,50);
}



void soundTick() {
 tone(80,2,0);
}
  
void soundPlayerJoin() {
    uint8_t i;
  for (i=255;i>=235;i-=10)
    tone(i,5,255);
}

void soundPlayerLeft() {
    uint8_t i;
  for (i=215;i<255;i+=10)
    tone(i,5,255);
}

void soundSelectMove() {
  tone(35,40,20);
  tone(30,40,0);
}

void soundCursor() {
  tone(45,10,0);
}

void soundCursorInvalid() {
  tone(118,5,0);
}

void soundTakeChip(uint8_t counter) {
  tone(118-counter*3,5,60);
}
void soundSelect()
{
    tone(35, 40, 20);
    tone(40, 40, 0);

    tone(25, 60, 0);
}

void soundHit()
{
    uint8_t i;

    // 最初に強めの一発「ドン」
    tone(24, 25, 8);       // 高めの短いアタック

    // 続けて、やや長めの下降スイープ「ドゥーン」
    for (i = 0; i < 10; ++i)
    {
        // period を増やしていくことで徐々に低くする
        uint16_t period = 40 + i * 5;   // 40 → 85 くらい
        tone(period, 3, 0);
    }
}

#define SPEAKER ((volatile uint8_t*)0xC030)
void soundMiss()
{
    uint8_t burst, i;
    uint8_t d;

    // short noise burst
    for (burst = 0; burst < 4; ++burst)
    {
        for (i = 0; i < 25; ++i)
        {
            *SPEAKER = 0;

            d = 6 + (rand() & 0x1f);      // about 6〜37 
            while (d--)
            {
                ;
            }
        }
        // short silence between burst
        for (i = 0; i < 30; ++i)
        {
            d = 40;
            while (d--)
            {
                ;
            }
        }
    }
}

void soundSunk()
{
     // Sound effect when ship sinks
    // Use descending scale pattern, referencing soundGameDone()
    tone(70, 30, 10);   // Mid tone
    tone(80, 30, 10);   // Slightly low tone
    tone(90, 40, 20);   // Low tone (sinking feeling)
    tone(100, 50, 0);   // Even lower tone
}


void disableKeySounds()
{
    // Not implemented in prototype
}
void enableKeySounds()
{
    // Not implemented in prototype
}
void soundInvalid()
{
    tone(80, 12, 2);
    tone(60, 14, 0); 
}
void soundAttack()
{
    uint8_t i;

    for (i = 0; i < 8; ++i)
    {
        uint16_t period = 42 - i * 2;
        tone(period, 4, 1);
    }
}

void soundPlaceShip()
{
    // Atari version: frequency 96, volume 5 → frequency 81, volume 4
    // Apple II version: similar to soundSelectMove() pattern
    // Change from high to low tone
    tone(35, 40, 20);  // High tone (equivalent to frequency 96)
    tone(40, 40, 0);   // Low tone (equivalent to frequency 81)
}