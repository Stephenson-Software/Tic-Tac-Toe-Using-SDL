#!/usr/bin/env bash
# Builds the browser version into web/build/ (index.html, index.js, index.wasm,
# index.data, and the 3D board: Strata's grid relay and web/view3d.js). Needs
# Emscripten's em++ on PATH (source emsdk_env.sh first), and curl to fetch
# Strata's release (or STRATA_WEB, below).
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

# The 3D board: Strata's prebuilt grid relay (its .wasm, glue and page
# scripts) beside the game, and the page's side of it. STRATA_WEB=<dir> uses an
# unpacked strata-<version>-wasm32-emscripten download instead of fetching one.
strata_web=${STRATA_WEB:-}
if [ -z "$strata_web" ]; then
  strata_web=$(tools/fetch-strata.sh web "${TMPDIR:-/tmp}/tic-tac-toe-strata")
fi
relay="$strata_web/kit/grid-relay/dist"
for f in grid-relay-scene.js grid-relay-scene.wasm grid-relay.js strata-kit.js strata-loader.js; do
  cp "$relay/$f" "$out/"
done
cp web/view3d.js "$out/"

echo "Built $out:"
ls -l "$out"
