; ---------------------------------------------------------------------------
; strings.inc -- building one screen row, and turning a u16 into digits.
;
; The builder keeps its write cursor in DC1 and its source in DC0, alternating
; with XDC. That is the same dance FNFIX does, and it is what the F8 gives you:
; two data counters, one of which is always the one LM and ST use.
;
;   DCI VLINE / XDC          start a row
;   DCI <source> / PI SBCAT  append a NUL-terminated string
;   PI SBCH                  append the character in r0
;   PI SBEND                 terminate
; ---------------------------------------------------------------------------

VD_V0	EQU 08150H		; DEC5 working value
VD_V1	EQU 08151H
VD_P0	EQU 08152H		; the power of ten being subtracted
VD_P1	EQU 08153H
VD_DIG	EQU 08154H
VD_IDX	EQU 08155H
VD_LEAD	EQU 08156H		; has a significant digit been emitted yet?
VD_T0	EQU 08157H		; two's complement scratch
VD_T1	EQU 08158H

; SBCAT -- append the NUL-terminated string at DC0 to the cursor in DC1.
;   Leaves the cursor on the last byte written, so appends chain.
;   Leaf. Clobbers A, DC0, DC1.
SBCAT:	LM
	CI 0
	BZ SBCX
	XDC
	ST
	XDC
	BR SBCAT
SBCX:	POP

; SBCH -- append r0. Leaf. Clobbers A, DC0, DC1.
SBCH:	XDC
	LR A,0
	ST
	XDC
	POP

; SBEND -- terminate. Leaf. Clobbers A, DC0, DC1.
SBEND:	XDC
	CLR
	ST
	XDC
	POP

; SUB16 -- if VD_V >= VD_P, subtract and return A = 0; otherwise leave VD_V
;   alone and return A = 1.
;
;   The F8 has no subtract, so this adds the two's complement and reads the
;   carry out: set means no borrow. LM, DCI, LR and ST all leave the flags
;   alone, which is what lets the carry from the low byte survive the four
;   memory accesses between it and the LNK that consumes it.
;
;   The high byte needs BOTH carries. LNK adds the low half's carry and can
;   itself carry out ($FF + 1), and the AS that follows then reports no carry
;   -- so taking only the AS loses it. That is not a corner case: subtracting 1
;   from 189 hits it, and the symptom was a decimal formatter that printed "0"
;   for every value because no subtraction ever appeared to fit.
;   Leaf. Clobbers A, r4, r5, r6, r7, DC0.
SUB16:	DCI VD_P0
	LM
	COM
	INC			; -P low; carry set if P low was 0
	DCI VD_T0
	ST
	DCI VD_P1
	LM
	COM
	LNK			; ~P high plus that carry
	DCI VD_T1
	ST

	DCI VD_V0
	LM
	LR 4,A
	DCI VD_T0
	LM
	AS 4
	LR 5,A			; tentative low
	DCI VD_V1
	LM
	LR 4,A
	DCI VD_T1
	LM
	LNK			; carry from the low add
	LR 6,A
	LI 0
	LNK			; and capture THIS add's carry before it is lost
	LR 7,A
	LR A,6
	AS 4
	LR 6,A			; tentative high
	BC SUB16O		; carry out of the second add...
	LR A,7
	CI 0
	BNZ SUB16O		; ...or out of the first: either is a carry out
	LI 1
	POP
SUB16O:	DCI VD_V0
	LR A,5
	ST
	LR A,6
	ST
	CLR
	POP

; DEC5 -- format the u16 in r0:r1 into VDIGIT, leading zeros suppressed.
;   Reached by PI; calls one level down.
DEC5:	ENTER
	DCI VD_V0
	LR A,0
	ST
	LR A,1
	ST
	DCI VD_LEAD
	CLR
	ST
	DCI VD_IDX
	CLR
	ST
	DCI VDIGIT
	XDC			; DC1 = the write cursor

DEC5P:	DCI VD_IDX
	LM
	SL 1			; two bytes per entry
	LR 4,A
	DCI POW10
	LR A,4
	ADC
	LM
	LR 5,A
	LM
	LR 6,A
	DCI VD_P0
	LR A,5
	ST
	LR A,6
	ST
	DCI VD_DIG
	CLR
	ST

DEC5S:	PI SUB16
	CI 0
	BNZ DEC5D
	DCI VD_DIG
	LM
	INC
	DCI VD_DIG
	ST
	BR DEC5S

DEC5D:	DCI VD_DIG
	LM
	LR 4,A
	CI 0
	BNZ DEC5E
	DCI VD_LEAD
	LM
	CI 0
	BNZ DEC5E
	DCI VD_IDX
	LM
	CI 4
	BNZ DEC5N		; a leading zero anywhere but the units place
DEC5E:	LIS 1
	DCI VD_LEAD
	ST
	LR A,4
	AI '0'
	XDC
	ST
	XDC

DEC5N:	DCI VD_IDX
	LM
	INC
	DCI VD_IDX
	ST
	CI 5
	BNZ DEC5P
	XDC
	CLR
	ST
	XDC
	LEAVE

POW10:	DB 10000&0FFH,10000>>8
	DB 1000&0FFH,1000>>8
	DB 100&0FFH,100>>8
	DB 10,0
	DB 1,0
