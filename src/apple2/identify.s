.export		_identify
.import		_detect_iigs
.include	"identify.inc"

IDBYTE1 := $fbb3
IDBYTE2 := $fb1e
IDBYTE3 := $fbc0
IDBYTE4 := $fbbe

; a2f_identify       identify machine type of apple 2 family. 
;                    result returns in Accumlator (lo) and X-register (hi)
;
.proc		_identify
	
		jsr	_detect_iigs		; returns 1 if running on IIGS, otherwise 0
		beq	t_eight_bit
		lda	#TYPE_IIGS
		ldx	#0
		rts
t_eight_bit:
		bit	$c082
		lda IDBYTE1
   		cmp #$06                ; IIe or IIc/IIc+ 
   		bne t_other
    	lda IDBYTE3 
	    cmp #$00                ; IIc? 
   		bne t_IIe 
                            	; IIc/IIc+
		lda	#TYPE_IIC
		ldx	#0
		rts
t_IIe:
		lda	#TYPE_IIE
		ldx	#0			; Set upper byte to 0
		rts
t_other:
		lda	#TYPE_II
		ldx	#0			; Set upper byte to 0
		rts
.endproc 