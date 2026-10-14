; ---------------------------------------------------------------------------
; lobby.inc -- choosing a table.
;
; The reply is a COUNT byte and then that many 36-byte records -- the records
; start at offset 1, not 0. Dividing the reply length by the stride looks
; equivalent and is not: 181 bytes is 1 + 5*36, and the division says 5 only by
; accident of rounding.
;
; Joining a table is not a call. Retrieving the state for one IS the join, so
; this screen does nothing but copy the id into VTABLE and hand over.
; ---------------------------------------------------------------------------

; ---------------------------------------------------------------------------
STABLE:	LI LAY_TXT
	LR 0,A
	PI LAYLD
	LI CVAL3
	LR 3,A
	DCI SCHOOSE
	XDC
	PI UITITLE
	PI UICLRL
	LI CVAL1
	LR 3,A
	DCI SJOINK
	XDC
	PI UIFOOT

	PI TBFTCH
	OI 0
	BZ TBOK
	DCI VFAILC
	ST
	JMP FAIL

TBOK:	DCI FNREPLY
	LM
	DCI VTCNT
	ST
	DCI VTTOP
	CLR
	ST
	DCI VCUR
	CLR
	ST
	PI TBDRAW

TB1:	PI INSCAN
	LR A,0
	CI EV_NONE
	BZ TB1
	LR A,0
	CI EV_UP
	BNZ TBK1
	DCI VCUR
	LM
	OI 0
	BZ TBPGU
	AI 0FFH
	DCI VCUR
	ST
	PI UICUR
	BR TB1
TBK1:	CI EV_DOWN
	BNZ TBK2
	DCI VCUR
	LM
	INC
	LR 1,A
	DCI VNROW
	LM
	XS 1
	BZ TBPGD		; past the last row on this page
	DCI VCUR
	LR A,1
	ST
	PI UICUR
	BR TB1
TBK2:	CI EV_PGUP
	BZ TBPGU
	CI EV_PGDN
	BZ TBPGD
	CI EV_BACK
	BNZ TBK3
	LI ST_EDIT		; MODE goes back and renames
	LR 0,A
	JMP GOTO
TBK3:	CI EV_SEL
	BNZ TB1
	DCI VNROW
	LM
	OI 0
	BZ TB1			; nothing on the page to join
	JMP TBJOIN

TBPGU:	DCI VTTOP
	LM
	OI 0
	BZ TB1
	DCI VTTOP
	LM
	AI 256-NROWS
	DCI VTTOP
	ST
	DCI VCUR
	CLR
	ST
	PI TBDRAW
	JMP TB1			; out of BR's +/-127 reach

TBPGD:	DCI VTTOP		; only if there is another page to go to
	LM
	AI NROWS
	LR 1,A
	DCI VTCNT
	LM
	COM
	INC
	AS 1
	BNC TBPGD1		; top + NROWS < count: there is another page
	JMP TB1
TBPGD1:	DCI VTTOP
	LM
	AI NROWS
	DCI VTTOP
	ST
	DCI VCUR
	CLR
	ST
	PI TBDRAW
	JMP TB1

; ---------------------------------------------------------------------------
; TBFTCH -- GET /tables. The floor is one byte: any nonempty reply carries the
;   count that bounds the walk, so there is nothing better to demand.
;   Reached by PI; calls two levels down.
; ---------------------------------------------------------------------------
TBFTCH:	ENTER
	LI RQTABLE
	DCI VREQ
	ST
	DCI VMIN0
	LIS 1
	ST
	CLR
	ST
	LI TMAXLEN&0FFH
	LR 0,A
	LI TMAXLEN>>8
	LR 1,A
	PI NFETCH
	LEAVE

; ---------------------------------------------------------------------------
; TBREC -- DC0 = the base of table record r0. Offset 1, past the count byte.
;   Leaf. Clobbers A, r0, DC0.
; ---------------------------------------------------------------------------
TBREC:	DCI FNREPLY+1
	LR A,0
	OI 0
	BZ TBRX
