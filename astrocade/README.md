# Battleship for the Bally Astrocade

A standalone Z80 assembly client in the texasHoldEm/astrocade mold: the
shared C core cannot fit this machine, so the game is written to it. It
talks to the Battleship server through the FujiNet Astrocade cartridge —
the RP2040 mailbox cart from `fujinet-firmware/pico/astrocade` — and its
gameplay logic is transcribed from the Intellivision port (`intv/`), the
freshest client written directly against the binary wire format.

Built and tested against a local server in MAME through a real
fujinet-pc BoIP listener.

## Building and running

    ./build.sh                # build/battleship.bin, exactly 8192 bytes
    ./run.sh                  # MAME with the fujinet cart device
    make smoke                # headless end-to-end test (see below)

Environment:

  * `ENDPOINT=` — game server, default
    `https://battleship.carr-designs.com/`. Regenerated into
    `build/endpoint.inc` on every build.
  * `ZMAC=` — assembler override. Otherwise zmac is taken from `PATH`,
    then `~/Workspace/zmac-1.3/zmac`, then `$FUJI_PICO/tools/zmac/zmac`.
  * `FUJI_PICO=` — the cartridge bring-up tree, only consulted as the
    last place to look for zmac.

The port is self-contained the way the Texas Hold'em client is:
`tools/checkrom.py` is vendored here and `fujilib.inc` / `HVGLIB.H` live
in the port, so a build needs no particular branch of the firmware tree
checked out. Those are copies of the bring-up's `testrom/` files — keep
them in step. `assets/font.inc` is the card-game ports' committed 4x6
font (originally generated from the Lynx 4x6 BMPs by 5cardstud's
`tools/mkfont.py`), vendored byte-for-byte.

`run.sh` expects the MAME tree with the fujinet cart device grafted in
(`pico/astrocade/emu/apply.sh`) at `MAME_DIR` (default `~/Workspace/mame`)
and a fujinet-pc BoIP listener at `FUJINET_TCP` (default 127.0.0.1:9995).
At the on-screen menu, keypad **1** starts the game.

## The cartridge budget

The cart serves an 8K window; the mailbox owns 1B00H up, so code and
data end at 1AFFH — 6,912 bytes, enforced by `checkrom.py` and itemised
by `tools/checksize.py` on every build (the `MB_*` labels in `battle.asm`
are its module fences). `build.sh` stamps the `FUJI` claim signature at
1CFCH, so when this image is booted over the network the cart keeps the
mailbox alive for it.

RAM is screen RAM, full stop. 90 visible lines use 4000H–4E0FH and
everything above is the game's — see the map in `battle.asm`. Interrupts
stay off for the program's whole life (fujilib's contract: with I = 0,
refresh strays land in OS ROM and never hit the hotspots).

## The board

Everything rides one alignment trick: at 2bpp four pixels are one byte,
a board cell is 4x4 pixels = one byte per row, and the whole layout
lives on a 4-pixel grid, so no blit in the game ever shifts. A 10x10
board is a 10-byte-wide, 40-line square; four of them fit with a
divider cross in the left 84 pixels, and the right 19 byte columns hold
the panel. Player i is quadrant i — the server rotates its player list
so this client is always index 0, the bottom-left board.

```
bytes 0-9 | 10 | 11-20 | 21 |   char cols 22-39
   q1 (TL)  div  q2 (TR)      per player i, rows 3i / 3i+1:
   ------ divider ------        TAG NAME      TURN
   q0 (YOU) div  q3 (BR)        #####         SUNK
row 14: server prompt                    clock (3 digits, red)
```

Four palette colors: sea blue, black, red, white. Cell states are
silhouettes, not hues — flat sea, solid red hit, a white splash dot for
a miss, solid white hull, a red core in the hull for your damaged
ships, a white-cored red block for the game-over reveal, and a white
ring for the cursor (which doubles as the pending placement, blinking).
The palette bytes were derived from MAME's `astrocade_palette()` math;
blue lives at the top of the hue circle (0F2H), not at hue 15, whatever
older notes say.

