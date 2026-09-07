#!/usr/bin/env python3
"""Generate src/coleco/charset.c and src/coleco/font.bin from the MS-DOS art.

The ColecoVision port renders the MS-DOS port's look on a TMS9918A driven in
GRAPHICS II as a cellmap (identity name table, each screen cell owning its own
8 pattern + 8 color bytes), so the tile sheet converts to per-row
(pattern, fg<<4|bg) pairs and the font to plain 1bpp glyphs colored at draw
time.

Inputs (run from the repo root):
  support/msdos/charset.dat  256 tiles,  16 bytes each: 8 rows x 2 bytes, 2bpp CGA
  support/msdos/ascii.dat    256 glyphs, same format

CGA index -> TMS9918 color, per the game's palette (blue field, cyan/red/white):
  0 background -> 4 dark blue
  1            -> 7 cyan
  2            -> 8 medium red
  3            -> 15 white

A tile row holding more than two colors cannot be represented in mode 2; the
two most frequent survive (ties break white > red > cyan > background) and the
rest join the foreground. Every such compromise is reported so it can be
eyeballed on the charset-viewer test screen.

Outputs:
  src/coleco/charset.c   const unsigned char tiles[256][16], font[96][8]
  src/coleco/font.bin    the same 96x8 1bpp font flat, for the MAME smoke
                         test's screen OCR (support/coleco/smoke.lua)
"""

import sys
from collections import Counter
from pathlib import Path

# ROM is tight, so only the referenced spans of the sheet are emitted: the
# bright chrome/game tiles 0x00-0x60, their +0x80 dim variants 0x82-0xE0, and
# the miss/band/attack-animation run 0xE1-0xE8. The unreferenced 0x61-0x81 and
# 0xE9-0xFF are dropped, and graphics.c's tilePtr() remaps: index >= 0x82
# subtracts 0x21. Keep the two in step.
TILE_KEEP = list(range(0x00, 0x61)) + list(range(0x82, 0xE9))
FONT_FIRST = 0x20
FONT_COUNT = 96

BG_CGA = 0
CGA_TO_TMS = {0: 4, 1: 7, 2: 8, 3: 15}
# Tie-break priority when reducing a row: brighter colors win the foreground.
CGA_PRIORITY = {3: 3, 2: 2, 1: 1, 0: 0}

TMS_BG = CGA_TO_TMS[BG_CGA]
DEFAULT_ATTR = (CGA_TO_TMS[3] << 4) | TMS_BG  # white on dark blue


def decode_row(row):
    """Two CGA bytes -> eight pixel values 0-3, leftmost first."""
    pixels = []
    for byte in row:
        for shift in (6, 4, 2, 0):
            pixels.append((byte >> shift) & 3)
    return pixels


def reduce_row(pixels, tile, rowno, warnings):
    """Eight CGA pixels -> (pattern byte, TMS color byte)."""
    counts = Counter(pixels)
    colors = sorted(counts, key=lambda c: (counts[c], CGA_PRIORITY[c]), reverse=True)

    if len(colors) == 1:
        if colors[0] == BG_CGA:
            return 0x00, DEFAULT_ATTR
        # Solid row of one color: paint it as the background nibble.
        tms = CGA_TO_TMS[colors[0]]
        return 0x00, (tms << 4) | tms

    if len(colors) > 2:
        # Background pixels must stay background (the tile has to keep blending
        # with the blue field), so the row keeps bg + the most frequent accent
        # and every other accent pixel joins the foreground: shape and dominant
        # color survive, detail color is lost.
        if BG_CGA in counts:
            bg_cga = BG_CGA
            accents = [c for c in colors if c != BG_CGA]
            fg_cga = max(accents, key=lambda c: (counts[c], CGA_PRIORITY[c]))
        else:
            fg_cga, bg_cga = colors[0], colors[1]
        warnings.append(
            "tile 0x%02X row %d has colors %s; keeping %s"
            % (tile, rowno, sorted(counts), sorted((fg_cga, bg_cga)))
        )
        pattern = 0
        for i, p in enumerate(pixels):
            if p != bg_cga:
                pattern |= 0x80 >> i
    else:
        if BG_CGA in colors:
            bg_cga = BG_CGA
            fg_cga = colors[0] if colors[0] != BG_CGA else colors[1]
        else:
            fg_cga = max(colors, key=lambda c: (counts[c], CGA_PRIORITY[c]))
            bg_cga = colors[0] if colors[0] != fg_cga else colors[1]
        pattern = 0
        for i, p in enumerate(pixels):
            if p == fg_cga:
                pattern |= 0x80 >> i

    return pattern, (CGA_TO_TMS[fg_cga] << 4) | CGA_TO_TMS[bg_cga]


