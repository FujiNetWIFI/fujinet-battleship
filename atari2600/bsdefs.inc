; bsdefs.inc -- what every bank of the Battleship client agrees on.
;
; Equates only, so this goes BEFORE the ORG next to fujinet.inc.
;
; This console has 128 bytes of RAM and the stack mirrors into the top of
; them. What that buys, once you accept it, is a design where the CARTRIDGE
; holds the state: the server's reply sits in the cartridge's 512-byte window
; and is read from there in place, the board is composed in the cartridge by
; FN_BLIT_FIELD, and the console holds a cursor, a phase and a handful of
; counters. Nothing here is a buffer.

; The banks. A bank switch replaces every byte of $1000-$17FF, so each carries
; its own copy of bslib, bsdisp and net; only these variables cross.
BANKLOB EQU     0               ; the table list and joining one
BANKGAM EQU     1               ; the game: state, the board, attacking
BANKPLC EQU     2               ; placing your five ships

; The trampoline, at a fixed address in the fixed tail because the banks are
; assembled separately. See bstail.asm.
BSGOTO  EQU     FNTAIL

; ---------------- zero page ----------------
; $80-$8E is the transport's (fujinet.inc) and $81/$82 the display kernel's.
; $A8/$A9 are INSCAN's. $AA-$BF is ours; the stack must stay above $C0.
BSLEAD  EQU     $8C             ; FNDEC's leading-zero latch
BSTMP   EQU     $8D             ; scratch, short-lived
DSTP    EQU     $93             ; where BSDEC sends its digits, 2 bytes

; $8F-$A7 is the ROM swap stub's landing ground in the firmware's own clients.
; This program never swaps -- it boots nothing -- so it is ours.
BSREQ   EQU     $8F             ; the request APICALL is running (RQ*)
BSTRIES EQU     $90             ; the STATUS settle loop's bound
BSDLY   EQU     $94             ; its inter-poll delay counter, in memory
                                ;   because APWAIT counts in X and Y
BSNAME  EQU     $91             ; the chosen player name, index into NAMES
BSTABLE EQU     $92             ; the chosen table, index into the listing

; The one thing that genuinely has to be copied into console RAM: the table
; id. Everything else this program displays is read out of the cartridge's
; reply window in place -- but the id chosen from a /tables listing has to
; survive every /state poll after it, and each poll repaints that window.
; Nine characters plus a terminator, and it is worth the ten bytes.
BSTBL   EQU     $95             ; 10 bytes, $95-$9E

; The five staged ship placements, pos + 100*dir, during STPLACE.
SHIPS   EQU     $A0             ; 5 bytes, $A0-$A4

BSPHASE EQU     $AA             ; the screen we are on (PH*)
BSCURX  EQU     $AB             ; board cursor, 0-9
BSCURY  EQU     $AC
BSSEL   EQU     $AD             ; list cursor on the table screen
BSCNT   EQU     $AE             ; rows on the list
BSVIEW  EQU     $AF             ; which opponent's board is shown
BSPCNT  EQU     $B0             ; playerCount from the last reply
BSSTAT  EQU     $B1             ; game status from the last reply
BSMYST  EQU     $B2             ; your player status
BSACT   EQU     $B3             ; the active player, $FF for nobody
BSERR   EQU     $B4             ; the last failure
BSSTEP  EQU     $B5             ; where it happened
BSTICK  EQU     $B6             ; frame counter, for the blinking cursor
BSPOLL  EQU     $B7             ; frames until the next /state
BSSHIP  EQU     $B8             ; which ship is being placed, 0-4
BSDIR   EQU     $B9             ; 0 horizontal, 1 vertical
BSAVL0  EQU     $BA             ; NET_STATUS bytes waiting
BSAVL1  EQU     $BB
BSPRV0  EQU     $BC             ; the previous reading, for the settle loop
BSPRV1  EQU     $BD
BSRXL0  EQU     $BE             ; the READ's reply length, captured at once
BSRXL1  EQU     $BF

; The placement screen's hull walk. It runs only in bank 2 and only between
; polls, so it shares the play bank's cells rather than claiming new ones.
BSDIRT  EQU     $BA             ; = BSAVL0
BSPOS   EQU     $BB             ; = BSAVL1
BSSEG   EQU     $BC             ; = BSPRV0

; ---------------- requests ----------------
RQTABLE EQU     0
RQSTATE EQU     1
RQREADY EQU     2
RQPLACE EQU     3
RQATTCK EQU     4
RQLEAVE EQU     5

; ---------------- screen ----------------
; 21 rows of 12 columns. One board at a time: two 10x10 grids plus a status
; line does not fit, and at this size showing one is more readable anyway.
;
;   0        prompt from the server, truncated to 12
;   1        whose board, and the move clock
;   2        column ruler
;   3-12     the board -- column 0 the row digit, 1-10 the cells
;   13       blank
;   14       the cursor coordinate and what is under it
;   15       blank
;   16-19    the players: name and status
;   20       the hint, or the error
BROW    EQU     3               ; the board's first row
RPROMPT EQU     0
RWHOSE  EQU     1
RRULER  EQU     2
RCOORD  EQU     14
RPLYR0  EQU     16
RHINT   EQU     20

MAXPLYR EQU     4
