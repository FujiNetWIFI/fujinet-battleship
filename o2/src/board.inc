; board.inc -- the board screen: ten rows of ten cells (fixed.inc row_write)
; between T0 at the top and T1 under them, on the sea. The rows' tables are
; the fixed half's page $6 (ROWS); a cell is its (pointer, control) there,
; written through POKE. One 256-byte page: cell_ptr and row_draw read this
; page's tables.

; Cell codes: what a cell shows (cellg, cellc).
CL_WATER equ 0                  ; nothing known
CL_SHIP  equ 1                  ; one of ours
CL_SHOT  equ 2                  ; one of ours, hit
CL_MISS  equ 3                  ; a miss
CL_HIT   equ 4                  ; a hit on theirs
CL_PLACE equ 5                  ; the ship being placed
CL_BAD   equ 6                  ; ... where it may not go
CL_PART  equ 7                  ; some of them have it marked (the target)
CL_SUNK  equ 8                  ; a ship of the winner's, shown at the end
CL_SEA   equ 9                  ; 9-12: water on seat 0-3's board, in its colour

; scr_board: the board screen -- every cell water, T0 and T1 blank, the
; sprites hidden. Rows first, then the band list: a frame between them
; draws the old screen. Clobbers A, r0-r7.
scr_board:
    mov r4,#0
sb_row:
    mov a,r4                    ; the row's table: Y ...
    call row_at
    dec a
    call poke_at
    mov a,r4
    add a,#rowy&0FFh
    movp a,@a
    call poke_b
    mov r6,#NCOLS               ; ... and ten cells of water
sb_cell:
    mov r0,#SCR0-1
    mov a,r6
    add a,r0
    mov r0,a
    mov a,#CL_WATER
    movx @r0,a
    djnz r6,sb_cell
    call row_draw
    inc r4
    mov a,r4
    xrl a,#NCOLS
    jnz sb_row
    call bands_board            ; the colour and the band list
    call spr_hide
    mov r1,#MODE
    mov a,#MODE_BOARD|MODE_SETUP
    mov @r1,a
    call lb_clear
    mov r4,#L_TOP
    call say_line
    call lb_clear
    mov r4,#L_BOT
    jmp say_line

; row_at: A = row A's first cell in page $6 (low byte); r2 = the page.
; Clobbers r6.
row_at:
    mov r6,a                    ; ROWS + 21A + 1
    rl a
    rl a
    add a,r6                    ; 5A
    rl a
    rl a                        ; 20A
    add a,r6
    add a,#(ROWS&0FFh)+1
    mov r2,#ROWS>>8
    ret

; row_draw: row r4 = the ten cell codes at SCR0. Clobbers A, r0-r3, r5-r7.
row_draw:
    mov a,r4
    call row_at
    call poke_at
    mov a,r4                    ; the bias: 3 - Y/2
    add a,#rowy&0FFh
    movp a,@a
    clr c
    rrc a
    cpl a
    add a,#1+CELL_ROW0
    mov r5,a
    mov r7,#SCR0
rd_l:
    mov a,r7
    mov r0,a
    movx a,@r0
    call cell_ptr
    mov a,r3
    call poke_b
    mov a,r1
    call poke_b
    inc r7
    mov a,r7
    xrl a,#SCR0+NCOLS
    jnz rd_l
    ret

; cell_ptr: cell code A on the row whose bias is r5 (3 - Y/2) -> its
; pointer, low byte, in r3 and its control byte in r1. The VDC adds Y/2 back
; while it draws, so the register holds glyph*8 + 3 - Y/2, nine bits; bit 8
; is the control byte's bit 0. Clobbers A, r0.
cell_ptr:
    mov r0,a
    add a,#cellg&0FFh
    movp a,@a                   ; the glyph, 0-63
    mov r1,a
    rl a                        ; glyph*8: bits 7-3 of the low byte ...
    rl a
    rl a
    anl a,#0F8h
    add a,r5                    ; ... plus the bias; C = no borrow
    mov r3,a
    mov a,r1                    ; ... and bit 8 is the glyph's bit 5,
    swap a                      ; less the borrow
    rr a
    anl a,#1
    jc cp_nb
    xrl a,#1
cp_nb:
    mov r1,a
    mov a,r0
    add a,#cellc&0FFh
    movp a,@a
    rl a                        ; the colour into bits 3-1
    orl a,r1
    mov r1,a
    ret

    ifdef CURSOR
; spr_cursor: sprite 0 over cell (r3 = column, r4 = row), or hidden when r2
; is 0. Clobbers A, r0-r2, r5, r6.
spr_cursor:
    mov a,r2
    jz scu_off
    mov a,r4
    add a,#rowy&0FFh
    movp a,@a
    mov r5,a                    ; Y
    mov a,r3
    rl a
    rl a
    rl a
    add a,#X_BOARD              ; X
    jmp scu_put
scu_off:
    mov r5,#0F8h
    clr a
scu_put:
    mov r6,a
    mov r2,#SPRTAB>>8
    mov a,#SPRTAB&0FFh
    call poke_at
    mov a,r5
    call poke_b
    mov a,r6
    call poke_b
    mov a,#C_TAN<<3
    jmp poke_b
    endif

; The rows' Y.
rowy:
Y_ROWK  set Y_ROW0
    rept NCOLS
    db Y_ROWK
Y_ROWK  set Y_ROWK+PITCH
    endm

; A cell code's glyph and colour.
cellg:
    db G_DASH, G_BLOCK, G_BLOCK, G_DOT, G_BALL, G_BLOCK, G_BLOCK, G_DOT, G_BLOCK
    db G_DASH, G_DASH, G_DASH, G_DASH
cellc:
    db C_BLUE, C_WHITE, C_RED, C_WHITE, C_RED, C_GREEN, C_VIOLET, C_DGREY, C_WHITE
    db C_CYAN, C_GREEN, C_TAN, C_VIOLET
