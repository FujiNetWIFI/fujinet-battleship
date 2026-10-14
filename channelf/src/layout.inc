; ---------------------------------------------------------------------------
; layout.inc -- where the boards go, and how big a cell is.
;
; The play screen scales with player count. A 10x10 board of 4x4 cells is
; 40x40, and two of those sit side by side inside the 95x58 safe area with
; room left over -- but four of them do not, because 58 rows will not take two
; 40-row boards. So a crowded table drops to a 3x2 cell and shows all four
; quadrants, and a heads-up table gets the big cells.
;
; A 3x2 cell is very nearly SQUARE on a 4:3 set: the Channel F puts 128x64
; into the raster, so a pixel is about 1.3x taller than it is wide. 2x2 would
; look tall and waste the horizontal room this machine actually has.
;
; Every layout is a record, copied into RAM at VLAY when the screen class or
; the seat count changes, so board.inc has exactly one code path. The record
; also carries the palette band, because the bands move with the boards.
;
; Quadrant map is the Astrocade port's, and the server rotates its player list
; so this client is always index 0:
;
;     Q1 (top-left)  | Q2 (top-right)
;    -------- divider cross --------
;     Q0 = YOU (b-l) | Q3 (bottom-right)
; ---------------------------------------------------------------------------

; --- record layout --------------------------------------------------------
LRCELW	EQU 0			; cell width in pixels
LRCELH	EQU 1			; cell height
LRNBRD	EQU 2			; boards on screen
LRBND0	EQU 3			; first row of the palette-1 band
LRBND1	EQU 4			; last row of it
LRFOOT	EQU 5			; footer glyph row
LRVX	EQU 6			; vertical divider: x, y, w, h (w = 0: none)
LRVY	EQU 7
LRVW	EQU 8
LRVH	EQU 9
LRHX	EQU 10			; horizontal divider
LRHY	EQU 11
LRHW	EQU 12
LRHH	EQU 13
LROX	EQU 14			; board origin x[4]
LROY	EQU 18			; board origin y[4]
LRPPX	EQU 22			; ship-pip column x[4]
LRPPY	EQU 26			; ship-pip top y[4]
LRPPDX	EQU 30			; pip-to-pip step, so one routine draws both a
LRPPDY	EQU 31			;   horizontal strip and a vertical column
LRFX	EQU 32			; the frame around the whole board group:
LRFY	EQU 33			;   x, y, width, height (width 0: no frame)
LRFW	EQU 34
LRFH	EQU 35
LRECSZ	EQU 36

; --- shared chrome --------------------------------------------------------
STATY	EQU DORGY		; status row glyphs, y = 4
PANX	EQU 70			; roster panel, 7 cells of 4px: x = 70..97
PANW	EQU 7
PANY	EQU 11			; its seven rows are PANY + 6*n
PIPX0	EQU 72			; five 3x3 pips on a 5px pitch: 72..94
PIPPIT	EQU 5
PLCPANX	EQU 50			; placement screen: the ship list column
FPIPX	EQU 38			; your own five pips, on the footer row:
				;   x 38..60, between the two text fields
FPIPY	EQU 56

; ---------------------------------------------------------------------------
; LAYQD -- 3 or 4 players. Four 30x20 boards around a divider cross, roster
; panel on the right.
;   x:  4..33 col L | 34..35 div | 36..65 col R | 68..95 panel
;   y:  4.. 9 status | 11..30 row T | 31..32 div | 33..52 row B | 55..59 footer
; ---------------------------------------------------------------------------
LAYQD:	DB 3,2,4
	DB 10,53,55		; the band covers the FRAME rows too, or its top
				;   and bottom edges land on palette 0 and come
				;   out white instead of blue
	DB 35,11,2,42		; vertical divider
	DB 5,31,62,2		; horizontal divider
	DB 5,5,37,37		; origin x: Q0 Q1 Q2 Q3
	DB 33,11,11,33		; origin y
	DB FPIPX,PIPX0,PIPX0,PIPX0
	DB FPIPY,PANY+2*6,PANY+4*6,PANY+6*6
	DB PIPPIT,0		; a horizontal strip of five
	DB 4,10,64,44		; frame: x 4..67, y 10..53. x=4 is the first
				;   VISIBLE column -- a frame at 3 is off the set

; ---------------------------------------------------------------------------
; LAYHU -- heads up (1 or 2 seats). Two 40x40 boards of 4x4 cells; yours on
; the right, the enemy on the left where the reticle lives.
;   x:  4..43 enemy | 45..46 div | 48..87 yours | 89..95 two pip columns
;   y:  4.. 9 status | 11..50 boards | 52..56 footer
; ---------------------------------------------------------------------------
LAYHU:	DB 4,4,2
	DB 10,51,53
	DB 45,11,2,40		; vertical divider
	DB 0,0,0,0		; no horizontal divider
	DB 47,5,0,0		; origin x: yours right, enemy left
	DB 11,11,0,0
	DB 93,89,0,0		; a pip column beside each board
	DB 11,11,0,0
	DB 0,8			; a vertical column of five
	DB 4,10,84,42		; frame: x 4..87, y 10..51

; ---------------------------------------------------------------------------
; LAYTXT -- the screens with no boards on them: the lobby roster, the table
; list, the name entry, help. The palette-1 band is parked on rows 1-3, which
; are above the visible area, so the whole screen reads as palette 0 and text
; gets the only white this machine has.
; ---------------------------------------------------------------------------
LAYTXT:	DB 4,6,0
	DB 1,3,DORGY+8*DCELLH
	DB 0,0,0,0
	DB 0,0,0,0
	DB 0,0,0,0
	DB 0,0,0,0
	DB 0,0,0,0
	DB 0,0,0,0
	DB 0,0
	DB 0,0,0,0		; no frame

; ---------------------------------------------------------------------------
; LAYPLC -- ship placement. One 40x40 board of your own, with the ship list
; down the right-hand side.
; ---------------------------------------------------------------------------
LAYPLC:	DB 4,4,1
	DB 10,51,53
	DB 0,0,0,0
	DB 0,0,0,0
	DB 5,0,0,0
	DB 11,0,0,0
	DB 0,0,0,0
	DB 0,0,0,0
	DB 0,0
	DB 4,10,42,42		; frame: x 4..45, y 10..51
