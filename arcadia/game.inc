; game.inc -- the single poll loop and everything it renders.
;
; There is no state variable and no dispatch table: the server's status
; byte IS the state, and CHKNEW maps it to one of four screen classes.
; Every poll is one opaque redraw -- no dirty rectangles, no digests --
; and a change of class or seat count triggers a full clear plus the
; statics, which is the only thing overdraw cannot fix (content that
; SHRANK).
;
; The five invariants the family has paid for, all of them live here:
;   1. PRVACT advances at the bottom of EVERY successful poll, lobby
;      included, so the lobby -> play transition rides the same
;      comparison as any other turn edge, and the cue fires BEFORE
;      targeting rather than after.
;   2. The ready toggle is sampled in the WAIT loop (every frame), never
;      once per poll -- a toggle read once every two seconds feels dead.
;   3. VALID8 has already gated the reply before anything here reads it.
;   4. Targeting is time-sliced and bails; nothing blocks forever.
;   5. One cursor: /attack carries a position and hits everyone.

GAMEIN: BSTA,UN GSTATIC
        LODI,R0 1
        STRA,R0 VIEWPL
; falls into the poll loop

GAMELP: LODA,R0 PENDACT         ; a staged action rides the next request
        BCFR,EQ GLREQ           ;   and its reply IS the next state
        LODI,R0 RQSTATE
GLREQ:  BSTA,UN APICALL
        BCFA,EQ GLFAIL
        LODA,R0 PENDACT         ; was that the /leave? then stop watching
        COMI,R0 RQLEAVE         ;   a table we are no longer in -- Clear
        BCTA,EQ GLGONE          ;   is staged from three different screens
        LODI,R0 0
        STRA,R0 PENDACT         ; consumed
        BSTA,UN GHDR
        BSTA,UN GSOUND
        BSTA,UN CHKNEW
        LODA,R0 V_ST            ; dispatch on the class CHKNEW derived
        BSTA,UN GCLASS
        COMI,R0 0
        BCTA,EQ GLLOB
        COMI,R0 1
        BCTA,EQ GLPLC
        COMI,R0 3
        BCTA,EQ GLOVER
; ---- class 2: in play --------------------------------------------------
        BSTA,UN VWCLMP
        BSTA,UN RENDER
        LODA,R0 V_PLST
        COMI,R0 PSPLAY
        BCFR,EQ GLW2            ; defeated or spectating: just watch
        LODA,R0 V_ACT
        BCFR,EQ GLW2            ; not your turn
        LODA,R0 V_ACT           ; the turn EDGE, cued before targeting
        COMA,R0 PRVACT
        BCTR,EQ GLT1
        BSTA,UN SNDTRN
GLT1:   BSTA,UN GTAIL           ; PRVACT advances before we block
        BSTA,UN TARGET
        BCTA,UN GAMELP
GLW2:   BSTA,UN GTAIL
        LODI,R0 120             ; ~2 s
        BCTA,UN GLWAIT

GLGONE: LODI,R0 0
        STRA,R0 PENDACT
        BCTA,UN LOBBY

; ---- class 0: the ready-up lobby ---------------------------------------
GLLOB:  BSTA,UN LOBBYR
        BSTA,UN GTAIL
        LODI,R0 120
        BCTA,UN GLWAIT

; ---- class 1: placement ------------------------------------------------
GLPLC:  LODA,R0 V_PLST
        COMI,R0 PSPLACE
        BCFR,EQ GLPLW
        BSTA,UN GTAIL
        BSTA,UN PLACSCR         ; blocking, by design: the server waits
        BCTA,UN GAMELP
GLPLW:  DAT     STATROW,0       ; already placed: wait for the others
        DCOL    ATEXT
        LODI,R1 16
        SETSTR  TWAITP
        BSTA,UN DPRTN
        BSTA,UN GTAIL
        LODI,R0 60
        BCTA,UN GLWAIT

; ---- class 3: game over ------------------------------------------------
GLOVER: BSTA,UN RENDGO
        BSTA,UN GTAIL
        LODI,R0 180
        BCTA,UN GLWAIT

