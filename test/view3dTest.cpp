// Checks the 3D view's scene, picking and wire messages (view3d.h). Needs no
// SDL, no Strata library and no display.
//
//   g++ -std=c++17 -Wall -Wextra -Werror test/view3dTest.cpp -o view3dTest && ./view3dTest
//
// With Strata's grid relay reader (relay.h, from the release's web kit; see
// tools/fetch-strata.sh) it also feeds every message to the reader the
// browser's relay runs and checks that it draws exactly what the desktop
// draws:
//
//   g++ -std=c++17 -Wall -Wextra -Werror -DVIEW3D_RELAY_H='"<dir>/relay.h"' -Ithird_party
//     test/view3dTest.cpp -o view3dTest && ./view3dTest
#include <cmath>
#include <cstdio>
#include <string>

#include "../view3d.h"

#ifdef VIEW3D_RELAY_H
#include VIEW3D_RELAY_H
#endif

using namespace view3d;

static int failures = 0;
static int checks = 0;

#define CHECK(cond, ...)                                  \
	do {                                                  \
		checks++;                                         \
		if (!(cond)) {                                    \
			failures++;                                   \
			std::printf("FAIL %s:%d: ", __FILE__, __LINE__); \
			std::printf(__VA_ARGS__);                     \
			std::printf("\n");                            \
		}                                                 \
	} while (0)

// The game's own click mapping (tictactoe.cpp, keyForClick): the box under a
// click in the 600 x 600 window, as (column, row).
static bool boxForClick(int x, int y, int& column, int& row) {
	if (x < 0 || y < 0 || x >= LAYOUT_W || y >= LAYOUT_H) {
		return false;
	}
	column = x * 3 / LAYOUT_W;
	row = y * 3 / LAYOUT_H;
	return true;
}

struct Window {
	const char* name;
	float width, height;
};

// A desktop window, a phone held upright and one turned sideways.
static const Window WINDOWS[] = {{"600x600", 600, 600}, {"390x844", 390, 844}, {"844x390", 844, 390}};

static void testPickingRoundTrip() {
	for (const Window& w : WINDOWS) {
		const Camera camera = fitCamera(w.width / w.height);
		for (int row = 0; row < ROWS; row++) {
			for (int column = 0; column < COLUMNS; column++) {
				float x, z, sx, sy;
				cellCenter(column, row, x, z);
				CHECK(projectToScreen(camera, x, BED_TOP, z, w.width, w.height, sx, sy), "%s: box (%d, %d) behind the camera",
						w.name, column, row);
				CHECK(sx > 0 && sx < w.width && sy > 0 && sy < w.height, "%s: box (%d, %d) off screen at %.0f, %.0f", w.name,
						column, row, sx, sy);
				int c = -1, r = -1;
				CHECK(pickCell(camera, sx, sy, w.width, w.height, c, r) && c == column && r == row,
						"%s: a press on box (%d, %d) picked (%d, %d)", w.name, column, row, c, r);
				// near the box's corners (inside the gap between tiles too)
				for (float dx : {-0.45f, 0.45f}) {
					for (float dz : {-0.45f, 0.45f}) {
						projectToScreen(camera, x + dx, BED_TOP, z + dz, w.width, w.height, sx, sy);
						CHECK(pickCell(camera, sx, sy, w.width, w.height, c, r) && c == column && r == row,
								"%s: a press near a corner of box (%d, %d) picked (%d, %d)", w.name, column, row, c, r);
					}
				}
			}
		}
		// the frame, the corners of the window and the sky pick nothing
		int c, r;
		float sx, sy;
		projectToScreen(camera, 1.5f + FRAME_WIDTH / 2, BED_TOP, 0.0f, w.width, w.height, sx, sy);
		CHECK(!pickCell(camera, sx, sy, w.width, w.height, c, r), "%s: a press on the frame picked (%d, %d)", w.name, c, r);
		CHECK(!pickCell(camera, 1, 1, w.width, w.height, c, r), "%s: a press in the top corner picked a box", w.name);
		CHECK(!pickCell(camera, w.width - 2, w.height - 2, w.width, w.height, c, r),
				"%s: a press in the bottom corner picked a box", w.name);
	}
	int c, r;
	CHECK(!pickCell(fitCamera(1.0f), 10, 10, 0, 0, c, r), "an empty window picks nothing");
}

static void testClickPointsReachTheGamesBoxes() {
	for (int row = 0; row < ROWS; row++) {
		for (int column = 0; column < COLUMNS; column++) {
			int x, y, c = -1, r = -1;
			clickPoint(column, row, LAYOUT_W, LAYOUT_H, x, y);
			CHECK(boxForClick(x, y, c, r) && c == column && r == row, "the click for box (%d, %d) at %d, %d is box (%d, %d)",
					column, row, x, y, c, r);
		}
	}
}

