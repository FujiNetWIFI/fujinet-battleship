; spike.inc -- page 0 of a SPIKE build: the board engine's test pattern.
; Every cell code in turn across the rows, T0 and T1 written, the cursor
; on the stick. No network. For measuring a band's slack: PADN pads
; row_write's gap (build.sh), and tools/rowcheck.py counts frames that lost
; a row.

    PAGE_HEAD
    db e_boot&0FFh
e_boot:
    jmp boot

; ===========================================================================
    org 900h
    include "disp.inc"

; ===========================================================================
    org 0A00h
    include "input.inc"

; ===========================================================================
    org 0B00h
    include "text.inc"

s_t0:   db "BATTLESHIP O2",0
s_t1:   db "0123456789ABCDEF",0

; ===========================================================================
    org 0C00h
    include "screen.inc"

; ===========================================================================
    org 0D00h
    include "board.inc"

; ===========================================================================
    org 0E00h

boot:
    FCALL vdc_reset
    call cur_shape
    call scr_board
    clr a
    mov t,a
    strt cnt
    en tcnti
    en i
    mov r4,#L_TOP
    mov a,#s_t0&0FFh
    call say
    mov r4,#L_BOT
    mov a,#s_t1&0FFh
    call say
    mov r4,#0                   ; row r: cell (r + c) mod 9
spk_row:
    mov r6,#0
spk_cell:
    mov a,r4
    add a,r6
spk_mod:
    add a,#256-9
    jc spk_mod
    add a,#9
    mov r1,a
    mov a,r6
    add a,#SCR0
    mov r0,a
    mov a,r1
    movx @r0,a
    inc r6
    mov a,r6
    xrl a,#NCOLS
    jnz spk_cell
    call row_draw
    inc r4
    mov a,r4
    xrl a,#NCOLS
    jnz spk_row
    clr a
    mov r0,#CURX
    movx @r0,a
    mov r0,#CURY
    movx @r0,a
spk_l:
    call frame_wait
    call joy_poll
    mov r0,#JOYEV
    movx a,@r0
    mov r5,a
    mov r0,#CURX
    jb2 spk_left
    jb3 spk_right
    mov r0,#CURY
    jb0 spk_left
    jb1 spk_right
    jmp spk_draw
spk_left:
    movx a,@r0
    jz spk_draw
    dec a
    movx @r0,a
    jmp spk_draw
spk_right:
    movx a,@r0
    xrl a,#NCOLS-1
    jz spk_draw
    xrl a,#NCOLS-1
    inc a
    movx @r0,a
spk_draw:
    mov r0,#CURX
    movx a,@r0
    mov r3,a
    mov r0,#CURY
    movx a,@r0
    mov r4,a
    mov r1,#FRAMES              ; on 16 frames, off 8
    mov a,@r1
    anl a,#18h
    xrl a,#18h
    mov r2,a
    call spr_cursor
    jmp spk_l

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

    PAGE_TAIL
