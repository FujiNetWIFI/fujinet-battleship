/*
  Platform specific sound functions
*/

#include <stdint.h>
#include <stdlib.h>
#include "../misc.h"
#include "../platform-specific/sound.h"

uint16_t ii;

// PC Speaker ports and values
#define PIT_CONTROL_PORT 0x43
#define PIT_CHANNEL2_PORT 0x42
#define SPEAKER_CONTROL_PORT 0x61

// PIT frequency
#define PIT_FREQUENCY 1193180UL

/* Because I don't want to drag all of conio into this project! */
_WCIRTLINK extern unsigned inp(unsigned __port);
_WCIRTLINK extern unsigned outp(unsigned __port, unsigned __value);

/**
 * @brief Beep the speaker for the specified # of VBLANK frames
 * @param frequency Frequency in Hz
 * @param frames # of vertical blank intervals (approx 16.67ms per interval)
 * @param wait # of vertical blank intervals to wait.
 */
void beep(unsigned int frequency, unsigned int frames, unsigned int wait) {
    unsigned int divisor;
    unsigned char tmp;

    if (prefs.disableSound)
        return;

    // Calculate the divisor for the given frequency
    divisor = PIT_FREQUENCY / frequency;

    // Set the PIT to mode 3 (square wave) on channel 2
    outp(PIT_CONTROL_PORT, 0xB6); // 1011 0110

    // Send frequency divisor to channel 2 (low byte, then high byte)
    outp(PIT_CHANNEL2_PORT, divisor & 0xFF);        // Low byte
    outp(PIT_CHANNEL2_PORT, (divisor >> 8) & 0xFF); // High byte

    // Turn on the speaker (enable bit 0 and 1)
    tmp = inp(SPEAKER_CONTROL_PORT);
    outp(SPEAKER_CONTROL_PORT, tmp | 0x03);

    // Delay for # of frames
    while (frames--)
    {
        // Wait until vblank starts
        while(!(inp(0x3DA) & 0x08));
        // Wait until vblank stops
	    while(inp(0x3DA) & 0x08);
    }

    // Turn off the speaker (clear bit 0, keep bit 1 for PIT gate)
    tmp = inp(SPEAKER_CONTROL_PORT);
    outp(SPEAKER_CONTROL_PORT, tmp & ~0x03);

    // Wait for # of frames
    while (wait--)
    {
        // Wait until vblank starts
        while(!(inp(0x3DA) & 0x08));
        // Wait until vblank stops
	    while(inp(0x3DA) & 0x08);
    }
}

void initSound()
{
}

void soundJoinGame()
{
    beep(430,5,8);
    beep(340,5,0);
    beep(500,5,0);
}


void soundMyTurn()
{
    beep(430,4,2);
    beep(430,4,2);
}

void soundGameDone()
{
    beep(311,10,0);
    beep(330,20,0);
    beep(392,10,0);
    beep(415,20,0);
}

void soundCursor()
{
    // Wait until vblank starts
    while(!(inp(0x3DA) & 0x08));
    // Wait until vblank stops
    while(inp(0x3DA) & 0x08);
    beep(300,1,0);
}

void soundPlaceShip()
{
    beep(300,3,1);
    beep(350,3,0);
}

void soundTick()
{
    beep(100,1,0);
}

void soundSelect()
{
    beep(350,2,1);
    beep(250,2,0);
    beep(150,2,0);
}

void soundMiss()
{
    beep(150, 0, 1);
    beep(170, 1, 0);
}

void soundInvalid()
{
    beep(150, 2, 2);
    beep(150, 2, 0);
}

void soundAttack()
{
    
    uint8_t i;
    for (i = 0; i <3; i++) {
        beep(rand() % 2+90, 2, 0);
    }
    for (i = 0; i <2; i++) {
        beep(rand() % 2+70, 2, 0);
    }
    //for (i = 0; i <1; i++) {
        beep(rand() % 2+60, 2, 0);
    //}
}

void soundHit()
{
    uint8_t i;
    for (i = 70; i >= 30; i -= 10) {
        beep(i, 2, 0);
    }
}

void soundSink()
{
    uint8_t i;
    for (i = 120; i >= 60; i -= 10) {
        beep(i, 2, 0);
    }
}

// Not applicable to msdos
void soundStop() {}
void disableKeySounds() {}
void enableKeySounds() {}
