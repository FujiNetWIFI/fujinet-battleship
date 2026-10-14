; board.inc -- board geometry and rendering.
;
; The screen is 16x26 and a framed 10x10 board is exactly 12 rows, so two
; of them plus a status row and a footer row fill it exactly:
;
;   row  0     status / server prompt + move clock
;   row  1     enemy frame top
;   rows 2-11  enemy board            cols 13-15: ship pips
;   row 12     enemy frame bottom
;   ------------ screen-half boundary ($18CF / $1A00) ------------
;   row 13     your frame top
;   rows 14-23 your board             cols 13-15: ship pips
;   row 24     your frame bottom
;   row 25     your name / cursor coordinate / viewed opponent
;
; The split falls exactly between the two boards, so EACH BOARD LIVES
; WHOLLY INSIDE ONE SCREEN HALF and no board render ever moves SCRP.
;
; The frames are drawn from the 2637's BUILT-IN graphics glyphs ($04-$0B
; are a complete box-drawing kit), which costs no UDCs and leaves all
; eight for the battleship shapes.
;
; Cell painting is opaque overdraw: every poll repaints both boards in
; full, so nothing tracks dirty rectangles.

BCOL    EQU     1               ; board column 0 -> screen column 1
BFLCOL  EQU     0               ; frame left
BFRCOL  EQU     BCOL+BRDDIM     ; frame right (11)
PIPCOL  EQU     13              ; ship-pip column

EBTOP   EQU     1               ; enemy frame top row
EBROW   EQU     EBTOP+1         ; enemy board row 0 (2)
EBBOT   EQU     EBROW+BRDDIM    ; enemy frame bottom (12)
YBTOP   EQU     13              ; your frame top row
YBROW   EQU     YBTOP+1         ; your board row 0 (14)
YBBOT   EQU     YBROW+BRDDIM    ; your frame bottom (24)
STATROW EQU     0
FOOTROW EQU     25

NOPLC   EQU     $FF             ; a SHIPS slot with nothing committed yet

SHPSIZ: DB      5,4,3,3,2       ; carrier, battleship, cruiser, sub, destroyer

; ---- small arithmetic --------------------------------------------------
; MUL10: R0 = v (0-9) -> R0 = 10*v. 8v + v + v, so no wrap concerns with
; WC off (RRL is a circular rotate). Leaf. Clobbers R0,R1.
MUL10:  STRZ    R1
        RRL,R0
        RRL,R0
        RRL,R0                  ; 8v
        ADDZ    R1
        ADDZ    R1              ; 10v
        RETC,UN

; ---- frames ------------------------------------------------------------
; BRDFRM: draw one board's frame; R1 = the frame's TOP row.
; Calls two down (DPOS, DFILL->DPUTC). Clobbers R0,R1,R2,R3, V_ROW.
BRDFRM: STRA,R1 V_ROW
        LODI,R0 AWATER
        STRA,R0 DCOLOR
        LODI,R2 BFLCOL          ; top: corner, ten edges, corner
        BSTA,UN DPOS
        LODI,R0 GFTL
        BSTA,UN DPUTC
        LODI,R1 GFTOP
        LODI,R3 BRDDIM
        BSTA,UN DFILL
        LODI,R0 GFTR
        BSTA,UN DPUTC
        LODA,R0 V_ROW           ; the ten side rows. DPOS and DPUTC leave
        ADDI,R0 1               ; R3 alone, so the counter needs no spill.
        STRA,R0 V_BTMP
        LODI,R3 BRDDIM
BFR1:   LODA,R0 V_BTMP
        STRZ    R1
        LODI,R2 BFLCOL
        BSTA,UN DPOS
        LODI,R0 GFLFT
        BSTA,UN DPUTC
        LODA,R0 V_BTMP
        STRZ    R1
        LODI,R2 BFRCOL
        BSTA,UN DPOS
        LODI,R0 GFRGT
        BSTA,UN DPUTC
        LODA,R0 V_BTMP
        ADDI,R0 1
        STRA,R0 V_BTMP
        BDRR,R3 BFR1
        LODA,R0 V_ROW           ; bottom: corner, ten edges, corner
        ADDI,R0 BRDDIM+1
        STRZ    R1
        LODI,R2 BFLCOL
        BSTA,UN DPOS
        LODI,R0 GFBL
        BSTA,UN DPUTC
        LODI,R1 GFBOT
        LODI,R3 BRDDIM
        BSTA,UN DFILL
        LODI,R0 GFBR
        BCTA,UN DPUTC           ; tail call

; ---- cells -------------------------------------------------------------
; CELLPT: paint one gamefield cell; R0 = 0 unknown / 1 hit / 2 miss.
; Calls one down (DPUTC). Clobbers R0,R2(+1), DCOLOR.
CELLPT: COMI,R0 CELLHIT
        BCTR,EQ CPHIT
        COMI,R0 CELLMIS
        BCTR,EQ CPMISS
        LODI,R0 AWATER          ; unknown: open sea
        STRA,R0 DCOLOR
        LODI,R0 0
        BCTA,UN DPUTC
