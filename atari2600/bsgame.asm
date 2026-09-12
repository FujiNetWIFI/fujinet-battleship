; bsgame.asm -- bank 1: the game. Poll /state, show a board, take a turn.
;
; The board is composed BY THE CARTRIDGE. Six stores set up an FN_BLIT_FIELD
; and the cartridge turns 100 gamefield bytes in its own reply window into ten
; rows of ready-to-stream glyph bytes; the console never touches a cell. That
; is not a shortcut, it is the only way this fits: doing it here would be a
; hundred reads through a 16-bit reply cursor and about 250 bytes of 6502 in a
; bank with rather less than that to spare.
;
; ONE BOARD AT A TIME. Two 10x10 grids and a status line do not fit in 21 rows
; of 12 columns, and at this size one board is more readable anyway. SELECT
; cycles which player's board is shown; yours is index 0 and gets its ship
; hulls overlaid.
;
; The cursor BLINKS rather than replacing a cell permanently: FN_BLIT_FIELD
; draws it as '+' over whatever is under it, so alternating cnt between the
; cell and FN_BLIT_NOCUR every sixteen frames shows both. On a board where the
; thing under the cursor is the whole point, a solid cursor hides the one cell
; you are looking at.

        CPU     6502
        INCLUDE "vcs.inc"

PAD3    EQU     $81
SAVSP   EQU     $82

        INCLUDE "fujinet.inc"
BSBANK  EQU     1               ; BANKGAM
        INCLUDE "bsdefs.inc"

POLLIVL EQU     90              ; frames between /state polls, about 1.5s

        ORG     $1000

START:  lda     #2
        sta     VBLANK
        lda     #0
        sta     BSERR
        sta     BSSTEP
        sta     BSTICK
        jsr     POLL
        lda     #0
        sta     VBLANK
        jsr     DINIT
        jmp     DLOOP

; ---------------------------------------------------------------------------
APPVBL: inc     BSTICK
        jsr     INSCAN
        sta     BSTMP
        bne     AVKEY

        ; No input. Blink the cursor, and poll when the timer runs out.
        lda     BSTICK
        and     #$0F
        bne     AVP
        jsr     DRBRD
AVP:    dec     BSPOLL
        bne     AVDONE
        jmp     AVPOLL
AVDONE: rts

AVKEY:  and     #IN_DOWN
        beq     AV1
        lda     BSCURY
        cmp     #BRDDIM-1
        bcs     AVCUR
        inc     BSCURY
        jmp     AVCUR
AV1:    lda     BSTMP
        and     #IN_UP
        beq     AV2
        lda     BSCURY
        beq     AVCUR
        dec     BSCURY
        jmp     AVCUR
AV2:    lda     BSTMP
        and     #IN_RIGHT
        beq     AV3
        lda     BSCURX
        cmp     #BRDDIM-1
        bcs     AVCUR
        inc     BSCURX
        jmp     AVCUR
AV3:    lda     BSTMP
        and     #IN_LEFT
        beq     AV4
        lda     BSCURX
        beq     AVCUR
        dec     BSCURX
        jmp     AVCUR

AV4:    lda     BSTMP
        and     #IN_SEL
        beq     AV5
        ldx     BSVIEW          ; cycle whose board is shown
        inx
        cpx     BSPCNT
        bcc     AV4A
        ldx     #0
AV4A:   stx     BSVIEW
        jsr     DRBRD
        jmp     DRWHO

AV5:    lda     BSTMP
        and     #IN_FIRE
        beq     AVDONE
        jmp     ACTION

AVCUR:  jsr     DRBRD
        jmp     DRCRD

; AVPOLL -- one /state round trip. The screen is blanked across it: a
; transaction is several socket round trips and the frame will not finish.
AVPOLL: lda     #2
        sta     VBLANK
        jsr     POLL
        lda     #0
        sta     VBLANK
        rts

; ---------------------------------------------------------------------------
; ACTION -- what FIRE means depends on the phase.
;   lobby   toggle ready
;   in play attack the cell under the cursor, if it is your turn
; The placement phase is not here: POLL hands over to that bank by itself.
ACTION: lda     BSSTAT
        bne     ACT2
        lda     #RQREADY
        jmp     ACTGO
