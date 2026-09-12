; bslobby.asm -- bank 0: pick a name, pick a table, join it.
;
; /tables returns a count byte and then 36-byte records: id[9], name[21],
; players[6] as the literal text "0 / 4". 181 bytes for five tables, so the
; whole listing sits in slice 0 of the reply window and every row is rendered
; straight out of it -- the console holds a cursor and nothing else.
;
; The one thing that does get copied into RAM is the chosen table's id, ten
; bytes at BSTBL, because every /state poll after this repaints the window
; that named it.

        CPU     6502
        INCLUDE "vcs.inc"

PAD3    EQU     $81             ; the display kernel's 3-cycle pad target
SAVSP   EQU     $82

        INCLUDE "fujinet.inc"
BSBANK  EQU     0               ; BANKLOB; set before bsdefs so the shared
        INCLUDE "bsdefs.inc"    ;   modules can leave out what this bank
                                ;   never calls

NTROW   EQU     3               ; the first table row
NTMAX   EQU     8               ; rows on screen; the server offers five

        ORG     $1000

START:  lda     #2
        sta     VBLANK
        lda     #0
        sta     BSERR
        sta     BSSTEP
        sta     BSSEL
        sta     BSCNT

        jsr     FNCHK
        beq     HAVE
        lda     #FNENOC
        sta     BSERR
        jmp     SHOWN

HAVE:   jsr     DRAW
SHOWN:  lda     #0
        sta     VBLANK
        jsr     DINIT
        jmp     DLOOP

; ---------------------------------------------------------------------------
APPVBL: jsr     INSCAN
        sta     BSTMP

        and     #IN_DOWN
        beq     AV1
        lda     BSSEL
        clc
        adc     #1
        cmp     BSCNT
        bcs     AVDONE
        sta     BSSEL
        jmp     AVLIST

AV1:    lda     BSTMP
        and     #IN_UP
        beq     AV2
        lda     BSSEL
        beq     AVDONE
        sec
        sbc     #1
        sta     BSSEL
        jmp     AVLIST

AV2:    lda     BSTMP           ; left and right cycle the player name
        and     #IN_RIGHT
        beq     AV3
        ldx     BSNAME
        inx
        cpx     #NNAMES
        bcc     AV2A
        ldx     #0
AV2A:   stx     BSNAME
        jsr     DRNAME
        rts

AV3:    lda     BSTMP
        and     #IN_LEFT
        beq     AV4
        ldx     BSNAME
        bne     AV3A
        ldx     #NNAMES
AV3A:   dex
        stx     BSNAME
        jsr     DRNAME
        rts

AV4:    lda     BSTMP
        and     #IN_SEL
        beq     AV5
        jmp     AVREFR          ; SELECT re-lists

AV5:    lda     BSTMP
        and     #IN_FIRE
        beq     AVDONE
        jmp     JOIN
AVDONE: rts

; Moving the cursor redraws the list from the reply window still in place --
; /tables has not been re-issued, so this costs no round trip.
AVLIST: jsr     DRLIST
        rts

AVREFR: lda     #2
        sta     VBLANK
        jsr     DRAW
        lda     #0
        sta     VBLANK
        rts

; ---------------------------------------------------------------------------
; DRAW -- title, the listing, the name, the hint.
DRAW:   ldx     #(TTITLE)&$FF
        ldy     #(TTITLE)>>8
        jsr     SETP
        lda     #0
        jsr     FNRSTR

        lda     #RQTABLE
        jsr     APICALL
        beq     DR1
        jmp     DRBAD
DR1:    lda     FNRPLY          ; the count byte
        cmp     #NTMAX+1
        bcc     DR2
        lda     #NTMAX
DR2:    sta     BSCNT
        jsr     DRLIST
        jsr     DRNAME
        ldx     #(THINT)&$FF
        ldy     #(THINT)>>8
        jsr     SETP
        lda     #RHINT
        jmp     FNRSTR

DRBAD:  jsr     SHOWERR
        rts

; DRLIST -- the table rows, from the reply window.
;
; Record n starts at 1 + n*36 and the id is its first nine bytes. Five records
; is 181 bytes, so a single-byte index reaches every one of them and the
; multiply is three shifts and two adds rather than anything general.
DRLIST: ldx     #0
DRL1:   stx     BSTMP
        txa
        clc
        adc     #NTROW
        jsr     FNROWA

        ldx     BSTMP
        cpx     BSSEL
        bne     DRLNC
        lda     #'>'
        jmp     DRLC
DRLNC:  lda     #' '
DRLC:   jsr     FNPCH

        jsr     RECOFF          ; X = 1 + BSTMP*36
        ldy     #FNTCOL-1
        jsr     FNPRPL          ; the id
        jsr     FNENDR

        ldx     BSTMP
        inx
        cpx     BSCNT
        bcc     DRL1
DRL2:   cpx     #NTMAX          ; blank whatever the listing did not fill
        bcs     DRL9
        stx     BSTMP
        txa
        clc
        adc     #NTROW
        jsr     FNROWA
        jsr     FNENDR
        ldx     BSTMP
        inx
        jmp     DRL2
DRL9:   rts

; RECOFF -- X = the reply offset of record BSTMP: 1 + n * 36.
RECOFF: lda     BSTMP
        asl     a               ; n*2
        sta     BSLEAD          ; borrowed; FNDEC is not running
        asl     a               ; n*4
        asl     a               ; n*8
        asl     a               ; n*16
        clc
        adc     BSLEAD          ; n*18
        asl     a               ; n*36
        clc
        adc     #1              ; past the count byte
        tax
        lda     #0
        sta     BSLEAD
        rts

; DRNAME -- the player name row.
DRNAME: lda     #RCOORD
        jsr     FNROWA
        ldx     #(TAS)&$FF
        ldy     #(TAS)>>8
        jsr     SETP
        jsr     FNPSTR
        lda     BSNAME
        asl     a
        asl     a
        asl     a
        tax
        ldy     #0
DRN1:   lda     NAMES,x
        beq     DRN2
        jsr     FNPCH
        inx
        iny
        cpy     #8
        bne     DRN1
DRN2:   jmp     FNENDR

; ---------------------------------------------------------------------------
; JOIN -- copy the selected table's id into RAM and enter the game.
;
; This is the only copy the program makes. Nine bytes, because every /state
; poll from here on repaints the window the id is sitting in.
JOIN:   lda     BSCNT
        bne     JN1
        rts                     ; nothing listed: nothing to join
JN1:    lda     BSSEL
        sta     BSTABLE
        sta     BSTMP
        jsr     RECOFF
        ldy     #0
JN2:    lda     FNRPLY,x
        sta     BSTBL,y
        beq     JN3
        inx
        iny
        cpy     #9
        bne     JN2
JN3:    lda     #0
        sta     BSTBL,y         ; terminate whatever the length was
        cpy     #0
        beq     JNBAD           ; an empty id is not a table

        lda     #0
        sta     BSCURX
        sta     BSCURY
        sta     BSVIEW
        sta     BSSHIP
        sta     BSDIR
        sta     BSPOLL
        lda     #BANKGAM
        jmp     BSGOTO          ; does not return
JNBAD:  rts

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

TTITLE: DB      "BATTLESHIP",0
THINT:  DB      "FIRE=JOIN",0
TAS:    DB      "AS ",0

        INCLUDE "bslib.inc"
        INCLUDE "net.inc"
        INCLUDE "url.inc"
        INCLUDE "fujidisp.inc"

        END
