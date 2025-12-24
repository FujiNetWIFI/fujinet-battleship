        .export     _waitvsync
        .export     _setVsyncProc
        .export     _vsyncCounter

; IIc
; It sets the CPU IRQ mask only during the actual wait.
; It restores the VBL interrupt mask to the previous state.
RDVBLBAR := $c019 ; >127 if not vertical blanking
RDVBLMSK := $c041 ; >127 if VBL interrupts enabled
DISVBL := $c05a ; Disable VBL interrupts
ENVBL := $c05b ; Enable VBL interrupts
PTRIG := $c070
IOUDISON := $c07e ; Disable IOU
IOUDISOFF := $c07f ; Enable IOU
.include	"identify.inc"

.data
_vsyncCounter:
        .word   0   ; 16-bit counter for VSYNC-based timer
                    ; Wraps around after ~18 minutes at 60Hz (not an issue for 20-second game timers)

.proc       _setVsyncProc
        ; Argument: machine type in A register (TYPE_II, TYPE_IIE, TYPE_IIC, TYPE_IIGS)
        cmp #TYPE_II
        beq set_II
        cmp #TYPE_IIE
        beq set_IIe
        cmp #TYPE_IIC
        beq set_IIc
        cmp #TYPE_IIGS
        beq set_IIGS
        brk                     ; boom!

set_IIc:
        lda #<IIc
        sta _waitvsync+1
        lda #>IIc
        sta _waitvsync+2
        rts
set_IIe:
        lda #<IIe
        sta _waitvsync+1
        lda #>IIe
        sta _waitvsync+2
        rts
set_IIGS:
        lda #<IIGS
        sta _waitvsync+1
        lda #>IIGS
        sta _waitvsync+2
        rts
set_II:
        lda #<II
        sta _waitvsync+1
        lda #>II
        sta _waitvsync+2
        rts
.endproc

.proc       _waitvsync
        jmp II           ; Initial value is II, dynamically rewritten at runtime
.endproc

IIe:
@skip:     
        bit RDVBLBAR
        bpl @skip
@wait:  
        bit RDVBLBAR
        bmi @wait
        inc _vsyncCounter      ; Increment low byte
        bne @skip_inc          ; If not zero, skip high byte increment
        inc _vsyncCounter+1    ; Increment high byte on carry
@skip_inc:
        rts
IIGS:
@skip: 
        bit RDVBLBAR
        bmi @skip
@wait: 
        bit RDVBLBAR
        bpl @wait
        inc _vsyncCounter      ; Increment low byte
        bne @skip_inc2         ; If not zero, skip high byte increment
        inc _vsyncCounter+1    ; Increment high byte on carry
@skip_inc2:
        rts
II:
        inc _vsyncCounter      ; Increment low byte
        bne @skip_inc3         ; If not zero, skip high byte increment
        inc _vsyncCounter+1    ; Increment high byte on carry
@skip_inc3:
        rts        
IIc:
        sei
        sta IOUDISOFF
        lda RDVBLMSK
        bit ENVBL
        bit PTRIG ; Reset VBL interrupt flag
@wait:
        bit RDVBLBAR
        bpl @wait
        asl
        bcs @skip ; VBL interrupts were already enabled
        bit DISVBL
@skip:
        sta IOUDISON ; IIc Tech Ref Man: The firmware normally leaves IOUDIS on.
        cli
        inc _vsyncCounter      ; Increment low byte
        bne @skip_inc4         ; If not zero, skip high byte increment
        inc _vsyncCounter+1    ; Increment high byte on carry
@skip_inc4:
        rts