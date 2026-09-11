; ---------------------------------------------------------------------------
; board.inc -- the boards, and the reason they are affordable.
;
; VRAM cannot be read back and a plotted pixel costs about 80us, so a 200-cell
; opaque repaint is the best part of a fifth of a second and would make the
; reticle feel like treacle. Instead every cell's last PAINTED state is kept in
; a 400-byte shadow in cart RAM, a poll computes what each cell WANTS to be
; into a 100-byte scratch, and only the differences are drawn. A typical poll
; dirties one to three cells; moving the reticle dirties exactly two. No
; sibling console can afford the shadow -- the Arcadia client has 84 bytes of
; RAM in total and repaints both boards every poll because it must.
;
; Cell art is 2bpp and OPAQUE (see tools/mkart.py), so a repaint writes each of
; the cell's pixels exactly once: no erase pass, nothing written twice.
;
; Computing the wanted state rather than overdrawing is what makes the shadow
; honest. The Arcadia port paints sea, then hulls, then overlays damage, and
; the screen ends up right; here the same three passes run over VWANT and the
; screen only ever sees the answer.
; ---------------------------------------------------------------------------

SHPSIZ:	DB 5,4,3,3,2		; carrier, battleship, cruiser, sub, destroyer

; --- layout ids -----------------------------------------------------------
LAY_QD	EQU 0
LAY_HU	EQU 1
LAY_PLC	EQU 2
LAY_TXT	EQU 3

; ---------------------------------------------------------------------------
; LAYLD -- make layout r0 the current one. A no-op if it already is; otherwise
;   the screen is cleared, the palette bands are laid down to the new record's
;   extent, the shadow is invalidated and the divider cross is drawn.
;   Reached by PI; calls two levels down (CLSALL -> PALBND -> DPALR).
; ---------------------------------------------------------------------------
LAYLD:	ENTER
	DCI VPRVLAY
	LM
	LR 1,A
	LR A,0
	XS 1
	BZ LAYX
	DCI VPRVLAY
	LR A,0
	ST

	LR A,0
	OI 0
	BNZ LAY1
	DCI LAYQD
	BR LAYCP
LAY1:	CI LAY_HU
	BNZ LAY2
	DCI LAYHU
	BR LAYCP
LAY2:	CI LAY_PLC
	BNZ LAY3
	DCI LAYPLC
	BR LAYCP
LAY3:	DCI LAYTXT

LAYCP:	XDC			; DC1 = the record in ROM
	DCI VLAY
	XDC			; DC0 = record, DC1 = VLAY
	LI LRECSZ
	LR 1,A
LAYC1:	LM			; source first -- the XDC dance has a direction,
	XDC			;   and doing the swap before the LM copies the
	ST			;   destination over the source, silently
	XDC
	DS 1
	BNZ LAYC1

	PI CLSALL
	PI SHDCLR
	PI DIVDRW
LAYX:	LEAVE

; ---------------------------------------------------------------------------
; SHDCLR -- invalidate the shadow. $FF is not a state any cell can want, so the
;   next flush repaints every one of them.
;   Leaf. Clobbers A, r0, r1, DC0.
; ---------------------------------------------------------------------------
SHDCLR:	DCI VSHDW
	LI 0FFH
	LR 0,A
	LI MAXPLR*BRDCELL/2
	LR 1,A
SHD1:	LR A,0
	ST
	ST
	DS 1
	BNZ SHD1
	POP

; ---------------------------------------------------------------------------
; DIVDRW -- the board chrome: the frame around the whole group and the divider
;   cross that separates the quadrants inside it. A zero width means the layout
;   has no bar of that axis.
;
;   This is what replaced the per-cell lattice. Drawn once when the layout
;   loads, it costs nothing per poll, where a dot in every cell cost a sixth of
;   every 3x2 cell forever and turned the sea into a stipple.
;   Reached by PI; calls one level down (DFILL).
; ---------------------------------------------------------------------------
DIVDRW:	ENTER
	PI FRAME
	DCI VLAY+LRVX
	LM
	LR 1,A
	LM
	LR 2,A
	LM
	LR 0,A
	LM
	LR 9,A
	LR A,0
	OI 0
	BZ DIV2
	LI CVAL1
	LR 3,A
	PI DFILL
