; nament.inc -- the title + name-entry screen (13-line doublescan mode).
;
; Eight slots, edited with the disc and keypad: disc up/down turns the
; character wheel A-Z 0-9 then '-', disc left/right (or keypad 4/6) move
; the cursor, Clear blanks the slot and steps back, Enter (or FIRE)
; accepts. '-' is the blank -- the wheel has no space, so a name can
; never need the '+' URL escape -- and everything from the first '-' on
; is dropped at accept. An all-blank name keeps what PLNBUF had (the
; "ARCADIA" default).
;
; The edit buffer lives in the $1A00 scratch that 13-line mode leaves
; free; GSTATIC clears it before the flip to 26 lines, where those bytes
; become visible screen.

V_NTMP  EQU     $1A09           ; NEDRAW's slot walker (13-line scratch)

NAMESCR: BSTA,UN KEYRAW         ; a held key must not leak in
        STRA,R0 V_KEY
        BSTA,UN DINI13
        BSTA,UN DCLSU
        BSTA,UN SPRINI
        DAT     1,3
        DCOL    AWATER
        SETSTR  TTITLE
        BSTA,UN DPRINT
        DAT     4,3
        SETSTR  TNAME
        BSTA,UN DPRINT
        DAT     9,1
        SETSTR  TEDIT
        BSTA,UN DPRINT
        DAT     10,1
        SETSTR  TACCPT
        BSTA,UN DPRINT
        LODI,R3 7               ; edit buffer = the name padded with '-'
NEIN1:  LODA,R0 PLNBUF,R3
        BCFR,EQ NEIN2           ; walking down, so a NUL (and the bytes
        LODI,R0 '-'             ; past it) blanks; real characters copy
NEIN2:  STRA,R0 NAMEED,R3
        BDRR,R3 NEIN1
        LODA,R0 PLNBUF
        STRA,R0 NAMEED
        EORZ    R0
        STRA,R0 V_NCUR
        BSTA,UN NEDRAW
NEKEY:  BSTA,UN GETKEY
        COMI,R0 KUP
        BCFR,EQ NEK1
        LODI,R1 1               ; wheel forward
        BCTA,UN NEWHL
NEK1:   COMI,R0 KDOWN
        BCFR,EQ NEK2
        LODI,R1 36              ; wheel back (mod 37)
        BCTA,UN NEWHL
NEK2:   COMI,R0 KLEFT           ; disc left or keypad 4: cursor left
        BCTR,EQ NEK2A
        COMI,R0 4
        BCFR,EQ NEK3
NEK2A:  LODA,R0 V_NCUR
        BCTA,EQ NEKEY
        SUBI,R0 1
        STRA,R0 V_NCUR
        BCTA,UN NEDRW
NEK3:   COMI,R0 KRIGHT          ; disc right or keypad 6: cursor right
        BCTR,EQ NEK3A
        COMI,R0 6
        BCFR,EQ NEK4
NEK3A:  LODA,R0 V_NCUR
        COMI,R0 7
        BCFA,LT NEKEY
        ADDI,R0 1
        STRA,R0 V_NCUR
        BCTA,UN NEDRW
NEK4:   COMI,R0 KCLR            ; Clear: blank the slot, step back
        BCFR,EQ NEK5
        LODA,R1 V_NCUR
        LODI,R0 '-'
        STRA,R0 NAMEED,R1
        LODA,R0 V_NCUR
        BCTR,EQ NEK4A
        SUBI,R0 1
        STRA,R0 V_NCUR
NEK4A:  BCTA,UN NEDRW
NEK5:   COMI,R0 KENT            ; Enter or FIRE accepts
        BCTR,EQ NEACC
        COMI,R0 2
        BCFA,EQ NEKEY
NEACC:  LODI,R3 0               ; copy up to the first '-'
NEAC1:  COMI,R3 8
        BCTR,EQ NEAC9
        LODA,R0 NAMEED,R3
        COMI,R0 '-'
        BCTR,EQ NEAC9
        STRA,R0 PLNBUF,R3
        ADDI,R3 1
        BCTR,UN NEAC1
NEAC9:  COMI,R3 0               ; an empty name keeps what PLNBUF had
        BCTA,EQ NEKEY
NEAC2:  COMI,R3 9               ; NUL-fill the rest so stale characters
        BCTR,EQ NEACD           ; never leak back into the editor
        EORZ    R0
        STRA,R0 PLNBUF,R3
        ADDI,R3 1
        BCTR,UN NEAC2
NEACD:  BSTA,UN SNDSEL
        RETC,UN

NEWHL:  LODA,R2 V_NCUR          ; step the wheel at the cursor by R1
        LODA,R0 NAMEED,R2
        LODI,R3 0               ; find its wheel index
NEWH1:  COMA,R0 WHEEL,R3
        BCTR,EQ NEWH2
        ADDI,R3 1
        BCTR,UN NEWH1
NEWH2:  LODZ    R3
        ADDZ    R1
NEWH3:  COMI,R0 37
        BCTR,LT NEWH4
        SUBI,R0 37
        BCTR,UN NEWH3
NEWH4:  STRZ    R3
        LODA,R0 WHEEL,R3
        STRA,R0 NAMEED,R2
NEDRW:  BSTA,UN SNDMOVE
        BSTA,UN NEDRAW
        BCTA,UN NEKEY

; NEDRAW: the eight slots at row 6, cols 4-11, cursor cell inverse.
; Calls two down (DXLAT / DPUTC). Clobbers R0-R3, V_NTMP.
NEDRAW: EORZ    R0
        STRA,R0 V_NTMP
NED1:   LODA,R0 V_NTMP
        COMI,R0 8
        BCFR,LT NED9
        LODI,R1 6
        ADDI,R0 4
        STRZ    R2
        BSTA,UN DPOS
        LODA,R0 V_NTMP
        COMA,R0 V_NCUR
        BCFR,EQ NED2
        LODI,R0 ATEXT           ; the cursor cell is inverse
        BCTR,UN NED3
NED2:   LODI,R0 AWATER
NED3:   STRA,R0 DCOLOR
        LODA,R1 V_NTMP
        LODA,R0 NAMEED,R1
        BSTA,UN DXLAT
        BSTA,UN DPUTC
        LODA,R0 V_NTMP
        ADDI,R0 1
        STRA,R0 V_NTMP
        BCTR,UN NED1
NED9:   RETC,UN

WHEEL:  DB      "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-"
