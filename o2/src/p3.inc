; p3.inc -- page 3: the lobby (status 0). The text screen: the server's
; prompt over lines 0-1, a line for each seat (at most four) with READY, and
; what the button does under them. The button sends ready/1 or ready/0 from
; our own status in the last state, never a toggle: two presses in one
; state's time can't undo each other.

    PAGE_HEAD
    db e_state&0FFh, e_tick&0FFh
e_state:
    jmp lb_state
e_tick:
    jmp lb_tick

; lb_state: a lobby state. Clobbers A, r0-r7.
lb_state:
    mov r0,#LSTAT
    movx a,@r0
    jz ls_up
    clr a                       ; the screen, once
    movx @r0,a
    call scr_text
ls_up:
    mov r6,#0                   ; the prompt: lines 0 and 1
    mov r5,#1
    call say_prompt
    clr a                       ; the seats: lines 2-5
ls_seat:
    mov r0,#TMP0
    movx @r0,a
    call seat_line
    mov r0,#TMP0
    movx a,@r0
    inc a
    xrl a,#4
    jz ls_x
    xrl a,#4
    jmp ls_seat
ls_x:
    mov r0,#AUTO                ; (AUTOPLAY: ready once a state)
    clr a
    movx @r0,a
    ret

; seat_line: line 2+A = seat A: a space, its name (8), READY when its
; status says so; tan for us (seat 0). Past the last seat: what the button
; does, or nothing. Clobbers A, r0-r7.
seat_line:
    mov r5,a
    call lb_clear
    mov r0,#TXCOL
    mov a,#C_WHITE
    movx @r0,a
    mov r0,#NSEAT
    movx a,@r0
    mov r6,a
    mov a,r5
    cpl a
    add a,r6                    ; seats - A - 1: a borrow = past the last
    jc sl_seat
    mov a,r5                    ; just past it, with company: the
    xrl a,r6                    ; button's line
    jnz sl_put
    mov a,r6
    xrl a,#1
    jz sl_put
    mov r0,#PSTAT
    movx a,@r0
    xrl a,#PS_READY
    jz sl_un
    mov a,#s_ready&0FFh
    call lb_str
    jmp sl_put
sl_un:
    mov a,#s_unready&0FFh
    call lb_str
    jmp sl_put
sl_seat:
    mov a,r5                    ; ST_LSEATS + 10A
    rl a
    mov r2,a
    rl a
    rl a
    add a,r2
    add a,#ST_LSEATS
    mov r2,a
    mov r3,#0
    mov r1,#LINEBUF+1
    mov r4,#8
    call lb_reply
    inc r2                      ; (r2 is past the name's 8: + 1)
    call st_byte                ; its status
    xrl a,#PS_READY
    jnz sl_col
    mov r0,#LINEBUF+11
    mov a,#s_rdy&0FFh
    call lb_at
sl_col:
    mov a,r5
    jnz sl_put
    mov r0,#TXCOL
    mov a,#C_TAN
    movx @r0,a
sl_put:
    mov r0,#TXCOL               ; TXCOL2 the same
    movx a,@r0
    inc r0
    movx @r0,a
    mov a,r5
    add a,#2
    mov r4,a
    jmp say_line_c

; lb_tick: the button: ready, or not. Clobbers A, r0.
lb_tick:
    if AUTOPLAY
    mov r0,#AUTO
    movx a,@r0
    jnz lt_btn
    mov a,#1
    movx @r0,a
    mov r0,#PSTAT
    movx a,@r0
    xrl a,#PS_READY
    jnz lt_send
    endif
lt_btn:
    mov r0,#JOYEV
    movx a,@r0
    anl a,#J_FIRE
    jz lt_x
lt_send:
    mov a,#SND_SELECT
    FCALL beep
    mov r0,#PSTAT
    movx a,@r0
    xrl a,#PS_READY
    jz lt_un
    mov a,#ACT_READY1
    jmp lt_act
lt_un:
    mov a,#ACT_READY0
lt_act:
    mov r0,#ACT
    movx @r0,a
lt_x:
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

s_ready:   db " FIRE TO READY",0
s_unready: db " FIRE TO CANCEL",0
s_rdy:     db "READY",0

    if $ > 0B00h
    error "page $A has run out"
    endif

; ===========================================================================
    org 0B00h
TEXTSCR equ 1
    include "screen.inc"

; ===========================================================================
    org 0C00h
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

    PAGE_TAIL
