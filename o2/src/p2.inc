; p2.inc -- page 2: the game's main loop. Each frame, in the quiet stretch
; after the last band: the keyboard and the stick, a step of the request in
; flight (page 1), and the phase's own page -- its on_state when a state has
; come, its tick every frame. All by farcall from here, the top level
; (tools/checkstack.py).
;
; The phases' pages: 3 the lobby, 4 placement, 5 play (7 its ticks), 6
; game over.

E_STATE equ 0                   ; a phase page's: a state has come ...
E_TICK  equ 1                   ; ... and every frame
POLLN   equ 20                  ; frames between polls: a third of a second
PG_BOOT equ 0
E_TABLES equ 1                  ; page 0's: back to the table list

    PAGE_HEAD
    db e_main&0FFh              ; entry 0: from the boot, one way
e_main:
    jmp main

main:
    mov r0,#LSTAT               ; nothing shown yet
    mov a,#0FFh
    movx @r0,a
    mov r0,#LASTPG
    movx @r0,a
    mov r0,#VIEWK
    movx @r0,a
    clr a
    mov r0,#POLLW
    movx @r0,a
    mov r0,#ACT
    movx @r0,a
    mov r0,#HELP
    movx @r0,a
    mov r0,#REDRAW
    movx @r0,a
ml:
    call frame_sync
    call key_poll
    call joy_poll
    mov r0,#HELP                ; the help screen up: a key takes it down
    movx a,@r0
    jz ml_keys
    xrl a,#1
    jnz ml_net
    call help_end
    jmp ml_net
ml_keys:
    mov r0,#KEY                 ; H or ?: the help screen
    movx a,@r0
    xrl a,#K_H
    jz ml_help
    xrl a,#K_H!K_QUEST
    jz ml_help
    xrl a,#K_QUEST!K_L          ; L: leave the table
    jnz ml_net
    mov r0,#ACT
    mov a,#ACT_LEAVE
    movx @r0,a
    jmp ml_net
ml_help:
    call phase_page             ; its context: the phase's page - 2
    mov a,r7
    add a,#256-2
    mov r5,a
    mov r0,#HELP
    mov a,#1
    movx @r0,a
    mov r7,#PG_HELP
    mov a,#E_HELP
    FCALL farcall
ml_net:
    mov r7,#PG_NET
    mov a,#E_NSTEP
    FCALL farcall
    mov r0,#NSTATE
    movx a,@r0
    jnz ml_tick                 ; a request is out
    mov r0,#ACTSENT             ; we have left: the table list
    movx a,@r0
    xrl a,#ACT_LEAVE
    jnz ml_have
    mov r7,#E_TABLES
    mov a,#PG_BOOT
    jmp far_go
ml_have:
    mov r0,#NETHAVE
    movx a,@r0
    xrl a,#NH_STATE
    jnz ml_idle
    clr a                       ; a state: taken
    movx @r0,a
    call st_header
    call st_page
    mov r0,#HELP                ; (not while the help is up)
    movx a,@r0
    xrl a,#1
    jz ml_idle
    mov a,#E_STATE
    FCALL farcall
    clr a                       ; the screen is back, if help was up
    mov r0,#HELP
    movx @r0,a
    mov r0,#REDRAW
    movx @r0,a
ml_idle:
    mov r0,#ACT                 ; an action goes at once; a poll waits its turn
    movx a,@r0
    jnz ml_begin
    mov r0,#POLLW
    movx a,@r0
    jz ml_begin
    dec a
    movx @r0,a
    jmp ml_tick
ml_begin:
    mov r0,#POLLW
    mov a,#POLLN
    movx @r0,a
    mov r7,#PG_NET
    mov a,#E_NBEGIN
    FCALL farcall
ml_tick:
    mov r0,#HELP                ; the phase's ticks wait for its screen
    movx a,@r0
    jnz ml_snd
    call tick_page
    mov a,#E_TICK
    FCALL farcall
ml_snd:
    call snd_go
    jmp ml

    if $ > 900h
    error "page $8 has run out"
    endif

; ===========================================================================
    org 900h

; help_end: a key or the button takes the help screen down: the phase's
; screen comes back with the next state, polled at once -- a text screen
; afresh (the lobby, game over), a board as it was (REDRAW). Until then
; HELP is 2: the states go to the phase, its ticks don't. The key is
; spent. Clobbers A, r0, r7.
help_end:
    mov r0,#KEY
    movx a,@r0
    xrl a,#K_NONE
    jnz he_go
    mov r0,#JOYEV
    movx a,@r0
    anl a,#J_FIRE
    jnz he_go
    ret
he_go:
    mov r0,#KEY
    mov a,#K_NONE
    movx @r0,a
    mov r0,#JOYEV
    clr a
    movx @r0,a
    mov r0,#POLLW
    movx @r0,a
    mov r0,#HELP
    mov a,#2
    movx @r0,a
    mov r0,#REDRAW
    mov a,#1
    movx @r0,a
    call phase_page             ; the lobby and game over: afresh
    mov a,r7
    xrl a,#3
    jz he_fresh
    xrl a,#3!6
    jnz he_x
he_fresh:
    mov r0,#LSTAT
    mov a,#0FFh
    movx @r0,a
he_x:
    ret

; st_header: the header's fields into XRAM, and the clock's second begun.
; Clobbers A, r0-r7.
st_header:
    mov r2,#0
    call rx_winr
    mov r5,#hdr_tab&0FFh
sh_l:
    mov a,r5
    movp a,@a                   ; the field's offset (0FFh: done)
    inc a
    jz sh_x
    dec a
    call rx_get
    mov r4,a
    inc r5
    mov a,r5
    movp a,@a                   ; where it goes
    mov r0,a
    mov a,r4
    movx @r0,a
    inc r5
    jmp sh_l
sh_x:
    mov r0,#SECF
    mov a,#60
    movx @r0,a
    ret
hdr_tab:
    db ST_COUNT, NSEAT
    db ST_STATUS, STAT
    db ST_PSTAT, PSTAT
    db ST_ACTIVE, ACTV
    db ST_MOVET, MOVET
    db ST_LASTAT, LASTAT
    db 0FFh

; phase_page: r7 = the page for STAT. Clobbers A, r0.
phase_page:
    mov r0,#STAT
    movx a,@r0
    mov r7,#3
    jz pp_x
    mov r7,#4
    xrl a,#STATUS_PLACE
    jz pp_x
    mov r7,#6
    xrl a,#STATUS_PLACE!STATUS_OVER
    jz pp_x
    mov r7,#5
pp_x:
    ret

; tick_page: r7 = the page for STAT's ticks: the battle's are page 7's.
; Clobbers A, r0.
tick_page:
    call phase_page
    mov a,r7
    xrl a,#5
    jnz tp_x
    mov r7,#7
tp_x:
    ret

; st_page: r7 = the page for this state. A state for another page than the
; last one's starts that page afresh: its screen is not up (LSTAT none).
; Clobbers A, r0.
st_page:
    call phase_page
    mov r0,#LASTPG
    movx a,@r0
    xrl a,r7
    jz stp_x
    mov a,r7
    movx @r0,a
    mov r0,#LSTAT
    mov a,#0FFh
    movx @r0,a
stp_x:
    ret

    if $ > 0A00h
    error "page 2's main loop has run out"
    endif

    if $ > 0A00h
    error "page $9 has run out"
    endif

; ===========================================================================
    org 0A00h
    include "rx.inc"
    include "input.inc"

; ===========================================================================
    org 0B00h
    include "snd.inc"

    PAGE_TAIL
