; ---------------------------------------------------------------------------
; input.inc -- hand controllers and console buttons.
;
; Both are read through the same ports the video uses, and the read is gated by
; the LATCH the last write left behind: MAME's port_read_with_latch is
; (~ext | latch), so a bit only reads back as 0 when the input is pressed AND
; the corresponding latch bit is clear. So every scan writes 0 to the port
; first, reads, and complements -- after which a 1 bit means pressed.
;
; Port 0 bit 6 additionally gates the hand controllers: it must be CLEAR for
; ports 1 and 4 to return them at all.
;
; Console buttons on port 0: TIME $01, HOLD $02, MODE $04, START $08.
; Hand controller: right $01, left $02, down $04, up $08, twist CCW $10,
; twist CW $20, pull up $40, push down $80.
;
; "Push down" is the plunger on top of the stick -- the natural fire button on
; this console, and what this port uses for select.
; ---------------------------------------------------------------------------

; VIPCTL and VIPPAN are declared in config.asm, which owns the RAM map.

EV_NONE	EQU 0
EV_UP	EQU 1
EV_DOWN	EQU 2
EV_SEL	EQU 3
EV_BACK	EQU 4
EV_PGUP	EQU 5
EV_PGDN	EQU 6
EV_COPY	EQU 7
EV_INFO	EQU 8
EV_LEFT	EQU 9
EV_RIGHT EQU 10

; ---------------------------------------------------------------------------
; INSCAN -- poll everything and return ONE edge-triggered event in r0, and
;   click if there was one (see INOUT at the bottom; r8 is clobbered for it).
;   Edge-triggered: a held direction fires once. There is no auto-repeat, and
;   with nine rows on screen nothing needs one.
;   Clobbers A, r0, r1, r2, r3, DC0.
; ---------------------------------------------------------------------------
INSCAN:	CLR
	OUTS 0			; latch 0 clear: buttons live, controllers enabled
	CLR
	OUTS 1			; latch 1 clear: right controller readable
	INS 1
	COM
	LR 1,A			; r1 = controller, 1 = pressed
	INS 0
	COM
	NI 00FH
	LR 2,A			; r2 = console buttons, 1 = pressed

	; controller edges: pressed now AND not pressed last time
	DCI VIPCTL
	LM
	COM
	NS 1
	LR 0,A			; r0 = newly pressed controller bits
	DCI VIPCTL
	LR A,1
	ST

	; console-button edges. The current state has to be stashed before r2 is
	; overwritten with the edges, or the next scan compares against the wrong
	; thing and every press fires twice.
	DCI VIPPAN
	LM
	COM
	NS 2
	LR 3,A			; r3 = newly pressed buttons
	DCI VIPPAN
	LR A,2
	ST			; save the state we actually saw
	LR A,3
	LR 2,A

	; decode, in priority order
	LR A,0
	NI 008H			; stick up
	BNZ INUP
	LR A,0
	NI 004H			; stick down
	BNZ INDN
	LR A,0
	NI 002H			; stick left
	BNZ INLT
	LR A,0
	NI 001H			; stick right
	BNZ INRT
	LR A,0
	NI 080H			; plunger: select
	BNZ INSEL
	LR A,2
	NI 008H			; START: select
	BNZ INSEL
	LR A,2
	NI 004H			; MODE: back
	BNZ INBK
	LR A,2
	NI 001H			; TIME: page up
	BNZ INPU
	LR A,2
	NI 002H			; HOLD: page down
	BNZ INPD
	LR A,0
	NI 010H			; twist counter-clockwise: adapter info
	BNZ ININF
	LR A,0
	NI 020H			; twist clockwise: copy
	BNZ INCPY
	CLR
	LR 0,A
	POP

; INOUT -- every real event leaves through here, so the key click costs one
;   copy of itself rather than ten. SNDCUE is inlined rather than called
;   because INSCAN is a leaf reached by PI from a dozen polling loops, and
;   giving it an ENTER would push every screen a K-level deeper.
;
;   Nothing waits: the tone rings and decays on its own, which is the only
;   reason a click is affordable in a loop that runs every 8 ms.
INOUT:	LR 0,A			; the event, safe from what follows
	INS 5
	NI 3FH
	LR 8,A
	OUTS 5			; through silence, or an identical blip will not retrigger
	LR A,8
	AI SND1
	OUTS 5
	POP

INUP:	LI EV_UP
	BR INOUT
INDN:	LI EV_DOWN
	BR INOUT
INLT:	LI EV_LEFT
	BR INOUT
INRT:	LI EV_RIGHT
	BR INOUT
INSEL:	LI EV_SEL
	BR INOUT
INBK:	LI EV_BACK
	BR INOUT
INPU:	LI EV_PGUP
	BR INOUT
INPD:	LI EV_PGDN
	BR INOUT
INCPY:	LI EV_COPY
	BR INOUT
ININF:	LI EV_INFO
	BR INOUT
