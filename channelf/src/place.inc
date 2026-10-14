; ---------------------------------------------------------------------------
; place.inc -- laying out your fleet.
;
; A RANDOM LEGAL LAYOUT IS SEEDED AND THEN ADJUSTED. That is the family's
; model and it is the right one: the player never builds from nothing, never
; has to solve the packing problem, and can accept the seed as-is with five
; presses. TIME reshuffles the lot for anyone who would rather re-roll.
;
; Illegal moves are REFUSED, not clamped -- a ship that would run off the board
; or through another one simply does not move, and says so with a tone. A
; clamp would silently put the ship somewhere the player did not ask for.
;
; Overlap testing uses a real 100-byte occupancy map. The Arcadia port cannot:
; it has 84 bytes of RAM in total and has to repaint the board carrying every
; ship but the one being moved and read the screen back as its map. Here the
; array is free, and VOCC is rebuilt from VSHIPS with the moving ship's slot
; excluded, which is the same trick without the screen in the loop.
; ---------------------------------------------------------------------------

PLCSLC	EQU 250			; idle passes before we go back and poll: ~4.5 s.
				;   A byte, so it cannot ask for longer -- and it
				;   should not: the server only advances when a
				;   client calls /state.
PLLX	EQU PLCPANX		; the ship list: a marker cell, then the hull
PLBARX	EQU PLCPANX+6		;   bar, one 3px block per segment
PLLY	EQU 12			; five rows on an 8px pitch: 12..46

; ---------------------------------------------------------------------------
; SPLCSC -- the placement screen. Time-sliced like targeting, for the same
;   reason: the server has its own clock and the client must keep polling.
;   Reached by PI; calls four levels down.
; ---------------------------------------------------------------------------
SPLCSC:	ENTER
	DCI VSEEDED
	LM
	OI 0
	BNZ SPL1
	PI PLSEED
	PI PLLOAD
	DCI VSEEDED
	LIS 1
	ST

SPL1:	PI PLDRAW
	DCI VCLOCK
	LI PLCSLC
	ST
SPL2:	PI INSCAN
	LR A,0
	CI EV_NONE
	BNZ SPLK
	LI IDLEMS
	LR 3,A
	PI DELAYMS
	DCI VCLOCK
	LM
	AI 0FFH
	DCI VCLOCK
	ST
	BNZ SPL2
	LEAVE

; ---------------------------------------------------------------------------
SPLK:	DCI VCLOCK
	LI PLCSLC
	ST
	LR A,0
	CI EV_UP
	BNZ SPK1
	PI PLSAVE
	DCI VSY
	LM
	OI 0
	BZ SPLBAD
	AI 0FFH
	DCI VSY
	ST
	JMP SPLTRY
SPK1:	CI EV_DOWN
	BNZ SPK2
	PI PLSAVE
	DCI VSY
	LM
	INC
	DCI VSY
	ST
	JMP SPLTRY
SPK2:	CI EV_LEFT
	BNZ SPK3
	PI PLSAVE
	DCI VSX
	LM
	OI 0
	BZ SPLBAD
	AI 0FFH
	DCI VSX
	ST
	JMP SPLTRY
SPK3:	CI EV_RIGHT
	BNZ SPK4
	PI PLSAVE
	DCI VSX
	LM
	INC
	DCI VSX
	ST
	JMP SPLTRY
SPK4:	CI EV_PGDN		; HOLD rotates the ship being placed
	BNZ SPK5
	PI PLSAVE
	DCI VSDIR
	LM
	XI 1
	DCI VSDIR
	ST
	JMP SPLTRY
SPK5:	CI EV_PGUP		; TIME reshuffles the whole fleet
	BNZ SPK6
	PI PLSEED
	PI PLLOAD
	JMP SPL1
SPK6:	CI EV_SEL
	BZ SPLSEL
	LEAVE			; MODE and anything else belong to the poll loop

SPLBAD:	PI SNDERR
	JMP SPL2

SPLTRY:	PI PLOCCB		; the map WITHOUT the ship being moved
	PI PLFITS
	OI 0
	BNZ SPLREV
	JMP SPL1
SPLREV:	PI PLREST
	JMP SPLBAD

; ---------------------------------------------------------------------------
SPLSEL:	PI PLOCCB
	PI PLFITS
	OI 0
	BNZ SPLBAD
	PI PLENC
	LR 0,A
	DCI VSHIP
	LM
	LR 1,A
	DCI VSHIPS
	LR A,1
	ADC
	LR A,0
	ST
	PI SNDKEY
	DCI VSHIP
	LM
	INC
	DCI VSHIP
	ST
	CI NSHIPS
	BZ SPLDON
	PI PLLOAD
	JMP SPL1
