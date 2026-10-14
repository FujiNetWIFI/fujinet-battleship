; ---------------------------------------------------------------------------
; target.inc -- your turn: move the reticle, then fire.
;
; TIME-SLICED, NOT BLOCKING. The server enforces its own move limit and skips
; a turn that runs out; a client that blocks here forever never polls again
; and never learns the game moved on. So this counts down locally and returns,
; and any keypress restarts the slice -- a player who is still thinking is not
; interrupted, and one who has walked away is.
;
; ONE CURSOR, IN LOCKSTEP. /attack carries a position and the server applies it
; to every live opponent at once, so the reticle paints on every board that
; shot will reach, and moving it repaints all of them.
;
; And this is where the shadow pays for itself: a move recomputes all 100 cells
; of each enemy board into VWANT, but only the two that actually changed reach
; the screen. A full opaque repaint of three boards would be most of a second.
; ---------------------------------------------------------------------------

TGTSLC	EQU 220			; idle passes of about 18 ms: roughly four
				;   seconds before we go back and poll
TGBLINK	EQU 7			; redraw on every eighth pass, so bit 3 of
				;   VBLINK flips at about 3 Hz

; ---------------------------------------------------------------------------
; STGT -- run one targeting slice. Reached by PI; calls four levels down.
; ---------------------------------------------------------------------------
STGT:	ENTER
	DCI VCLOCK
	LI TGTSLC
	ST
	PI TGDRAW		; light the reticle before the first wait

TG1:	PI INSCAN
	LR A,0
	CI EV_NONE
	BNZ TGKEY

	DCI VBLINK
	LM
	INC
	DCI VBLINK
	ST
	NI TGBLINK
	BNZ TGNB
	PI TGDRAW		; the phase in bit 3 just changed
TGNB:	LI IDLEMS
	LR 3,A
	PI DELAYMS
	DCI VCLOCK
	LM
	AI 0FFH
	DCI VCLOCK
	ST
	BNZ TG1
	LEAVE			; the slice ran out: go back and poll

; ---------------------------------------------------------------------------
TGKEY:	DCI VCLOCK		; any key restarts the slice
	LI TGTSLC
	ST
	LR A,0
	CI EV_UP
	BNZ TGK1
	DCI VCURY
	LM
	OI 0
	BZ TG1
	AI 0FFH
	DCI VCURY
	ST
	BR TGMOVE
TGK1:	CI EV_DOWN
	BNZ TGK2
	DCI VCURY
	LM
	CI BRDDIM-1
	BZ TG1
	INC
	DCI VCURY
	ST
	BR TGMOVE
TGK2:	CI EV_LEFT
	BNZ TGK3
	DCI VCURX
	LM
	OI 0
	BZ TG1
	AI 0FFH
	DCI VCURX
	ST
	BR TGMOVE
TGK3:	CI EV_RIGHT
	BNZ TGK4
	DCI VCURX
	LM
	CI BRDDIM-1
	BZ TG1
	INC
	DCI VCURX
	ST
	BR TGMOVE
TGK4:	CI EV_SEL
	BZ TGFIRE
	LEAVE			; anything else belongs to the poll loop

TGMOVE:	DCI VBLINK		; hold the reticle LIT while it is moving, or it
	LI 8			;   appears to skip around
	ST
	PI TGDRAW
	PI GFOOT		; the coordinate readout follows the reticle
	JMP TG1			; out of BR's +/-127 reach

; ---------------------------------------------------------------------------
; TGFIRE -- stage an attack at the reticle, if it can land anywhere.
;
;   The server refuses a position already resolved on every live opponent and
;   simply does not advance, which looks to the player like the fire button
;   doing nothing at all -- over and over. So it is checked here and refused
;   with a tone instead.
; ---------------------------------------------------------------------------
TGFIRE:	PI TGLEGL
	OI 0
	BNZ TGBAD
	PI TGANIM
	LI RQATTCK
	LR 0,A
	DCI VPEND
	ST
	LEAVE			; the reply to this IS the next state
TGBAD:	PI SNDERR
	JMP TG1

; ---------------------------------------------------------------------------
; TGLEGL -- A = 0 if the reticle sits on an unshot cell of at least one
;   opponent who is still in the game.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
TGLEGL:	ENTER
	DCI VCURY
	LM
	LR 1,A
	SL 1
	SL 1
	SL 1
	AS 1
	AS 1
	LR 2,A
	DCI VCURX
	LM
	AS 2
	DCI VTMP3
	ST			; 10y + x, out of the way of the draw calls
	DCI VTMP
	LIS 1
	ST
