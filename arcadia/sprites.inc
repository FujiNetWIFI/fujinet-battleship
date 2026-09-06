; sprites.inc -- the 2637's movable objects, trimmed to the one this
; game needs: sprite 0, the targeting reticle.
;
; Sprite images ARE the first four UDC slots, so sprite 0's shape is
; whatever sits at UDC $38 -- which is why GCURS is pinned there and the
; other seven UDCs carry the ship art. Sprites 1-3 are parked for good;
; their images ($39-$3B) are just ordinary glyphs to us.
;
; Position registers take raw values: y raw = 256 - y_internal,
; x raw = x_internal + 43 (the MAME model; SPYOFF/SPXOFF absorb the
; calibration against real cells, tuned by DEMO screenshot).

SPYOFF  EQU     0               ; added to row*8 before negation
SPXOFF  EQU     43              ; added to col*8

; SPRINI: colour, size, and park everything. Sprite 0 is white and 8x8;
; the rest are parked offscreen. Leaf. Clobbers R0,R3.
SPRINI: LODI,R0 $C0             ; b7/b6 8x8, spr0 white(0), spr1 white(0)
        STRA,R0 SPRC01
        LODI,R0 $C0
        STRA,R0 SPRC23
        LODI,R3 7
SPIN1:  EORZ    R0
        STRA,R0 SPRPOS,R3
        BDRR,R3 SPIN1
        EORZ    R0
        STRA,R0 SPRPOS
        RETC,UN

; SPRSET: sprite R1 (0-3) to raw y=R2, x=R3. Leaf. Clobbers R0,R1.
SPRSET: LODZ    R1
        RRL,R0
        STRZ    R1              ; index*2
        LODZ    R2
        STRA,R0 SPRPOS,R1
        LODZ    R3
        STRA,R0 SPRPOS+1,R1
        RETC,UN

; SPRCELL: sprite R1 at cell row R2 / col R3. Cells are 8x8 in 26-line
; mode, so the mapping is exact. Calls one down (SPRSET).
; Clobbers R0,R1,R2,R3.
SPRCELL: LODZ   R2
        RRL,R0
        RRL,R0
        RRL,R0                  ; row*8
        ADDI,R0 SPYOFF
        EORI,R0 $FF             ; 256 - v
        ADDI,R0 1
        STRZ    R2
        LODZ    R3
        RRL,R0
        RRL,R0
        RRL,R0                  ; col*8
        ADDI,R0 SPXOFF
        STRZ    R3
        BCTR,UN SPRSET          ; tail call

; SPRPARK: park sprite R1 offscreen. Calls one down. Clobbers R0-R3.
SPRPARK: LODI,R2 0
        LODI,R3 0
        BCTR,UN SPRSET          ; tail call

; CURSHOW: put the reticle on the enemy board at the cursor (CURX,CURY).
; Calls one down (SPRCELL). Clobbers R0-R3.
CURSHOW: LODA,R0 CURY
        ADDI,R0 EBROW           ; board row 0 lives at screen row EBROW
        STRZ    R2
        LODA,R0 CURX
        ADDI,R0 BCOL            ; board col 0 lives at screen col BCOL
        STRZ    R3
        LODI,R1 0
        BCTA,UN SPRCELL         ; tail call

; CURHIDE: take the reticle off screen. Calls one down. Clobbers R0-R3.
CURHIDE: LODI,R1 0
        BCTA,UN SPRPARK         ; tail call

; CURMOVE: act on a control from KEYSCN -- move the reticle one cell if
; it was a disc direction, clamped to the board, with the move click.
; Anything else is ignored and returns quietly.
; Calls two down (CURSHOW -> SPRCELL, or SNDMOVE -> BEEP).
; Clobbers R0-R3.
CURMOVE: COMI,R0 KUP
        BCTR,EQ CMUP
        COMI,R0 KDOWN
        BCTR,EQ CMDN
        COMI,R0 KLEFT
        BCTR,EQ CMLF
        COMI,R0 KRIGHT
        BCTR,EQ CMRT
        RETC,UN
CMUP:   LODA,R0 CURY
        BCTR,EQ CM9             ; already against the edge
        SUBI,R0 1
        STRA,R0 CURY
        BCTR,UN CMOK
CMDN:   LODA,R0 CURY
        ADDI,R0 1
        COMI,R0 BRDDIM
        BCTR,EQ CM9
        STRA,R0 CURY
        BCTR,UN CMOK
CMLF:   LODA,R0 CURX
        BCTR,EQ CM9
        SUBI,R0 1
        STRA,R0 CURX
        BCTR,UN CMOK
CMRT:   LODA,R0 CURX
        ADDI,R0 1
        COMI,R0 BRDDIM
        BCTR,EQ CM9
        STRA,R0 CURX
CMOK:   BSTA,UN CURSHOW
        BCTA,UN SNDMOVE         ; tail call
CM9:    RETC,UN
