#!/usr/bin/env python3
"""Generate the Sega Master System tile set for the Battleship client.

The art is the CoCo 3 client's 16-colour sheet, support/coco/charset-16.image
(grit's packed 4bpp export of charset-16.png: 113 tiles, low nibble = left
pixel). Mode 4 tiles are 4bpp too, and the CoCo 3's 6-bit RGB maps exactly
onto the Master System's 6-bit CRAM, so the sheet comes across tile for tile
with no colour lost. Tiles keep their CoCo 3 indices, so src/sms/graphics.c
uses the same vocabulary as src/adam/graphics.c (the cell-aligned layout of
the CoCo 3 art).

Text does not: the sheet's own letters are blocky and take the ASCII slots
some of the art lives in, so text comes from the MS-DOS font
(support/msdos/ascii.dat) in a separate tile range, and the sheet's letter
and digit slots are left out of the ROM.

Outputs, all regenerated on every build (Makefile, sms/r2r):
  src/sms/tileset.c       palettes, the art as raw 4bpp runs, 1bpp font
  src/sms/tiles.h         tile numbers
  support/sms/tilemap.lua tile -> character, for support/sms/smoke.lua

VRAM tile map (9-bit tile numbers; the name table at $3800 caps it at 448):
  0x000-0x070  CoCo 3 art at its own index (minus the text slots)
  0x071-0x078  tiles made here: rule, badge fills, bullets, merged edges
  0x080-0x0BF  name-plate font, strip at the bottom (lower boards), $20-$5F
  0x0C0-0x0FF  name-plate font, strip at the top (upper boards), $20-$5F
  0x100-0x15F  text font, $20-$7F

The two fonts are 1bpp here and expanded to 4bpp by initGraphics(), in the
colours tiles.h names. Palette 1 is palette 0 with the chrome colours
dimmed the way the CoCo 3's ROP_INACTIVE mask dims them (index & 7), and
with the text colour, index 15, turned to foam, the CoCo 3's ROP_ALT.

Run from the repo root:  python3 src/sms/mktiles.py
"""

import os

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SHEET = os.path.join(REPO, "support", "coco", "charset-16.image")
FONT = os.path.join(REPO, "support", "msdos", "ascii.dat")
OUT_C = os.path.join(REPO, "src", "sms", "tileset.c")
OUT_H = os.path.join(REPO, "src", "sms", "tiles.h")
OUT_LUA = os.path.join(REPO, "support", "sms", "tilemap.lua")

# CoCo 3 palette, RGB monitor (src/coco/graphics.c palette[]), as GIME bytes:
# bit 5 R1, 4 G1, 3 B1, 2 R0, 1 G0, 0 B0. The sheet's index 15 is a dark green
# placeholder only the letters use; here it is the text colour, white.
COCO3_RGB = [0, 7, 56, 63, 28, 11, 1, 9, 25, 27, 4, 36, 38, 52, 54, 63]

BLACK, WHITE, TEAL, FOAM, REDORANGE, TEXT = 0, 3, 4, 8, 12, 15


def gime_to_cram(b):
    r = ((b >> 5) & 1) << 1 | ((b >> 2) & 1)
    g = ((b >> 4) & 1) << 1 | ((b >> 1) & 1)
    bl = ((b >> 3) & 1) << 1 | (b & 1)
    return r | g << 2 | bl << 4


PAL0 = [gime_to_cram(b) for b in COCO3_RGB]
# Dimmed: the chrome's warm colours drop to their index & 7 counterparts
# (red-orange to teal, and so on); 0-7 are unchanged; text turns foam.
PAL1 = PAL0[:8] + [PAL0[i & 7] for i in range(8, 15)] + [PAL0[FOAM]]

# The sheet's text slots - letters, digits and the punctuation drawn in the
# letter style. Text never comes from them on the SMS, so they stay out of the
# ROM. 0x62 (the solid teal "inactive" cell) and the corner-pixel tiles
# 0x69/0x6A are subsumed by palette 1 and the badge fills below.
SKIP = set([0x21, 0x27, 0x2C, 0x2D, 0x2E, 0x2F, 0x62, 0x69, 0x6A])
SKIP |= set(range(0x30, 0x3A))
SKIP |= set(range(0x41, 0x5B))

SHEET_TILES = 0x71

# Tiles made here, from 0x71
T_RULE = 0x71
T_BADGE_BOT = 0x72
T_BADGE_TOP = 0x73
T_BULLET_BOT = 0x74
T_BULLET_TOP = 0x75
T_JOIN_LEFT = 0x76
T_JOIN = 0x77
T_JOIN_RIGHT = 0x78
MADE_END = 0x79

NAME_BOT_BASE = 0x080  # tile = base + c - 0x20, c in 0x20-0x5F
NAME_TOP_BASE = 0x0C0
TEXT_BASE = 0x100      # tile = base + c - 0x20, c in 0x20-0x7F
TILE_LIMIT = 448


