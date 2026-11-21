// view3d.h: what the 3D view of the board shows, as plain data.
//
// The game keeps its rules, its keys and its turns; this header only says how
// a board looks in 3D and which box a click lands on:
//
//   - the board is a 3 x 3 grid of tiles in a dark frame, and every claimed
//     box has its mark standing on it: an X of red blocks or an O ring of blue
//     beads, each on a low disc (the look of graphik's grid3d tic-tac-toe);
//   - the end screen ("You Win!", "You lose!", "It's a tie!") is screen text
//     on a panel over the dimmed board;
//   - a click or tap is turned into the box under it by casting a ray from
//     the camera through the pointer onto the tiles (pickCell), and then into
//     a click at the middle of that box in the 2D window (clickPoint), so the
//     game's own click handling (keyForClick) decides what it means.
//
// The same scene is drawn by desktop Strata (view3d_strata.h) and, in a
// browser, by Strata's grid relay, which reads the binary messages encoded at
// the bottom of this file (Strata's web/kit/grid-relay/WIRE.md, format 1.0).
//
// Pure C++17: no SDL, no Strata, no GPU, so test/view3dTest.cpp checks it
// natively.
#ifndef VIEW3D_H
#define VIEW3D_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace view3d {

// ------------------------------------------------------------------ the game

enum Mark { EMPTY = 0, MARK_X = 1, MARK_O = 2 };
enum Result { RESULT_NONE = 0, RESULT_PLAYER = 1, RESULT_COMPUTER = 2, RESULT_TIE = 3 };

const int COLUMNS = 3;
const int ROWS = 3;

// The 2D window's size (tictactoe.cpp's SCREEN_WIDTH and SCREEN_HEIGHT): a
// click in 3D is handed to the game in its coordinates.
const int LAYOUT_W = 600;
const int LAYOUT_H = 600;

// What the end screen says: the words of playerWin.png, computerWin.png and
// tie.png.
inline const char* resultText(Result result) {
	switch (result) {
		case RESULT_PLAYER: return "You Win!";
		case RESULT_COMPUTER: return "You lose!";
		case RESULT_TIE: return "It's a tie!";
		default: return "";
	}
}

// ------------------------------------------------------------- scene records

// Strata's primitive kinds (strata.h, STRATA_PRIMITIVE_*).
const int CUBE = 0;
const int CYLINDER = 2;
const int SPHERE = 3;

struct Shape {
	int kind;
	float position[3];
	float scale[3];
	float color[4];
};

// A screen rectangle and a run of screen text, in pixels from the top-left.
struct Panel {
	float x, y, width, height;
	float color[4];
};

struct Label {
	float x, y, pixelSize;
	float color[4];
	std::string text;
};

struct Camera {
	float eye[3];
	float target[3];
	float fovDegrees;
};

struct Hud {
	std::vector<Panel> panels;
	std::vector<Label> labels;
};

// graphik grid3d's lighting and projection, so the desktop and the relay draw
// the same picture.
const float CLEAR_COLOR[4] = {0.10f, 0.12f, 0.16f, 1.0f};
const float SUN_DIRECTION[3] = {-0.45f, 0.85f, 0.35f};
const float NEAR_PLANE = 0.3f;
const float FAR_PLANE = 200.0f;
const float FOV_DEGREES = 42.0f;
const float SHADOW_DISTANCE = 40.0f;

// --------------------------------------------------------------------- theme

const float BED_TOP = 0.0f;  // y of the tiles' top face; marks stand on it
const float TILE_HEIGHT = 0.18f;
const float TILE_GAP = 0.12f;
const float FRAME_WIDTH = 0.3f;
const float TILE_LIGHT[3] = {0.86f, 0.85f, 0.80f};
const float TILE_DARK[3] = {0.78f, 0.77f, 0.72f};
const float FRAME_COLOR[3] = {0.10f, 0.10f, 0.12f};
const float X_COLOR[3] = {0.86f, 0.22f, 0.18f};
const float O_COLOR[3] = {0.20f, 0.46f, 0.88f};
const float DISC_SHADE = 0.45f;
const float MARK_HEIGHT = 0.16f;
const float MARK_BASE_HEIGHT = 0.04f;
const float DISC_SIZE = 0.78f;
const float TEXT_COLOR[4] = {0.97f, 0.96f, 0.92f, 1.0f};
const float OVERLAY_COLOR[4] = {0.04f, 0.05f, 0.07f, 0.55f};
const float RESULT_PANEL_COLOR[4] = {0.10f, 0.12f, 0.16f, 0.92f};

