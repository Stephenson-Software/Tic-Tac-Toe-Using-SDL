// Draws the 3D view of some boards headlessly with desktop Strata and saves
// each as a PNG: the start, a game under way, and the three end screens, in a
// 600 x 600 window and a phone held upright (390 x 844).
//
//   g++ -std=c++17 -I/usr/include/SDL2 test/view3dShots.cpp -o view3dShots -lSDL2 -ldl
//   STRATA_LIBRARY=<path to libstrata.so.0.10> ./view3dShots <output directory>
//
// Needs no display: Strata renders offscreen (EGL). Exits 1 when Strata can't
// run here or a picture can't be saved.
#include <cstdio>
#include <string>

#include "../view3d_strata.h"

using view3d::EMPTY;
using view3d::MARK_O;
using view3d::MARK_X;

int main(int argc, char** argv) {
	if (argc < 2) {
		std::fprintf(stderr, "usage: %s <output directory>\n", argv[0]);
		return 2;
	}
	const std::string dir = argv[1];
	struct Board {
		const char* name;
		view3d::Mark cells[9];
		view3d::Result result;
	};
	const Board boards[] = {
		{"start", {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY}, view3d::RESULT_NONE},
		{"midgame", {MARK_O, EMPTY, EMPTY, EMPTY, MARK_X, EMPTY, EMPTY, MARK_O, MARK_X}, view3d::RESULT_NONE},
		{"you-win", {MARK_X, MARK_O, MARK_O, EMPTY, MARK_X, EMPTY, EMPTY, EMPTY, MARK_X}, view3d::RESULT_PLAYER},
		{"you-lose", {MARK_X, MARK_X, EMPTY, MARK_O, MARK_O, MARK_O, MARK_X, EMPTY, EMPTY}, view3d::RESULT_COMPUTER},
		{"tie", {MARK_X, MARK_O, MARK_X, MARK_X, MARK_O, MARK_O, MARK_O, MARK_X, MARK_X}, view3d::RESULT_TIE},
	};
	struct Size {
		const char* name;
		int width, height;
	};
	const Size sizes[] = {{"600x600", 600, 600}, {"phone-390x844", 390, 844}};
	for (const Size& size : sizes) {
		if (!view3d::open(size.width, size.height, nullptr, true)) {
			std::fprintf(stderr, "no 3D view here: %s\n", view3d::reason().c_str());
			return 1;
		}
		for (const Board& board : boards) {
			// the first frame of a fresh engine fits its shadow cascades; draw twice
			view3d::show(board.cells, board.result);
			view3d::show(board.cells, board.result);
			const std::string path = dir + "/tic-tac-toe-sdl-3d-" + board.name + "-" + size.name + ".png";
			if (!view3d::savePng(path.c_str())) {
				std::fprintf(stderr, "could not save %s\n", path.c_str());
				return 1;
			}
			std::printf("%s\n", path.c_str());
		}
		view3d::close();
	}
	return 0;
}
