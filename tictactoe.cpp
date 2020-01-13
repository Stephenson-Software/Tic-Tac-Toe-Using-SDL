// using sDL, SDL_image, iostream, string
#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <string>
#include <time.h>

// using declaration
using namespace std;

// screen constants
const int SCREEN_WIDTH = 600;
const int SCREEN_HEIGHT = 600;

// tracker
int moves;

// starts up SDL and creates window
bool init();

// loads media
bool loadMedia();

// frees media and shuts down SDL
void close();

// loads individual image as texture
SDL_Texture* loadTexture(string path);

// -------------------------------------------------

// the window we'll be rendering to
SDL_Window* window = NULL;

// the window renderer
SDL_Renderer* renderer = NULL;

// possible displayed textures
SDL_Texture* texture = NULL;
SDL_Texture* oTexture = NULL;
SDL_Texture* xTexture = NULL;

// texture locations
SDL_Texture* topLeftT = NULL;
SDL_Texture* topMiddleT = NULL;
SDL_Texture* topRightT = NULL;
SDL_Texture* middleLeftT = NULL;
SDL_Texture* middleMiddleT = NULL;
SDL_Texture* middleRightT = NULL;
SDL_Texture* bottomLeftT = NULL;
SDL_Texture* bottomMiddleT = NULL;
SDL_Texture* bottomRightT = NULL;

// end screens
SDL_Texture* playerWin = NULL;
SDL_Texture* computerWin = NULL;
SDL_Texture* tie = NULL;

// -------------------------------------------------

