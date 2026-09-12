; bslib.inc -- the mailbox transport, trimmed to what a networked game needs.
;
; A SUBSET of fujinet-firmware/pico/atari-2600/testrom/fujilib.inc, not a
; rewrite: every routine here is that file's, character for character. What is
; gone is everything about the filesystem -- FNSWAP, FNPATH, the payload
; builder, the working directory -- because this program mounts nothing, boots
; nothing and reads no directory. It opens one HTTP connection and polls it.
;
; That matters because of the bank budget. Every bank of a banked client
; carries its own copy of the library: the high 2K of the window is the
; mailbox in its entirety, so there is no fixed code region to share one from.
; Six hundred bytes duplicated three ways is most of a bank.
;
; A register write is TWO stores on this console. The cartridge port has no
; R/W line, so the cart cannot tell a read of a write port from a write to it;
; arming and committing separately means one stray mutates nothing, because an
; arm carries no value and a commit with nothing armed is discarded.
;
; And NEVER an RMW instruction on the control or TX pages: INC/DEC/ASL and the
; rest are three bus cycles at one address, and the cartridge's "last value
; before the address changed" sampling has no defined answer in that burst.
; tools/checkrom.py fails the build on them.

; FNRW -- register X = A.
FNRW:   sta     FNRSEL,x
        sta     FNCMT
        rts

; FNARM -- open the decode gate. Two stores, in order, carrying two specific
; values; a 7800 BIOS probing cartridge space cannot produce that pair.
FNARM:  lda     #FNAM1
        sta     FNRSEL+FH_ARM1
        lda     #FNAM2
        sta     FNRSEL+FH_ARM2
        rts

; FNCHK -- Z set if a FujiNet cartridge is answering.
FNCHK:  lda     FNMAG0
        cmp     #'F'
        bne     FNCHK1
        lda     FNMAG1
        cmp     #'N'
FNCHK1: rts

; FNBEG -- begin the transaction described by FNDEV / FNCMD / FNNPR.
; Rewinds the TX stream last, so parameters may be appended immediately.
FNBEG:  lda     #16             ; ~9s; longer than the cart's own 5s budget so
        sta     FNTMO           ;   a real timeout is reported as ITS error
        lda     FNDEV
        ldx     #FR_DEV
        jsr     FNRW
        lda     FNCMD
        ldx     #FR_CMD
        jsr     FNRW
        lda     FNNPR
        ldx     #FR_NPAR
        jsr     FNRW
        lda     #0
        ldx     #FR_DRST
        jmp     FNRW

; FNPB -- append a one-byte parameter with value A.
; The stream is NPARAM x { size byte, then that many value bytes } then the
; payload, identical to every sibling port.
FNPB:   pha
        lda     #1
        sta     FNTX
        pla
        sta     FNTX
        rts

; FNPW -- append a two-byte little-endian parameter. A = low, X = high.
FNPW:   pha
        lda     #2
        sta     FNTX
        pla
        sta     FNTX
        txa
        sta     FNTX
        rts

; FNGO -- commit and wait. Returns A = FNE* (0 = the transaction completed).
;
; THE RULE EVERY PORT IN THIS FAMILY LEARNED THE HARD WAY: the next sequence
; number comes from the CART'S OWN persisted ACKSEQ + 1, never from a counter
; in RAM. A console RESET restarts the client and re-zeroes its variables but
; does NOT reset the cartridge -- there is no reset line on the connector and
; the RESET switch is a bit in a RIOT register the cart never sees. A
; program-local counter would restart at 1, collide with a sequence already
; answered, and the transaction would be dropped in silence.
FNGO:   lda     FNACKS
        clc
        adc     #1
        bne     FNGO1
        lda     #1              ; 0 is reserved as "never used"
FNGO1:  sta     FNSEQ
        ldx     #FR_SEQ
        jsr     FNRW            ; launching it is this single commit
        lda     FNTMO
        sta     FNCNT
FNGO2:  ldy     #0
FNGO3:  ldx     #0
        ; ACKSEQ is published LAST, after the whole reply is in place, so a
        ; match means everything behind it is already readable.
FNGO4:  lda     FNACKS
        cmp     FNSEQ
        beq     FNGO5
        dex
        bne     FNGO4
        dey
        bne     FNGO3
        dec     FNCNT
        bne     FNGO2
        lda     #FNEWAIT
        rts
FNGO5:  lda     FNERR
        rts

; FNACK -- 0 on success, FNENAK if the server said no.
FNACK:  lda     FNRCMD
        cmp     #$06
        beq     FNACK1
        lda     #FNENAK
        rts
FNACK1: lda     #FNEOK
        rts

; ---------------------------------------------------------------------------
; Text, composed by the cartridge.

; FNROWA -- begin text row A.
FNROWA: sta     FNRSEL+FH_TROW
        rts

