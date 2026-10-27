; p0.inc -- page 0: the boot. The cart, who we are, which table; then the
; game's main loop (page 2), one way.
;
; The player's name is FujiNet's shared one, appkey creator 1, app 1, key 0.
; Without it we ask for one on the keyboard and keep it there.
;
; The top level runs at stack depth 0: it is where the farcalls into the
; network page are made (tools/checkstack.py).

PG_MAIN equ 2                   ; the game's main loop, entered one way

; BT_REQ: the queued request (ACT; a poll if none), to its end. A =
; NETHAVE. A macro: the farcalls have to be made from the top level.
BT_REQ  macro {GLOBALSYMBOLS}
    mov r7,#PG_NET
    mov a,#E_NBEGIN
    FCALL farcall
.btr_l:
    call frame_wait
    mov r7,#PG_NET
    mov a,#E_NSTEP
    FCALL farcall
    mov r0,#NSTATE
    movx a,@r0
    jnz .btr_l
    mov r0,#NETHAVE
    movx a,@r0
    endm

    PAGE_HEAD
    db e_boot&0FFh              ; entry 0: the boot (fixed.inc start)
    db e_tables&0FFh            ; entry 1: we have left a table (page 2)
e_boot:
    jmp boot
e_tables:
    jmp bt_left

; ak_write_name: key 0 = SCNAME. Best effort. Clobbers A, r0-r7.
ak_write_name:
    mov r5,#0
    mov r1,#1
    call ak_open
    mov r2,#FUJI_DEVICEID
    mov r3,#FC_AKWRITE
    mov r4,#0
    call fn_begin
    mov r1,#SCNAME
akw_l:
    mov a,r1
    mov r0,a
    movx a,@r0
    jz akw_go
    call fn_tx
    inc r1
    jmp akw_l
akw_go:
    jmp fn_go

; tbl_show: the NTBL tables in the reply (a count, then TB_SIZE-byte
; records) on lines 1-5: name and players. Clobbers A, r0-r7.
tbl_show:
    clr a
ts_l:
    mov r0,#TMP0                ; (tbl_line keeps no register)
    movx @r0,a
    mov r6,a
    mov r0,#NTBL
    movx a,@r0
    xrl a,r6
    jz ts_x
    mov a,r6
    call tbl_line
    mov r0,#TMP0
    movx a,@r0
    inc a
    jmp ts_l
ts_x:
    ret

; tbl_line: line 1+A = table A: its name (11), its players ("1/2"); the
; arrow and tan when it is SEL. Clobbers A, r0-r7.
tbl_line:
    mov r5,a
    call lb_clear
    mov a,r5
    call tbl_off
    mov a,r2                    ; the name
    add a,#TB_NAME
    mov r2,a
    mov r1,#LINEBUF+1
    mov r4,#11
tl_n:
    call tl_ch
    djnz r4,tl_n
    mov a,r2                    ; the players: "c / m" as "c/m"
    add a,#TB_PLAYERS-TB_NAME-11
    mov r2,a
    mov r1,#LINEBUF+13
    call tl_ch
    mov a,#'/'
    movx @r1,a
    inc r1
    mov a,r2
    add a,#3
    mov r2,a
    call tl_ch
    mov r0,#TXCOL
    mov a,#C_WHITE
    movx @r0,a
    mov r0,#SEL
    movx a,@r0
    xrl a,r5
    jnz tl_put
    mov r0,#LINEBUF
    mov a,#CH_ARROW
    movx @r0,a
    mov r0,#TXCOL
    mov a,#C_TAN
    movx @r0,a
tl_put:
    mov r0,#TXCOL               ; TXCOL2 the same
    movx a,@r0
    inc r0
    movx @r0,a
    mov a,r5
    inc a
    mov r4,a
    jmp say_line_c

; tbl_choose: the stick picks one of NTBL tables; SCTABL = its id. A move
; redraws just the two lines it changes: a whole list takes frames, and a
; quick press would be lost meanwhile. Clobbers A, r0-r7.
tbl_choose:
    clr a
    mov r0,#SEL
    movx @r0,a
    call tbl_show
tc_l:
    call frame_wait
    call joy_poll
    mov r0,#JOYEV
    movx a,@r0
    jb4 tc_take
    jb0 tc_up
    jb1 tc_down
    jmp tc_l