CPHIT:  LODI,R0 AHIT
        STRA,R0 DCOLOR
        LODI,R0 GHIT
        BCTA,UN DPUTC
CPMISS: LODI,R0 AWATER
        STRA,R0 DCOLOR
        LODI,R0 GMISS
        BCTA,UN DPUTC

; CELLOV: like CELLPT but an OVERLAY -- value 0 leaves what is already
; there (your ships) and merely steps the cell cursor. The dummy read
; through the auto-increment is the cheapest way to advance R2.
; Calls one down. Clobbers R0,R2(+1).
CELLOV: COMI,R0 0
        BCFR,EQ CELLPT
        LODA,R0 *SCRP,R2,+
        RETC,UN

; ---- board renderers ---------------------------------------------------
; DRAWEB: repaint the enemy board from the viewed opponent's gamefield.
; 100 sequential RXNEXT reads -- the cursor is seeked once and RXNEXT
; crosses the slice boundary itself (a record 3 gamefield runs past 256).
; RXNEXT preserves R2, which is exactly the screen cell cursor, so the
; read and the paint interleave with no spilling.
; Calls three down (RXNEXT -> MSLICE). Clobbers R0-R3, V_ROW, V_COL.
DRAWEB: LODA,R0 VIEWPL
        STRZ    R1
        LODI,R2 PLFIELD
        BSTA,UN PLSEEK
        LODI,R0 0
        STRA,R0 V_ROW
EBR1:   LODA,R0 V_ROW
        ADDI,R0 EBROW
        STRZ    R1
        LODI,R2 BCOL
        BSTA,UN DPOS
        LODI,R0 BRDDIM
        STRA,R0 V_COL
EBC1:   BSTA,UN RXNEXT
        BSTA,UN CELLPT
        LODA,R0 V_COL
        SUBI,R0 1
        STRA,R0 V_COL
        BCFR,EQ EBC1
        LODA,R0 V_ROW
        ADDI,R0 1
        STRA,R0 V_ROW
        COMI,R0 BRDDIM
        BCFA,EQ EBR1
        RETC,UN

; DRAWYB: repaint your board -- open sea, then your five ships from
; myShips[0-4], then the gamefield OVERLAID so incoming hits and misses
; sit on top of the hulls. Order is the whole point: ships first, damage
; second. Calls three down. Clobbers R0-R3 and the board temporaries.
DRAWYB: BSTA,UN YBCLR
        LODI,R0 GIMYSHP         ; your own placements
        STRZ    R1
        BSTA,UN RXHDR           ; seeks and reads myShips[0]
        BSTA,UN YSHIPS
        LODI,R1 0               ; you are always player index 0
        LODI,R2 PLFIELD
        BSTA,UN PLSEEK
        LODI,R0 0
        STRA,R0 V_ROW
YBR1:   LODA,R0 V_ROW
        ADDI,R0 YBROW
        STRZ    R1
        LODI,R2 BCOL
        BSTA,UN DPOS
        LODI,R0 BRDDIM
        STRA,R0 V_COL
YBC1:   BSTA,UN RXNEXT
        BSTA,UN CELLOV          ; 0 leaves the hull showing
        LODA,R0 V_COL
        SUBI,R0 1
        STRA,R0 V_COL
        BCFR,EQ YBC1
        LODA,R0 V_ROW
        ADDI,R0 1
        STRA,R0 V_ROW
        COMI,R0 BRDDIM
        BCFA,EQ YBR1
        RETC,UN

; YBCLR: fill your board area with open sea.
; Calls two down. Clobbers R0-R3, V_ROW.
YBCLR:  LODI,R0 AWATER
        STRA,R0 DCOLOR
        LODI,R3 BRDDIM
        LODI,R0 YBROW
        STRA,R0 V_ROW
YBCL1:  LODA,R0 V_ROW
        STRZ    R1
        LODI,R2 BCOL
        BSTA,UN DPOS
        LODA,R0 V_ROW
        ADDI,R0 1
        STRA,R0 V_ROW
        LODI,R1 0
        LODI,R3 BRDDIM
        BSTA,UN DFILL
        LODA,R0 V_ROW
        COMI,R0 YBROW+BRDDIM
        BCFR,EQ YBCL1
        RETC,UN

; YSHIPS: draw the five hulls. R0 holds myShips[0] on entry and the
; remaining four are read sequentially -- the reply cursor is RAM, so the
; drawing in between does not disturb it.
; Calls three down (SHIPDR -> DPOS). Clobbers R0-R3 and the temporaries.
YSHIPS: LODI,R0 AWATER
        STRA,R0 DCOLOR
        LODI,R1 0
        STRA,R1 V_SHP
YS1:    STRA,R0 V_PLC           ; the placement byte just read
        BSTA,UN SHIPDR
        LODA,R0 V_SHP
        ADDI,R0 1
        STRA,R0 V_SHP
        COMI,R0 NSHIPS
        BCTR,EQ YS9
        BSTA,UN RXNEXT
        BCTR,UN YS1
YS9:    RETC,UN

