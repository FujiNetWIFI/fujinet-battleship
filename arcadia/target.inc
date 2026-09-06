; target.inc -- your turn: move the reticle, fire.
;
; Time-sliced, never blocking: the slice expires after TGTSLC frames and
; hands control back to the poll loop, so the move clock keeps running
; and the board keeps refreshing even if the player walks away. That is
; the family's "the clock bails" rule.
;
; One cursor, in lockstep: /attack carries only a position, and the shot
; lands on every opponent at once. Keypad 4/6 change WHICH opponent's
; board is on screen, which is a view choice and nothing more.

; This module sits in BLOCK 2 for the ROM budget, so the handful of
; page-0 cells it owns are reached through the pointer words at the
; bottom -- indirect addressing carries the full 15 bits.

TGTSLC  EQU     240             ; ~4 s at one KEYSCN per frame

TARGET: BSTA,UN CURSHOW
        LODI,R0 TGTSLC
        STRA,R0 *P_V_TICK
TGT1:   BSTA,UN KEYSCN
        COMI,R0 KNONE
        BCTR,EQ TGT8
        STRZ    R1
        COMI,R0 KUP             ; the four disc directions walk the
        BCTR,LT TGT2            ;   reticle; digits are below $80
        COMI,R0 KRIGHT
        BCTR,GT TGT2
        LODZ    R1
        BSTA,UN CURMOVE
        BCTR,UN TGT8
TGT2:   LODZ    R1
        COMI,R0 KENT            ; Enter or FIRE shoots
        BCTA,EQ TGTFIR
        COMI,R0 2
        BCTA,EQ TGTFIR
        COMI,R0 4               ; 4 / 6 page through the opponents
        BCTA,EQ TGTPRV
        COMI,R0 6
        BCTA,EQ TGTNXT
        COMI,R0 KCLR            ; Clear leaves the table
        BCTA,EQ TGTLV
        COMI,R0 0               ; 0 polls now
        BCTR,EQ TGT9
TGT8:   LODA,R0 *P_V_TICK
        SUBI,R0 1
        STRA,R0 *P_V_TICK
        BCFA,EQ TGT1
TGT9:   BCTA,UN CURHIDE         ; tail call

TGTFIR: LODA,R0 *P_CURY         ; position = y*10 + x
        BSTA,UN MUL10
        ADDA,R0 *P_CURX
        STRA,R0 *P_ATKPOS
        LODI,R0 RQATTCK
        STRA,R0 *P_PENDACT
        BSTA,UN SNDSEL
        BCTA,UN CURHIDE         ; tail call

TGTLV:  LODI,R0 RQLEAVE
        STRA,R0 *P_PENDACT
        BCTA,UN CURHIDE         ; tail call

TGTPRV: LODA,R0 *P_VIEWPL
        SUBI,R0 1
        STRA,R0 *P_VIEWPL
        BCTR,UN TGTVW
TGTNXT: LODA,R0 *P_VIEWPL
        ADDI,R0 1
        STRA,R0 *P_VIEWPL
TGTVW:  BSTA,UN VWCLMP
        BSTA,UN DRAWEB
        BSTA,UN FOOTER
        BSTA,UN CURSHOW
        BSTA,UN SNDMOVE
        BCTA,UN TGT8

; VWCLMP: keep VIEWPL on a real opponent. You are always index 0, so the
; opponents are 1 .. playerCount-1 and the view wraps between them.
; Leaf. Clobbers R0.
VWCLMP: LODA,R0 *P_V_PC
        COMI,R0 2
        BCTR,LT VWC0            ; nobody to look at
        LODA,R0 *P_VIEWPL
        COMI,R0 1
        BCTR,LT VWCHI           ; below 1: wrap to the last
        COMA,R0 *P_V_PC
        BCTR,LT VWC9            ; 1 <= VIEWPL < playerCount: fine
        LODI,R0 1               ; past the end: wrap to the first
        STRA,R0 *P_VIEWPL
        RETC,UN
VWCHI:  LODA,R0 *P_V_PC
        SUBI,R0 1
        STRA,R0 *P_VIEWPL
        RETC,UN
VWC0:   LODI,R0 1
        STRA,R0 *P_VIEWPL
VWC9:   RETC,UN

P_CURY:  DWBE    CURY
P_CURX:  DWBE    CURX
P_ATKPOS: DWBE    ATKPOS
P_PENDACT: DWBE    PENDACT
P_V_TICK: DWBE    V_TICK
P_VIEWPL: DWBE    VIEWPL
P_V_PC:  DWBE    V_PC