DIV2:	DCI VLAY+LRHX
	LM
	LR 1,A
	LM
	LR 2,A
	LM
	LR 0,A
	LM
	LR 9,A
	LR A,0
	OI 0
	BZ DIVX
	LI CVAL1
	LR 3,A
	PI DFILL
DIVX:	LEAVE

; ---------------------------------------------------------------------------
; FRAME -- a one-pixel outline around the board group, from the record.
;   Four DFILLs, and neither count may be zero in any of them.
;   Reached by PI; calls one level down (DFILL).
; ---------------------------------------------------------------------------
FRAME:	ENTER
	DCI VLAY+LRFW
	LM
	OI 0
	BZ FRMX
	LI CVAL1
	LR 3,A

	DCI VLAY+LRFX		; top
	LM
	LR 1,A
	LM
	LR 2,A
	LM
	LR 0,A
	LIS 1
	LR 9,A
	PI DFILL

	DCI VLAY+LRFX		; bottom
	LM
	LR 1,A
	LM
	LR 2,A
	LM
	LR 0,A
	LM
	AS 2
	AI 0FFH
	LR 2,A			; y + h - 1
	LIS 1
	LR 9,A
	PI DFILL

	DCI VLAY+LRFX		; left
	LM
	LR 1,A
	LM
	LR 2,A
	LM
	LM
	LR 9,A			; height
	LIS 1
	LR 0,A
	PI DFILL

	DCI VLAY+LRFX		; right
	LM
	LR 1,A
	LM
	LR 2,A
	LM
	AS 1
	AI 0FFH
	LR 1,A			; x + w - 1
	DCI VLAY+LRFH
	LM
	LR 9,A
	LIS 1
	LR 0,A
	PI DFILL
FRMX:	LEAVE

; ---------------------------------------------------------------------------
; BSHADR -- DC0 = the shadow byte for cell VTMP (0-99) of board VBRD.
;   The board stride is 100, which does not fit a signed ADC, so it is added
;   in hundreds -- at most three times.
;   Leaf. Clobbers A, r0, r1, DC0.
; ---------------------------------------------------------------------------
BSHADR:	DCI VTMP
	LM
	LR 1,A
	DCI VBRD
	LM
	LR 0,A
	DCI VSHDW
	LR A,0
	OI 0
	BZ BSH2
BSH1:	LI BRDCELL
	ADC
	DS 0
	BNZ BSH1
BSH2:	LR A,1
	ADC
	POP

; ---------------------------------------------------------------------------
; BPAINT -- paint cell (VBCX,VBCY) of board VBRD as state VBST, unconditionally.
;   r9 = cell width and r10 = cell height must already hold the record's, which
;   BFLUSH loads once for the whole board.
;   Reached by PI and calls nothing, so its return address stays in PC1 and
;   the closing JMP is a tail call: DCELL's POP returns to OUR caller. That is
;   one K-stack level saved on the deepest chain in the program.
; ---------------------------------------------------------------------------
BPAINT:	DCI VBRD
	LM
	LR 0,A
	DCI VLAY+LROX
	LR A,0
	ADC
	LM
	LR 1,A			; origin x
	DCI VBCX
	LM
	LR 2,A
	OI 0
	BZ BP1
BP0:	LR A,1
	AS 9
	LR 1,A
	DS 2
	BNZ BP0

BP1:	DCI VBRD
	LM
	LR 0,A
	DCI VLAY+LROY
	LR A,0
	ADC
	LM
	LR 2,A			; origin y
	DCI VBCY
	LM
	LR 0,A
	OI 0
	BZ BP3
