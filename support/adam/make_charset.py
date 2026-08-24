#!/usr/bin/env python3
"""Generate src/adam/charset.c for the Coleco Adam (TMS9918) port.

Sources:
  - support/coco/charset-16.png : the CoCo3 16-color tile sheet, 113 tiles
    (indices 0x00-0x70) whose layout matches src/coco/graphics.c's COCO3
    branch. Game tiles are decoded at their native indices with a
    CoCo3-palette -> TMS9918 color map and a per-row two-color reduction
    (TMS mode 2 allows one fg + one bg per 8x1 row).
  - support/msdos/ascii.dat : CGA 2bpp ASCII font. Used for the text glyphs
    (crisper than the CoCo3 block font); pattern bit = any nonzero pixel.

Output glyph map:
  0x00-0x70  CoCo3 tiles at native indices (text slots substituted from the
             MSDOS font; the same punctuation sacrifices as the CoCo port)
  0x71       horizontal rule (drawLine)
  0x72       badge fill: white top line over black body
  0x73       endgame band: solid yellow
  0xA0-0xDA  copies of 0x20-0x5A colored for drawTextAlt (alt text)

Run from the repo root:  python3 support/adam/make_charset.py
The generated src/adam/charset.c is committed; tweak the OVERRIDES table
here and regenerate rather than hand-editing the output.
"""

import os

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
COCO3_PNG = os.path.join(REPO, "support", "coco", "charset-16.png")
MSDOS_FONT = os.path.join(REPO, "support", "msdos", "ascii.dat")
OUT = os.path.join(REPO, "src", "adam", "charset.c")

# TMS9918 colors
BLACK, MGREEN, LGREEN, DBLUE, LBLUE, DRED, CYAN = 1, 2, 3, 4, 5, 6, 7
MRED, LRED, DYELLOW, LYELLOW, DGREEN, MAGENTA, GRAY, WHITE = 8, 9, 10, 11, 12, 13, 14, 15

# Approximate TMS9918 palette RGB (for nearest-of-two row reduction)
TMS_RGB = {
    BLACK: (0, 0, 0), MGREEN: (33, 200, 66), LGREEN: (94, 220, 120),
    DBLUE: (84, 85, 237), LBLUE: (125, 118, 252), DRED: (170, 60, 60),
    CYAN: (66, 235, 245), MRED: (255, 85, 85), LRED: (255, 121, 120),
    DYELLOW: (212, 193, 84), LYELLOW: (230, 206, 128), DGREEN: (33, 176, 59),
    MAGENTA: (201, 91, 186), GRAY: (204, 204, 204), WHITE: (255, 255, 255),
}

# CoCo3 sheet palette index -> TMS color
# (sheet: 0 blk, 1 dgray, 2 lgray, 3 white, 4 teal, 5 lblue, 6 dblue,
#  7 sea, 8 foam, 9 lfoam, 10 dred, 11 red, 12 rdorange, 13 orange,
#  14 yellow, 15 dgreen)
COCO3_TO_TMS = {
    0: BLACK, 1: BLACK, 2: GRAY, 3: WHITE, 4: CYAN, 5: LBLUE, 6: DBLUE,
    7: DBLUE, 8: LBLUE, 9: CYAN, 10: DRED, 11: MRED, 12: LRED,
    13: DYELLOW, 14: LYELLOW, 15: DGREEN,
}

# Per-tile map overrides {tile: {coco3_index: tms_color}}
OVERRIDES = {
    0x1A: {8: WHITE, 9: WHITE},  # miss splash: white for readability
}

PNG_TILES = 0x71
GLYPH_COUNT = 0x74
ALT_FIRST, ALT_LAST = 0x20, 0x5A

# ASCII indices rendered from the MSDOS font (the rest of the ASCII range is
# CoCo3 tile art - same sacrifices as the CoCo client; 0x25 '%' is the drawer
# bottom-right corner glyph, so it stays tile art too)
TEXT_GLYPHS = set([0x20, 0x21, 0x27, 0x2C, 0x2D, 0x2E, 0x2F])
TEXT_GLYPHS |= set(range(0x30, 0x3A))   # digits
TEXT_GLYPHS |= set(range(0x41, 0x5B))   # A-Z

TEXT_ATTR = (WHITE << 4) | BLACK
ALT_ATTR = (LYELLOW << 4) | BLACK
YELLOW_ATTR = (LYELLOW << 4) | BLACK


