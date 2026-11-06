; rx.inc -- a phase page's reader of the reply: st_byte (stbyte.inc) for one
; byte anywhere, rx_winr + rx_get for a run of them through one window.

    include "stbyte.inc"

; rx_winr: show the reply from byte r2*16 at $F30 (r2 < 256: under 4K).
; Clobbers A, r0.
rx_winr:
    mov r0,#FN_W_REGSEL
    mov a,#FN_REG_RXWIN
    movx @r0,a
    inc r0
    mov a,r2
    movx @r0,a
    ret

; rx_get: A = reply byte at offset A of the current window (0-207).
rx_get:
    add a,#FN_R_DATA&0FFh
    jmp mb_get