BP2:	LR A,2
	AS 10
	LR 2,A
	DS 0
	BNZ BP2

BP3:	DCI VBST		; art offset = state * cell height
	LM
	LR 0,A
	CLR
	LR 3,A
	LR A,0
	OI 0
	BZ BP5
BP4:	LR A,3
	AS 10
	LR 3,A
	DS 0
	BNZ BP4
BP5:	LR A,10
	CI 2
	BNZ BP6
	DCI ARTQ
	BR BP7
BP6:	DCI ARTH
BP7:	LR A,3
	ADC
	JMP DCELL

; ---------------------------------------------------------------------------
; BFLUSH -- draw the difference between VWANT and board VBRD's shadow.
;   The wanted state is carried in r11, the one register neither DCELL nor the
;   K stack touches.
;   Reached by PI; calls two levels down (BPAINT -> DCELL).
; ---------------------------------------------------------------------------
BFLUSH:	ENTER
	DCI VLAY+LRCELW
	LM
	LR 9,A
	LM
	LR 10,A
	DCI VTMP
	CLR
	ST
	DCI VBCY
	CLR
	ST
BFY:	DCI VBCX
	CLR
	ST
BFX:	DCI VTMP
	LM
	LR 2,A
	DCI VWANT
	LR A,2
	ADC
	LM
	LR 11,A			; the state this cell wants
	PI BSHADR
	LM
	LR 0,A			; the state it last had
	LR A,11
	XS 0
	BZ BFNEXT
	DCI VBST
	LR A,11
	ST
	PI BSHADR
	LR A,11
	ST
	PI BPAINT
BFNEXT:	DCI VTMP
	LM
	INC
	DCI VTMP
	ST
	DCI VBCX
	LM
	INC
	DCI VBCX
	ST
	CI BRDDIM
	BNZ BFX
	DCI VBCY
	LM
	INC
	DCI VBCY
	ST
	CI BRDDIM
	BNZ BFY
	LEAVE

; ---------------------------------------------------------------------------
; BWFILL -- fill VWANT with the state in r0.
;   Leaf. Clobbers A, r0, r1, DC0.
; ---------------------------------------------------------------------------
BWFILL:	DCI VWANT
	LI BRDCELL
	LR 1,A
BWF1:	LR A,0
	ST
	DS 1
	BNZ BWF1
	POP

; ---------------------------------------------------------------------------
; BWIRE -- fill VWANT from board VBRD's gamefield in the reply window.
;
;   The wire only ever carries 0, 1 (hit) and 2 (miss): FIELD_SUNK is internal
;   to the server's AI and is never serialised, so there is no fourth case to
;   handle and shipsLeft is the only thing that says a ship went down.
;
;   Calls nothing, so the counter can live in r1 -- this is the one loop in the
;   port that does not have to spill it.
;   Leaf. Clobbers A, r0, r1, DC0, DC1.
; ---------------------------------------------------------------------------
BWIRE:	DCI VWANT
	XDC			; DC1 = the write cursor
	DCI VBRD
	LM
	LR 0,A
	DCI FNREPLY+GIPLYRS+PLFIELD
	LR A,0
	OI 0
	BZ BW1
BW0:	LI PLSTRID
	ADC
	DS 0
	BNZ BW0
BW1:	LI BRDCELL
	LR 1,A
BW2:	LM			; the wire byte; source first
	CI CELLHIT
	BNZ BW3
	LI CHIT
	BR BW5
BW3:	CI CELLMIS
	BNZ BW4
	LI CMISS
	BR BW5
BW4:	CLR			; CSEA
BW5:	XDC
	ST
	XDC
	DS 1
	BNZ BW2
	POP