; PLCDEC: decode V_PLC (pos + 100*dir) and V_SHP into the walking
; description every other routine here works from: V_SX, V_SY, V_DIR,
; V_LEN. Leaf. Clobbers R0,R1.
PLCDEC: LODA,R0 V_PLC
        LODI,R1 0
        COMI,R0 100             ; dir: 0 horizontal, 1 vertical
        BCTR,LT PLD1
        SUBI,R0 100
        LODI,R1 1
PLD1:   STRA,R1 V_DIR
        LODI,R1 0               ; divide by ten, inlined: it had exactly
PLD2:   COMI,R0 10              ;   one caller and this is the deepest
        BCTR,LT PLD3            ;   chain in the program, so the return-
        SUBI,R0 10              ;   stack frame it saves is worth more
        ADDI,R1 1               ;   than the handful of bytes
        BCTR,UN PLD2
PLD3:   STRA,R0 V_SX            ; R1 = y, R0 = x
        STRA,R1 V_SY
        LODA,R0 V_SHP
        STRZ    R1
        LODA,R0 SHPSIZ,R1
        STRA,R0 V_LEN
        RETC,UN

; SHIPDR: draw one hull in the CALLER'S DCOLOR (so the ship being moved
; during placement can be highlighted). V_PLC = pos + 100*dir, V_SHP =
; the ship index (which is also its size slot). A one-cell ship cannot happen -- the
; smallest is 2 -- so every hull has two end caps.
; Calls one down (PLCDEC, DPOS, DPUTC). Clobbers R0-R3 and the
; temporaries.
SHIPDR: BSTA,UN PLCDEC
        LODI,R0 0
        STRA,R0 V_SEG
SHD2:   LODA,R0 V_SY            ; position this segment
        ADDI,R0 YBROW
        STRZ    R1
        LODA,R0 V_SX
        ADDI,R0 BCOL
        STRZ    R2
        BSTA,UN DPOS
        LODA,R0 V_SEG           ; end caps at both ends, hull between
        BCTR,EQ SHDCAP
        ADDI,R0 1
        COMA,R0 V_LEN
        BCTR,EQ SHDCAP
        LODA,R0 V_DIR
        BCTR,EQ SHDH
        LODI,R0 GSHIPV
        BCTR,UN SHDPUT
SHDH:   LODI,R0 GSHIPH
        BCTR,UN SHDPUT
SHDCAP: LODI,R0 GSHIPE
SHDPUT: BSTA,UN DPUTC
        LODA,R0 V_DIR           ; step along the hull
        BCTR,EQ SHDSX
        LODA,R0 V_SY
        ADDI,R0 1
        STRA,R0 V_SY
        BCTR,UN SHDNXT
SHDSX:  LODA,R0 V_SX
        ADDI,R0 1
        STRA,R0 V_SX
SHDNXT: LODA,R0 V_SEG
        ADDI,R0 1
        STRA,R0 V_SEG
        COMA,R0 V_LEN
        BCFA,EQ SHD2
        RETC,UN

; YSHIPR: draw the hulls from the SHIPS RAM array rather than the reply
; window. This is what the placement screen and the demo use -- during
; placement the layout only exists in RAM, and a slot of NOPLC is a ship
; not yet committed.
; Calls three down (SHIPDR -> DPOS). Clobbers R0-R3 and the temporaries.
YSHIPR: LODI,R0 AWATER
        STRA,R0 DCOLOR
        LODI,R0 0
        STRA,R0 V_SHP
YSR1:   LODA,R0 V_SHP
        STRZ    R1
        LODA,R0 SHIPS,R1
        COMI,R0 NOPLC
        BCTR,EQ YSR2            ; not placed yet: skip it
        STRA,R0 V_PLC
        BSTA,UN SHIPDR
YSR2:   LODA,R0 V_SHP
        ADDI,R0 1
        STRA,R0 V_SHP
        COMI,R0 NSHIPS
        BCFA,EQ YSR1
        RETC,UN

; PIPS: draw one board's five ship pips down PIPCOL -- filled while the
; ship is afloat, blank once it is sunk. R1 = the board's row 0, and the
; five shipsLeft bytes are read sequentially from the cursor, which the
; caller has already seeked to PLSHIPS.
; Calls three down (RXNEXT -> MSLICE). Clobbers R0-R3, V_ROW, V_COL.
PIPS:   STRA,R1 V_ROW
        LODI,R0 NSHIPS
        STRA,R0 V_COL
PIP1:   LODA,R0 V_ROW
        STRZ    R1
        LODI,R2 PIPCOL
        BSTA,UN DPOS
        BSTA,UN RXNEXT          ; 1 afloat, 0 sunk
        BCTR,EQ PIPSNK
        LODI,R0 AWATER
        STRA,R0 DCOLOR
        LODI,R0 GPIP
        BCTR,UN PIPPUT
PIPSNK: LODI,R0 AHIT
        STRA,R0 DCOLOR
        LODI,R0 GSUNK
PIPPUT: BSTA,UN DPUTC
        LODA,R0 V_ROW
        ADDI,R0 1
        STRA,R0 V_ROW
        LODA,R0 V_COL
        SUBI,R0 1
        STRA,R0 V_COL
        BCFA,EQ PIP1
        RETC,UN
