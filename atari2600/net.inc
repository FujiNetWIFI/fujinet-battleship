; net.inc -- the N: device round trip.
;
; A 6502 transcription of arcadia/net.inc's sequence (which is the
; astrocade's, which is the Odyssey 2's). The rules it carries forward, each
; learned somewhere else in this family:
;
;  * The CLOSE belongs at the START of the next request, never after the READ.
;    Every transaction repaints the whole reply window, a CLOSE reply is
;    empty, and every screen here renders straight out of the window between
;    polls -- so closing after the read would wipe the thing being displayed.
;  * FNRXLO/FNRXHI belong to the MOST RECENT transaction, so the reply length
;    is captured immediately after the READ and not a transaction later.
;  * The ESP32 reports STATUS as soon as SOME of the response has arrived.
;    Poll until two consecutive readings agree, bounded, rather than guessing
;    a delay.
;  * The 4th STATUS byte is nDevStatus_t and checking it is not optional: the
;    HTTP GET is deferred until the first STATUS, and an HTTP error page still
;    has a perfectly readable body.
;
; Every routine returns A = 0 on success and sets Z accordingly, so callers
; branch on the return directly.

; NCLOSE -- close the previous request's connection. The first call has
; nothing to close and earns a NAK; both results are ignored on purpose.
NCLOSE: lda     #NETDEV
        sta     FNDEV
        lda     #NCCLOSE
        sta     FNCMD
        lda     #0
        sta     FNNPR
        jsr     FNBEG
        jsr     FNGO
        lda     #0
        rts

; NOPEN -- OPEN the devicespec as an HTTP GET with no translation.
; The devicespec is streamed by BLDURL straight into the TX page; nothing is
; assembled in RAM, because there is no RAM to assemble it in.
NOPEN:  lda     #NETDEV
        sta     FNDEV
        lda     #NCOPEN
        sta     FNCMD
        lda     #2
        sta     FNNPR
        jsr     FNBEG
        lda     #NMHTTPG
        jsr     FNPB
        lda     #NTRNONE
        jsr     FNPB
        jsr     BLDURL
        jsr     FNGO
        cmp     #FNEOK
        bne     NOBAD
        jsr     FNACK
        cmp     #FNEOK
        bne     NOBAD
        lda     #FNEOK
        rts
NOBAD:  sta     BSERR
        lda     #1
        sta     BSSTEP
        lda     #$FF
        rts

; NSTATUS -- one STATUS poll. On success BSAVL0/1 is the byte count waiting.
NSTATUS: lda    #NETDEV
        sta     FNDEV
        lda     #NCSTAT
        sta     FNCMD
        lda     #2
        sta     FNNPR
        jsr     FNBEG
        lda     #0
        jsr     FNPB
        lda     #0
        jsr     FNPB
        jsr     FNGO
        cmp     #FNEOK
        bne     NSBAD
        jsr     FNACK
        cmp     #FNEOK
        bne     NSBAD
        lda     FNRPLY+NSDEVST
        cmp     #1              ; SUCCESS -- anything else is an HTTP error
        bne     NSBAD2
        lda     FNRPLY+NSAVLO
        sta     BSAVL0
        lda     FNRPLY+NSAVHI
        sta     BSAVL1
        lda     #FNEOK
        rts
NSBAD2: lda     FNRPLY+NSDEVST  ; report what the server actually said
NSBAD:  sta     BSERR
        lda     #2
        sta     BSSTEP
        lda     #$FF
        rts

; NREAD -- READ min(available, GMAXLEN) bytes, then capture the reply length.
;
; The clamp is not caution: GMAXLEN is 509 and the reply window is 512, so a
; reply that fits is a reply that can be read in place with no slice paging at
; all. That is the whole reason the window is 512 and not 256.
NREAD:  lda     #NETDEV
        sta     FNDEV
        lda     #NCREAD
        sta     FNCMD
        lda     #1
        sta     FNNPR
        jsr     FNBEG

        lda     BSAVL1          ; available vs GMAXLEN, 16-bit
        cmp     #GMAXLEN>>8
        bcc     NRTAKE
        bne     NRCLMP
        lda     BSAVL0
        cmp     #(GMAXLEN&$FF)+1
        bcs     NRCLMP
NRTAKE: lda     BSAVL0
        ldx     BSAVL1
        jmp     NRGO
NRCLMP: lda     #GMAXLEN&$FF
        ldx     #GMAXLEN>>8
NRGO:   jsr     FNPW
        jsr     FNGO
        cmp     #FNEOK
        bne     NRBAD
        jsr     FNACK
        cmp     #FNEOK
        bne     NRBAD
        lda     FNRXLO          ; capture NOW: the next transaction's reply
        sta     BSRXL0          ;   length would replace it
        lda     FNRXHI
        sta     BSRXL1
        lda     #FNEOK
        rts
NRBAD:  sta     BSERR
        lda     #3
        sta     BSSTEP
        lda     #$FF
        rts

; ---------------------------------------------------------------------------
; APICALL -- A = the request (RQ*). One full round trip; on success (A = 0)
; the reply is in the window and stays valid until the next APICALL.
APICALL: sta    BSREQ
        jsr     NCLOSE          ; the PREVIOUS request's connection
        jsr     NOPEN
        beq     APOK
        rts                     ; open failed, and it already closed
