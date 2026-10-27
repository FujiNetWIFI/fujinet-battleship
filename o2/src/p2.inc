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
ml:
    call frame_sync
    call key_poll
    call joy_poll
    mov r0,#KEY                 ; L: leave the table
    movx a,@r0
    xrl a,#K_L
    jnz ml_net
    mov r0,#ACT
    mov a,#ACT_LEAVE
    movx @r0,a
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
    mov a,#E_STATE
    FCALL farcall
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
    call tick_page
    mov a,#E_TICK
    FCALL farcall
    call snd_go
    jmp ml

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

; ===========================================================================
    org 0A00h
    include "rx.inc"
    include "input.inc"

; ===========================================================================
    org 0B00h
    include "snd.inc"

    PAGE_TAIL
