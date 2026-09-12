; fujinet.inc -- equates for the FujiNet cartridge mailbox.
;
; Copied from fujinet-firmware/pico/atari-2600/testrom/fujinet.inc, which is
; itself hand-mirrored from firmware/include/fuji_mailbox.h. tools/checkdefs.py
; compares this file against that header on every build, so the copy cannot
; drift.
;
; TRIMMED: the FUJI device's file and boot commands are gone. This program
; mounts nothing, boots nothing and reads no directory -- it opens one HTTP
; connection to the Battleship server and polls it. What is left is the
; transport, the text ports, the blit port and the input map.
;
; Split from fujilib.inc because that file contains CODE, and code has to be
; assembled inside the ORG'd program. The equates have to come first, so they
; live here. (Including the code half early instead assembles the whole
; transport at address $0000 and every JSR to it becomes `20 00 00` -- which
; assembles cleanly and fails at run time.)

; ---------------- cart -> console, painted ----------------
FNTEXT  EQU     $1800           ; six 128-byte text planes
FNRPLY  EQU     $1B00           ; the reply window, 512 bytes
FNACKS  EQU     $1F00           ; echoes SEQ when the reply is ready
FNSTAT  EQU     $1F01           ; bit0 link up, bit1 busy
FNERR   EQU     $1F02           ; FNE* below
FNRCMD  EQU     $1F03           ; $06 ACK / $15 NAK
FNRXLO  EQU     $1F04           ; reply length, little-endian
FNRXHI  EQU     $1F05
FNMAG0  EQU     $1F09           ; 'F'
FNMAG1  EQU     $1F0A           ; 'N'
FNPVER  EQU     $1F0B
FNSECH  EQU     $1F0C           ; slice echo, published last after a repaint
FNBTXG  EQU     $1F0D           ; last text row rendered, plus a toggle
FNBBNK  EQU     $1F0E           ; the live bank
FNBFLG  EQU     $1F0F           ; bit0 armed, bit1 claim honoured

; ---------------- console -> cart, write-only ----------------
; NEVER READ THESE PAGES. The cart does not drive them -- not driving them is
; exactly what lets it sample the data bus -- so a read returns the floating
; bus, and worse, is decoded as an access.
FNRSEL  EQU     $1D00           ; + n (n < $80): arm register n
FNCMT   EQU     $1DFF           ; commit the armed register with this data
FNTX    EQU     $1E00           ; a write anywhere in the page appends

; Registers, unchanged from the sibling ports so fujimail.c compiles verbatim.
FR_DEV  EQU     $00
FR_CMD  EQU     $01
FR_NPAR EQU     $02
FR_DRST EQU     $05             ; any value: rewind the TX write pointer
FR_RXSL EQU     $06
FR_SEQ  EQU     $10             ; nonzero and != ACKSEQ: launch

; One-shot operations, in the bit7-set half of the control page.
FH_BANK EQU     $80
FH_TROW EQU     $F0
FH_TCHR EQU     $F1
FH_TEND EQU     $F2
FH_ARM1 EQU     $FC
FH_ARM2 EQU     $FD
FH_SWAP EQU     $FE

FNAM1   EQU     $B5             ; the arming pair
FNAM2   EQU     $4A

FNTCOL  EQU     12              ; text columns

; $1F20-$1FFB is the client's fixed tail: the cold stub, the bank trampolines,
; and anything else that must be at the same address from every bank.
FNTAIL  EQU     $1F20

; Devices and commands, from include/fujiCommandID.h and fujiDeviceID.h.
FNDEVN  EQU     $71             ; N1:


; FN_R_BOOT_STATE values.

FNEOK   EQU     0
FNEWAIT EQU     $FF             ; the cart never answered at all
FNENAK  EQU     $EE             ; the server said NAK
FNEBADL EQU     $FC             ; the reply is shorter than its header implies
FNENOC  EQU     $FD             ; no cartridge answered at all -- deliberately
                                ;   not 1, which is FN_ERR_NOLINK

; ---------------- zero page ----------------
FNSEQ   EQU     $80             ; the sequence we are waiting for
FNCNT   EQU     $83             ; timeout quanta remaining
FNDEV   EQU     $86             ; transaction setup, read by FNBEG
FNCMD   EQU     $87
FNNPR   EQU     $88
FNTMO   EQU     $89             ; timeout in ~0.6s quanta
FNPTRL  EQU     $8A
FNPTRH  EQU     $8B
FNPCNT  EQU     $8E             ; bytes written into the current path payload
INCUR   EQU     $A8             ; input, above the swap stub's landing ground
INPREV  EQU     $A9


; ---------------- input ----------------
; Joystick 0 in SWCHA bits 7-4 and the console switches in SWCHB, all active
; low, folded by INSCAN into one active-high byte.
IN_UP   EQU     $01
IN_DOWN EQU     $02
IN_LEFT EQU     $04
IN_RIGHT EQU    $08
IN_FIRE EQU     $10
IN_SEL  EQU     $20
IN_RST  EQU     $40

