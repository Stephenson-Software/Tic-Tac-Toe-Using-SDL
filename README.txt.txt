To play the game, you need to draw an X in a box each time it is your turn.

To draw an X in a box, tap the corresponding key:

Q W E
A S D
Z X C

Each of these correspond to one of the nine boxes represented once the game is launched.

Building from source:

You need a C++ compiler, the SDL2 development package, and the SDL2_image
development package with PNG support.

tictactoe.cpp includes <SDL.h> and <SDL_image.h> rather than <SDL2/SDL.h>, so
the SDL2 include directory has to be on the include path. On Linux and macOS,
pkg-config supplies both that path and the libraries:

    g++ -std=c++17 tictactoe.cpp -o tictactoe $(pkg-config --cflags --libs sdl2 SDL2_image)

Without pkg-config, the same thing spelled out:

    g++ -std=c++17 -I/usr/include/SDL2 tictactoe.cpp -o tictactoe -lSDL2 -lSDL2_image

Running:

The six PNG files are loaded by relative path, so the game has to be started
from the directory that contains them:

    ./tictactoe

On Windows, tictactoe.exe can be clicked instead, with the PNG files kept
next to it. That binary is prebuilt and is not rebuilt when tictactoe.cpp
changes, so it may not match the current source.

The window is 600x600.

Note:
I used the SDL tutorial located at the page below as a jumping off point for this.
http://lazyfoo.net/tutorials/SDL/index.php#Hello%20SDL
