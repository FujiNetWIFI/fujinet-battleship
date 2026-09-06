; fujilib.inc -- what remains of the bring-up library in page 0.
;
; This client moved the whole mailbox conversation into block 2 (see
; mailbox.inc), where the hotspots are directly addressable. The one
; helper that must stay in page 0 is the string streamer: its pointer
; word FSTRP is page-0 RAM, and an indirect operand can only be NAMED
; from the page it lives in -- page-1 code sets it through the PFSTRH/
; PFSTRL pointers and calls down here.

; FNTXSTR: append the 0-terminated string whose console address is in
; FSTRP (big-endian) to the TX stream -- stopping at the NUL, never
; through it. Works for strings in ROM (either page), RAM, or the reply
; window itself (the reply-stability invariant makes the last one
; legal). Leaf. Clobbers R0,R1,R3.
FNTXSTR: LODI,R3 $FF
FNTXS1: LODA,R0 *FSTRP,R3,+    ; str[++R3]; a load sets CC
        BCTR,EQ FNTXS2
        STRZ    R1
        LODA,R0 *FTXP,R1        ; append
        BCTR,UN FNTXS1
FNTXS2: RETC,UN

FTXP:   DWBE    FNDATA
