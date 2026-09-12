#!/usr/bin/env bash
# run.sh -- run the Battleship client in a FujiNet-patched MAME.
#
#   ./run.sh [lua-script]
#
# With a script it runs headless and exits; without one it opens a window.
#
# The MAME tree must have had fujinet-firmware/pico/atari-2600/emu/apply.sh
# run against it, and a fujinet-pc must be listening -- the cartridge model
# talks to a real one over BoIP.
#
# Two environment facts this wraps, both of which cost time to rediscover:
#   - MAME must run FROM ITS OWN TREE or -autoboot_script is silently ignored.
#   - SDL_VIDEODRIVER=dummy is required wherever there is no DISPLAY, because
#     SDL is brought up before the video backend is chosen.
#
# fujinet-pc's BoIP listener takes ONE client, so a MAME left running starves
# the next run and the symptom is a hang, not an error. Kill any stray first.

set -euo pipefail
cd "$(dirname "$0")"
HERE=$(pwd)

SCRIPT=${1:-}
MAME=${MAME:-$HOME/Workspace/mame}

pkill -f "mame a2600" 2>/dev/null || true

args=(a2600 -cartslot fujinet -cart "$HERE/build/battleship.bin")
export A2600_EMU="$HERE/emu"

if [ -n "$SCRIPT" ]; then
    args+=(-autoboot_script "$HERE/emu/$SCRIPT.lua"
           -video none -sound none -nothrottle
           -seconds_to_run "${SECS:-60}")
fi

[ -n "${DISPLAY:-}" ] || export SDL_VIDEODRIVER=dummy

cd "$MAME"
exec ./mame "${args[@]}"
