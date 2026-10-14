; demo.inc -- DEMO=1: a static mock board with no network at all, for
; art, palette and sprite-calibration work by screenshot.
;
; It deliberately exercises every drawing path the game has: both frames,
; all four attributes, open sea, hits, misses, hulls in both
; orientations, end caps, sunk pips, the reticle sprite -- and, copying
; the 5 Card Stud port's trick, a row of SPACES printed at attribute
; $C0, which is the block-graphics trap. If DPUTC's guard ever regresses,
; the footer row turns to garbage and the screenshot shows it.

DEMSHP: DB      3, 100+31, 45, 100+7, 88   ; the five mock placements

DEMO0:  BSTA,UN GSTATIC
        LODI,R3 NSHIPS-1        ; mock placements into the RAM array
DEM1:   LODA,R0 DEMSHP,R3
        STRA,R0 SHIPS,R3
        BDRR,R3 DEM1
        LODA,R0 DEMSHP
        STRA,R0 SHIPS
        BSTA,UN YBCLR
        BSTA,UN YSHIPR
        BSTA,UN DEMCEL          ; scatter damage over both boards
        DAT     STATROW,0       ; status row: white on black
        DCOL    ATEXT
        SETSTR  DMSG1
        BSTA,UN DPRINT
        DAT     FOOTROW,0       ; the $C0 guard regression row
        DCOL    AALERT
        SETSTR  DMSG2
        BSTA,UN DPRINT
        BSTA,UN DEMPIP
        BSTA,UN CURSHOW
DEMLP:  BSTA,UN KEYSCN          ; the disc walks the reticle so sprite
        COMI,R0 KNONE           ; calibration can be eyeballed live
        BCTR,EQ DEMLP
        BSTA,UN CURMOVE
        BCTA,UN DEMLP

DMSG1:  DB      "HIT B7",0
DMSG2:  DB      "   ",0         ; spaces at $C0: the trap DPUTC defuses

; DEMCEL: poke a scatter of hits and misses onto both boards straight
; through the cell painter, so CELLPT and CELLOV both get exercised.
; Calls three down. Clobbers R0-R3.
DEMCEL: LODI,R3 (DEMCE-DEMCB)-1
DEMC1:  STRA,R3 V_COL
        LODA,R0 DEMCB,R3
        STRZ    R1              ; packed: row in the high nibble...
        ANDI,R0 $0F
        STRZ    R2
        LODZ    R1
        RRR,R0
        RRR,R0
        RRR,R0
        RRR,R0
        ANDI,R0 $0F             ; ...column in the low
        STRZ    R1
        LODA,R0 V_COL
        ANDI,R0 $01             ; alternate hit / miss
        ADDI,R0 1
        STRA,R0 V_BTMP
        LODA,R0 V_COL
        COMI,R0 (DEMCE-DEMCB)/2
        BCTR,LT DEMCY
        LODZ    R1
        ADDI,R0 EBROW
        BCTR,UN DEMCP
DEMCY:  LODZ    R1
        ADDI,R0 YBROW
DEMCP:  STRZ    R1
        LODZ    R2
        ADDI,R0 BCOL
        STRZ    R2
        BSTA,UN DPOS
        LODA,R0 V_BTMP
        BSTA,UN CELLPT
        LODA,R0 V_COL
        STRZ    R3
        BDRR,R3 DEMC1
        RETC,UN

; row<<4 | col; the first half lands on your board, the second on the
; enemy's, so both renderers show damage.
DEMCB:  DB      $03,$14,$25,$36,$47,$58
DEMCE   EQU     $

; DEMPIP: five pips per board, one sunk each, without the network --
; PIPS reads from the reply window, so the demo paints them directly.
; Calls two down. Clobbers R0-R3.
DEMPIP: LODI,R0 EBROW
        STRA,R0 V_ROW
        BSTA,UN DEMPI1
        LODI,R0 YBROW
        STRA,R0 V_ROW
DEMPI1: LODI,R3 NSHIPS
DEMPI2: STRA,R3 V_SEG
        LODA,R0 V_ROW
        STRZ    R1
        LODI,R2 PIPCOL
        BSTA,UN DPOS
        LODA,R0 V_SEG
        COMI,R0 2               ; one of the five is down
        BCTR,EQ DEMPS
        LODI,R0 AWATER
        STRA,R0 DCOLOR
        LODI,R0 GPIP
        BCTR,UN DEMPP
DEMPS:  LODI,R0 AHIT
        STRA,R0 DCOLOR
        LODI,R0 GSUNK
DEMPP:  BSTA,UN DPUTC
        LODA,R0 V_ROW
        ADDI,R0 1
        STRA,R0 V_ROW
        LODA,R0 V_SEG
        STRZ    R3
        BDRR,R3 DEMPI2
        RETC,UN
