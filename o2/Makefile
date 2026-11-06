# Makefile -- Battleship for the Odyssey 2: build/battleship.bin (build.sh).
# Standalone, like studio2/ and atari2600/: not part of the root mekkogx build.
#
#   make                  the image, against the live server
#   make ENDPOINT=http://host:port/
#   make spike            the board engine's test pattern (build/spike)
#   make run              play it in a MAME window (run.sh)
#   make shot             the spike in MAME, headless: build/spike/run/*.png
#   make rig              build for the private server (test/rig.sh)
#   make test             the live scenarios against the private rig
#   make soak             an AUTOPLAY game against AI-1, to game over and back

# Every target rebuilds: ENDPOINT and the flags are not files make can see.
ENDPOINT ?= https://battleship.carr-designs.com/
RIG_ENDPOINT = http://localhost:8085/

all:
	ENDPOINT='$(ENDPOINT)' ./build.sh

spike:
	SPIKE=1 OUT=build/spike ./build.sh

shot: spike
	O2SIM_SECONDS=30 python3 tools/o2sim.py test/scen/spike.scen build/spike/run build/spike/battleship.bin
	python3 tools/rowcheck.py build/spike/run rows

run: all
	./run.sh

rig:
	ENDPOINT='$(RIG_ENDPOINT)' ./build.sh

test: rig
	test/rig.sh test/scen/boot.scen build/run/boot 40
	test/rig.sh test/scen/lobby.scen build/run/lobby 60
	test/rig.sh test/scen/place.scen build/run/place 90
	test/rig.sh test/scen/battle.scen build/run/battle 120

soak:
	AUTOPLAY=1 OUT=build/auto ENDPOINT='$(RIG_ENDPOINT)' ./build.sh
	test/rig.sh test/scen/autoplay.scen build/run/auto 900 build/auto/battleship.bin

clean:
	rm -rf build

.PHONY: all spike shot run rig test soak clean
