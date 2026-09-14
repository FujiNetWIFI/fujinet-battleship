; bsplace.asm -- bank 5: put your five ships on the water.
;
; The five placements are staged in console RAM -- five bytes at SHIPS, each
; pos + 100*dir, the server's own encoding -- and sent when all five are
; down, through BANKNET like every request. The board shows them as the
; cartridge composes them: every committed hull as a bracket, the one being
; placed blinking, both through FB_PFCEL one cell at a time.
;
; Overlaps are left to the server. Bounds are checked here, because a hull
; running off the edge is a thing the player can see going wrong and should
; not have to make a round trip to find out about.

        CPU     6502
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "bsdefs.inc"

BSBANK  EQU     BANKPLC
BSHASUI EQU     1
BSHASED EQU     0
BSHASNET EQU    0
BSHASCLS EQU    0
BSHASSTR EQU    1
BSHASDEC EQU    0
SNDFULL EQU     1

        INCLUDE "../build/tail.inc"

; The hull walk's cells, live only inside PHULLS. The game bank's clock
; cells, which it reloads on its next compose -- not its edge memory, which
; has to survive placement or the first poll after it hears a phantom shot.
BSPOS   EQU     BSCLK
BSSEG   EQU     BSTICK
BSPDIR  EQU     BSPOLL

        ORG     $1000

PENTRY: ldx     #NSHIPS-1
        lda     #$FF            ; not yet placed
PE1:    sta     SHIPS,x
        dex
        bpl     PE1
        lda     #0
        sta     BSSHIP
        sta     BSDIR
        sta     BSBLINK
        sta     BSCURX
        sta     BSCURY
        sta     BSLIVE
        lda     #1              ; the layout follows the seat count
        ldx     BSPCNT
        cpx     #3
        bcs     PE2
        lda     #0
PE2:    sta     BSMODE          ; no DINIT: the network bank's kernel is this
        jsr     PCLEAR          ;   one, and the TIA is as it left it
        jsr     PTEXT
        lda     BSENT
        cmp     #ENPLFAIL
        bne     PE3
        ldx     #(TAGAIN)&$FF   ; the server refused the last set
        ldy     #(TAGAIN)>>8
        jsr     PSTAT
PE3:    jsr     PHULLS          ; all of it inside the overscan the network
PRUN:   jsr     DFRAME          ;   bank's last frame armed
        jmp     PRUN

; ---------------------------------------------------------------------------
APPVBL: jsr     SNDTICK
        jsr     INREPT
        sta     BSINP
        and     #IN_RST
        beq     PA1
        lda     #ENMENU
        sta     BSENT
        lda     #BANKMNU
        jmp     BSGOTO
PA1:    lda     BSINP
        and     #INDIRS
        beq     PA2
        jsr     PMOVE
        jmp     PHULLS
PA2:    lda     BSINP
        and     #IN_SEL
        beq     PA3
        lda     BSDIR           ; rotate
        eor     #1
        sta     BSDIR
        lda     #SNDMOVE
        jsr     SNDSOFT
        jmp     PHULLS
PA3:    lda     BSINP
        and     #IN_FIRE
        beq     PA4
        jmp     PPLACE
PA4:    inc     BSBLINK         ; the pending ship blinks
        lda     BSBLINK
        and     #BLINKFR-1
        bne     PAX
        jmp     PHULLS
PAX:    rts

PMOVE:  lda     BSINP
        lsr     a
        bcc     PM1             ; up
        lda     BSCURY
        beq     PM4
        dec     BSCURY
        jmp     PM4
PM1:    lsr     a
        bcc     PM2             ; down
        lda     BSCURY
        cmp     #BRDDIM-1
        bcs     PM4
        inc     BSCURY
        jmp     PM4
PM2:    lsr     a
        bcc     PM3             ; left
        lda     BSCURX
        beq     PM4
        dec     BSCURX
        jmp     PM4
PM3:    lda     BSCURX          ; right
        cmp     #BRDDIM-1
        bcs     PM4
        inc     BSCURX
PM4:    lda     #SNDMOVE
        jmp     SNDSOFT

; ---------------------------------------------------------------------------
; PPLACE -- stage the current ship, and send them all once the last is down.
PPLACE: jsr     FITS
        bne     PBAD            ; off the edge: leave it where it was
        jsr     CELLNO          ; A = y*10 + x
        ldx     BSDIR
        beq     PP1
        clc
        adc     #BRDCELL        ; the server's encoding: pos + 100*dir
PP1:    ldx     BSSHIP
        sta     SHIPS,x
        inx
        cpx     #NSHIPS
        bcs     PSEND
        stx     BSSHIP
        lda     #SNDPLC
        jsr     SNDFIRE
        jsr     PTEXT
        jmp     PHULLS

PSEND:  lda     #SNDSEL
        jsr     SNDFIRE
        lda     #RQPLACE
        sta     BSREQ2
        lda     #ENFETCH
        sta     BSENT
        lda     #BANKNET
        jmp     BSGOTO

PBAD:   lda     #SNDERR
        jsr     SNDFIRE
        ldx     #(TOFF)&$FF
        ldy     #(TOFF)>>8
        jmp     PSTAT

; FITS -- Z set if the current ship fits on the board from the cursor.
FITS:   ldx     BSSHIP
        lda     SHPSIZ,x
        clc
        adc     #$FF            ; length - 1, the last segment's offset
        ldx     BSDIR
        bne     FIT1
        clc
        adc     BSCURX
        jmp     FIT2
