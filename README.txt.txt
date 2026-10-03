Play in your browser: https://danielstephenson.dev/play/tic-tac-toe-using-sdl

To play the game, you need to draw an X in a box each time it is your turn.

To draw an X in a box, tap the corresponding key:

Q W E
A S D
Z X C

Each of these correspond to one of the nine boxes represented once the game is launched.

A box can also be clicked (or tapped, on a touch screen) instead of pressing its key.

A box that already holds an X or an O ignores its key and clicks. After each X
that does not end the game, the computer waits a second and then draws an O in
a randomly chosen empty box.

Three of the same mark in a row, column or diagonal wins; a full board with no
line is a tie.
About a second after the game ends, the whole window shows the result (you
win, the computer wins, or a tie) and stays that way until the window is
closed. There is no new-game key: to play again, start the game again.

Play in your browser:

The game also runs in a web browser, built with Emscripten
(https://emscripten.org):

    https://tic-tac-toe-sdl.play.danielstephenson.dev
    more games: https://danielstephenson.dev/play

There, boxes are clicked or tapped (the keys work too), and the "New game"
button under the board starts again after a result. The browser version does
not send usage reports.

To build it, install and activate the Emscripten SDK
(https://emscripten.org/docs/getting_started/downloads.html), then run:

    web/build.sh

This writes index.html, index.js, index.wasm and index.data (the six
preloaded PNG files) to web/build/. Serve that directory over HTTP to play it
locally, for example:

    python3 -m http.server --directory web/build 8000

and open http://localhost:8000. The page itself is web/shell.html. The
"Browser build" workflow builds it on every pull request and deploys it to the
address above.

Building from source:

You need a C++ compiler, the SDL2 development package, and the SDL2_image
development package with PNG support.

tictactoe.cpp includes <SDL.h> and <SDL_image.h> rather than <SDL2/SDL.h>, so
the SDL2 include directory has to be on the include path. On Linux and macOS,
pkg-config supplies both that path and the libraries:

    g++ -std=c++17 -pthread tictactoe.cpp -o tictactoe $(pkg-config --cflags --libs sdl2 SDL2_image)

Without pkg-config, the same thing spelled out:

    g++ -std=c++17 -pthread -I/usr/include/SDL2 tictactoe.cpp -o tictactoe -lSDL2 -lSDL2_image

-pthread is there because the game reports usage on a background thread (see
"Usage reporting" below).

Running:

The six PNG files are loaded by relative path, so the game has to be started
from the directory that contains them:

    ./tictactoe

If an image cannot be loaded (for example, when started from another
directory), or SDL cannot open the window, the game prints the reason on
stderr and exits with status 1 instead of showing a blank window.

On Windows, tictactoe.exe can be clicked instead, with the PNG files kept
next to it. That binary is prebuilt and is not rebuilt when tictactoe.cpp
changes, so it may not match the current source.

The window is 600x600.

Usage reporting:

The game reports to trace (https://trace.danielstephenson.dev) by default: one
startup event per launch, carrying the program name (Tic-Tac-Toe-Using-SDL),
its version from version.txt, and a random installation ID. Nothing about you,
your machine or the game is sent (the trace server sees the IP address of the
request, as every web server does).

The first run prints one line saying so on stderr and writes a small settings
file, usage-reporting.conf, to $XDG_CONFIG_HOME/Tic-Tac-Toe-Using-SDL/ (by
default ~/.config/Tic-Tac-Toe-Using-SDL/; ~/Library/Application Support/
Tic-Tac-Toe-Using-SDL/ on macOS, %APPDATA%\Tic-Tac-Toe-Using-SDL\ on Windows).
To turn reporting off:

  - set enabled=false in that file, or
  - set TRACE_USAGE_REPORTING=off or DO_NOT_TRACK=1 in the environment (this
    turns it off for every trace-reporting program, and nothing is printed or
    written).

The installation ID is a random UUID, made the first time reporting is on and
kept in a file named trace-install-id in $XDG_DATA_HOME/tic-tac-toe-using-sdl/
(by default ~/.local/share/tic-tac-toe-using-sdl/; ~/Library/Application
Support/tic-tac-toe-using-sdl/ on macOS, %APPDATA%\tic-tac-toe-using-sdl\ on
Windows). It is not derived from anything about you or your machine; it only
lets trace count installations rather than launches. Delete the file to reset
it. Setting TRACE_INSTALL_ID sends that value instead, and the file is left
alone. Every opt-out above also stops the ID: with reporting off, the file is
never created, read or sent.

The event is sent in the background by the vendored trace-client-cpp header
(trace_client.hpp, https://github.com/Stephenson-Software/trace-client-cpp)
through the system curl; if curl is missing or the machine is offline, nothing
is sent and the game is unaffected. The prebuilt tictactoe.exe predates this
and does not report until it is rebuilt.
TIC_TAC_TOE_USING_SDL_USAGE_REPORTING_ENDPOINT points reporting at another
server, e.g. a local one while testing.
Details: https://github.com/Stephenson-Software/trace#usage-reporting

Note:
I used the SDL tutorial located at the page below as a jumping off point for this.
http://lazyfoo.net/tutorials/SDL/index.php#Hello%20SDL