static void testBoardAndMarks() {
	const std::vector<Shape> board = boardShapes();
	CHECK(board.size() == 9 + 4, "the board is nine tiles and four frame sides, not %zu shapes", board.size());
	for (int i = 0; i < 9; i++) {
		float x, z;
		cellCenter(i % 3, i / 3, x, z);
		CHECK(board[i].kind == CUBE && board[i].position[0] == x && board[i].position[2] == z,
				"tile %d is a cube on its box", i);
		CHECK(board[i].position[1] + board[i].scale[1] / 2 == BED_TOP, "tile %d's top is the bed", i);
	}
	CHECK(board[0].color[0] != board[1].color[0], "neighbouring tiles are checkered");

	Mark cells[9] = {MARK_X, EMPTY, EMPTY, EMPTY, MARK_O, EMPTY, EMPTY, EMPTY, EMPTY};
	std::vector<Shape> pieces = pieceShapes(cells);
	CHECK(pieces.size() == 18 + 17, "an X (a disc and 17 blocks) and an O (a disc and 16 beads): %zu shapes", pieces.size());
	float x0, z0;
	cellCenter(0, 0, x0, z0);
	CHECK(pieces[0].kind == CYLINDER && pieces[0].position[0] == x0 && pieces[0].position[2] == z0,
			"the X's disc stands on the top left box");
	for (size_t i = 1; i < 18; i++) {
		CHECK(pieces[i].kind == CUBE && std::fabs(pieces[i].position[0] - x0) < 0.3f &&
						std::fabs(pieces[i].position[2] - z0) < 0.3f && pieces[i].color[0] > pieces[i].color[2],
				"block %zu of the X is a red cube on its box", i);
	}
	for (size_t i = 19; i < pieces.size(); i++) {
		const float d = std::hypot(pieces[i].position[0], pieces[i].position[2]);
		CHECK(pieces[i].kind == SPHERE && std::fabs(d - 0.26f) < 1e-4f && pieces[i].color[2] > pieces[i].color[0],
				"bead %zu of the O is a blue sphere on the ring round the middle box", i);
	}
	Mark empty[9] = {};
	CHECK(pieceShapes(empty).empty(), "an empty board has no marks");
	Mark full[9] = {MARK_X, MARK_O, MARK_X, MARK_X, MARK_O, MARK_O, MARK_O, MARK_X, MARK_X};
	CHECK(pieceShapes(full).size() == 5 * 18 + 4 * 17, "a full board has all nine marks");
}

static void testResultScreens() {
	CHECK(resultHud(RESULT_NONE, 600, 600).panels.empty() && resultHud(RESULT_NONE, 600, 600).labels.empty(),
			"a game under way has no end screen");
	const Result results[] = {RESULT_PLAYER, RESULT_COMPUTER, RESULT_TIE};
	const char* words[] = {"You Win!", "You lose!", "It's a tie!"};
	for (int i = 0; i < 3; i++) {
		for (const Window& w : WINDOWS) {
			const Hud hud = resultHud(results[i], w.width, w.height);
			CHECK(hud.labels.size() == 1 && hud.labels[0].text == words[i], "%s: the end screen says \"%s\"", w.name,
					words[i]);
			const Label& l = hud.labels[0];
			CHECK(l.x >= 0 && l.x + textWidth(l.text, l.pixelSize) <= w.width && l.y >= 0 && l.y + l.pixelSize <= w.height,
					"%s: \"%s\" fits in the window", w.name, words[i]);
			CHECK(l.pixelSize >= 16, "%s: \"%s\" is large (%.0f px)", w.name, words[i], l.pixelSize);
			CHECK(hud.panels.size() == 2 && hud.panels[0].width == w.width && hud.panels[0].height == w.height,
					"%s: the whole board is dimmed under the result", w.name);
			// clear of the board: the card ends above the far edge of the frame
			const Camera camera = fitCamera(w.width / w.height);
			float sx, sy;
			projectToScreen(camera, 0.0f, BED_TOP, -1.5f - FRAME_WIDTH, w.width, w.height, sx, sy);
			CHECK(hud.panels[1].y + hud.panels[1].height <= sy + 1,
					"%s: the result card (to y %.0f) stays above the board (from y %.0f)", w.name,
					hud.panels[1].y + hud.panels[1].height, sy);
		}
	}
}

static uint32_t u32At(const std::vector<uint8_t>& m, size_t at) {
	return uint32_t(m[at]) | uint32_t(m[at + 1]) << 8 | uint32_t(m[at + 2]) << 16 | uint32_t(m[at + 3]) << 24;
}

