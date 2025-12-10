/*
  Platform specific sound functions for Commodore 64
*/

#include <stdint.h>
#include <stdlib.h>
#include <peekpoke.h>
#include "../misc.h"
#include "../platform-specific/sound.h"
#include "../platform-specific/util.h"

// C64 SID chip registers
#define SID_BASE 0xD400

#define SID_FREQ_LO 0xD400
#define SID_FREQ_HI 0xD401
#define SID_PW_LO 0xD402
#define SID_PW_HI 0xD403
#define SID_CONTROL 0xD404
#define SID_ATTACK_DECAY 0xD405
#define SID_SUSTAIN_RELEASE 0xD406

#define SID_FILTER_FREQ_LO 0xD415
#define SID_FILTER_FREQ_HI 0xD416
#define SID_FILTER_RES_FILT 0xD417
#define SID_FILTER_MODE_VOL 0xD418

// Gate bit in control register
#define GATE_ON 0x01
#define GATE_OFF 0x00

// Waveform bits
#define WAVEFORM_TRIANGLE 0x10
#define WAVEFORM_SAWTOOTH 0x20
#define WAVEFORM_PULSE 0x40
#define WAVEFORM_NOISE 0x80

uint16_t ii;

void initSound()
{
    POKE(SID_FILTER_MODE_VOL, 0x0F);  // Set volume to max, no filter
}

// More capable play function: frequency is 16-bit SID tone value
// duration is in milliseconds
void playToneFull(uint16_t frequency, uint8_t duration, uint8_t waveform, uint16_t pulseWidth, uint8_t attack_decay, uint8_t sustain_release)
{
    uint16_t elapsed = 0;
    uint16_t duration_ticks = 0;
    uint16_t release_ticks = 0;

    if (prefs.disableSound)
        return;

    // Set frequency (16-bit)
    POKE(SID_FREQ_LO, frequency & 0xFF);
    POKE(SID_FREQ_HI, (frequency >> 8) & 0xFF);

    // Set pulse width (12-bit) - low and high parts
    POKE(SID_PW_LO, pulseWidth & 0xFF);
    POKE(SID_PW_HI, (pulseWidth >> 8) & 0x0F);

    // Set envelope (ADSR)
    POKE(SID_ATTACK_DECAY, attack_decay);
    POKE(SID_SUSTAIN_RELEASE, sustain_release);

    // Gate on with requested waveform
    POKE(SID_CONTROL, waveform | GATE_ON);

    // Play for specified duration in milliseconds
    resetTimer();
    duration_ticks = (uint16_t)duration * 1000;  // Convert ms to approximate clock ticks
    while (elapsed < duration_ticks) {
        elapsed = getTime();
    }

    // Gate off (stop sound) - this triggers RELEASE phase
    POKE(SID_CONTROL, waveform);
    
    // Wait for release phase to complete (approximately 100-200ms for typical release times)
    resetTimer();
    elapsed = 0;
    release_ticks = 150000;  // Wait ~150ms for release
    while (elapsed < release_ticks) {
        elapsed = getTime();
    }
}

void soundCursor()
{
    if (prefs.disableSound)
        return;

    // short pulse clicks (narrow pulsewidth)
    playToneFull(1678, 33, WAVEFORM_PULSE, 0x0400, 0x96, 0x33);
    playToneFull(2013, 33, WAVEFORM_PULSE, 0x0400, 0x96, 0x33);
    playToneFull(2349, 33, WAVEFORM_PULSE, 0x0400, 0x96, 0x33);
}

void soundSelect()
{
    if (prefs.disableSound)
        return;

    // Modern selection sound: quick bright rising arpeggio followed by a small sparkle
    playToneFull(3691, 16, WAVEFORM_PULSE, 0x0400, 0x94, 0x33);
    playToneFull(4362, 16, WAVEFORM_PULSE, 0x0600, 0x84, 0x33);
    playToneFull(5033, 33, WAVEFORM_PULSE, 0x0800, 0x84, 0x44);
    // small high sparkle to finish
    playToneFull(13422, 16, WAVEFORM_TRIANGLE, 0, 0x92, 0x33);
}

void soundStop()
{
    // Stop any playing sound
    POKE(SID_CONTROL, 0);
}

void soundJoinGame()
{
    if (prefs.disableSound)
        return;

    // Cheerful short melody using triangle with gentle envelope
    playToneFull(1342, 100, WAVEFORM_TRIANGLE, 0, 0x63, 0x54);
    playToneFull(1007, 100, WAVEFORM_TRIANGLE, 0, 0x63, 0x54);
    playToneFull(1678, 133, WAVEFORM_TRIANGLE, 0, 0x63, 0x54);
}