inline Shape shape(int kind, float x, float y, float z, float sx, float sy, float sz, const float rgb[3],
		float shadeBy = 1.0f) {
	Shape s;
	s.kind = kind;
	s.position[0] = x;
	s.position[1] = y;
	s.position[2] = z;
	s.scale[0] = sx;
	s.scale[1] = sy;
	s.scale[2] = sz;
	for (int i = 0; i < 3; i++) {
		s.color[i] = std::max(0.0f, std::min(1.0f, rgb[i] * shadeBy));
	}
	s.color[3] = 1.0f;
	return s;
}

// --------------------------------------------------------------------- board

// World (x, z) of the middle of box (column, row). Row 0 is the top row of the
// 2D window, the far edge of the 3D board.
inline void cellCenter(int column, int row, float& x, float& z) {
	x = column - (COLUMNS - 1) / 2.0f;
	z = row - (ROWS - 1) / 2.0f;
}

// graphik grid3d's fitCamera: south of the board, looking down about 55
// degrees, far enough back that the board and its frame fit; a tall window
// (a phone held upright) backs it off further.
inline Camera fitCamera(float aspect) {
	float size = std::max(COLUMNS, ROWS) + 1.0f;
	size *= std::max(1.0f, 1.0f / std::max(aspect, 0.5f));
	const float distance = 0.86f * size + 0.9f;
	Camera c;
	c.eye[0] = 0.0f;
	c.eye[1] = 1.35f * distance;
	c.eye[2] = 0.95f * distance;
	c.target[0] = 0.0f;
	c.target[1] = -0.3f;
	c.target[2] = -0.04f * size;
	c.fovDegrees = FOV_DEGREES;
	return c;
}

// The tiles (checkered) and the frame: the part that never changes.
inline std::vector<Shape> boardShapes() {
	std::vector<Shape> shapes;
	const float edge = 1.0f - TILE_GAP;
	for (int row = 0; row < ROWS; row++) {
		for (int column = 0; column < COLUMNS; column++) {
			float x, z;
			cellCenter(column, row, x, z);
			const float* color = (column + row) % 2 ? TILE_DARK : TILE_LIGHT;
			shapes.push_back(shape(CUBE, x, BED_TOP - TILE_HEIGHT / 2, z, edge, TILE_HEIGHT, edge, color));
		}
	}
	const float halfW = COLUMNS / 2.0f, halfD = ROWS / 2.0f;
	const float height = TILE_HEIGHT + 0.12f;
	const float yMid = BED_TOP - TILE_HEIGHT + height / 2;
	const float wide = COLUMNS + 2 * FRAME_WIDTH;
	shapes.push_back(shape(CUBE, 0.0f, yMid, -halfD - FRAME_WIDTH / 2, wide, height, FRAME_WIDTH, FRAME_COLOR));
	shapes.push_back(shape(CUBE, 0.0f, yMid, halfD + FRAME_WIDTH / 2, wide, height, FRAME_WIDTH, FRAME_COLOR));
	shapes.push_back(shape(CUBE, -halfW - FRAME_WIDTH / 2, yMid, 0.0f, FRAME_WIDTH, height, (float)ROWS, FRAME_COLOR));
	shapes.push_back(shape(CUBE, halfW + FRAME_WIDTH / 2, yMid, 0.0f, FRAME_WIDTH, height, (float)ROWS, FRAME_COLOR));
	return shapes;
}

