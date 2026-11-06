; net.inc -- talking to the server, never waiting (M.U.L.E.'s clients/o2
; net.inc, by way of the Channel F's).
;
; Every request is an HTTP GET of
;     N:<endpoint><route>?table=<t>&player=<name>&bin=1&v=2
; (the table list: N:<endpoint>tables?bin=1) and every route but leave
; answers with the whole state. It is CLOSE (the last one), OPEN, STATUS
; until the answer has all come, READ: one cart transaction each, net_step
; posting the next when the cart has echoed the last, so the loop never
; waits. The cart keeps only the latest reply, so the answer is read in ONE
; read and then read where it lies, through the window (stbyte.inc), until
; the next request.
;
; A reply is taken only if its length is the one its own header implies
; (the Studio II's rule): a short read, or a reply cut off, is a failed poll
; and the next one tries again.
;
; Placed by the includer at NETBASE (page-aligned): five pages, each holding
; the tables its own MOVPs read.

NTMOUT  equ 250                 ; frames before a request is given up

; ===========================================================================
    org NETBASE
    include "mail.inc"

; net_begin: start the queued action, or a poll, unless a request is out.
; Clobbers A, r0-r7.
net_begin:
    mov r0,#NSTATE
    movx a,@r0
    jz nb_go
    ret
nb_go:
    clr a
    mov r0,#NETHAVE
    movx @r0,a
    mov r0,#NWAIT
    movx @r0,a
    mov r0,#ACT
    movx a,@r0
    jnz nb_act
    mov a,#ACT_STATE
nb_act:
    mov r0,#ACTSENT
    movx @r0,a
    mov r0,#ACTARG              ; NARG = ACTARG
    movx a,@r0
    mov r0,#NARG
    movx @r0,a
    clr a
    mov r0,#ACT
    movx @r0,a
    call np_close
    mov a,#NS_CLOS
    mov r0,#NSTATE
    movx @r0,a
    ret

; np_*: post one transaction to the network device.
np_close:
    mov r3,#NC_CLOSE
    mov r4,#0
    call np_begin
    jmp fn_post
np_status:
    mov r3,#NC_STATUS
    mov r4,#2
    call np_begin
    clr a
    call fn_par1
    clr a
    call fn_par1
    jmp fn_post
np_open:
    mov r3,#NC_OPEN
    mov r4,#2
    call np_begin
    mov a,#NM_GET
    call fn_par1
    clr a
    call fn_par1
    call url_build
    jmp fn_post
np_begin:
    mov r2,#NET_DEVICEID
    jmp fn_begin

; np_read: READ r3:r2 bytes. Clobbers A, r0-r7.
np_read:
    mov a,r2
    mov r5,a
    mov a,r3
    mov r1,a
    mov r3,#NC_READ
    mov r4,#2
    call np_begin
    mov a,#2                    ; a two-byte parameter: the length
    call fn_tx
    mov a,r5
    call fn_tx
    mov a,r1
    call fn_tx
    clr a
    call fn_par1
    jmp fn_post

; ===========================================================================
    org NETBASE+100h

; net_step: one step. A = 1 when the request is over (NETHAVE says how),
; else 0. Clobbers A, r0-r7.
net_step:
    mov r0,#NSTATE
    movx a,@r0
    jnz nst_out
    ret
nst_out:
    mov r0,#NWAIT               ; one more frame: give up at NTMOUT
    movx a,@r0
    inc a
    movx @r0,a
    add a,#256-NTMOUT
    jc nst_fail
    call fn_done
    jz nst_done
    clr a
    ret
nst_done:
    mov r0,#NSTATE
    movx a,@r0
    add a,#nst_tab&0FFh
    jmpp @a
nst_tab:
    db nst_fail&0FFh            ; (NS_IDLE: cannot happen)
    db nst_clos&0FFh
    db nst_open&0FFh
    db nst_stat&0FFh
    db nst_read&0FFh

nst_clos:                       ; (a CLOSE of nothing NAKs: either is fine)
    call np_open
    mov a,#NS_OPEN
    jmp nst_set
nst_open:
    call fn_ack
    jnz nst_fail
    clr a                       ; no STATUS reading yet
    mov r0,#NBWL
    movx @r0,a
    inc r0
    movx @r0,a
    jmp nst_ask
nst_stat:
    call fn_ack
    jnz nst_fail
    clr a
    call rx_win
    clr a
    call rx_get                 ; bytes waiting
    mov r2,a
    mov a,#1
    call rx_get
    mov r3,a
    mov a,#3
    call rx_get                 ; nDevStatus
    mov r4,a
    mov a,r2                    ; nothing waiting: an error ends it, else ask
    orl a,r3
    jnz nst_some
    mov a,r4
    xrl a,#1
    jz nst_ask
    jmp nst_fail
nst_some:
    mov a,r4                    ; the server has finished: read it all
    xrl a,#NS_EOF
    jz nst_rd
    mov r0,#NBWL                ; the same as last time: it has settled
    movx a,@r0
    xrl a,r2
    jnz nst_new
    inc r0
    movx a,@r0
    xrl a,r3
    jz nst_rd
nst_new:
    mov r0,#NBWL
    mov a,r2
    movx @r0,a
    inc r0
    mov a,r3
    movx @r0,a
nst_ask:
    call np_status
    mov a,#NS_STAT
    jmp nst_set
nst_rd:
    call np_read
    mov a,#NS_READ
    jmp nst_set

nst_read:
    call fn_ack
    jnz nst_fail
    clr a
    call rx_win
    mov a,#FN_R_RXLEN_LO&0FFh   ; RXLEN: what came
    call mb_get
    mov r0,#RXLEN
    movx @r0,a
    mov a,#FN_R_RXLEN_HI&0FFh
    call mb_get
    inc r0
    movx @r0,a
    clr a
    call rx_get
    mov r5,a                    ; the count: tables, or seats
    mov r0,#ACTSENT
    movx a,@r0
    xrl a,#ACT_TABLS
    jz nst_tbls
    call st_len                 ; r3:r2 = the length the header implies
    call len_chk
    jnz nst_short               ; at game over, records may be 10 bytes
    mov a,#NH_STATE
    jmp nst_have
nst_short:
    mov a,#ST_STATUS
    call rx_get
    xrl a,#STATUS_OVER
    jnz nst_fail
    call st_len10
    call len_chk
    jnz nst_fail
    mov a,#NH_STATE
    jmp nst_have
nst_tbls:
    mov a,r5                    ; 1 + 36n
    mov r2,#1
    mov r3,#0
    mov r6,#36
    call len_add
    call len_chk
    jnz nst_fail
    mov a,#NH_TABLES
    jmp nst_have
nst_fail:
    clr a
nst_have:
    mov r0,#NETHAVE
    movx @r0,a
    clr a
    mov r0,#NSTATE
    movx @r0,a
    mov a,#1
    ret
nst_set:
    mov r0,#NSTATE
    movx @r0,a
    clr a
    ret

; ===========================================================================
    org NETBASE+200h

; url_build: the devicespec for ACTSENT (argument NARG) into the TX stream.
; The deepest thing a farcalled net_step runs, so its string writers are
; leaves: the TX store is inlined, not called. Clobbers A, r0-r7.
url_build:
    mov a,#u_n&0FFh
    call txs
    mov a,#u_endp&0FFh
    call txs
    mov r0,#ACTSENT
    movx a,@r0
    xrl a,#ACT_TABLS
    jnz ub_route
    mov a,#u_tabls&0FFh
    call txs
    jmp ub_nul
ub_route:
    call tx_route
    mov a,#u_table&0FFh
    call txs
    mov r1,#SCTABL
    call txx
    mov a,#u_player&0FFh
    call txs
    mov r1,#SCNAME
    call txx
    mov a,#u_bin&0FFh
    call txs
ub_nul:
    clr a
    mov r0,#FN_W_DATA
    movx @r0,a
    ret

; txs: the NUL-terminated string at offset A of THIS page into the TX
; stream. Clobbers A, r0, r5.
txs:
    mov r5,a
txs_l:
    mov a,r5
    movp a,@a
    jz txs_x
    mov r0,#FN_W_DATA
    movx @r0,a
    inc r5
    jmp txs_l
txs_x:
    ret

; txx: the NUL-terminated string at external RAM r1 (at most 8), lower
; case, a space as '+'. Clobbers A, r0, r1, r6.
txx:
    mov a,r1
    mov r0,a
    movx a,@r0
    jz txx_x
    mov r6,a
    xrl a,#' '
    jnz txx_ch
    mov r6,#'+'
txx_ch:
    mov a,r6                    ; 'A'-'Z': lower case
    add a,#256-'A'
    jnc txx_put
    add a,#256-26
    jc txx_put
    mov a,r6
    orl a,#20h
    mov r6,a
txx_put:
    mov a,r6
    mov r0,#FN_W_DATA
    movx @r0,a
    inc r1
    jmp txx
txx_x:
    ret

u_n:      db "N:",0
u_tabls:  db "tables?bin=1",0
u_table:  db "?table=",0
u_player: db "&player=",0
u_bin:    db "&bin=1&v=2",0
    include "endpoint.inc"      ; u_endp: the server (build.sh ENDPOINT)

; ===========================================================================
    org NETBASE+300h

; tx_route: the route for ACTSENT, and what follows it: the number NARG
; (attack), or the five of FLEET (place). Clobbers A, r0-r7.
tx_route:
    mov r0,#ACTSENT
    movx a,@r0
    mov r7,a
    add a,#rt_str&0FFh
    movp a,@a
    call txs3
    mov a,r7
    xrl a,#ACT_ATTACK
    jz tr_num
    xrl a,#ACT_ATTACK!ACT_PLACE
    jz tr_fleet
    ret
tr_num:
    mov r0,#NARG
    movx a,@r0
    jmp tx_dec
tr_fleet:
    mov r7,#FLEET
trf_l:
    mov a,r7
    mov r0,a
    movx a,@r0
    call tx_dec
    inc r7
    mov a,r7
    xrl a,#FLEET+5
    jz trf_x
    mov r0,#FN_W_DATA
    mov a,#','
    movx @r0,a
    jmp trf_l
trf_x:
    ret

; txs3: txs for this page's strings. Clobbers A, r0, r5.
txs3:
    mov r5,a
txs3_l:
    mov a,r5
    movp a,@a
    jz txs3_x
    mov r0,#FN_W_DATA
    movx @r0,a
    inc r5
    jmp txs3_l
txs3_x:
    ret

; tx_dec: A (0-255) in decimal into the TX stream, no leading zeros. A
; leaf: url_build runs four calls deep in a farcall. Clobbers A, r0,
; r2-r4.
tx_dec:
    mov r2,a
    mov r4,#0                   ; bit 0: a digit has gone out
    mov r3,#100
txd_p:
    mov r0,#'0'
txd_s:
    mov a,r3                    ; value - power: no borrow = at least it
    cpl a
    inc a
    add a,r2
    jnc txd_e
    mov r2,a
    inc r0
    jmp txd_s
txd_e:
    mov a,r0
    xrl a,#'0'
    orl a,r4
    jz txd_n
    mov r4,#1
    mov a,r0
    mov r0,#FN_W_DATA
    movx @r0,a
txd_n:
    mov a,r3                    ; 100, then 10
    xrl a,#10
    jz txd_u
    mov r3,#10
    jmp txd_p
txd_u:
    mov a,r2
    add a,#'0'
    mov r0,#FN_W_DATA
    movx @r0,a
    ret

; The routes by ACT_*.
rt_str:
    db r_nul&0FFh, r_state&0FFh, r_ready1&0FFh, r_ready0&0FFh
    db r_place&0FFh, r_attack&0FFh, r_leave&0FFh
r_nul:    db 0
r_state:  db "state",0
r_ready1: db "ready/1",0
r_ready0: db "ready/0",0
r_place:  db "place/",0
r_attack: db "attack/",0
r_leave:  db "leave",0

; ===========================================================================
    org NETBASE+400h

; st_len: r3:r2 = the length a state of r5 seats implies: the lobby's
; 59 + 10n, placement's 49 + 10n, play's 49 + 115n. st_len10: 49 + 10n.
; Clobbers A, r6, r7.
st_len:
    mov a,#ST_STATUS
    call rx_get
    jz stl_lobby
    xrl a,#STATUS_PLACE
    jz st_len10
    mov r2,#ST_SEATS
    mov r3,#0
    mov r6,#SEAT_PLAY
    jmp stl_add
stl_lobby:
    mov r2,#ST_LSEATS
    mov r3,#0
    mov r6,#SEAT_LOBBY
    jmp stl_add
st_len10:
    mov r2,#ST_SEATS
    mov r3,#0
    mov r6,#SEAT_LOBBY
stl_add:
    mov a,r5
    ; fall into len_add

; len_add: r3:r2 += A * r6. Clobbers A, r7.
len_add:
    jz la_x
    mov r7,a
la_l:
    mov a,r2
    add a,r6
    mov r2,a
    clr a
    addc a,r3
    mov r3,a
    djnz r7,la_l
la_x:
    ret

; len_chk: A = 0 if RXLEN = r3:r2. Clobbers A, r0.
len_chk:
    mov r0,#RXLEN
    movx a,@r0
    xrl a,r2
    jnz lc_x
    inc r0
    movx a,@r0
    xrl a,r3
lc_x:
    ret
