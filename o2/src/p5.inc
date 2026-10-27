; p5.inc -- page 5: the battle (status 10-13). The board screen: one board
; at a time, T0 whose turn it is and the seconds left, T1 each seat's name
; and ships afloat in its colour.
;
; The boards (VIEW):
;   V_TARGET  ours to fire at. A shot hits every live opponent at once, so
;             one board stands for them all: red where any has a hit, a
;             white dot where all have it marked (no use firing there), a
;             dark dot where only some have
;   V_FLEET   our own: the ships, and their shots
;   1-3       an opponent's field, its water in its colour
; Our turn shows the target, another's our fleet; keys 0-3 and T choose,
; until the turn changes.
;
; The reply is the cart's latest, and the next request replaces it: all
; that a tick needs (the cells we may fire at, our ships, the seats) is
; taken at state time, here. The ticks -- the keys, the cursor, the shot,
; the clock -- are page 7's (p7.inc): one page doesn't hold both.

    PAGE_HEAD
    db e_state&0FFh, e_tick&0FFh
e_state:
    jmp pv_state
e_tick:                         ; (the battle's ticks are page 7's)
    ret

; pv_state: a state of the battle. Clobbers A, r0-r7.
pv_state:
    mov r0,#LSTAT               ; the screen up already: LSTAT in 10-13
    movx a,@r0
    mov r6,a
    mov r0,#STAT
    movx a,@r0
    mov r0,#LSTAT
    movx @r0,a
    mov a,r6
    add a,#256-STATUS_START
    add a,#256-4
    jnc pv_old
    call scr_board              ; a new screen
    call cur_shape
    mov a,#0FFh
    mov r0,#VIEWK
    movx @r0,a
    mov r0,#VIEW
    movx @r0,a
    mov r0,#LRES
    movx @r0,a
    mov r0,#LACTV
    movx @r0,a
    mov a,#4
    mov r0,#CURX
    movx @r0,a
    mov r0,#CURY
    movx @r0,a
    clr a
    mov r0,#FIRED
    movx @r0,a
    mov r0,#DRAWN
    movx @r0,a
    mov r0,#AUTO
    movx @r0,a
pv_old:
    mov r0,#STAT                ; anything new: a shot, the turn passed
    movx a,@r0
    mov r2,a
    mov r0,#LRES
    movx a,@r0
    xrl a,r2
    jnz pv_new
    mov r0,#LASTAT
    movx a,@r0
    mov r2,a
    mov r0,#LPOS
    movx a,@r0
    xrl a,r2
    jnz pv_new
    mov r0,#ACTV
    movx a,@r0
    mov r2,a
    mov r0,#LACTV
    movx a,@r0
    xrl a,r2
    jz pv_view
pv_new:
    call cues
    call seats
    call t1_seats
    call t0_turn
    clr a
    mov r0,#DRAWN
    movx @r0,a
pv_view:
    mov r0,#VIEWK               ; the board: a key's, or the turn's
    movx a,@r0
    inc a
    jnz pv_key
    mov r0,#ACTV
    movx a,@r0
    mov r2,#V_TARGET
    jz pv_set
    mov r2,#V_FLEET
    jmp pv_set
pv_key:
    dec a
    mov r2,a
pv_set:
    mov r0,#VIEW
    movx a,@r0
    xrl a,r2
    jnz pv_draw
    mov r0,#DRAWN
    movx a,@r0
    jz pv_draw
    ret
pv_draw:
    mov r0,#VIEW
    mov a,r2
    movx @r0,a
    mov r0,#DRAWN
    mov a,#1
    movx @r0,a
    jmp draw_view

; cues: the sound of what changed -- a shot's result (status 11-13, new or
; at a new cell), else our turn come -- and the signature kept: a new turn
; forgets the key's board and our shot. Clobbers A, r0-r3.
cues:
    mov r0,#STAT
    movx a,@r0
    mov r3,a
    add a,#256-STATUS_MISS      ; 11-13: 0-2
    mov r2,a
    add a,#256-3
    jc cu_turn
    mov r0,#LRES
    movx a,@r0
    xrl a,r3
    jnz cu_shot
    mov r0,#LASTAT
    movx a,@r0
    mov r1,a
    mov r0,#LPOS
    movx a,@r0
    xrl a,r1
    jz cu_turn
cu_shot:
    mov a,r2
    add a,#SND_MISS
    jmp cu_beep
