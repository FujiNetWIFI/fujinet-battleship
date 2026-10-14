; ---------------------------------------------------------------------------
; net.inc -- the N: device round trip.
;
; An F8 transcription of the Arcadia port's sequence, which is the Astrocade's,
; which is the Odyssey 2's. The five rules it carries forward, every one of
; them learned the hard way on an earlier console:
;
;   * The CLOSE belongs at the START of the next request, never after the READ.
;     Every transaction repaints the whole reply window, a CLOSE reply is
;     empty, and every screen renders straight out of that window between
;     polls -- so closing here would wipe the data the READ just delivered.
;   * The reply length belongs to the most recent transaction, so it is
;     captured immediately after the READ and before anything else runs.
;   * The ESP32 reports STATUS as soon as SOME of the response has arrived.
;     Poll until two consecutive readings agree, bounded, rather than guessing
;     a delay.
;   * The 4th STATUS byte is nDevStatus_t and checking it is NOT optional: the
;     HTTP GET is deferred until the first STATUS, and an HTTP error page has a
;     perfectly readable body.
;   * Validate the reply length against what playerCount implies before
;     believing a byte of it.
;
; Every routine returns A = 0 on success, nonzero on failure.
; ---------------------------------------------------------------------------

; NCLOSE -- close the previous request. The first call has nothing to close and
;   earns a NAK; both outcomes are ignored on purpose.
NCLOSE:	ENTER
	LI NDEV
	LR 0,A
	LI NCCLOSE
	LR 1,A
	CLR
	LR 2,A
	PI FNBEG
	PI FNGO
	CLR
	LEAVE

; NOPEN -- OPEN the devicespec as an HTTP GET with no translation.
NOPEN:	ENTER
	LI NDEV
	LR 0,A
	LI NCOPEN
	LR 1,A
	LIS 2
	LR 2,A
	PI FNBEG
	LI NMHTTPG
	LR 0,A
	PI FNPB
	LI NTRNONE
	LR 0,A
	PI FNPB
	PI BLDURL
	PI FNGO
	PI FNACK
	LEAVE

; NSTATUS -- one STATUS poll. On success VAVLO/VAVHI hold the bytes waiting.
NSTATUS: ENTER
	LI NDEV
	LR 0,A
	LI NCSTAT
	LR 1,A
	LIS 2
	LR 2,A
	PI FNBEG
	CLR
	LR 0,A
	PI FNPB
	CLR
	LR 0,A
	PI FNPB
	PI FNGO
	PI FNACK
	CI 0
	BNZ NSBAD
	DCI FNREPLY+NSDEVST
	LM
	CI 1			; SUCCESS; anything else is an HTTP error page
	BNZ NSBAD
	DCI FNREPLY+NSAVLO
	LM
	DCI VAVLO
	ST
	DCI FNREPLY+NSAVHI
	LM
	DCI VAVHI
	ST
	CLR
	LEAVE
NSBAD:	LI 0FFH
	LEAVE

; NSETTLE -- poll STATUS until at least VMIN bytes are waiting AND two
;   consecutive readings agree, bounded.
;
;   The minimum is what makes this reliable, and it is not optional. The
;   adapter reports whatever has landed so far, so two readings taken a moment
;   apart agree while the body is still streaming: a /state fetch settled at
;   129 bytes of a 385-byte response and handed back a plausible header
;   followed by 40 bytes of nothing. Waiting for a length the caller knows the
;   reply cannot be shorter than removes the guess.
NSETTLE: ENTER
	LI 60
	DCI VNTRY
	ST
	CLR
	DCI VPRVLO
	ST
	DCI VPRVHI
	ST
NSET1:	PI NSTATUS
	CI 0
	BNZ NSETB

	PI NCMPMIN		; avail >= min?
	CI 0
	BNZ NSETN

NSETC:	; enough has arrived; now require it to stop growing
	DCI VAVLO
	LM
	LR 4,A
	DCI VPRVLO
	LM
	XS 4
	BNZ NSETN
	DCI VAVHI
	LM
	LR 4,A
	DCI VPRVHI
	LM
	XS 4
	BNZ NSETN
	CLR
	LEAVE

