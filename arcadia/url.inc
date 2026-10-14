; url.inc -- building the devicespec, streamed straight into the TX byte
; stream a character at a time. Nothing is assembled in RAM: the cart
; counts the payload itself, so nothing here tracks a length. The worst
; case (/place with five 3-digit positions) is ~100 bytes against the
; 320-byte TX ceiling.
;
; The staged move is raw bytes, not text -- SHIPS[5] and ATKPOS -- and
; BYTDEC turns each into decimal digits on the way out, so /place needs
; no 20-character move buffer.
;
; TBLBUF and PLNBUF are NUL-terminated copies of fixed-width wire fields
; -- pushed up to the NUL, never through it (padding in the middle of a
; URL corrupts the request on the wire).

TXB     MACRO   ch              ; append one literal byte (clobbers R0,R1)
        LODI,R1 ch
        LODA,R0 FNDATA,R1
        ENDM

; BLDURL: stream the devicespec for request (V_URL).
; Calls one down (FNTXSTR). Clobbers R0-R3, FSTRP.
BLDURL: LODA,R0 UENDPT          ; "N:" unless the endpoint carries its own
        COMI,R0 'N'
        BCTR,EQ BURL1
        TXB     'N'
        TXB     ':'
BURL1:  SETFS1  UENDPT
        BSTA,UN FNTXSTR
        LODA,R0 *PVURL
        BCFR,EQ BURL2
        SETFS1  UTABLS          ; tables: no table, no player
        BCTA,UN FNTXSTR         ; tail call
BURL2:  COMI,R0 RQSTATE
        BCFR,EQ BURL3
        SETFS1  USTATE
        BSTA,UN FNTXSTR
        BCTA,UN BURLQ
BURL3:  COMI,R0 RQREADY
        BCFR,EQ BURL4
        SETFS1  UREADY          ; bare /ready TOGGLES, matching the
        BSTA,UN FNTXSTR         ;   Intellivision and astrocade clients
        BCTA,UN BURLQ
BURL4:  COMI,R0 RQPLACE
        BCFA,EQ BURL5
        SETFS1  UPLACE
        BSTA,UN FNTXSTR
        LODI,R0 0
        STRA,R0 *PSHIX
BURLP:  LODA,R0 *PSHIX          ; indexing needs R0 as the data register,
        STRZ    R2              ;   so the index goes through R0 first
        LODA,R0 *PSHIPS,R2      ; indexed indirect: one pointer, five bytes
        STRZ    R1
        BSTA,UN BYTDEC
        LODA,R0 *PSHIX
        ADDI,R0 1
        STRA,R0 *PSHIX
        COMI,R0 NSHIPS
        BCTR,EQ BURLQ           ; no comma after the last
        TXB     ','
        BCTR,UN BURLP
BURL5:  COMI,R0 RQATTCK
        BCFR,EQ BURL6
        SETFS1  UATTCK
        BSTA,UN FNTXSTR
        LODA,R0 *PATKP
        STRZ    R1
        BSTA,UN BYTDEC
        BCTR,UN BURLQ
BURL6:  SETFS1  ULEAVE
        BSTA,UN FNTXSTR
BURLQ:  SETFS1  UQTAB
        BSTA,UN FNTXSTR
        SETFS1  TBLBUF
        BSTA,UN FNTXSTR
        SETFS1  UQPLR
        BSTA,UN FNTXSTR
        SETFS1  PLNBUF
        BSTA,UN FNTXSTR
        SETFS1  UQBIN2
        BCTA,UN FNTXSTR         ; tail call

; BYTDEC: append R1 (0-255) to the TX stream as 1-3 decimal digits, no
; leading zeros. Leaf. Clobbers R0-R3.
BYTDEC: LODZ    R1
        STRZ    R3              ; R3 = the running remainder
        LODI,R2 0               ; R2 = "a digit has been emitted"
        LODI,R1 0
BD1:    LODZ    R3              ; hundreds
        COMI,R0 100
        BCTR,LT BD2
        SUBI,R0 100
        STRZ    R3
        LODZ    R1
        ADDI,R0 1
        STRZ    R1
        BCTR,UN BD1
BD2:    LODZ    R1
        BCTR,EQ BD3
        ADDI,R0 '0'
        STRZ    R1
        LODA,R0 FNDATA,R1
        LODI,R2 1
BD3:    LODI,R1 0
BD4:    LODZ    R3              ; tens
        COMI,R0 10
        BCTR,LT BD5
        SUBI,R0 10
        STRZ    R3
        LODZ    R1
        ADDI,R0 1
        STRZ    R1
        BCTR,UN BD4
BD5:    LODZ    R1
        BCFR,EQ BD6             ; tens nonzero: emit it
        LODZ    R2
        BCTR,EQ BD7             ; nothing emitted yet and tens 0: skip
BD6:    LODZ    R1
        ADDI,R0 '0'
        STRZ    R1
        LODA,R0 FNDATA,R1
BD7:    LODZ    R3              ; units, always
        ADDI,R0 '0'
        STRZ    R1
        LODA,R0 FNDATA,R1
        RETC,UN

        INCLUDE "build/endpoint.inc"
UTABLS: DB      "tables?bin=1",0
USTATE: DB      "state",0
UREADY: DB      "ready",0
UPLACE: DB      "place/",0
UATTCK: DB      "attack/",0
ULEAVE: DB      "leave",0
UQTAB:  DB      "?table=",0
UQPLR:  DB      "&player=",0
UQBIN2: DB      "&bin=1&v=2",0  ; v=2: winner + ship reveal at gameover