cu_turn:
    mov r0,#ACTV
    movx a,@r0
    jnz cu_keep
    mov r0,#LACTV
    movx a,@r0
    jz cu_keep
    mov a,#SND_TURN
cu_beep:
    FCALL beep
cu_keep:
    mov r0,#STAT
    movx a,@r0
    mov r0,#LRES
    movx @r0,a
    mov r0,#LASTAT
    movx a,@r0
    mov r0,#LPOS
    movx @r0,a
    mov r0,#ACTV
    movx a,@r0
    mov r2,a
    mov r0,#LACTV
    movx a,@r0
    xrl a,r2
    jz cu_x
    mov a,r2
    movx @r0,a
    clr a                       ; a new turn
    mov r0,#FIRED
    movx @r0,a
    mov r0,#AUTO
    movx @r0,a
    mov r0,#VIEWK
    mov a,#0FFh
    movx @r0,a
cu_x:
    ret

; mark_ships: our five ships (FLEET) onto SCR0 where they cross row ROWN.
; Clobbers A, r0-r7.
mark_ships:
    clr a
ms_l:
    mov r0,#SHIPJ
    movx @r0,a
    call ship_dec
    mov r7,#CL_SHIP
    call mark_row
    mov r0,#SHIPJ
    movx a,@r0
    inc a
    xrl a,#5
    jz ms_x
    xrl a,#5
    jmp ms_l
ms_x:
    ret

    if $ > 900h
    error "page $8 has run out"
    endif

; ===========================================================================
    org 900h
    include "disp.inc"

; ===========================================================================
    org 0A00h
    include "text.inc"

; t0_turn: T0 = whose turn, in their colour, and the seconds left (the
; clock redraws it from LINEBUF: T0 is the last line written). Clobbers A,
; r0-r7.
t0_turn:
    call lb_clear
    mov r0,#PSTAT
    movx a,@r0
    xrl a,#PS_DEFEAT
    jnz tt_alive
    mov a,#s_sunk&0FFh
    call lb_str
    clr a
    jmp tt_say
tt_alive:
    mov r0,#ACTV
    movx a,@r0
    mov r5,a
    jnz tt_other
    mov a,#s_yours&0FFh
    call lb_str
    jmp tt_secs
tt_other:
    inc a
    jz tt_say                   ; nobody's
    mov a,r5                    ; their name, then AIMS
    call seat_name
    mov r0,#LINEBUF+9
    mov a,#s_aims&0FFh
    call lb_at
tt_secs:
    mov r0,#MOVET
    movx a,@r0
    jz tt_col
    mov r1,#LINEBUF+13
    call lb_num3
tt_col:
    mov a,r5
tt_say:
    anl a,#3
    call seat_col
    mov r0,#TXCOL
    movx @r0,a
    inc r0
    movx @r0,a
    mov r4,#L_TOP
    jmp say_line_c

; t1_seats: T1 = each seat's name (3) and ships afloat, in its colour.
; Clobbers A, r0-r7.
t1_seats:
    call lb_clear
    clr a
t1_l:
    mov r0,#SEATK
    movx @r0,a
    mov r5,a
    mov r0,#NSEAT
    movx a,@r0
    xrl a,r5
    jz t1_say
    mov a,r5                    ; LINEBUF + 4k: the name's first 3
    rl a
    rl a
    add a,#LINEBUF
    mov r1,a
    mov r4,#3
    mov a,r5
    call seat_nm
    mov a,r5
    add a,#SUNKM
    mov r0,a
    movx a,@r0
    add a,#'0'
    movx @r1,a
    mov r0,#SEATK
    movx a,@r0
    inc a
    xrl a,#4
    jz t1_say
    xrl a,#4
    jmp t1_l
t1_say:
    mov r0,#TXCOL
    mov a,#80h                  ; each four columns in their seat's colour
    movx @r0,a
    inc r0
    movx @r0,a
    mov r4,#L_BOT
    jmp say_line_c

; seat_name: seat A's name (8) into LINEBUF; seat_nm: r4 of its characters
; into LINEBUF at r1 (a NUL as a space). Clobbers A, r0-r3, r6.
seat_name:
    mov r1,#LINEBUF
    mov r4,#8
seat_nm:
    add a,#seatn&0FFh
    mov r0,a
    movp a,@a
    mov r2,a
    mov a,r0
    add a,#4
    movp a,@a
    mov r3,a
sn_l:
    call st_byte
    jnz sn_1
    mov a,#' '
sn_1:
    movx @r1,a
    inc r1
    inc r2
    djnz r4,sn_l
    ret