FIT1:   clc
        adc     BSCURY
FIT2:   cmp     #BRDDIM
        bcs     FITNO
        lda     #0
        rts
FITNO:  lda     #1
        rts

; ---------------------------------------------------------------------------
; PHULLS -- your board's AUX plane: the ships already down, and the one
; being placed on the blink's lit phase.
PHULLS: jsr     PSLOT0
        tax
        stx     BSLIVE          ; your slot, for the cells below
        lda     #PFM_AUX
        ldy     #0
        jsr     FNBARG
        lda     #FB_PFCLR
        jsr     FNBLIT
        ldx     #0
PH1:    cpx     BSSHIP
        bcs     PH2
        stx     BSIDX
        lda     SHIPS,x
        jsr     HULL
        ldx     BSIDX
        inx
        jmp     PH1
PH2:    lda     BSBLINK
        and     #BLINKFR
        bne     PH9
        jsr     FITS
        bne     PH9             ; off the edge: show nothing rather than a
        jsr     CELLNO          ;   hull wrapping into the next row
        ldx     BSDIR
        beq     PH3
        clc
        adc     #BRDCELL
PH3:    ldx     BSSHIP
        stx     BSIDX
        jmp     HULL
PH9:    rts

; HULL -- the cells of ship BSIDX placed at A (pos + 100*dir), into AUX.
HULL:   ldx     #0
        cmp     #BRDCELL
        bcc     HU1
        sec
        sbc     #BRDCELL
        ldx     #1
HU1:    stx     BSPDIR
        sta     BSPOS
        ldx     BSIDX
        lda     SHPSIZ,x
        sta     BSSEG
HU2:    lda     #PFM_AUX
        ldx     BSLIVE
        ldy     BSPOS
        jsr     FNBARG
        lda     #FB_PFCEL
        jsr     FNBLIT
        lda     BSPOS
        clc
        ldx     BSPDIR
        beq     HU3
        adc     #BRDDIM         ; down a row
        jmp     HU4
HU3:    adc     #1              ; along
HU4:    sta     BSPOS
        dec     BSSEG
        bne     HU2
        rts

; PSLOT0 -- A = your slot: top-right with two players, bottom-left with more.
PSLOT0: lda     #SLTR
        ldx     BSMODE
        beq     PSL1
        lda     #SLBL
PSL1:   rts

; ---------------------------------------------------------------------------
; PTEXT -- the status row and the name over your board.
PTEXT:  lda     #RSTAT
        jsr     FNROWA
        ldx     #(TPLACE)&$FF
        ldy     #(TPLACE)>>8
        jsr     FNSETP
        jsr     FNSTRA
        lda     BSSHIP
        clc
        adc     #'1'
        jsr     FNCHR
        jsr     FNENDW
        lda     #RLABA
        ldx     BSMODE
        beq     PT1
        lda     #RLABB
PT1:    jsr     FNROWA
        lda     BSMODE
        bne     PT2
        lda     #7              ; two players: you are on the right
        jsr     FNSPC
PT2:    ldx     #(TYOU)&$FF
        ldy     #(TYOU)>>8
        jsr     FNSETP
        jsr     FNSTRA
        jmp     FNENDW

; PSTAT -- the status row from the string at X/Y.
PSTAT:  jsr     FNSETP
        lda     #RSTAT
        jsr     FNROWA
        jsr     FNSTRA
        jmp     FNENDW

; PCLEAR -- the rows this screen owns blank, every slot cleared.
PCLEAR: ldx     #0
PCL1:   stx     BSIDX
        lda     PCLROWS,x
        jsr     FNROWA
        jsr     FNENDW
        ldx     BSIDX
        inx
        cpx     #11
        bne     PCL1
        ldx     #PFSLOTS-1
PCL2:   stx     BSIDX
        lda     #PFM_ALL
        ldy     #0
        jsr     FNBARG
        lda     #FB_PFCLR
        jsr     FNBLIT
        ldx     BSIDX
        dex
        bpl     PCL2
        lda     BSMODE
        bne     PCLX
        lda     #RLOW0          ; the two-player layout's hint rows
        jsr     FNROWA
        ldx     #(THINT1)&$FF
        ldy     #(THINT1)>>8
        jsr     FNSETP
        jsr     FNSTRA
        jsr     FNENDW
        lda     #RLOW0+1
        jsr     FNROWA
        ldx     #(THINT2)&$FF
        ldy     #(THINT2)>>8
        jsr     FNSETP
        jsr     FNSTRA
        jsr     FNENDW
PCLX:   rts

PCLROWS: DB     0, 1, 2, 3, 4, 15, 16, 17, 18, 19, 20

; The classic five, longest first, matching the server's myShips[] order.
SHPSIZ: DB      5, 4, 3, 3, 2

TPLACE: DB      "PLACE SHIP ",0
TYOU:   DB      "YOU",0
TOFF:   DB      "WON'T FIT",0
TAGAIN: DB      "REFUSED-AGAIN",0
THINT1: DB      "SEL TURNS",0
THINT2: DB      "FIRE PLACES",0

        INCLUDE "bslib.inc"
        INCLUDE "state.inc"
        INCLUDE "sound.inc"
        INCLUDE "dispgame.inc"

        END