SPLDON:	LI RQPLACE		; the reply to this is the next state
	LR 0,A
	DCI VPEND
	ST
	PI SNDSEL
	LEAVE

; ---------------------------------------------------------------------------
; PLDEC -- decode the placement byte in r0 into VSDIR, VSX, VSY, which are
;   three consecutive cells so the write is one DCI.
;   Takes the byte in r0 rather than A, because PI clobbered A on the way in.
;   Leaf. Clobbers A, r0, r3, r4, r5, DC0.
; ---------------------------------------------------------------------------
PLDEC:	LR A,0
	LR 5,A
	CLR
	LR 4,A
	LR A,5
	AI 156			; -100: a vertical placement
	BNC PLD1
	LR 5,A
	LIS 1
	LR 4,A
PLD1:	CLR
	LR 3,A
	LR A,5
PLD2:	AI 246			; -10
	BNC PLD3
	LR 5,A
	LR A,3
	INC
	LR 3,A
	LR A,5
	BR PLD2
PLD3:	DCI VSDIR
	LR A,4
	ST
	LR A,5
	ST
	LR A,3
	ST
	POP

; ---------------------------------------------------------------------------
; PLENC -- A = VSX + 10*VSY + 100*VSDIR, the wire's own encoding.
;   Leaf. Clobbers A, r1, r2, DC0.
; ---------------------------------------------------------------------------
PLENC:	DCI VSY
	LM
	LR 1,A
	SL 1
	SL 1
	SL 1
	AS 1
	AS 1
	LR 2,A
	DCI VSX
	LM
	AS 2
	LR 2,A
	DCI VSDIR
	LM
	OI 0
	BZ PLE1
	LR A,2
	AI 100
	LR 2,A
PLE1:	LR A,2
	POP

; ---------------------------------------------------------------------------
; PLSAVE / PLREST -- shelter the candidate placement across a rejected move.
;
;   Into VSAVD/X/Y, and deliberately NOT into the shared VTMP cells: PLOCCB
;   walks its ship list in VTMP2, and every move does PLSAVE then PLOCCB then
;   PLFITS. Sharing the scratch put VSX into PLOCCB's loop counter, which then
;   ran past its own terminator marking ships that do not exist -- and PLWALK
;   walked a length read out of a random byte, across the cell shadow.
;   Leaf. Clobbers A, r0, r1, r2, DC0.
; ---------------------------------------------------------------------------
PLSAVE:	DCI VSDIR
	LM
	LR 0,A
	LM
	LR 1,A
	LM
	LR 2,A
	DCI VSAVD
	LR A,0
	ST
	LR A,1
	ST
	LR A,2
	ST
	POP

PLREST:	DCI VSAVD
	LM
	LR 0,A
	LM
	LR 1,A
	LM
	LR 2,A
	DCI VSDIR
	LR A,0
	ST
	LR A,1
	ST
	LR A,2
	ST
	POP

; ---------------------------------------------------------------------------
; PLLOAD -- bring ship VSHIP's committed placement and length into the working
;   cells. A slot still holding NOPLC starts at the top-left.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
PLLOAD:	ENTER
	DCI VSHIP
	LM
	LR 0,A
	DCI SHPSIZ
	LR A,0
	ADC
	LM
	DCI VSLEN
	ST
	DCI VSHIP
	LM
	LR 0,A
	DCI VSHIPS
	LR A,0
	ADC
	LM
	CI NOPLC
	BZ PLLD0
	LR 0,A
	PI PLDEC
	LEAVE
PLLD0:	DCI VSDIR
	CLR
	ST
	ST
	ST
	LEAVE

; ---------------------------------------------------------------------------
; PLWALK -- write 1 into VOCC for VSLEN cells from (VSX,VSY) along VSDIR.
;   Leaf. Clobbers A, r0-r4, DC0.
; ---------------------------------------------------------------------------
PLWALK:	DCI VSDIR
	LM
	LR 4,A
	LM
	LR 2,A
	LM
	LR 3,A
	DCI VSLEN
	LM
	LR 1,A
PLW1:	LR A,3
	LR 0,A
	SL 1
	SL 1
	SL 1
	AS 0
	AS 0
	AS 2
	LR 0,A			; 10y + x
	DCI VOCC
	LR A,0
	ADC
	LIS 1
	ST
	LR A,4
	OI 0
	BZ PLW2
	LR A,3
	INC
	LR 3,A
	BR PLW3
