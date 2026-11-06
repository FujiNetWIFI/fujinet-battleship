; stbyte.inc -- st_byte, a phase page's one reader of the reply (mail.inc
; has the rest of the mailbox, for the pages that talk to FujiNet).

; st_byte: A = byte r3:r2 (hi:lo) of the reply -- the window moved to cover
; it. A leaf but for mb_get: the map builder reaches it deep in a farcall.
; Clobbers r0, r6.
st_byte:
    mov a,r2                    ; window = offset / 16
    anl a,#0F0h
    orl a,r3
    swap a                      ; (r3 < 16: offsets under 4K)
    mov r6,a
    mov r0,#FN_W_REGSEL
    mov a,#FN_REG_RXWIN
    movx @r0,a
    inc r0
    mov a,r6
    movx @r0,a
    mov a,r2
    anl a,#0Fh
    add a,#FN_R_DATA&0FFh
    jmp mb_get
