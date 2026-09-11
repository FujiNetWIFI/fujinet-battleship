; ---------------------------------------------------------------------------
; battleship.asm -- FujiNet Battleship for the Fairchild Channel F.
;
; A standalone F8 client in the mould of ../arcadia and ../astrocade: the
; shared C core assumes a keyboard and kilobytes of buffers, so the game is
; written to the machine instead. The game logic is the Arcadia port's, re-cut
; for a console that has the opposite problem.
;
; WHY THIS MACHINE IS THE EASY ONE. Astrocade, Arcadia and ColecoVision all
; push both mailbox directions through the READ path, because none of those
; cart edges carries a write strobe -- which is why the Arcadia client lives in
; 84 bytes of RAM and the Odyssey2 client in 39. A Channel F cart is not a ROM;
; it is a peer on the F8 bus that services ROMC 05 stores. So the FujiNet cart
; hands the console 16K of ROM at $0800, 30K of read/write RAM at $8000, and
; the whole 1K reply window flat at $F800. Nothing here is rationed.
;
; WHAT IS SCARCE IS PIXELS. VRAM is 128x64 at 2bpp, write-only through I/O
; ports, and one plotted pixel costs about 80us -- a full-screen clear is most
; of a second. Two things follow, and they shape the whole port:
;
;   * The palette is BANDED per scanline (columns 125/126 of each row pick it),
;     so the chrome rows run palette 0 for black-on-white contrast while the
;     board band runs palette 1, whose value-0 ground is LTBLUE and therefore
;     IS the open sea. The sea costs zero pixels.
;   * Boards repaint by DIRTY CELL against a 400-byte shadow in cart RAM. A
;     poll typically dirties one to three cells; moving the reticle dirties
;     exactly two. No sibling can afford the shadow; here it is free.
;
; STRUCTURE. The F8 keeps exactly one return address (PI copies PC0 into PC1,
; POP restores it), so this is a flat state machine that JMPs between states
; and never returns into one. Calls nest through the BIOS K stack at r40-r58
; -- nine levels -- via f8call.inc's ENTER/LEAVE.
;
; THE TRAP THAT GOVERNS EVERY ROUTINE HERE: PI and JMP both stash the target
; address's high byte in the accumulator, so A DOES NOT SURVIVE A CALL, and
; the drawing primitives write r0 and r2-r8 between them. Arguments, loop
; counters and error codes therefore live in RAM, not registers.
; ---------------------------------------------------------------------------
	CPU F3850

; Macros must be defined before they are used, so the three EQU/macro-only
; includes come first. They emit no code, so nothing lands before the header.
	INCLUDE "flags.inc"
	INCLUDE "f8call.inc"
	INCLUDE "fujinet.inc"

; --- state ids ------------------------------------------------------------
ST_NAME	EQU 0			; fetch the shared username from its appkey
ST_EDIT	EQU 1			; type one, if the appkey was empty
ST_TABLE EQU 2			; pick a table
ST_GAME	EQU 3			; the poll loop: lobby, placement, play, over
ST_HELP	EQU 4			; how to play

; ---------------------------------------------------------------------------
; RAM ledger -- the 30K arena at $8000. There is no rationing here and no
; ledger discipline to keep; the addresses are grouped only so a probe script
; can find them. VSTATE is what the emulator harnesses watch.
; ---------------------------------------------------------------------------
; --- display scratch ---
VHEX	EQU 08000H		; 3 bytes: two hex digits and a NUL
VPALR0	EQU 08003H		; DPALR row range
VPALR1	EQU 08004H
VSTATE	EQU 08006H		; <- the harness reads this
VROW	EQU 08007H		; ui.inc loop counter
VPIXY	EQU 08008H		; ui.inc pixel row
VCUR	EQU 08009H		; ui.inc list cursor
VNROW	EQU 0800AH		; list rows filled on this page
VTMP	EQU 0800BH
VTMP2	EQU 0800CH
VTMP3	EQU 0800DH
; --- input ---
VIPCTL	EQU 08010H		; input.inc: last controller state
VIPPAN	EQU 08011H		; last console-button state
VIPRPT	EQU 08012H		; auto-repeat countdown
VIPHLD	EQU 08013H		; the direction being held
; --- net ---
VREQ	EQU 08020H		; RQ*: which request BLDURL builds
VAVLO	EQU 08021H		; STATUS bytes-waiting
VAVHI	EQU 08022H
VPRVLO	EQU 08023H		; the settle loop's previous reading
VPRVHI	EQU 08024H
VNTRY	EQU 08025H		; settle attempts left
VRXLO	EQU 08026H		; reply length, captured right after the READ
VRXHI	EQU 08027H
VMIN0	EQU 08028H		; the length the reply cannot be shorter than
VMIN1	EQU 08029H
VFAILC	EQU 0802AH		; failure code -- in RAM, because a draw routine
				;   would have overwritten a register holding it
