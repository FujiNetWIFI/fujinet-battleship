# Battleship for the Emerson Arcadia 2001

A 2650 assembly client for the [Battleship server](https://battleship.carr-designs.com/),
running over the FujiNet cartridge mailbox from the Arcadia bring-up at
`fujinet-firmware/pico/arcadia` (protocol v1: everything rides reads, no
banking). The game logic is the astrocade port's, re-cut for this
hardware: binary `?bin=1&v=2` wire format, zero-copy rendering straight
out of the reply window, opaque overdraw per poll.

## The screen

**26-line mode** (16x26 of 8x8 cells, the lower 13 rows fed from `$1A00`)
plus **multicolor mode** (`$18FD` b7). A framed 10x10 board is exactly 12
rows, so two of them plus a status row and a footer fill the screen with
nothing left over:

```
row  0     status / server prompt + move clock
row  1     enemy frame top
rows 2-11  enemy board                cols 13-15: ship pips
row 12     enemy frame bottom
------------- screen-half boundary ($18CF / $1A00) -------------
row 13     your frame top
rows 14-23 your board                 cols 13-15: ship pips
row 24     your frame bottom
row 25     viewed opponent / cursor coordinate
```

The split falls exactly between the two boards, so **each board lives
wholly inside one screen half** and no board render ever has to move the
`SCRP` pointer mid-board.

In multicolor, cell bit 6 picks the ink and bit 7 the background from two
palette registers -- `$19F8 = $46` (26-line, white on blue) and
`$19F9 = $AF` (8x8 cells, red on black):

| attr | bits | looks like | used for |
|------|------|-----------|----------|
| `$00` | 00 | white on blue | the sea, your hulls, misses, board frames |
| `$40` | 01 | red on blue | hits, sunk markers |
| `$80` | 10 | white on black | status and footer text |
| `$C0` | 11 | red on black | alerts |

The one hazard: a cell of exactly `$C0` -- a *space* with attrs 11 --
flips the rest of the row into block graphics. The single glyph writer
(`DPUTC` in disp.inc) rewrites that byte to `$80` (same look, no trap);
nothing else stores attributed cells, and `DEMO=1` paints a row of
spaces at `$C0` on purpose so a regression shows up in the screenshot.

`$19F9` bit 6 is **also the pot mux**, so input.inc drives that register
between `$AF` and `$EF`; `BGCBAS` holds the current mode's value.

## Board art

The **frames come from the 2637's built-in character ROM**: `$04`-`$0B`
happen to be a complete box-drawing kit (edges and all four corners),
which costs no UDCs and is why all eight are free for the ships.

The eight UDCs at `$1980` are one bank, loaded once at boot. The first
four **are** the sprite images, so slot `$38` has to hold the reticle:

* `$38` targeting reticle (**= sprite 0's image**), corner brackets only
  so the cell underneath still reads
* `$39` / `$3A` hull midsection, horizontal / vertical (edge-to-edge, so
  segments tile into a continuous ship)
* `$3B` bow/end cap · `$3C` hit burst · `$3D` miss splash ·
  `$3E` wreck · `$3F` ship pip

`assets/udc.inc` is generated from ASCII art by `tools/mkudc.py`
(`make assets`). `DXLAT` never emits a UDC code, so a stray `-` or `:` in
a server string cannot render as a hull.

## The sprite

One sprite: the targeting reticle, positioned by `SPRCELL` (cell row/col
-> raw). Cells are 8x8 in 26-line mode so the mapping is exact --
`SPYOFF 0` / `SPXOFF 43` landed the reticle dead on an 8x8 cell first
try, verified by parking it over a solid block. It is parked offscreen
whenever it is not your turn.

## Controls

* **disc** -- move the reticle (both axes; see below), move a ship during
  placement, spin the character wheel on the name screen
* **FIRE** or **Enter** -- fire / select / accept / ready up in the lobby
* **keypad 5** -- rotate the ship being placed
* **keypad 4 / 6** -- page the enemy board through the opponents (a view
  choice only: a shot lands on every opponent at once), cursor left/right
  on the name screen
* **keypad 0** -- poll now (lobby: refetch) · **Clear** -- leave / back

**Both disc axes arrive on `$19FF`**, selected by the pot mux: clear =
vertical, set = horizontal. input.inc flips the mux once per frame and
samples only the selected axis, but caches BOTH raw values and decodes
the direction from the cache -- a held direction then reads the same
every frame, which is what edge detection needs. Sampling after a full
frame with the mux settled also gives real 2637 silicon time to digitise,
which a flip-and-read-immediately scheme would not. A held direction
auto-repeats; nine presses to cross the board would be miserable.

## Sound

One tone voice, shared with the display registers (`$18FD` b7 is the
multicolor bit, `$18FE` b5-7 the horizontal shift -- both preserved).
Higher pitch value = lower tone. Cues: a short click as the reticle
steps, and miss / hit / sunk on the shot result, which fires on the
**status edge** so an opponent's shot is heard too. Plus select, error,
your-turn and game-over.

## Placement

A random legal layout is seeded and then adjusted -- the astrocade
port's model. **Overlap testing uses the screen as the occupancy map**: a
10x10 byte array is 100 bytes and this machine has 84 in total, so the
board is repainted carrying every ship except the one being moved, and a
candidate is legal when every cell it would cover reads as open sea.
Storing `NOPLC` into the moving ship's slot for the duration of the
repaint is what excludes it, so the whole trick costs nothing but the two
bytes that shelter the slot.

## Code layout

Block 1 (CPU `$0000-$0FFF`, page 0) holds everything that touches the
screen; block 2 (CPU `$2000-$2AFF`, page 1) shares its 2650 page with the
mailbox, so the whole network stack lives there and a register arm or TX
append is a direct indexed load. Page-1 code reaches the few page-0 cells
it needs through `DWBE` pointer words.

Block 1 is the tight one, so anything inert or screen-independent has
been pushed across: the string literals, the ASCII and keypad translate
tables, `sound.inc` and `target.inc`. That is legal because indirect
operands carry the full 15 bits while the direct indexed forms they
started as cannot leave their own 8K page. `DEFNAM` and `WHEEL` are the
two literals that must stay, both being read with direct indexed loads.

net.inc carries the family's five hard-won rules verbatim (CLOSE opens
the *next* request; capture RXLEN immediately after READ; STATUS settles
on two agreeing readings; STATUS byte 3 must be 1; validate the length
the status implies before believing a byte). `MCOMMIT` derives every
sequence number from the cart's persisted ACKSEQ, which `emu/resettest.lua`
proves across a console RESET.

RAM is the whole design constraint: 84 bytes in 26-line mode (zone A
`$18D0-$18EF`, the four bytes at `$18F8-$18FB`, zone B `$1AD0-$1AFF` --
the ledger is in battleship.asm, and zone B is spoken for to the last
byte). Nothing is buffered: strings render straight out of the reply
window, the 100-cell gamefield is a purely sequential `RXNEXT` walk, and
the 13-line screens keep their edit buffer in `$1A00` scratch.

Module budgets (`tools/checksize.py`, run every build): block 1 ~3.9K of
4096, block 2 ~1.9K of 2816.

## Build and run

```
./build.sh              # build/battleship.bin (8K, FUJI-claimed)
DEMO=1 ./build.sh       # static mock board, no network (art/palette work)
./run.sh                # MAME arcadia + fujinet cart, interactive
make smoke              # headless: join the AI room, place, play
make resettest          # console RESET mid-session; SEQ must continue
make probe              # headless play with a running state trace
make fullgame           # play a raster until the server says game over
make shot               # one screenshot of the DEMO board
```

`checkdepth.py` runs every build but reports the DEMO path, because it
parses every source regardless of the flag; the real worst chain is the
placement redraw, hand-audited at 6 of the 8 RAS entries:
`PLACSCR > PLMOVE > PLDRWO > YSHIPR > SHIPDR > PLCDEC`. Divide-by-ten is
inlined into PLCDEC for exactly that reason.

Environment: `ENDPOINT` (server), `FUJI_FIRMWARE` (tools come from
`pico/arcadia`), `MAME_DIR` (a tree with `emu/apply.sh` applied),
`FUJINET_TCP` (a live fujinet-pc BoIP listener, default
`127.0.0.1:9995`), `FUJINET_DEBUG=1` (log every mailbox transaction).

Against a local server:

```
cd <servers>/fujinet-game-system/battleship && go run .
ENDPOINT=http://127.0.0.1:8080/ ./build.sh && ./run.sh
```

Tables `ai1`..`ai3` always have bots; the list is High Seas, Cape Fuji,
AI-3, AI-2, AI-1, so **"AI - 1 on 1" is digit 5**.

Two traps the harnesses under `emu/` exist to avoid repeating:

* **MAME's `machine.time.seconds` returns the INTEGER second**, so a Lua
  script written in fractional seconds collapses a press and its release
  into one frame and the key is never seen. Count frames instead.
* **`/ready` toggles, and the server's start countdown is wall clock**, so
  a script that keeps pressing FIRE in the lobby un-readies and cancels
  the countdown every time. Press once and wait for the status to move.
  The harnesses are driven by the server's status byte rather than a
  fixed timeline for exactly this reason.

## Not here

Astrocade parity, minus the help screen (dropped for ROM): no JSON, no
banking, no appkeys/username persistence (default name ARCADIA, editable
per boot), no torpedo animation -- every poll is one opaque redraw. The
winner's ship layout is not revealed at game over (the wire carries it in
`myShips[5-9]`); the winner's *board* is shown instead. Never run on real
hardware -- none exists.
