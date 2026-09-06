#!/usr/bin/env bash
# run.sh -- run the client in the patched MAME against a live fujinet-pc.
#
# Needs the MAME tree with the fujinet cart device grafted in
# (fujinet-firmware/pico/arcadia/emu/apply.sh) and a fujinet-pc BoIP
# listener on FUJINET_TCP (default 127.0.0.1:9995).
#
# Env: MAME_DIR, FUJINET_TCP, FUJINET_DEBUG=1 (log every transaction)

set -euo pipefail
cd "$(dirname "$0")"

MAME_DIR="${MAME_DIR:-$HOME/Workspace/mame}"
export FUJINET_TCP="${FUJINET_TCP:-127.0.0.1:9995}"

[ -f build/battleship.bin ] || ./build.sh
exec "$MAME_DIR/mame" arcadia -cartslot fujinet -cart "$PWD/build/battleship.bin" \
    -window -nomax "$@"
