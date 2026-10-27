; p4.inc -- page 4: placing the fleet (status 1). The board screen: the
; ships kept in white, the one being placed in green.
;
; Each ship starts somewhere legal (beside the ones kept); the stick moves
; it, R turns it, the button keeps it, Z places the rest at random and is
; done. A move or a turn that would leave the board or cross a kept ship is
; refused with a buzz, so the fleet is always legal: the server would
; ignore one that isn't (src/gamelogic.c testShip). With all five kept it
; goes as place/a,b,c,d,e -- each pos + 100*dir.
;
; Once it's sent, or when we've placed already (a state with our status not
; PS_PLACE), the board shows our fleet and the server's prompt.

; Placement's scratch (XRAM, SCR0 beyond the row's ten cells).
SHIPI   equ SCR0+10             ; the ship a helper works on
SPLO    equ SCR0+13             ; draw_span's rows
SPHI    equ SCR0+14

    PAGE_HEAD
    db e_state&0FFh, e_tick&0FFh
e_state:
    jmp pl_state
e_tick:
    jmp pl_tick

; pl_state: a placement state. Clobbers A, r0-r7.
pl_state:
    mov r0,#LSTAT
    movx a,@r0
    xrl a,#STATUS_PLACE
    jz ps_old
    mov a,#STATUS_PLACE         ; a new screen
    movx @r0,a
    call scr_board
    mov r1,#FRAMES              ; the generator's seed: never 0
    mov a,@r1
    mov r0,#RNDS
    movx @r0,a
    inc r0
    orl a,#1
    movx @r0,a
    mov r0,#PSTAT
    movx a,@r0
    xrl a,#PS_PLACE
    jnz ps_wait
    clr a                       ; placing: ship 0, at random
    mov r0,#PSHIP
    movx @r0,a
    mov r0,#PLSENT
    movx @r0,a
    mov r0,#VIEW
    movx @r0,a
ps_first:
    call occ_build
    mov r0,#PSHIP
    movx a,@r0
    call place_random
    call draw_all
    jmp t0_ship