static void testWireLayout() {
	const std::vector<uint8_t> board = encodeBoard();
	CHECK(board.size() == 8 + 8 + 13 * 48, "BOARD is a header, two counts and 13 shapes (%zu bytes)", board.size());
	CHECK(board[0] == 'G' && board[1] == '3' && board[2] == 1 && board[3] == 0 && board[4] == KIND_BOARD,
			"BOARD's header is G3, format 1.0, kind 1");
	CHECK(u32At(board, 8) == 13 && u32At(board, 12) == 0, "BOARD counts 13 shapes and no lights");
	Mark cells[9] = {MARK_X, EMPTY, EMPTY, EMPTY, MARK_O, EMPTY, EMPTY, EMPTY, EMPTY};
	const std::vector<uint8_t> pieces = encodePieces(cells);
	CHECK(pieces.size() == 8 + 16 + 2 * 16 + 35 * 48, "PIECES is two anchors and 35 shapes (%zu bytes)", pieces.size());
	CHECK(u32At(pieces, 12) == 2 && u32At(pieces, 16) == 35, "PIECES counts two anchors and 35 shapes");
	const std::vector<uint8_t> view = encodeView(fitCamera(1.0f));
	CHECK(view.size() == 8 + 64 && view[4] == KIND_VIEW, "VIEW is 64 bytes after its header");
	const std::vector<uint8_t> hud = encodeHud(resultHud(RESULT_TIE, 600, 600));
	CHECK(hud.size() % 4 == 0, "HUD pads its text to four bytes");
	CHECK(hud.size() == 8 + 8 + 2 * 32 + 32 + 12, "HUD is two panels and one label of 11 letters (%zu bytes)", hud.size());
	CHECK(std::string(hud.begin() + 8 + 8 + 64 + 32, hud.begin() + 8 + 8 + 64 + 32 + 11) == "It's a tie!",
			"the label's text is in the HUD");
}

#ifdef VIEW3D_RELAY_H
static bool same(const float* a, const float* b, int n) {
	for (int i = 0; i < n; i++) {
		if (std::fabs(a[i] - b[i]) > 1e-5f) {
			return false;
		}
	}
	return true;
}

// The browser's relay reads every message and draws exactly the shapes,
// panels and text the desktop view submits for the same board.
static void testRelayDrawsWhatTheDesktopDraws() {
	Mark cells[9] = {MARK_O, EMPTY, EMPTY, EMPTY, MARK_X, EMPTY, EMPTY, MARK_O, MARK_X};
	for (const Window& w : WINDOWS) {
		grid_relay::Relay relay;
		const Camera camera = fitCamera(w.width / w.height);
		const Hud hud = resultHud(RESULT_COMPUTER, w.width, w.height);
		const std::vector<uint8_t> messages[] = {encodeBoard(), encodeView(camera), encodePieces(cells), encodeHud(hud)};
		for (const std::vector<uint8_t>& m : messages) {
			const int status = relay.submit(m.data(), m.size());
			CHECK(status == 0, "%s: the relay refused message kind %d with status %d", w.name, m[4], status);
		}
		CHECK(relay.hasBoard() && relay.hasView(), "%s: the relay has a board and a view", w.name);
		CHECK(same(relay.view().eye, camera.eye, 3) && same(relay.view().target, camera.target, 3) &&
						relay.view().fov == camera.fovDegrees,
				"%s: the relay's camera is the desktop's", w.name);
		grid_relay::Frame frame;
		relay.build(0.0, false, 1.0f, frame);
		std::vector<Shape> desktop = boardShapes();
		const std::vector<Shape> marks = pieceShapes(cells);
		desktop.insert(desktop.end(), marks.begin(), marks.end());
		CHECK(frame.items.size() == desktop.size(), "%s: the relay draws %zu shapes, the desktop %zu", w.name,
				frame.items.size(), desktop.size());
		for (size_t i = 0; i < std::min(frame.items.size(), desktop.size()); i++) {
			const StrataRenderItem& it = frame.items[i];
			const Shape& s = desktop[i];
			CHECK(it.kind == s.kind && same(it.position, s.position, 3) && same(it.scale, s.scale, 3) &&
							same(it.color, s.color, 4),
					"%s: shape %zu differs between the relay and the desktop", w.name, i);
		}
		CHECK(frame.lights.empty(), "%s: no point lights", w.name);
		CHECK(frame.rects.size() == hud.panels.size() && frame.texts.size() == hud.labels.size(),
				"%s: the relay draws the end screen's panels and text", w.name);
		if (frame.texts.size() == 1) {
			CHECK(std::string(frame.texts[0].utf8) == "You lose!" && frame.texts[0].x == hud.labels[0].x &&
							frame.texts[0].y == hud.labels[0].y && frame.texts[0].pixel_size == hud.labels[0].pixelSize,
					"%s: the relay's text is the desktop's", w.name);
		}
		// a later board replaces the last one's marks
		Mark empty[9] = {};
		const std::vector<uint8_t> none = encodePieces(empty);
		CHECK(relay.submit(none.data(), none.size()) == 0, "%s: an empty board's PIECES is read", w.name);
		relay.build(1.0, false, 1.0f, frame);
		CHECK(frame.items.size() == 13, "%s: with no marks the relay draws only the board", w.name);
	}
}
#endif

int main() {
	testPickingRoundTrip();
	testClickPointsReachTheGamesBoxes();
	testBoardAndMarks();
	testResultScreens();
	testWireLayout();
#ifdef VIEW3D_RELAY_H
	testRelayDrawsWhatTheDesktopDraws();
	std::printf("(with the grid relay's reader)\n");
#else
	std::printf("(without the grid relay's reader: define VIEW3D_RELAY_H to check the messages against it)\n");
#endif
	std::printf("%d of %d checks passed\n", checks - failures, checks);
	return failures ? 1 : 0;
}
