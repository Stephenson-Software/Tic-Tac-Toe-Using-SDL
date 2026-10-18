#!/usr/bin/env bash
# Regenerates docs/images/gameplay.gif by playing one real game of the native build.
# Requires: Xvfb, xdotool, ImageMagick (import/convert), plus the SDL2 build deps.
# Usage (from the repository root): bash docs/capture.sh [display-number]
# The computer's moves are random, so each run records a different game.
set -euo pipefail
N="${1:-74}"
OUT="$PWD/docs/images"
TMP="$(mktemp -d)"
mkdir -p "$OUT"

g++ -std=c++17 -pthread tictactoe.cpp -o tictactoe $(pkg-config --cflags --libs sdl2 SDL2_image)

# no usage report from a capture run
export TRACE_USAGE_REPORTING=off DO_NOT_TRACK=1
# software GL keeps Xvfb from loading a host GPU driver
export LIBGL_ALWAYS_SOFTWARE=1 GALLIUM_DRIVER=llvmpipe
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-$TMP}"
Xvfb ":$N" -screen 0 1280x800x24 >/dev/null 2>&1 &
XPID=$!
trap 'kill $XPID 2>/dev/null; rm -rf "$TMP"' EXIT
export DISPLAY=":$N"
for _ in $(seq 50); do xdotool getdisplaygeometry >/dev/null 2>&1 && break; sleep 0.2; done

./tictactoe &
PID=$!
sleep 1.5
WIN=$(xdotool search --pid "$PID" | tail -1)
i=0
frame() { import -window "$WIN" "$TMP/f_$(printf %02d $i).png"; i=$((i + 1)); }

frame
# centre first, then corners, then edges; keys for occupied boxes are ignored
for key in s q c e z w x a d; do
	xdotool key --window "$WIN" "$key"
	sleep 0.3; frame
	sleep 1.2; frame   # the computer answers about a second after each X
done
sleep 1.5; frame; frame; frame   # the result screen
kill "$PID"; wait "$PID" 2>/dev/null || true

# drop consecutive duplicate frames, then hold the last one
convert -delay 70 -loop 0 "$TMP"/f_*.png -layers RemoveDups -resize 50% -layers Optimize "$OUT/gameplay.gif"
echo "wrote $OUT/gameplay.gif"