tc_up:
    mov r0,#SEL
    movx a,@r0
    jz tc_l
    dec a
    jmp tc_move
tc_down:
    mov r0,#SEL
    movx a,@r0
    inc a
    mov r6,a
    mov r0,#NTBL
    movx a,@r0
    xrl a,r6
    jz tc_l
    mov a,r6
tc_move:
    mov r6,a                    ; the old line, then the new
    mov r0,#SEL
    movx a,@r0
    mov r0,#TMP1
    movx @r0,a
    mov a,r6
    mov r0,#SEL
    movx @r0,a
    mov r0,#TMP1
    movx a,@r0
    call tbl_line
    mov r0,#SEL
    movx a,@r0
    call tbl_line
    jmp tc_l
tc_take:
    mov r0,#SEL                 ; its id: 8 bytes, NUL-padded
    movx a,@r0
    call tbl_off
    mov r1,#SCTABL
    mov r4,#8
tc_id:
    call st_byte
    movx @r1,a
    inc r1
    inc r2
    djnz r4,tc_id
    clr a
    movx @r1,a
    ret

    if $ > 900h
    error "page $8 has run out"
    endif

    org 900h
    include "disp.inc"

; ===========================================================================
    org 0A00h
    include "mail.inc"

; fn_go: post and wait (the boot only: the game never waits). A = 0 if it
; worked and was ACKed. Clobbers A, r0, r2, r6.
fn_go:
    call fn_post
    mov r2,#0
fg_l:
    call fn_done
    jz fg_done
    djnz r2,fg_l
    mov a,#0FFh
    ret
fg_done:
    jmp fn_ack

; ak_open: OPEN_APPKEY key r5, mode r1 (0 read, 1 write): creator 1, app 1,
; no parameters, a 6-byte payload. A = 0 if it worked. Clobbers A, r0, r2-r4,
; r6, r7.
ak_open:
    mov r2,#FUJI_DEVICEID
    mov r3,#FC_AKOPEN
    mov r4,#0
    call fn_begin
    mov a,#1                    ; creator, LE
    call fn_tx
    clr a
    call fn_tx
    mov a,#1                    ; app
    call fn_tx
    mov a,r5                    ; key
    call fn_tx
    mov a,r1                    ; mode
    call fn_tx
    clr a                       ; reserved
    call fn_tx
    jmp fn_go

; ak_read: key r5 into the reply: its length (2 bytes), then the key. A = the
; length, at most 64; 0: none. Clobbers A, r0-r7.
ak_read:
    mov r1,#0
    call ak_open
    jnz ak_none
    mov r2,#FUJI_DEVICEID
    mov r3,#FC_AKREAD
    mov r4,#0
    call fn_begin
    call fn_go
    jnz ak_none
    clr a
    call rx_win
    mov a,#1
    call rx_get
    jnz ak_64
    clr a
    call rx_get
    mov r6,a
    add a,#256-65
    jc ak_64
    mov a,r6
    ret
ak_64:
    mov a,#64
    ret
ak_none:
    clr a
    ret

; ak_name: SCNAME from key 0: at most 8 letters, digits and spaces, not
; starting with a space; anything else means no name. Clobbers A, r0-r7.
ak_name:
    clr a
    mov r0,#SCNAME
    movx @r0,a
    mov r5,#0
    call ak_read
    jz akn_x
    mov r4,a                    ; the length, at most 8
    add a,#256-9
    jnc akn_ok
    mov r4,#8
akn_ok:
    mov a,#2                    ; no leading space
    call rx_get
    xrl a,#' '
    jz akn_bad
    mov r3,#0
akn_l:
    mov a,r3                    ; reply byte 2 + i
    add a,#2
    call rx_get
    jz akn_end                  ; (a NUL-padded key)
    call name_ch
    jz akn_bad
    mov r6,a
    mov a,r3
    add a,#SCNAME
    mov r0,a
    mov a,r6
    movx @r0,a
    inc r3
    mov a,r3
    xrl a,r4
    jnz akn_l
akn_end:
    mov a,r3
    add a,#SCNAME
    mov r0,a
    clr a
    movx @r0,a
akn_x:
    ret
