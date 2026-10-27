; disp.inc -- the main program's side of the display: it writes the fixed
; half's tables (fixed.inc) through the mailbox's POKE register and leaves the
; VDC to the interrupts. Main program, P1 = P1_MAIN, any register bank 0.
;
; One 256-byte page, placed by the includer: tx_build's MOVPs read charmap and
; txorder out of the page they run in.

; poke_at: aim POKE at the fixed half's address r2:A (r2 = $4-$7). Leaves the
; register file selecting POKE, so each poke_b after it is one MOVX.
; Clobbers A, r0, r1.
poke_at:
    mov r1,a
    mov r0,#FN_W_REGSEL
    mov a,#FN_REG_POKE_LO
    movx @r0,a
    inc r0
    mov a,r1
    movx @r0,a
    dec r0
    mov a,#FN_REG_POKE_HI
    movx @r0,a
    inc r0
    mov a,r2
    add a,#0FCh                 ; - 4: offset from $400
    movx @r0,a
; poke_sel: select POKE again, the pointer where it was (a st_byte selects
; RXWIN). Clobbers A, r0.
poke_sel:
    mov r0,#FN_W_REGSEL
    mov a,#FN_REG_POKE
    movx @r0,a
    ret

; poke_b: store A at the POKE pointer and advance it. Clobbers r0.
poke_b:
    mov r0,#FN_W_REGDATA
    movx @r0,a
    ret

; gptr: the character pointer for code A on the line whose bias is r5
; (-(Y/2)): low byte in r3, bit 8 in r1. The VDC adds Y/2 back while it
; draws, so the register holds glyph*8 - Y/2, nine bits.
; Clobbers A, r0.
gptr:
    mov r0,a                    ; fold $60-$7F onto $40-$5F: lowercase
    anl a,#60h
    xrl a,#60h
    jnz gp_keep
    mov a,r0
    anl a,#5Fh
    jmp gp_up
gp_keep:
    mov a,r0
gp_up:
    add a,#charmap&0FFh
    movp a,@a                   ; the glyph, 0-63
    mov r0,a
    rl a                        ; glyph*8: bits 7-3 of the low byte ...
    rl a
    rl a
    anl a,#0F8h
    add a,r5                    ; ... less Y/2; C = no borrow
    mov r3,a
    mov a,r0                    ; ... and bit 8 is the glyph's bit 5,
    swap a                      ; less the borrow
    rr a
    anl a,#1
    jc gp_nb
    xrl a,#1
gp_nb:
    mov r1,a
    ret

    ifndef DISP_MAP
; tx_build: LINEBUF into the text table at r2:r3, for the line at Y = r4.
; Colour TXCOL, or TXCOL2 for a character with bit 7 set; a colour with bit 7
; set (TX_SEATS) colours each four columns in its seat's colour (an
; auction's fields). The table's order is the writer's: quad A takes columns
; 0, 2, 4, 6, B 1, 3, 5, 7, C and D the next eight (txorder).
; Clobbers A, r0-r7.
tx_build:
    mov a,r4
    rr a
    cpl a
    inc a
    mov r5,a                    ; the bias: -(Y/2)
    mov a,r3
    call poke_at
    mov a,r4
    call poke_b                 ; Y
    mov r6,#0
txb_l:
    mov a,r6
    add a,#txorder&0FFh
    movp a,@a
    mov r2,a                    ; the column
    add a,#LINEBUF
    mov r0,a
    movx a,@r0
    mov r7,a
    anl a,#7Fh
    call gptr
    mov a,r3
    call poke_b                 ; pointer, low byte
    mov a,r7
    mov r0,#TXCOL
    jb7 txb_c2
    jmp txb_c1
txb_c2:
    inc r0                      ; TXCOL2
txb_c1:
    movx a,@r0
    jb7 txb_seat
txb_put:
    rl a
    orl a,r1
    call poke_b                 ; colour and pointer bit 8
    inc r6
    mov a,r6
    xrl a,#TXT_LEN
    jnz txb_l
    ret
txb_seat:
    mov a,r2                    ; the field's seat
    rr a
    rr a
    anl a,#3
    add a,#fieldcol&0FFh
    movp a,@a
    jmp txb_put
fieldcol:
    db C_RED, C_GREEN, C_TAN, C_VIOLET

; lb_clear: LINEBUF to spaces. Clobbers A, r0, r2.
lb_clear:
    mov r0,#LINEBUF
    mov r2,#TXT_LEN
    mov a,#' '
lbc_l:
    movx @r0,a
    inc r0
    djnz r2,lbc_l
    ret

txorder:
    db 0,2,4,6, 1,3,5,7, 8,10,12,14, 9,11,13,15
    endif                       ; (DISP_MAP: a map page, no text lines)
    include "charmap.inc"