def convert_tiles(data, warnings):
    tiles = []
    for t in TILE_KEEP:
        raw = data[t * 16 : t * 16 + 16]
        patterns = []
        attrs = []
        for r in range(8):
            pattern, attr = reduce_row(decode_row(raw[r * 2 : r * 2 + 2]), t, r, warnings)
            patterns.append(pattern)
            attrs.append(attr)
        tiles.append(patterns + attrs)
    return tiles


def convert_font(data):
    glyphs = []
    for g in range(FONT_FIRST, FONT_FIRST + FONT_COUNT):
        raw = data[g * 16 : g * 16 + 16]
        rows = []
        for r in range(8):
            pattern = 0
            for i, p in enumerate(decode_row(raw[r * 2 : r * 2 + 2])):
                if p:
                    pattern |= 0x80 >> i
            rows.append(pattern)
        glyphs.append(rows)
    return glyphs


def emit_rows(out, rows, per_line=16):
    for i in range(0, len(rows), per_line):
        out.append("    " + ",".join("0x%02X" % b for b in rows[i : i + per_line]) + ",")


def main():
    root = Path(__file__).resolve().parent.parent.parent
    charset_dat = (root / "support/msdos/charset.dat").read_bytes()
    ascii_dat = (root / "support/msdos/ascii.dat").read_bytes()
    if len(charset_dat) != 4096 or len(ascii_dat) != 4096:
        sys.exit("charset.dat/ascii.dat must be 4096 bytes each")

    warnings = []
    tiles = convert_tiles(charset_dat, warnings)
    font = convert_font(ascii_dat)

    # Flat 1D arrays: sccz80 rejects flattened initializers for 2D arrays.
    # graphics.c indexes as tiles + (icon << 4) and font + (glyph << 3).
    out = [
        "// GENERATED by support/coleco/make_charset.py from support/msdos/{charset,ascii}.dat",
        "// Do not edit: tiles are per-row (8 pattern bytes, then 8 fg<<4|bg color",
        "// bytes) for TMS9918 mode 2; the font is 1bpp and colored at draw time.",
        "// Sheet indices 0x00-0x60 then 0x82-0xE8 - graphics.c's tilePtr() remaps.",
        "#ifdef BUILD_COLECO",
        "",
        "const unsigned char tiles[%d] = {" % (len(TILE_KEEP) * 16),
    ]
    for t, rows in zip(TILE_KEEP, tiles):
        out.append("    // 0x%02X" % t)
        emit_rows(out, rows)
    out.append("};")
    out.append("")
    out.append("const unsigned char font[768] = {")
    for g, rows in enumerate(font):
        ch = chr(FONT_FIRST + g)
        out.append("    // 0x%02X '%s'" % (FONT_FIRST + g, ch if ch != "\\" else "backslash"))
        emit_rows(out, rows, per_line=8)
    out.append("};")
    out.append("")
    out.append("#endif /* BUILD_COLECO */")

    (root / "src/coleco").mkdir(parents=True, exist_ok=True)
    (root / "src/coleco/charset.c").write_text("\n".join(out) + "\n")
    (root / "src/coleco/font.bin").write_bytes(bytes(b for g in font for b in g))

    for w in warnings:
        print("charset: " + w)
    print(
        "charset: wrote src/coleco/charset.c (%d tiles, %d glyphs), %d row(s) reduced"
        % (len(TILE_KEEP), FONT_COUNT, len(warnings))
    )


if __name__ == "__main__":
    main()
