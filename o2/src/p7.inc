; p7.inc -- page 7: the battle's ticks (page 5 is its states). Each
; frame: a key chooses a board, the clock counts the turn's seconds down on
; T0, and on our turn the cursor moves and the button fires -- at a cell
; page 5 found some live opponent has unmarked (OCC).

    PAGE_HEAD
    db e_state&0FFh, e_tick&0FFh
e_state:                        ; (the battle's states are page 5's)
    ret
e_tick:
    jmp pv_tick

; pv_tick: the keys choose a board; the clock; on our turn the cursor and
; the button. Clobbers A, r0-r7.
pv_tick:
    mov r0,#KEY
    movx a,@r0
    mov r5,a
    xrl a,#K_T
    jz pk_tgt
    mov a,r5                    ; 0-3: a seat's board, if it has one
    add a,#256-4
    jc pk_none
    mov r0,#NSEAT
    movx a,@r0
    mov r2,a
    mov a,r5
    cpl a
    add a,r2                    ; seats > k
    jnc pk_none
    mov a,r5
    jmp pk_set
pk_tgt:
    mov a,#V_TARGET
pk_set:
    mov r0,#VIEWK               ; drawn by the next state: the reply that
    movx @r0,a                  ; holds the fields may be gone by now
    mov a,#SND_SELECT
    FCALL beep
pk_none:
    call clock
    call my_shot                ; A = 0: our turn, a shot to take
    jnz pc_hide
    if AUTOPLAY
    jmp auto_shot
    endif
    mov r0,#VIEW
    movx a,@r0
    jz pc_hide                  ; (not on our own fleet)
    mov r0,#JOYEV
    movx a,@r0
    jb4 fire
    call joy_dir
    jz pc_show
    mov r5,a
    mov r0,#CURX
    movx a,@r0
    mov r2,a
    mov r0,#CURY
    movx a,@r0
    mov r3,a
    mov a,r5
    jb0 pm_up
    jb1 pm_down
    jb2 pm_left
    inc r2
    jmp pm_try
pm_up:
    dec r3
    jmp pm_try
pm_down:
    inc r3
    jmp pm_try
pm_left:
    dec r2
pm_try:
    mov a,r2                    ; on the board (a step off it wraps to 255)
    add a,#256-NCOLS
    jc pc_show
    mov a,r3
    add a,#256-NCOLS
    jc pc_show
    mov r0,#CURX
    mov a,r2
    movx @r0,a
    mov r0,#CURY
    mov a,r3
    movx @r0,a
    mov a,#SND_MOVE
    FCALL beep
pc_show:
    mov r1,#FRAMES              ; on 24 frames, off 8
    mov a,@r1
    anl a,#18h
    xrl a,#18h
    mov r2,a
    jmp pc_put
pc_hide:
    mov r2,#0
pc_put:
    mov r0,#CURX
    movx a,@r0
    mov r3,a
    mov r0,#CURY
    movx a,@r0
    mov r4,a
    jmp spr_cursor

; fire: at the cursor's cell, if some live opponent has it unmarked.
fire:
    call cur_cell
    call occ_test
    jnz fi_go
    mov a,#SND_ERROR
    FCALL beep
    jmp pc_show
fi_go:
    mov r0,#ACTARG
    mov a,r6
    movx @r0,a
    mov r0,#ACT
    mov a,#ACT_ATTACK
    movx @r0,a
    mov r0,#FIRED
    mov a,#1
    movx @r0,a
    mov a,#SND_FIRE
    FCALL beep
    jmp pc_hide

    if AUTOPLAY
; auto_shot: a second into our turn, the first cell we may fire at.
auto_shot:
    mov r0,#AUTO
    movx a,@r0
    inc a
    movx @r0,a
    xrl a,#60
    jnz pc_hide
    mov r6,#0
as_l:
    mov a,r6
    call occ_test
    jnz as_go
    inc r6
    mov a,r6
    xrl a,#100
    jnz as_l
    jmp pc_hide
as_go:
    mov a,r6                    ; the cursor to it, and fire
    mov r2,#0
as_y:
    add a,#256-NCOLS
    jnc as_x
    inc r2
    jmp as_y
as_x:
    add a,#NCOLS
    mov r0,#CURX
    movx @r0,a
    mov r0,#CURY
    mov a,r2
    movx @r0,a
    jmp fire
    endif

; my_shot: A = 0 when it's our turn, we're afloat, and we haven't fired.
; Clobbers A, r0, r1.
my_shot:
    mov r0,#ACTV
    movx a,@r0
    mov r1,a
    mov r0,#PSTAT
    movx a,@r0
    orl a,r1
    mov r1,a
    mov r0,#FIRED
    movx a,@r0
    orl a,r1
    ret

; cur_cell: r6 = A = the cursor's cell, 10y + x. Clobbers A, r0.
cur_cell:
    mov r0,#CURY
    movx a,@r0
    mov r6,a
    rl a
    rl a
    add a,r6
    rl a
    mov r6,a
    mov r0,#CURX
    movx a,@r0
    add a,r6
    mov r6,a
    ret

    if $ > 0900h
    error "page $8 has run out"
    endif

; ===========================================================================
    org 900h
    include "disp.inc"

; ===========================================================================
    org 0A00h
    include "text.inc"

; clock: the turn's seconds, counted down here between states; T0's two
; digits again each second, when it shows them. Clobbers A, r0-r7.
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
    mov r0,#LINEBUF+15
    xch a,r0
    movx a,@r0
    xrl a,#' '
    jz ck_x
    mov a,r0
    mov r1,#LINEBUF+13
    call lb_num3
    mov r4,#L_TOP
    jmp say_line_c
ck_x:
    ret

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

    if $ > 0B00h
    error "page $A has run out"
    endif

; ===========================================================================
    org 0B00h
BOARD   equ 1
    include "screen.inc"

; ===========================================================================
    org 0C00h
CURSOR  equ 1
    include "board.inc"

; ===========================================================================
    org 0D00h

; occ_test: A = nonzero if cell A is set in OCC. Clobbers A, r0, r1.
occ_test:
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
    movx a,@r0
    anl a,r1
    ret
bitmx:
    db 1, 2, 4, 8, 16, 32, 64, 128

; joy_dir: A = the stick's direction bits when it moves the cursor: as it
; is pressed, then every 6 frames from 18 on while it's held. Clobbers A,
; r0, r1.
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

    PAGE_TAIL
