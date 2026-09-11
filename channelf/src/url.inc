; ---------------------------------------------------------------------------
; url.inc -- building the devicespec straight into the TX page.
;
; Nothing is assembled in RAM: the cart counts the payload itself, so a write
; ANYWHERE in the TX page appends and the whole request is a run of stores.
; The worst case is /place with five three-digit positions, about 100 bytes
; against the 320-byte ceiling.
;
; Every helper re-points DC0 at the TX page itself rather than trusting the
; caller to have left it there. A DCI between FNBEG and an append -- reading a
; variable, say -- otherwise redirects the bytes into RAM: the command goes out
; with no payload AND scribbles over whatever the pointer was aimed at. The
; write cursor lives in the cart, so re-pointing cannot rewind the stream.
; ---------------------------------------------------------------------------

; UTXB -- append the single byte in r0. Leaf. Clobbers A, DC0.
UTXB:	DCI FNTX
	LR A,0
	ST
	POP

; ---------------------------------------------------------------------------
; BLDURL -- stream the devicespec for the request in VREQ.
;   Reached by PI; calls one level down (FNTXS, UTXB, UDEC).
; ---------------------------------------------------------------------------
BLDURL:	ENTER
	DCI UNPFX
	XDC
	PI FNTXS
	DCI UENDPT
	XDC
	PI FNTXS

	DCI VREQ
	LM
	CI RQTABLE
	BNZ BU1
	DCI UTABLS		; tables takes neither a table nor a player
	XDC
	PI FNTXS
	LEAVE

BU1:	CI RQSTATE
	BNZ BU2
	DCI USTATE
	XDC
	PI FNTXS
	BR BUQ

BU2:	CI RQREADY
	BNZ BU3
	; /ready/1 and /ready/0, NOT the deprecated bare /ready that toggles.
	; Sending the value we want removes the whole class of bug where a
	; press between two polls readies and un-readies, and where a harness
	; holding FIRE cancels the server's countdown every time round.
	DCI UREADY
	XDC
	PI FNTXS
	DCI VREADY
	LM
	AI '0'
	LR 0,A
	PI UTXB
	BR BUQ

BU3:	CI RQPLACE
	BNZ BU4
	DCI UPLACE
	XDC
	PI FNTXS
	DCI VTMP2
	CLR
	ST			; ship index
BU3L:	DCI VTMP2
	LM
	LR 0,A
	DCI VSHIPS
	LR A,0
	ADC
	LM
	DCI VTMP3
	ST
	PI UDEC			; the staged move is raw bytes, not text, so
				;   there is no 20-character move buffer here
	DCI VTMP2
	LM
	INC
	DCI VTMP2
	ST
	CI NSHIPS
	BZ BUQ			; no comma after the last
	LI ','
	LR 0,A
	PI UTXB
	BR BU3L

BU4:	CI RQATTCK
	BNZ BU5
	DCI UATTCK
	XDC
	PI FNTXS
	DCI VCURY
	LM
	LR 0,A
	PI MUL10
	LR 1,A			; capture 10y NOW. MUL10 returns it in A, and the
	DCI VCURX		;   LM two instructions down overwrites A with x --
	LM			;   which shipped every shot as x+y instead of
	AS 1			;   x+10y, so every row landed in row 0.
	DCI VTMP3
	ST
	PI UDEC
	BR BUQ

BU5:	DCI ULEAVE
	XDC
	PI FNTXS

BUQ:	DCI UQTAB
	XDC
	PI FNTXS
	DCI VTABLE
	XDC
	PI FNTXS
	DCI UQPLR
	XDC
	PI FNTXS
	DCI VNAME
	XDC
	PI FNTXS
	DCI UQBIN
	XDC
	PI FNTXS
	LEAVE

; ---------------------------------------------------------------------------
; UDEC -- append VTMP3 (0-255) as one to three decimal digits, no leading
;   zeros. Takes its byte in RAM, not a register: BLDURL's own loop counter
;   lives there too and FNTXS writes the data counters.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
UDEC:	ENTER
	DCI VTMP3
	LM
	LR 1,A			; the running remainder
	CLR
	LR 2,A			; hundreds
UD1:	LR A,1
	AI 156			; -100
	BNC UD2
	LR 1,A
	LR A,2
	INC
	LR 2,A
	BR UD1
UD2:	CLR
	LR 3,A			; tens
UD3:	LR A,1
	AI 246			; -10
	BNC UD4
	LR 1,A
	LR A,3
	INC
	LR 3,A
	BR UD3
UD4:	LR A,2
	OI 0
	BZ UD5
	AI '0'
	LR 0,A
	PI UTXB
	LIS 1
	LR 4,A			; something has been emitted
	BR UD6
UD5:	CLR
	LR 4,A
UD6:	LR A,3
	OI 0
	BNZ UD7
	LR A,4
	OI 0
	BZ UD8			; nothing emitted yet and tens 0: skip
UD7:	LR A,3
	AI '0'
	LR 0,A
	PI UTXB
UD8:	LR A,1
	AI '0'
	LR 0,A
	PI UTXB
	LEAVE

; ---------------------------------------------------------------------------
; MUL10 -- A = 10 * r0, for r0 <= 25. 8v + v + v, no multiply on this CPU.
;   Leaf. Clobbers A, r1.
; ---------------------------------------------------------------------------
MUL10:	LR A,0
	SL 1
	SL 1
	SL 1
	LR 1,A			; 8v
	LR A,0
	AS 1
	AS 0			; 8v + v + v
	POP

	INCLUDE "endpoint.inc"
UNPFX:	DB "N:",0
UTABLS:	DB "tables?bin=1",0
USTATE:	DB "state",0
UREADY:	DB "ready/",0
UPLACE:	DB "place/",0
UATTCK:	DB "attack/",0
ULEAVE:	DB "leave",0
UQTAB:	DB "?table=",0
UQPLR:	DB "&player=",0
UQBIN:	DB "&bin=1&v=2",0		; v=2: winner index and ship reveal at gameover
