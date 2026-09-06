; main.inc -- boot, the game-screen statics, and small shared helpers.
;
; The top level is a dispatch of ABSOLUTE BRANCHES, not calls: NAMESCR
; returns, but LOBBY / GAMEIN / GAMELP chain with BCTA so no screen ever
; accumulates a return-stack frame. That is what keeps the worst chain
; inside the 2650's 8-entry RAS.

DEFNAM: DB      "ARCADIA",0

START:  EORZ    R0
        LPSU
        LODI,R0 $02             ; COM=1: logical compares everywhere
        LPSL
        LODI,R3 7               ; default name -> PLNBUF = "ARCADIA"
DEFCP:  LODA,R0 DEFNAM,R3
        STRA,R0 PLNBUF,R3
        BDRR,R3 DEFCP
        LODA,R0 DEFNAM
        STRA,R0 PLNBUF
        LODI,R0 JOYCTR          ; the pot caches must start centred or the
        STRA,R0 AXV             ; first scan reads a phantom direction
        STRA,R0 AXH
        LODI,R0 0
        STRA,R0 AXSEL
        STRA,R0 PENDACT
        STRA,R0 VIEWPL
        LODI,R0 KNONE
        STRA,R0 V_KEY
        EORZ    R0              ; no table joined yet -- an uninitialised
        STRA,R0 TBLBUF          ;   id would ride out in the next URL
        STRA,R0 V_CNT           ; and no table list fetched yet
        LODI,R0 5               ; park the cursor mid-board
        STRA,R0 CURX
        STRA,R0 CURY
        BSTA,UN UDCLD           ; one bank, loaded once: no per-screen swap
        BSTA,UN SEEDRD
    IF DEMO
        BCTA    UN,DEMO0
    ELSEIF
        BSTA,UN MCHECK
        BCTR,EQ MHAVE
        BSTA,UN DINI13          ; no cart: say so and stop
        BSTA,UN DCLSU
        DAT     5,1
        DCOL    AALERT
        SETSTR  SNOFN
        BSTA,UN DPRINT
MHALT:  BCTA,UN MHALT
MHAVE:  BSTA,UN NAMESCR
        BCTA,UN LOBBY
    ENDIF


; GSTATIC: flip to the 26-line game screen and lay down everything that
; never changes -- both board frames. DCLSL runs FIRST: $1A00-$1ACF was
; 13-line scratch (the name editor lives there) and becomes visible
; screen the instant 26-line mode comes on.
; Calls three down (BRDFRM -> DPOS). Clobbers R0-R3.
GSTATIC: BSTA,UN DCLSL
        BSTA,UN DCLSU
        BSTA,UN DINI26
        BSTA,UN SPRINI
        LODI,R1 EBTOP
        BSTA,UN BRDFRM
        LODI,R1 YBTOP
        BSTA,UN BRDFRM
        RETC,UN

; ---- the LFSR ----------------------------------------------------------
; A 16-bit Galois LFSR (poly mask $2D, period 65535), stepped a full 8
; bits per call so consecutive outputs are independent bytes -- the
; astrocade port's generator, retargeted.
;
; The 16-bit shift is done with the 2650's carry: WC is turned ON so
; ADDZ R0 doubles a byte and carries between them, and turned OFF again
; before returning because the REST OF THE CODEBASE ASSUMES WC = 0 (RRL
; is used as a circular rotate everywhere else).

RNDL    EQU     RNDST           ; low byte
RNDH    EQU     RNDST+1

; SEEDRD: (re)seed from what the machine can offer -- the cart's ACKSEQ,
; which survives a console RESET and has advanced once by placement
; time, mixed with the raw pot. Called at boot and again when placement
; starts, which is the draw that actually matters.
; Calls one down (MACKSQ). Clobbers R0,R1.
SEEDRD: LODA,R0 JOYP1
        IORI,R0 $01             ; never seed the LFSR all-zero
        STRA,R0 RNDL
        BSTA,UN MACKSQ
        EORI,R0 $A5
        IORI,R0 $01
        STRA,R0 RNDH
        RETC,UN

; RND: R0 = the next pseudorandom byte.
; PRESERVES R1 and R2 -- RNDMOD depends on that (see the note there).
; Leaf. Clobbers R0,R3.
RND:    PPSL    $08             ; WC on for the 16-bit shift
        CPSL    $01             ; carry clear going in
        LODI,R3 8
RND1:   LODA,R0 RNDL
        ADDZ    R0              ; lo = lo*2 + carry-in
        STRA,R0 RNDL
        LODA,R0 RNDH
        ADDZ    R0              ; hi = hi*2 + carry out of lo
        STRA,R0 RNDH
        TPSL    $01             ; the bit that left bit 15
        BCFR,EQ RND2
        LODA,R0 RNDL
        EORI,R0 $2D             ; maximal 16-bit polynomial
        STRA,R0 RNDL
        CPSL    $01             ; consumed: do not carry it into the next step
RND2:   BDRR,R3 RND1
        CPSL    $09             ; WC off, carry clear -- restore the
        LODA,R0 RNDL            ;   convention the whole codebase runs on
        IORA,R0 RNDH
        BCFR,EQ RND3
        LODI,R0 1               ; never park on zero
        STRA,R0 RNDL
RND3:   LODA,R0 RNDL
        RETC,UN

; RNDMOD: R0 = a value 0 .. R1-1 (R1 >= 1).
; The modulus stays in R1 across the RND call, which is safe ONLY
; because RND touches R0 and R3 and nothing else -- the astrocade port's
; placement bug was exactly this pattern with a helper that did clobber
; the register, and every ship ended up seeded at the same column. If
; RND ever grows to use R1, this breaks silently.
; Calls one down (RND). Clobbers R0,R3. Preserves R1.
RNDMOD: BSTA,UN RND
RMOD1:  COMZ    R1
        BCTR,LT RMOD9
        SUBZ    R1
        BCTR,UN RMOD1
RMOD9:  RETC,UN
