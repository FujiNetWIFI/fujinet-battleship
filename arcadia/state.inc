; state.inc -- reading the reply where it lies.
;
; The reply stays painted in the cart's 256-byte slice window until the
; next transaction, so nothing is buffered: code sets the flat cursor
; (RXPTR = slice, RXPTR+1 = offset) and then calls RXNEXT per byte.
; RXNEXT switches slices only when the cursor's slice differs from the
; cached CURSLC; every transaction ends with the cart republishing slice
; 0 and net.inc re-homing the cache.
;
; Battleship needs more of this than 5 Card Stud did: a player record
; lives at GIPLYRS + PLSTRID*i, which for 4 players runs to offset 509 --
; well into slice 1. RXSEEK sets the cursor from a flat 16-bit offset and
; lets RXNEXT stream from there; the 100-byte gamefield is then a purely
; sequential read, which is what RXNEXT is fast at.

; RXSEEK: set the reply cursor to the flat offset R1 (hi) : R2 (lo).
; Leaf. Clobbers R0. Preserves R1,R2.
RXSEEK: LODZ    R1
        STRA,R0 *PRXSLC
        LODZ    R2
        STRA,R0 *PRXOFF
        RETC,UN

; RXNEXT: R0 = the reply byte at the cursor; the cursor advances.
; PRESERVES R2 (render loops keep their screen cell there). Calls one
; down (MSLICE, on a slice cross only). Clobbers R0,R1,R3.
RXNEXT: LODA,R0 *PRXSLC
        COMA,R0 *PCURSL
        BCTR,EQ RXN1
        STRA,R2 *PSAV2          ; shelter the caller's R2
        STRZ    R2              ; wanted slice
        STRA,R0 *PCURSL
        BSTA,UN MSLICE
        LODA,R2 *PSAV2
RXN1:   LODA,R1 *PRXOFF
        LODA,R0 FNRDATA,R1      ; the byte
        STRZ    R3
        ADDI,R1 1
        STRA,R1 *PRXOFF
        BCFR,EQ RXN2            ; wrapped to 0: next slice
        LODA,R0 *PRXSLC
        ADDI,R0 1
        STRA,R0 *PRXSLC
RXN2:   LODZ    R3
        RETC,UN

; RXHDR: R0 = the header byte at offset R1 (0-37, always slice 0).
; Calls one down (RXNEXT). Clobbers R0,R1,R3. Preserves R2.
RXHDR:  LODZ    R1
        STRA,R0 *PRXOFF
        LODI,R0 0
        STRA,R0 *PRXSLC
        BCTA,UN RXNEXT          ; tail call

; PLSEEK: point the cursor at player record R1's field R2.
; offset = GIPLYRS + PLSTRID*R1 + R2, which for record 3 reaches 504 --
; well into slice 1 -- so it is done 16-bit off a base table rather than
; by repeated addition. The carry-out of the low add is read with
; TPSL $01 (STRA and LODI leave C alone; only arithmetic sets it).
; Leaf. Clobbers R0,R1,R3. R2 must hold the field offset on entry.
PLSEEK: LODZ    R1
        RRL,R0                  ; idx*2 (WC=0: circular, safe for idx<4)
        STRZ    R1
        LODA,R0 PLBTAB+1,R1     ; base low
        ADDZ    R2              ; + field: C is the carry into the slice
        STRA,R0 *PRXOFF
        LODI,R0 0
        TPSL    $01
        BCFR,EQ PSK1
        LODI,R0 1
PSK1:   ADDA,R0 PLBTAB,R1       ; + base high
        STRA,R0 *PRXSLC
        RETC,UN

; The four player-record bases, high byte first:
;   49, 164, 279, 394  =  GIPLYRS + 115*i
PLBTAB: DB      (GIPLYRS+0*PLSTRID)>>8, (GIPLYRS+0*PLSTRID)&$FF
        DB      (GIPLYRS+1*PLSTRID)>>8, (GIPLYRS+1*PLSTRID)&$FF
        DB      (GIPLYRS+2*PLSTRID)>>8, (GIPLYRS+2*PLSTRID)&$FF
        DB      (GIPLYRS+3*PLSTRID)>>8, (GIPLYRS+3*PLSTRID)&$FF

; LBSEEK: point the cursor at lobby player record R1's field R2.
; offset = GLPLYRS + 10*R1 + R2; max 59+30+9 = 98, always slice 0.
; Leaf. Clobbers R0,R1,R3.
LBSEEK: LODZ    R1
        RRL,R0
        STRZ    R3              ; idx*2
        RRL,R0
        RRL,R0                  ; idx*8
        ADDZ    R3              ; idx*10
        ADDI,R0 GLPLYRS
        ADDZ    R2
        STRA,R0 *PRXOFF
        LODI,R0 0
        STRA,R0 *PRXSLC
        RETC,UN
