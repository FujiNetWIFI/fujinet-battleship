; ---------------------------------------------------------------------------
; game.inc -- the poll loop and everything it renders.
;
; There is no state variable for the game: the SERVER'S STATUS BYTE IS THE
; STATE, and GCLASS maps it to one of four screen classes. That is the Arcadia
; port's design and it carries over unchanged.
;
; The five invariants, every one of them learned on an earlier console:
;
;   1. VPRVACT advances at the bottom of EVERY successful poll, lobby
;      included, so the lobby -> play transition rides the same comparison as
;      any other turn edge, and the your-turn cue fires BEFORE targeting
;      rather than after it.
;   2. The ready toggle is sampled in the WAIT loop, every ~8 ms, never once
;      per poll -- a toggle read once every two seconds feels dead.
;   3. VALID8 gates the reply before anything here reads a byte of it. The
;      reply window is never cleared, so a short read leaves stale bytes from
;      a longer previous one.
;   4. Targeting is time-sliced and bails. The server skips a turn that runs
;      out; a client that blocks forever never polls again and never learns
;      the game moved on.
;   5. One cursor: /attack carries a position and hits every live opponent, so
;      the reticle paints on every board that shot will reach.
;
; And a sixth this machine adds: NSETTLE's minimum length comes from GMINLN
; run against the PREVIOUS poll's seat count, so the settle does not have to
; guess. If the count shrank, VALID8 rejects the short reply and the next poll
; retries with a lower floor.
; ---------------------------------------------------------------------------

; --- queued cues ----------------------------------------------------------
CUENONE	EQU 0
CUETURN	EQU 1
CUEMISS	EQU 2
CUEHIT	EQU 3
CUESUNK	EQU 4
CUEOVER	EQU 5

; --- the footer's two text fields ----------------------------------------
; Eight cells on the left, eight on the right, and your own five ship pips in
; the 23 pixels between them. Nothing here may write a pixel the pips will
; overwrite, or the other way round.
FOOTW	EQU 8
FOOTRX	EQU 64

; --- poll pacing ----------------------------------------------------------
; An idle pass is INSCAN plus DELAYMS(IDLEMS), and A DELAYMS UNIT IS ABOUT
; 9 MS, not one -- so a pass is roughly 18 ms and these counts are the poll
; interval in fiftieths of a second. Getting that unit wrong the first time
; turned a two-second lobby poll into half a minute.
IDLEMS	EQU 2			; ~18 ms per input scan
TICKLOB	EQU 110			; lobby: ~2 s
TICKPLC	EQU 55			; waiting on other players to place: ~1 s
TICKPLY	EQU 80			; in play: ~1.5 s
TICKOVR	EQU 170			; game over: ~3 s; the server restarts on its own
TICKTURN EQU 4			; while it is your turn: straight back to aiming

; ---------------------------------------------------------------------------
SGAME:	ENTER
	LI LAY_TXT
	LR 0,A
	PI LAYLD
	LI CVAL2
	LR 3,A
	LI DORGX
	LR 1,A
	LI DORGY+3*DCELLH
	LR 2,A
	DCI SJOIN
	PI DSTR
	DCI VPEND
	CLR
	ST			; nothing staged: the first poll is a plain /state
	DCI VBLINK
	LIS 8
	ST

; ---------------------------------------------------------------------------
SGPOLL:	DCI VPEND		; a staged action rides the next request, and
	LM			;   its reply IS the next state -- there is no
	OI 0			;   second round trip to see what it did.
	BNZ SGPQ		; Zero means nothing is staged. It cannot mean
	LI RQSTATE		;   RQSTATE, because RQTABLE is request 0 and a
SGPQ:	DCI VREQ		;   cleared VPEND then refetches the table list
	ST			;   forever, which is exactly what it did.
	DCI VPEND
	CLR
	ST

	IF DEMO
	PI GDEMO
	ELSE
	; the settle floor, from what the LAST poll saw
	DCI VPRVST
	LM
	DCI VTMP
	ST
	DCI VPRVPC
	LM
	DCI VTMP2
	ST
	PI GMINLN
	LI GMAXLEN&0FFH
	LR 0,A
	LI GMAXLEN>>8
	LR 1,A
	PI NFETCH
	OI 0
	BZ SGVAL
	DCI VFAILC
	ST
	JMP FAIL

SGVAL:	PI VALID8
	OI 0
	BZ SGHDR
	DCI VFAILC
	ST
	JMP FAIL

SGHDR:	PI GHDR
	ENDIF

	PI GCLASS
	PI GCUES		; the edges, BEFORE the draw
	PI GLAY
	PI GDRAW
	PI GPLAY		; and the cue AFTER it, over the finished picture
	PI GADV			; advance the previous-poll shadows

	; --- the interactive phases hang off the bottom of the poll ----------
	DCI VCLASS
	LM
	CI CLPLACE
	BNZ SGP1
	DCI VPLST
	LM
	CI PSPLACE
	BNZ SGWAIT
	PI SPLCSC		; we still owe five ships
	BR SGWAIT