; ---- the wait -----------------------------------------------------------
; R0 = frames. One KEYSCN costs exactly one frame, so the countdown and
; the input scan are the same loop -- which is why the ready toggle is
; responsive without a second timer.
GLWAIT: STRA,R0 V_TICK
GLW1:   BSTA,UN KEYSCN
        COMI,R0 KNONE
        BCFR,EQ GLKEY
GLW3:   LODA,R0 V_TICK
        SUBI,R0 1
        STRA,R0 V_TICK
        BCFA,EQ GLW1
        BCTA,UN GAMELP
GLKEY:  COMI,R0 KCLR            ; Clear: leave the table
        BCTR,EQ GLKLV
        COMI,R0 0               ; 0: poll now
        BCTA,EQ GAMELP
        COMI,R0 KENT            ; Enter or FIRE: ready up, but only in
        BCTR,EQ GLKRDY          ;   the lobby -- elsewhere it is noise
        COMI,R0 2
        BCTR,EQ GLKRDY
        COMI,R0 4
        BCTA,EQ GLKVW
        COMI,R0 6
        BCTA,EQ GLKVW
        BCTA,UN GLW3
GLKLV:  LODI,R0 RQLEAVE
        STRA,R0 PENDACT
        BCTA,UN GAMELP
GLKRDY: LODA,R0 V_ST
        BCFA,EQ GLW3            ; not the lobby: ignore
        LODI,R0 RQREADY
        STRA,R0 PENDACT
        BSTA,UN SNDSEL
        BCTA,UN GAMELP
GLKVW:  LODA,R0 V_ST            ; page the enemy board between polls
        COMI,R0 STSTART
        BCFA,LT GLW3
        LODA,R0 VIEWPL
        ADDI,R0 1
        STRA,R0 VIEWPL
        BSTA,UN VWCLMP
        BSTA,UN DRAWEB
        BSTA,UN FOOTER
        BSTA,UN SNDMOVE
        BCTA,UN GLW3

; ---- a failed round trip ------------------------------------------------
GLFAIL: DAT     STATROW,0
        DCOL    AALERT
        LODI,R1 16
        SETSTR  TENET2
        BSTA,UN DPRTN
        BSTA,UN SNDERR
        LODI,R0 120
        STRA,R0 V_TICK
GLF1:   BSTA,UN KEYSCN
        COMI,R0 KCLR
        BCTR,EQ GLFLV
        LODA,R0 V_TICK
        SUBI,R0 1
        STRA,R0 V_TICK
        BCFR,EQ GLF1
        BCTA,UN GAMELP
GLFLV:  BCTA,UN LOBBY

; ---- header, class, edges ----------------------------------------------
; GHDR: cache the four header fields this loop dispatches on. status,
; playerStatus and activePlayer are adjacent, so one seek serves three.
; Calls two down (RXHDR -> RXNEXT). Clobbers R0-R3.
GHDR:   LODI,R1 GOPLCNT
        BSTA,UN RXHDR
        STRA,R0 V_PC
        LODI,R1 GOSTAT
        BSTA,UN RXHDR
        STRA,R0 V_ST
        BSTA,UN RXNEXT
        STRA,R0 V_PLST
        BSTA,UN RXNEXT
        STRA,R0 V_ACT
        RETC,UN

; GCLASS: R0 = status -> R0 = screen class (0 lobby, 1 place, 2 play,
; 3 over). Leaf. Clobbers R0.
GCLASS: COMI,R0 STLOBBY
        BCTR,EQ GC0
        COMI,R0 STPLACE
        BCTR,EQ GC1
        COMI,R0 STOVER
        BCTR,EQ GC3
        LODI,R0 2
        RETC,UN
GC0:    LODI,R0 0
        RETC,UN
GC1:    LODI,R0 1
        RETC,UN
GC3:    LODI,R0 3
        RETC,UN

; CHKNEW: opaque overdraw cannot repair content that SHRANK, so a change
; of screen class or seat count earns a full clear and a fresh set of
; statics. Calls four down (GSTATIC -> BRDFRM -> DFILL -> DPUTC).
; Clobbers R0-R3.
CHKNEW: LODA,R0 V_ST
        BSTA,UN GCLASS
        STRZ    R1
        COMA,R0 PRVCLS
        BCFR,EQ CHKN1
        LODA,R0 V_PC
        COMA,R0 PRVPC
        BCTR,EQ CHKN9
