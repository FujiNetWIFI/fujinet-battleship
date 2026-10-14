; strings.inc -- every literal the screens print, parked in BLOCK 2.
;
; They live here purely for the ROM budget: block 1 is the tight one and
; these are inert bytes. It is legal because DPRINT/DPRTN reach a string
; through DSTRP with an INDIRECT load, and indirect operands carry the
; full 15 bits -- so a page-0 caller can name a page-1 string.
;
; The two literals that CANNOT move are DEFNAM and WHEEL: both are read
; with direct indexed loads (LODA,R0 WHEEL,R3), which never cross a page.

SNOFN:  DB      "NO FUJINET",0
TTITLE: DB      "BATTLESHIP",0
TNAME:  DB      "YOUR NAME",0
TEDIT:  DB      "DISC SPINS 4 6",0
TACCPT: DB      "ENTER ACCEPTS",0
TCHOOS: DB      "CHOOSE A TABLE",0
TJOINK: DB      "ENTER JOINS",0
TBACKK: DB      "CLEAR RENAMES",0
TENET:  DB      "NET ERR - RETRY",0
TPLACE: DB      "PLACE YOUR SHIPS",0
TPLKEY: DB      "5 TURNS  FIRE OK",0
TWAITP: DB      "WAITING ON SHIPS",0
TENET2: DB      "NET ERROR",0
TLOBBY: DB      "LOBBY",0
TREADY: DB      "FIRE = READY",0
TYTURN: DB      "YOUR TURN",0
TRMISS: DB      "MISS",0
TRHIT:  DB      "HIT",0
TRSUNK: DB      "SUNK",0
TBLANK: DB      0
