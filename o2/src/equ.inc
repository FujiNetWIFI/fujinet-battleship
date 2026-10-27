; equ.inc -- constants: the console, the cartridge, the screen, the server.

; ---- P1 (all active low but COPY and LUM) ----
; Main code runs with external RAM and the cartridge on the bus; the interrupts
; switch to the VDC and put P1 back. COPY low always, or the cart's write
; strobe is gated off. The bank bits are left high: APPBANK pages are picked
; through the mailbox, not by P10/P11.
P1_MAIN equ 0AFh                ; XRAM + cart selected, VDC parked
P1_VDC  equ 0B7h                ; VDC selected
P1_JOY  equ 0BBh                ; keyboard scanner (joystick) selected

; ---- the mailbox (fuji_mailbox.h, protocol 2) ----
FN_W_REGSEL     equ 0E0h
FN_W_REGDATA    equ 0E1h
FN_W_DATA       equ 0E2h
FN_W_SEQ        equ 0E3h
FN_REG_DEVICE   equ 000h
FN_REG_CMD      equ 001h
FN_REG_NPARAM   equ 002h
FN_REG_DATA_RST equ 005h
FN_REG_CODEBANK equ 008h
FN_REG_POKE_LO  equ 00Ah
FN_REG_POKE_HI  equ 00Bh
FN_REG_POKE     equ 00Ch
FN_REG_RXWIN    equ 00Dh
FN_R_ACKSEQ     equ 0F20h
FN_R_ERR        equ 0F22h
FN_R_REPLY_CMD  equ 0F23h
FN_R_RXLEN_LO   equ 0F24h
FN_R_RXLEN_HI   equ 0F25h
FN_R_MAGIC0     equ 0F29h
FN_R_MAGIC1     equ 0F2Ah
FN_R_PROTO_VER  equ 0F2Bh
FN_R_DATA       equ 0F30h       ; the read window: 208 bytes from RXWIN*16
FN_PROTO_VER    equ 2
O2_APP_HDR      equ 0F30h
O2_APP_VERSION  equ 1

FUJI_DEVICEID   equ 070h
FC_ACK          equ 006h
FC_AKOPEN       equ 0DCh        ; OPEN_APPKEY
FC_AKREAD       equ 0DDh        ; READ_APPKEY
FC_AKWRITE      equ 0DEh        ; WRITE_APPKEY
NET_DEVICEID    equ 071h
NC_OPEN         equ 04Fh
NC_CLOSE        equ 043h
NC_READ         equ 052h
NC_STATUS       equ 053h
NM_GET          equ 00Ch        ; open mode: HTTP GET
NS_EOF          equ 136         ; nDevStatus: end of file

; ---- the VDC's character generator (o2doc appendix C) ----
G_SPACE equ 12
G_DOT   equ 39
G_DASH  equ 40
G_TIMES equ 41
G_BLOCK equ 47
G_BALL  equ 49

; Character colours, as MAME's i8244 draws them (control byte bits 1-3)
C_DGREY  equ 0
C_RED    equ 1
C_GREEN  equ 2
C_TAN    equ 3
C_BLUE   equ 4
C_VIOLET equ 5
C_CYAN   equ 6
C_WHITE  equ 7

; The colour register ($A3): bits 5-3 the background, 2-0 the grid
BG_BLACK equ 000h
BG_BLUE  equ 008h               ; dark blue: the sea

; ---- the screen ----
; Characters are 7 rows of 2 lines. Every screen has text line T0 at the
; top, drawn in the vblank. A text screen has T1-T5 below it, each re-pointed
; by a band after the line above has ended. A board screen has ten rows of
; ten cells -- singles 0-9, re-pointed by a band after each row -- and T1
; under them.
;
; A cell shows only rows 3-6 of its glyph (fixed.inc row_write): pointer =
; glyph*8 + 3 - Y/2, so the VDC stops at the end of the glyph's 8-byte slot
; after 4 rows. 8 lines of cell, and the rest of the pitch is the gap the
; next row is written in.
X_TEXT  equ 16                  ; 16 characters at 8 px, centred
TXT_LEN equ 16
TXT_TAB equ 1+2*TXT_LEN         ; a text table: Y, then 16 x (pointer, control)
Y_T0    equ 16                  ; MAME hides characters above Y=14 (NTSC)
TXT_PITCH equ 36                ; a text screen's lines: Y_T0 + 36n
NCOLS   equ 10
ROW_TAB equ 1+2*NCOLS           ; a board row: Y, then 10 x (pointer, control)
X_BOARD equ 40                  ; the board's left edge: 80 px, centred
CELL_ROW0 equ 3                 ; the glyph row a cell starts at
CELL_H  equ 2*(7-CELL_ROW0)     ; its lines
    ifndef PITCH
PITCH   equ 18                  ; a board row's lines, cell and gap
    endif
    ifndef LEAD
LEAD    equ 1                   ; lines a band starts before its line ends
    endif
    ifndef ROWGAP
ROWGAP  equ 10                  ; lines from T0's end to row 0
    endif
    ifndef TGAP
TGAP    equ 18                  ; lines from row 9's end to T1
    endif
Y_ROW0  equ Y_T0+14+ROWGAP
Y_BT1   equ Y_ROW0+9*PITCH+CELL_H+TGAP  ; the board screen's T1

; ---- the stick (input.inc): JOY / JOYEV bits ----
J_UP    equ 01h
J_DOWN  equ 02h
J_LEFT  equ 04h
J_RIGHT equ 08h
J_FIRE  equ 10h

; ---- the keyboard (keys.inc): BIOS readkey codes, row x 8 + column ----
K_0     equ 00h                 ; 0-9 are 00h-09h
K_SPACE equ 0Ch
K_L     equ 0Eh
K_R     equ 13h
K_T     equ 14h
K_Z     equ 21h
K_YES   equ 2Ch
K_NO    equ 2Dh
K_CLR   equ 2Eh
K_ENT   equ 2Fh
K_NONE  equ 0FFh

; ---- sounds (snd.inc): beep ids; from SND_SOFT on they never cut another ----
SND_SELECT equ 0
SND_FIRE   equ 1
SND_HIT    equ 2
SND_MISS   equ 3
SND_SUNK   equ 4
SND_TURN   equ 5
SND_ERROR  equ 6
SND_WIN    equ 7
SND_LOSE   equ 8
SND_SOFT   equ 9
SND_TICK   equ 9
SND_MOVE   equ 10