PLW2:	LR A,2
	INC
	LR 2,A
PLW3:	DS 1
	BNZ PLW1
	POP

; PLWOVR -- the same walk, but stamping CREVL into VWANT: the outline that
;   marks the ship currently under the player's hand, against the solid hulls
;   of the ones already set.
;   Leaf. Clobbers A, r0-r4, DC0.
PLWOVR:	DCI VSDIR
	LM
	LR 4,A
	LM
	LR 2,A
	LM
	LR 3,A
	DCI VSLEN
	LM
	LR 1,A
PLO1:	LR A,3
	LR 0,A
	SL 1
	SL 1
	SL 1
	AS 0
	AS 0
	AS 2
	LR 0,A
	DCI VWANT
	LR A,0
	ADC
	LI CREVL
	ST
	LR A,4
	OI 0
	BZ PLO2
	LR A,3
	INC
	LR 3,A
	BR PLO3
PLO2:	LR A,2
	INC
	LR 2,A
PLO3:	DS 1
	BNZ PLO1
	POP

; ---------------------------------------------------------------------------
; PLFITS -- A = 0 if the working placement is on the board and clear of VOCC.
;   Leaf. Clobbers A, r0-r4, DC0.
; ---------------------------------------------------------------------------
PLFITS:	DCI VSDIR
	LM
	LR 4,A
	LM
	LR 2,A
	LM
	LR 3,A
	DCI VSLEN
	LM
	LR 1,A
	LR A,4
	OI 0
	BZ PLF0
	LR A,3			; vertical: the bound is on y
	BR PLF0A
PLF0:	LR A,2
PLF0A:	AS 1
	CI BRDDIM
	BM PLFBAD		; x + len past the edge
PLF1:	LR A,3
	LR 0,A
	SL 1
	SL 1
	SL 1
	AS 0
	AS 0
	AS 2
	LR 0,A
	DCI VOCC
	LR A,0
	ADC
	LM
	OI 0
	BNZ PLFBAD
	LR A,4
	OI 0
	BZ PLF2
	LR A,3
	INC
	LR 3,A
	BR PLF3
PLF2:	LR A,2
	INC
	LR 2,A
PLF3:	DS 1
	BNZ PLF1
	CLR
	POP
PLFBAD:	LIS 1
	POP

; ---------------------------------------------------------------------------
; PLOCCB -- rebuild VOCC from VSHIPS, EXCLUDING the slot in VSHIP. Excluding
;   the moving ship is what lets a candidate overlap the cells that ship
;   already occupies without reporting a collision with itself.
;   Reached by PI; calls two levels down.
; ---------------------------------------------------------------------------
PLOCCB:	ENTER
	DCI VOCC
	LI BRDCELL
	LR 1,A
	CLR
	LR 0,A
POC0:	LR A,0
	ST
	DS 1
	BNZ POC0
	DCI VTMP2
	CLR
	ST
POC1:	DCI VTMP2
	LM
	LR 0,A
	DCI VSHIP
	LM
	XS 0
	BZ POCN
	DCI VTMP2
	LM
	LR 0,A
	DCI VSHIPS
	LR A,0
	ADC
	LM
	CI NOPLC
	BZ POCN
	PI PLOMRK
POCN:	DCI VTMP2
	LM
	INC
	DCI VTMP2
	ST
	CI NSHIPS
	BNZ POC1
	LEAVE

; PLOMRK -- mark ship VTMP2's cells in VOCC. It borrows the working cells and
;   puts them back, because PLWALK reads them and the caller is mid-move.
;   Reached by PI; calls one level down.
PLOMRK:	ENTER
	PI PLSAVE
	DCI VTMP2
	LM
	LR 0,A
	DCI SHPSIZ
	LR A,0
	ADC
	LM
	DCI VSLEN
	ST
	DCI VTMP2
	LM
	LR 0,A
	DCI VSHIPS
	LR A,0
	ADC
	LM
	LR 0,A
	PI PLDEC
	PI PLWALK
	PI PLREST
	DCI VSHIP		; PLSAVE/PLREST used VTMP..VTMP3; VSLEN belongs to
	LM			;   the ship being moved, so restore that too
	LR 0,A
	DCI SHPSIZ
	LR A,0
	ADC
	LM
	DCI VSLEN
	ST
	LEAVE

