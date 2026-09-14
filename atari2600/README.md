# Battleship for the Atari 2600

A 6502 client for the [Battleship server](https://battleship.carr-designs.com/),
running over the FujiNet cartridge mailbox from the 2600 bring-up at
`fujinet-firmware/pico/atari-2600`, in the `?bin=1&v=2` wire format the
Intellivision, Astrocade, Arcadia and Channel F ports share.

**Every board on screen at once, in colour.** Up to four 10x10 boards as
coloured cells -- blue water, red hits, white misses, gold brackets for your
own hulls and for the cursor -- on a console with 128 bytes of RAM, no
framebuffer, and a playfield that has exactly one colour per scanline.

```sh
make                       # build/battleship.bin, 16384 bytes
make layout                # the screen with no network at all
make shot                  # ...snapshotted and read back, cell by cell
make run                   # in a window
make drive                 # headless: a game against the live server (AI1)
make drive4                # ...at the four-seat table (AI3)
make frames                # every frame across a whole game must be 262 lines
make resetleave            # the RESET switch: menu, LEAVE, back to the list
make resettest             # a 6507 restart mid-game
make sounds                # the cues, logged as they fire
make hosttest              # the cartridge's playfield composer, byte for byte
```

`build.sh` needs Macroassembler AS and the firmware tree; `run.sh` needs a
MAME with `pico/atari-2600/emu/apply.sh` applied and a fujinet-pc listening.

## The screen

```
 YOUR TURN 45         the status row, synthesised (see below)
 BOB   >      ALICE   names over the pair: left owner, a marker each, right
 +-----------+-----------+
 |           |           |   board pair A, 80 lines: two 10x10 boards of
 |    Q1     |    Q2     |   8x8-pixel cells, abutting at pixel 80 with a
 |           |           |   two-clock black missile down the seam
 +-----------+-----------+
 YOU          CAROL
 +-----------+-----------+
 |    Q0     |    Q3     |   pair B
 +-----------+-----------+
```

Three or four seats use the family's quadrant convention: you bottom-left,
the others clockwise from top-left. **Two seats -- the AI1 table, the common
case -- get one pair of 8x12 cells, which is square on a 4:3 set, and the
lower half becomes text**: the server's prompt in full, each fleet as pips
(`#` afloat, `.` sunk), and a hint.

### How a playfield with one colour per line shows three colours per board

A cell is two playfield bits -- eight pixels -- so two boards side by side
are the whole 40-bit asymmetric playfield, rewritten twice a scanline. The
colour is decided **by line within the cell**: each line of the cell draws
a different table in a different colour.

```
 line 0    AUX  gold     your hulls; the cursor on the enemy boards
 line 1-2  HIT  red      a hit: the top of the block
 line 3-4  MID  white    hit OR miss: a hit's white core, a miss's dash
 line 5-6  HIT  red      the bottom of the block
 line 7    AUX  gold
```

So a hit is a red block with a white core, a miss is a white dash, and a
hull or the cursor is a pair of gold bars above and below the cell -- a
bracket that never hides what is under it, which is why the cursor does not
blink. A shot lands on every live enemy at once (the rule this game does not
share with the board game), so the same bracket is on every live enemy
board.

The tables are **composed by the cartridge**. Six 20-byte tables per kind
-- one per playfield register, one entry per cell row -- live in the text
plane region the cartridge already publishes, between text rows 4 and 15,
and four transforms the cartridge grew for this client fill them straight
out of the reply window: `FN_BLIT_PFIELD` (a gamefield's 100 cells to HIT
and MID), `FN_BLIT_PFHULL` (five placements to AUX), `FN_BLIT_PFCELL` (one
cell: the cursor, a pending hull) and `FN_BLIT_PFCLR`. Doing that on the
console would be four hundred reads through a 16-bit reply cursor in a bank
that has not got the bytes, into RAM it has not got at all. The console's
part is `lda table,y / sta PFn` eighteen times a line on a cycle-exact
schedule -- `dispgame.inc` has the windows.

### Every frame is 262 lines

The blanked bands are timed with the RIOT timer, as in the 5 Card Stud
port, and so is the **visible** band: the kernel's own lines add up to less
than 192 and a timed pad at the bottom absorbs the difference, so an
overrun moves nothing.

A network poll does not blank the screen and does not shudder. Four
things make that true:

- The network bank carries the board kernel and spends every wait for the
  cartridge drawing a frame out of the tables it is still holding
  (`NFRAME`, `NPGO`).
- **The overscan is waited out at the START of the next frame, not the end
  of the one before.** `DFRAME2` arms the timer and returns; `DFRAME` waits
  on it. So whatever a bank does between two frames -- stream a URL into
  the TX page, recompose the boards, switch banks -- is spent inside the
  overscan and moves nothing, as long as it fits in thirty lines. With the
  wait at the end, all of that landed between the overscan and the next
  VSYNC: a played game measured 96 frames in 2,400 that were 273 to 310
  lines, and every one was a transaction's setup or a recompose.
- **A switch INTO a frame is taken inside the vblank hook, and the bank
  entered finishes the frame** (`DFRAME2`, the kernel's second half). The
  RIOT timer does not care which bank is mapped. A switch that started a
  fresh frame from the hook would throw a 40-line frame at the set.
- **The transport draws one frame after every launch before its first look
  at the acknowledgement.** In emulation the cartridge answers inside the
  commit, so without that a whole transaction and the next one's setup --
  a URL streamed a character at a time -- piled into one overscan.
- **A poll's recompose is two passes**, in a bank of its own: the cue
  edges and the four boards between frames, in the overscan; the text a
  frame later, in the vblank the game bank's hook hands over from, and only
  the rows a poll can change -- the status row, the fleets, and the two
  turn-marker cells of each name row through `FN_BLIT_TCELL`. A screen
  change (a phase change, the menu closing) is three passes more: the rows
  blanked, the status and the names, the lower rows.

`make frames` histograms VSYNC-to-VSYNC across a played game and prints any
frame that is not 262 lines; `emu/banktime.lua` says where a long one went,
in scanlines, by bank switch.

### The status row

The server's prompt is empty for the whole of play, deliberately, so the
row is synthesised: `MISS YOU  45` / `HIT  ENEMY` / `SUNK YOU  12` -- the
last result, whose turn, and your clock, which counts down locally and polls
when it runs out (the server has moved on by then). The lobby shows the
server's own countdown; game over says `YOU WIN!` or `ALICE WINS`.

## Controls

| | lobby | placement | in play |
|---|---|---|---|
| stick | -- | move the ship | move the cursor |
| FIRE | ready up (a toggle) | put the ship down | attack that cell |
| SELECT | poll now | rotate | poll now |
| RESET | | the menu | **the menu: resume, how to play, leave** |

RESET is a switch on this console, a bit in a RIOT register the program
reads; it restarts nothing, and what it means is the client's to choose.
LEAVE sends `/leave` and then a fresh `/tables`, so the seat is given up
rather than abandoned.

The placement screen is not reached by pressing anything: the client hands
over when a poll says the phase is placement **and your own status is still
PLACE**. The phase stays at placement until everyone has placed, so testing
the phase alone would drop you back into it the moment you had finished.

A shot at a cell already resolved on every live enemy is refused with a
tone: the server refuses it silently and the turn never passes.

## Sounds

One TIA channel, a script engine that steps once a frame, and the family's
vocabulary: a click for every cursor step (one frame, so auto-repeat does
not drag), rising notes for a choice, a falling swoosh when a shot goes
out, a short low splash for a miss, a longer brighter explosion for a hit,
the explosion and three falling tones for a sinking, three rising tones on
your turn (before the draw: an alert after the fact is not one), a click on
the clock's last seconds, a low buzz for a refused move, and three long
falling tones at game over. The result cue rides the **status edge paired
with `lastAttackPos`**, so an opponent's shot is heard too and a result that
repeats across polls is heard once.

## Banks

Seven 2K banks and the fixed half, 16384 bytes. A bank switch replaces every
byte of `$1000-$17FF`, so each carries its own copy of every module it
calls; only zero page crosses, and the text planes, the playfield tables
and the reply window are cartridge state that survives.

| | | bytes |
|---|---|---|
| 0 `bslobby` | the cold start and the table list | 1156 |
| 1 `bsgame` | the board kernel, the cursor, the cues, a shot | 1718 |
| 2 `bsnet` | one request, with the picture up, and back | 1931 |
| 3 `bsmenu` | the RESET menu, the help, leaving | 1232 |
| 4 `bsname` | the keyboard and the shared username | 1341 |
| 5 `bsplace` | the five ships | 1971 |
| 6 `bscomp` | compose the game screen, in passes | 2042 |

The composer is a bank because the game bank came in 961 bytes over with it
in. The shared transport and the text primitives are in the fixed tail
(`bscore.inc`), the only region every bank sees at the same address;
`tools/mktail.py` reads their addresses out of the tail's own listing into
`build/tail.inc`, so there is no hand-kept list to go stale. The trampoline
resets the stack on every switch.

## The name

The shared FujiNet username -- appkey creator 1, app 1, key 0, the slot
every game in the family reads -- goes straight from the appkey reply into
a cartridge path buffer and from there into every URL; it never touches
console RAM. If the slot is empty, or the adapter has no SD card, a
three-row keyboard types one and writes it back. The joined table's id
lives in another path buffer for the same reason: every `/state` poll
repaints the window the listing came from.

## Traps paid for here

- **The cartridge blit is a single-slot request on the RP2040.** The bus
  core hands it to the other core and there is no queue, so a burst of
  thirteen would lose most of them on hardware -- and lose none in MAME,
  where a blit runs inside the store. The firmware now publishes
  `FN_B_BLITGEN` ($1F19), bumped when a blit has landed, and `FNBLIT` waits
  (bounded) for it before the next; `FNENDW` does the same for a text row
  through `FN_B_TEXTGEN`.
- **Missiles replicate with their player's copy count.** The divider is
  missile 1; with NUSIZ1's three close copies for the text there were three
  dividers. It is switched to one copy in the lead-in line before a board
  and back after.
- **A text row leaks.** The six-copy kernel goes on displaying its last two
  bytes down the screen until something blanks them, and the row routine's
  exit is too long to do it on the last ink line: a `jsr` right after its
  `rts` lands at cycle 79 and costs a scanline without anything looking
  wrong. Every text row here has a seam line that blanks, and the routine
  returns early in it.
- **NTSC hue 4 is pink** in MAME's palette; red is hue 3.
- **The text block is at clocks 52-99**, not 46-93: a player positioned by
  `RESPn` completing at cycle N lands at clock 3N-63, a missile one clock
  earlier. Both measured from the raster with `tools/pfcheck.py`.
- **A "done" flag in the network bank must not alias a settle-loop cell.**
  Its hook runs inside every frame the settle loop draws; an alias of the
  delay counter switched to a garbage bank mid-poll, and the symptom was a
  cold start after the first `/state`.
- **Placement scratch must not alias the game bank's edge memory**, or the
  first poll after placing hears a phantom shot.
- **The lobby's row count comes from the listing's count byte**, set in the
  validator; a `/tables` that is only length-checked lists nothing.
- **A cold start's RAM clear writes every zero-page cell**, so a Lua write
  tap on the request cell sees a spurious "request 0 in bank 0" -- that is
  the clear, not a fetch.
- Everything the 5 Card Stud port learned still applies: RAM is not cleared
  by a reset and the cold stub forces a cold entry; the RESET switch is a
  switch; MAME must run from its own tree; throttled is not a performance
  choice, because the server's countdown and move clock are wall clock.

## How it is checked

| | |
|---|---|
| `make hosttest` | the four playfield transforms against a picture of the tables built from the bit map in prose, every slot, poisoned outside |
| `make layout && make shot` | the kernel with no network: tables baked into the image, snapshotted, and every cell line of every board read back and required to be the colour the picture says (`tools/pfcheck.py`), plus the divider at 79-80; both layouts |
| `make drive` | types a name if asked, picks the table off the screen, readies, places, fires until a result shows; after every poll reads the reply window's gamefields and the tables back and requires them to agree byte for byte |
| `make frames` | the same game with a VSYNC tap: nothing but 262 |
| `make resetleave` / `make resettest` | the switch and the restart, which are two different things |
| build gates | `checkdefs.py` (the equates against the firmware header), `checkrom.py` (size, the `FUJI` claim, no RMW or indirect store on the control pages), `checkbanks.py` per bank, `mktail.py` on the tail. All fail the build. |

## Not here

- **Real hardware.** The firmware builds and its host tests pass; bus timing
  is the one thing emulation cannot settle.
- **PAL.** The line counts are NTSC.
- **The leftmost and rightmost cells sit at the very edge of the active
  area.** Two 80-pixel boards are the whole line; a CRT that overscans may
  clip a column. There is no playfield-granularity fix, and MAME shows all
  160.
- **Vertical ships read as a ladder of rungs** (a bracket per cell); a
  shape at eight pixels has no better answer.