akn_bad:
    clr a
    mov r0,#SCNAME
    movx @r0,a
    ret

    if $ > 0B00h
    error "page $A has run out"
    endif

; ===========================================================================
    org 0B00h

; boot: the top level. Every farcall into the network page is made here.
boot:
    FCALL vdc_reset
    call scr_text
    call ints_on
    mov r4,#1
    mov a,#s_title&0FFh
    call say
    mov r4,#2
    mov a,#s_sub&0FFh
    call say
    mov r4,#4
    mov a,#s_fuji&0FFh
    call say
    mov r7,#120                 ; the title, two seconds
bt_ttl:
    call frame_wait
    djnz r7,bt_ttl

    mov a,#FN_R_MAGIC0&0FFh     ; a FujiNet cartridge, speaking protocol 2?
    call mb_get
    xrl a,#'F'
    jnz bt_nocart
    mov a,#FN_R_PROTO_VER&0FFh
    call mb_get
    xrl a,#FN_PROTO_VER
    jz bt_cart
bt_nocart:
    mov r4,#5
    mov a,#s_nocart&0FFh
    call say
bt_stop:
    jmp bt_stop

bt_cart:
    call ak_name
    mov r0,#SCNAME
    movx a,@r0
    jnz bt_tables
    call name_entry
    call ak_write_name

; The table list. From a table we have left, the text screen first.
bt_left:
    call scr_text
bt_tables:
    call scr_clear
    mov r4,#0
    mov a,#s_tables&0FFh
    call say
    mov r4,#5
    mov a,#s_load&0FFh
    call say
    mov r0,#ACT
    mov a,#ACT_TABLS
    movx @r0,a
    BT_REQ
    xrl a,#NH_TABLES
    jnz bt_none
    clr a                       ; NTBL = how many (at most 5)
    call rx_win
    clr a
    call rx_get
    mov r6,a
    add a,#256-6
    jnc bt_n
    mov r6,#5
bt_n:
    mov a,r6
    mov r0,#NTBL
    movx @r0,a
    jnz bt_pick
bt_none:
    mov r4,#5
    mov a,#s_notbl&0FFh
    call say
    call wait_fire
    jmp bt_tables
bt_pick:
    call tbl_choose             ; SCTABL

; Join: the first /state.
    call scr_clear
    mov r4,#2
    mov a,#s_join&0FFh
    call say
bt_jagain:
    BT_REQ
    xrl a,#NH_STATE
    jnz bt_jagain               ; until it answers
    mov r0,#NETHAVE             ; the state is in: the game takes it from here
    mov a,#NH_STATE
    movx @r0,a
    mov r7,#0
    mov a,#PG_MAIN
    jmp far_go

; ints_on: the line counter and the vblank interrupt.
ints_on:
    clr a
    mov t,a
    strt cnt
    en tcnti
    en i
    ret

; wait_fire: until the button is pressed. Clobbers A, r0-r4.
wait_fire:
    call frame_wait
    call joy_poll
    mov r0,#JOYEV
    movx a,@r0
    anl a,#J_FIRE
    jz wait_fire
    ret

; name_ch: A as a name's character -- a letter upper-cased, a digit or a
; space as it is; 0 for anything else. Clobbers r6.
name_ch:
    mov r6,a
    xrl a,#' '
    jz nc_ok
    mov a,r6
    add a,#256-'a'              ; 'a'..'z': up
    jnc nc_1
    add a,#256-26
    jc nc_1
    mov a,r6
    add a,#256-32
    ret
nc_1:
    mov a,r6                    ; 'A'..'Z'
    add a,#256-'A'
    jnc nc_2
    add a,#256-26
    jnc nc_ok
nc_2:
    mov a,r6                    ; '0'..'9'
    add a,#256-'0'
    jnc nc_no
    add a,#256-10
    jnc nc_ok
nc_no:
    clr a
    ret
nc_ok:
    mov a,r6
    ret

    if $ > 0C00h
    error "page $B has run out"
    endif

; ===========================================================================
    org 0C00h
    include "text.inc"

s_title:  db "   BATTLESHIP",0
s_sub:    db "   ODYSSEY 2",0
s_fuji:   db "    FUJINET",0
s_nocart: db "NO FUJINET CART",0
s_tables: db "CHOOSE A TABLE",0
s_load:   db "  LOADING...",0
s_notbl:  db "NO TABLES  FIRE",0
s_join:   db "  JOINING...",0
s_yname:  db "  YOUR NAME?",0
s_nk1:    db "TYPE IT THEN ENT",0
s_nk2:    db "CLR RUBS OUT",0

