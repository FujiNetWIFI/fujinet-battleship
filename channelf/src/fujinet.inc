; ---------------------------------------------------------------------------
; fujinet.inc -- N: device commands and the Battleship binary wire format.
;
; EQUs only; no code. The format is the v2 binary one (?bin=1&v=2), shared with
; the Arcadia, Astrocade and Intellivision clients -- see
; fujinet-battleship/arcadia/fujinet.inc and the server's util.go, which is
; where every offset below was checked.
;
; Unlike every sibling console, the whole reply is FLAT: GMAXLEN is 509 and the
; cart's reply window at $F800 is 1K, so a player record is one DCI and the
; gamefield is a straight walk. No slices, no cursor, no bounce buffer.
; ---------------------------------------------------------------------------

; --- N: device ------------------------------------------------------------
NDEV	EQU 071H		; N1:
NCOPEN	EQU 04FH		; 'O'
NCCLOSE	EQU 043H		; 'C'
NCREAD	EQU 052H		; 'R'
NCSTAT	EQU 053H		; 'S'
NMHTTPG	EQU 00CH		; open mode: HTTP GET
NTRNONE	EQU 000H		; translation: none

; NET_STATUS reply, 4 bytes at the head of the reply window
NSAVLO	EQU 0			; bytes waiting, low
NSAVHI	EQU 1			; bytes waiting, high
NSCONN	EQU 2			; connected flag
NSDEVST	EQU 3			; nDevStatus_t; 1 = SUCCESS

; --- Fuji device, for the shared username appkey --------------------------
FDEV	EQU 070H
FC_AKOPN EQU 0DCH		; OPEN_APPKEY
FC_AKRD	EQU 0DDH		; READ_APPKEY
FC_AKWR	EQU 0DEH		; WRITE_APPKEY
AKCREAT	EQU 1			; the shared username every client reads
AKAPP	EQU 1
AKKEY	EQU 0
AKRDMOD	EQU 0
AKWRMOD	EQU 1

; --- request ids (VREQ) ---------------------------------------------------
RQTABLE	EQU 0
RQSTATE	EQU 1
RQREADY	EQU 2
RQPLACE	EQU 3
RQATTCK	EQU 4
RQLEAVE	EQU 5

; --- header, 38 bytes, always present -------------------------------------
GOPLCNT	EQU 0			; playerCount, max 4
GOPRMPT	EQU 1			; prompt[33]
GOSTAT	EQU 34			; STATUS_*
GOPLST	EQU 35			; YOUR PLAYER_STATUS_*
GOACTPL	EQU 36			; int8; $FF = nobody, 0 = your turn.
				;   At gameover (v=2) the WINNER's index.
GOMVTIM	EQU 37			; seconds left, 0-250
GHDRLEN	EQU 38

; --- lobby body (status == STLOBBY) ---------------------------------------
GLSRVNM	EQU 38			; serverName[21]
GLPLYRS	EQU 59			; playerCount x LBSTRID
LBSTRID	EQU 10
LBNAME	EQU 0			; name[9]
LBSTAT	EQU 9			; 3 = ready

; --- in-play body (status != STLOBBY) -------------------------------------
GILASTP	EQU 38			; lastAttackPos, 0-99
GIMYSHP	EQU 39			; myShips[10]: [0-4] yours,
				;   [5-9] the winner's (gameover, v=2)
GIPLYRS	EQU 49			; playerCount x PLSTRID -- ONLY once the
				;   gamefields exist (status >= STSTART)
PLSTRID	EQU 115
PLNAME	EQU 0			; name[9]
PLSTAT	EQU 9			; playerStatus
PLFIELD	EQU 10			; gamefield[100]: y*10+x
PLSHIPS	EQU 110			; shipsLeft[5]: 1 afloat, 0 sunk

; During STPLACE the player records are name+status ONLY -- 10 bytes, not 115.
; NEVER index a gamefield in the place phase: the serializer appends the
; gamefield only once it exists.
PPSTRID	EQU 10

; --- /tables --------------------------------------------------------------
TBSTRID	EQU 36
TBID	EQU 0			; table id[9]
TBNAME	EQU 9			; name[21]
TBPLYRS	EQU 30			; players[6], literal "cur / max"

; --- lengths --------------------------------------------------------------
GMAXLEN	EQU 509			; GIPLYRS + 4*PLSTRID
GMINLEN	EQU GHDRLEN
TMAXLEN	EQU 361

; --- status ---------------------------------------------------------------
STLOBBY	EQU 0
STPLACE	EQU 1
STSTART	EQU 10
STMISS	EQU 11
STHIT	EQU 12
STSUNK	EQU 13
STOVER	EQU 99

; --- screen classes -- what the client actually dispatches on -------------
CLLOBBY	EQU 0
CLPLACE	EQU 1
CLPLAY	EQU 2
CLOVER	EQU 3

; --- player status --------------------------------------------------------
PSPLAY	EQU 0
PSDEFT	EQU 1			; defeated
PSVIEW	EQU 2			; spectator
PSREADY	EQU 3
PSPLACE	EQU 10

; --- board / ships --------------------------------------------------------
BRDDIM	EQU 10			; 10 x 10
BRDCELL	EQU 100
NSHIPS	EQU 5
MAXPLR	EQU 4
CELLUNK	EQU 0			; the wire only ever carries these three:
CELLHIT	EQU 1			;   FIELD_SUNK is internal to the server's AI
CELLMIS	EQU 2			;   and is never serialised
NOPLC	EQU 0FFH		; a VSHIPS slot with nothing committed yet