; --- this poll's header ---
VPC	EQU 08030H		; playerCount
VST	EQU 08031H		; status
VPLST	EQU 08032H		; your playerStatus
VACT	EQU 08033H		; activePlayer
VMVT	EQU 08034H		; moveTime
VLASTP	EQU 08035H		; lastAttackPos
VCLASS	EQU 08036H		; screen class, CL*
; --- the previous poll, for edge detection ---
VPRVPC	EQU 08038H
VPRVST	EQU 08039H
VPRVACT	EQU 0803AH
VPRVCLS	EQU 0803BH
VCUEQ	EQU 0803CH		; the cue queued for after the draw
VPRVLAY	EQU 0803DH		; which layout record is loaded
; --- cursor, targeting, pacing ---
VCURX	EQU 08040H
VCURY	EQU 08041H
VBLINK	EQU 08042H		; reticle blink phase
VTICK	EQU 08043H		; poll countdown
VPEND	EQU 08044H		; staged request, sent with the next poll
VREADY	EQU 08045H		; our own idea of the ready flag
VCLOCK	EQU 08046H		; local move clock -- the client bails on its own
VSEEDED	EQU 08047H		; placement: is this phase's fleet seeded yet?
; --- placement ---
VSHIP	EQU 08048H		; the ship being adjusted
VSDIR	EQU 08049H
VSX	EQU 0804AH
VSY	EQU 0804BH
VSLEN	EQU 0804CH
VSEG	EQU 0804DH
VRNDL	EQU 0804EH		; u16 LFSR
VRNDH	EQU 0804FH
; --- board renderer ---
VBRD	EQU 08050H		; board index being drawn
VBCX	EQU 08051H		; cell x, y
VBCY	EQU 08052H
VBST	EQU 08053H		; the state this cell WANTS to be
VBOX	EQU 08054H		; this board's origin
VBOY	EQU 08055H
VBSHP	EQU 08056H		; hull walker
VBFLG	EQU 08057H		; render flags
VBPLR	EQU 08058H		; the player whose record we are reading
; --- list screens ---
VTTOP	EQU 08090H		; first record shown on this page
VTCNT	EQU 08091H		; records the reply carried
; --- placement: the candidate, sheltered across a rejected move ---
; These are NOT VTMP..VTMP3. PLOCCB walks its ship list in VTMP2, so a save
; into the shared scratch and a rebuild of the occupancy map destroy each
; other -- which sent the loop counter off to VSX, past its terminator, and
; then marched PLWALK through the shadow with a length read out of nowhere.
VSAVD	EQU 08094H
VSAVX	EQU 08095H
VSAVY	EQU 08096H
; --- the loaded layout record ---
VLAY	EQU 08060H		; LRECSZ bytes, copied from LAY*
; --- string building ---
VLINE	EQU 08100H		; one screen row under construction
VDIGIT	EQU 08140H		; DEC5 output
				; strings.inc owns 08150H-08158H
VNAME	EQU 08160H		; player name, 8 + NUL
VTABLE	EQU 08170H		; table id, 8 + NUL
VPLBUF	EQU 08180H		; a name lifted out of the reply window
VENTRY	EQU 08190H		; the on-screen keyboard's accumulator
; --- editor ---
VEDX	EQU 081A0H
VEDY	EQU 081A1H
VEDLEN	EQU 081A2H
VEDCASE	EQU 081A3H
VEDDR	EQU 081A4H
VEDDC	EQU 081A5H
; --- arrays ---
VSHIPS	EQU 08200H		; NSHIPS committed placements, pos + 100*dir
VOCC	EQU 08210H		; BRDCELL occupancy map for placement
VWANT	EQU 08280H		; BRDCELL: the state each cell WANTS to be, this poll
VSHDW	EQU 08300H		; MAXPLR x BRDCELL: the cell state last PAINTED
NAMEMAX	EQU 8

	ORG 0800H
	DB 55H			; cart signature -- the BIOS compares this
	DB 00H			; $0801 is skipped; the BIOS jumps to $0802

