; ---------------------------------------------------------------------------
; help.inc -- how to play.
;
; It exists for one sentence: A SHOT LANDS ON EVERY ENEMY AT ONCE. That is the
; rule this game does not share with the board game, and nothing on the play
; screen can teach it. The Astrocade port carried a help screen and the Arcadia
; one dropped it for ROM budget; there are six kilobytes spare here.
;
; The lines are one blob of NUL-terminated strings walked by DC0, which DSTR
; leaves pointing just past each terminator. The row counter rides in r9,
; because every DCI on this path would wipe the pointer doing the walking.
; ---------------------------------------------------------------------------

SHELP:	LI LAY_TXT
	LR 0,A
	PI LAYLD
	LI CVAL3
	LR 3,A
	DCI SHELP1
	XDC
	PI UITITLE
	PI UICLRL
	LI CVAL1
	LR 3,A
	DCI SHELP9
	XDC
	PI UIFOOT

	DCI SHELPT
	CLR
	LR 9,A
SHL1:	LR A,9
	LR 0,A
	LI CVAL2
	LR 3,A
	XDC
	PI UIROW		; DC0 comes back pointing at the next line
	LR A,9
	INC
	LR 9,A
	CI NHELP
	BNZ SHL1

SHL2:	PI INSCAN
	LR A,0
	CI EV_NONE
	BZ SHL2
	LI ST_GAME
	LR 0,A
	JMP GOTO

NHELP	EQU 7
SHELPT:	DB "ONE SHOT HITS EVERY",0
	DB "ENEMY AT ONCE. THE",0
	DB "LAST FLEET AFLOAT WINS",0
	DB "",0
	DB "STICK AIMS  FIRE SHOOTS",0
	DB "HOLD TURNS A SHIP WHILE",0
	DB "YOU ARE PLACING IT",0
