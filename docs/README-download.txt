Tic-Tac-Toe Using SDL
=====================

Draw an X in a box with the keys

    Q W E
    A S D
    Z X C

or by clicking the box. The computer answers with an O. Three in a row wins.
README.md has the full rules.

Starting the game
-----------------

Keep the PNG files next to the game and start it from this folder: they are
loaded by relative path.

  Windows x64: unzip the whole folder first, then double-click tictactoe.exe.
  The SDL2 and SDL2_image DLLs it needs are in this folder. If the game
  closes at once with no message, it could not load its PNG files: check
  that it is being started from this unzipped folder.

  Linux x64:   ./tictactoe
               The game needs the SDL2 and SDL2_image libraries, for example
               on Debian or Ubuntu:
                   sudo apt install libsdl2-2.0-0 libsdl2-image-2.0-0

Usage reporting
---------------

The game sends one startup event per launch to trace
(https://trace.danielstephenson.dev): its name, its version and a random
installation ID. Set TRACE_USAGE_REPORTING=off (or DO_NOT_TRACK=1) to turn it
off. README.md, "Usage reporting", has the details.