ACT2:   lda     BSACT           ; not your turn: the server would reject it,
        bne     ACTX            ;   and the prompt row already says whose go
        lda     #RQATTCK
ACTGO:  pha
        lda     #2
        sta     VBLANK
        pla
        jsr     APICALL
        bne     ACTBAD
        jsr     REDRAW
        lda     #0
        sta     VBLANK
        rts
ACTBAD: jsr     SHOWERR
ACTX:   lda     #0
        sta     VBLANK
        rts

; ---------------------------------------------------------------------------
; POLL -- /state, then the whole screen.
POLL:   lda     #POLLIVL
        sta     BSPOLL
        lda     #RQSTATE
        jsr     APICALL
        bne     PLBAD
        ; Placement is not a choice, so it does not wait for a button. The
        ; test is BOTH the game's phase and YOUR player status: the phase
        ; stays STPLACE until everyone has placed, so the phase alone would
        ; drop you back into the placement screen after you had finished.
        lda     BSSTAT
        cmp     #STPLACE
        bne     REDRAW
        lda     BSMYST
        cmp     #PSPLACE
        bne     REDRAW
        lda     #BANKPLC
        jmp     BSGOTO          ; does not return
REDRAW: jsr     DRPRM
        jsr     DRWHO
        jsr     DRRUL
        jsr     DRBRD
        jsr     DRCRD
        jsr     DRPLR
        ldx     #(THINT)&$FF
        ldy     #(THINT)>>8
        jsr     SETP
        lda     #RHINT
        jmp     FNRSTR
PLBAD:  jmp     SHOWERR

; DRPRM -- the server's prompt, truncated to the screen.
DRPRM:  lda     #RPROMPT
        jsr     FNROWA
        ldx     #GOPRMPT
        ldy     #FNTCOL
        jsr     FNPRPL
        jmp     FNENDR

; DRWHO -- whose board is shown, and the move clock.
DRWHO:  lda     #RWHOSE
        jsr     FNROWA
        lda     BSVIEW
        bne     DRW1
        ldx     #(TYOU)&$FF
        ldy     #(TYOU)>>8
        jmp     DRW2
DRW1:   ldx     #(TFOE)&$FF
        ldy     #(TFOE)>>8
DRW2:   jsr     SETP
        jsr     FNPSTR
        lda     BSVIEW
        clc
        adc     #'0'
        jsr     FNPCH
        lda     #' '
        jsr     FNPCH
        lda     #(BDCH)&$FF     ; the move clock goes on the screen, not the
        sta     DSTP            ;   wire, so BSDEC is pointed at the text port
        lda     #(BDCH)>>8
        sta     DSTP+1
        lda     FNRPLY+GOMVTIM
        jsr     BSDEC
        jmp     FNENDR

; DRRUL -- the column ruler, one space in from the row-digit gutter.
DRRUL:  ldx     #(TRULE)&$FF
        ldy     #(TRULE)>>8
        jsr     SETP
        lda     #RRULER
        jmp     FNRSTR

; DRCRD -- the cursor's coordinate.
DRCRD:  lda     #RCOORD
        jsr     FNROWA
        lda     #'@'
        jsr     FNPCH
        lda     BSCURX
        clc
        adc     #'0'
        jsr     FNPCH
        lda     #','
        jsr     FNPCH
        lda     BSCURY
        clc
        adc     #'0'
        jsr     FNPCH
        jmp     FNENDR

; ---------------------------------------------------------------------------
; DRBRD -- the board, composed by the cartridge.
;
; During STPLACE the player records are name+status only -- ten bytes, not 115
; -- so there IS no gamefield to point at and indexing one would read the next
; record's name as cells. Show open sea instead until the placement bank has
; run.
DRBRD:  lda     BSSTAT
        cmp     #STPLACE
        bne     DRB1
DRBSEA: lda     #0
        sta     FNRSEL+FH_BSL
        sta     FNRSEL+FH_BSH
        sta     FNRSEL+FH_BCNT
        lda     #BLTSEA
        sta     FNRSEL+FH_BGO
        jmp     DRBPNT