; ---------------------------------------------------------------------------
; PLSEED -- a random legal fleet. Every slot is cleared first, then each ship
;   is rolled until it fits against the ones already down.
;   Reached by PI; calls three levels down.
; ---------------------------------------------------------------------------
PLSEED:	ENTER
	DCI VSHIPS
	LI NOPLC
	LR 0,A
	LI NSHIPS
	LR 1,A
PSD0:	LR A,0
	ST
	DS 1
	BNZ PSD0
	DCI VSHIP
	CLR
	ST
PSD1:	DCI VSHIP
	LM
	LR 0,A
	DCI SHPSIZ
	LR A,0
	ADC
	LM
	DCI VSLEN
	ST
	PI PLOCCB
PSD2:	DCI VTMP
	LI BRDDIM
	ST
	PI RNDMOD
	DCI VSX
	ST
	DCI VTMP
	LI BRDDIM
	ST
	PI RNDMOD
	DCI VSY
	ST
	DCI VTMP
	LIS 2
	ST
	PI RNDMOD
	DCI VSDIR
	ST
	PI PLFITS
	OI 0
	BNZ PSD2
	PI PLENC
	LR 0,A
	DCI VSHIP
	LM
	LR 1,A
	DCI VSHIPS
	LR A,1
	ADC
	LR A,0
	ST
	DCI VSHIP
	LM
	INC
	DCI VSHIP
	ST
	CI NSHIPS
	BNZ PSD1
	DCI VSHIP
	CLR
	ST
	LEAVE

; ---------------------------------------------------------------------------
; PLWANT -- VWANT from VOCC, then the ship under the player's hand on top.
;   Leaf. Clobbers A, r0-r4, DC0, DC1.
; ---------------------------------------------------------------------------
PLWANT:	DCI VWANT
	XDC
	DCI VOCC
	LI BRDCELL
	LR 1,A
PLWA1:	LM			; source first
	OI 0
	BZ PLWA2
	LI CHULL
	BR PLWA3
PLWA2:	CLR			; CSEA
PLWA3:	XDC
	ST
	XDC
	DS 1
	BNZ PLWA1
	JMP PLWOVR		; tail call: its POP returns to OUR caller

; ---------------------------------------------------------------------------
; PLDRAW -- the placement board and the ship list beside it.
;   Reached by PI; calls three levels down.
; ---------------------------------------------------------------------------
PLDRAW:	ENTER
	PI PLOCCB
	PI PLWANT
	DCI VBRD
	CLR
	ST
	PI BFLUSH
	PI PLLIST
	LEAVE

; ---------------------------------------------------------------------------
; PLLIST -- the fleet down the right-hand side: one bar per ship, as long as
;   the ship is. Green is set, red is the one in your hand, blue is still to
;   come -- so the list doubles as the progress indicator.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
PLLIST:	ENTER
	DCI VTMP
	CLR
	ST
PLL1:	DCI VTMP
	LM
	LR 0,A
	SL 1
	SL 1
	SL 1
	AI PLLY
	DCI VTMP2
	ST			; the bar's pixel row

	LI 96-PLLX		; blank the whole strip first: a bar that just
	LR 0,A			;   got shorter has to stop being drawn
	LI PLLX
	LR 1,A
	DCI VTMP2
	LM
	LR 2,A
	LIS 3
	LR 9,A
	LI CVAL0
	LR 3,A
	PI DFILL

	PI PLLCOL
	DCI VTMP3
	ST
	DCI VTMP
	LM
	LR 0,A
	DCI SHPSIZ
	LR A,0
	ADC
	LM
	SL 1
	SL 1
	AI 0FFH
	LR 0,A			; len*4 - 1 pixels: blocks with a gap between
	LI PLBARX
	LR 1,A
	DCI VTMP2
	LM
	LR 2,A
	LIS 3
	LR 9,A
	DCI VTMP3
	LM
	LR 3,A
	PI DFILL

	DCI VTMP
	LM
	INC
	DCI VTMP
	ST
	CI NSHIPS
	BNZ PLL1
	LEAVE

; PLLCOL -- A = the colour ship VTMP's bar should be.
;   Leaf. Clobbers A, r0, r1, DC0.
PLLCOL:	DCI VTMP
	LM
	LR 0,A
	DCI VSHIP
	LM
	XS 0
	BZ PLLCC		; the one in your hand
	DCI VTMP
	LM
	LR 0,A
	DCI VSHIPS
	LR A,0
	ADC
	LM
	CI NOPLC
	BZ PLLCP
	LI CVAL3		; set
	POP
PLLCP:	LI CVAL1		; still to come
	POP
PLLCC:	LI CVAL2
	POP
