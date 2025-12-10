#include <peekpoke.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "../fujinet-fuji.h"
#include "../platform-specific/graphics.h"

// C64 timer location - CIA1 timer A
#define CIA1_TIMER_A_LO 0xDC04
#define CIA1_TIMER_A_HI 0xDC05

void resetTimer()
{
    // Reset CIA timer - set to count from $FFFF
    POKE(CIA1_TIMER_A_LO, 0xFF);
    POKE(CIA1_TIMER_A_HI, 0xFF);
}

uint16_t getTime()
{
    // Read CIA timer A (counts down from $FFFF at 1 MHz)
    uint16_t timer = PEEK(CIA1_TIMER_A_HI) * 256 + PEEK(CIA1_TIMER_A_LO);
    // Return elapsed time (invert to show progression)
    return 0xFFFF - timer;
}

void quit()
{
    resetScreen();
    resetGraphics();
    exit(0);
}

void housekeeping()
{
}

uint8_t getJiffiesPerSecond()
{
    uint8_t tvSystem = PEEK(0x02A6);
    
    if (tvSystem == 1) {
        return 50;  // PAL
    } else {
        return 60;  // NTSC
    }
}

uint8_t getRandomNumber(uint8_t maxExclusive)
{
    if (maxExclusive == 0) return 0;

    srand(clock());  // Seed with clock for pseudo-randomness, but maybe we can use Sid Meyer's trick with SID noise?
    return (uint8_t)(rand() % maxExclusive);

    // OR we can use the SID noise generator for better randomness on C64

    // // Use SID noise generator for randomness while preserving existing SID state
    // #define SID_V3_FREQ_LO  0xD40E
    // #define SID_V3_FREQ_HI  0xD40F
    // #define SID_V3_CTRL     0xD412
    // #define SID_V3_OSC      0xD41B
    // #define SID_VOLUME      0xD418

    // // Save current voice-3 related registers so we can restore them
    // uint8_t prev_lo   = PEEK(SID_V3_FREQ_LO);
    // uint8_t prev_hi   = PEEK(SID_V3_FREQ_HI);
    // uint8_t prev_ctrl = PEEK(SID_V3_CTRL);
    // uint8_t prev_vol  = PEEK(SID_VOLUME);

    // // Configure voice 3 for noise: set a small non-zero frequency, enable noise+gate
    // POKE(SID_V3_FREQ_LO, 0xFF);
    // POKE(SID_V3_FREQ_HI, 0xFF);
    // POKE(SID_V3_CTRL, 0x81); // noise waveform (0x80) + gate on (0x01)

    // // Mute audio output by clearing the lower nibble of volume
    // POKE(SID_VOLUME, (prev_vol & 0xF0) | 0x00);

    // // Read oscillator a couple of times and mix to reduce correlation
    // uint8_t a = PEEK(SID_V3_OSC);
    // uint8_t b = PEEK(SID_V3_OSC);
    // uint8_t mixed = a ^ b;

    // // Restore previous SID registers so music/voice 3 remain unchanged
    // POKE(SID_V3_FREQ_LO, prev_lo);
    // POKE(SID_V3_FREQ_HI, prev_hi);
    // POKE(SID_V3_CTRL, prev_ctrl);
    // POKE(SID_VOLUME, prev_vol);

    // return (uint8_t)(mixed % maxExclusive);
}
