; p8.inc -- page 8: the help screen, on H or ?. A text screen of the keys
; for where we are (r5: HC_*); the caller takes it down again on any key or
; the button (page 2 for the game, page 0 for the table list).

    PAGE_HEAD
    db e_help&0FFh
e_help:
    jmp help_show

; help_show: the help for context r5. Clobbers A, r0-r7.
help_show:
    mov r0,#TMP1
    mov a,r5
    movx @r0,a
    call scr_text
    mov r4,#0                   ; the title, tan
    mov r5,#C_TAN
    mov a,#h_title&0FFh
    call hl_saya
    mov r0,#TMP1                ; its lines: 5 a context, 0 none
    movx a,@r0
    mov r6,a
    rl a
    rl a
    add a,r6
    mov r0,#TMP0                ; TMP0 = the line's string, TMP2 = the line
    movx @r0,a
    mov r0,#TMP2
    mov a,#1
    movx @r0,a
hs_l:
    mov r0,#TMP0
    movx a,@r0
    add a,#helptab&0FFh
    movp a,@a
    jz hs_n
    mov r6,a
    mov r0,#TMP2
    movx a,@r0
    mov r4,a
    mov r5,#C_WHITE
    mov r0,#TMP1                ; placing, the battle: page $B's
    movx a,@r0
    add a,#256-HC_PLACE
    add a,#256-2
    mov a,r6
    jnc hs_b
    call hl_saya                ; (keeps no register)
    jmp hs_n
hs_b:
    call hl_say
hs_n:
    mov r0,#TMP0
    movx a,@r0
    inc a
    movx @r0,a
    mov r0,#TMP2
    movx a,@r0
    inc a
    movx @r0,a
    xrl a,#6
    jnz hs_l
    ret

; Each context's lines 1-5: their strings, 0 for a blank line -- placing's
; and the battle's in page $B (hl_say), the others' in page $A (hl_saya).
helptab:
    db h_choose&0FFh, h_join&0FFh, 0, 0, 0
    db h_ready&0FFh, h_again&0FFh, h_bots&0FFh, 0, h_leave&0FFh
    db h_move&0FFh, h_keep&0FFh, h_turn&0FFh, h_auto&0FFh, h_leavb&0FFh
    db h_shoot&0FFh, h_views&0FFh, h_enemy&0FFh, h_key&0FFh, h_leavb&0FFh
    db h_over1&0FFh, h_over2&0FFh, 0, 0, h_leave&0FFh

    if $ > 900h
    error "page $8 has run out"
    endif

; ===========================================================================
    org 900h
    include "disp.inc"

; ===========================================================================
    org 0A00h
    include "text.inc"

; The help's strings for the table list, the lobby and game over, and the
; title: lb_str's (text.inc), so in its page.
h_title:  db "HELP    ANY KEY",0
h_choose: db "STICK  CHOOSES",0
h_join:   db "FIRE   JOINS",0
h_ready:  db "FIRE   READY UP",0
h_again:  db " AGAIN: CANCEL",0
h_bots:   db "AI TABLES: BOTS",0
h_over1:  db "BACK TO THE",0
h_over2:  db "LOBBY SOON",0
h_leave:  db "L      LEAVES",0

; hl_saya: line r4 = the help string at offset A of THIS page ($A), in
; colour r5. Clobbers A, r0-r7.
hl_saya:
    mov r6,a
    call lb_clear
    mov r0,#TXCOL
    mov a,r5
    movx @r0,a
    inc r0
    movx @r0,a
    mov a,r6
    call lb_str
    jmp say_line_c

    if $ > 0B00h
    error "page $A has run out"
    endif

; ===========================================================================
    org 0B00h

; hl_say: line r4 = the help string at offset A of this page, in colour r5;
; a character with bit 7 set (the hit's ball) red. Clobbers A, r0-r7.
hl_say:
    mov r6,a
    call lb_clear
    mov r0,#TXCOL
    mov a,r5
    movx @r0,a
    inc r0
    mov a,#C_RED
    movx @r0,a
    mov r0,#LINEBUF
    mov a,r6
    mov r1,a
hy_l:
    mov a,r1
    movp a,@a
    jz hy_x
    movx @r0,a
    inc r0
    inc r1
    jmp hy_l
hy_x:
    jmp say_line_c

; Placing's and the battle's (hl_say's: in its page).
h_move:   db "STICK  MOVES",0
h_keep:   db "FIRE   KEEPS",0
h_turn:   db "R      TURNS",0
h_auto:   db "Z  PLACES REST",0
h_shoot:  db "STICK+FIRE SHOOT",0
h_views:  db "T TARGET 0 YOURS",0
h_enemy:  db "1-3    THEIRS",0
h_key:    db 80h," HIT  . MISS",0
h_leavb:  db "L      LEAVES",0


    if $ > 0C00h
    error "page $B has run out"
    endif

; ===========================================================================
    org 0C00h
TEXTSCR equ 1
    include "screen.inc"

    PAGE_TAIL
