; bsplace.asm -- bank 2: put your five ships on the water.
;
; The five placements are staged in console RAM -- five bytes at SHIPS, each
; pos + 100*dir, the server's own encoding -- and only sent when all five are
; down. That is why this screen cannot use FN_BLIT_FIELD: there is no
; gamefield in the reply window yet, because the server has not been told
; anything. It builds the board out of FN_BLIT_SEA and FN_BLIT_CELL instead
; and paints it once, which is the same cartridge-composed board by another
; route.
;
; Overlaps are left to the server. Bounds are checked here, because a hull
; running off the edge is a thing the player can see going wrong and should
; not have to make a round trip to find out about.

        CPU     6502
        INCLUDE "vcs.inc"

PAD3    EQU     $81
SAVSP   EQU     $82

        INCLUDE "fujinet.inc"
BSBANK  EQU     2               ; BANKPLC
        INCLUDE "bsdefs.inc"

        ORG     $1000

START:  lda     #2
        sta     VBLANK
        lda     #0
        sta     BSERR
        sta     BSSTEP
        sta     BSSHIP
        sta     BSDIR
        sta     BSTICK
        sta     BSCURX
        sta     BSCURY
        ldx     #NSHIPS-1
CLRSH:  lda     #$FF            ; not yet placed
        sta     SHIPS,x
        dex
        bpl     CLRSH

        jsr     DRAW
        lda     #0
        sta     VBLANK
        jsr     DINIT
        jmp     DLOOP

; ---------------------------------------------------------------------------
APPVBL: inc     BSTICK
        jsr     INSCAN
        sta     BSTMP
        bne     AVKEY
        lda     BSTICK          ; blink the ship being placed
        and     #$0F
        bne     AVDONE
        jmp     DRBRD
AVDONE: rts

AVKEY:  and     #IN_DOWN
        beq     AV1
        lda     BSCURY
        cmp     #BRDDIM-1
        bcs     AVRE
        inc     BSCURY
        jmp     AVRE
AV1:    lda     BSTMP
        and     #IN_UP
        beq     AV2
        lda     BSCURY
        beq     AVRE
        dec     BSCURY
        jmp     AVRE
AV2:    lda     BSTMP
        and     #IN_RIGHT
        beq     AV3
        lda     BSCURX
        cmp     #BRDDIM-1
        bcs     AVRE
        inc     BSCURX
        jmp     AVRE
AV3:    lda     BSTMP
        and     #IN_LEFT
        beq     AV4
        lda     BSCURX
        beq     AVRE
        dec     BSCURX
        jmp     AVRE
AV4:    lda     BSTMP
        and     #IN_SEL
        beq     AV5
        lda     BSDIR           ; rotate
        eor     #1
        sta     BSDIR
        jmp     AVRE
AV5:    lda     BSTMP
        and     #IN_FIRE
        beq     AVDONE
        jmp     PLACE
AVRE:   jsr     DRBRD
        jmp     DRSHIP

; ---------------------------------------------------------------------------
; PLACE -- stage the current ship, and send them all once the last is down.
PLACE:  jsr     FITS
        bne     PLBAD           ; off the edge: leave it where it was
        jsr     CELLNO          ; A = y*10 + x
        ldx     BSDIR
        beq     PL1
        clc
        adc     #BRDCELL        ; the server's encoding: pos + 100*dir
PL1:    ldx     BSSHIP
        sta     SHIPS,x
        inx
        cpx     #NSHIPS
        bcs     PLSEND
        stx     BSSHIP
        jsr     DRBRD
        jmp     DRSHIP

PLSEND: lda     #2
        sta     VBLANK
        lda     #RQPLACE
        jsr     APICALL
        bne     PLSBAD
        lda     #BANKGAM        ; placed: back to the game
        jmp     BSGOTO          ; does not return
PLSBAD: jsr     SHOWERR
        lda     #0              ; let them try again: nothing was staged
        sta     BSSHIP          ;   that the server accepted
        sta     VBLANK
        rts

PLBAD:  ldx     #(TOFF)&$FF
        ldy     #(TOFF)>>8
        jsr     SETP
        lda     #RHINT
        jmp     FNRSTR

; FITS -- Z set if the current ship fits on the board from the cursor.
FITS:   ldx     BSSHIP
        lda     SHPSIZ,x
        clc
        adc     #$FF            ; length - 1, the last segment's offset
        ldx     BSDIR
        bne     FIT1
        clc
        adc     BSCURX
        cmp     #BRDDIM
        bcs     FITNO
        lda     #0
        rts
FIT1:   clc
        adc     BSCURY
        cmp     #BRDDIM
        bcs     FITNO
        lda     #0
        rts
FITNO:  lda     #1
        rts

; CELLNO -- A = the cursor's cell, y*10 + x.
CELLNO: lda     BSCURY
        asl     a
        sta     BSLEAD          ; borrowed; BSDEC is not running
        asl     a
        asl     a
        clc
        adc     BSLEAD
        clc
        adc     BSCURX
        pha
        lda     #0
        sta     BSLEAD
        pla
        rts

