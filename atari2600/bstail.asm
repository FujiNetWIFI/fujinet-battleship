; bstail.asm -- the fixed half: the cold stub and the bank trampoline.
;
; $1800-$1F1F is the cartridge's mailbox -- text planes, reply window, control
; page, TX page, status -- and the cartridge paints it. The client owns
; $1F20-$1FFB, which fuji_mailbox.h calls the fixed tail, plus the vectors.
;
; Both routines here have to be at addresses that do not move. The store that
; switches bank is the last instruction fetched from the OLD bank and the very
; next fetch comes from the new one, so the jump after it cannot be in a bank.
; And this console has no reset line to the cartridge: the RESET switch
; restarts the 6507 with whatever bank was last selected still mapped, so a
; cold stub living in bank 0 would simply not be there when it was needed.

        CPU     6502
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "bsdefs.inc"

; BSGOTO is not a label: bsdefs.inc gives it a fixed address and this ORG is
; what makes that true, so the two cannot drift apart.
        ORG     BSGOTO

; ---------------------------------------------------------------------------
; BSGOTO -- select bank A and enter it at $1000.
;
; `sta FNRSEL,x` is the documented-safe indexed form: the base low byte is $00
; so the index cannot carry, which means the dummy read STA abs,X always
; performs lands on the same address as the write.
        clc
        adc     #FH_BANK
        tax
        sta     FNRSEL,x
        jmp     $1000

; ---------------------------------------------------------------------------
; BSCOLD -- power-on and RESET.
;
; The gate is opened inline because BSGOTO needs it: banking is a control-page
; op and the control page is dead until an ordered pair of stores carrying two
; specific values arrives.
;
; A console RESET during a game restarts here and goes back to the table list.
; That is the right behaviour and not a limitation: the cartridge keeps its
; sequence number across the reset -- see FNGO -- so the very next transaction
; continues the conversation rather than colliding with one already answered.
BSCOLD: sei
        cld
        ldx     #$FF
        txs
        lda     #0
BSCL1:  sta     $00,x           ; $00-$7F is the TIA, $80-$FF is RAM
        dex
        bne     BSCL1
        sta     $00

        lda     #2
        sta     VBLANK

        lda     #FNAM1
        sta     FNRSEL+FH_ARM1
        lda     #FNAM2
        sta     FNRSEL+FH_ARM2

        lda     #BANKLOB
        jmp     BSGOTO

        ORG     $1FFC
        DW      BSCOLD
        DW      BSCOLD

        END
