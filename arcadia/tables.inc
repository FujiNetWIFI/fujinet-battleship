; tables.inc -- lookup tables parked in BLOCK 2 for the ROM budget.
;
; Both are reached from block-1 code through page-0 DWBE pointer words
; (PXLAT in disp.inc, PKT0/1/2 in input.inc): an indirect operand
; carries the full 15 bits, while the direct indexed form these started
; as cannot leave its own 8K page.

; ---- ASCII $20-$7F -> glyph, 96 entries; no UDC codes ------------------
XLAT:   DB      $00,$34,$34,$34,$37,$34,$34,$34 ; spc ! " # $ % & '
        DB      $34,$34,$34,$36,$35,$34,$34,GDIAG ; ( ) * + , - . /
        DB      $10,$11,$12,$13,$14,$15,$16,$17 ; 0-7
        DB      $18,$19,$34,$34,$34,$34,$34,$34 ; 8 9 : ; < = > ?
        DB      $34,$1A,$1B,$1C,$1D,$1E,$1F,$20 ; @ A-G
        DB      $21,$22,$23,$24,$25,$26,$27,$28 ; H-O
        DB      $29,$2A,$2B,$2C,$2D,$2E,$2F,$30 ; P-W
        DB      $31,$32,$33,$34,$34,$34,$34,$34 ; X Y Z [ \ ] ^ _
        DB      $34,$1A,$1B,$1C,$1D,$1E,$1F,$20 ; ` a-g
        DB      $21,$22,$23,$24,$25,$26,$27,$28 ; h-o
        DB      $29,$2A,$2B,$2C,$2D,$2E,$2F,$30 ; p-w
        DB      $31,$32,$33,$34,$34,$34,$34,$34 ; x y z { | } ~ del

; nibble -> key code, highest bit wins (top-row keys take precedence) so
; a mashed pair cannot alias to a third key.
KT0:    DB      KNONE,KCLR,7,7,4,4,4,4,1,1,1,1,1,1,1,1
KT1:    DB      KNONE,0,8,8,5,5,5,5,2,2,2,2,2,2,2,2
KT2:    DB      KNONE,KENT,9,9,6,6,6,6,3,3,3,3,3,3,3,3
