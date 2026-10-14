# Battleship for the Fairchild Channel F.
ASL       ?= $(HOME)/asl/asl
P2BIN     ?= $(HOME)/asl/p2bin
MAME_DIR  ?= $(HOME)/Workspace/mame
FUJINET_TCP ?= 127.0.0.1:9995

SRCS := $(wildcard src/*.asm src/*.inc) tools/mkfont.py tools/mkart.py

all: build/battleship.bin

build/battleship.bin: $(SRCS) build.sh
	ASL=$(ASL) P2BIN=$(P2BIN) ./build.sh

demo:
	DEMO=1 ASL=$(ASL) P2BIN=$(P2BIN) ./build.sh

run: all
	./run.sh

rundemo: demo
	./run.sh

# Headless: one screenshot of the DEMO position. The art/palette iteration loop.
shot: demo
	FAST=1 SHOT_FRAMES=900 SECS=20 ./run.sh shot

smoke: all
	FUJINET_TCP=$(FUJINET_TCP) SECS=220 ./run.sh smoke

resettest: all
	FUJINET_TCP=$(FUJINET_TCP) SECS=120 ./run.sh resettest

probe: all
	FUJINET_TCP=$(FUJINET_TCP) SECS=120 ./run.sh probe

assets:
	python3 tools/mkfont.py src/font.inc
	python3 tools/mkart.py src/art.inc

clean:
	rm -rf build src/font.inc src/art.inc src/endpoint.inc src/flags.inc

.PHONY: all demo run rundemo shot smoke resettest probe assets clean