TBR1:	LI TBSTRID
	ADC
	DS 0
	BNZ TBR1
TBRX:	POP

; ---------------------------------------------------------------------------
; TBDRAW -- one page of the list.
;   Reached by PI; calls three levels down.
; ---------------------------------------------------------------------------
TBDRAW:	ENTER
	PI UICLRL
	DCI VNROW
	CLR
	ST
	DCI VTMP
	CLR
	ST
TBD1:	DCI VTTOP
	LM
	LR 0,A
	DCI VTMP
	LM
	AS 0
	LR 1,A
	DCI VTCNT
	LM
	COM
	INC
	AS 1			; record - count
	BC TBDX			; no borrow: off the end of the list
	PI TBROW
	DCI VNROW
	LM
	INC
	DCI VNROW
	ST
	DCI VTMP
	LM
	INC
	DCI VTMP
	ST
	CI NROWS
	BNZ TBD1
TBDX:	DCI VNROW
	LM
	OI 0
	BNZ TBDC
	LI CVAL2		; an empty list still has to say something
	LR 3,A
	CLR
	LR 0,A
	DCI SNOTBL
	XDC
	PI UIROW
	LEAVE
TBDC:	PI UICUR
	LEAVE

; ---------------------------------------------------------------------------
; TBROW -- list row VTMP: the table's name, and its occupancy right-aligned.
;   Reached by PI; calls three levels down.
; ---------------------------------------------------------------------------
TBROW:	ENTER
	DCI VTTOP
	LM
	LR 0,A
	DCI VTMP
	LM
	AS 0
	DCI VTMP2
	ST			; the record index, where a DCI cannot reach it

	LI TBNAME
	LR 0,A
	LI 14
	LR 1,A
	PI TBFLD
	DCI VTMP
	LM
	LR 0,A
	LI CVAL3
	LR 3,A
	DCI VLINE
	XDC
	PI UIROW

	LI TBPLYRS
	LR 0,A
	LIS 6
	LR 1,A
	PI TBFLD
	DCI VTMP
	LM
	LR 0,A
	LI CVAL1
	LR 3,A
	DCI VLINE
	XDC
	PI UIROWR
	LEAVE

; TBFLD -- build field r0 of record VTMP2 into VLINE, folded, r1 chars max.
;   Both arguments move into r9 and r10 first: everything after this owns DC0,
;   and reading them back would need a DCI that wipes it.
;   Reached by PI; calls two levels down.
TBFLD:	ENTER
	LR A,0
	LR 9,A
	LR A,1
	LR 10,A
	DCI VTMP2
	LM
	LR 0,A
	PI TBREC
	LR A,9
	ADC
	XDC
	DCI VLINE
	XDC
	LR A,10
	LR 0,A
	PI GUPCPY
	LEAVE

; ---------------------------------------------------------------------------
; TBJOIN -- take the seat under the cursor. Everything the last table left
;   behind is cleared here rather than on the way out, because a net error can
;   leave by a path that never runs.
; ---------------------------------------------------------------------------
TBJOIN:	DCI VTTOP
	LM
	LR 0,A
	DCI VCUR
	LM
	AS 0
	LR 0,A
	PI TBREC
	XDC
	DCI VTABLE
	XDC
	LI NAMEMAX+1
	LR 4,A
TBJ1:	LM			; source first
	XDC
	ST
	XDC
	OI 0
	BZ TBJ2
	DS 4
	BNZ TBJ1
TBJ2:	DCI VTABLE+NAMEMAX
	CLR
	ST

	DCI VPRVPC		; the previous-poll shadows, the ready flag, the
	CLR			;   seeded fleet and the reticle all belong to the
	ST			;   table we just left. VPRVPC through VCUEQ are
	ST			;   consecutive, so clearing them is one DCI.
	ST
	ST
	ST
	DCI VREADY
	CLR
	ST
	DCI VSEEDED
	CLR
	ST
	DCI VCURX
	CLR
	ST
	ST
	LI ST_GAME
	LR 0,A
	JMP GOTO