SGP1:	CI CLPLAY
	BNZ SGWAIT
	DCI VACT
	LM
	OI 0
	BNZ SGWAIT
	DCI VPLST
	LM
	OI 0
	BNZ SGWAIT		; defeated or watching: no turn to take
	PI STGT			; time-sliced; it returns, it never blocks

; ---------------------------------------------------------------------------
; SGWAIT -- hold the picture, watching for input, then poll again.
; ---------------------------------------------------------------------------
SGWAIT:	PI GTICK
SGIDLE:	PI INSCAN
	LR A,0
	CI EV_NONE
	BNZ SGKEY
	PI RND			; stir the LFSR from the player's own timing, so
				;   the seeded fleet is not the same every boot
	LI IDLEMS
	LR 3,A
	PI DELAYMS
	DCI VTICK
	LM
	AI 0FFH
	DCI VTICK
	ST
	BNZ SGIDLE
	JMP SGPOLL

; SGKEY -- one event, sampled every ~20 ms rather than once per poll.
SGKEY:	LR A,0
	CI EV_BACK
	BNZ SGK1
	LI RQLEAVE		; MODE leaves the table
	LR 0,A
	DCI VREQ
	ST
	PI NFETCH0
	LI ST_TABLE
	LR 0,A
	JMP GOTO
SGK1:	CI EV_PGUP
	BNZ SGK2
	LI ST_HELP
	LR 0,A
	JMP GOTO
SGK2:	CI EV_SEL
	BNZ SGK3
	DCI VCLASS
	LM
	CI CLLOBBY
	BNZ SGK3
	; FIRE toggles ready, sampled HERE rather than once per poll: a quick
	; tap between polls is not missed, and because /ready/1 and /ready/0
	; carry the value we want, one press cannot ready and un-ready across
	; two polls the way the deprecated bare /ready does.
	DCI VREADY
	LM
	XI 1
	DCI VREADY
	ST
	LI RQREADY
	LR 0,A
	DCI VPEND
	ST
	JMP SGPOLL
SGK3:	JMP SGIDLE

; ---------------------------------------------------------------------------
; GTICK -- how long to hold this picture before polling again.
;   Leaf. Clobbers A, DC0.
; ---------------------------------------------------------------------------
GTICK:	DCI VCLASS
	LM
	CI CLPLAY		; your own turn: poll at once and get back to
	BNZ GT0			;   aiming. Targeting is time-sliced, so the wait
	DCI VACT		;   loop between slices is dead time in which a
	LM			;   stick press is discarded -- keep it short.
	OI 0
	BNZ GT0
	LI TICKTURN
	BR GTX
GT0:	DCI VCLASS
	LM
	CI CLLOBBY
	BNZ GT1
	LI TICKLOB
	BR GTX
GT1:	CI CLPLACE
	BNZ GT2
	LI TICKPLC
	BR GTX
GT2:	CI CLOVER
	BNZ GT3
	LI TICKOVR
	BR GTX
GT3:	LI TICKPLY
GTX:	DCI VTICK
	ST
	POP

; ---------------------------------------------------------------------------
; NFETCH0 -- fire a request whose reply nobody reads (/leave). The result is
;   ignored on purpose: we are walking away either way.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
NFETCH0: ENTER
	IF DEMO
	ELSE
	LI GMAXLEN&0FFH
	LR 0,A
	LI GMAXLEN>>8
	LR 1,A
	DCI VMIN0
	LIS 1
	ST
	CLR
	ST
	PI NFETCH
	ENDIF
	LEAVE

; ---------------------------------------------------------------------------
; GHDR -- lift the 38-byte header out of the reply window.
;   Leaf. Clobbers A, DC0.
; ---------------------------------------------------------------------------
GHDR:	DCI FNREPLY+GOPLCNT
	LM
	DCI VPC
	ST
	DCI FNREPLY+GOSTAT
	LM
	DCI VST
	ST
	DCI FNREPLY+GOPLST
	LM
	DCI VPLST
	ST
	DCI FNREPLY+GOACTPL
	LM
	DCI VACT
	ST
	DCI FNREPLY+GOMVTIM
	LM
	DCI VMVT
	ST
	DCI FNREPLY+GILASTP
	LM
	DCI VLASTP
	ST
	POP

; ---------------------------------------------------------------------------
; GCLASS -- the server's status byte, collapsed to the four screens the client
;   actually has.
;   Leaf. Clobbers A, DC0.
; ---------------------------------------------------------------------------
GCLASS:	DCI VST
	LM
	CI STLOBBY
	BNZ GC1
	LI CLLOBBY
	BR GCX
GC1:	CI STPLACE
	BNZ GC2
	LI CLPLACE
	BR GCX
GC2:	CI STOVER
	BNZ GC3
	LI CLOVER
	BR GCX
GC3:	LI CLPLAY
GCX:	DCI VCLASS
	ST
	CI CLPLACE		; leaving the placement phase forgets the seeded
	BZ GCXX			;   fleet, so the NEXT game seeds a fresh one.
	DCI VSEEDED		;   Without this the client comes back to
	CLR			;   placement with all five already committed and
	ST			;   FIRE walks the ship index past its terminator.
GCXX:	POP

