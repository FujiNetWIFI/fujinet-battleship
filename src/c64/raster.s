.export _irqVsyncWait

.segment "CODE"

;Bottom Raster
RASTER2 = (8 * 25) + 50
continueFlag:
        .BYT $00
;------------------------------------------------------------------------------
; void registerIrq(void)
; https://www.c64-wiki.com/wiki/Raster_interrupt
_irqVsyncWait:
        LDA #0               ; set flag to block continuation of main program
        STA continueFlag

        SEI                  ; Disable interrupts
        LDA #%01111111       ; Interrupt release on all bits
        STA $DC0D            ; CIA Interrupt control and status register

        LDA #$1b             ; clear most significant bit of
        STA $D011            ; VIC Control Register 1

        LDA $DC0D            ; acknowledge pending interrupts from CIA-1
        LDA $DD0D            ; acknowledge pending interrupts from CIA-2

        LDA #RASTER2         ; set rasterline where interrupt shall occur in the
        STA $D012

        LDA #<Irq1           ; set interrupt vectors, pointing to interrupt service routine below
        STA $0314
        LDA #>Irq1
        STA $0315

        LDA #1               ; enable raster interrupt signals from VIC
        STA $D01A

        CLI                  ; clear interrupt flag, allowing the CPU to respond to interrupt requests
waitLoop:
        LDA continueFlag
        BEQ waitLoop         ; wait until interrupt sets continueFlag to non-zero
        RTS

;------------------------------------------------------------------------------
; Interrupts
Irq1:
        LDA #1               ; set flag to allow continuation of main program
        STA continueFlag

        ASL $D019            ; acknowledge the interrupt by clearing the VIC's interrupt flag

        LDA #%01111111       ; Interrupt release on all bits
        STA $DC0D            ; CIA Interrupt control and status register

        LDA #$1b             ; clear most significant bit of
        STA $D011            ; VIC Control Register 1

        LDA $DC0D            ; acknowledge pending interrupts from CIA-1
        LDA $DD0D            ; acknowledge pending interrupts from CIA-2

        LDA #$31             ; set interrupt vectors, pointing to interrupt service routine below
        STA $0314
        LDA #$ea
        STA $0315

        JMP $EA81            ; jump into shorter ROM routine to only restore registers from the stack etc.
