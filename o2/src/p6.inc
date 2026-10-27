; p6.inc -- page 6: game over (status 99). The server's prompt -- who won
; -- over T0 and T1, a fanfare or a dirge, and the winner's board: their
; fleet, shown at last (myShips 5-9, or ours), under the shots it took. A
; game abandoned while the fleets were being placed has no boards (its
; seats are 10-byte records): the prompt alone. The table goes back to the
; lobby by itself.

    PAGE_HEAD
    db e_state&0FFh, e_tick&0FFh
e_state:
    jmp go_state
e_tick:
    ret

; go_state: a game-over state: the screen once. Clobbers A, r0-r7.
go_state:
    mov r0,#LSTAT
    movx a,@r0
    xrl a,#STATUS_OVER
    jnz go_new
    ret
go_new:
    mov a,#STATUS_OVER
    movx @r0,a
    call scr_board
    mov r0,#REDRAW              ; (back from the help screen: no sound)
    movx a,@r0
    jnz go_say
    mov r0,#ACTV                ; ours: a fanfare
    movx a,@r0
    jnz go_lose
    mov a,#SND_WIN
    jmp go_snd
go_lose:
    mov a,#SND_LOSE
go_snd:
    FCALL beep
go_say:
    mov r6,#L_TOP
    mov r5,#L_BOT
    call say_prompt
    mov r0,#ACTV                ; nobody won: no board
    movx a,@r0
    add a,#256-4
    jc go_x
    mov r0,#NSEAT               ; 10-byte seats: no fields
    movx a,@r0
    mov r6,a
    mov r2,#ST_SEATS
    mov r3,#0
go_len:
    mov a,r6
    jz go_cmp
    mov a,r2
    add a,#SEAT_LOBBY
    mov r2,a
    clr a
    addc a,r3
    mov r3,a
    dec r6
    jmp go_len
go_cmp:
    mov r0,#RXLEN
    movx a,@r0
    xrl a,r2
    jnz go_fleet
    inc r0
    movx a,@r0
    xrl a,r3
    jz go_x
go_fleet:
    mov r2,#0                   ; the winner's fleet: ours, or myShips 5-9
    call rx_winr
    mov r2,#ST_SHIPS
    mov r0,#ACTV
    movx a,@r0
    jz go_f0
    mov r2,#ST_SHIPS+5
go_f0:
    mov r1,#FLEET
    mov r4,#5
go_fl:
    mov a,r2
    call rx_get
    movx @r1,a
    inc r1
    inc r2
    djnz r4,go_fl
    clr a
go_row:
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
    jz go_x
    xrl a,#NCOLS
    jmp go_row
go_x:
    ret

; build_row: SCR0 = row ROWN of the winner's board: its sea, its ships (ours
; white, theirs tan), the shots it took. Clobbers A, r0-r7.
build_row:
    mov r0,#ACTV
    movx a,@r0
    add a,#CL_SEA
    call row_fill
    clr a
br_s:
    mov r0,#SHIPJ
    movx @r0,a
    call ship_dec
    mov r7,#CL_SHIP
    mov r0,#ACTV
    movx a,@r0
    jz br_m
    mov r7,#CL_SUNK
br_m:
    call mark_row
    mov r0,#SHIPJ
    movx a,@r0
    inc a
    xrl a,#5
    jz br_f
    xrl a,#5
    jmp br_s
br_f:
    mov r0,#ACTV
    movx a,@r0
    call row_at_seat            ; r3 = the row's first cell in the window
    mov r6,#NCOLS
    mov r1,#SCR0
bf_l:
    mov a,r3
    call rx_get
    jz bf_n
    xrl a,#F_HIT
    jnz bf_miss
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

    if $ > 900h
    error "page $8 has run out"
    endif

; ===========================================================================
    org 900h
    include "disp.inc"

; ===========================================================================
    org 0A00h
    include "text.inc"

    if $ > 0B00h
    error "page $A has run out"
    endif

; ===========================================================================
    org 0B00h
BOARD   equ 1
    include "screen.inc"

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

    if $ > 0C00h
    error "page $B has run out"
    endif

; ===========================================================================
    org 0C00h
    include "board.inc"

; ===========================================================================
    org 0D00h
    include "rx.inc"

; say_prompt: the prompt on lines r6 and r5, broken at its last space in
; the first 16 if it needs two. Clobbers A, r0-r7.
say_prompt:
    mov r0,#TMP1
    mov a,r6
    movx @r0,a
    mov r0,#TMP2
    mov a,r5
    movx @r0,a
    mov r2,#0
    call rx_winr
    mov r7,#16                  ; r7 = line A's length, r6 = line B's start
    mov r6,#17
    mov a,#ST_PROMPT+16
    call rx_get
    jz sp_cut                   ; it fits
    mov r5,#ST_PROMPT+32        ; its end: the first NUL from 17 on
sp_e:
    mov a,r5
    dec r5
    add a,#256-ST_PROMPT-17
    jz sp_hard
    mov a,r5
    call rx_get
    jz sp_e
    inc r5                      ; r5 = its length (from ST_PROMPT)
    mov a,r5
    add a,#256-ST_PROMPT
    mov r5,a
sp_l:
    mov a,r7                    ; a space at r7, line B no longer than 16
    add a,#ST_PROMPT
    call rx_get
    xrl a,#' '
    jnz sp_n
    mov a,r7
    add a,#17
    cpl a
    add a,r5                    ; length > r7 + 17: line B wouldn't fit
    jnc sp_sp
sp_n:
    djnz r7,sp_l
sp_hard:
    mov r7,#16                  ; no such space: cut at 16
    mov r6,#16
    jmp sp_cut
sp_sp:
    mov a,r7
    inc a
    mov r6,a
sp_cut:
    mov r0,#TMP0                ; (line B's start, across say_line)
    mov a,r6
    movx @r0,a
    call lb_clear
    mov r1,#LINEBUF
    mov r2,#ST_PROMPT
    mov r3,#0
    mov a,r7
    mov r4,a
    call lb_reply
    mov r0,#TMP1
    movx a,@r0
    mov r4,a
    call say_line
    call lb_clear
    mov r0,#TMP0
    movx a,@r0
    add a,#ST_PROMPT
    mov r2,a
    mov r3,#0
    mov r1,#LINEBUF
    mov r4,#16
    call lb_reply
    mov r0,#TMP2
    movx a,@r0
    mov r4,a
    jmp say_line

; lb_reply: r4 bytes of the reply from r3:r2 into LINEBUF at r1, a NUL as a
; space; r2 advanced (a run never crosses 256). Clobbers A, r0, r6.
lb_reply:
    call st_byte
    jnz lr_1
    mov a,#' '
lr_1:
    movx @r1,a
    inc r1
    inc r2
    djnz r4,lb_reply
    ret

    if $ > 0E00h
    error "page $D has run out"
    endif

; ===========================================================================
    org 0E00h

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