; ---------------------------------------------------------------------------
; DRAW -- the whole screen.
DRAW:   ldx     #(TTITLE)&$FF
        ldy     #(TTITLE)>>8
        jsr     SETP
        lda     #RPROMPT
        jsr     FNRSTR
        jsr     DRRUL
        jsr     DRBRD
        jsr     DRSHIP
        ldx     #(THINT)&$FF
        ldy     #(THINT)>>8
        jsr     SETP
        lda     #RHINT
        jmp     FNRSTR

DRRUL:  lda     #RRULER
        jsr     FNROWA
        lda     #' '
        jsr     FNPCH
        ldx     #0
DRR1:   txa
        clc
        adc     #'0'
        jsr     FNPCH
        inx
        cpx     #BRDDIM
        bne     DRR1
        jmp     FNENDR

; DRSHIP -- which ship, how long, and which way it points.
DRSHIP: lda     #RCOORD
        jsr     FNROWA
        lda     #'S'
        jsr     FNPCH
        lda     BSSHIP
        clc
        adc     #'1'
        jsr     FNPCH
        lda     #' '
        jsr     FNPCH
        ldx     BSSHIP
        lda     SHPSIZ,x
        clc
        adc     #'0'
        jsr     FNPCH
        lda     #' '
        jsr     FNPCH
        lda     BSDIR
        beq     DRS1
        lda     #'V'
        jmp     DRS2
DRS1:   lda     #'H'
DRS2:   jsr     FNPCH
        jmp     FNENDR

; ---------------------------------------------------------------------------
; DRBRD -- open sea, the ships already down, and the one being placed.
;
; FN_BLIT_CELL pokes a cell without repainting, so seventeen cells cost
; seventeen three-store sequences and ONE ten-row paint at the end rather than
; seventeen of them.
DRBRD:  lda     #0
        sta     FNRSEL+FH_BSL
        sta     FNRSEL+FH_BSH
        sta     FNRSEL+FH_BCNT
        lda     #BLTSEA
        sta     FNRSEL+FH_BGO

        ldx     #0
DRB1:   cpx     BSSHIP
        bcs     DRB2
        stx     BSTMP
        lda     SHIPS,x
        jsr     HULL            ; a ship already down
        ldx     BSTMP
        inx
        jmp     DRB1

        ; The one being placed, blinking so it is never confused with the
        ; ones that are already committed.
DRB2:   lda     BSTICK
        and     #$10
        bne     DRB3
        lda     BSSHIP
        sta     BSTMP
        jsr     FITS
        bne     DRB3            ; off the edge: show nothing rather than a
        jsr     CELLNO          ;   hull wrapping into the next row
        ldx     BSDIR
        beq     DRB2A
        clc
        adc     #BRDCELL
DRB2A:  jsr     HULL

DRB3:   lda     #BROW
        sta     FNRSEL+FH_BDL
        lda     #0
        sta     FNRSEL+FH_BDH
        lda     #BLTPNT
        sta     FNRSEL+FH_BGO
        rts

; HULL -- poke the cells of ship BSTMP placed at A (pos + 100*dir).
; Nothing is drawn for an unplaced ship ($FF is not a legal placement).
HULL:   cmp     #$FF
        beq     HULX
        ldx     #0              ; X = direction
        cmp     #BRDCELL
        bcc     HU1
        sec
        sbc     #BRDCELL
        ldx     #1
HU1:    stx     BSDIRT
        sta     BSPOS
        ldx     BSTMP
        lda     SHPSIZ,x
        sta     BSSEG
HU2:    lda     #FN_HULL
        sta     FNRSEL+FH_BSL
        lda     #0
        sta     FNRSEL+FH_BSH
        lda     BSPOS
        sta     FNRSEL+FH_BCNT
        lda     #BLTCELL
        sta     FNRSEL+FH_BGO
        lda     BSDIRT
        beq     HU3
        lda     BSPOS           ; vertical: down a row
        clc
        adc     #BRDDIM
        jmp     HU4
HU3:    lda     BSPOS           ; horizontal: along
        clc
        adc     #1
HU4:    sta     BSPOS
        dec     BSSEG
        bne     HU2
HULX:   rts

; ---------------------------------------------------------------------------
SETP:   stx     FNPTRL
        sty     FNPTRH
        rts

SHOWERR:
        lda     #RHINT
        jsr     FNROWA
        lda     #'E'
        jsr     FNPCH
        lda     BSSTEP
        jsr     FNHEX
        lda     #' '
        jsr     FNPCH
        lda     BSERR
        jsr     FNHEX
        jmp     FNENDR

; The classic five, longest first, matching the server's myShips[] order.
SHPSIZ: DB      5, 4, 3, 3, 2

TTITLE: DB      "PLACE SHIPS",0
THINT:  DB      "SEL=TURN",0
TOFF:   DB      "OFF BOARD",0

        INCLUDE "bslib.inc"
        INCLUDE "net.inc"
        INCLUDE "url.inc"
        INCLUDE "fujidisp.inc"

        END