## The Intellivision lessons carried over

* **The turn edge.** `activePlayer` stays 0 for every poll of your
  turn, so it alone cannot tell "your turn just began" from "still your
  turn". `PRVACT` advances at the bottom of every successful poll —
  lobby and placement included — so the lobby->play transition rides
  the same comparison, and the cue fires BEFORE targeting (which blocks
  for up to a slice).
* **The ready toggle lives in the input path**, sampled every ~20 ms in
  `GLWAIT` and edge-detected, never in the once-per-poll renderer: a
  quick tap between polls is not missed, one press cannot ready and
  un-ready across two polls, and the trigger that joined the table
  cannot toggle on arrival while still held.
* **Validate before believing.** The reply window is never cleared, so
  a short read leaves stale bytes from a bigger previous reply.
  `VALID8` gates on the header, `playerCount <= 4`, a status byte on
  the enum, and the minimum length that status implies — and during
  PLACE_SHIPS the player records are 10 bytes, not 115, so nothing
  indexes them in that phase.
* **The clock bails.** The server enforces its own move limit and skips
  a turn that runs out; a client that blocks forever in targeting never
  polls again and never learns the game moved on. Targeting counts down
  locally and returns at zero.
* **One cursor, in lockstep.** A shot fires at that coordinate on every
  other playing board simultaneously, so the single cursor ring paints
  on all live enemy quadrants at once.

And one improvement over the Intellivision: targeting is time-sliced
(~5 s, restarted by any keypress) instead of fully blocking, so the
client keeps polling even inside the 250-second single-player clock.
The erase path needs no shadow boards at all — the reply window IS the
shadow, and cursor cells repaint from the wire.

## Controls

    stick / keypad arrows   move the cursor / lists / the name wheel
    trigger                 select / ready up / place a ship / FIRE
    keypad 0                rotate the pending ship; poll now elsewhere
    CE                      leave the table (name screen from the list)
    .                       how to play

## Testing

`make smoke` runs MAME headless with `emu/smoke.lua` against a LOCAL
server: launch, accept the default name, join "AI - 1 on 1" by digit 5,
ready up, confirm the five seeded ships blind (seeds are always legal),
then fire at the resumed cursor whenever the turn comes around;
snapshot to `build/astrocde/0000.png`. `FUJINET_DEBUG=1` (default) logs
every mailbox transaction; a `/state` for an N-player game reads back
`49 + 115*N` bytes.

`emu/resettest.lua` is the RESET-continuity check: join, soft-reset the
console mid-session, rejoin, and confirm the sequence numbers continue
instead of restarting — they come from the cart's persisted ACKSEQ,
never a local counter.

Against a local server:

```sh
cd <servers-repo>/fujinet-game-system/battleship && go run .
ENDPOINT=http://127.0.0.1:8080/ ./build.sh && ./run.sh
```

The hidden `test` table (`?table=test`) is joinable directly but never
starts without a second player; the `ai1`..`ai3` tables always have
bots to play against.

## Status

Working end to end against a local server in MAME: table list, join,
lobby ready-up, random-seeded ship placement with move/rotate/confirm
and local overlap rejection, the four-quadrant live render, targeting
with the lockstep cursor and local countdown, hit/miss/sunk result
cues, and RESET continuity. Not yet done: appkey persistence for the
username and the lobby room handoff (core-only scope for v1), the
torpedo launch animation, and nothing has run on real hardware, because
the cartridge itself has not been built.

## Bank switching

Firmware protocol v2 supports banked carts: `fujilib.inc` now carries the
`FNBKSEL`/`FNBKMAX` equates (one read maps a 4K image page into
2000H-2FFFH with the mailbox fully live; the high half never moves). This
client still fits the single 8K window and does not use them -- see
`firmware/include/fuji_mailbox.h` in fujinet-firmware for the scheme.