TGL1:	DCI VTMP
	LM
	LR 0,A
	DCI VPC
	LM
	COM
	INC
	AS 0
	BC TGLNO		; ran out of seats without finding one

	DCI VTMP
	LM
	LR 0,A
	PI GPREC
	LI PLSTAT
	ADC
	LM
	OI 0
	BNZ TGLNX		; not PSPLAY: this board takes no shot

	DCI VTMP3
	LM
	LR 0,A
	PI TGPEEK		; TGPEEK rebuilds the record base itself, because
				;   reading the offset needs a DCI of its own
	OI 0
	BZ TGLOK		; unshot: the shot can land here

TGLNX:	DCI VTMP
	LM
	INC
	DCI VTMP
	ST
	BR TGL1
TGLOK:	CLR
	LEAVE
TGLNO:	LIS 1
	LEAVE

; TGPEEK -- A = the byte r0 past the record base. The base has to be rebuilt,
;   because reading r0 needed a DCI of its own -- so this takes the offset in
;   r0 and the seat in VTMP and does both halves itself.
;   Reached by PI; calls one level down.
TGPEEK:	ENTER
	LR A,0
	LR 9,A			; the offset, parked where no DCI can reach
	DCI VTMP
	LM
	LR 0,A
	PI GPREC
	LI PLFIELD
	ADC
	LR A,9
	ADC
	LM
	LEAVE

; ---------------------------------------------------------------------------
; TGDRAW -- repaint every enemy board, which is what moves the reticle.
;   Board 0 is yours and never carries one.
;   Reached by PI; calls four levels down.
; ---------------------------------------------------------------------------
TGDRAW:	ENTER
	DCI VBRD
	LIS 1
	ST
TGD1:	DCI VBRD
	LM
	LR 1,A
	DCI VLAY+LRNBRD
	LM
	XS 1
	BZ TGDX
	PI BDRAW
	DCI VBRD
	LM
	INC
	DCI VBRD
	ST
	BR TGD1
TGDX:	LEAVE

; ---------------------------------------------------------------------------
; TGANIM -- the shot going out: the target cell collapses to a splash on every
;   live enemy board, then the poll's own redraw replaces it with whatever the
;   server says happened.
;
;   It costs one cell per board per frame -- about a millisecond -- because the
;   shadow means only that cell is written. An opaque repaint could not afford
;   an animation at all.
;   Reached by PI; calls three levels down.
; ---------------------------------------------------------------------------
TGANIM:	ENTER
	DCI VTMP2
	LIS 3
	ST			; three frames
TGA1:	DCI VBRD
	LIS 1
	ST
TGA2:	DCI VBRD
	LM
	LR 1,A
	DCI VLAY+LRNBRD
	LM
	XS 1
	BZ TGA3
	PI TGASPL
	DCI VBRD
	LM
	INC
	DCI VBRD
	ST
	BR TGA2
TGA3:	LI 6
	LR 3,A
	PI DELAYMS
	DCI VTMP2
	LM
	AI 0FFH
	DCI VTMP2
	ST
	BNZ TGA1
	PI SNDSEL
	LEAVE

; TGASPL -- paint the splash frame on board VBRD's target cell and mark the
;   shadow so the next poll's flush repaints it from the wire.
;   Reached by PI; calls one level down (BPAINT tail-jumps into DCELL).
TGASPL:	ENTER
	DCI VLAY+LRCELW
	LM
	LR 9,A
	LM
	LR 10,A
	DCI VCURX
	LM
	DCI VBCX
	ST
	DCI VCURY
	LM
	DCI VBCY
	ST
	DCI VBST
	LI CSPL
	ST
	PI BPAINT
	DCI VCURY		; invalidate this cell's shadow, or the next
	LM			;   flush sees no change and leaves the splash
	LR 1,A			;   on screen for the rest of the game
	SL 1
	SL 1
	SL 1
	AS 1
	AS 1
	LR 2,A
	DCI VCURX
	LM
	AS 2
	DCI VTMP
	ST
	PI BSHADR
	LI 0FFH
	ST
	LEAVE