; FNENDR -- render the composed row.
FNENDR: lda     #0
        sta     FNRSEL+FH_TEND
        rts

; FNRSTR -- render the NUL-terminated string at FNPTRL/H into text row A.
; Select, stream, render: the middle of those is FNPSTR, so this is three
; instructions rather than a second copy of the loop.
FNRSTR: sta     FNRSEL+FH_TROW
        jsr     FNPSTR
        jmp     FNENDR

; FNPSTR -- append the NUL-terminated string at FNPTRL/H to the row being
; composed. (FNRSTR selects the row itself; this one does not, so something
; else can go first.)
FNPSTR: ldy     #0
FNPS1:  lda     (FNPTRL),y
        beq     FNPS2
        sta     FNRSEL+FH_TCHR
        iny
        cpy     #FNTCOL
        bne     FNPS1
FNPS2:  rts

; FNPRPL -- append up to Y bytes of the reply window at offset X to the row
; being composed, stopping at NUL.
;
; The zero-copy path, and on this console not an optimisation. The reply
; window is repainted only by a SEQ commit or a slice select, so it is stable
; while the row is streamed -- and the bytes go from the cartridge back to the
; cartridge without ever occupying any of the console's 128 bytes of RAM.
FNPRPL: lda     FNRPLY,x
        beq     FNPR2
        sta     FNRSEL+FH_TCHR
        inx
        dey
        bne     FNPRPL
FNPR2:  rts

; FNPCH -- append A to the row being composed. Preserves X and Y.
FNPCH:  sta     FNRSEL+FH_TCHR
        rts

; FNHEX -- append A as two hex digits.
FNHEX:  pha
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        jsr     FNHEX1
        pla
        and     #$0F
FNHEX1: cmp     #10
        bcc     FNHEX2
        adc     #6              ; carry is set here, so this adds 7 in total
FNHEX2: adc     #'0'
        sta     FNRSEL+FH_TCHR
        rts


; ---------------------------------------------------------------------------
; BSDEC -- append A as up to three decimal digits, no leading zeros, through
; whichever port DSTP names: BDTX for the TX stream, BDCH for the row being
; composed.
;
; ONE routine, not two. It began as two -- a display one storing to
; FNRSEL+FH_TCHR and a wire one storing to FNTX -- and sixty-five duplicated
; bytes is a third of what a bank has spare once the shared modules are in.
;
; The dispatch is an indirect JMP and NOT `sta (DSTP),y`, which is the obvious
; way to do it and is wrong here. STA (zp),y always performs an internal READ
; at the address it is about to write. On the control and TX pages that read
; is decoded as an access carrying whatever the floating bus held -- an extra
; character appended before every real one. tools/checkrom.py bans the
; addressing mode outright for exactly this reason.
BDTX:   sta     FNTX
        rts
BDCH:   sta     FNRSEL+FH_TCHR
        rts

BSDEC:  ldx     #0              ; hundreds
BD1:    cmp     #100
        bcc     BD2
        sbc     #100
        inx
        bne     BD1
BD2:    pha
        txa
        beq     BD3
        jsr     BDPUT
BD3:    pla
        ldx     #0              ; tens
BD4:    cmp     #10
        bcc     BD5
        sbc     #10
        inx
        bne     BD4
BD5:    pha
        txa
        bne     BD6
        lda     BSLEAD          ; a zero tens digit prints only after a
        beq     BD7             ;   nonzero hundreds
        lda     #0
BD6:    jsr     BDPUT
BD7:    pla
        jsr     BDPUT
        lda     #0
        sta     BSLEAD
        rts
BDPUT:  clc
        adc     #'0'
        jsr     BDGO
        lda     #1
        sta     BSLEAD
        rts
BDGO:   jmp     (DSTP)

; ---------------------------------------------------------------------------
; INSCAN -- fold the joystick and console switches into one active-high byte
; and return only what is NEWLY pressed, so a held stick steps once.
INSCAN: lda     SWCHA
        eor     #$FF            ; the port is active low
        and     #$F0            ; right, left, down, up in bits 7-4
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        sta     INCUR
        lda     INPT4
        bmi     INSC1           ; bit 7 high = fire not pressed
        lda     INCUR
        ora     #IN_FIRE
        sta     INCUR
INSC1:  lda     SWCHB
        and     #$01
        bne     INSC2
        lda     INCUR
        ora     #IN_RST
        sta     INCUR
INSC2:  lda     SWCHB
        and     #$02
        bne     INSC3
        lda     INCUR
        ora     #IN_SEL
        sta     INCUR
INSC3:  lda     INPREV          ; newly pressed = current AND NOT previous
        eor     #$FF
        and     INCUR
        pha
        lda     INCUR
        sta     INPREV
        pla
        rts