; ---------------- the blit port ----------------
; Six stores move a screenful of bytes the console has neither the RAM nor the
; raster time to move itself. FN_BLIT_FIELD is here for this program: turning
; a 100-byte gamefield into ten rows of text is 100 reads, 100 compares and a
; 16-bit reply cursor, about 250 bytes of 6502 in a bank that has under a
; thousand to spare.
FH_BSL  EQU     $F5             ; source offset into the reply, low
FH_BSH  EQU     $F6             ; ...high
FH_BDL  EQU     $F7             ; destination, low (a text ROW for FIELD)
FH_BDH  EQU     $F8             ; ...high
FH_BCNT EQU     $F9             ; count, or the cursor cell for FIELD
FH_BGO  EQU     $FA             ; data = the transform; fire
BLTRAW  EQU     0
BLTTEXT EQU     1
BLTFLD  EQU     2               ; gamefield -> ten rows
BLTHULL EQU     3               ; five placements -> a hull overlay
BLTSEA  EQU     4               ; fill the composed board with open sea
BLTCELL EQU     5               ; board[cnt] = the low byte of the source
BLTPNT  EQU     6               ; paint the composed board into ten rows
BLTNCUR EQU     $FF             ; FIELD: no cursor
FN_SEA  EQU     '.'             ; the composed board's cell glyphs
FN_HIT  EQU     'X'
FN_MISS EQU     'O'
FN_HULL EQU     '#'
FN_CUR  EQU     '+'

; ---------------- N: device ----------------
; Verbatim from fujinet-battleship/arcadia/fujinet.inc; the wire format is the
; v2 binary one (?bin=1&v=2), shared with the astrocade and Intellivision
; ports.
NETDEV  EQU     $71             ; N1:
NCOPEN  EQU     $4F             ; 'O'
NCCLOSE EQU     $43             ; 'C'
NCREAD  EQU     $52             ; 'R'
NCSTAT  EQU     $53             ; 'S'
NMHTTPG EQU     $0C             ; open mode: HTTP GET
NTRNONE EQU     0               ; translation: none

; NET_STATUS reply: 4 bytes in slice 0.
NSAVLO  EQU     0               ; bytes waiting, low
NSAVHI  EQU     1               ; ...high
NSCONN  EQU     2               ; connected flag
NSDEVST EQU     3               ; nDevStatus_t; 1 = SUCCESS

; ---------------- the Battleship wire format ----------------
; Header, 38 bytes, always present.
GOPLCNT EQU     0               ; playerCount, max 4
GOPRMPT EQU     1               ; prompt[33]
GOSTAT  EQU     34              ; ST*
GOPLST  EQU     35              ; YOUR player status
GOACTPL EQU     36              ; int8; $FF nobody, else the active player.
                                ;   At gameover (v=2) the WINNER's index.
GOMVTIM EQU     37              ; seconds left, 0-250
GHDRLEN EQU     38

; Lobby body (status == STLOBBY).
GLSRVNM EQU     38              ; serverName[21]
GLPLYRS EQU     59              ; playerCount x LBSTRID
LBSTRID EQU     10
LBNAME  EQU     0               ; name[9]
LBSTAT  EQU     9

; In-play body (status != STLOBBY).
GILASTP EQU     38              ; lastAttackPos, 0-99
GIMYSHP EQU     39              ; myShips[10]: [0-4] yours, [5-9] the winner's
GIPLYRS EQU     49              ; playerCount x PLSTRID, ONLY once the
                                ;   gamefields exist (status >= STSTART)
PLSTRID EQU     115
PLNAME  EQU     0               ; name[9]
PLSTAT  EQU     9
PLFIELD EQU     10              ; gamefield[100]: y*10+x
PLSHIPS EQU     110             ; shipsLeft[5]: 1 afloat, 0 sunk

; During STPLACE the player records are name+status ONLY -- 10 bytes, not 115.
; Never index a gamefield in the place phase.
PPSTRID EQU     10

; /tables
TBSTRID EQU     36
TBID    EQU     0               ; table id[9]
TBNAME  EQU     9               ; name[21]
TBPLYRS EQU     30              ; players[6], literal "cur / max"

GMAXLEN EQU     509             ; GIPLYRS + 4*PLSTRID -- fits slice 0 whole,
                                ;   which is why the reply window is 512
GMINLEN EQU     GHDRLEN
TMAXLEN EQU     361

; Game status.
STLOBBY EQU     0
STPLACE EQU     1
STSTART EQU     10
STMISS  EQU     11
STHIT   EQU     12
STSUNK  EQU     13
STOVER  EQU     99

; Player status.
PSPLAY  EQU     0
PSDEFT  EQU     1               ; defeated
PSVIEW  EQU     2               ; spectator
PSREADY EQU     3
PSPLACE EQU     10

BRDDIM  EQU     10
BRDCELL EQU     100
NSHIPS  EQU     5