// One mark standing at world (x, z): a low disc in the mark's colour,
// darkened, and an X of blocks laid along both diagonals (a staircase, as
// Strata's primitives are axis-aligned) or an O ring of beads.
inline void markShapes(Mark mark, float x, float z, std::vector<Shape>& out) {
	if (mark == EMPTY) {
		return;
	}
	const float* color = mark == MARK_X ? X_COLOR : O_COLOR;
	out.push_back(shape(CYLINDER, x, BED_TOP + MARK_BASE_HEIGHT / 2, z, DISC_SIZE, MARK_BASE_HEIGHT, DISC_SIZE, color,
			DISC_SHADE));
	const float y = BED_TOP + MARK_BASE_HEIGHT + MARK_HEIGHT / 2;
	if (mark == MARK_X) {
		const int steps = 9;
		const float reach = 0.27f, block = 0.13f;
		for (int i = 0; i < steps; i++) {
			const float t = -reach + 2 * reach * i / (steps - 1);
			out.push_back(shape(CUBE, x + t, y, z + t, block, MARK_HEIGHT, block, color));
			if (i != steps / 2) {  // the centre block is shared by both strokes
				out.push_back(shape(CUBE, x + t, y, z - t, block, MARK_HEIGHT, block, color));
			}
		}
	} else {
		const int beads = 16;
		const float radius = 0.26f, bead = 0.15f;
		for (int i = 0; i < beads; i++) {
			const float angle = 2.0f * 3.14159265358979f * i / beads;
			out.push_back(shape(SPHERE, x + radius * std::cos(angle), y, z + radius * std::sin(angle), bead,
					MARK_HEIGHT, bead, color));
		}
	}
}

// Every mark on the board, boxes in reading order (top left first).
inline std::vector<Shape> pieceShapes(const Mark cells[9]) {
	std::vector<Shape> shapes;
	for (int i = 0; i < 9; i++) {
		float x, z;
		cellCenter(i % COLUMNS, i / COLUMNS, x, z);
		markShapes(cells[i], x, z, shapes);
	}
	return shapes;
}

// --------------------------------------------------------------------- HUD

// Strata's screen font is 8 x 8 cells, about one pixel size wide per glyph.
inline float textWidth(const std::string& text, float pixelSize) {
	return text.size() * pixelSize;
}

// The end screen over a width x height window: the board dimmed, and the
// result in large text on a panel near the top, clear of the board, so the
// last move stays in sight. Nothing while the game is on (the 2D board has no
// text either).
inline Hud resultHud(Result result, float width, float height) {
	Hud hud;
	if (result == RESULT_NONE) {
		return hud;
	}
	const std::string text = resultText(result);
	const float shortest = std::min(width, height);
	float size = std::floor(std::min(0.085f * shortest, 0.8f * width / text.size()));
	size = std::max(8.0f, std::min(size, 96.0f));
	const float textW = textWidth(text, size);
	const float pad = std::round(size * 0.6f);
	Panel overlay = {0.0f, 0.0f, width, height, {0, 0, 0, 0}};
	std::copy(OVERLAY_COLOR, OVERLAY_COLOR + 4, overlay.color);
	hud.panels.push_back(overlay);
	const float top = std::round(std::max(pad, 0.12f * height - size / 2));
	Panel card = {std::round((width - textW) / 2 - pad), top - pad, textW + 2 * pad, size + 2 * pad, {0, 0, 0, 0}};
	std::copy(RESULT_PANEL_COLOR, RESULT_PANEL_COLOR + 4, card.color);
	hud.panels.push_back(card);
	Label label;
	label.x = std::round((width - textW) / 2);
	label.y = top;
	label.pixelSize = size;
	std::copy(TEXT_COLOR, TEXT_COLOR + 4, label.color);
	label.text = text;
	hud.labels.push_back(label);
	return hud;
}

// ----------------------------------------------------------------- picking

// The same right-handed look-at and OpenGL perspective Strata draws with (up
// is +y, the field of view vertical), so a press lands on the tile the player
// sees, on the desktop and in a browser alike: graphik grid3d's pick.py.
struct Vec {
	double x, y, z;
};