; ---------------------------------------------------------------------------
; BHULLS -- overlay five ship placements onto VWANT.
;   VTMP2 = the myShips base index: 0 for yours, 5 for the winner's, which v=2
;           sends at game over.
;   VTMP3 = what an unshot hull cell becomes (CHULL, or CREVL for the reveal).
;
;   A cell that already reads as a hit becomes CHDMG -- red hull with a green
;   core -- so your own damage never looks like an enemy cell you have shot.
;   A miss is left alone: it cannot be on a hull.
;   Leaf. Clobbers A, r0-r8, DC0.
; ---------------------------------------------------------------------------
BHULLS:	DCI VTMP2
	LM
	LR 6,A
	CLR
	LR 0,A			; ship index
BHS1:	DCI FNREPLY+GIMYSHP
	LR A,6
	ADC
	LR A,0
	ADC
	LM
	LR 5,A			; pos + 100*dir
	CLR
	LR 4,A
	LR A,5
	AI 156			; -100: a vertical ship
	BNC BHS2
	LR 5,A
	LIS 1
	LR 4,A
BHS2:	CLR
	LR 3,A
	LR A,5
BHS3:	AI 246			; -10
	BNC BHS4
	LR 5,A
	LR A,3
	INC
	LR 3,A
	LR A,5
	BR BHS3
BHS4:	LR A,5
	LR 2,A			; x; r3 = y
	DCI SHPSIZ
	LR A,0
	ADC
	LM
	LR 1,A			; length

BHS5:	LR A,3
	LR 7,A
	SL 1
	SL 1
	SL 1
	AS 7
	AS 7
	AS 2
	LR 7,A			; 10y + x
	DCI VWANT
	LR A,7
	ADC
	LM
	CI CHIT
	BNZ BHS6
	LI CHDMG
	BR BHS7
BHS6:	CI CSEA
	BNZ BHS8		; a miss, or already hull: leave it
	DCI VTMP3
	LM
BHS7:	LR 8,A
	DCI VWANT
	LR A,7
	ADC
	LR A,8
	ST
BHS8:	LR A,4
	OI 0
	BZ BHS9
	LR A,3
	INC
	LR 3,A
	BR BHS10
BHS9:	LR A,2
	INC
	LR 2,A
BHS10:	DS 1
	BNZ BHS5
	LR A,0
	INC
	LR 0,A
	CI NSHIPS
	BNZ BHS1
	POP

; ---------------------------------------------------------------------------
; BPIPS -- the five ship pips for board VBRD, from its shipsLeft. A pip strip
;   with a zero y is a board that has none (yours in QUAD lives on the footer
;   row instead, and the placement layout has no pips at all).
;   Reached by PI; calls one level down (DCELL).
; ---------------------------------------------------------------------------
BPIPS:	ENTER
	DCI VBRD
	LM
	LR 0,A
	DCI VPC
	LM
	COM
	INC
	AS 0			; board - count
	BC BPPX			; no borrow: this seat is empty
	DCI VBRD
	LM
	LR 0,A
	DCI VLAY+LRPPX
	LR A,0
	ADC
	LM
	LR 1,A
	DCI VBRD
	LM
	LR 0,A
	DCI VLAY+LRPPY
	LR A,0
	ADC
	LM
	LR 2,A
	OI 0
	BZ BPPX
	LIS 3
	LR 9,A
	LR 10,A
	DCI VTMP
	CLR
	ST

BPP1:	DCI VTMP
	LM
	LR 0,A			; pip index -- read BEFORE the pointer is built,
	DCI VBRD		;   because a DCI would reset DC0 afterwards
	LM
	LR 3,A
	IF DEMO
	AS 0			; a plausible mix of afloat and sunk
	NI 1
	BZ BPPSNK
	DCI PIPOK
	BR BPPDR
	ENDIF
	DCI FNREPLY+GIPLYRS+PLSHIPS
	LR A,0
	ADC
	LR A,3
	OI 0
	BZ BPP2
BPP1A:	LI PLSTRID
	ADC
	DS 3
	BNZ BPP1A