CHKN1:  STRA,R1 PRVCLS
        BSTA,UN GSTATIC
CHKN9:  LODA,R0 V_PC
        STRA,R0 PRVPC
        RETC,UN

; GSOUND: one cue per resolved shot, on the STATUS EDGE -- so an
; opponent's shot is heard too, which is most of the drama in a
; four-player game. Calls two down. Clobbers R0,R1,R3.
GSOUND: LODA,R0 V_ST
        COMA,R0 PRVST
        BCTR,EQ GSN9            ; nothing resolved since the last poll
        LODA,R0 V_ST
        COMI,R0 STMISS
        BCTR,EQ GSNMS
        COMI,R0 STHIT
        BCTR,EQ GSNHT
        COMI,R0 STSUNK
        BCTR,EQ GSNSK
        COMI,R0 STOVER
        BCTR,EQ GSNOV
GSN9:   RETC,UN
GSNMS:  BCTA,UN SNDMISS
GSNHT:  BCTA,UN SNDHIT
GSNSK:  BCTA,UN SNDSUNK
GSNOV:  BCTA,UN SNDOVER

; GTAIL: the bottom of every successful poll. PRVACT and PRVST advance
; HERE and nowhere else -- putting them in the class-2 path is what
; makes the lobby -> play cue miss. Leaf. Clobbers R0.
GTAIL:  LODA,R0 V_ACT
        STRA,R0 PRVACT
        LODA,R0 V_ST
        STRA,R0 PRVST
        RETC,UN

; ---- renderers ----------------------------------------------------------
; RENDER: one opaque repaint of everything that moves.
; Calls four down (DRAWEB -> RXNEXT -> MSLICE). Clobbers R0-R3.
RENDER: BSTA,UN PROMPT
        BSTA,UN DRAWEB
        BSTA,UN DRAWYB
        BSTA,UN ALLPIP
        BCTA,UN FOOTER          ; tail call

; RENDGO: game over. v=2 puts the winner's index in activePlayer, so the
; enemy half switches to their board and the server's win message lands
; in the prompt. Their ship LAYOUT also arrives (myShips[5-9]) but is not
; drawn: SHIPDR is hardwired to your board's row base, and giving it a
; second one needs a RAM cell this machine does not have spare.
; Calls four down. Clobbers R0-R3.
RENDGO: LODA,R0 V_ACT           ; v=2 puts the WINNER in activePlayer;
        COMI,R0 1               ;   show their board, unless it is yours
        BCTR,LT RGO1            ;   (0) or nobody won ($FF)
        COMI,R0 4
        BCTR,GT RGO1
        STRA,R0 VIEWPL
RGO1:   BCTA,UN RENDER          ; tail call: the rest is an ordinary repaint

; PROMPT: the server's status line, truncated to the first 11 cells,
; then the move clock. Calls four down (RXPRNT -> RXNEXT -> MSLICE).
; Clobbers R0-R3.
PROMPT: LODI,R1 0               ; SEEK FIRST: RXSEEK/PLSEEK/LBSEEK take
        LODI,R2 GOPRMPT         ;   the offset in R2, which is also the
        BSTA,UN RXSEEK          ;   screen cell cursor -- seeking after
        BSTA,UN RXNEXT          ;   DAT would silently move the write
        STRA,R0 V_BTMP          ; peek: is the server's prompt empty?
        LODI,R1 0
        LODI,R2 GOPRMPT
        BSTA,UN RXSEEK
        DAT     STATROW,0
        DCOL    ATEXT
        LODA,R0 V_BTMP
        BCTR,EQ PRMLOC
        LODI,R1 11
        BSTA,UN RXPRNT
        BCTA,UN PRMNUM
