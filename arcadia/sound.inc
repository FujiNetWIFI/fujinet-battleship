; sound.inc -- the 2637's single tone voice.
;
; Both sound registers are SHARED with the display, which is the whole
; gotcha here:
;   $18FD (PITCH)  b0-6 frequency, b7 is the MULTICOLOR bit -- every
;                  frequency write must preserve it.
;   $18FE (VOLUME) b0-2 loudness, b3 enable, b4 noise, b5-7 HORIZONTAL
;                  SHIFT -- volume writes touch b0-3 only so the shift
;                  stays 0 and the screen does not jump.
;
; Frequency relation (arcadia_v.cpp): 1/Freq = 2*(pitch+1)*line_period,
; so a HIGHER pitch value is a LOWER tone.
;
; Every cue is blocking and tail-calls into BEEP, so none of them costs
; a return-stack entry beyond BEEP's own. Nothing outside this file
; writes either register after DINIT.
;
; This module sits in BLOCK 2 purely for the ROM budget, so both
; registers are reached through the pointer words below.

PPITCH: DWBE    PITCH           ; sound.inc lives in block 2 for the ROM
PVOLUM: DWBE    VOLUME          ;   budget; the registers are page-0

; BEEP: frequency R1 (0-$7F), duration R3 frames.
; Calls one down (WAITVB). Clobbers R0,R3.
BEEP:   LODZ    R1
        IORI,R0 $80             ; keep multicolor on
        STRA,R0 *PPITCH
        LODI,R0 $0F             ; enable + full loudness, shift 0
        STRA,R0 *PVOLUM
BEEP1:  BSTA,UN WAITVB
        BDRR,R3 BEEP1
        EORZ    R0
        STRA,R0 *PVOLUM
        LODI,R0 $80
        STRA,R0 *PPITCH
        RETC,UN

; SNDMOVE: the targeting cursor stepped a cell -- a very short, high
; click. Deliberately 1 frame: it fires on every cursor move, including
; the auto-repeat, so anything longer would drag the cursor.
SNDMOVE: LODI,R1 $06
        LODI,R3 1
        BCTA,UN BEEP

; SNDSEL: a menu selection / a shot going out.
SNDSEL: LODI,R1 $10
        LODI,R3 4
        BCTA,UN BEEP

; SNDERR: rejected input (illegal placement, not your turn).
SNDERR: LODI,R1 $60
        LODI,R3 12
        BCTA,UN BEEP

; SNDMISS: a shot resolved as a miss -- low and short, a dull splash.
SNDMISS: LODI,R1 $48
        LODI,R3 6
        BCTA,UN BEEP

; SNDHIT: a shot resolved as a hit -- bright and a little longer.
; Calls one down. Clobbers R0,R1,R3.
SNDHIT: LODI,R1 $0A
        LODI,R3 5
        BSTA,UN BEEP
        LODI,R1 $04
        LODI,R3 5
        BCTA,UN BEEP

; SNDSUNK: a ship went down -- three descending tones (rising pitch
; VALUES), the most emphatic cue in the game.
; Calls one down. Clobbers R0,R1,R3.
SNDSUNK: LODI,R1 $08
        LODI,R3 6
        BSTA,UN BEEP
        LODI,R1 $20
        LODI,R3 6
        BSTA,UN BEEP
        LODI,R1 $48
        LODI,R3 12
        BCTA,UN BEEP

; SNDTRN: your turn just began -- a two-tone rising cue.
; Calls one down. Clobbers R0,R1,R3.
SNDTRN: LODI,R1 $18
        LODI,R3 6
        BSTA,UN BEEP
        LODI,R1 $0C
        LODI,R3 8
        BCTA,UN BEEP

; SNDPLC: a ship was committed to the board.
SNDPLC: LODI,R1 $14
        LODI,R3 3
        BCTA,UN BEEP

; SNDOVER: the game ended.
; Calls one down. Clobbers R0,R1,R3.
SNDOVER: LODI,R1 $0C
        LODI,R3 10
        BSTA,UN BEEP
        LODI,R1 $18
        LODI,R3 10
        BSTA,UN BEEP
        LODI,R1 $30
        LODI,R3 20
        BCTA,UN BEEP