seatn:                          ; each seat's record: low bytes, high bytes
    db (ST_SEATS)&0FFh, (ST_SEATS+SEAT_PLAY)&0FFh
    db (ST_SEATS+2*SEAT_PLAY)&0FFh, (ST_SEATS+3*SEAT_PLAY)&0FFh
    db (ST_SEATS)>>8, (ST_SEATS+SEAT_PLAY)>>8
    db (ST_SEATS+2*SEAT_PLAY)>>8, (ST_SEATS+3*SEAT_PLAY)>>8

s_yours: db "YOUR SHOT",0
s_aims:  db "AIMS",0
s_sunk:  db "YOU ARE SUNK",0

    if $ > 0B00h
    error "page $A has run out"
    endif

; ===========================================================================
    org 0B00h
BOARD   equ 1
    include "screen.inc"

; seats: from the state -- LIVE (bit k: seat k not defeated), SUNKM (each
; seat's ships afloat), FLEET (ours), and on our turn OCC: the cells some
; live opponent has unmarked, the ones we may fire at. Clobbers A, r0-r7.
seats:
    mov r2,#0                   ; our fleet: window 0
    call rx_winr
    mov r1,#FLEET
    mov r2,#ST_SHIPS
fl_l:
    mov a,r2
    call rx_get
    movx @r1,a
    inc r1
    inc r2
    mov a,r2
    xrl a,#ST_SHIPS+5
    jnz fl_l
    clr a
    mov r0,#LIVE
    movx @r0,a
se_l:
    mov r0,#SEATK               ; seat k
    movx @r0,a
    mov r5,a
    mov r0,#NSEAT
    movx a,@r0
    xrl a,r5
    jz se_fire
    mov a,r5
    call seat_win               ; r3 = its field's offset in the window
    mov a,r3                    ; its status
    dec a
    call rx_get
    xrl a,#PS_DEFEAT
    jz se_dead
    mov a,r5                    ; alive
    add a,#bitm&0FFh
    movp a,@a
    mov r2,a
    mov r0,#LIVE
    movx a,@r0
    orl a,r2
    movx @r0,a
se_dead:
    mov r4,#5                   ; its ships afloat
    mov r6,#0
    mov a,r3
    add a,#SE_LEFT-SE_FIELD
    mov r3,a
se_left:
    mov a,r3
    call rx_get
    jz se_s
    inc r6
se_s:
    inc r3
    djnz r4,se_left
    mov a,r5
    add a,#SUNKM
    mov r0,a
    mov a,r6
    movx @r0,a
    mov a,r5
    inc a
    jmp se_l
se_fire:
    mov r0,#ACTV
    movx a,@r0
    jnz se_x
    jmp fire_map
se_x:
    ret

; seat_win: the reply's window at seat A's field (every seat's 100 cells
; sit in one), r3 = the field's offset in it. Clobbers A, r0, r2.
seat_win:
    mov r0,a
    add a,#seato&0FFh
    movp a,@a
    mov r3,a
    mov a,r0
    add a,#seatw&0FFh
    movp a,@a
    mov r2,a
    jmp rx_winr
SEATF   set ST_SEATS+SE_FIELD
seatw:  db SEATF>>4, (SEATF+SEAT_PLAY)>>4, (SEATF+2*SEAT_PLAY)>>4, (SEATF+3*SEAT_PLAY)>>4
seato:  db SEATF&15, (SEATF+SEAT_PLAY)&15, (SEATF+2*SEAT_PLAY)&15, (SEATF+3*SEAT_PLAY)&15
bitm:   db 1, 2, 4, 8, 16, 32, 64, 128

; cur_shape: sprite 0's shape, the cursor: a box over a cell's four rows.
; Clobbers A, r0, r1.
cur_shape:
    mov a,#P1_VDC
    outl p1,a
    mov r0,#vdc_spr0_shape
    mov r1,#cur_bits&0FFh
cs_l:
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r1
    mov a,r1
    xrl a,#cur_end&0FFh
    jnz cs_l
    mov a,#P1_MAIN
    outl p1,a
    ret
cur_bits:
    db 0FFh, 081h, 081h, 0FFh, 0, 0, 0, 0
cur_end:

    if $ > 0C00h
    error "page $B has run out"
    endif

; ===========================================================================
    org 0C00h
CURSOR  equ 1
    include "board.inc"

; lb_num3: A (0-255) into LINEBUF at r1, three columns, no leading zeros.
; Clobbers A, r0-r4.
lb_num3:
    mov r3,#0                   ; a digit has been written
    mov r2,#256-100
    call ln_d
    mov r2,#256-10
    call ln_d
    mov r3,#1                   ; the units always
    mov r2,#256-1
ln_d:                           ; the digit of A for the power -r2; A = the rest
    mov r0,#'0'
ln_l:
    mov r4,a
    add a,r2
    jnc ln_e
    inc r0
    jmp ln_l
ln_e:
    mov a,r0
    xrl a,#'0'
    orl a,r3
    jz ln_sp
    mov r3,#1
    mov a,r0
    jmp ln_out
ln_sp:
    mov a,#' '
ln_out:
    movx @r1,a
    inc r1
    mov a,r4
    ret

    if $ > 0D00h
    error "page $C has run out"
    endif

; ===========================================================================
    org 0D00h
    include "rx.inc"

; draw_view: the board VIEW, every row. Clobbers A, r0-r7.
draw_view:
    clr a
dv_l:
    mov r0,#ROWN
    movx @r0,a
    call build_row
    mov r0,#ROWN
    movx a,@r0
    mov r4,a
    call row_draw
    mov r0,#ROWN
    movx a,@r0
    inc a
    xrl a,#NCOLS
    jz dv_x
    xrl a,#NCOLS
    jmp dv_l
dv_x:
    ret

; build_row: SCR0 = row ROWN of the board VIEW. Clobbers A, r0-r7.
build_row:
    mov r0,#VIEW
    movx a,@r0
    xrl a,#V_TARGET
    jz br_tgt
    xrl a,#V_TARGET             ; a seat's: its sea, our ships, its field
    mov r5,a
    add a,#CL_SEA
    call row_fill
    mov a,r5
    jnz br_fld
    call mark_ships
    mov r5,#V_FLEET
    mov a,r5
br_fld:
    call row_at_seat            ; r3 = the row's first cell in the window
    mov r6,#NCOLS
    mov r1,#SCR0
bf_l:
    mov a,r3
    call rx_get
    jz bf_n
    xrl a,#F_HIT
    jnz bf_miss
    mov a,r5                    ; a hit: on ours a red block, theirs a ball
    jz bf_ours
    mov a,#CL_HIT
    jmp bf_put
bf_ours:
    mov a,#CL_SHOT
    jmp bf_put
bf_miss:
    mov a,#CL_MISS
bf_put:
    movx @r1,a
bf_n:
    inc r1
    inc r3
    djnz r6,bf_l
    ret
br_tgt:
    clr a                       ; the target: per cell, 80h a hit, + 1 for
    call row_fill               ; each opponent that has it marked
    mov r0,#NLIVE
    movx @r0,a
    mov a,#1
bt_l:
    mov r0,#SEATK
    movx @r0,a
    mov r5,a
    mov r0,#NSEAT
    movx a,@r0
    xrl a,r5
    jz bt_conv
    mov a,r5
    add a,#bitd&0FFh
    movp a,@a
    mov r2,a
    mov r0,#LIVE
    movx a,@r0
    anl a,r2
    jz bt_n
    mov r0,#NLIVE
    movx a,@r0
    inc a
    movx @r0,a
    mov a,r5
    call row_at_seat
    mov r6,#NCOLS
    mov r1,#SCR0
bt_c:
    mov a,r3
    call rx_get
    jz bt_cn
    xrl a,#F_HIT
    mov r2,a                    ; 0: a hit
    movx a,@r1
    inc a
    xch a,r2
    jnz bt_cs
    mov a,r2
    orl a,#80h
    mov r2,a
bt_cs:
    mov a,r2
    movx @r1,a
bt_cn:
    inc r1
    inc r3
    djnz r6,bt_c
bt_n:
    mov r0,#SEATK
    movx a,@r0
    inc a
    jmp bt_l
bt_conv:
    mov r0,#NLIVE
    movx a,@r0
    mov r5,a
    mov r1,#SCR0
    mov r6,#NCOLS
bc_l:
    movx a,@r1
    jb7 bc_hit
    jz bc_w
    xrl a,r5
    jz bc_all
    mov a,#CL_PART
    jmp bc_put
bc_hit:
    mov a,#CL_HIT
    jmp bc_put
bc_all:
    mov a,#CL_MISS
    jmp bc_put
bc_w:
    mov a,#CL_WATER
bc_put:
    movx @r1,a
    inc r1
    djnz r6,bc_l
    ret

; row_fill: SCR0's ten cells = A. Clobbers r0, r2.
row_fill:
    mov r0,#SCR0
    mov r2,#NCOLS
rf_l:
    movx @r0,a
    inc r0
    djnz r2,rf_l
    ret

; row_at_seat: seat A's field in the window, r3 = row ROWN's first cell.
; Clobbers A, r0, r2.
row_at_seat:
    call seat_win
    mov r0,#ROWN
    movx a,@r0
    mov r2,a
    rl a
    rl a
    add a,r2
    rl a                        ; 10 * row
    add a,r3
    mov r3,a
    ret

bitd:   db 1, 2, 4, 8               ; (build_row's: a seat's bit in LIVE)

    if $ > 0E00h
    error "page $D has run out"
    endif

; ===========================================================================
    org 0E00h

; fire_map: OCC = the cells some live opponent has unmarked. Clobbers A,
; r0-r7.
fire_map:
    mov r0,#OCC
    mov r2,#13
    clr a
fm_c:
    movx @r0,a
    inc r0
    djnz r2,fm_c
    mov a,#1
fm_l:
    mov r0,#SEATK
    movx @r0,a
    mov r5,a
    mov r0,#NSEAT
    movx a,@r0
    xrl a,r5
    jz fm_x
    mov a,r5
    add a,#bitmx&0FFh
    movp a,@a
    mov r2,a
    mov r0,#LIVE
    movx a,@r0
    anl a,r2
    jz fm_n
    mov a,r5
    call seat_win
    mov r6,#0
fm_p:
    mov a,r3
    add a,r6
    call rx_get
    jnz fm_m
    mov a,r6
    call occ_set
fm_m:
    inc r6
    mov a,r6
    xrl a,#100
    jnz fm_p
fm_n:
    mov r0,#SEATK
    movx a,@r0
    inc a
    jmp fm_l
fm_x:
    ret

; occ_set: cell A set in OCC. Clobbers A, r0, r1.
occ_set:
    call occ_at
    movx a,@r0
    orl a,r1
    movx @r0,a
    ret
occ_at:                         ; r0 = its byte, r1 = its bit
    mov r0,a
    rr a
    rr a
    rr a
    anl a,#1Fh
    add a,#OCC
    xch a,r0
    anl a,#7
    add a,#bitmx&0FFh
    movp a,@a
    mov r1,a
    ret
bitmx:
    db 1, 2, 4, 8, 16, 32, 64, 128

; ship_dec: ship A (0-4) -> r2 = x, r3 = y, r4 = dir, r5 = its length.
; Clobbers A, r0.
ship_dec:
    mov r0,a
    add a,#shiplen&0FFh
    movp a,@a
    mov r5,a
    mov a,r0
    add a,#FLEET
    mov r0,a
    movx a,@r0
    mov r4,#1
    add a,#256-100
    jc sd_pos                   ; 100 on: down
    add a,#100
    mov r4,#0
sd_pos:
    mov r3,#0                   ; y = pos / 10, x = pos mod 10
sd_l:
    add a,#256-10
    jnc sd_x
    inc r3
    jmp sd_l
sd_x:
    add a,#10
    mov r2,a
    ret
shiplen:
    db 5, 4, 3, 3, 2

; mark_row: the ship at r2 = x, r3 = y, r4 = dir, r5 = length onto SCR0 as
; code r7, where it crosses row ROWN. Clobbers A, r0, r6.
mark_row:
    mov r0,#ROWN
    movx a,@r0
    mov r6,a                    ; the row
    mov a,r4
    jnz mr_v
    mov a,r3                    ; across: on this row, len cells from x
    xrl a,r6
    jnz mr_x
    mov a,r2
    add a,#SCR0
    mov r0,a
    mov a,r5
    mov r6,a
mr_h:
    mov a,r7
    movx @r0,a
    inc r0
    djnz r6,mr_h
mr_x:
    ret
mr_v:
    mov a,r6                    ; down: row - y in 0..len-1
    cpl a
    add a,r3                    ; y > row: above it
    jc mr_x
    mov a,r3
    cpl a
    inc a
    add a,r6
    mov r6,a
    mov a,r5
    cpl a
    inc a
    add a,r6
    jc mr_x                     ; below it
    mov a,r2
    add a,#SCR0
    mov r0,a
    mov a,r7
    movx @r0,a
    ret

    if $ > 0F00h
    error "page $E has run out"
    endif

    PAGE_TAIL