void soundMyTurn()
{
    uint8_t prevCtrl1, prevCtrl2, prevFilter;
    uint16_t v1freq, v2freq;
    if (prefs.disableSound)
        return;

    // Bell/gong-like: use two voices with ring modulation to produce metallic overtones.
    // Save previous control/filter registers so we can restore them.
    prevCtrl1 = PEEK(SID_CONTROL);
    prevCtrl2 = PEEK(SID_CONTROL + 7);
    prevFilter = PEEK(SID_FILTER_MODE_VOL);

    // Tweak filter to allow some body for the gong
    POKE(SID_FILTER_MODE_VOL, (prevFilter & 0xF0) | 0x08);

    // Voice 1: lower fundamental (triangle)
    v1freq = 2517;
    POKE(SID_FREQ_LO, v1freq & 0xFF);
    POKE(SID_FREQ_HI, (v1freq >> 8) & 0xFF);
    POKE(SID_ATTACK_DECAY, 0xF4);    // fast attack, moderate decay
    POKE(SID_SUSTAIN_RELEASE, 0x44);  // medium sustain, medium release

    // Voice 2: higher partial to ring-modulate (triangle), placed at voice2 registers (+7)
    v2freq = 15099;
    POKE(SID_FREQ_LO + 7, v2freq & 0xFF);
    POKE(SID_FREQ_HI + 7, (v2freq >> 8) & 0xFF);
    POKE(SID_ATTACK_DECAY + 7, 0xE4);   // quick attack, longer decay
    POKE(SID_SUSTAIN_RELEASE + 7, 0x33); // lower sustain, moderate release

    // Gate both voices on; enable ring modulation on voice2 (bit 0x04)
    POKE(SID_CONTROL, WAVEFORM_TRIANGLE | GATE_ON);
    POKE(SID_CONTROL + 7, WAVEFORM_TRIANGLE | GATE_ON | 0x04);

    // Let the gong ring for a short period (coarse frames)
    {
        uint8_t t = 20;
        while (t--)
            for (ii = 0; ii < 200; ii++)
                ;
    }

    // Gate off and restore previous control/filter states
    POKE(SID_CONTROL + 7, WAVEFORM_TRIANGLE);
    POKE(SID_CONTROL, WAVEFORM_TRIANGLE);
    POKE(SID_FILTER_MODE_VOL, prevFilter);
}

void soundGameDone()
{
    uint8_t i;
    uint8_t prevFilter;
    if (prefs.disableSound)
        return;

    // Modern end-game fanfare: rising triumphant pulse notes, then a final bell chord.
    // Victory march: rising pulse tones with wider pulse width and snappy ADSR
    playToneFull(3355, 67, WAVEFORM_PULSE, 0x0800, 0x84, 0x44);
    playToneFull(4027, 67, WAVEFORM_PULSE, 0x0A00, 0x84, 0x44);
    playToneFull(4698, 67, WAVEFORM_PULSE, 0x0C00, 0x84, 0x44);
    playToneFull(5369, 100, WAVEFORM_PULSE, 0x0E00, 0x74, 0x55);

    // Brief pause
    {
        uint8_t t = 2;
        while (t--)
            for (ii = 0; ii < 100; ii++)
                ;
    }

    // Final triumphant bell: two-voice gong with ring modulation
    prevFilter = PEEK(SID_FILTER_MODE_VOL);
    POKE(SID_FILTER_MODE_VOL, (prevFilter & 0xF0) | 0x0F);

    // Voice 1: low bell fundamental
    POKE(SID_FREQ_LO, 2517 & 0xFF);
    POKE(SID_FREQ_HI, (2517 >> 8) & 0xFF);
    POKE(SID_ATTACK_DECAY, 0x64);    // gentle attack
    POKE(SID_SUSTAIN_RELEASE, 0x36);  // lower sustain for better decay, longer release

    // Voice 2: higher bell partial with ring mod
    POKE(SID_FREQ_LO + 7, (20133 & 0xFF));
    POKE(SID_FREQ_HI + 7, ((20133 >> 8) & 0xFF));
    POKE(SID_ATTACK_DECAY + 7, 0x54);   // quick attack
    POKE(SID_SUSTAIN_RELEASE + 7, 0x35); // lower sustain, longer ring

    // Gate both voices on with ring modulation on voice2
    POKE(SID_CONTROL, WAVEFORM_TRIANGLE | GATE_ON);
    POKE(SID_CONTROL + 7, WAVEFORM_TRIANGLE | GATE_ON | 0x04);

    // Let the bell ring (coarse frames)
    {
        uint8_t t = 30;
        while (t--)
            for (ii = 0; ii < 200; ii++)
                ;
    }

    // Gate off
    POKE(SID_CONTROL + 7, WAVEFORM_TRIANGLE);
    POKE(SID_CONTROL, WAVEFORM_TRIANGLE);
    POKE(SID_FILTER_MODE_VOL, prevFilter);
}

