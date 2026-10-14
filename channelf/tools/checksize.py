#!/usr/bin/env python3
"""checksize.py -- per-module ROM budgets from the AS listing.

The astrocade and arcadia ports' mechanism, retargeted to the Channel F's
single 16K window: battleship.asm brackets every module with an MB_* fence
label, this parses them out of the listing's symbol table, and prints a size
table -- so ROM growth is visible per module long before the hard limit.

The window is $0800-$47FF, and the top four bytes are the "FUJI" claim rather
than code, so the real code ceiling is $47FC.

  checksize.py build/battleship.lst
"""

import re
import sys

ROM_BASE = 0x0800
ROM_TOP = 0x47FC          # the claim sits at $47FC-$47FF
WARN_HEADROOM = 512

# " MB_NET :   1234 C |" -- two columns per listing line, values in hex.
SYM_RE = re.compile(r"\*?(MB_\w+)\s*:\s+([0-9A-F]+)\s+C\b")


def main(argv):
    if len(argv) != 2:
        sys.exit("usage: checksize.py <listing>")
    text = open(argv[1], errors="replace").read()
    fences = {m.group(1): int(m.group(2), 16) for m in SYM_RE.finditer(text)}
    if not fences:
        print("checksize: no MB_* fence labels in the listing", file=sys.stderr)
        return 0

    ordered = sorted(fences.items(), key=lambda kv: kv[1])
    print("  module              start   end   size")
    print("  ------------------------------------")
    # A fence label names the region that ENDS at it, not the one that starts.
    prev_addr = ROM_BASE
    for name, addr in ordered:
        print("  %-18s  %04X  %04X  %5d" % (name[3:].lower(), prev_addr,
                                            addr - 1, addr - prev_addr))
        prev_addr = addr

    used = prev_addr - ROM_BASE
    cap = ROM_TOP - ROM_BASE
    print("  ------------------------------------")
    print("  total               %04X  %04X  %5d of %d (%d free)"
          % (ROM_BASE, prev_addr - 1, used, cap, cap - used))

    if prev_addr > ROM_TOP:
        print("checksize: OVERFLOW -- code runs to $%04X, past the $%04X ceiling"
              % (prev_addr - 1, ROM_TOP - 1), file=sys.stderr)
        return 1
    if cap - used < WARN_HEADROOM:
        print("checksize: only %d bytes of headroom left" % (cap - used),
              file=sys.stderr)
    return 0


sys.exit(main(sys.argv))
