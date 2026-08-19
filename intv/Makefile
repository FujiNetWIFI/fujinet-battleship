# intv/Makefile -- IntyBASIC build for the Intellivision port.
#
# This is a standalone Makefile rather than a target wired into the root
# Makefile: the root build (mekkogx) drives cc65/cmoc-family C toolchains
# for the other platforms, and IntyBASIC/as1600 is a different toolchain
# entirely -- forcing it through the same machinery would add complexity
# without buying anything.
#
# Override INTYBASIC/AS1600/LIBDIR on the command line if your tools
# aren't on PATH, e.g.:
#   make INTYBASIC=/path/to/intybasic LIBDIR=/path/to/IntyBASIC/intybasic

INTYBASIC ?= intybasic
AS1600     ?= as1600
LIBDIR     ?= $(HOME)/Workspace/IntyBASIC/intybasic/

PRODUCT = battleship
# Every .bas that participates in the build, listed explicitly: relying on
# a wildcard or a partial list means editing one file alone doesn't trigger
# a rebuild (fujinet-5cardstud/intv/Makefile shipped with exactly that bug
# -- its SOURCES omitted sound.bas).
SOURCES = battleship.bas constants.bas input.bas fujinet.bas state.bas board.bas sound.bas

.PHONY: all clean run

all: $(PRODUCT).bin $(PRODUCT).rom

$(PRODUCT).asm: $(SOURCES)
	$(INTYBASIC) battleship.bas $(PRODUCT).asm $(LIBDIR)

# .bin + .cfg -- for loading from SD via the PiRTO II menu.
$(PRODUCT).bin: $(PRODUCT).asm
	$(AS1600) -o $(PRODUCT).bin -l $(PRODUCT).lst $(PRODUCT).asm

# .rom -- Intellicart format, for jzIntv.
$(PRODUCT).rom: $(PRODUCT).asm
	$(AS1600) -o $(PRODUCT).rom $(PRODUCT).asm

run: $(PRODUCT).rom
	jzintv $(PRODUCT).rom

clean:
	rm -f $(PRODUCT).asm $(PRODUCT).bin $(PRODUCT).cfg $(PRODUCT).rom $(PRODUCT).lst