; ---------------------------------------------------------------------------
; GLAY -- pick the layout this class and seat count want, and load it.
;   Reached by PI; the closing JMP is a tail call into LAYLD, whose LEAVE
;   returns to OUR caller.
; ---------------------------------------------------------------------------
GLAY:	DCI VCLASS
	LM
	CI CLLOBBY
	BZ GLTXT
	CI CLPLACE
	BNZ GL2
	DCI VPLST		; still owe ships: the placement board.
	LM			;   Otherwise there is nothing to place and the
	CI PSPLACE		;   screen is a text one.
	BNZ GLTXT
	LI LAY_PLC
	BR GLX
GL2:	DCI VPC
	LM
	CI 2
	BP GLHU			; two seats or fewer: the big 4x4 cells
	LI LAY_QD
	BR GLX
GLHU:	LI LAY_HU
	BR GLX
GLTXT:	LI LAY_TXT
GLX:	LR 0,A
	JMP LAYLD

; ---------------------------------------------------------------------------
; GMINLN -- VMIN = the length the status in VTMP and the seat count in VTMP2
;   imply. Used twice: as NSETTLE's floor before the fetch (against the last
;   poll's numbers) and as VALID8's test after it (against this one's).
;
;     lobby  59 + 10*pc     place  49     game/over  49 + 115*pc
;
;   A minimum, not an exact length: the serializer's per-player append makes
;   the place phase variable and a longer reply is harmless. A SHORTER one
;   means the fixed offsets would read stale bytes out of a window that is
;   never cleared.
;   Leaf. Clobbers A, r0-r4, DC0.
; ---------------------------------------------------------------------------
GMINLN:	DCI VTMP2
	LM
	LR 0,A
	DCI VTMP
	LM
	CI STLOBBY
	BNZ GML1
	LI GLPLYRS
	LR 1,A
	LI LBSTRID
	LR 2,A
	BR GMLADD
GML1:	CI STPLACE
	BNZ GML2
	LI GIPLYRS
	LR 1,A
	CLR
	LR 2,A
	BR GMLADD
GML2:	LI GIPLYRS
	LR 1,A
	LI PLSTRID
	LR 2,A
GMLADD:	LR A,1
	LR 3,A
	CLR
	LR 4,A
	LR A,0
	OI 0
	BZ GMLX
GMLL:	LR A,3
	AS 2
	LR 3,A			; LR leaves the flags alone, so the carry from
	BNC GMLN		;   the AS survives to here
	LR A,4
	INC
	LR 4,A
GMLN:	DS 0
	BNZ GMLL
GMLX:	DCI VMIN0
	LR A,3
	ST
	LR A,4
	ST
	POP

; ---------------------------------------------------------------------------
; GCMPRX -- A = 0 if the captured reply length is at least VMIN, else 1.
;   Unsigned, and it has to be: subtracting and testing the SIGN is a signed
;   comparison, and 181 - 1 is 180 with bit 7 set.
;   Leaf. Clobbers A, r4, r5, DC0.
; ---------------------------------------------------------------------------
GCMPRX:	DCI VMIN0
	LM
	COM
	INC			; -min low; carry set if the low byte was 0
	LR 4,A
	DCI VMIN1
	LM
	COM
	LNK
	LR 5,A
	DCI VRXLO
	LM
	AS 4
	DCI VRXHI
	LM
	LNK			; the carry from the low half
	AS 5
	BC GCMPOK
	LI 1
	POP
GCMPOK:	CLR
	POP

; ---------------------------------------------------------------------------
; VALID8 -- sanity-check the reply before anything believes a byte of it.
;   Seats on the enum, a status on the enum, and at least the length the two
;   of them imply.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
VALID8:	ENTER
	DCI FNREPLY+GOPLCNT
	LM
	CI MAXPLR
	BM VALBAD		; more seats than the game has
	OI 0
	BZ VALBAD		; and an empty table is not a reply either
	LR 0,A
	DCI FNREPLY+GOSTAT
	LM
	LR 1,A
	DCI STATTB
	LI 7
	LR 2,A
VAL1:	LM
	XS 1
	BZ VAL2
	DS 2
	BNZ VAL1
	BR VALBAD		; a status nothing downstream can dispatch on
VAL2:	DCI VTMP
	LR A,1
	ST
	DCI VTMP2
	LR A,0
	ST
	PI GMINLN
	PI GCMPRX
	OI 0
	BNZ VALBAD
	CLR
	LEAVE
VALBAD:	LI 0EEH
	LEAVE

STATTB:	DB STLOBBY,STPLACE,STSTART,STMISS,STHIT,STSUNK,STOVER

; ---------------------------------------------------------------------------
; GCUES -- queue at most one cue, from the edges this poll crossed.
;
;   THE TURN EDGE IS THE SUBTLE ONE. activePlayer stays 0 for every poll of
;   your turn, so testing the level machine-guns the cue; and because VPRVACT
;   advances on every successful poll including the lobby's, the
;   lobby -> play transition rides this same comparison.
;
;   The result cue rides the STATUS edge rather than your own shot, so an
;   opponent's shot is heard too.
;   Leaf. Clobbers A, r0, DC0.
; ---------------------------------------------------------------------------
GCUES:	DCI VCUEQ
	CLR
	ST

	DCI VST
	LM
	LR 0,A
	DCI VPRVST
	LM
	XS 0
	BZ GCU2			; the status did not move

	LR A,0
	CI STOVER
	BNZ GCU1
	LI CUEOVER
	BR GCUSET
GCU1:	CI STMISS
	BNZ GCU1A
	LI CUEMISS
	BR GCUSET
GCU1A:	CI STHIT
	BNZ GCU1B
	LI CUEHIT
	BR GCUSET
GCU1B:	CI STSUNK
	BNZ GCU2
	LI CUESUNK
	BR GCUSET

GCU2:	DCI VCLASS		; your turn just began?
	LM
	CI CLPLAY
	BNZ GCUX
	DCI VACT
	LM
	OI 0
	BNZ GCUX
	DCI VPRVACT
	LM
	OI 0
	BZ GCUX			; it was already your turn last poll
	LI CUETURN
GCUSET:	DCI VCUEQ
	ST
GCUX:	POP

; ---------------------------------------------------------------------------
; GPLAY -- play the queued cue, now that the board it announces is on screen.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
GPLAY:	ENTER
	DCI VCUEQ
	LM
	CI CUETURN
	BNZ GPL1
	PI SNDTRN
	BR GPLX
GPL1:	CI CUEMISS
	BNZ GPL2
	PI SNDMISS
	BR GPLX
GPL2:	CI CUEHIT
	BNZ GPL3
	PI SNDHIT
	BR GPLX
GPL3:	CI CUESUNK
	BNZ GPL4
	PI SNDSUNK
	BR GPLX
GPL4:	CI CUEOVER
	BNZ GPLX
	PI SNDEND
GPLX:	LEAVE

; ---------------------------------------------------------------------------
; GADV -- carry this poll's numbers into the previous-poll shadows. This runs
;   at the bottom of EVERY successful poll, which is what invariant 1 means.
;   Leaf. Clobbers A, DC0.
; ---------------------------------------------------------------------------
GADV:	DCI VST
	LM
	DCI VPRVST
	ST
	DCI VACT
	LM
	DCI VPRVACT
	ST
	DCI VPC
	LM
	DCI VPRVPC
	ST
	DCI VCLASS
	LM
	DCI VPRVCLS
	ST
	POP

; ===========================================================================
; The drawing half.
; ===========================================================================

; ---------------------------------------------------------------------------
; GUPCPY -- copy the NUL-terminated string at DC0 to DC1, at most r0
;   characters, folding a-z to A-Z, and terminate.
;
;   The fold is not cosmetic. The server lowercases every string it sends, and
;   this font's lower case is the upper-case glyph squashed into four rows --
;   so left alone a name sits a row short of everything beside it.
;   Leaf. Clobbers A, r1, r2, DC0, DC1.
; ---------------------------------------------------------------------------
GUPCPY:	LR A,0
	LR 1,A
GUP1:	LM			; source first: the XDC dance has a direction
	CI 0
	BZ GUPZ
	LR 2,A
	CI 060H			; 'a'-1, so BP means A is not a lower-case letter
	BP GUP2
	LR A,2
	CI 07AH			; 'z', so BM means A is past one
	BM GUP2
	LR A,2
	AI 0E0H			; -32
	LR 2,A
GUP2:	LR A,2
	XDC
	ST
	XDC
	DS 1
	BNZ GUP1
GUPZ:	XDC
	CLR
	ST
	XDC
	POP

; ---------------------------------------------------------------------------
; GBUILD -- copy the string at DC0 into VLINE, folded, at most r0 characters.
;   Returns with DC0 pointing at VLINE, ready for DSTR: PI does not touch the
;   data counters, so a pointer is the one thing that CAN survive a call here.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
GBUILD:	ENTER
	XDC			; DC1 = source
	DCI VLINE
	XDC			; DC0 = source, DC1 = VLINE
	PI GUPCPY
	DCI VLINE
	LEAVE

; ---------------------------------------------------------------------------
; GPREC -- DC0 = the base of player r0's record in the reply window.
;
;   The stride is not constant. In the lobby a record is 10 bytes at
;   GLPLYRS; during PLACE_SHIPS it is also 10, but at GIPLYRS, because the
;   serializer appends the gamefield only once it exists; in play it is 115.
;   Indexing a place-phase record as if it were 115 bytes reads off the end of
;   the reply into whatever the last one left there.
;   Leaf. Clobbers A, r1, r2, DC0.
; ---------------------------------------------------------------------------
GPREC:	LR A,0
	LR 1,A
	DCI VST
	LM
	CI STLOBBY
	BNZ GPR1
	DCI FNREPLY+GLPLYRS
	LI LBSTRID
	LR 2,A
	BR GPRADD
GPR1:	CI STPLACE
	BNZ GPR2
	DCI FNREPLY+GIPLYRS
	LI PPSTRID
	LR 2,A
	BR GPRADD
GPR2:	DCI FNREPLY+GIPLYRS
	LI PLSTRID
	LR 2,A
GPRADD:	LR A,1
	OI 0
	BZ GPRX
GPRL:	LR A,2
	ADC
	DS 1
	BNZ GPRL
GPRX:	POP

; ---------------------------------------------------------------------------
; GPNAME -- build player VTMP's name into VLINE, folded to upper case, at most
;   r0 characters. Returns DC0 = VLINE, ready for DSTR.
;
;   The character count is parked in r9 rather than RAM because every RAM read
;   needs a DCI, and DC0 is holding the record pointer the whole way through.
;   r9 is the one register nothing on this path writes.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
GPNAME:	ENTER
	LR A,0
	LR 9,A
	DCI VTMP
	LM
	LR 0,A
	IF DEMO
	PI DMONM		; canned names: the reply window is empty, and a
	ELSE			;   roster of blanks tells you nothing about the art
	PI GPREC		; DC0 = the record base; PLNAME is offset 0
	ENDIF
	XDC
	DCI VLINE
	XDC			; DC0 = record, DC1 = VLINE
	LR A,9
	LR 0,A
	PI GUPCPY
	DCI VLINE
	LEAVE

; ---------------------------------------------------------------------------
; GNUM -- draw the byte in r0 at (r1,r2) in colour r3, leading zeros dropped.
;   DEC5 wants the value in r0:r1 and writes most of the file, so the
;   coordinates go to RAM for the duration -- VTMP, VTMP2 and VTMP3 are
;   consecutive, so parking all three is one DCI and three stores.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
GNUM:	ENTER
	DCI VTMP
	LR A,1
	ST
	LR A,2
	ST
	LR A,3
	ST
	CLR
	LR 1,A
	PI DEC5
	DCI VTMP
	LM
	LR 1,A
	LM
	LR 2,A
	LM
	LR 3,A
	DCI VDIGIT
	PI DSTR
	LEAVE

; ---------------------------------------------------------------------------
; GSSRC -- DC0 = the string the status row should show.
;
;   THE PROMPT IS EMPTY FOR THE WHOLE OF PLAY. The server sets it deliberately
;   ("updating prompt based on attack result was getting too busy"), so the
;   client synthesises the line from activePlayer and status instead. Where
;   the server does have something to say -- the lobby, the placement phase,
;   the game-over message -- its own words win.
;   Leaf. Clobbers A, DC0.
; ---------------------------------------------------------------------------
GSSRC:	DCI VCLASS
	LM
	CI CLPLAY
	BZ GSSPL
	DCI FNREPLY+GOPRMPT	; a NUL-padded fixed field: empty means byte 0
	LM
	OI 0
	BZ GSSNOP
	DCI FNREPLY+GOPRMPT
	POP

GSSNOP:	DCI VCLASS
	LM
	CI CLLOBBY
	BNZ GSS1
	DCI SLOBBY
	POP
GSS1:	CI CLPLACE
	BNZ GSS2
	DCI VPLST
	LM
	CI PSPLACE
	BNZ GSS1W
	DCI SPLACE
	POP
GSS1W:	DCI SWAITP
	POP
GSS2:	DCI VACT		; game over, and the server had nothing to say
	LM
	OI 0
	BNZ GSS2L
	DCI SYOUWIN
	POP
GSS2L:	DCI SGOVER
	POP

GSSPL:	DCI VST			; in play: the result of the last shot, or
	LM			;   whose turn it is
	CI STMISS
	BNZ GSP1
	DCI SRMISS
	POP
GSP1:	CI STHIT
	BNZ GSP2
	DCI SRHIT
	POP
GSP2:	CI STSUNK
	BNZ GSP3
	DCI SRSUNK
	POP
GSP3:	DCI VACT
	LM
	OI 0
	BNZ GSP4
	DCI SYTURN
	POP
GSP4:	DCI STHEIRS
	POP

; ---------------------------------------------------------------------------
; GSTAT -- the status row: the line above, plus the move clock right-aligned
;   in the last three cells.
;   Reached by PI; calls two levels down.
; ---------------------------------------------------------------------------
GSTAT:	ENTER
	LI DCOLS
	LR 0,A
	LI DORGX
	LR 1,A
	LI STATY
	LR 2,A
	PI DCLRR

	PI GSSRC		; DC0 = the source; PI leaves the data counters
	LI DCOLS-4		;   alone, so a pointer survives the call
	LR 0,A
	PI GBUILD		; DC0 = VLINE
	LI CVAL2
	LR 3,A
	LI DORGX
	LR 1,A
	LI STATY
	LR 2,A
	PI DSTR

	DCI VCLASS		; the clock only means anything in play
	LM
	CI CLPLAY
	BNZ GSTX
	DCI VMVT
	LM
	OI 0
	BZ GSTX
	LR 0,A
	LI DORGX+20*DCELLW
	LR 1,A
	LI STATY
	LR 2,A
	LI CVAL3
	LR 3,A
	PI GNUM
GSTX:	LEAVE

; ---------------------------------------------------------------------------
; GFOOT -- the footer: a hint on the left, the targeting coordinate where
;   there is one, and MODE=OUT on the right.
;
;   The left field stops at ten cells and the right one starts at x=64,
;   because in the quad layout your own five ship pips sit between them at
;   x=46. Nothing here ever writes a pixel the pips will overwrite.
;   Reached by PI; calls two levels down.
; ---------------------------------------------------------------------------
GFOOT:	ENTER
	LI DCOLS
	LR 0,A
	LI DORGX
	LR 1,A
	DCI VLAY+LRFOOT
	LM
	LR 2,A
	PI DCLRR

	DCI VCLASS
	LM
	CI CLPLAY
	BNZ GFT1
	PI GFCOOR		; "B7", built into VLINE
	BR GFTDRW
GFT1:	CI CLLOBBY
	BNZ GFT2
	DCI VREADY
	LM
	OI 0
	BZ GFT1W
	DCI SREADY
	BR GFTB
GFT1W:	DCI SWAIT
	BR GFTB
GFT2:	CI CLPLACE
	BNZ GFT3
	DCI SPLKEY
	BR GFTB
GFT3:	DCI SGOVER
GFTB:	LI FOOTW
	LR 0,A
	PI GBUILD

GFTDRW:	DCI VLAY+LRFOOT
	LM
	LR 2,A
	LI CVAL3
	LR 3,A
	LI DORGX
	LR 1,A
	DCI VLINE
	PI DSTR

	DCI VLAY+LRFOOT
	LM
	LR 2,A
	LI CVAL1
	LR 3,A
	LI FOOTRX
	LR 1,A
	DCI VCLASS
	LM
	CI CLPLACE
	BNZ GFTR1
	DCI STRNKEY
	BR GFTR2
GFTR1:	DCI SOUTKEY
GFTR2:	PI DSTR
	LEAVE

; ---------------------------------------------------------------------------
; GFCOOR -- build the cursor coordinate into VLINE: a column letter and a row
;   digit, the way a paper game names a square.
;   Leaf. Clobbers A, r0, DC0.
; ---------------------------------------------------------------------------
GFCOOR:	DCI VCURX
	LM
	AI 'A'
	LR 0,A
	DCI VLINE
	LR A,0
	ST
	DCI VCURY
	LM
	AI '0'
	LR 0,A
	DCI VLINE+1
	LR A,0
	ST
	CLR
	ST
	DCI VLINE
	POP

; ---------------------------------------------------------------------------
; GROSTR -- the roster panel, which only the quad layout has: the move clock,
;   then each opponent's name over its own board's ship pips.
;
;   The panel sits INSIDE the palette-1 band, so its text is coloured rather
;   than white -- and that is the point. Red is the player to move, green a
;   player still in it, blue one who is out.
;   Reached by PI; calls three levels down.
; ---------------------------------------------------------------------------
GROSTR:	ENTER
	DCI VLAY+LRNBRD
	LM
	CI 4
	BNZ GROX

	DCI VTMP		; row 0 is your own name; the move clock lives on
	CLR			;   the status row, where there is room for all
	ST			;   three of its digits
	PI GROW0

	DCI VTMP		; rows 1, 3, 5: the three opponents
	LIS 1
	ST
GRO1:	PI GROW
	DCI VTMP
	LM
	INC
	DCI VTMP
	ST
	CI MAXPLR
	BNZ GRO1
GROX:	LEAVE

; GROW0 -- your own name, on panel row 0.
;   Reached by PI; calls two levels down.
GROW0:	ENTER
	DCI VTMP2
	LI PANY
	ST
	LI PANW
	LR 0,A
	LI PANX
	LR 1,A
	LI PANY
	LR 2,A
	PI DCLRR
	PI GRWCOL
	DCI VTMP3
	ST
	LI PANW
	LR 0,A
	PI GPNAME
	DCI VTMP3
	LM
	LR 3,A
	LI PANX
	LR 1,A
	LI PANY
	LR 2,A
	DCI VLINE
	PI DSTR
	LEAVE

; GROW -- one opponent's name on panel row 2*VTMP-1.
;   Reached by PI; calls two levels down.
GROW:	ENTER
	DCI VTMP
	LM
	LR 0,A
	SL 1
	AI 0FFH
	LR 1,A			; panel row = 2i - 1
	CLR
	LR 2,A
	LI DCELLH
	LR 3,A
GRW1:	LR A,2
	AS 3
	LR 2,A
	DS 1
	BNZ GRW1
	LR A,2
	AI PANY
	DCI VTMP2
	ST			; the pixel row, out of harm's way

	LI PANW
	LR 0,A
	LI PANX
	LR 1,A
	DCI VTMP2
	LM
	LR 2,A
	PI DCLRR

	DCI VTMP		; an empty seat leaves the row blank
	LM
	LR 0,A
	DCI VPC
	LM
	COM
	INC
	AS 0
	BC GRWX

	PI GRWCOL		; A = the colour this seat's state calls for
	DCI VTMP3
	ST
	LI PANW
	LR 0,A
	PI GPNAME		; DC0 = VLINE
	DCI VTMP3
	LM
	LR 3,A
	LI PANX
	LR 1,A
	DCI VTMP2
	LM
	LR 2,A
	DCI VLINE
	PI DSTR
GRWX:	LEAVE

; GRWCOL -- A = the colour for seat VTMP: red to move, green still in it,
;   blue out of it. It calls GPREC, so it cannot be a leaf -- a PI of its own
;   would overwrite the PC1 holding its return address.
;   Reached by PI; calls one level down. Clobbers A, r0-r2, DC0.
GRWCOL:	ENTER
	DCI VTMP
	LM
	LR 0,A
	DCI VACT
	LM
	XS 0
	BZ GRWCR
	DCI VTMP
	LM
	LR 0,A
	PI GPREC
	LI PLSTAT
	ADC
	LM
	OI 0
	BZ GRWCG
	LI CVAL1
	LEAVE
GRWCG:	LI CVAL3
	LEAVE
GRWCR:	LI CVAL2
	LEAVE

; ---------------------------------------------------------------------------
; GDRAW -- repaint the screen for this poll's class.
;   Reached by PI; calls four levels down on the play path
;   (GDPLAY -> BDRAW -> BFLUSH -> BPAINT, which tail-jumps into DCELL).
; ---------------------------------------------------------------------------
GDRAW:	ENTER
	PI GSTAT		; the chrome FIRST: GFOOT clears the whole footer
	PI GFOOT		;   row, and your own five ship pips sit on it
	DCI VCLASS
	LM
	CI CLLOBBY
	BNZ GDR1
	PI GDLOB
	BR GDRC
GDR1:	CI CLPLACE
	BNZ GDR2
	DCI VPLST
	LM
	CI PSPLACE
	BZ GDRC			; the placement screen paints itself, and is
	PI GDWAIT		;   about to run
	BR GDRC
GDR2:	PI GDPLAY
GDRC:	LEAVE

; ---------------------------------------------------------------------------
; GDPLAY -- every board on this layout, then its pips, then the roster panel.
;   Reached by PI; calls three levels down.
; ---------------------------------------------------------------------------
GDPLAY:	ENTER
	DCI VBRD
	CLR
	ST
GDP1:	PI BDRAW
	PI BPIPS
	DCI VBRD
	LM
	INC
	DCI VBRD
	ST
	LR 1,A
	DCI VLAY+LRNBRD
	LM
	XS 1
	BNZ GDP1
	PI GROSTR
	LEAVE

; ---------------------------------------------------------------------------
; GDLOB -- the lobby roster: the server's name, then who is sitting down and
;   whether they have readied up.
;   Reached by PI; calls three levels down.
; ---------------------------------------------------------------------------
GDLOB:	ENTER
	PI UICLRL
	LI 21
	LR 0,A
	DCI FNREPLY+GLSRVNM
	PI GBUILD
	CLR
	LR 0,A
	LI CVAL3
	LR 3,A
	DCI VLINE
	XDC
	PI UIROW

	DCI VTMP
	CLR
	ST
GDL1:	DCI VTMP
	LM
	LR 0,A
	DCI VPC
	LM
	COM
	INC
	AS 0
	BC GDLX			; that seat and every one after it is empty
	PI GDLROW
	DCI VTMP
	LM
	INC
	DCI VTMP
	ST
	BR GDL1
GDLX:	LEAVE

; GDLROW -- one lobby seat on list row VTMP+2: the name, then READY or
;   WAITING in the colour that says which.
;   Reached by PI; calls three levels down.
GDLROW:	ENTER
	LI 9
	LR 0,A
	PI GPNAME
	DCI VTMP
	LM
	AI 2
	LR 0,A
	LI CVAL2
	LR 3,A
	DCI VLINE
	XDC
	PI UIROW

	DCI VTMP		; LBSTAT and PLSTAT are both offset 9, which is
	LM			;   the one thing the two record shapes share
	LR 0,A
	PI GPREC
	LI LBSTAT
	ADC
	LM
	CI PSREADY
	BNZ GDLRW
	DCI SREADY
	XDC
	DCI VTMP
	LM
	AI 2
	LR 0,A
	LI CVAL3
	LR 3,A
	PI UIROWR
	LEAVE
GDLRW:	DCI SWAITGO
	XDC
	DCI VTMP
	LM
	AI 2
	LR 0,A
	LI CVAL1
	LR 3,A
	PI UIROWR
	LEAVE

; ---------------------------------------------------------------------------
; UIROWR -- like UIROW, but right-aligned into the last eight cells, for the
;   status word that follows a name. Same flat shape and the same tail jump.
;   Clobbers A, r0, r1, r2, r4-r8, DC0, DC1.
; ---------------------------------------------------------------------------
UIROWR:	LR A,0
	INC
	LR 6,A
	LI DCELLH
	LR 5,A
	CLR
	LR 2,A
UIRR1:	LR A,2
	AS 5
	LR 2,A
	DS 6
	BNZ UIRR1
	LR A,2
	AI DORGY
	LR 2,A
	LI DORGX+15*DCELLW
	LR 1,A
	XDC
	JMP DSTR

; ---------------------------------------------------------------------------
; GDWAIT -- the placement phase, once our own five are in: nothing to do but
;   say so.
;   Reached by PI; calls two levels down.
; ---------------------------------------------------------------------------
GDWAIT:	ENTER
	PI UICLRL
	LI CVAL3
	LR 3,A
	LI DORGX
	LR 1,A
	LI DORGY+3*DCELLH
	LR 2,A
	DCI SPLSENT
	PI DSTR
	LEAVE

	IF DEMO
; DMONM -- DC0 = a canned name for seat r0. Takes the seat in r0 and reads it
;   back here, because PI clobbered the accumulator on the way in -- the first
;   version tested A, got the high byte of its own address, and walked DC0 two
;   and a half kilobytes into the ROM.
;   Leaf. Clobbers A, DC0.
DMONM:	DCI DMONAM
	LR A,0
	OI 0
	BZ DMNX
DMN1:	LIS 9
	ADC
	DS 0
	BNZ DMN1
DMNX:	POP

; Nine bytes each -- the wire's name field width, and what DMONM steps by.
; At eight they were a byte short and every seat after the first showed its
; name with the front chewed off: LYD, G, K.
DMONAM:	DB "you",0,0,0,0,0,0
	DB "clyd",0,0,0,0,0
	DB "meg",0,0,0,0,0,0
	DB "kirk",0,0,0,0,0

; ---------------------------------------------------------------------------
; The DEMO position. DEMO=1 builds a client that never touches the network and
; paints a fixed four-board position instead, which is how the art, the cell
; states and the palette bands get iterated: one `make shot` per change,
; against no server and no seat at a table.
;
; It exercises every cell state the game can show, which is the point -- a
; state with no art, or art in the wrong colour, shows up in the screenshot.
; ---------------------------------------------------------------------------
GDEMO:	DCI VPC
	LIS 4
	ST			; VPC
	LI STHIT
	ST			; VST
	LI PSPLAY
	ST			; VPLST
	CLR
	ST			; VACT -- your turn, so the reticle is up
	LI 41
	ST			; VMVT
	LI 55
	ST			; VLASTP
	DCI VCURX
	LIS 6
	ST
	LIS 3
	ST			; VCURY
	DCI VBLINK
	LIS 8
	ST
	POP

; ---------------------------------------------------------------------------
; DMOFIL -- fill VWANT for board VBRD out of DMOTAB, rotated by seven cells a
;   board so the four quadrants are visibly different positions. An enemy's
;   hulls are not on the wire, so they are mapped away here too -- a demo that
;   showed them would be lying about what the real render can know.
;   Leaf. Clobbers A, r0-r3, DC0, DC1.
; ---------------------------------------------------------------------------
DMOFIL:	DCI VWANT
	XDC			; DC1 = the write cursor
	DCI VBRD
	LM
	LR 0,A
	LR 3,A			; keep the board index for the hull mapping
	DCI DMOTAB
	LR A,0
	OI 0
	BZ DMF1
DMF0:	LIS 7
	ADC
	DS 0
	BNZ DMF0
DMF1:	LI BRDCELL
	LR 1,A
DMF2:	LM
	LR 2,A
	LR A,3
	OI 0
	BZ DMF4			; your own board keeps its hulls
	LR A,2
	CI CHULL
	BNZ DMF3
	LI CSEA
	LR 2,A
	BR DMF4
DMF3:	LR A,2
	CI CHDMG
	BNZ DMF4
	LI CHIT
	LR 2,A
DMF4:	LR A,2
	XDC
	ST
	XDC
	DS 1
	BNZ DMF2
	POP

; DMOTAB -- the mock position DEMO=1 paints, so art and palette work
; needs neither a server nor a seat at a table. 128 bytes: the hundred
; cells plus 28 of wrap, so BOARD*7 can index it without a modulo.
DMOTAB:
	DB CHULL,CHULL,CHULL,CHULL,CHULL,CSEA,CSEA,CMISS,CSEA,CSEA
	DB CSEA,CSEA,CSEA,CSEA,CSEA,CSEA,CSEA,CSEA,CSEA,CSEA
	DB CSEA,CMISS,CSEA,CSEA,CHDMG,CHDMG,CHDMG,CHDMG,CSEA,CSEA
	DB CSEA,CSEA,CSEA,CSEA,CSEA,CSEA,CSEA,CSEA,CMISS,CSEA
	DB CSEA,CSEA,CHULL,CSEA,CSEA,CSEA,CMISS,CSEA,CSEA,CSEA
	DB CSEA,CSEA,CHULL,CSEA,CSEA,CSEA,CSEA,CSEA,CSEA,CSEA
	DB CSEA,CSEA,CHULL,CSEA,CSEA,CSEA,CSEA,CSEA,CSEA,CSEA
	DB CSEA,CMISS,CSEA,CSEA,CSEA,CHIT,CHIT,CSEA,CSEA,CSEA
	DB CSEA,CSEA,CSEA,CSEA,CSEA,CSEA,CSEA,CSEA,CSEA,CSEA
	DB CHULL,CHULL,CSEA,CSEA,CMISS,CSEA,CSEA,CSEA,CSEA,CSEA
	DB CHULL,CHULL,CHULL,CHULL,CHULL,CSEA,CSEA,CMISS,CSEA,CSEA
	DB CSEA,CSEA,CSEA,CSEA,CSEA,CSEA,CSEA,CSEA,CSEA,CSEA
	DB CSEA,CMISS,CSEA,CSEA,CHDMG,CHDMG,CHDMG,CHDMG
	ENDIF
