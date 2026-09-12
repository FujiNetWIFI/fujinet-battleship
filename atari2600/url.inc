; url.inc -- building the devicespec, streamed straight into the TX page a
; character at a time.
;
; Nothing is assembled in RAM. The cartridge counts the payload itself, so
; nothing here tracks a length -- which is the only reason a console with 128
; bytes can build a URL like
;
;   N:https://battleship.carr-designs.com/attack/47?table=ai1&player=VCS&bin=1&v=2
;
; at all. The table id and the player name are not in RAM either: the table id
; is read back out of the cartridge's reply window, where the /tables listing
; that named it is still sitting, and the player name is a string in ROM.
;
; The worst case is /place with five three-digit positions, about 110 bytes
; against the TX page's 256.

; UTXB -- append the literal byte A.
UTXB:   sta     FNTX
        rts

; UTXS -- append the NUL-terminated string at FNPTRL/H.
UTXS:   ldy     #0
UTXS1:  lda     (FNPTRL),y
        beq     UTXS2
        sta     FNTX
        iny
        bne     UTXS1
UTXS2:  rts

; USETP -- point FNPTRL/H at the string whose address is in X (lo) / Y (hi).
USETP:  stx     FNPTRL
        sty     FNPTRH
        rts

        IF      BSBANK<>BANKLOB
; UDEC -- append A as decimal to the TX stream. The staged move is a raw cell
; number, not text, so this is what turns it into one on the way out and saves
; a twenty-character move buffer this console does not have.
UDEC:   pha
        lda     #(BDTX)&$FF
        sta     DSTP
        lda     #(BDTX)>>8
        sta     DSTP+1
        pla
        jmp     BSDEC
        ENDIF

; ---------------------------------------------------------------------------
; BLDURL -- stream the devicespec for the request in BSREQ.
BLDURL: lda     #0
        sta     BSLEAD
        lda     #'N'
        sta     FNTX
        lda     #':'
        sta     FNTX
        ldx     #(UENDPT)&$FF
        ldy     #(UENDPT)>>8
        jsr     USETP
        jsr     UTXS

        lda     BSREQ
        IF      BSBANK=BANKLOB
        ; The lobby issues exactly one request, and it carries its own query
        ; and takes neither a table nor a player.
        ldx     #(UTABLS)&$FF
        ldy     #(UTABLS)>>8
        jsr     USETP
        jmp     UTXS
        ELSE
        cmp     #RQSTATE
        bne     BU2
        ldx     #(USTATE)&$FF
        ldy     #(USTATE)>>8
        jsr     USETP
        jsr     UTXS
        jmp     BUQ

        IF      BSBANK=BANKGAM
BU2:    cmp     #RQREADY
        bne     BU3
        ldx     #(UREADY)&$FF   ; a bare /ready TOGGLES, matching the
        ldy     #(UREADY)>>8    ;   Intellivision and astrocade clients
        jsr     USETP
        jsr     UTXS
        jmp     BUQ
        ELSE
BU2:
        ENDIF

        IF      BSBANK=BANKPLC
BU3:    cmp     #RQPLACE
        bne     BU5
        ldx     #(UPLACE)&$FF
        ldy     #(UPLACE)>>8
        jsr     USETP
        jsr     UTXS
        ldx     #0
BU4:    stx     BSTMP
        lda     SHIPS,x
        jsr     UDEC
        ldx     BSTMP
        inx
        cpx     #NSHIPS
        beq     BUQ             ; no comma after the last
        lda     #','
        sta     FNTX
        jmp     BU4
        ELSE
BU3:
        ENDIF

        IF      BSBANK=BANKGAM
BU5:    cmp     #RQATTCK
        bne     BUQ
        ldx     #(UATTCK)&$FF
        ldy     #(UATTCK)>>8
        jsr     USETP
        jsr     UTXS
        lda     BSCURY
        asl     a               ; y * 10 + x
        sta     BSTMP
        asl     a
        asl     a
        clc
        adc     BSTMP
        clc
        adc     BSCURX
        jsr     UDEC
        ELSE
BU5:
        ENDIF

; The query.
BUQ:    ldx     #(UQTAB)&$FF
        ldy     #(UQTAB)>>8
        jsr     USETP
        jsr     UTXS
        jsr     UTABID
        ldx     #(UQPLR)&$FF
        ldy     #(UQPLR)>>8
        jsr     USETP
        jsr     UTXS
        jsr     UPLRNM
        ldx     #(UQBIN2)&$FF
        ldy     #(UQBIN2)>>8
        jsr     USETP
        jmp     UTXS
        ENDIF

        IF      BSBANK<>BANKLOB
; UTABID -- append the chosen table id.
UTABID: ldx     #0
UTI1:   lda     BSTBL,x
        beq     UTI2
        sta     FNTX
        inx
        cpx     #9
        bne     UTI1
UTI2:   rts

; UPLRNM -- append the chosen player name. Eight fixed slots of eight bytes,
; so the index is a shift and there is no name buffer.
;
; A real name would want an on-screen keyboard, and twelve columns can hold
; one -- but a keyboard is a screen of its own and this program is already
; three banks. Picking from a list is honest and it is one instruction.
UPLRNM: lda     BSNAME
        asl     a
        asl     a
        asl     a
        tax
        ldy     #0
UPN1:   lda     NAMES,x
        beq     UPN2
        sta     FNTX
        inx
        iny
        cpy     #8
        bne     UPN1
UPN2:   rts
        ENDIF

        INCLUDE "build/endpoint.inc"
        IF      BSBANK=BANKLOB
UTABLS: DB      "tables?bin=1",0
        ELSE
USTATE: DB      "state",0
        IF      BSBANK=BANKGAM
UREADY: DB      "ready",0
UATTCK: DB      "attack/",0
        ELSE
UPLACE: DB      "place/",0
        ENDIF
UQTAB:  DB      "?table=",0
UQPLR:  DB      "&player=",0
UQBIN2: DB      "&bin=1&v=2",0  ; v=2: the winner and the ship reveal at
                                ;   gameover
        ENDIF
NAMES:  DB      "VCS",0,0,0,0,0
        DB      "ATARI",0,0,0
        DB      "STELLA",0,0
        DB      "PADDLE",0,0
        DB      "JOYSTICK"
        DB      "COMBAT",0,0
        DB      "PITFALL",0
        DB      "FUJI",0,0,0,0
NNAMES  EQU     8