; ---------------------------------------------------------------------------
; ENTRY. The BIOS has already zeroed all 64 scratchpad registers and set r59
; (the K-stack pointer) to 40, so nested calls work from this instruction on.
; Interrupts stay off for the program's whole life: the console wires no
; interrupt source at all, so there is no vblank and every wait is a busy one.
; ---------------------------------------------------------------------------
ENTRY:	DI

	PI CLSALL		; clear, then band the palette -- in that order,
				;   the BIOS clear paints columns 125/126 too

	; zero the small variables one page at a time. A block store is one DCI
	; and a run of STs riding DC0's auto-increment.
	DCI 08000H
	LI 0F0H
	LR 1,A
EZ1:	CLR
	ST
	DS 1
	BNZ EZ1

	DCI VNAME
	CLR
	ST
	DCI VTABLE
	CLR
	ST
	DCI VRNDL		; any nonzero seed. The idle loops stir it by
	LI 0A5H			;   drawing from it once per input poll, so the
	ST			;   placement layout depends on how long the
	LI 05CH			;   player spent on the name and table screens.
	ST
	DCI VPRVLAY
	LI 0FFH
	ST			; no layout loaded yet

	PI FNCHK
	BZ MAIN

	; No cart answering. Say so and stop -- there is nothing else this
	; program can do, and the two magic bytes it read are worth showing.
	LI CVAL2
	LR 3,A
	LI DORGX
	LR 1,A
	LI DORGY+2*DCELLH
	LR 2,A
	DCI SNOFN
	PI DSTR
HLTNC:	BR HLTNC

; ---------------------------------------------------------------------------
; MAIN -- the state dispatcher. Every state is entered by JMP and leaves by
; jumping to GOTO, so the K stack never accumulates depth across a transition.
; ---------------------------------------------------------------------------
MAIN:	DCI VSTATE
	LM
	CI ST_NAME
	BNZ MD1
	JMP SNAME
MD1:	CI ST_EDIT
	BNZ MD2
	JMP SEDIT
MD2:	CI ST_TABLE
	BNZ MD3
	JMP STABLE
MD3:	CI ST_HELP
	BNZ MD4
	JMP SHELP
MD4:	CI ST_GAME
	BNZ MAIN
	JMP SGAME

; GOTO -- enter the state in r0. Takes its argument in r0, not A: the JMP that
; gets here would have clobbered A on the way.
;
; It also REWINDS THE K STACK. Every state is entered by JMP and left by
; jumping back here, so the ENTER at the top of a state never meets its LEAVE
; and each visit would otherwise leak two bytes of the nine-level stack at
; r40-r58. On the tenth the push lands on r59 -- the stack pointer itself --
; and the next return goes somewhere arbitrary. Since no state ever returns
; into another, starting each one at the bottom is exactly right.
GOTO:	LISU 7			; ISAR = 59, the BIOS K-stack pointer
	LISL 3
	LI 40
	LR S,A
	DCI VSTATE
	LR A,0
	ST
	JMP MAIN

; ---------------------------------------------------------------------------
; CLSALL -- clear the screen and put the default two-band palette back.
;   The bands are the port's one genuinely Channel F trick: palette 0 for the
;   chrome rows (the only place this machine HAS white) and palette 1 for the
;   board band, where value 0 is LTBLUE and is therefore the sea itself.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
CLSALL:	ENTER
	LI CVAL0
	LR 3,A
	PI BCLRSCR
	PI PALBND
	LEAVE

; PALBND -- lay the three palette bands down over the whole screen, taking the
;   board band's extent from the loaded layout record. Reached by PI; calls one
;   level down.
PALBND:	ENTER
	DCI VLAY+LRBND0
	LM
	LR 0,A			; band top
	LM
	LR 1,A			; band bottom
	LR A,0
	CI 0
	BNZ PBND1
	LI 11			; no record loaded yet: the common case
	LR 0,A
	LI 52
	LR 1,A

