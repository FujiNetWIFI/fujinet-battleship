; fujinet.inc -- N: device commands and the Battleship binary wire format.
;
; EQUs only; no code. The wire format is the v2 binary one (?bin=1&v=2),
; shared with the astrocade and Intellivision ports -- see
; fujinet-battleship/astrocade/fujinet.inc and src/misc.h.

; ---- N: device ---------------------------------------------------------
NETDEV  EQU     $71             ; N1:
NCOPEN  EQU     $4F             ; 'O'
NCCLOSE EQU     $43             ; 'C'
NCREAD  EQU     $52             ; 'R'
NCSTAT  EQU     $53             ; 'S'
NMHTTPG EQU     $0C             ; open mode: HTTP GET
NTRNONE EQU     0               ; translation: none
FCACK   EQU     $06
FCNAK   EQU     $15

; NET_STATUS reply: 4 bytes in slice 0
NSAVLO  EQU     0               ; bytes waiting, low
NSAVHI  EQU     1               ; bytes waiting, high
NSCONN  EQU     2               ; connected flag
NSDEVST EQU     3               ; nDevStatus_t; 1 = SUCCESS

; ---- request ids (V_URL) ----------------------------------------------
RQTABLE EQU     0
RQSTATE EQU     1
RQREADY EQU     2
RQPLACE EQU     3
RQATTCK EQU     4
RQLEAVE EQU     5

; ---- header, 38 bytes, always present ----------------------------------
GOPLCNT EQU     0               ; playerCount, max 4
GOPRMPT EQU     1               ; prompt[33]
GOSTAT  EQU     34              ; STATUS_*
GOPLST  EQU     35              ; YOUR PLAYER_STATUS_*
GOACTPL EQU     36              ; int8; $FF = nobody, 0 = your turn.
                                ;   At gameover (v=2) the WINNER's index.
GOMVTIM EQU     37              ; seconds left, 0-250
GHDRLEN EQU     38

; ---- lobby body (status == STATUS_LOBBY) -------------------------------
GLSRVNM EQU     38              ; serverName[21]
GLPLYRS EQU     59              ; playerCount x LBSTRID
LBSTRID EQU     10
LBNAME  EQU     0               ; name[9]
LBSTAT  EQU     9               ; 3 = ready

; ---- in-play body (status != STATUS_LOBBY) -----------------------------
GILASTP EQU     38              ; lastAttackPos, 0-99
GIMYSHP EQU     39              ; myShips[10]: [0-4] yours,
                                ;   [5-9] the winner's (gameover, v=2)
GIPLYRS EQU     49              ; playerCount x PLSTRID -- ONLY once the
                                ;   gamefields exist (status >= GAMESTART)
PLSTRID EQU     115
PLNAME  EQU     0               ; name[9]
PLSTAT  EQU     9               ; playerStatus
PLFIELD EQU     10              ; gamefield[100]: y*10+x
PLSHIPS EQU     110             ; shipsLeft[5]: 1 afloat, 0 sunk

; During STATUS_PLACE_SHIPS the player records are name+status ONLY --
; 10 bytes, not 115. Never index a gamefield in the place phase.
PPSTRID EQU     10

; ---- /tables ------------------------------------------------------------
TBSTRID EQU     36
TBID    EQU     0               ; table id[9]
TBNAME  EQU     9               ; name[21]
TBPLYRS EQU     30              ; players[6], literal "cur / max"

; ---- lengths ------------------------------------------------------------
GMAXLEN EQU     509             ; GIPLYRS + 4*PLSTRID
GMINLEN EQU     GHDRLEN
TMAXLEN EQU     361

; ---- status -------------------------------------------------------------
STLOBBY EQU     0
STPLACE EQU     1
STSTART EQU     10
STMISS  EQU     11
STHIT   EQU     12
STSUNK  EQU     13
STOVER  EQU     99

; ---- player status ------------------------------------------------------
PSPLAY  EQU     0
PSDEFT  EQU     1               ; defeated
PSVIEW  EQU     2               ; spectator
PSREADY EQU     3
PSPLACE EQU     10

; ---- board / ships ------------------------------------------------------
BRDDIM  EQU     10              ; 10 x 10
BRDCELL EQU     100
NSHIPS  EQU     5
CELLUNK EQU     0
CELLHIT EQU     1
CELLMIS EQU     2