; tl_ch: reply byte r3:r2 into LINEBUF at r1 (NUL as a space), both
; advanced (r2 only: a table's fields never cross 256).
tl_ch:
    call st_byte
    jnz tlc_1
    mov a,#' '
tlc_1:
    movx @r1,a
    inc r1
    inc r2
    ret

; tbl_off: r3:r2 = 1 + 36*A, table A in the reply. Clobbers r6.
tbl_off:
    mov r6,a                    ; 36A = 32A + 4A
    rl a
    rl a
    anl a,#0FCh
    mov r2,a
    mov a,r6
    swap a
    rl a
    anl a,#0E0h
    add a,r2
    inc a
    mov r2,a
    mov r3,#0
    ret

    if $ > 0D00h
    error "page $C has run out"
    endif

; ===========================================================================
    org 0D00h
TEXTSCR equ 1
    include "screen.inc"
    include "input.inc"

    if $ > 0E00h
    error "page $D has run out"
    endif

; ===========================================================================
    org 0E00h

; name_entry: SCNAME from the keyboard: letters, digits and spaces (not
; first), at most 8; CLR rubs out, ENT ends. Clobbers A, r0-r7.
name_entry:
    call scr_clear
    mov r4,#0
    mov a,#s_yname&0FFh
    call say
    mov r4,#4
    mov a,#s_nk1&0FFh
    call say
    mov r4,#5
    mov a,#s_nk2&0FFh
    call say
    clr a
    mov r0,#SCNAME
    movx @r0,a
    mov r0,#NAMEN
    movx @r0,a
ne_l:
    call frame_sync
    call key_poll
    mov r0,#KEY
    movx a,@r0
    mov r5,a
    xrl a,#K_NONE
    jz ne_draw
    mov a,r5
    xrl a,#K_ENT
    jz ne_end
    xrl a,#K_ENT!K_CLR
    jz ne_rub
    mov a,r5                    ; a character: its ASCII
    add a,#256-48
    jc ne_draw
    mov a,r5
    add a,#keychar&0FFh
    movp a,@a
    call name_ch
    jz ne_draw
    mov r5,a
    mov r0,#NAMEN
    movx a,@r0
    xrl a,#8
    jz ne_draw                  ; full
    xrl a,#8
    jnz ne_add
    mov a,r5                    ; no leading space
    xrl a,#' '
    jz ne_draw
    mov r0,#NAMEN
    movx a,@r0
ne_add:
    mov r1,a
    inc a
    movx @r0,a
    mov a,r1
    add a,#SCNAME
    mov r0,a
    mov a,r5
    movx @r0,a
    inc r0
    clr a
    movx @r0,a
    jmp ne_draw
ne_rub:
    mov r0,#NAMEN
    movx a,@r0
    jz ne_draw
    dec a
    movx @r0,a
    add a,#SCNAME
    mov r0,a
    clr a
    movx @r0,a
    jmp ne_draw
ne_end:
    mov r0,#NAMEN
    movx a,@r0
    jz ne_draw
    ret
ne_draw:
    call lb_clear               ; the name, then '_' blinking after it
    mov r1,#0
nd_l:
    mov a,r1
    add a,#SCNAME
    mov r0,a
    movx a,@r0
    jz nd_x
    mov r6,a
    mov a,r1
    add a,#LINEBUF+4
    mov r0,a
    mov a,r6
    movx @r0,a
    inc r1
    jmp nd_l
nd_x:
    mov a,r1
    add a,#LINEBUF+4
    mov r1,a
    mov r0,#FRAMES
    mov a,@r0
    anl a,#16
    jz nd_put
    mov a,#'_'
    movx @r1,a
nd_put:
    mov r4,#2
    call say_line
    jmp ne_l

; A key's character (readkey codes 0-47; 0: none).
keychar:
    db "0123456789",0,0," ?LP+WERTUIOQSDFGHJKAZXCVBM.-*/=YN",0,0

    if $ > 0F00h
    error "page $E has run out"
    endif

    PAGE_TAIL
