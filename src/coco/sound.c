/*
  Platform specific sound functions
*/

#include "../misc.h"

//extern void stopWaterAnim();
//extern void startWaterAnim();


uint16_t ii;

void tone(uint8_t period, uint8_t dur, uint8_t wait)
{
    if (!prefs.disableSound)
    {
        //stopWaterAnim();
        sound(period, dur);
        //startWaterAnim();
    }

    while (wait--)
        for (ii = 0; ii < 60; ii++)
            ;
}

// // Keeping this here in case I need it
// void toneFinder() {
//   clearCommonInput();
//   while (input.key != KEY_RETURN || i<2) {
//     while (!kbhit());
//     input.key = cgetc();
//     switch (input.key) {
//       case KEY_DOWN_ARROW: i--; break;
//       case KEY_UP_ARROW: i++; break;
//       case KEY_RIGHT_ARROW: i+=20; break;
//       case KEY_LEFT_ARROW: i-=20; break;
//       case KEY_ESCAPE: return;
//     }

//     printf("%d ",i);
//     tone(i,1,0);
//   }
// }

void initSound() {}

void soundJoinGame()
{
    tone(40, 1, 50);
    tone(2, 1, 50);
    tone(40, 1, 0);
}

void soundMyTurn()
{
    setTimer(0);
    tone(40, 1, 30);
    tone(40, 1, 0);
}

void soundGameDone()
{
    tone(0, 2, 20);
    tone(70, 3, 100);
    tone(90, 3, 20);
    tone(110, 5, 20);
}

void soundCursor()
{
    static int i;
    // tone(100, 0, 0);
    // tone(120, 0, 0);
    // tone(90, 0, 0);
    // tone(10, 0, 0);
    // tone(8, 0, 0);

    tone(40, 0, 1);
    tone(70, 0, 2);
    tone(10, 0, 3);
    // tone(10, 0, 0);
    //  tone(0, 0, 0);
    //  tone(0, 0, 0);
}

void soundPlaceShip()
{
    uint8_t i;
    for (i = 0; i < 10; i++)
        tone((i * 13 + 8) % 100, 0, 0);
}

void soundTick()
{
    tone(0, 0, 0);
}

void soundSelect()
{
    tone(80, 1, 0);
    tone(90, 1, 0);
}

void soundMiss()
{
    uint16_t i, j;
    uint8_t c;

    tone(50, 0, 0);
    tone(60, 0, 0);
    tone(70, 0, 0);
    // Enable 6 bit dac
    // *(uint8_t *)0xFF23 = 0x3F;

    // for (i = 200; i > 0; --i)
    // {
    //     *(uint8_t *)0xFF20 = (rand() >> 2) + 128;
    // }
    // for (c = 100; c > 0; --c)
    //     for (i = 10; i > 0; i--)
    //     {
    //         *(uint8_t *)0xFF20 = 100 - c / 2 + rand() % c;
    //     }
}

void soundInvalid()
{
    uint8_t i;
    uint8_t c;

    for (i = 5; i > 0; i--)
    {
        tone(i * 2, 0, 0);
        // Enable 6 bit dac
        *(uint8_t *)0xFF23 = 0x3F;

        for (c = 10; c > 0; c--)
        {
            *(uint8_t *)0xFF20 = (uint8_t)(rand() % 50);
        }
    }
}

void soundAttack()
{
    uint8_t i;
    for (i = 100; i < 250; i -= 5)
        tone(i, 0, 0);
}

void soundHit()
{
    uint16_t i;
    uint8_t c;

    // Enable 6 bit dac
    *(uint8_t *)0xFF23 = 0x3F;

    for (i = 240; i > 0; --i)
    {
        c = rand() % i;
        *(uint8_t *)0xFF20 = c & c;
        *(uint8_t *)0xFF20 = c & c - 1;
        *(uint8_t *)0xFF20 = c & c + 1;
        *(uint8_t *)0xFF20 = c & c + 10;
    }
}

void soundSink()
{
    soundHit();
}

// Not applicable to CoCo
void soundStop() {}
void disableKeySounds() {}
void enableKeySounds() {}