ps_old:
    mov r0,#VIEW
    movx a,@r0
    jnz ps_x                    ; waiting: nothing changes
    mov r0,#PLSENT
    movx a,@r0
    jz ps_x                     ; placing
    mov r0,#PSTAT               ; sent: taken, or not
    movx a,@r0
    xrl a,#PS_PLACE
    jnz ps_taken
    mov r0,#ACTSENT             ; the answer to ours still wants a fleet:
    movx a,@r0                  ; refused (it can't be, but): again
    xrl a,#ACT_PLACE
    jnz ps_x
    clr a
    mov r0,#PSHIP
    movx @r0,a
    mov r0,#PLSENT
    movx @r0,a
    mov a,#SND_ERROR
    FCALL beep
    jmp ps_first
ps_taken:
    mov r0,#VIEW
    mov a,#1
    movx @r0,a
    jmp say_wait
ps_wait:
    mov r0,#VIEW                ; placed already: our fleet from the state
    mov a,#1
    movx @r0,a
    mov r0,#PSHIP
    mov a,#5
    movx @r0,a
    mov r3,#0
    mov r2,#ST_SHIPS
    mov r1,#FLEET
    mov r4,#5
pw_l:
    call st_byte
    movx @r1,a
    inc r1
    inc r2
    djnz r4,pw_l
    call draw_all
    jmp say_wait
ps_x:
    ret

; pl_tick: the stick, the keys, the clock -- while a ship is being placed.
; Clobbers A, r0-r7.
pl_tick:
    mov r0,#VIEW
    movx a,@r0
    jnz pt_x
    mov r0,#PLSENT
    movx a,@r0
    jnz pt_x
    if AUTOPLAY
    jmp pt_auto
    endif
    call clock
    mov r0,#KEY
    movx a,@r0
    xrl a,#K_R
    jz pt_turn
    xrl a,#K_R!K_Z
    jz pt_auto
    mov r0,#JOYEV
    movx a,@r0
    jb4 pt_keep
    call joy_dir                ; A = a direction to step, or 0
    jz pt_x
    jmp step_ship
pt_turn:
    jmp turn_ship
pt_auto:
    jmp auto_rest
pt_keep:
    jmp keep_ship
pt_x:
    ret

; joy_dir: A = the stick's direction bits when it moves the ship: as it is
; pressed, then every 6 frames from 18 on while it's held. Clobbers A, r0,
; r1.
joy_dir:
    mov r0,#JOY
    movx a,@r0
    anl a,#0Fh
    jz jd_x
    mov r1,a
    mov r0,#JOYEV
    movx a,@r0
    anl a,#0Fh
    jz jd_held
    mov r0,#REP
    mov a,#18
    movx @r0,a
    mov a,r1
    ret
jd_held:
    mov r0,#REP
    movx a,@r0
    dec a
    movx @r0,a
    jnz jd_no
    mov a,#6
    movx @r0,a
    mov a,r1
    ret
jd_no:
    clr a
jd_x:
    ret

; clock: the turn's seconds, counted down here between states; T0 again
; each second. Clobbers A, r0-r7.
clock:
    mov r0,#SECF
    movx a,@r0
    dec a
    movx @r0,a
    jnz ck_x
    mov a,#60
    movx @r0,a
    mov r0,#MOVET
    movx a,@r0
    jz ck_x
    dec a
    movx @r0,a
    jmp t0_ship
ck_x:
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

; t0_ship: T0 = the ship being placed and the seconds left; T1 the keys.
; Clobbers A, r0-r7.
t0_ship:
    call lb_clear
    mov r0,#PSHIP
    movx a,@r0
    add a,#shipname&0FFh
    movp a,@a
    call lb_str
    mov r0,#MOVET
    movx a,@r0
    jz ts_say
    mov r1,#LINEBUF+14
    call lb_num2
ts_say:
    mov r4,#L_TOP
    call say_line
    mov r4,#L_BOT
    mov a,#s_keys&0FFh
    jmp say

; say_wait: T0 and T1 while the others place. Clobbers A, r0-r7.
say_wait:
    mov r4,#L_TOP
    mov a,#s_placed&0FFh
    call say
    mov r4,#L_BOT
    mov a,#s_wait&0FFh
    jmp say

shipname:
    db sn_0&0FFh, sn_1&0FFh, sn_2&0FFh, sn_3&0FFh, sn_4&0FFh
sn_0:   db "CARRIER 5",0
sn_1:   db "BATTLESHIP 4",0
sn_2:   db "CRUISER 3",0
sn_3:   db "SUBMARINE 3",0
sn_4:   db "DESTROYER 2",0
s_keys: db "R:TURN    Z:AUTO",0
s_sent: db "FLEET SENT",0
s_placed: db "FLEET PLACED",0
s_wait: db "WAITING FOR ALL",0

    if $ > 0B00h
    error "page $A has run out"
    endif

; ===========================================================================
    org 0B00h
BOARD   equ 1
    include "screen.inc"

; ---- placing at random, and the cells taken ----

; keep_ship: the ship being placed is kept; the next starts at random, or
; with all five the fleet goes. Clobbers A, r0-r7.
keep_ship:
    mov a,#SND_SELECT
    FCALL beep
    call cur_ship
    call occ_ship
    mov r0,#PSHIP
    movx a,@r0
    inc a
    movx @r0,a
    xrl a,#5
    jz send_fleet
    xrl a,#5
    call place_random
    call draw_all
    jmp t0_ship

; auto_rest: the ships not yet kept, at random, and the fleet goes.
; Clobbers A, r0-r7.
auto_rest:
    mov r0,#PSHIP
    movx a,@r0
ar_l:
    mov r0,#PSHIP
    movx @r0,a
    xrl a,#5
    jz ar_go
    xrl a,#5
    call place_random
    call cur_ship
    call occ_ship
    mov r0,#PSHIP
    movx a,@r0
    inc a
    jmp ar_l
ar_go:
    call draw_all
    ; fall into send_fleet

; send_fleet: place/a,b,c,d,e. Clobbers A, r0-r7.
send_fleet:
    mov r0,#ACT
    mov a,#ACT_PLACE
    movx @r0,a
    mov r0,#PLSENT
    mov a,#1
    movx @r0,a
    mov r4,#L_TOP
    mov a,#s_sent&0FFh
    call say
    call lb_clear
    mov r4,#L_BOT
    jmp say_line

; place_random: ship A somewhere legal, at random. Clobbers A, r0-r7.
place_random:
    mov r0,#SHIPI
    movx @r0,a
pr_l:
    mov r0,#SHIPI
    movx a,@r0
    call ship_len
    mov r5,a
    call rnd                    ; across or down
    anl a,#1
    mov r4,a
    mov a,r5                    ; 11 - length starts on its long axis
    cpl a
    add a,#NCOLS+2
    mov r6,a
    mov r7,#NCOLS
    mov a,r4
    jnz pr_x
    mov a,r6
    mov r7,a
pr_x:
    call rnd_mod
    mov r2,a
    mov r7,#NCOLS
    mov a,r4
    jz pr_y
    mov a,r5
    cpl a
    add a,#NCOLS+2
    mov r7,a
pr_y:
    call rnd_mod
    mov r3,a
    call fits
    jnz pr_l
    jmp ship_enc

; occ_build: OCC = the cells of the ships kept. Clobbers A, r0-r7.
occ_build:
    mov r0,#OCC
    mov r2,#13
    clr a
ob_c:
    movx @r0,a
    inc r0
    djnz r2,ob_c
ob_l:
    mov r0,#SHIPI               ; (A = the ship)
    movx @r0,a
    mov r6,a
    mov r0,#PSHIP
    movx a,@r0
    xrl a,r6
    jz ob_x
    mov a,r6
    call ship_dec
    call occ_ship
    mov r0,#SHIPI
    movx a,@r0
    inc a
    jmp ob_l
ob_x:
    ret

    if $ > 0C00h
    error "page $B has run out"
    endif

; ===========================================================================
    org 0C00h
    include "board.inc"

; cur_ship: r2-r5 = the ship being placed (SHIPI = it), SPLO-SPHI its rows.
; Clobbers A, r0, r1.
cur_ship:
    mov r0,#PSHIP
    movx a,@r0
    mov r0,#SHIPI
    movx @r0,a
    call ship_dec
    mov r0,#SPLO
    mov a,r3
    movx @r0,a
    call ship_hi
    mov r0,#SPHI
    movx @r0,a
    ret

; span_add: SPLO-SPHI widened to take in the rows of the ship at r3 = y,
; r4 = dir, r5 = length. Clobbers A, r0, r1.
span_add:
    mov r0,#SPLO
    movx a,@r0
    mov r1,a
    mov a,r3                    ; lo > y: lo = y
    cpl a
    add a,r1
    jnc sa_hi
    mov a,r3
    movx @r0,a
sa_hi:
    call ship_hi
    mov r1,a
    mov r0,#SPHI                ; its last row > hi: hi = it
    movx a,@r0
    cpl a
    add a,r1
    jnc sa_x
    mov a,r1
    movx @r0,a
sa_x:
    ret

; ship_hi: A = the last row of the ship at r3 = y, r4 = dir, r5 = length.
ship_hi:
    mov a,r4
    jz sh_y
    mov a,r5
    dec a
    add a,r3
    ret
sh_y:
    mov a,r3
    ret

; lb_num2: A (0-99) as two digits into LINEBUF at r1 (a leading space).
; Clobbers A, r0, r1.
lb_num2:
    mov r0,#'0'-1
ln_l:
    inc r0
    add a,#256-10
    jc ln_l
    add a,#10+'0'
    xch a,r0                    ; A = tens, r0 = units
    xrl a,#'0'
    jz ln_sp
    xrl a,#'0'
    jmp ln_put
ln_sp:
    mov a,#' '
ln_put:
    movx @r1,a
    inc r1
    mov a,r0
    movx @r1,a
    ret

; ===========================================================================
    org 0D00h
    include "rx.inc"


; draw_all: the whole board. draw_span: rows SPLO-SPHI. Clobbers A, r0-r7.
draw_all:
    clr a
    mov r0,#SPLO
    movx @r0,a
    mov a,#NCOLS-1
    mov r0,#SPHI
    movx @r0,a
draw_span:
    mov r0,#SPLO
    movx a,@r0
dsp_l:
    mov r0,#ROWN
    movx @r0,a
    call build_row
    mov r0,#ROWN
    movx a,@r0
    mov r4,a
    call row_draw
    mov r0,#ROWN
    movx a,@r0
    mov r1,a                    ; the row: the last?
    mov r0,#SPHI
    movx a,@r0
    xrl a,r1
    jz dsp_x
    mov a,r1
    inc a
    jmp dsp_l
dsp_x:
    ret

; build_row: SCR0 = row ROWN's ten cells: water, the kept ships, the one
; being placed. Clobbers A, r0-r7.
build_row:
    mov r0,#SCR0
    mov r2,#NCOLS
br_w:
    mov a,#CL_WATER
    movx @r0,a
    inc r0
    djnz r2,br_w
    clr a
br_l:
    mov r0,#SHIPJ
    movx @r0,a
    mov r6,a
    mov r0,#PSHIP               ; ships 0..PSHIP: past it, or past 4, done
    movx a,@r0
    cpl a
    add a,r6                    ; j - PSHIP - 1: no borrow = past it
    jc br_x
    mov a,r6
    xrl a,#5
    jz br_x
    mov a,r6
    call ship_dec
    mov r0,#SHIPJ               ; its code: kept, or being placed
    movx a,@r0
    mov r6,a
    mov r0,#PSHIP
    movx a,@r0
    xrl a,r6
    mov r7,#CL_PLACE
    jz br_mark
    mov r7,#CL_SHIP
br_mark:
    call mark_row
    mov r0,#SHIPJ
    movx a,@r0
    inc a
    jmp br_l
br_x:
    ret

; occ_ship: the cells of the ship at r2-r5 taken. Clobbers A, r0, r1, r6,
; r7.
occ_ship:
    mov a,r3
    rl a
    mov r6,a
    rl a
    rl a
    add a,r6
    add a,r2
    mov r6,a
    mov a,r5
    mov r7,a
oc_l:
    mov a,r6
    call occ_set
    mov a,r4
    jz oc_1
    mov a,r6
    add a,#NCOLS
    jmp oc_s
oc_1:
    mov a,r6
    inc a
oc_s:
    mov r6,a
    djnz r7,oc_l
    ret

; ---- the fleet's moves ----

; step_ship: the ship being placed one cell in the direction A (J_*), if it
; fits there. Clobbers A, r0-r7.
step_ship:
    mov r0,#TMP0
    movx @r0,a
    call cur_ship               ; r2-r5, and its rows as the span
    mov r0,#TMP0
    movx a,@r0
    jb0 ss_up
    jb1 ss_down
    jb2 ss_left
    inc r2
    jmp ss_try
ss_up:
    dec r3
    jmp ss_try
ss_down:
    inc r3
    jmp ss_try
ss_left:
    dec r2
ss_try:
    call fits
    jnz refuse
    mov a,#SND_MOVE
moved:
    FCALL beep
    call ship_enc
    call span_add
    jmp draw_span

; turn_ship: the ship being placed turned about its first cell -- pulled
; back onto the board if its far end leaves it -- if it fits. Clobbers A,
; r0-r7.
turn_ship:
    call cur_ship
    mov a,r4
    xrl a,#1
    mov r4,a
    mov a,r5                    ; the furthest it can start: 10 - length
    cpl a
    add a,#NCOLS+1
    mov r6,a
    mov a,r4
    jz tu_h
    mov a,r6                    ; down: y at most that
    cpl a
    add a,r3
    jnc tu_try
    mov a,r6
    mov r3,a
    jmp tu_try
tu_h:
    mov a,r6                    ; across: x at most that
    cpl a
    add a,r2
    jnc tu_try
    mov a,r6
    mov r2,a
tu_try:
    call fits
    jnz refuse
    mov a,#SND_SELECT
    jmp moved

refuse:
    mov a,#SND_ERROR
    FCALL beep
    ret

    if $ > 0E00h
    error "page $D has run out"
    endif

; ===========================================================================
    org 0E00h

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

; ship_enc: ship SHIPI = r2 = x, r3 = y, r4 = dir. Clobbers A, r0, r1.
ship_enc:
    mov a,r3                    ; 10y + x
    rl a
    mov r0,a
    rl a
    rl a
    add a,r0
    add a,r2
    mov r0,a
    mov a,r4
    jz se_h
    mov a,r0
    add a,#100
    mov r0,a
se_h:
    mov r1,#SHIPI
    movx a,@r1
    add a,#FLEET
    mov r1,a
    mov a,r0
    movx @r1,a
    ret

; fits: A = 0 if the ship at r2 = x, r3 = y, r4 = dir, r5 = length is on
; the board and crosses no kept ship (OCC). Clobbers A, r0, r1, r6, r7.
fits:
    mov a,r2                    ; x, y in 0-9 (a step off the edge wraps
    add a,#256-NCOLS            ; to 255)
    jc ft_bad
    mov a,r3
    add a,#256-NCOLS
    jc ft_bad
    mov a,r4                    ; its far end: x or y + length <= 10
    jz ft_h
    mov a,r3
    jmp ft_b
ft_h:
    mov a,r2
ft_b:
    add a,r5
    add a,#256-NCOLS-1
    jc ft_bad
    mov a,r3                    ; its cells, from 10y + x
    rl a
    mov r6,a
    rl a
    rl a
    add a,r6
    add a,r2
    mov r6,a
    mov a,r5
    mov r7,a
ft_l:
    mov a,r6
    call occ_test
    jnz ft_bad
    mov a,r4
    jz ft_s1
    mov a,r6
    add a,#NCOLS
    jmp ft_s
ft_s1:
    mov a,r6
    inc a
ft_s:
    mov r6,a
    djnz r7,ft_l
    clr a
    ret
ft_bad:
    mov a,#1
    ret

; occ_test: A = nonzero if cell A is taken. occ_set: take it. Clobbers A,
; r0, r1.
occ_test:
    call occ_at
    movx a,@r0
    anl a,r1
    ret
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
    add a,#bitm&0FFh
    movp a,@a
    mov r1,a
    ret
bitm:
    db 1, 2, 4, 8, 16, 32, 64, 128

; ship_len: A = ship A's length.
ship_len:
    add a,#shiplen&0FFh
    movp a,@a
    ret
shiplen:
    db 5, 4, 3, 3, 2

; rnd: A = a random byte: eight steps of a 16-bit Galois LFSR. Clobbers r0,
; r6.
rnd:
    mov r6,#8
rn_l:
    mov r0,#RNDS+1
    movx a,@r0
    clr c
    rrc a
    movx @r0,a
    mov r0,#RNDS
    movx a,@r0
    rrc a
    movx @r0,a
    jnc rn_n
    mov r0,#RNDS+1
    movx a,@r0
    xrl a,#0B4h
    movx @r0,a
rn_n:
    djnz r6,rn_l
    mov r0,#RNDS
    movx a,@r0
    ret

; rnd_mod: A = a random number below r7 (1-255). Clobbers r0, r6.
rnd_mod:
    call rnd
rm_l:
    mov r0,a
    mov a,r7
    cpl a
    inc a
    add a,r0
    jc rm_l
    mov a,r0
    ret

    if $ > 0F00h
    error "page $E has run out"
    endif

    PAGE_TAIL