DRB1:   lda     BSSTAT          ; the lobby has no gamefields either
        beq     DRBSEA
        ; src = GIPLYRS + view * PLSTRID + PLFIELD, and it passes 256 at the
        ; second player, so it is built as a 16-bit value.
        lda     #0
        sta     BSTMP
        lda     #GIPLYRS+PLFIELD
        ldx     BSVIEW
        beq     DRB3
DRB2:   clc
        adc     #PLSTRID
        bcc     DRB2A
        inc     BSTMP
DRB2A:  dex
        bne     DRB2
DRB3:   sta     FNRSEL+FH_BSL
        lda     BSTMP
        sta     FNRSEL+FH_BSH
        lda     #BROW
        sta     FNRSEL+FH_BDL
        lda     #0
        sta     FNRSEL+FH_BDH
        jsr     CURCEL
        sta     FNRSEL+FH_BCNT
        lda     #BLTFLD
        sta     FNRSEL+FH_BGO

        lda     BSVIEW          ; your own board gets its hulls overlaid
        bne     DRB9
        lda     #GIMYSHP
        sta     FNRSEL+FH_BSL
        lda     #0
        sta     FNRSEL+FH_BSH
        lda     #NSHIPS
        sta     FNRSEL+FH_BCNT
        lda     #BLTHULL
        sta     FNRSEL+FH_BGO
DRB9:   rts

DRBPNT: jsr     CURCEL
        cmp     #BLTNCUR
        beq     DRBP1
        tax
        lda     #FN_CUR
        sta     FNRSEL+FH_BSL
        lda     #0
        sta     FNRSEL+FH_BSH
        stx     FNRSEL+FH_BCNT
        lda     #BLTCELL
        sta     FNRSEL+FH_BGO
DRBP1:  lda     #BROW
        sta     FNRSEL+FH_BDL
        lda     #0
        sta     FNRSEL+FH_BDH
        lda     #BLTPNT
        sta     FNRSEL+FH_BGO
        rts

; CURCEL -- A = the cursor's cell, or BLTNCUR on the blink's off phase.
CURCEL: lda     BSTICK
        and     #$10
        bne     CURNO
        lda     BSCURY
        asl     a
        sta     BSLEAD          ; borrowed; FNDEC is not running
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
CURNO:  lda     #BLTNCUR
        rts

; ---------------------------------------------------------------------------
; DRPLR -- the VIEWED player's name, straight out of the reply window.
;
; One row, not a roster. Four names would cost about 180 bytes in a bank that
; has under 200 to spare, and SELECT already walks every player one at a time
; with their board -- which is the thing you actually want to see.
;
; The record stride depends on the phase and getting it wrong reads a name as
; a gamefield: ten bytes in the lobby and during placement, 115 once the
; gamefields exist.
DRPLR:  lda     #RPLYR0
        jsr     FNROWA
        lda     BSSTAT
        cmp     #STSTART
        bcs     DRPL2
        cmp     #STPLACE
        bne     DRPL1
        lda     #GIPLYRS
        jmp     DRPL3
DRPL1:  lda     #GLPLYRS
DRPL3:  ldx     BSVIEW
        beq     DRPL9
DRPL4:  clc
        adc     #LBSTRID        ; PPSTRID is the same ten bytes
        dex
        bne     DRPL4
        jmp     DRPL9
; In play the stride is 115, so player 2 onward is past 256 -- out of a
; single-byte index's reach. Their BOARD is still shown, because the blit
; takes a 16-bit source; only the name is skipped.
DRPL2:  ldx     BSVIEW
        beq     DRPL2A
        cpx     #1
        bne     DRPLX
        lda     #GIPLYRS+PLSTRID
        jmp     DRPL9
DRPL2A: lda     #GIPLYRS
DRPL9:  tax
        ldy     #FNTCOL-1
        jsr     FNPRPL
DRPLX:  jmp     FNENDR

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

TYOU:   DB      "YOU ",0
TFOE:   DB      "FOE ",0
THINT:  DB      "SEL=VIEW",0
TRULE:  DB      " 0123456789",0

        INCLUDE "bslib.inc"
        INCLUDE "net.inc"
        INCLUDE "url.inc"
        INCLUDE "fujidisp.inc"

        END
