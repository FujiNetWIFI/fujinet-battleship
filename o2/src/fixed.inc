; fixed.inc -- the fixed half, $400-$7FF: the display engine.
;
; The 8244's 28 characters a frame are re-pointed between lines of text and
; rows of the board by a line-counter interrupt, and the 8048 holds A11 at 0
; for a whole interrupt routine -- so everything here runs below $800, and
; every table an interrupt reads with MOVP sits in the page of the code that
; reads it. The tables are cartridge RAM: the main program writes them through
; the mailbox's POKE register (disp.inc) and never touches the VDC's object
; registers itself (the sprites' shapes aside).
;
; A screen is a band list (page $7): one counter interrupt per band. The
; vblank interrupt draws the sprites and text line T0 (the four quads), then
; arms the first band. Two kinds of band:
;   h_txt   a 16-character text line (the four quads) from its table in
;           page $5: T1-T5 on a text screen, T1 under a board
;   h_row   a board row: singles 0-9 from ROWBUF, then the next row's table
;           into ROWBUF -- internal RAM is the only memory fast enough for
;           the gap (row_write)
;
; Assembled into every page's build; build.sh takes it from page 0's and
; checks the others agree.

CTL_ON  equ vdc_ctrl_fore
CTL_OFF equ 0

; ISR_IN / ISR_VDC / ISR_OFF / ISR_ON / ISR_OUT: the frame round every band.
; Register bank 1; P1 saved in r6, A in r7.
ISR_IN  macro
    sel rb1
    mov r7,a
    endm
ISR_VDC macro
    in a,p1
    mov r6,a
    mov a,#P1_VDC
    outl p1,a
    endm
ISR_OFF macro                   ; foreground off: objects may be written
    mov r0,#vdc_control
    mov a,#CTL_OFF
    movx @r0,a
    endm
ISR_ON  macro                   ; foreground on again
    mov r0,#vdc_control
    mov a,#CTL_ON
    movx @r0,a
    endm
ISR_OUT macro
    mov a,r6
    outl p1,a
    mov a,r7
    retr
    endm

; ===========================================================================
;  page $4: vectors, boot, page switching, the board row writer
; ===========================================================================
    org 400h
    jmp start                   ; $400 reset
    jmp vbl_isr                 ; $402 external IRQ: the VDC's vblank
    jmp tim_isr                 ; $404 counter overflow: a band has ended
    jmp start                   ; $406 (the BIOS's own vblank chain: unused)
    jmp start                   ; $408 (after the BIOS's game select: unused)
    retr                        ; $40A
    nop

; start: a console RESET does not reset the cartridge, so the page mapped and
; every table survive it; boot puts back what it needs.
start:                          ; (a reset leaves interrupts off, the counter
    mov a,#P1_MAIN              ; stopped)
    outl p1,a
    clr a
    call sel_page               ; page 0
    mov r0,#ROWBUF              ; internal RAM $20-$3C
    mov r2,#03Dh-ROWBUF
    clr a
st_clr:
    mov @r0,a
    inc r0
    djnz r2,st_clr
st_boot:
    jmp 800h                    ; page 0's entry 0 (A = 0): the boot

; sel_page: map page A at $800-$F1F. Main program, P1 = P1_MAIN.
; Clobbers A, r0, r1.
sel_page:
    mov r1,#CURPG
    mov @r1,a
    mov r0,#FN_W_REGSEL
    mov a,#FN_REG_CODEBANK
    movx @r0,a
    inc r0
    mov a,@r1
    movx @r0,a
    ret

; farcall: entry A of page r7, then the caller's page back. From a page:
;     mov r7,#PG_x / mov a,#E_y / FCALL farcall
; One level: an entry it runs may not farcall. Results come back in r2-r5 or
; external RAM (A and r0, r1, r6 are spent getting home). Clobbers r6.
farcall:
    mov r6,a
    mov r1,#CURPG
    mov a,@r1
    mov r1,#OLDPG
    mov @r1,a
    mov a,r7
    call sel_page
    mov a,r6
    call 800h                   ; the page's dispatcher (page.inc)
    sel mb0
    mov r1,#OLDPG
    mov a,@r1
    call sel_page
    ret

; vdc_reset: display off and every object parked. Main program, at boot:
; FCALL. Clobbers A, r0, r2.
vdc_reset:
    mov a,#P1_VDC
    outl p1,a
    mov r0,#vdc_control
    clr a
    movx @r0,a
    mov r0,#0                   ; $00-$9F: sprites, characters, quads, shapes
    mov r2,#0A0h
vr_obj:
    movx @r0,a
    inc r0
    djnz r2,vr_obj
    mov r0,#vdc_soundctrl
    movx @r0,a
    mov a,#P1_MAIN
    outl p1,a
    ret

; row_write: one board row out of ROWBUF into singles 0-9 -- Y, then each
; one's (pointer, control); their X is the screen's (obj_setup). VDC
; selected, foreground off. About 155 cycles, from internal RAM: MOVP from a
; table costs two more a byte. Clobbers A, r0-r2.
row_write:
    mov r1,#ROWBUF
    mov a,@r1
    mov r2,a                    ; Y
    inc r1
    mov r0,#vdc_char0
    rept NCOLS
    mov a,r2
    movx @r0,a                  ; Y
    inc r0
    inc r0                      ; (X)
    mov a,@r1
    movx @r0,a                  ; pointer
    inc r0
    inc r1
    mov a,@r1
    movx @r0,a                  ; control
    inc r0
    inc r1
    endm
    ret

; snd_vbl: the vblank's sound, A = SNDCNT (nonzero), r1 at it. A queued one
; waits for the main loop; a playing one counts down and stops at 0. VDC
; selected. Clobbers A, r0.
snd_vbl:
    jb7 sv_x
    dec a
    mov @r1,a
    jnz sv_x
    mov r0,#vdc_soundctrl
    movx @r0,a
sv_x:
    ret

; obj_setup: a new screen's singles, A = MODE: X at the board's columns,
; and Y parked -- on a board, the rows place 0-9 and only 10 and 11 park.
; VDC selected, foreground off. Clobbers A, r0-r3.
obj_setup:
    mov r1,a
    mov r0,#vdc_char0
    mov r3,#X_BOARD
    mov r2,#12
os_l:
    mov a,r1
    jb0 os_x
    mov a,#0F8h
    movx @r0,a
os_x:
    inc r0
    mov a,r3
    movx @r0,a
    add a,#8
    mov r3,a
    inc r0
    inc r0
    inc r0
    djnz r2,os_l
    mov a,#0F8h
    mov r0,#vdc_chara
    movx @r0,a
    mov r0,#vdc_charb
    movx @r0,a
    ret

    if $ > 500h
    error "page $4 has left the page"
    endif

; ===========================================================================
;  page $5: the text writer and lines T0-T5
; ===========================================================================
    org 500h

; txt_write: the 16-character line whose table is at r1 in this page, into
; the four quads interleaved: A takes columns 0, 2, 4, 6, B 1, 3, 5, 7, C and
; D the next eight (disp.inc txorder). VDC selected, foreground off. About
; 360 cycles. Clobbers A, r0-r4.
txt_write:
    mov a,r1
    movp a,@a
    mov r4,a                    ; Y
    inc r1
    mov r0,#vdc_quad0
    mov r3,#X_TEXT
    call tq
    mov r0,#vdc_quad1
    mov r3,#X_TEXT+8
    call tq
    mov r0,#vdc_quad2
    mov r3,#X_TEXT+64
    call tq
    mov r0,#vdc_quad3
    mov r3,#X_TEXT+72
tq:
    mov a,r4                    ; Y and X are mirrored across a quad
    movx @r0,a
    inc r0
    mov a,r3
    movx @r0,a
    inc r0
    mov r2,#4
tql:
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r1
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r1
    inc r0
    inc r0
    inc r0
    djnz r2,tql
    ret

T0: db TXT_TAB dup (0)          ; text lines: TXT_TAB bytes each
T1: db TXT_TAB dup (0)
T2: db TXT_TAB dup (0)
T3: db TXT_TAB dup (0)
T4: db TXT_TAB dup (0)
T5: db TXT_TAB dup (0)
    if $ > 600h
    error "page $5's tables have left the page"
    endif

; ===========================================================================
;  page $6: the board's rows
; ===========================================================================
    org 600h

; row_copy: the row table at offset A of this page into ROWBUF. Main or
; interrupt. About 135 cycles. Clobbers A, r0-r2.
row_copy:
    mov r1,a
    mov r0,#ROWBUF
    mov r2,#ROW_TAB
rc_l:
    mov a,r1
    movp a,@a
    mov @r0,a
    inc r0
    inc r1
    djnz r2,rc_l
    ret

; fx6_get: A = offset in page $6 -> the byte there (main program: FCALL).
fx6_get:
    movp a,@a
    ret

; beep: queue sound A (SND_*) for the main loop (snd.inc snd_go), which
; plays it from its table; the vblank counts it down and stops it. SNDCNT:
; bit 7 a sound queued (its id below), else the frames it has left. The
; soft ones (SND_SOFT on) never cut another sound. Main program: FCALL.
; Clobbers A, r0, r1.
beep:
    mov r0,a
    mov r1,#SNDCNT
    add a,#256-SND_SOFT
    jnc bp_q
    mov a,@r1
    jnz bp_x
bp_q:
    mov a,r0
    orl a,#80h
    mov @r1,a
bp_x:
    ret

ROWS:                           ; 10 x ROW_TAB, row 0 first
    db NCOLS*ROW_TAB dup (0)
    if $ > 700h
    error "page $6's tables have left the page"
    endif

; ===========================================================================
;  page $7: the bands and both interrupts; the sprites
; ===========================================================================
    org 700h

; The band list, poked per screen (disp.inc). BL_FIRST is 256 minus the line
; of the first band; each band is (handler slot, argument, 256 - lines to the
; next band, or 0 for the last).
SCRCOL  equ 700h                ; the colour register for this screen
BL_FIRST equ 701h
BANDS   equ 702h
NBANDS  equ 11
    db 0, 0
    db NBANDS*3 dup (0)

; vbl_isr: once a frame, in the blank. Arms the first band -- exact anywhere
; in the blank, because the counter only counts display lines (T1 is
; vblank|hblank) -- then the sprites, T0 and, on a board screen, row 0
; staged for the first band. The interrupt nests two calls deep, so the main
; program may nest five.
vbl_isr:
    ISR_IN
    ISR_VDC
    mov r0,#vdc_status
    movx a,@r0                  ; reading the status drops the IRQ
    mov a,#BL_FIRST&0FFh
    movp a,@a
    mov t,a
    mov r5,#BANDS&0FFh
    mov r1,#SNDCNT              ; the sound's frame
    mov a,@r1
    jz vb_q
    call snd_vbl
vb_q:
    mov r1,#FRAMES
    inc @r1
    ISR_OFF
    mov a,#SCRCOL&0FFh
    movp a,@a
    mov r0,#vdc_color
    movx @r0,a
    call spr_write
    mov r1,#T0&0FFh
    call txt_write
    mov r1,#MODE
    mov a,@r1
    jb1 vb_setup
vb_s1:
    jb0 vb_rows
vb_on:
    ISR_ON
    ISR_OUT
vb_setup:
    anl a,#~MODE_SETUP
    mov @r1,a
    call obj_setup
    mov r1,#MODE
    mov a,@r1
    jmp vb_s1
vb_rows:
    mov a,#ROWS&0FFh
    call row_copy
    jmp vb_on

; tim_isr: a band has ended. The counter is re-armed first, plus what it
; has counted since the overflow: a band that starts late (a long vblank)
; doesn't make the next one late. The last band ticks QUIET: the frame's
; stretch with nothing due (input.inc frame_sync). Then the handler runs.
tim_isr:
    ISR_IN
    mov a,r5
    add a,#2
    movp a,@a
    mov r3,a
    mov a,t
    add a,r3
    mov t,a
    mov a,r3
    jnz ti_go
    mov r1,#QUIET
    inc @r1
ti_go:
    mov a,r5
    inc a
    movp a,@a
    mov r3,a                    ; the argument
    mov a,r5
    add a,#3
    xch a,r5                    ; the next band
    movp a,@a
    jmpp @a                     ; the handler slot -> the handler
jtab:
J_ROW   equ $&0FFh
    db h_row&0FFh
J_TXT   equ $&0FFh
    db h_txt&0FFh
J_NONE  equ $&0FFh
    db h_none&0FFh

; h_none: a band past the end of the list.
h_none:
    clr a
    mov t,a
    mov a,r7
    retr

; h_row: write the row staged in ROWBUF, then stage the one at offset r3 of
; page $6 (0FFh: none).
h_row:
    ISR_VDC
    ISR_OFF
    call row_write
    ISR_ON
    mov a,r3
    inc a
    jz hr_done
    dec a
    call row_copy
hr_done:
    ISR_OUT

; h_txt: the line whose table is at offset r3 of page $5.
h_txt:
    ISR_VDC
    ISR_OFF
    mov a,r3
    mov r1,a
    call txt_write
    ISR_ON
    ISR_OUT

; spr_write: the four sprites' (Y, X, attribute) from SPRTAB. Their shapes
; the main program writes itself: the VDC takes those with the display on.
; VDC selected, foreground off. Clobbers A, r0-r2.
spr_write:
    mov r1,#SPRTAB&0FFh
    mov r0,#vdc_spr0_ctrl
    mov r2,#4
sw_l:
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r1
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r1
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r0
    inc r1
    djnz r2,sw_l
    ret

SPRTAB:                         ; 4 x (Y, X, attribute)
    db 12 dup (0)

    if $ > 800h
    error "page $7's tables have left the page"
    endif
