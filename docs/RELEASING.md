# Releasing

A release is a version tag on `master`, `v` and the version in
[`version.txt`](../version.txt) (`v0.1.0`), with a GitHub release on it. The tag
starts [`.github/workflows/release.yml`](../.github/workflows/release.yml), which
builds the game for Linux x64 and Windows x64 and attaches the downloads. The
browser build is separate: it deploys from `master` ([README](../README.md),
"Browser version").

## Steps

1. **Set the version.** On a branch, set `version.txt` and open a pull request.
   The release build builds that version into the game (`-DTIC_TAC_TOE_VERSION`),
   and the workflow refuses a tag that does not match it.
2. **Tag the merged commit**, with the full SHA so the tag lands on exactly the
   commit that was checked:

   ```bash
   git fetch origin
   git tag -a v0.1.0 -m "Tic-Tac-Toe-Using-SDL 0.1.0" "$(git rev-parse origin/master)"
   git push origin v0.1.0
   ```

   A pushed tag with no release gets a pre-release made for it. Creating the
   release first (`gh release create v0.1.0 --target <sha>`) also works.
3. **Wait for the builds.** The release gets four assets:

   | Asset | Contents |
   | --- | --- |
   | `TicTacToe-<version>-linux-x64.tar.gz` | `tictactoe`, the six PNG files, `LICENSE.txt`, `README.md`, `README-download.txt` |
   | `TicTacToe-<version>-win-x64.zip` | `tictactoe.exe`, the SDL2 and SDL2_image DLLs, the same files |
   | `*.sha256` | The SHA-256 of each archive |

   If the workflow fails, fix the cause and re-run it (`gh run rerun <id>`);
   uploads replace assets of the same name.

## What each build checks

The packaged game, started from its own folder with only the download's
libraries, is still running after five seconds, and started from any other
folder it stops at once on the missing images (exit 1). Linux runs it under
Xvfb. The Linux download links SDL2 and SDL2_image from the system; the
Windows one carries them.

The prebuilt `tictactoe.exe` at the top of the repository is older than this
workflow and is not touched by it.

## Checking the packaging without a release

A pull request that changes `release.yml`, `tictactoe.cpp` or `version.txt` runs
the same builds as a dry run and keeps the archives as workflow artifacts. So
does `gh workflow run release.yml --ref <branch>`.