; The server sets Prompt = "" for the whole of play on purpose
; (gameLogic.go: "updating prompt based on attack result was getting too
; busy"), so this row would otherwise sit blank exactly when there is
; most to say. Fill it from what we already know.
PRMLOC: LODA,R0 V_ACT
        BCFR,EQ PRMR
        SETSTR  TYTURN
        BCTR,UN PRMP
PRMR:   LODA,R0 V_ST
        COMI,R0 STMISS
        BCFR,EQ PRMR1
        SETSTR  TRMISS
        BCTR,UN PRMP
PRMR1:  COMI,R0 STHIT
        BCFR,EQ PRMR2
        SETSTR  TRHIT
        BCTR,UN PRMP
PRMR2:  COMI,R0 STSUNK
        BCFR,EQ PRMR3
        SETSTR  TRSUNK
        BCTR,UN PRMP
PRMR3:  SETSTR  TBLANK
PRMP:   LODI,R1 11
        BSTA,UN DPRTN
PRMNUM: LODI,R1 GOMVTIM
        BSTA,UN RXHDR
        STRZ    R1
        EORZ    R0
        STRA,R0 V_NUM
        STRA,R1 V_NUM+1
        BCTA,UN NUMPRT          ; tail call

; FOOTER: the viewed opponent's name and the cursor coordinate.
; Calls four down. Clobbers R0-R3.
FOOTER: LODA,R0 VIEWPL          ; seek before positioning (see PROMPT)
        STRZ    R1
        LODI,R2 PLNAME
        BSTA,UN PLSEEK
        DAT     FOOTROW,0
        DCOL    ATEXT
        LODI,R1 9
        BSTA,UN RXPRNT
        LODI,R0 0
        BSTA,UN DPUTC
        LODA,R0 CURX            ; columns read as letters, rows as digits
        ADDI,R0 'A'
        BSTA,UN DXLAT
        BSTA,UN DPUTC
        LODA,R0 CURY
        ADDI,R0 '0'
        BSTA,UN DXLAT
        BSTA,UN DPUTC
        LODI,R1 0
        LODI,R3 4
        BCTA,UN DFILL           ; tail call

; ALLPIP: both pip columns. Calls four down. Clobbers R0-R3.
ALLPIP: LODA,R0 VIEWPL
        STRZ    R1
        LODI,R2 PLSHIPS
        BSTA,UN PLSEEK
        LODI,R1 EBROW
        BSTA,UN PIPS
        LODI,R1 0
        LODI,R2 PLSHIPS
        BSTA,UN PLSEEK
        LODI,R1 YBROW
        BCTA,UN PIPS            ; tail call

; LOBBYR: the ready-up roster, drawn inside the enemy frame.
; Calls four down. Clobbers R0-R3.
LOBBYR: DAT     STATROW,0
        DCOL    ATEXT
        LODI,R1 16
        SETSTR  TLOBBY
        BSTA,UN DPRTN
        LODI,R0 0
        STRA,R0 V_ROW
LBR1:   LODA,R0 V_ROW
        COMA,R0 V_PC
        BCFA,LT LBR9
        LODA,R0 V_ROW           ; the ready flag FIRST: LBSEEK wants the
        STRZ    R1              ;   field in R2 and would otherwise wipe
        LODI,R2 LBSTAT          ;   the screen cursor out from under the
        BSTA,UN LBSEEK          ;   name that follows
        BSTA,UN RXNEXT
        COMI,R0 PSREADY
        BCFR,EQ LBR2
        LODI,R0 GPIP
        BCTR,UN LBR2A
LBR2:   LODI,R0 0
LBR2A:  STRA,R0 V_BTMP
        LODA,R0 V_ROW
        STRZ    R1
        LODI,R2 LBNAME
        BSTA,UN LBSEEK
        LODA,R0 V_ROW
        ADDI,R0 EBROW
        STRZ    R1
        LODI,R2 BCOL
        BSTA,UN DPOS
        DCOL    AWATER
        LODI,R1 9
        BSTA,UN RXPRNT
        LODA,R0 V_BTMP          ; ready shows as a filled pip
        BSTA,UN DPUTC
        LODA,R0 V_ROW
        ADDI,R0 1
        STRA,R0 V_ROW
        BCTA,UN LBR1
LBR9:   DAT     FOOTROW,0
        DCOL    ATEXT
        LODI,R1 16
        SETSTR  TREADY
        BCTA,UN DPRTN           ; tail call

