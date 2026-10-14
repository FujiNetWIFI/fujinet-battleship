/*
  Platform specific sound functions - Coleco Adam SN76489 (port 0xFF)

  The sound set is transposed from src/atari/sound.c. sound() keeps the
  POKEY-style (voice, frequency, distortion, volume) interface and converts:
    - frequency: POKEY 64kHz divisor f -> SN tone period N = 7*(f+1)/2
      (63921/(2*(f+1)) Hz == 3579545/(32*N) Hz)
    - volume 0-15 -> SN attenuation 15-vol
    - distortion 10 = pure tone on the requested voice; anything else runs
      the noise channel clocked by tone generator 2 so pitch sweeps carry
      over (mode 0xE7 = white noise, rate from tone 2)
*/

#include "../misc.h"

#define SN_PORT 0xFF

static uint8_t noiseMode = 0;

static void snw(uint8_t b)
{
    outp(SN_PORT, b);
}

static void tone(uint8_t ch, uint16_t n)
{
    snw(0x80 | (ch << 5) | (n & 0x0F));
    snw((n >> 4) & 0x3F);
}

static void vol(uint8_t ch, uint8_t atten)
{
    snw(0x90 | (ch << 5) | atten);
}

void soundStop()
{
    snw(0x9F);
    snw(0xBF);
    snw(0xDF);
    snw(0xFF);
    noiseMode = 0;
}

static void sound(uint8_t voice, uint8_t frequency, uint8_t distortion, uint8_t volume)
{
    static uint16_t n;

    if (prefs.disableSound)
        return;

    if (!volume)
    {
        soundStop();
        return;
    }

    n = ((uint16_t)frequency + 1) * 7 / 2;
    if (n > 1023)
        n = 1023;

    if (distortion == 10)
    {
        tone(voice, n);
        vol(voice, 15 - volume);
    }
    else
    {
        // Noise pitched by tone generator 2
        tone(2, n);
        if (noiseMode != 0xE7)
        {
            noiseMode = 0xE7;
            snw(noiseMode);
        }
        vol(3, 15 - volume);
    }
}

static void note(uint8_t n, uint8_t n2, uint8_t n3, uint8_t d, uint8_t f, uint8_t p)
{
    static uint8_t i;

    if (prefs.disableSound)
        return;

    sound(0, n, 10, 8);
    if (n2)
        sound(1, n2, 10, 6);
    if (n3)
        sound(2, n3, 10, 4);

    pause(d);

    for (i = 7; i < 255; i--)
    {
        sound(0, n, 10, i);
        if (n2 && i > 1)
            sound(1, n2, 10, i - 2);
        if (n3 && i > 3)
            sound(2, n3, 10, i - 4);
        pause(f);
    }
    soundStop();
    pause(p);
}

void initSound()
{
    soundStop();
}

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
    soundStop();
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
    static uint8_t i;
    for (i = 0; i < 10; i++)
    {
        sound(0, 0, 8, 4 - i / 2);
        pause(2);
    }
    soundStop();
}

void soundInvalid()
{
    static uint8_t i;
    for (i = 6; i < 255; i--)
    {
        sound(0, 255 - i * 5, 10, i);
        waitvsync();
    }
    soundStop();
}

void soundAttack()
{
    static uint8_t i;
    for (i = 1; i < 8; i++)
    {
        sound(0, 200 + i, 2, 7 - i);
        pause(2);
    }
    soundStop();
}

void soundHit()
{
    static uint8_t i;
    for (i = 0; i < 10; i++)
    {
        sound(0, 80 + i, 0, 9 - i);
        pause(2);
    }
    soundStop();
}

void soundSink()
{
    static uint8_t i;
    for (i = 0; i < 10; i++)
    {
        sound(0, 50 + i, 0, 9 - i);
        pause(1);
        sound(0, 80 + i, 0, 9 - i);
        pause(1);
    }
    soundStop();
}

void disableKeySounds()
{
}

void enableKeySounds()
{
}