APOK:   lda     #0
        sta     BSPRV0
        sta     BSPRV1
        lda     #20             ; the settle loop's bound
        sta     BSTRIES
APSET:  jsr     NSTATUS
        beq     APS1
        jmp     APFAIL
APS1:   lda     BSAVL0
        ora     BSAVL1
        beq     APAGN           ; nothing has arrived at all yet
        lda     BSAVL0
        cmp     BSPRV0
        bne     APAGN
        lda     BSAVL1
        cmp     BSPRV1
        beq     APRD            ; two readings agree: it has landed
APAGN:  lda     BSAVL0
        sta     BSPRV0
        lda     BSAVL1
        sta     BSPRV1
        lda     #3              ; ~3 frames between polls
        sta     BSDLY           ; NOT in X or Y: APWAIT is a register-counted
APDLY:  jsr     APWAIT           ;   delay and comes back with both clobbered
        dec     BSDLY
        bne     APDLY
        dec     BSTRIES
        bne     APSET
APFAIL: jsr     NCLOSE          ; failed before the READ: nothing to keep
        lda     #$FF
        rts
APRD:   jsr     NREAD
        beq     APRD1
        jmp     APFAIL
; A validation failure AFTER the read returns nonzero WITHOUT closing: the
; next APICALL's leading NCLOSE retires the connection, and closing here would
; wipe a window the caller may still be rendering the last good reply from.
APRD1:  jmp     VALID8

; APWAIT -- one frame, roughly. There is no vertical-blank interrupt on this
; console and the display loop is not running during a transaction, so this is
; a plain delay; the point is only to space the STATUS polls out.
;
; IT CLOBBERS X AND Y, because it counts in them. Its first caller counted its
; own repeats in Y as well, so every wait ran 256 times over -- seventy
; seconds where seventeen milliseconds were meant, which looks exactly like a
; server that never answers.
APWAIT: ldx     #16             ; ~17ms at 1.19MHz
APW1:   ldy     #0
APW2:   dey
        bne     APW2
        dex
        bne     APW1
        rts

; ---------------------------------------------------------------------------
; VALID8 -- sanity-check the reply length against what the header implies,
; transcribed from arcadia/net.inc's (itself the Intellivision's
; validate_state).
;
; Minimum, not exact: the serializer's per-player append makes the place phase
; variable and a longer reply is harmless. A SHORTER one means the fixed
; offsets would read stale bytes out of a window that is never cleared, which
; is the failure this exists to stop.
;
;   tables    any nonzero length
;   lobby     59 + 10 * playerCount
;   place     49          (records are name+status only)
;   in play   49 + 115 * playerCount
        IF      BSBANK=BANKLOB
VALID8: lda     BSRXL0          ; tables: any nonzero length will do
        ora     BSRXL1
        beq     VALBAD
        lda     #FNEOK
        rts
        ELSE
VALID8: lda     BSRXL1          ; a header at minimum, always
        bne     VALG1
        lda     BSRXL0
        cmp     #GHDRLEN
        bcc     VALBAD
VALG1:  lda     FNRPLY+GOPLCNT
        cmp     #MAXPLYR+1
        bcs     VALBAD
        sta     BSPCNT

        lda     FNRPLY+GOSTAT
        sta     BSSTAT
        bne     VALG2
        ; lobby: 59 + 10 * playerCount
        lda     #0
        ldx     BSPCNT
        beq     VALG1B
VALG1A: clc
        adc     #LBSTRID
        dex
        bne     VALG1A
VALG1B: clc
        adc     #GLPLYRS
        jmp     VALMIN          ; always < 256

VALG2:  cmp     #STPLACE
        bne     VALG3
        lda     #GIPLYRS        ; place: the fixed header only
        jmp     VALMIN

VALG3:  ; in play: 49 + 115 * playerCount, which passes 256 at two players
        lda     #0
        sta     BSTMP           ; the high byte accumulates here
        lda     #GIPLYRS
        ldx     BSPCNT
        beq     VALMIN
VALG3A: clc
        adc     #PLSTRID
        bcc     VALG3B
        inc     BSTMP
VALG3B: dex
        bne     VALG3A
        ldx     BSTMP
        beq     VALMIN
        ; 16-bit compare: high byte first
        cpx     BSRXL1
        beq     VALLO
        bcc     VALOK
        bcs     VALBAD
VALLO:  cmp     BSRXL0
        beq     VALOK
        bcc     VALOK
        bcs     VALBAD

; VALMIN -- A is the minimum length and it is under 256.
VALMIN: ldx     BSRXL1
        bne     VALOK           ; the reply is 256+, so it is long enough
        cmp     BSRXL0
        beq     VALOK
        bcs     VALBAD
VALOK:  lda     FNRPLY+GOPLST
        sta     BSMYST
        lda     FNRPLY+GOACTPL
        sta     BSACT
        lda     #FNEOK
        rts
        ENDIF
VALBAD: lda     #FNEBADL
        sta     BSERR
        lda     #4
        sta     BSSTEP
        lda     #$FF
        rts