def read_sheet():
    data = open(SHEET, "rb").read()
    tiles = []
    for t in range(SHEET_TILES):
        px = []
        for r in range(8):
            row = []
            for c in range(8):
                b = data[t * 32 + r * 4 + c // 2]
                row.append((b >> (4 * (c & 1))) & 15)
            px.append(row)
        tiles.append(px)
    return tiles


def solid_rows(spec):
    """spec: list of 8 colour indices, one per row."""
    return [[c] * 8 for c in spec]


def merge_join(upper, lower):
    """Row 11 is shared by an upper board's lower edge and a lower board's
    upper edge (the CoCo 3 has a 25th row and draws them a row apart). Each
    edge hugs its own field, so the joint keeps four rows of each: the upper
    tile's sea, band, outline and white line, and the lower tile's mirror."""
    return [upper[r] for r in (0, 1, 3, 4)] + [lower[r] for r in (3, 4, 6, 7)]


def to_planar(px):
    out = []
    for row in px:
        for plane in range(4):
            b = 0
            for c in range(8):
                if (row[c] >> plane) & 1:
                    b |= 0x80 >> c
            out.append(b)
    return out


def read_font():
    """CGA 2bpp ASCII -> 1bpp rows (a bit for every non-zero pixel)."""
    data = open(FONT, "rb").read()
    glyphs = []
    for ch in range(0x20, 0x80):
        rows = []
        for r in range(8):
            b0, b1 = data[ch * 16 + r * 2], data[ch * 16 + r * 2 + 1]
            bits = 0
            for p in range(4):
                if (b0 >> (6 - 2 * p)) & 3:
                    bits |= 0x80 >> p
                if (b1 >> (6 - 2 * p)) & 3:
                    bits |= 0x08 >> p
            rows.append(bits)
        if rows[7]:
            raise SystemExit("mktiles: glyph %r uses row 7, which the name plates need" % chr(ch))
        glyphs.append(rows)
    return glyphs


def main():
    sheet = read_sheet()
    tiles = {}
    for t in range(SHEET_TILES):
        if t not in SKIP:
            tiles[t] = sheet[t]

    tiles[T_RULE] = solid_rows([0, REDORANGE, REDORANGE, 0, 0, 0, 0, 0])
    # Name badge body: the corner tiles 0x5C-0x5F carry a white outline on the
    # outer side, so the body between them carries it too - along the bottom
    # under a lower board, along the top over an upper one.
    tiles[T_BADGE_BOT] = solid_rows([REDORANGE] * 7 + [WHITE])
    tiles[T_BADGE_TOP] = solid_rows([WHITE] + [REDORANGE] * 7)
    bullet = sheet[0x5B]
    tiles[T_BULLET_BOT] = bullet[:7] + [[WHITE] * 8]
    tiles[T_BULLET_TOP] = [[WHITE] * 8] + bullet[1:]
    tiles[T_JOIN_LEFT] = merge_join(sheet[0x0C], sheet[0x0F])
    tiles[T_JOIN] = merge_join(sheet[0x0D], sheet[0x10])
    tiles[T_JOIN_RIGHT] = merge_join(sheet[0x0E], sheet[0x11])

    # Runs of consecutive tiles: { first lo, first hi, count, count*32 bytes }
    runs = []
    for t in sorted(tiles):
        if runs and runs[-1][0] + len(runs[-1][1]) == t and len(runs[-1][1]) < 255:
            runs[-1][1].append(t)
        else:
            runs.append([t, [t]])

    font = read_font()
    if TEXT_BASE + len(font) > TILE_LIMIT:
        raise SystemExit("mktiles: text font overruns the name table")
    if MADE_END > NAME_BOT_BASE:
        raise SystemExit("mktiles: art overruns the name-plate font")

    def hexlines(data, indent="    "):
        lines = []
        for i in range(0, len(data), 16):
            lines.append(indent + ", ".join("0x%02X" % b for b in data[i:i + 16]) + ",")
        return "\n".join(lines)

    blob = []
    for first, ts in runs:
        blob += [first & 0xFF, first >> 8, len(ts)]
        for t in ts:
            blob += to_planar(tiles[t])
    blob += [0, 0, 0]

    fontbytes = [b for g in font for b in g]

    with open(OUT_C, "w") as f:
        f.write("/* GENERATED by src/sms/mktiles.py from the CoCo 3 sheet and the MS-DOS\n"
                "   font - edit the generator, not this file. */\n\n"
                "#ifdef BUILD_SMS\n\n"
                "#include \"tiles.h\"\n\n")
        f.write("/* CRAM: palette 0, then palette 1 (dimmed chrome, foam text). */\n")
        f.write("const unsigned char sms_palette[32] = {\n%s\n};\n\n" % hexlines(PAL0 + PAL1))
        f.write("/* %d art tiles in %d runs of { first lo, first hi, count, 4bpp... },\n"
                "   ended by a zero count. */\n" % (len(tiles), len(runs)))
        f.write("const unsigned char sms_tiles[%d] = {\n%s\n};\n\n" % (len(blob), hexlines(blob)))
        f.write("/* ASCII $20-$7F, 1bpp, row 7 always clear. */\n")
        f.write("const unsigned char sms_font[%d] = {\n%s\n};\n\n" % (len(fontbytes), hexlines(fontbytes)))
        f.write("#endif /* BUILD_SMS */\n")

    with open(OUT_H, "w") as f:
        f.write("""/* GENERATED by src/sms/mktiles.py - edit the generator, not this file. */

#ifndef SMS_TILES_H
#define SMS_TILES_H

/* CoCo 3 sheet indices used by name (the rest are named in graphics.c) */
#define T_BLANK         0x20
#define T_SEA           0x18
#define T_HIT           0x19
#define T_MISS          0x1A
#define T_HIT2          0x1B
#define T_HIT_LEGEND    0x1C
#define T_CLOCK         0x1D
#define T_CONN_1        0x1E
#define T_CONN_2        0x1F
#define T_ATTACK_ANIM   0x63    /* 6 frames */
#define T_WATER         0x6B    /* 6 sparkle variants */

/* Made by the generator */
#define T_RULE          0x%02X
#define T_BADGE_BOT     0x%02X
#define T_BADGE_TOP     0x%02X
#define T_BULLET_BOT    0x%02X
#define T_BULLET_TOP    0x%02X
#define T_JOIN_LEFT     0x%02X
#define T_JOIN          0x%02X
#define T_JOIN_RIGHT    0x%02X

/* Fonts: tile = base + character - 0x20 */
#define NAME_BOT_BASE   0x%03X  /* $20-$5F, white on red-orange, strip below */
#define NAME_TOP_BASE   0x%03X  /* $20-$5F, white on red-orange, strip above */
#define TEXT_BASE       0x%03X  /* $20-$7F, colour 15 on black */
#define NAME_GLYPHS     64
#define TEXT_GLYPHS     96

/* Colour indices the fonts are expanded in */
#define C_BLACK         %d
#define C_WHITE         %d
#define C_BADGE         %d
#define C_TEXT          %d

extern const unsigned char sms_palette[32];
extern const unsigned char sms_tiles[];
extern const unsigned char sms_font[];

#endif /* SMS_TILES_H */
""" % (T_RULE, T_BADGE_BOT, T_BADGE_TOP, T_BULLET_BOT, T_BULLET_TOP, T_JOIN_LEFT,
       T_JOIN, T_JOIN_RIGHT, NAME_BOT_BASE, NAME_TOP_BASE, TEXT_BASE,
       BLACK, WHITE, REDORANGE, TEXT))

    names = {}
    for t in tiles:
        names[t] = "#"
    for t in (0x00, 0x20):
        names[t] = " "
    for t in [0x18] + list(range(0x6B, 0x71)):
        names[t] = "~"
    for t in range(0x12, 0x18):
        names[t] = "="
    names[0x19] = names[0x1B] = "*"
    names[0x1A] = "o"
    names[0x1C] = "x"
    names[0x2A] = "@"
    names[0x2B] = ">"
    names[0x3A] = "_"
    for t in (T_BADGE_BOT, T_BADGE_TOP):
        names[t] = " "
    for t in (T_BULLET_BOT, T_BULLET_TOP, 0x5B):
        names[t] = ">"
    for c in range(0x20, 0x60):
        names[NAME_BOT_BASE + c - 0x20] = chr(c)
        names[NAME_TOP_BASE + c - 0x20] = chr(c)
    for c in range(0x20, 0x80):
        names[TEXT_BASE + c - 0x20] = chr(c) if c < 0x7F else " "

    def lua_str(s):
        return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'

    os.makedirs(os.path.dirname(OUT_LUA), exist_ok=True)
    with open(OUT_LUA, "w") as f:
        f.write("-- GENERATED by src/sms/mktiles.py: what each tile reads as. Text reads as\n"
                "-- itself; art: \"~\" sea, \"=\" ship, \"*\" hit, \"o\" miss, \"x\" sunk,\n"
                "-- \"@\" player, \">\" marker, \"_\" text cursor, \"#\" board chrome.\n"
                "return {\n")
        for t in sorted(names):
            f.write("  [%d] = %s,\n" % (t, lua_str(names[t])))
        f.write("}\n")

    print("mktiles: %d art tiles in %d runs (%d bytes), font %d bytes" %
          (len(tiles), len(runs), len(blob), len(fontbytes)))


if __name__ == "__main__":
    main()
