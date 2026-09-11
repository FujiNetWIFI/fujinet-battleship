# Battleship for the Fairchild Channel F

A standalone F8 assembly client for the [Battleship
server](https://battleship.carr-designs.com/), in the mould of `../arcadia`
and `../astrocade`: the shared C core assumes a keyboard and kilobytes of
buffers, so the game is written to the machine instead. The game logic is the
Arcadia port's, re-cut for a console that has the opposite problem.

Built and tested in MAME through a real fujinet-pc BoIP listener, against a
local server.

## Why this console is the roomy one

Astrocade, Arcadia and ColecoVision all push both mailbox directions through
the *read* path, because none of those cart edges carries a write strobe. That
is why the Arcadia client lives in 84 bytes of RAM and the Odyssey² client in
39.

A Channel F cart is not a ROM. It is a peer on the F8 bus that services read
**and** write cycles (`ROMC 05` is a store), so the FujiNet cart hands the
console 16K of ROM at `$0800`, **30K of read/write RAM at `$8000`**, and the
whole 1K reply window flat at `$F800`. `GMAXLEN` is 509, so a `/state` reply
fits the window entire: a player record is one `DCI` and a gamefield is a
straight walk. No slices, no cursor, no bounce buffer.

What *is* scarce is pixels. VRAM is 128x64 at 2bpp, **write-only** through I/O
ports, and a plotted pixel costs about 80 µs — a full-screen clear is most of a
second. Everything below follows from that.

## The screen

```
  95 x 58 safe pixels, three palette BANDS

+---------------------------------------------+
| HIT                                     41  |  rows 0-9   palette 0
+------------+------------+                   |
|            |            |  YOU              |
|   Q1       |   Q2       |  CLYD             |
|            |            |  * * * * *        |  rows 10-53 palette 1
+------------+------------+  MEG              |
|            |            |  * * * * *        |  LTBLUE ground = the sea,
| Q0 = YOU   |   Q3       |  KIRK             |  and it costs zero pixels
|            |            |  * * * * *        |
+------------+------------+                   |
| G3      * * * * *      MODE=OUT             |  rows 54-63 palette 0
+---------------------------------------------+
```

### The palette is banded, per scanline

A Channel F pixel is two bits, and what those bits *show* depends on the row's
palette, chosen by the pixel values in columns 125 and 126 **of that row**.
Every sibling Channel F program picks one palette for the whole screen. This
one uses two:

| rows | palette | |
|---|---|---|
| 0-9, 54-63 | 0 | BLACK ground, WHITE ink — the only white this machine has |
| 10-53 | 1 | LTBLUE ground, then BLUE / RED / GREEN |

So the status row and the footer get maximum contrast, and **the open sea is
the cleared screen** — value 0 in palette 1 is light blue, and an empty cell
costs nothing to draw. `DPALR` sets a row *range*; columns 125/126 are outside
the visible area, so writing them is free, and it has to run *after* a clear
because the BIOS clear paints those columns too.

| | value | used for |
|---|---|---|
| LTBLUE | 0 | open sea |
| BLUE | 1 | the frame and divider cross, miss splashes |
| RED | 2 | hits, damaged hull, the player to move |
| GREEN | 3 | your hulls, the reticle, ships still afloat |

GREEN never appears on an enemy board except as the reticle, so the reticle can
never be mistaken for a hit.

### The boards scale with the table

A 10x10 board of 4x4 cells is 40x40, and two of those sit side by side in the
safe area with room to spare — but four do not, because 58 rows will not take
two 40-row boards. So:

* **2 seats** — `LAYHU`, two boards of **4x4** cells, yours on the right.
* **3 or 4 seats** — `LAYQD`, four quadrants of **3x2** cells around a divider
  cross, with a roster panel down the right.
* **placement** — `LAYPLC`, one 4x4 board of your own and the fleet list beside
  it.
* **lobby, tables, name, help** — `LAYTXT`, 23x9 text cells and no band at all.

A **3x2 cell is very nearly square on a 4:3 set**: the console puts 128x64 into
the raster, so a pixel is about 1.3x taller than it is wide. 2x2 would look
tall and waste the horizontal room this machine actually has.

Every layout is a record copied into `VLAY`, so `board.inc` has exactly one
code path and the cell renderer never knows which screen it is drawing.

### Nothing is written twice

Two decisions, and between them they are the whole frame budget.

**Cell art is 2bpp and opaque.** One byte is four pixels, the MSB pair
leftmost — which is already the `CVALn` encoding `DCELL` hands to port 1. So a
cell repaint writes each of its pixels exactly once and never needs an erase
pass first. At 3x2 that is six pixel writes per cell. A 1bpp stamp would only
*set* pixels and every cell would cost twice as much.

**Boards repaint by dirty cell.** A 400-byte shadow in cart RAM holds the state
each cell was last PAINTED as; a poll computes what every cell *wants* to be
into a 100-byte scratch and only the differences reach the screen. A typical
poll dirties one to three cells. **Moving the reticle dirties exactly two.** No
sibling console can afford the shadow — the Arcadia client has 84 bytes of RAM
in total and repaints both boards every poll because it must.

That is also what makes the shot animation affordable: three frames of a splash
on every live enemy board is one cell per board per frame.

The sea used to carry a lattice dot per cell. At 3x2 a cell has six pixels, so
that was a sixth of every cell forever and the board read as a blue stipple.
It was replaced by a drawn frame and the divider cross, which are chrome and
are painted once when the layout loads.

## Controls

| | |
|---|---|
| stick | aim, move a ship, walk the keyboard |
| plunger (push down) | fire, select, ready up |
| HOLD | turn the ship you are placing |
| TIME | help — and it accepts your name on the keyboard |
| MODE | leave the table — and deletes on the keyboard |

## The name

It comes from the FujiNet **shared username appkey** (creator 1, app 1, key 0),
the same one every other client in the family reads, so a player who has typed
their name into 5 Card Stud or CONFIG never types it again. Only when that key
is empty does the on-screen keyboard come up, and what is typed is written
back. The Arcadia and Astrocade ports skipped appkeys entirely and take a
default name per boot; they had no RAM for the buffers.

There is no way to invert a cell — VRAM cannot be read back at all — so the
keyboard's selection is a caret drawn in the gap row underneath, which is one
3-pixel bar to erase and one to draw per keypress.

## The rules that are not the board game's

**A shot lands on every live opponent at once.** That is what the help screen
exists to say; nothing on the play screen can teach it. One reticle, painted on
every board the shot will reach.

Five ships of 5, 4, 3, 3 and 2. A placement is `pos + 100*dir`. The wire's
gamefield only ever carries 0, 1 (hit) and 2 (miss) — `FIELD_SUNK` is internal
to the server's AI and is never serialised, so `shipsLeft` is the only thing
that says a ship went down.

## The invariants carried from the earlier consoles

Every one of these was learned the hard way on an earlier port, and they are
transcribed rather than rediscovered:

* **The turn edge.** `activePlayer` stays 0 for every poll of your turn, so
  testing the level machine-guns the cue. `VPRVACT` advances at the bottom of
  *every* successful poll, lobby included, so the lobby→play transition rides
  the same comparison, and the cue fires BEFORE targeting rather than after.
* **The ready toggle lives in the input path**, sampled every ~20 ms in the
  wait loop, never once per poll. A toggle read once every two seconds feels
  dead.
* **Validate before believing.** The reply window is never cleared, so a short
  read leaves stale bytes from a longer previous reply. `VALID8` gates on seats
  ≤ 4, a status on the enum, and the minimum length the two imply.
* **The place-phase trap.** During `STATUS_PLACE_SHIPS` a player record is
  name+status only — 10 bytes, not 115. Nothing indexes a gamefield there.
* **The clock bails.** The server skips a turn that runs out; targeting counts
  down locally and returns, so the client never stops polling.
* **The prompt is empty for the whole of play** — the server sets it
  deliberately — so the status line is synthesised from `activePlayer` and
  `status`: YOUR TURN / MISS / HIT / SUNK.
* **The result cue rides the status edge**, not your own shot, so an opponent's
  shot is heard too.
* Placement seeds a **random legal fleet** and lets the player adjust it.
  Illegal moves are refused with a tone, never clamped.

And one improvement on the siblings: this client sends **`/ready/1` and
`/ready/0`**, not the deprecated bare `/ready` that toggles. That removes the
whole class of bug where a press between two polls readies and un-readies, and
where a harness holding FIRE cancels the server's countdown every time round.

## The five transport rules, plus two

`net.inc` carries the family's five verbatim:

1. The CLOSE belongs at the **start of the next** request, never after the
   READ — every transaction repaints the whole reply window, and every screen
   renders straight out of it between polls.
2. Capture the reply length **immediately** after the READ.
3. Poll STATUS until two consecutive readings agree, bounded.
4. The 4th STATUS byte is `nDevStatus_t` and checking it is **not optional**:
   the GET is deferred to the first STATUS, and an HTTP error page has a
   perfectly readable body.
5. Validate the length against what `playerCount` implies before believing it.

The Channel F 5 Card Stud port added a sixth: two agreeing readings are not
enough on their own — a `/state` settled at 129 bytes of a 385-byte response —
so `NSETTLE` also waits for a caller-supplied **minimum**. Here that minimum
comes from `GMINLN` run against the PREVIOUS poll's seat count and status, so
nothing has to guess; if the count shrank, `VALID8` rejects the short reply and
the next poll retries with a lower floor.

And a seventh, which is this port's own: **request 0 is `/tables`**, so a
cleared "pending request" cell cannot mean `/state`. `VPEND` of zero means
nothing is staged, and `SGPOLL` substitutes `RQSTATE` explicitly. The first
version did not, and every poll of every game refetched the table list.

## F8 traps this port paid for

* **`PI` and `JMP` clobber the accumulator** — both stash the target address's
  high byte in A (`m_a = m_dbus` in MAME's `f8_pi`). A does not survive a call.
  The demo name table tested A after a `PI`, got the high byte of its own
  address, and walked DC0 two and a half kilobytes into the ROM.
* **And neither does a RESULT in A survive the next memory read.** `MUL10`
  hands back `10*y` in the accumulator, and the `LM` two instructions later
  loaded `x` over it — so every shot went out as `x + y` instead of `x + 10y`,
  and the whole board collapsed into row 0. Capture a returned value into a
  register on the very next instruction.
* **RAM scratch aliases just like a register does.** `PLSAVE` sheltered the
  candidate placement in the shared `VTMP` cells, which is also where `PLOCCB`
  keeps its ship-list counter — and every move calls one then the other. The
  counter ran past its own terminator marking ships that do not exist, and the
  ship walker then marched a length read out of a random byte straight across
  the cell shadow. Scratch that spans a call needs its own address.
* **`DELAYMS` units are not milliseconds.** One is about 9 ms. Every pacing
  constant in the port was written as if it were one, which turned a
  two-second lobby poll into half a minute and gave `NSETTLE` a
  fifty-four-second worst case.
* **The F8 keeps exactly one return address.** `ENTER`/`LEAVE` push PC1 onto the
  BIOS K stack at r40-r58 — nine levels. Every state is entered by `JMP` and
  left by jumping to `GOTO`, so the `ENTER` at the top of a state never meets
  its `LEAVE`; **`GOTO` rewinds r59 to 40**, or the tenth visit pushes onto the
  stack pointer itself.
* **Drawing eats the register file.** `DSTR`, `DCLRR` and `DCELL` between them
  write r0 and r2-r8. Every loop counter and every error code lives in RAM.
  Arguments that must survive a draw go in r9, r10 or r11.
* **A pointer is the one thing that DOES survive a call** — `PI` does not touch
  the data counters — so the routines that pick a string return it in DC0.
* **Order of operations around DC0.** Anything that needs a RAM value *and* a
  built pointer must read the value FIRST: a `DCI` to fetch it afterwards wipes
  the pointer. Three routines here were written the wrong way round.
* **Unsigned comparisons need the carry, not the sign.** 181 − 1 is 180 with
  bit 7 set and reads as negative under `BM`.
* **Relative branches reach ±127.** Five loops here needed a `JMP` trampoline.
* **The footer clear wipes the footer.** Your own ship pips sit on the footer
  row, so the chrome is drawn BEFORE the boards, not after.

## Build and run

```sh
./build.sh                # build/battleship.bin, exactly 16384 bytes
DEMO=1 ./build.sh         # a static mock position, no network -- art work
make shot                 # headless screenshot of that position
./run.sh                  # MAME with the fujinet cart device, windowed
make smoke                # headless: join the AI table, place, play
make probe                # headless play with a running state trace
```

The image must be **exactly 16K with the `FUJI` claim at `$47FC`**, or the cart
boots it with the mailbox dead and the arena at `$8000` reverts to plain RAM.
`tools/checkrom.py --claim` enforces that on every build and
`tools/checksize.py` reports per-module sizes against the 16K window.

Environment: `ENDPOINT` (default `https://battleship.carr-designs.com/`,
regenerated into `src/endpoint.inc` on **every** build so a stale value cannot
survive an environment change), `ASL` / `P2BIN` (default `~/asl`), `MAME_DIR`,
`FUJINET_TCP` (default `127.0.0.1:9995`), `FUJINET_DEBUG=1` to log every
mailbox transaction, `FAST=1` to unthrottle a headless run.

`run.sh` needs the MAME tree with the Channel F FujiNet cart device grafted in
(`fujinet-firmware/pico/channelf/emu/apply.sh`) and a fujinet-pc BoIP listener.

Against a local server:

```sh
cd <servers>/fujinet-game-system/battleship && go run .
ENDPOINT=http://127.0.0.1:8080/ ./build.sh && make smoke
```

Tables `ai1`..`ai3` always have bots; the list is High Seas, Cape Fuji, AI-3,
AI-2, AI-1, so **"AI - 1 on 1" is the fifth row**.

### The harness traps

* MAME's `machine.time.seconds` returns the **integer** second, so a script
  written in fractional seconds collapses a press and its release into one
  frame and the key is never seen. Count frames.
* **A state variable changes before the screen it belongs to is drawn**, and a
  BIOS clear is most of a second. A press issued the instant the state moves is
  released before the client's first input scan runs, and edge detection cannot
  see a key that was never held across a scan. Every step waits for the state
  to be *stable*.
* **Headless runs are throttled on purpose.** The server's start countdown and
  its move clock are wall clock, so a run at 250x emulated speed sits at
  "starting in 2" forever.
* `manager.machine.ioport.ports` must be re-fetched, not cached.

## Toolchain

Macro Assembler AS with `CPU F3850`, the same assembler the Arcadia (2650) and
Odyssey² (8048) ports use. It is not packaged; build it:

```sh
curl -O http://john.ccac.rwth-aachen.de:8000/ftp/as/source/c_version/asl-current.tar.gz
tar xzf asl-current.tar.gz && cd asl-current
cp Makefile.def-samples/Makefile.def-x86_64-unknown-linux Makefile.def
make -j$(nproc)
mkdir -p ~/asl && cp asl p2bin ~/asl/
```

`p2bin`'s `-f` filters records out and would silently produce an empty image;
`-s` would append a checksum byte. Neither is wanted.

## Status

Working end to end in MAME against a local server: the shared-username appkey,
the on-screen keyboard, the table list, joining, lobby ready-up, a random
seeded fleet with move / turn / confirm and local overlap rejection, the live
board render, targeting with the lockstep reticle and a local countdown, the
shot animation, hit / miss / sunk cues, and RESET continuity. A `make smoke`
run plays a real game: one `/place`, one `/ready/1`, and a hundred-odd
`/attack`s marching across the board, with the server reporting MISS, HIT and
SUNK back.

**Not yet exercised end to end**: the game-over screen. The winner's board and
the `myShips[5-9]` hull reveal are written and the status byte reaches them,
but no smoke run has yet sunk a whole fleet, so that path has only been
reasoned about, not watched. It is the first thing to check next.

`make resettest` is the one worth watching: it soft-resets the console
mid-session and the cart's sequence numbers keep climbing across the reset
(1-6, reset, 7-12) instead of starting again — which is the whole reason
`FNGO` takes them from the cart's persisted `ACKSEQ` rather than a counter in
the client's own RAM, since a console reset restarts the client and not the
cartridge.

About 10K of the 16K window; the 30K arena is barely touched.

## Not here

No hardware. No Channel F FujiNet cartridge has been built, and the bus
**timing** — which edge of WRITE marks a cycle, and when the CPU's write data
is valid — is marked PROVISIONAL in the cartridge firmware and cannot be
settled without a scope. Nothing here has run on real silicon.

Also deliberately out of scope, as on every console port: device-slot
management, mount/eject, and the lobby room handoff.
