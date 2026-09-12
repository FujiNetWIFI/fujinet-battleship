# Battleship for the Atari 2600

A 6502 client for the [Battleship server](https://battleship.carr-designs.com/),
running over the FujiNet cartridge mailbox from the 2600 bring-up at
`fujinet-firmware/pico/atari-2600`. The game logic is the Arcadia port's,
which is the astrocade's, which is the Intellivision's: binary `?bin=1&v=2`
wire format, zero-copy rendering straight out of the reply window, and a
CLOSE at the *start* of the next request rather than after the read.

    ./build.sh            # -> build/battleship.bin, 8192 bytes
    ./run.sh              # a window
    ./run.sh bsplay       # headless, plays a whole game, prints PASS or FAIL

`build.sh` needs Macroassembler AS and the firmware tree; `run.sh` needs a
MAME with `pico/atari-2600/emu/apply.sh` applied and a fujinet-pc listening.

## Why this console is different

Every other port in this family has somewhere to put things. This one has
**128 bytes of RAM, and the stack mirrors into the top of them.** A single
gamefield is 100 bytes. The reply this program reads is up to 509.

So nothing is buffered. The server's reply stays in the cartridge's 512-byte
window and is read from there in place; the board is composed *by the
cartridge* out of that same window; the URL is streamed a character at a time
into the cartridge's TX page and never assembled anywhere. What the console
holds is a cursor, a phase, and a table id.

The table id is the one exception, and it is worth naming: ten bytes at
`BSTBL`, copied out of the `/tables` listing when you join, because every
`/state` poll after that repaints the window the listing was sitting in.

## The screen

21 rows of 12 columns, from the 3x5 font the cartridge keeps. **One board at
a time** -- two 10x10 grids and a status line do not fit, and at this size one
board reads better anyway.

```
row  0     the server's prompt, truncated
row  1     whose board this is, and the move clock
row  2     the column ruler
rows 3-12  the board: column 0 the row digit, columns 1-10 the cells
row 14     the cursor's coordinate
row 16     the viewed player's name
row 20     the hint, or an error as "E<step> <code>"
```

Cells are `.` open sea, `X` a hit, `O` a miss, `#` one of your own hulls. The
cursor is drawn as `+` **over** whatever is under it and blinks, because on a
board where the thing under the cursor is the whole point, a solid cursor
hides the one cell you are looking at.

`FN_BLIT_FIELD` does all of that: six stores, and the cartridge turns 100
gamefield bytes into ten rows of ready-to-stream glyph bytes. On the 6502 the
same thing is a hundred reads through a 16-bit reply cursor and about 250
bytes of code, in a bank that has under a hundred to spare.

## Controls

| | lobby | placement | in play |
|---|---|---|---|
| stick | -- | move the ship | move the cursor |
| FIRE | ready up | put the ship down | attack that cell |
| SELECT | -- | rotate | show the next player's board |
| RESET | back to the table list | | |

The placement screen is not reached by pressing anything: the client hands
over as soon as a `/state` poll says the phase is placement **and its own
player status is still PLACE**. Both halves matter -- the phase stays at
placement until everyone has placed, so testing the phase alone would drop you
back into the placement screen the moment you finished.

## Banking

8192 bytes: three 2K banks then the 2K fixed half.

```
bank 0  bslobby   a name, a table, and joining it
bank 1  bsgame    poll /state, show a board, attack        2017 / 2048
bank 2  bsplace   put five ships down
fixed   bstail    the cold stub and the bank trampoline
```

Each bank carries **its own copy** of the mailbox library, the network layer,
the URL builder and the display kernel. There is nowhere else to put them: the
high 2K of the window is the cartridge's mailbox in its entirety, so there is
no fixed code region, and the fixed tail carved out of the status page is 220
bytes. That is what F8 games have always done.

It is also why those modules are guarded on `BSBANK`. At 2048 bytes a bank,
what a bank never calls is worth leaving out: the lobby carries no `/attack`
and no decimal-to-wire converter, the play bank carries no `/place`, and the
whole client would not fit if they did.

Two things must be at addresses that do not move, and both are in the fixed
tail. `BSGOTO`, because the store that switches bank is the last instruction
fetched from the *old* bank and the very next fetch comes from the new one.
And `BSCOLD`, because this console has no reset line to the cartridge: the
RESET switch restarts the 6507 with whatever bank was last selected still
mapped, so a cold stub in bank 0 would not be there when it was needed.

## Traps paid for here

**`sta (zp),y` is banned near the write ports.** It always performs an
internal read at the address it is about to write, and on a page the cartridge
does not drive that read is decoded as an access carrying the floating bus. A
decimal printer that dispatched between the TX page and the text port through
an indirect store would append a garbage character before every real one.
`BSDEC` dispatches through an indirect `JMP` to an absolute store instead, and
`tools/checkrom.py` fails the build on the addressing mode.

**`p2bin` pads to its `-r` range**, so a bank that runs past `$17FF` is
truncated *silently*: the image builds, the size is right, `checkrom` passes,
and the instructions past the end are simply gone. The play bank did exactly
that, by two bytes. `build.sh` now reads each bank's end address out of the
assembler's own listing and refuses to build.

**A delay loop that counts in a register a subroutine clobbers.** `APWAIT`
counts in X and Y; its first caller counted its own repeats in Y as well, so
every wait ran 256 times over -- seventy seconds where seventeen milliseconds
were meant. It looks exactly like a server that never answers.

**The prompt is empty once play starts.** The server sends 33 NULs, so a
harness waiting for text on row 0 waits forever.

## Testing

    ./run.sh bstest       # the transport: a live table list and a board
    ./run.sh bsplay       # the game: ready, place, attack, and see it land
    BS_TABLE=AI2 ./run.sh bsplay

`bsplay` reports what it actually did rather than a fixed sentence, because a
public server keeps its tables between runs: the same script legitimately
joins a fresh lobby one time and a game already in progress the next, and a
PASS line claiming to have placed ships when it did not would be a lie about
the one thing the test is for.

The AI tables (`ai1`, `ai2`, `ai3`) are the only ones a lone client can play.