PBND1:	CLR			; rows 0 .. bandtop-1 : palette 0
	DCI VPALR0
	ST
	LR A,0
	AI 0FFH
	DCI VPALR1
	ST
	LI PAL0A
	LR 3,A
	LI PAL0B
	LR 4,A
	PI DPALR

	LR A,0			; the board band : palette 1, LTBLUE sea
	DCI VPALR0
	ST
	LR A,1
	DCI VPALR1
	ST
	LI PAL1A
	LR 3,A
	LI PAL1B
	LR 4,A
	PI DPALR

	LR A,1			; bandbottom+1 .. 63 : palette 0
	INC
	DCI VPALR0
	ST
	LI 63
	DCI VPALR1
	ST
	LI PAL0A
	LR 3,A
	LI PAL0B
	LR 4,A
	PI DPALR
	LEAVE

; ---------------------------------------------------------------------------
; RND -- a 16-bit Galois LFSR, polynomial $B400. Returns the low byte in A.
;   The state is in RAM because nothing may hold a value in a register across
;   the draw calls that surround every caller.
;   Leaf. Clobbers A, r0-r5, DC0.
; ---------------------------------------------------------------------------
RND:	DCI VRNDL
	LM
	LR 0,A			; low
	LM
	LR 1,A			; high
	LR A,0
	NI 1
	LR 2,A			; the bit shifted out

	LR A,1
	SR 1
	LR 3,A			; high >> 1
	LR A,0
	SR 1
	LR 5,A			; low >> 1
	LR A,1
	NI 1
	OI 0			; LR A,r does not set flags; this does
	BZ RND1
	LR A,5
	OI 080H
	LR 5,A			; the old high bit 0 becomes low bit 7
RND1:	LR A,2
	OI 0
	BZ RND2
	LR A,3
	XI 0B4H			; the taps, folded into the high byte
	LR 3,A
RND2:	DCI VRNDL
	LR A,5
	ST
	LR A,3
	ST
	LR A,5
	POP

; ---------------------------------------------------------------------------
; RNDMOD -- A = a random value in 0 .. VTMP-1, for VTMP <= 128.
;   Mask to seven bits and redraw on an out-of-range value rather than taking a
;   remainder: the F8 has no divide, and at the counts this game uses (10, 100)
;   the rejection rate is not worth a subtraction loop.
;   The modulus travels in RAM because RND writes r0-r5.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
RNDMOD:	ENTER
RNDM1:	PI RND
	NI 07FH
	LR 0,A
	DCI VTMP
	LM
	COM
	INC			; -m
	AS 0			; v - m
	BC RNDM1		; carry = no borrow = v >= m: redraw
	LR A,0
	LEAVE

; ---------------------------------------------------------------------------
; FAIL -- the shared error screen. The code comes from VFAILC, not a register:
;   the draw calls below write most of the file.
; ---------------------------------------------------------------------------
FAIL:	PI CLSALL
	LI CVAL2
	LR 3,A
	LI DORGX
	LR 1,A
	LI DORGY+3*DCELLH
	LR 2,A
	DCI SNETER
	PI DSTR
	DCI VFAILC
	LM
	LR 0,A
	DCI VHEX
	PI DHEXS
	LI CVAL2
	LR 3,A
	LI DORGX+9*DCELLW
	LR 1,A
	LI DORGY+3*DCELLH
	LR 2,A
	DCI VHEX
	PI DSTR
	LI CVAL1
	LR 3,A
	LI DORGX
	LR 1,A
	LI DORGY+5*DCELLH
	LR 2,A
	DCI SANYKEY
	PI DSTR
FAIL1:	PI INSCAN
	LR A,0
	CI EV_NONE
	BZ FAIL1
	LI ST_TABLE
	LR 0,A
	JMP GOTO

MB_MAIN:

	INCLUDE "fujidisp.inc"
MB_DISP:
	INCLUDE "ui.inc"
	INCLUDE "strings.inc"
	INCLUDE "sound.inc"
	INCLUDE "input.inc"
MB_UI:
	INCLUDE "fujilib.inc"
	INCLUDE "net.inc"
	INCLUDE "url.inc"
MB_NET:
	INCLUDE "layout.inc"
	INCLUDE "board.inc"
MB_BOARD:
	INCLUDE "nament.inc"
	INCLUDE "lobby.inc"
MB_LOBBY:
	INCLUDE "place.inc"
	INCLUDE "target.inc"
	INCLUDE "game.inc"
MB_GAME:
	INCLUDE "help.inc"
	INCLUDE "strdata.inc"
	INCLUDE "font.inc"
	INCLUDE "art.inc"
MB_DATA:

	ORG 0800H+FN_ROM_CLAIM
	DB "FUJI"		; without this the cart boots us with the
				;   mailbox dead and the arena reverts to RAM
	END
