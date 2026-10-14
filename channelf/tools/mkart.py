#!/usr/bin/env python3
"""Emit src/art.inc: the board cell art, as 2bpp opaque cell bitmaps.

Every cell is stored at the screen's own depth -- one byte holds four pixels,
the MSB pair leftmost, which is exactly the CVALn encoding DCELL hands to
port 1. That makes a cell paint OPAQUE: a repaint writes each of its pixels
once and never needs an erase pass first, which is the whole budget on a
machine where a plotted pixel costs ~80us.

The cursor is not an overlay. Compositing it at runtime would need a mask and
a second pass over the cell; instead every base state is composited with the
reticle here, and the runtime picks bank 1 by setting bit 3 of the state. So
hovering a cell still shows whether it has already been shot -- which is what
tells you whether firing there is legal.

Two cell sizes, because the layout scales with player count:
  QUAD     3x2, four boards on screen   (3 or 4 players)
  HEADSUP  4x4, two boards on screen    (2 players)
A 3x2 cell is very nearly square on a 4:3 set, where a Channel F pixel is
about 1.3x taller than it is wide.

Art characters: '.' sea (value 0), 'b' BLUE (1), 'r' RED (2), 'g' GREEN (3).
In the board band's palette 1 those are LTBLUE / BLUE / RED / GREEN.
"""
import sys

VAL = {'.': 0, 'b': 1, 'r': 2, 'g': 3}

# Cell state codes. Bit 3 selects the cursor-composited bank.
STATES = ["CSEA", "CHIT", "CMISS", "CHULL", "CHDMG", "CREVL", "CDASH", "CSPL"]

# At 3x2 a cell has six pixels, so ANY per-cell marking is a sixth of it. A
# lattice dot that reads as fine texture in the 4x4 cells turns the whole
# board into a blue stipple here -- so the small sea is flat, and the board's
# extent comes from a drawn frame instead, which costs nothing per poll
# because chrome is painted once per layout.
QUAD = {
    "CSEA":  ["...",
              "..."],
    "CHIT":  ["rrr",
              "rrr"],
    # a splash, not a bar: three pixels against the hit's six, so the two never
    # read alike at a glance across the room
    "CMISS": [".b.",
              "b.b"],
    "CHULL": ["ggg",
              "ggg"],
    # your own ship, hit: red hull with a green core, so it never reads as an
    # enemy cell you have already shot
    "CHDMG": ["rgr",
              "rrr"],
    "CREVL": ["ggg",
              "g.g"],
    "CDASH": ["...",
              "..."],
    # the frame the shot animation lands on before the result arrives: bigger
    # than the miss it may become, so the landing reads as an event
    "CSPL":  ["bbb",
              "b.b"],
}
QUAD_RET = ["g.g",
            "g.g"]

HEADSUP = {
    "CSEA":  ["b...",
              "....",
              "....",
              "...."],
    "CHIT":  [".rr.",
              "rrrr",
              "rrrr",
              ".rr."],
    # a RING, not a blob: the sea carries a one-pixel lattice dot at this size,
    # and a filled miss was only "the same mark, wider". A ring reads as a
    # splash at a glance and cannot be confused with either the dot or the
    # filled blob a hit paints.
    "CMISS": [".bb.",
              "b..b",
              "b..b",
              ".bb."],
    # solid, so hull cells tile into a continuous ship rather than a row of tiles
    "CHULL": ["gggg",
              "gggg",
              "gggg",
              "gggg"],
    "CHDMG": ["rrrr",
              "rggr",
              "rggr",
              "rrrr"],
    "CREVL": ["gggg",
              "g..g",
              "g..g",
              "gggg"],
    "CDASH": ["....",
              ".bb.",
              "....",
              "...."],
    # the impact frame, before the server says what it was: a solid centre, so
    # the animation reads as a strike that then resolves into a ring or a blob
    "CSPL":  ["....",
              ".bb.",
              ".bb.",
              "...."],
}
HEADSUP_RET = ["g..g",
               "....",
               "....",
               "g..g"]

PIPS = {
    "PIPOK":   ["ggg",
                "ggg",
                "ggg"],
    "PIPSUNK": ["r.r",
                ".r.",
                "r.r"],
}


def pack(row):
    """One art row -> one byte, MSB pair leftmost."""
    v = 0
    for i, ch in enumerate(row):
        if ch not in VAL:
            raise SystemExit("mkart: bad art character %r" % ch)
        v |= VAL[ch] << (6 - 2 * i)
    return v


def composite(base, ret):
    """Reticle pixels win; '.' in the reticle is transparent."""
    return ["".join(r if r != '.' else b for b, r in zip(brow, rrow))
            for brow, rrow in zip(base, ret)]


def emit(out, name, art, ret, w, h):
    for row in art.values():
        if len(row) != h or any(len(r) != w for r in row):
            raise SystemExit("mkart: %s art is not %dx%d" % (name, w, h))
    out.append("; %s -- %dx%d cells, 16 entries of %d bytes:" % (name, w, h, h))
    out.append(";   0-7 plain, 8-15 the same states with the reticle composited.")
    out.append("%s:" % name)
    for bank in (0, 1):
        for st in STATES:
            rows = art[st]
            if bank:
                rows = composite(rows, ret)
            out.append("\tDB %s\t; %s%s"
                       % (",".join("0%02XH" % pack(r) for r in rows),
                          st, " + cursor" if bank else ""))


def main(argv):
    path = argv[1] if len(argv) > 1 else "src/art.inc"
    out = ["; GENERATED by tools/mkart.py -- do not edit by hand.",
           "; 2bpp opaque cell bitmaps: one byte = four pixels, MSB pair leftmost.",
           ""]
    for i, st in enumerate(STATES):
        out.append("%s\tEQU %d" % (st, i))
    out.append("CCURS\tEQU 8\t\t; OR into a state to pick the reticle bank")
    out.append("NCELLS\tEQU 16\t\t; art entries: 8 states x 2 banks")
    out.append("")
    emit(out, "ARTQ", QUAD, QUAD_RET, 3, 2)
    out.append("")
    emit(out, "ARTH", HEADSUP, HEADSUP_RET, 4, 4)
    out.append("")
    out.append("; Ship pips, 3x3, for the roster panel and the footer.")
    for name, rows in PIPS.items():
        out.append("%s:\tDB %s" % (name, ",".join("0%02XH" % pack(r) for r in rows)))
    out.append("")
    with open(path, "w") as f:
        f.write("\n".join(out) + "\n")


main(sys.argv)
