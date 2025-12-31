/*
  Platform specific sound functions
*/

#include <atari.h>
#include <stdlib.h>
#include <peekpoke.h>
#include "../misc.h"
#include "../platform-specific/sound.h"

uint16_t ii;

void initSound()
{
    // Silence SIO noise
    OS.soundr = 0;
    disableKeySounds();
}

void sound(unsigned char voice, unsigned char frequency, unsigned char distortion, unsigned char volume)
{
    if (!prefs.disableSound)
        _sound(voice, frequency, distortion, volume);
}

void note(uint8_t n, uint8_t n2, uint8_t n3, uint8_t d, uint8_t f, uint8_t p)
{
    static uint8_t i;
    if (prefs.disableSound)
        return;

    _sound(0, n, 10, 8);
    if (n2)
        _sound(1, n2, 10, 6);
    if (n3)
        _sound(2, n3, 10, 4);

    pause(d);

    for (i = 7; i < 255; i--)
    {
        _sound(0, n, 10, i);
        if (n2 && i > 1)
            _sound(1, n2, 10, i - 2);
        if (n3 && i > 3)
            _sound(2, n3, 10, i - 4);
        pause(f);
    }
    pause(p);
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

void soundJoinGame()
{
    static uint8_t j;
    for (j = 0; j < 2; j++)
    {
        note(81, 0, 0, 0, 1, 0);
        if (j == 0)
            note(96, 0, 0, 0, 1, 0);
    }
}

void soundMyTurn()
{
    static uint8_t i, j;
    for (j = 0; j < 2; j++)
    {
        sound(0, 81, 10, 5);
        pause(2);
        for (i = 6; i < 255; i--)
        {
            sound(0, 81, 10, i);
            waitvsync();
        }
        waitvsync();
    }
}

void soundGameDone()
{
    note(128, 204, 64, 6, 2, 0);
    note(96, 153, 193, 25, 2, 3);
    note(76, 128, 153, 6, 2, 0);
    note(96, 153, 193, 25, 2, 3);
}

void soundCursor()
{
    sound(0, 91, 10, 7);
    pause(1);
    sound(0, 91, 10, 3);
    pause(1);
    soundStop();
}

void soundPlaceShip()
{
    sound(0, 96, 10, 5);
    pause(2);
    sound(0, 81, 10, 4);
    pause(2);
    soundStop();
}

void soundTick()
{
    sound(0, 200, 8, 7);
    waitvsync();
    soundStop();
}

void soundSelect()
{
    sound(0, 96, 10, 5);
    pause(2);
    sound(0, 81, 10, 4);
    pause(2);
    soundStop();
}

void soundMiss()
{
    uint8_t i;
    for (i = 0; i < 10; i++)
    {
        sound(0, 0, 8, 4 - i / 2);
        pause(2);
    }
}

void soundInvalid()
{
    static uint8_t i;
    for (i = 6; i < 255; i--)
    {
        sound(0, 255 - i * 5, 10, i);
        waitvsync();
    }
}

void soundAttack()
{
    uint8_t i;
    for (i = 1; i < 8; i++)
    {
        sound(0, 200 + i, 2, 7 - i);
        pause(2);
    }
}

void soundHit()
{
    uint8_t i;
    for (i = 0; i < 10; i++)
    {
        sound(0, 80 + i, 0, 9 - i);
        pause(2);
    }
    soundStop();
}

void soundSink()
{
    uint8_t i;
    for (i = 0; i < 10; i++)
    {
        sound(0, 50 + i, 0, 9 - i);
        pause(1);
        sound(0, 80 + i, 0, 9 - i);
        pause(1);
    }
    soundStop();
}

void soundStop()
{
    _sound(0, 0, 0, 0);
}

void disableKeySounds()
{
    OS.noclik = 255;
}

void enableKeySounds()
{
    OS.noclik = 0;
}
