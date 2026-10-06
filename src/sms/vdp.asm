; vdp.asm -- Master System VDP port access with rules C cannot promise, the
; tile uploader, the name table blitter and a correct clear_vram.
;
; From the 5 Card Stud SMS client (src/sms/vdp.asm there). The control port
; takes an address in two writes, and the frame interrupt's status read
; between them would reset the latch and land the second byte as an address
; low byte: vdp_addr keeps the pair under DI. The data port wants 26 T-states
; between VRAM writes during active display, as crt0's RST $18 spaces them;
; vdp_word and vdp_blit pace themselves to that.

    SECTION code_user

    PUBLIC  _vdp_addr
    PUBLIC  _vdp_word
    PUBLIC  _vdp_byte
    PUBLIC  _vdp_runs
    PUBLIC  _vdp_blit
    PUBLIC  _blitCount

; void vdp_addr(unsigned int cmd) __z88dk_fastcall -- L first, then H
_vdp_addr:
    ld      a,l
    di
    out     ($BF),a
    ld      a,h
    out     ($BF),a
    ei
    ret

; void vdp_word(unsigned int w) __z88dk_fastcall -- a name table entry, L first
_vdp_word:
    ld      a,l
    out     ($BE),a                 ; 11
    ld      a,h                     ; 4
    sub     0                       ; 7
    nop                             ; 4 = 26
    out     ($BE),a
    ret

; void vdp_byte(unsigned char b) __z88dk_fastcall
_vdp_byte:
    ld      a,l
    out     ($BE),a
    ret

; void vdp_blit(const unsigned int *cells) __z88dk_fastcall
;
; Stream blitCount name table entries from cells to wherever the VRAM address
; already points. Every OUT is at least 26 T-states after the last, so this
; is safe with the display on.
_vdp_blit:
    ld      bc,(_blitCount)
    ld      a,b
    or      c
    ret     z
vb_loop:
    ld      a,(hl)                  ; 7
    out     ($BE),a                 ; 11
    inc     hl                      ; 6
    ld      a,(hl)                  ; 7
    inc     hl                      ; 6
    nop                             ; 4
    nop                             ; 4 = 27 since the last OUT
    out     ($BE),a                 ; 11
    dec     bc                      ; 6
    ld      a,b                     ; 4
    or      c                       ; 4
    jr      nz,vb_loop              ; 12, then 7 + 11 = 33 to the next OUT
    ret

; void vdp_runs(const unsigned char *runs) __z88dk_fastcall
;
; Upload src/sms/tileset.c's art, display off: a run is { first tile lo, hi,
; count } and count * 32 bytes of 4bpp planar rows; a zero count ends it.
_vdp_runs:
    ld      e,(hl)
    inc     hl
    ld      d,(hl)
    inc     hl
    ld      a,(hl)
    inc     hl
    or      a
    ret     z
    ld      b,a                     ; tiles in this run
    push    hl
    ex      de,hl
    add     hl,hl
    add     hl,hl
    add     hl,hl
    add     hl,hl
    add     hl,hl                   ; tile * 32
    ld      a,l
    di
    out     ($BF),a
    ld      a,h
    or      $40                     ; VRAM write
    out     ($BF),a
    ei
    pop     hl
vr_tile:
    ld      c,32
vr_byte:
    ld      a,(hl)
    out     ($BE),a
    inc     hl
    dec     c
    jr      nz,vr_byte
    djnz    vr_tile
    jr      _vdp_runs

; crt0 clears VRAM before main. z88dk's clear_vram starts its counter at
; $4000 with L = 0 and so writes the 16K four times over -- most of a second
; of black screen; defining the symbol here keeps that module out of the
; link. The display is still off, so the writes need no spacing.

    PUBLIC  clear_vram
    PUBLIC  _clear_vram

clear_vram:
_clear_vram:
    xor     a
    out     ($BF),a
    ld      a,$40                   ; VRAM write from $0000
    out     ($BF),a
    xor     a
    ld      c,$40                   ; 64 pages of 256
cv_page:
    ld      b,a
cv_byte:
    out     ($BE),a
    djnz    cv_byte
    dec     c
    jr      nz,cv_page
    ret

    SECTION bss_user

_blitCount:
    defw    0
