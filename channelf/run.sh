#!/usr/bin/env bash
# run.sh -- run the client in the patched MAME against a live fujinet-pc.
#
# Needs the MAME tree with the Channel F FujiNet cart device grafted in
# (fujinet-firmware/pico/channelf/emu/apply.sh) and a fujinet-pc BoIP listener
# on FUJINET_TCP (default 127.0.0.1:9995).
#
#   ./run.sh                 interactive window
#   ./run.sh smoke           headless, driven by emu/smoke.lua
#
# MAME must run FROM ITS OWN TREE or -autoboot_script is silently ignored, so
# every path below is absolute.
set -euo pipefail
cd "$(dirname "$0")"
HERE="$PWD"

MAME_DIR="${MAME_DIR:-$HOME/Workspace/mame}"
export FUJINET_TCP="${FUJINET_TCP:-127.0.0.1:9995}"
SCRIPT="${1:-}"

[ -f build/battleship.bin ] || ./build.sh

args=(channelf -bios sl31253 -cartslot fujinet -cart "$HERE/build/battleship.bin"
      -snapshot_directory "$HERE/build/snap" -cfg_directory "$HERE/cfg")

if [ -n "$SCRIPT" ]; then
    shift
    # MAME still opens SDL even with -video none, so a headless run needs the
    # dummy drivers or it dies with "No available video device".
    export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
    args+=(-autoboot_script "$HERE/emu/$SCRIPT.lua"
           -video none -sound none -seconds_to_run "${SECS:-90}")
    # THROTTLED BY DEFAULT, and that is not a performance choice: the server's
    # start countdown and its move clock are WALL CLOCK, so a run at 250x
    # emulated speed sits at "starting in 2" forever because barely any real
    # time passes. FAST=1 is for the DEMO screenshot, which talks to nobody.
    [ -n "${FAST:-}" ] && args+=(-nothrottle)
else
    args+=(-window -nomax)
fi

cd "$MAME_DIR"
exec ./mame "${args[@]}" "$@"