inline Vec sub(const float a[3], const float b[3]) {
	return {double(a[0]) - b[0], double(a[1]) - b[1], double(a[2]) - b[2]};
}
inline double dot(Vec a, Vec b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec cross(Vec a, Vec b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
inline Vec unit(Vec a) {
	const double length = std::sqrt(dot(a, a));
	return {a.x / length, a.y / length, a.z / length};
}

inline void cameraBasis(const Camera& camera, Vec& forward, Vec& right, Vec& up) {
	forward = unit(sub(camera.target, camera.eye));
	right = unit(cross(forward, {0.0, 1.0, 0.0}));
	up = cross(right, forward);
}

// Where world point (x, y, z) lands in a width x height window, in pixels
// from the top-left; false when it is behind the camera.
inline bool projectToScreen(const Camera& camera, float x, float y, float z, float width, float height, float& sx,
		float& sy) {
	Vec forward, right, up;
	cameraBasis(camera, forward, right, up);
	const float point[3] = {x, y, z};
	const Vec offset = sub(point, camera.eye);
	const double depth = dot(offset, forward);
	if (depth <= 0) {
		return false;
	}
	const double focal = 1.0 / std::tan(camera.fovDegrees * 3.14159265358979 / 360.0);
	const double aspect = width / double(height);
	const double ndcX = focal / aspect * dot(offset, right) / depth;
	const double ndcY = focal * dot(offset, up) / depth;
	sx = float((ndcX + 1.0) / 2.0 * width);
	sy = float((1.0 - ndcY) / 2.0 * height);
	return true;
}

// The box under window pixel (x, y): true and its (column, row) when the ray
// through the pixel meets the tiles' top inside the board. The whole box
// counts, the gap between tiles included, so no press between two tiles is
// lost; a press on the frame or off the board picks nothing.
inline bool pickCell(const Camera& camera, float x, float y, float width, float height, int& column, int& row) {
	if (width <= 0 || height <= 0) {
		return false;
	}
	Vec forward, right, up;
	cameraBasis(camera, forward, right, up);
	const double focal = 1.0 / std::tan(camera.fovDegrees * 3.14159265358979 / 360.0);
	const double aspect = width / double(height);
	const double ndcX = x / double(width) * 2.0 - 1.0;
	const double ndcY = 1.0 - y / double(height) * 2.0;
	const Vec d = {forward.x + right.x * ndcX * aspect / focal + up.x * ndcY / focal,
			forward.y + right.y * ndcX * aspect / focal + up.y * ndcY / focal,
			forward.z + right.z * ndcX * aspect / focal + up.z * ndcY / focal};
	if (d.y >= 0) {
		return false;  // above the horizon
	}
	const double t = (BED_TOP - camera.eye[1]) / d.y;
	if (t <= 0) {
		return false;
	}
	const double wx = camera.eye[0] + d.x * t, wz = camera.eye[2] + d.z * t;
	const int c = int(std::floor(wx + COLUMNS / 2.0)), r = int(std::floor(wz + ROWS / 2.0));
	if (c < 0 || c >= COLUMNS || r < 0 || r >= ROWS) {
		return false;
	}
	column = c;
	row = r;
	return true;
}

// The middle of box (column, row) in the 2D window (layoutWidth x
// layoutHeight, split into thirds as the game's viewports are): where a click
// on that box in 3D is handed to the game, so its own keyForClick maps it.
inline void clickPoint(int column, int row, int layoutWidth, int layoutHeight, int& x, int& y) {
	x = column * layoutWidth / COLUMNS + layoutWidth / (2 * COLUMNS);
	y = row * layoutHeight / ROWS + layoutHeight / (2 * ROWS);
}

// ------------------------------------------------- the grid relay's messages

// Strata's web/kit/grid-relay/WIRE.md, format 1.0: an 8-byte header ("G3",
// major, minor, kind, flags, reserved) and a little-endian body.
const uint8_t WIRE_MAJOR = 1;
const uint8_t WIRE_MINOR = 0;
enum Kind { KIND_BOARD = 1, KIND_PIECES = 2, KIND_HUD = 3, KIND_VIEW = 4 };
enum Attach { ATTACH_WORLD = 0, ATTACH_PIECE = 1 };

class Writer {
public:
	std::vector<uint8_t> bytes;
	void u8(uint8_t v) { bytes.push_back(v); }
	void u16(uint16_t v) {
		u8(uint8_t(v & 0xff));
		u8(uint8_t(v >> 8));
	}
	void u32(uint32_t v) {
		for (int i = 0; i < 4; i++) {
			u8(uint8_t((v >> (8 * i)) & 0xff));
		}
	}
	void f32(float v) {
		uint32_t bits;
		std::memcpy(&bits, &v, 4);
		u32(bits);
	}
	void floats(const float* v, int n) {
		for (int i = 0; i < n; i++) {
			f32(v[i]);
		}
	}
	void header(Kind kind) {
		u8('G');
		u8('3');
		u8(WIRE_MAJOR);
		u8(WIRE_MINOR);
		u8(uint8_t(kind));
		u8(0);
		u16(0);
	}
	// A 48-byte shape record; for a piece shape, x and z are offsets from
	// anchor `anchor`.
	void shapeRecord(const Shape& s, Attach attach, uint16_t anchor, float dx = 0.0f, float dz = 0.0f) {
		u8(uint8_t(s.kind));
		u8(uint8_t(attach));
		u16(anchor);
		u16(0);
		u16(0);
		f32(s.position[0] - dx);
		f32(s.position[1]);
		f32(s.position[2] - dz);
		floats(s.scale, 3);
		floats(s.color, 4);
	}
};

// BOARD: the tiles and the frame, as world shapes, with no lights.
inline std::vector<uint8_t> encodeBoard() {
	const std::vector<Shape> shapes = boardShapes();
	Writer w;
	w.header(KIND_BOARD);
	w.u32(uint32_t(shapes.size()));
	w.u32(0);
	for (const Shape& s : shapes) {
		w.shapeRecord(s, ATTACH_WORLD, 0);
	}
	return w.bytes;
}

// PIECES: one anchor per mark, standing on its box (marks never slide), and
// the mark's shapes measured from it. A tick of 0 puts them there at once.
inline std::vector<uint8_t> encodePieces(const Mark cells[9]) {
	std::vector<float> anchors;
	std::vector<std::pair<uint16_t, Shape> > shapes;
	for (int i = 0; i < 9; i++) {
		if (cells[i] == EMPTY) {
			continue;
		}
		float x, z;
		cellCenter(i % COLUMNS, i / COLUMNS, x, z);
		const uint16_t anchor = uint16_t(anchors.size() / 4);
		anchors.insert(anchors.end(), {x, z, x, z});
		std::vector<Shape> mark;
		markShapes(cells[i], x, z, mark);
		for (const Shape& s : mark) {
			shapes.push_back(std::make_pair(anchor, s));
		}
	}
	Writer w;
	w.header(KIND_PIECES);
	w.f32(0.0f);
	w.u32(uint32_t(anchors.size() / 4));
	w.u32(uint32_t(shapes.size()));
	w.u32(0);
	w.floats(anchors.data(), int(anchors.size()));
	for (const auto& entry : shapes) {
		const float* a = &anchors[entry.first * 4];
		w.shapeRecord(entry.second, ATTACH_PIECE, entry.first, a[2], a[3]);
	}
	return w.bytes;
}

// HUD: panels (drawn first), then labels with their text padded to 4 bytes.
inline std::vector<uint8_t> encodeHud(const Hud& hud) {
	Writer w;
	w.header(KIND_HUD);
	w.u32(uint32_t(hud.panels.size()));
	w.u32(uint32_t(hud.labels.size()));
	for (const Panel& p : hud.panels) {
		w.f32(p.x);
		w.f32(p.y);
		w.f32(p.width);
		w.f32(p.height);
		w.floats(p.color, 4);
	}
	for (const Label& l : hud.labels) {
		w.f32(l.x);
		w.f32(l.y);
		w.f32(l.pixelSize);
		w.floats(l.color, 4);
		const std::string text = l.text.substr(0, 65535);
		w.u16(uint16_t(text.size()));
		w.u16(0);
		for (char ch : text) {
			w.u8(uint8_t(ch));
		}
		while (w.bytes.size() % 4) {
			w.u8(0);
		}
	}
	return w.bytes;
}

// VIEW: the camera, clear colour and direction to the sun (64 bytes).
inline std::vector<uint8_t> encodeView(const Camera& camera) {
	Writer w;
	w.header(KIND_VIEW);
	w.floats(camera.eye, 3);
	w.floats(camera.target, 3);
	w.f32(camera.fovDegrees);
	w.f32(NEAR_PLANE);
	w.f32(FAR_PLANE);
	w.floats(CLEAR_COLOR, 4);
	w.floats(SUN_DIRECTION, 3);
	return w.bytes;
}

}  // namespace view3d

#endif