bool init() {
	// initialize SDL
	SDL_Init(SDL_INIT_VIDEO);
	
	// set texture filtering to linear
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
	
	// create window
	window = SDL_CreateWindow("Viewport", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	
	// create renderer for window
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	
	// initialize renderer color
	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	
	// initialize PNG loading
	int imgFlags = IMG_INIT_PNG;
	IMG_Init(imgFlags);
	
	return true;
}

bool loadMedia() {
	// load texture
	texture = loadTexture("viewport.png");
	oTexture = loadTexture("o.png");
	xTexture = loadTexture("x.png");
	
	topLeftT = texture;
	topMiddleT = texture;
	topRightT = texture;
	middleLeftT = texture;
	middleMiddleT = texture;
	middleRightT = texture;
	bottomLeftT = texture;
	bottomMiddleT = texture;
	bottomRightT = texture;
	
	playerWin = loadTexture("playerWin.png");
	computerWin = loadTexture("computerWin.png");
	tie = loadTexture("tie.png");
	
	return true;
}

void close() {
	// free loaded image
	SDL_DestroyTexture(texture);
	
	// destroy window
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	window = NULL;
	renderer = NULL;
	
	// quit SDL subsystems
	IMG_Quit();
	SDL_Quit();
}

SDL_Texture* loadTexture(string path) {
	// the final texture
	SDL_Texture* newTexture = NULL;
	
	// load image at specified path
	SDL_Surface* loadedSurface = IMG_Load(path.c_str());
	
	// create texture from surface pixels
	newTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
	
	// get rid of old loaded surface
	SDL_FreeSurface(loadedSurface);
	
	return newTexture;
}

string checkForWinner() {
	string winner = "none";
	
	// case top row
	if (topLeftT == xTexture && topMiddleT == xTexture && topRightT == xTexture) {
		winner = "player";
	}
	
	if (topLeftT == oTexture && topMiddleT == oTexture && topRightT == oTexture) {
		winner = "computer";
	}
	
	// case middle row
	if (middleLeftT == xTexture && middleMiddleT == xTexture && middleRightT == xTexture) {
		winner = "player";
	}
	
	if (middleLeftT == oTexture && middleMiddleT == oTexture && middleRightT == oTexture) {
		winner = "computer";
	}
	
	// case bottom row
	if (bottomLeftT == xTexture && bottomMiddleT == xTexture && bottomRightT == xTexture) {
		winner = "player";
	}
	
	if (bottomLeftT == oTexture && bottomMiddleT == oTexture && bottomRightT == oTexture) {
		winner = "computer";
	}
	
	// case left column
	if (topLeftT == xTexture && middleLeftT == xTexture && bottomLeftT == xTexture) {
		winner = "player";
	}
	
	if (topLeftT == oTexture && middleLeftT == oTexture && bottomLeftT == oTexture) {
		winner = "computer";
	}
	
	// case middle column
	if (topMiddleT == xTexture && middleMiddleT == xTexture && bottomMiddleT == xTexture) {
		winner = "player";
	}
	
	if (topMiddleT == oTexture && middleMiddleT == oTexture && bottomMiddleT == oTexture) {
		winner = "computer";
	}
	
	// case right column
	if (topRightT == xTexture && middleRightT == xTexture && bottomRightT == xTexture) {
		winner = "player";
	}
	
	if (topRightT == oTexture && middleRightT == oTexture && bottomRightT == oTexture) {
		winner = "computer";
	}
	
	// case right diagonal
	if (bottomLeftT == xTexture && middleMiddleT == xTexture && topRightT == xTexture) {
		winner = "player";
	}
	
	if (bottomLeftT == oTexture && middleMiddleT == oTexture && topRightT == oTexture) {
		winner = "computer";
	}
	
	// case left diagonal
	if (topLeftT == xTexture && middleMiddleT == xTexture && bottomRightT == xTexture) {
		winner = "player";
	}
	
	if (topLeftT == oTexture && middleMiddleT == oTexture && bottomRightT == oTexture) {
		winner = "computer";
	}
	
	// case tie
	if (moves == 9) {
		winner = "tie"; // hope this works
	}
	
	return winner;
}

void winScreen() {
	// main loop flag
	bool running = true;
	
	// event handler
	SDL_Event e;
	
	// while app is running
	while (running) {
		
		// handle events on queue
		while (SDL_PollEvent(&e) != 0) {
			
			// user requests quit
			if (e.type == SDL_QUIT) {
				close();
			}
			
		}
		
		// clear screen
		SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
		SDL_RenderClear(renderer);

		// full screen viewpoint
		SDL_Rect bottomRight;
		bottomRight.x = 0;
		bottomRight.y = 0;
		bottomRight.w = SCREEN_WIDTH;
		bottomRight.h = SCREEN_HEIGHT;
		SDL_RenderSetViewport(renderer, &bottomRight);

		// render texture to screen
		SDL_RenderCopy(renderer, playerWin, NULL, NULL);
		
		// update screen
		SDL_RenderPresent(renderer);
	}
}

void loseScreen() {
	// main loop flag
	bool running = true;
	
	// event handler
	SDL_Event e;
	
	// while app is running
	while (running) {
		
		// handle events on queue
		while (SDL_PollEvent(&e) != 0) {
			
			// user requests quit
			if (e.type == SDL_QUIT) {
				close();
			}
			
		}
		
		// clear screen
		SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
		SDL_RenderClear(renderer);

		// full screen viewpoint
		SDL_Rect bottomRight;
		bottomRight.x = 0;
		bottomRight.y = 0;
		bottomRight.w = SCREEN_WIDTH;
		bottomRight.h = SCREEN_HEIGHT;
		SDL_RenderSetViewport(renderer, &bottomRight);
		
		// render texture to screen
		SDL_RenderCopy(renderer, computerWin, NULL, NULL);
		
		// update screen
		SDL_RenderPresent(renderer);
	}
}

void tieScreen() {
	// main loop flag
	bool running = true;
	
	// event handler
	SDL_Event e;
	
	// while app is running
	while (running) {
		
		// handle events on queue
		while (SDL_PollEvent(&e) != 0) {
			
			// user requests quit
			if (e.type == SDL_QUIT) {
				close();
			}
			
		}
		
		// clear screen
		SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
		SDL_RenderClear(renderer);

		// full screen viewpoint
		SDL_Rect bottomRight;
		bottomRight.x = 0;
		bottomRight.y = 0;
		bottomRight.w = SCREEN_WIDTH;
		bottomRight.h = SCREEN_HEIGHT;
		SDL_RenderSetViewport(renderer, &bottomRight);

		// render texture to screen
		SDL_RenderCopy(renderer, tie, NULL, NULL);
		
		// update screen
		SDL_RenderPresent(renderer);
	}
}

void renderBoxes() {
	// clear screen
	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	SDL_RenderClear(renderer);
	
	// top left viewport
	SDL_Rect topLeft;
	topLeft.x = 0;
	topLeft.y = 0;
	topLeft.w = SCREEN_WIDTH/3;
	topLeft.h = SCREEN_HEIGHT/3;
	SDL_RenderSetViewport(renderer, &topLeft);
	
	// render texture to screen
	SDL_RenderCopy(renderer, topLeftT, NULL, NULL);
	
	// top middle viewport
	SDL_Rect topMiddle;
	topMiddle.x = SCREEN_WIDTH/3;
	topMiddle.y = 0;
	topMiddle.w = SCREEN_WIDTH/3;
	topMiddle.h = SCREEN_HEIGHT/3;
	SDL_RenderSetViewport(renderer, &topMiddle);
	
	// render texture to screen
	SDL_RenderCopy(renderer, topMiddleT, NULL, NULL);		
	
	// top right viewport
	SDL_Rect topRight;
	topRight.x = SCREEN_WIDTH/3 * 2;
	topRight.y = 0;
	topRight.w = SCREEN_WIDTH/3;
	topRight.h = SCREEN_HEIGHT/3;
	SDL_RenderSetViewport(renderer, &topRight);
	
	// render texture to screen
	SDL_RenderCopy(renderer, topRightT, NULL, NULL);	
	
	// middle left viewport
	SDL_Rect middleLeft;
	middleLeft.x = 0;
	middleLeft.y = SCREEN_WIDTH/3;
	middleLeft.w = SCREEN_WIDTH/3;
	middleLeft.h = SCREEN_HEIGHT/3;
	SDL_RenderSetViewport(renderer, &middleLeft);
	
	// render texture to screen
	SDL_RenderCopy(renderer, middleLeftT, NULL, NULL);
	
	// middle middle viewport
	SDL_Rect middleMiddle;
	middleMiddle.x = SCREEN_WIDTH/3;
	middleMiddle.y = SCREEN_WIDTH/3;
	middleMiddle.w = SCREEN_WIDTH/3;
	middleMiddle.h = SCREEN_HEIGHT/3;
	SDL_RenderSetViewport(renderer, &middleMiddle);
	
	// render texture to screen
	SDL_RenderCopy(renderer, middleMiddleT, NULL, NULL);		
	
	// middle right viewport
	SDL_Rect middleRight;
	middleRight.x = SCREEN_WIDTH/3 * 2;
	middleRight.y = SCREEN_WIDTH/3;
	middleRight.w = SCREEN_WIDTH/3;
	middleRight.h = SCREEN_HEIGHT/3;
	SDL_RenderSetViewport(renderer, &middleRight);
	
	// render texture to screen
	SDL_RenderCopy(renderer, middleRightT, NULL, NULL);
	
	// bottom left viewport
	SDL_Rect bottomLeft;
	bottomLeft.x = 0;
	bottomLeft.y = SCREEN_WIDTH/3 * 2;
	bottomLeft.w = SCREEN_WIDTH/3;
	bottomLeft.h = SCREEN_HEIGHT/3;
	SDL_RenderSetViewport(renderer, &bottomLeft);
	
	// render texture to screen
	SDL_RenderCopy(renderer, bottomLeftT, NULL, NULL);
	
	// bottom middle viewport
	SDL_Rect bottomMiddle;
	bottomMiddle.x = SCREEN_WIDTH/3;
	bottomMiddle.y = SCREEN_WIDTH/3 * 2;
	bottomMiddle.w = SCREEN_WIDTH/3;
	bottomMiddle.h = SCREEN_HEIGHT/3;
	SDL_RenderSetViewport(renderer, &bottomMiddle);
	
	// render texture to screen
	SDL_RenderCopy(renderer, bottomMiddleT, NULL, NULL);		
	
	// bottom right viewport
	SDL_Rect bottomRight;
	bottomRight.x = SCREEN_WIDTH/3 * 2;
	bottomRight.y = SCREEN_WIDTH/3 * 2;
	bottomRight.w = SCREEN_WIDTH/3;
	bottomRight.h = SCREEN_HEIGHT/3;
	SDL_RenderSetViewport(renderer, &bottomRight);
	
	// render texture to screen
	SDL_RenderCopy(renderer, bottomRightT, NULL, NULL);
	
	// update screen
	SDL_RenderPresent(renderer);

	string result = checkForWinner();

	if (result == "player") {
		//Wait 1 seconds
		SDL_Delay(1000);
		
		winScreen();
	}
	if (result == "computer") {
		//Wait 1 seconds
		SDL_Delay(1000);
		
		loseScreen();
	}
	if (result == "tie") {
		//Wait 1 seconds
		SDL_Delay(1000);
		
		tieScreen();
	}

}

void computerTurn() {
	if (moves < 9) {
		
		renderBoxes();
		
		//Wait 1 seconds
		SDL_Delay(1000);
		
		bool turn = true;
		
		while (turn) {
			srand(time(NULL));
				
			int num = rand() % 9 + 1;
			
			switch(num) {
				case 1:
					if (topLeftT == texture) {
						topLeftT = oTexture;
						turn = false;
					}
					break;
				case 2:
					if (topMiddleT == texture) {
						topMiddleT = oTexture;
						turn = false;
					}
					break;
				case 3:
					if (topRightT == texture) {
						topRightT = oTexture;
						turn = false;
					}
					break;
				case 4:
					if (middleLeftT == texture) {
						middleLeftT = oTexture;
						turn = false;
					}
					break;
				case 5:
					if (middleMiddleT == texture) {
						middleMiddleT = oTexture;
						turn = false;
					}
					break;
				case 6:
					if (middleRightT == texture) {
						middleRightT = oTexture;
						turn = false;
					}
					break;
				case 7:
					if (bottomLeftT == texture) {
						bottomLeftT = oTexture;
						turn = false;
					}
					break;
				case 8:
					if (bottomMiddleT == texture) {
						bottomMiddleT = oTexture;
						turn = false;
					}
					break;
				case 9:
					if (bottomRightT == texture) {
						bottomRightT = oTexture;
						turn = false;
					}
					break;
			}
		}
		moves = moves + 1;
	}
}

int main(int argc, char* args[]) {	
	// start up SDL and create window
	init();
	
	// load media
	loadMedia();
	
	// main loop flag
	bool running = true;
	
	// event handler
	SDL_Event e;
	
	// while app is running
	while (running) {
		
		// handle events on queue
		while (SDL_PollEvent(&e) != 0) {
			
			// user requests quit
			if (e.type == SDL_QUIT) {
				running = false;
			}
			else if (e.type == SDL_KEYDOWN) {
				// select surfaces based on key press
				switch(e.key.keysym.sym) {
					case SDLK_q:
					if (topLeftT == texture) {
						topLeftT = xTexture;
						moves = moves + 1;
						computerTurn();
					}
					break;
					case SDLK_w:
					if (topMiddleT == texture) {
						topMiddleT = xTexture;
						moves = moves + 1;
						computerTurn();
					}
					break;
					case SDLK_e:
					if (topRightT == texture) {
						topRightT = xTexture;
						moves = moves + 1;						
						computerTurn();
					}
					break;
					case SDLK_a:
					if (middleLeftT == texture) {
						middleLeftT = xTexture;
						moves = moves + 1;
						computerTurn();
					}
					break;
					case SDLK_s:
					if (middleMiddleT == texture) {
						middleMiddleT = xTexture;
						moves = moves + 1;
						computerTurn();
					}
					break;
					case SDLK_d:
					if (middleRightT == texture) {
						middleRightT = xTexture;
						moves = moves + 1;
						computerTurn();
					}
					break;
					case SDLK_z:
					if (bottomLeftT == texture) {
						bottomLeftT = xTexture;
						moves = moves + 1;
						computerTurn();
					}
					break;
					case SDLK_x:
					if (bottomMiddleT == texture) {
						bottomMiddleT = xTexture;
						moves = moves + 1;
						computerTurn();
					}
					
					break;
					case SDLK_c:
					if (bottomRightT == texture) {
						bottomRightT = xTexture;
						moves = moves + 1;
						computerTurn();
					}
					break;
					default:
					break;
				}
			}
		}
		
		renderBoxes();
	}
	
	// free resources and close SDL
	close();
	
	return 0;
}