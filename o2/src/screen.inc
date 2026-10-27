; screen.inc -- the text screen: T0-T5 at Y = Y_T0 + 36n on black. One
; 256-byte page, the template with the code that copies it.

; scr_text: the text screen, every line blank, the sprites hidden.
; Clobbers A, r0-r7.
scr_text:
    mov r2,#SCRCOL>>8           ; the colour, then the band list
    mov a,#SCRCOL&0FFh
    call poke_at
    mov r4,#scr_text_t&0FFh
    mov r6,#2+NBANDS*3
    call copy_rom
    call spr_hide
    mov r1,#MODE                ; no board rows; the singles parked
    mov a,#MODE_SETUP
    mov @r1,a
    ; fall into scr_clear

; scr_clear: all six lines blank. Clobbers A, r0-r7.
scr_clear:
    clr a
sc_l:
    mov r0,#TMP0                ; (say_line keeps no register)
    movx @r0,a
    call lb_clear
    mov r0,#TMP0
    movx a,@r0
    mov r4,a
    call say_line
    mov r0,#TMP0
    movx a,@r0
    inc a
    mov r6,a
    xrl a,#6
    jz sc_x
    mov a,r6
    jmp sc_l
sc_x:
    ret

; bands_board: the board screen's colour and band list (board.inc
; scr_board). Clobbers A, r0-r2, r4, r6.
bands_board:
    mov r2,#SCRCOL>>8
    mov a,#SCRCOL&0FFh
    call poke_at
    mov r4,#scr_board_t&0FFh
    mov r6,#2+NBANDS*3
    jmp copy_rom

; spr_hide: the four sprites below the screen. Clobbers A, r0-r2, r6.
spr_hide:
    mov r2,#SPRTAB>>8
    mov a,#SPRTAB&0FFh
    call poke_at
    mov r6,#4
sh_l:
    mov a,#0F8h
    call poke_b
    clr a
    call poke_b
    clr a
    call poke_b
    djnz r6,sh_l
    ret

; copy_rom: r6 bytes from offset r4 of this page to the POKE pointer.
copy_rom:
    mov a,r4
    movp a,@a
    call poke_b
    inc r4
    djnz r6,copy_rom
    ret

; poke_n: r6 copies of A at the POKE pointer. Clobbers r0, r6.
poke_n:
    call poke_b
    djnz r6,poke_n
    ret

TB_FIRST equ 256-(Y_T0+14-LEAD)
scr_text_t:
    db BG_BLACK
    db TB_FIRST                 ; the first band: T0's end
    db J_TXT, T1&0FFh, 256-TXT_PITCH
    db J_TXT, T2&0FFh, 256-TXT_PITCH
    db J_TXT, T3&0FFh, 256-TXT_PITCH
    db J_TXT, T4&0FFh, 256-TXT_PITCH
    db J_TXT, T5&0FFh, 0        ; the last
    rept NBANDS-5
    db J_NONE, 0, 0
    endm

TBB_FIRST equ 256-(Y_T0+14-LEAD)
scr_board_t:
    db BG_BLUE
    db TBB_FIRST                ; the first band: T0's end, row 0
    db J_ROW, (ROWS&0FFh)+ROW_TAB, 256-(ROWGAP+CELL_H)
K   set 2
    rept NCOLS-2                ; rows 1-8, each staging the next
    db J_ROW, (ROWS&0FFh)+K*ROW_TAB, 256-PITCH
K   set K+1
    endm
    db J_ROW, 0FFh, 256-PITCH   ; row 9
    db J_TXT, T1&0FFh, 0        ; T1 under the board: the last
