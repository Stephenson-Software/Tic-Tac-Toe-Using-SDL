#!/usr/bin/env bash
# Builds the browser version into web/build/ (index.html, index.js, index.wasm,
# index.data). Needs Emscripten's em++ on PATH (source emsdk_env.sh first).
# The native build is unaffected.
set -euo pipefail
cd "$(dirname "$0")/.."

out=web/build
rm -rf "$out"
mkdir -p "$out"

# -sASYNCIFY lets the game's own while loops run unchanged: SDL_Delay yields to
# the browser when Asyncify is on. No pthreads, so the page needs no
# cross-origin isolation. The images are preloaded at the root of the virtual
# file system, where the game looks for them by relative path.
images=""
for png in viewport.png o.png x.png playerWin.png computerWin.png tie.png; do
  images="$images --preload-file $png"
done

# shellcheck disable=SC2086
em++ -O2 -std=c++17 -Wall \
  tictactoe.cpp \
  -sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sSDL2_IMAGE_FORMATS='["png"]' \
  -sASYNCIFY -sALLOW_MEMORY_GROWTH \
  $images \
  --shell-file web/shell.html \
  -o "$out/index.html"

echo "Built $out:"
ls -l "$out"