BPP2:	LM
	OI 0
	BZ BPPSNK
	DCI PIPOK
	BR BPPDR
BPPSNK:	DCI PIPSUNK
BPPDR:	PI DCELL
	DCI VLAY+LRPPDX
	LM
	AS 1
	LR 1,A
	DCI VLAY+LRPPDY
	LM
	AS 2
	LR 2,A
	DCI VTMP
	LM
	INC
	DCI VTMP
	ST
	CI NSHIPS
	BNZ BPP1
BPPX:	LEAVE

; ---------------------------------------------------------------------------
; BDRAW -- repaint board VBRD: wire, then your hulls, then the winner's reveal,
;   then the reticle, then the flush.
;
;   Order is load-bearing, as on every sibling: hulls before damage, so an
;   incoming hit sits ON your ship rather than under it.
;   Reached by PI; calls three levels down (BFLUSH -> BPAINT -> DCELL).
; ---------------------------------------------------------------------------
BDRAW:	ENTER
	DCI VBRD		; a quadrant with nobody in it paints as flat
	LM			;   sea with no lattice, which is visibly not a
	LR 0,A			;   board rather than an empty one
	DCI VPC
	LM
	COM
	INC			; -playerCount
	AS 0			; board - count; carry = no borrow = board >= count
	BNC BDLIVE
	LI CDASH
	LR 0,A
	PI BWFILL
	BR BDFLSH

BDLIVE:
	IF DEMO
	PI DMOFIL		; a mock position, and no network at all
	BR BD2
	ELSE
	PI BWIRE
	DCI VBRD
	LM
	OI 0
	BNZ BD1
	DCI VPLST		; a spectator has no ships to draw
	LM
	CI PSVIEW
	BZ BD1
	DCI VTMP2
	CLR
	ST
	DCI VTMP3
	LI CHULL
	ST
	PI BHULLS

BD1:	DCI VST			; at game over v=2 puts the winner's own layout
	LM			;   in myShips[5-9]; show it on their board
	CI STOVER
	BNZ BD2
	DCI VBRD
	LM
	LR 0,A
	OI 0
	BZ BD2
	DCI VACT
	LM
	XS 0
	BNZ BD2
	DCI VTMP2
	LIS 5
	ST
	DCI VTMP3
	LI CREVL
	ST
	PI BHULLS

	ENDIF

BD2:	PI BCURS

BDFLSH:	PI BFLUSH
	LEAVE

; ---------------------------------------------------------------------------
; BCURS -- OR the reticle into VWANT if it belongs on board VBRD.
;
;   It belongs there only on an enemy board, only while you are playing, only
;   on the blink's lit phase -- and only against a player who is still in the
;   game, because a shot lands on every live opponent at once and the reticle
;   has to show exactly the boards that shot will reach.
;   Leaf. Clobbers A, r0-r3, DC0.
; ---------------------------------------------------------------------------
BCURS:	DCI VCLASS
	LM
	CI CLPLAY
	BNZ BCX
	DCI VACT
	LM
	OI 0
	BNZ BCX			; not your turn
	DCI VBLINK
	LM
	NI 8			; bit 3: the reticle phase, flipped every eighth
	BZ BCX			;   pass of the targeting loop, about 3 Hz
	DCI VBRD
	LM
	LR 0,A
	OI 0
	BZ BCX			; never on your own board
	DCI FNREPLY+GIPLYRS+PLSTAT
	LR A,0
	OI 0
	BZ BC1
BC0:	LI PLSTRID
	ADC
	DS 0
	BNZ BC0
BC1:	LM
	OI 0
	BNZ BCX			; PSPLAY only: a defeated board takes no shot

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
	LR 2,A			; 10y + x
	DCI VWANT
	LR A,2
	ADC
	LM
	OI CCURS
	LR 3,A
	DCI VWANT
	LR A,2
	ADC
	LR A,3
	ST
BCX:	POP