def dist2(a, b):
    ar, ag, ab = TMS_RGB[a]
    br, bg, bb = TMS_RGB[b]
    return (ar - br) ** 2 + (ag - bg) ** 2 + (ab - bb) ** 2


def reduce_row(colors):
    """8 TMS colors -> (pattern_bits, attr_byte) with 2-color reduction."""
    counts = {}
    for c in colors:
        counts[c] = counts.get(c, 0) + 1
    ranked = sorted(counts, key=lambda c: (-counts[c], c))
    bg = ranked[0]
    fg = ranked[1] if len(ranked) > 1 else WHITE
    bits = 0
    for i, c in enumerate(colors):
        if c == fg or (c != bg and dist2(c, fg) < dist2(c, bg)):
            bits |= 0x80 >> i
    return bits, (fg << 4) | bg


def decode_png_tile(px, tile, cmap):
    pattern, attr = [], []
    for r in range(8):
        colors = [cmap[px[c, tile * 8 + r]] for c in range(8)]
        bits, a = reduce_row(colors)
        pattern.append(bits)
        attr.append(a)
    return pattern, attr


def decode_msdos_glyph(data, idx):
    """CGA 2bpp 8x8 -> 1bpp pattern (bit = nonzero pixel)."""
    pattern = []
    for r in range(8):
        b0, b1 = data[idx * 16 + r * 2], data[idx * 16 + r * 2 + 1]
        bits = 0
        for p in range(4):
            if (b0 >> (6 - 2 * p)) & 3:
                bits |= 0x80 >> p
            if (b1 >> (6 - 2 * p)) & 3:
                bits |= 0x08 >> p
        pattern.append(bits)
    return pattern


def main():
    from PIL import Image
    im = Image.open(COCO3_PNG)
    px = im.load()
    font = open(MSDOS_FONT, "rb").read()

    patterns = [[0] * 8 for _ in range(GLYPH_COUNT)]
    attrs = [[TEXT_ATTR] * 8 for _ in range(GLYPH_COUNT)]

    for g in range(PNG_TILES):
        if g in TEXT_GLYPHS:
            patterns[g] = decode_msdos_glyph(font, g)
            attrs[g] = [TEXT_ATTR] * 8
        else:
            cmap = dict(COCO3_TO_TMS)
            cmap.update(OVERRIDES.get(g, {}))
            patterns[g], attrs[g] = decode_png_tile(px, g, cmap)

    # 0x71 horizontal rule (drawLine underlines/menu rules)
    patterns[0x71] = [0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00]
    attrs[0x71] = [YELLOW_ATTR] * 8
    # 0x72 badge fill: plain black (matches the text background)
    patterns[0x72] = [0x00] * 8
    attrs[0x72] = [TEXT_ATTR] * 8
    # 0x73 endgame band: solid yellow
    patterns[0x73] = [0xFF] * 8
    attrs[0x73] = [YELLOW_ATTR] * 8

    alt_patterns = [patterns[g] for g in range(ALT_FIRST, ALT_LAST + 1)]
    alt_attrs = [[ALT_ATTR] * 8 for _ in alt_patterns]

    def emit(name, rows):
        flat = [b for glyph in rows for b in glyph]
        lines = []
        for i in range(0, len(flat), 8):
            lines.append("    " + ", ".join("0x%02X" % b for b in flat[i:i + 8]) + ",")
        return "const uint8_t %s[%d] = {\n%s\n};\n" % (name, len(flat), "\n".join(lines))

    with open(OUT, "w") as f:
        f.write("/* GENERATED by support/adam/make_charset.py - do not hand-edit\n"
                "   wholesale; tweak the generator's OVERRIDES and regenerate. */\n\n"
                "#include <stdint.h>\n\n")
        f.write("/* Glyphs 0x00-0x73: patterns + per-row color (fg<<4|bg) */\n")
        f.write(emit("charsetPatterns", patterns))
        f.write("\n")
        f.write(emit("charsetColors", attrs))
        f.write("\n/* Alt-color copies of 0x20-0x5A, loaded at glyph 0xA0 */\n")
        f.write(emit("altPatterns", alt_patterns))
        f.write("\n")
        f.write(emit("altColors", alt_attrs))
    print("wrote", OUT, "(%d + %d glyphs)" % (GLYPH_COUNT, len(alt_patterns)))


if __name__ == "__main__":
    main()