NSETN:	DCI VAVLO
	LM
	DCI VPRVLO
	ST
	DCI VAVHI
	LM
	DCI VPRVHI
	ST
	; About 100 ms between readings, not a handful: the adapter answers as
	; soon as SOME of the body has landed.
	;
	; DELAYMS UNITS ARE NOT MILLISECONDS. One unit is about 9 ms -- the name
	; is aspirational and the routine says on the tin that it is approximate.
	; Passing 100 here asks for the best part of a second per reading and 54
	; seconds of worst case, which is what a poll loop that managed three
	; requests in ninety seconds was actually waiting on.
	LI 11
	LR 3,A
	PI DELAYMS
	DCI VNTRY
	LM
	AI 0FFH
	DCI VNTRY
	ST
	CI 0
	BNZ NSET1
NSETB:	LI 0FFH
	LEAVE

; NCMPMIN -- A = 0 if VAV >= VMIN, else 1.
;
;   Unsigned, and it has to be. The obvious version subtracts and tests the
;   SIGN, which is a signed comparison: 181 - 1 is 180, bit 7 set, and reads as
;   "negative" -- so a settled 181-byte reply looked short of a 1-byte minimum
;   and the poll never terminated. The carry out of the two's-complement add is
;   the unsigned answer: set means no borrow.
;   Leaf. Clobbers A, r4, r5, DC0.
NCMPMIN: DCI VMIN0
	LM
	COM
	INC			; -min, low; carry set if min low was 0
	LR 4,A
	DCI VMIN1
	LM
	COM
	LNK			; -min, high, with that carry
	LR 5,A
	DCI VAVLO
	LM
	AS 4
	DCI VAVHI
	LM
	LNK			; carry from the low half; DCI and LM leave flags alone
	AS 5
	BC NCMPOK
	LI 1
	POP
NCMPOK:	CLR
	POP

; NREAD -- READ min(VAV, r0:r1 cap) bytes into the reply window, then capture
;   the reply length before anything else can repaint it.
;   r0 = cap low, r1 = cap high.
NREAD:	ENTER
	LR A,0
	LR 4,A
	LR A,1
	LR 5,A
	; want = min(avail, cap), unsigned -- same trap as NCMPMIN. Park the cap
	; in VMIN and reuse that comparison rather than writing a second one.
	DCI VMIN0
	LR A,4
	ST
	LR A,5
	ST
	PI NCMPMIN		; avail >= cap?
	CI 0
	BNZ NRDAV		; no: avail is the smaller
	LR A,4
	LR 6,A
	LR A,5
	LR 7,A
	BR NRDGO
NRDAV:	DCI VAVLO
	LM
	LR 6,A
	DCI VAVHI
	LM
	LR 7,A

NRDGO:	LI NDEV
	LR 0,A
	LI NCREAD
	LR 1,A
	LIS 2
	LR 2,A
	PI FNBEG
	LR A,6
	LR 0,A
	LR A,7
	LR 1,A
	PI FNPW
	CLR
	LR 0,A
	PI FNPB
	PI FNGO
	PI FNACK
	CI 0
	BNZ NRDBAD
	; FNRXLO/HI belong to the transaction that just finished; take them now
	PI FNRXL
	DCI VRXLO
	LR A,0
	ST
	DCI VRXHI
	LR A,1
	ST
	CLR
	LEAVE
NRDBAD:	LI 0FFH
	LEAVE

; NFETCH -- the whole cycle for request VREQ: close the last one, open this
;   one, let STATUS settle, read up to r0:r1 bytes.
NFETCH:	ENTER
	LR A,0
	LR 4,A
	LR A,1
	LR 5,A
	PI NCLOSE
	PI NOPEN
	CI 0
	BNZ NFBAD
	PI NSETTLE
	CI 0
	BNZ NFBAD
	LR A,4
	LR 0,A
	LR A,5
	LR 1,A
	PI NREAD
	LEAVE
NFBAD:	LI 0FFH
	LEAVE
