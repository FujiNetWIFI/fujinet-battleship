# Battleship for the Odyssey 2

Fuji Battleship for the Magnavox Odyssey 2 / Philips Videopac, over the
FujiNet O2 cartridge. It plays the same server as every other client: 2-4
seats, bots on the AI tables, and one shot that hits every live opponent at
once.

It is 8048 assembly (Macroassembler AS), built as a **protocol 2 (APPBANK)**
image: the fixed 1K of cartridge RAM at `$400-$7FF` holds the display engine,
and eight 2K pages at `$800` hold everything else. It needs firmware with
APPBANK (fujinet-firmware PR #2177, merged 2026-10-24).

Only tested in MAME (`-cartslot fujinet`), never on a real console.

## Playing

| Where | Stick | Button | Keys |
|---|---|---|---|
| Name (first run) | | | letters, digits, space; CLR rubs out, ENT ends |
| Tables | up/down | join | |
| Lobby | | ready / not ready | |
| Placing | moves the ship | keeps it | R turns it, Z places the rest |
| Battle | moves the cursor | fires | 0 your fleet, 1-3 an opponent, T the target |
| Anywhere at a table | | | L leaves it |

The name is FujiNet's shared one (appkey creator 1, app 1, key 0). If it is
empty you type one, and it is kept there.

## The screen

The 8244 shows 28 characters a frame, and the grid is 9x8, so four boards at
once (the 2600's and the Studio II's layout) can't be done. The O2 shows
**one full 10x10 board at a time**:

- **Target** (yours to fire at; shown on your turn): one board for all the
  live opponents. Red where any has a hit, a white dot where all have it
  marked (firing there is refused), a dark dot where only some have.
- **Your fleet** (shown on the others' turns): your ships, and their shots.
- **An opponent's field** (keys 1-3): its water in its seat's colour.

A key's board stays until the turn changes. T0 at the top says whose turn
it is, in their colour, with the seconds left. T1 at the bottom shows each
seat's name and ships afloat, in its colour. At game over the winner's board
shows their fleet at last.

### How it's drawn

`src/fixed.inc` re-points the characters between rows from a line-counter
interrupt (the M.U.L.E. client's engine):

- A board row is singles 0-9, written in the gap after the row above from an
  internal-RAM buffer staged while that row displays.
- A cell shows only rows 3-6 of its glyph: the pointer is `glyph*8 + 3 - Y/2`,
  so the VDC stops at the end of the glyph's 8-byte slot. A row is 8 lines,
  and the pitch 18.
- The four quads carry the 16-character text lines: T0 in the vblank, T1 in
  the gap after row 9.

Measured in MAME (`make spike`, `PITCH=`): pitch 18 holds with about a line
to spare, and 16 drops lines. The gaps before row 0 (`ROWGAP` 10) and before
T1 (`TGAP` 18) are at their floors.

## Building

```
make                                  # build/battleship.bin, the live server
make ENDPOINT=http://localhost:8085/  # another server
make spike                            # the engine's test pattern
make run                              # MAME, in a window (run.sh)
```

The build needs `~/asl/asl` and `p2bin` (`ASL=`, `P2BIN=`). After assembling
each page, `build.sh` runs these checks, each for a fault that is silent at
run time:

- `checkflags`: a JZ/JNZ that tests a MOV instead of the ALU result.
- `checkobj`, `checklayout`: code overlapping, or a pinned routine growing
  into the next `org`.
- `checkmb`: a cross-bank call with no `sel mb` restore.
- `checkpage`: a MOVP table outside its reader's 256-byte page.
- `checkstack`: the interrupts plus the deepest call chain must fit the
  8-level stack.

It also checks that the fixed half is the same bytes in every page.

### Pages

| Page | |
|---|---|
| 0 | boot: cart check, name, table list, join |
| 1 | the network: CLOSE, OPEN, STATUS until settled, READ; one step a frame |
| 2 | the main loop: keys, stick, polls, dispatch by status |
| 3 | lobby |
| 4 | placing the fleet |
| 5 | the battle's states: boards, seats, sounds |
| 6 | game over |
| 7 | the battle's ticks: keys, cursor, fire, clock |

A phase page can't farcall, so the battle is split across two pages: one for
its states and one for its ticks.

Every reply is checked against the length its header implies (lobby
`59+10n`, placement `49+10n`, play `49+115n`). A short read is a failed poll,
and the next poll tries again.

## Testing

Everything runs headless in MAME through `tools/o2sim.py`. The rig never
touches the production server or the everyday fujinet-pc on 9995.

```
make shot      # the spike: snapshots + tools/rowcheck.py
make test      # boot, lobby, placement, battle against a private rig
make soak      # an AUTOPLAY=1 build plays AI-1 to game over and back
```

`test/rig.sh SCEN OUT [SECS [IMAGE]]` sets up the rig, then runs the
scenario:

- It builds and starts the server from
  `~/Workspace/servers/fujinet-game-system/battleship` on port 8085.
- It starts a private copy of fujinet-pc-rs232 on BoIP port 19970, with the
  name appkey seeded.
- `HOLD=1` keeps the rig up afterwards, for `./run.sh` or a second player
  (`test/player.py`).

An `AUTOPLAY=1` build readies itself, places its fleet at random and fires
at the first cell it may.

## Not done

- **Lobby handoff (appkey key 5).** There is no O2 lobby, and an APPBANK
  image can't boot another image until the console is power-cycled.
- **PAL.** The band timing is NTSC's.
- **Real hardware.** T1 sits at lines 228-241, which a TV's overscan may
  clip.
