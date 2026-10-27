; ram.inc -- where everything lives.
;
; Internal RAM (64 bytes):
;   $00-$07  register bank 0: the main program
;   $08-$17  the stack, 8 levels: the interrupts take up to 3
;   $18-$1F  register bank 1: the interrupts' own (r5: the next band)
;   $20-$3C  below
;   $3D-$3F  the BIOS's (its keyboard and IRQ routines)

ROWBUF  equ 020h                ; ROW_TAB: the next board row, staged
FRAMES  equ 035h                ; vblanks, for the main loop's clock
QUIET   equ 036h                ; frames whose last band has run
MODE    equ 037h                ; bit 0: a board screen (fixed.inc)
CURPG   equ 038h                ; the page mapped at $800
OLDPG   equ 039h                ; farcall's caller's page
SNDCNT  equ 03Ah                ; the sound: frames left (fixed.inc beep)
BLINKC  equ 03Bh                ; the cursor's blink clock (vblank)

MODE_BOARD equ 001h
MODE_SETUP equ 002h              ; the singles need setting up (fixed.inc obj_setup)

; External RAM (128 bytes, $00-$7F, MOVX with P1_MAIN):
LINEBUF equ 000h                ; 16: a text line being built (disp.inc)
TXCOL   equ 010h                ; its colour, and for characters with bit 7 set
TXCOL2  equ 011h
; the request in flight (net.inc)
NSTATE  equ 012h                ; NS_*
NSEQ    equ 013h                ; the sequence number posted
NWAIT   equ 014h                ; frames without progress
ACT     equ 015h                ; the action queued (0: none, a poll)
ACTARG  equ 016h                ; its argument
ACTSENT equ 017h                ; the action under way
NARG    equ 018h                ; its argument
NBWL    equ 019h                ; 2: STATUS's last bytes-waiting
NETHAVE equ 01Bh                ; how it ended: NH_*
RXLEN   equ 01Ch                ; 2: the reply's length
KLAST   equ 01Eh                ; BIOS readkey's memory (its r7)
KEY     equ 01Fh                ; the key pressed this frame (K_NONE: none)
; who and where
SCNAME  equ 020h                ; 9: the player's name, NUL-terminated
SCTABL  equ 029h                ; 9: the table's id
; input (input.inc)
JOY     equ 032h                ; the stick now: J_* bits
JOYP    equ 033h                ; last frame's
JOYEV   equ 034h                ; pressed since last frame
REP     equ 035h                ; auto-repeat countdown
TMP0    equ 036h                ; a loop's index across calls that keep nothing
TMP1    equ 037h
; the state's header, kept at state time (the window moves on after it)
NSEAT   equ 038h                ; playerCount
STAT    equ 039h                ; status
PSTAT   equ 03Ah                ; our playerStatus
ACTV    equ 03Bh                ; activePlayer (0FFh none)
MOVET   equ 03Ch                ; moveTime, seconds
LASTAT  equ 03Dh                ; lastAttackPos
LSTAT   equ 03Eh                ; the status the screen shows (0FFh none)
LACTV   equ 03Fh                ; the active player the screen shows
POLLW   equ 040h                ; frames until the next poll
SECF    equ 041h                ; frames to the clock's next second
LASTPG  equ 042h                ; the page the last state went to
; the boot's
AKLEN   equ 043h
SEL     equ 044h                ; a list's selection
NTBL    equ 045h                ; tables listed
NAMEN   equ 046h                ; the name typed so far
; the board (board.inc)
VIEW    equ 047h                ; the board shown: V_*
VIEWK   equ 048h                ; the view a key chose (0FFh: the default)
CURX    equ 049h                ; the cursor's cell
CURY    equ 04Ah
CURON   equ 04Bh                ; the cursor shows
DRAWN   equ 04Ch                ; the board shown is up to date
; the fleet being placed (p4.inc), and ours from the state (p5.inc)
FLEET   equ 04Dh                ; 5: each ship's pos + 100*dir
PSHIP   equ 052h                ; the ship being placed (5: all kept)
RNDS    equ 053h                ; 2: the random generator's state
PLSENT  equ 055h                ; the fleet has gone to the server
; the play (p5.inc)
SUNKM   equ 056h                ; 4: each seat's ships afloat, a bit each
LRES    equ 05Ah                ; the last shot's result shown (status 11-13)
LPOS    equ 05Bh                ; and its position
FIRED   equ 05Ch                ; we have fired this turn
SHOTP   equ 05Dh                ; the cell we fired at
LIVE    equ 05Eh                ; bit n: seat n is alive (not defeated)
AUTO    equ 05Fh                ; AUTOPLAY builds: the next cell to try
SCR0    equ 060h                ; scratch, 16 bytes: per-routine
OCC     equ 070h                ; 13: placement's cells taken, a bit each
HELP    equ 07Dh                ; 1: the help screen is up; 2: closing (p2.inc)
REDRAW  equ 07Eh                ; the phase's screen to come back, as it was
TMP2    equ 07Fh