void soundTick()
{
    if (prefs.disableSound)
        return;

    // Clock-like tick: sharp high click (narrow pulse, very fast attack) + subtle low body
    // Click: narrow pulse, very short
    playToneFull(15099, 67, WAVEFORM_PULSE, 0x0200, 0x96, 0x36);
    // Subtle body to give weight to the tick
    playToneFull(2013, 83, WAVEFORM_TRIANGLE, 0, 0x86, 0x33);
}

void soundPlaceShip()
{
    uint8_t i;
    uint8_t prevFilter;
    if (prefs.disableSound)
        return;

    // Low foghorn: a long low triangle with slow attack and long release,
    // a detuned pulse underneath for thickness and two short echoes.
    prevFilter = PEEK(SID_FILTER_MODE_VOL);
    // emphasize low frequencies for the horn
    POKE(SID_FILTER_MODE_VOL, (prevFilter & 0xF0) | 0x08);

    // Main horn: deep, slow attack, long sustain/release
    playToneFull(1007, 250, WAVEFORM_TRIANGLE, 0, 0x63, 0x48);

    // Supporting detuned pulse for body
    playToneFull(1208, 167, WAVEFORM_PULSE, 0x0A00, 0x84, 0x36);

    // Two trailing short echoes
    for (i = 0; i < 2; i++)
    {
        playToneFull(1007, 67, WAVEFORM_TRIANGLE, 0, 0x84, 0x34);
    }

    // restore filter/volume
    POKE(SID_FILTER_MODE_VOL, prevFilter);
}

void soundAttack()
{
    uint8_t i;
    uint16_t freq;
    uint16_t freqs[] = {3355, 3188, 3020, 2852, 2684, 2517, 2349, 2181, 2013, 1845};
    if (prefs.disableSound)
        return;

    // Whoosh / attack: descending sawtooth with quick decay
    // Frequencies: 3355, 3188, 3020, 2852, 2684, 2517, 2349, 2181, 2013, 1845
    for (i = 0; i < 10; i++)
    {
        playToneFull(freqs[i], 83, WAVEFORM_SAWTOOTH, 0, 0x84, 0x44);
    }
}

void soundInvalid()
{
    if (prefs.disableSound)
        return;

    // Short dissonant buzzer using narrow pulsewidth and fast envelope
    playToneFull(2013, 50, WAVEFORM_PULSE, 0x0100, 0x98, 0x33);
    playToneFull(1510, 50, WAVEFORM_PULSE, 0x0200, 0x98, 0x33);
    playToneFull(1174, 50, WAVEFORM_PULSE, 0x0300, 0x98, 0x33);
}

void soundHit()
{
    uint8_t i;
    uint8_t prevFilter;
    uint16_t freqs[] = {5033, 4362, 3691, 3020, 2349, 1678};
    if (prefs.disableSound)
        return;

    // Explosion / hit: layered noise with falling tone and punchy envelope
    // Increase resonance/lowpass to get a boom-like noise.
    // Temporarily tweak filter mode/volume register to emphasize low end.
    prevFilter = PEEK(SID_FILTER_MODE_VOL);
    // Set filter mode to lowpass with some volume (keep top nibble as mode, low nibble volume)
    POKE(SID_FILTER_MODE_VOL, (prevFilter & 0xF0) | 0x0F);

    // Explosion frequencies: 5033, 4362, 3691, 3020, 2349, 1678
    for (i = 0; i < 6; i++)
    {
        // Start bright and fall down
        playToneFull(freqs[i], 100, WAVEFORM_NOISE, 0, 0x84, 0x55);
    }

    // restore filter/volume
    POKE(SID_FILTER_MODE_VOL, prevFilter);
}

void soundMiss()
{
    uint8_t i;
    uint8_t prevFilter;
    uint16_t ripple_freqs[] = {3355, 3859, 4362, 4865};
    if (prefs.disableSound)
        return;

    // Stone-into-water: a low, short noise "splash" followed by small high-pitched ripples
    prevFilter = PEEK(SID_FILTER_MODE_VOL);
    // emphasize low end for splash
    POKE(SID_FILTER_MODE_VOL, (prevFilter & 0xF0) | 0x0F);

    // Splash: short noise burst with punchy envelope
    playToneFull(5033, 100, WAVEFORM_NOISE, 0, 0x84, 0x55);

    // Ripples: quick triangle ticks with tiny noise taps in between
    // Ripple frequencies: 3355, 3859, 4362, 4865
    for (i = 0; i < 4; i++)
    {
        playToneFull(ripple_freqs[i], 50, WAVEFORM_TRIANGLE, 0, 0x64, 0x33);
        playToneFull(0, 16, WAVEFORM_NOISE, 0, 0xF8, 0x11);
    }

    // restore filter/volume
    POKE(SID_FILTER_MODE_VOL, prevFilter);
}

void disableKeySounds()
{
}

void enableKeySounds()
{
}
