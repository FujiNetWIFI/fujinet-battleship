; p1.inc -- page 1: the network engine (net.inc), reached by farcall from
; the other pages: entry 0 net_begin, entry 1 net_step.

    PAGE_HEAD
    db e_nbegin&0FFh, e_nstep&0FFh
e_nbegin:
    jmp net_begin
e_nstep:
    jmp net_step

NETBASE equ 900h
    include "net.inc"

    PAGE_TAIL
