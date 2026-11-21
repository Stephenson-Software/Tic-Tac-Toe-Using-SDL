// view3d_strata.h: the 3D view, drawn by Strata (https://github.com/Preponderous-Software/strata).
//
// The game calls four things, and keeps every rule, key and turn of its own:
//
//   view3d::open(...)       desktop: load Strata and open its window; false
//                           when it can't run here, and the 2D window is used
//   view3d::active()        whether the board is being shown in 3D now
//   view3d::show(cells, r)  draw the board (and the end screen for result r)
//   view3d::pollEvent(&e)   the player's input in the 3D view, as the SDL
//                           events the 2D view would have produced: a key is
//                           the same key, and a click or tap on a box becomes
//                           a click at the middle of that box in the 2D
//                           window, so keyForClick decides what it means
//
// On the desktop, libstrata (libstrata.so.0.10 on Linux, strata.dll on Windows)
// is loaded at run time from beside the executable, from STRATA_LIBRARY, or
// from the library path. Without it, or without a GPU that can run it, the
// game opens its 2D window as before. Strata draws in its own window (SDL3
// inside the library, with its symbols hidden), and the game's SDL2 window is
// created hidden and keeps its textures, which is how the rules tell the
// marks apart.
//
// In a browser (Emscripten), the page loads Strata's grid relay
// (web/view3d.js), and this file sends it the scene as wire messages
// (view3d.h) and reads its input back; the 2D canvas stays underneath and is
// shown whenever the 3D view is off or can't run.
#ifndef VIEW3D_STRATA_H
#define VIEW3D_STRATA_H

#include <SDL.h>
#include <cstdlib>
#include <string>
#include <vector>

#include "view3d.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define STRATA_STATIC  // declarations only: every entry point is looked up at run time
#include "third_party/strata/strata.h"
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif
#endif

namespace view3d {

// A click or tap in a width x height 3D view, as the SDL event the 2D window
// would have produced; a press off the board becomes a click outside the
// window, which the game ignores like any other unknown key.
inline void clickEvent(const Camera& camera, int button, float x, float y, float width, float height, SDL_Event* e) {
	int column = 0, row = 0, cx = -1, cy = -1;
	if (pickCell(camera, x, y, width, height, column, row)) {
		clickPoint(column, row, LAYOUT_W, LAYOUT_H, cx, cy);
	}
	SDL_zerop(e);
	e->type = SDL_MOUSEBUTTONDOWN;
	e->button.button = Uint8(button);
	e->button.state = SDL_PRESSED;
	e->button.clicks = 1;
	e->button.x = cx;
	e->button.y = cy;
}

// A key (by USB HID usage id, which SDL2 scancodes are) as the SDL key event
// the 2D window would have produced, through the same keyboard layout.
// Behind a desktop 3D window SDL has no video subsystem, and so no keyboard
// layout: letters and digits are then read by where they are on a US keyboard
// (the Q W E / A S D / Z X C block is where it always is).
inline void keyEvent(int scancode, int repeat, SDL_Event* e) {
	SDL_zerop(e);
	e->type = SDL_KEYDOWN;
	e->key.state = SDL_PRESSED;
	e->key.repeat = Uint8(repeat ? 1 : 0);
	e->key.keysym.scancode = SDL_Scancode(scancode);
	if (SDL_WasInit(SDL_INIT_VIDEO)) {
		e->key.keysym.sym = SDL_GetKeyFromScancode(SDL_Scancode(scancode));
	} else if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z) {
		e->key.keysym.sym = SDL_Keycode('a' + (scancode - SDL_SCANCODE_A));
	} else if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_9) {
		e->key.keysym.sym = SDL_Keycode('1' + (scancode - SDL_SCANCODE_1));
	} else if (scancode == SDL_SCANCODE_0) {
		e->key.keysym.sym = SDLK_0;
	} else {
		e->key.keysym.sym = SDL_SCANCODE_TO_KEYCODE(scancode);
	}
}

#ifndef __EMSCRIPTEN__

// ------------------------------------------------------------------ desktop

struct StrataApi {
	decltype(&strata_abi_version) abiVersion;
	decltype(&strata_abi_minor) abiMinor;
	decltype(&strata_last_error) lastError;
	decltype(&strata_create) create;
	decltype(&strata_destroy) destroy;
	decltype(&strata_poll_event) pollEvent;
	decltype(&strata_get_drawable_size) drawableSize;
	decltype(&strata_set_camera) setCamera;
	decltype(&strata_camera_look_at) lookAt;
	decltype(&strata_lighting_defaults) lightingDefaults;
	decltype(&strata_set_lighting) setLighting;
	decltype(&strata_set_sun_direction) setSunDirection;
	decltype(&strata_set_point_lights) setPointLights;
	decltype(&strata_begin_frame) beginFrame;
	decltype(&strata_submit_primitives) submitPrimitives;
	decltype(&strata_submit_rects) submitRects;
	decltype(&strata_submit_text) submitText;
	decltype(&strata_end_frame) endFrame;
	decltype(&strata_present) present;
	decltype(&strata_save_screenshot_png) saveScreenshotPng;
};

struct Desktop {
	StrataApi api;
	StrataEngine* engine = nullptr;
	int width = LAYOUT_W, height = LAYOUT_H;
	std::string reason;
};

inline Desktop& desktop() {
	static Desktop d;
	return d;
}

inline std::string executableDirectory() {
#ifdef _WIN32
	char path[MAX_PATH];
	const DWORD n = GetModuleFileNameA(NULL, path, MAX_PATH);
	std::string s(path, n);
	const size_t slash = s.find_last_of("\\/");
#else
	char path[4096];
	const ssize_t n = readlink("/proc/self/exe", path, sizeof(path) - 1);
	std::string s(path, n > 0 ? size_t(n) : 0);
	const size_t slash = s.find_last_of('/');
#endif
	return slash == std::string::npos ? std::string() : s.substr(0, slash + 1);
}

// Loads libstrata and looks up every entry point; false (and the reason) when
// the library or one of them is missing.
inline bool loadStrata(StrataApi& api, std::string& reason) {
	std::vector<std::string> candidates;
	const char* own = std::getenv("STRATA_LIBRARY");
	if (own && *own) {
		candidates.push_back(own);
	}
#ifdef _WIN32
	candidates.push_back(executableDirectory() + "strata.dll");
	candidates.push_back("strata.dll");
	HMODULE lib = NULL;
	for (const std::string& c : candidates) {
		if ((lib = LoadLibraryA(c.c_str())) != NULL) break;
	}
	auto find = [&](const char* name) { return reinterpret_cast<void*>(GetProcAddress(lib, name)); };
#else
	candidates.push_back(executableDirectory() + "libstrata.so.0.10");
	candidates.push_back("libstrata.so.0.10");
	candidates.push_back("libstrata.so");
	void* lib = nullptr;
	for (const std::string& c : candidates) {
		if ((lib = dlopen(c.c_str(), RTLD_NOW | RTLD_LOCAL)) != nullptr) break;
	}
	auto find = [&](const char* name) { return dlsym(lib, name); };
#endif
	if (!lib) {
		reason = "libstrata was not found";
		return false;
	}
	bool ok = true;
#define VIEW3D_FIND(field, name) \
	ok = ok && (*reinterpret_cast<void**>(&api.field) = find(name)) != nullptr
	VIEW3D_FIND(abiVersion, "strata_abi_version");
	VIEW3D_FIND(abiMinor, "strata_abi_minor");
	VIEW3D_FIND(lastError, "strata_last_error");
	VIEW3D_FIND(create, "strata_create");
	VIEW3D_FIND(destroy, "strata_destroy");
	VIEW3D_FIND(pollEvent, "strata_poll_event");
	VIEW3D_FIND(drawableSize, "strata_get_drawable_size");
	VIEW3D_FIND(setCamera, "strata_set_camera");
	VIEW3D_FIND(lookAt, "strata_camera_look_at");
	VIEW3D_FIND(lightingDefaults, "strata_lighting_defaults");
	VIEW3D_FIND(setLighting, "strata_set_lighting");
	VIEW3D_FIND(setSunDirection, "strata_set_sun_direction");
	VIEW3D_FIND(setPointLights, "strata_set_point_lights");
	VIEW3D_FIND(beginFrame, "strata_begin_frame");
	VIEW3D_FIND(submitPrimitives, "strata_submit_primitives");
	VIEW3D_FIND(submitRects, "strata_submit_rects");
	VIEW3D_FIND(submitText, "strata_submit_text");
	VIEW3D_FIND(endFrame, "strata_end_frame");
	VIEW3D_FIND(present, "strata_present");
	VIEW3D_FIND(saveScreenshotPng, "strata_save_screenshot_png");
#undef VIEW3D_FIND
	if (!ok) {
		reason = "libstrata is missing an entry point";
		return false;
	}
	// Screen rectangles arrived in ABI 1.11.
	if (api.abiVersion() != STRATA_ABI_VERSION || api.abiMinor() < 11) {
		reason = "libstrata is too old or too new (ABI " + std::to_string(api.abiVersion()) + "." +
				std::to_string(api.abiMinor()) + ", needs 1.11 or later 1.x)";
		return false;
	}
	return true;
}

// Opens the 3D view: a width x height window (or, headless, an offscreen
// target for still pictures). False, with the reason in reason(), when Strata
// can't run here.
inline bool open(int width, int height, const char* title, bool headless = false) {
	Desktop& d = desktop();
	if (d.engine) {
		return true;
	}
	if (!loadStrata(d.api, d.reason)) {
		return false;
	}
	StrataEngineDesc desc = {};
	desc.struct_size = sizeof(desc);
	desc.abi_version = STRATA_ABI_VERSION;
	desc.surface = headless ? STRATA_SURFACE_HEADLESS : STRATA_SURFACE_WINDOW;
	desc.width = width;
	desc.height = height;
	desc.flags = headless ? 0u : uint32_t(STRATA_FLAG_VSYNC);
	desc.title = headless ? nullptr : title;
	if (d.api.create(&desc, &d.engine) != STRATA_OK) {
		d.reason = std::string("Strata could not start: ") + d.api.lastError();
		d.engine = nullptr;
		return false;
	}
	int32_t w = width, h = height;
	if (d.api.drawableSize(d.engine, &w, &h) == STRATA_OK && w > 0 && h > 0) {
		d.width = w;
		d.height = h;
	} else {
		d.width = width;
		d.height = height;
	}
	// graphik grid3d's lighting: the sun, shadows out to 40 units, no fog.
	StrataLighting lighting;
	d.api.lightingDefaults(STRATA_LIGHTING_SUN, &lighting);
	lighting.shadow_distance = SHADOW_DISTANCE;
	lighting.fog = 0;
	d.api.setLighting(d.engine, &lighting);
	d.api.setSunDirection(d.engine, SUN_DIRECTION);
	d.api.setPointLights(d.engine, nullptr, 0);
	return true;
}

inline const std::string& reason() { return desktop().reason; }

inline bool active() { return desktop().engine != nullptr; }

// Whether the 3D view has a window of its own (on the desktop it does, and
// the game then opens none of its own).
inline bool ownsWindow() { return active(); }

inline Camera camera() {
	const Desktop& d = desktop();
	return fitCamera(d.width / float(d.height));
}

// One frame: the board, its marks and, for a finished game, the end screen.
inline void show(const Mark cells[9], Result result) {
	Desktop& d = desktop();
	if (!d.engine) {
		return;
	}
	const Camera c = camera();
	const float up[3] = {0.0f, 1.0f, 0.0f};
	StrataCamera sc;
	d.api.lookAt(c.eye, c.target, up, c.fovDegrees, d.width / float(d.height), NEAR_PLANE, FAR_PLANE, &sc);
	d.api.setCamera(d.engine, &sc);
	StrataFrameDesc frame = {};
	frame.clear_flags = STRATA_CLEAR_COLOR | STRATA_CLEAR_DEPTH;
	std::copy(CLEAR_COLOR, CLEAR_COLOR + 4, frame.clear_color);
	d.api.beginFrame(d.engine, &frame);
	std::vector<Shape> shapes = boardShapes();
	const std::vector<Shape> marks = pieceShapes(cells);
	shapes.insert(shapes.end(), marks.begin(), marks.end());
	std::vector<StrataRenderItem> items(shapes.size());
	for (size_t i = 0; i < shapes.size(); i++) {
		items[i].kind = shapes[i].kind;
		std::copy(shapes[i].position, shapes[i].position + 3, items[i].position);
		std::copy(shapes[i].scale, shapes[i].scale + 3, items[i].scale);
		std::copy(shapes[i].color, shapes[i].color + 4, items[i].color);
	}
	d.api.submitPrimitives(d.engine, items.data(), int32_t(items.size()));
	const Hud hud = resultHud(result, float(d.width), float(d.height));
	if (!hud.panels.empty()) {
		std::vector<StrataRect> rects;
		for (const Panel& p : hud.panels) {
			StrataRect r = {p.x, p.y, p.width, p.height, {p.color[0], p.color[1], p.color[2], p.color[3]}};
			rects.push_back(r);
		}
		d.api.submitRects(d.engine, rects.data(), int32_t(rects.size()));
	}
	if (!hud.labels.empty()) {
		std::vector<StrataText> texts;
		for (const Label& l : hud.labels) {
			StrataText t = {l.x, l.y, l.pixelSize, STRATA_TEXT_SHADOW, {l.color[0], l.color[1], l.color[2], l.color[3]},
					l.text.c_str()};
			texts.push_back(t);
		}
		d.api.submitText(d.engine, texts.data(), int32_t(texts.size()));
	}
	d.api.endFrame(d.engine);
	d.api.present(d.engine);
}

// The next input from the 3D window, as an SDL event; 0 when there is none.
inline int pollEvent(SDL_Event* e) {
	Desktop& d = desktop();
	if (!d.engine) {
		return 0;
	}
	StrataEvent ev;
	while (d.api.pollEvent(d.engine, &ev)) {
		switch (ev.type) {
			case STRATA_EVENT_QUIT:
				SDL_zerop(e);
				e->type = SDL_QUIT;
				return 1;
			case STRATA_EVENT_KEY_DOWN:
				keyEvent(ev.scancode, ev.repeat, e);
				return 1;
			case STRATA_EVENT_MOUSE_BUTTON_DOWN:
				clickEvent(camera(), ev.button, ev.x, ev.y, float(d.width), float(d.height), e);
				return 1;
			case STRATA_EVENT_RESIZED:
				if (ev.width > 0 && ev.height > 0) {
					d.width = ev.width;
					d.height = ev.height;
				}
				break;
			default:
				break;
		}
	}
	return 0;
}

// The last frame as a PNG (for the screenshots in docs/).
inline bool savePng(const char* path) {
	Desktop& d = desktop();
	return d.engine && d.api.saveScreenshotPng(d.engine, path) == STRATA_OK;
}

inline void close() {
	Desktop& d = desktop();
	if (d.engine) {
		d.api.destroy(d.engine);
		d.engine = nullptr;
	}
}

#else

// ------------------------------------------------------------------ browser

// web/view3d.js keeps the page's side: whether the player wants 3D, loading
// the relay, swapping the canvases and queueing the relay's input.
EM_JS(int, view3d_js_active, (), {
	return (typeof window !== "undefined" && window.TicTacToe3D) ? window.TicTacToe3D.active() : 0;
});

EM_JS(int, view3d_js_send, (const uint8_t* data, int size), {
	return window.TicTacToe3D.send(HEAPU8.slice(data, data + size));
});

// The next relay event as [type, scancode, repeat, button, x, y, width,
// height] into out; 0 when there is none.
EM_JS(int, view3d_js_next, (float* out), {
	if (typeof window === "undefined" || !window.TicTacToe3D) return 0;
	const ev = window.TicTacToe3D.next();
	if (!ev) return 0;
	const f = [ev.type || 0, ev.scancode || 0, ev.repeat || 0, ev.button || 0, ev.x || 0, ev.y || 0,
		ev.width || 0, ev.height || 0];
	for (let i = 0; i < 8; i++) HEAPF32[(out >> 2) + i] = f[i];
	return 1;
});

struct Browser {
	int width = LAYOUT_W, height = LAYOUT_H;  // the relay canvas, CSS pixels
	std::vector<uint8_t> board, view, pieces, hud;  // what the relay was last sent
};

inline Browser& browser() {
	static Browser b;
	return b;
}

// The page decides (web/view3d.js): the 3D view starts on its own where it
// can run and the player can switch it off.
inline bool open(int, int, const char*, bool = false) { return false; }

inline const std::string& reason() {
	static const std::string none;
	return none;
}

inline bool active() { return view3d_js_active() != 0; }

// The relay draws on a canvas beside the game's, which stays for the 2D view.
inline bool ownsWindow() { return false; }

inline Camera camera() {
	const Browser& b = browser();
	return fitCamera(b.width / float(b.height));
}

inline void sendIfChanged(std::vector<uint8_t>& last, const std::vector<uint8_t>& message) {
	if (message != last && view3d_js_send(message.data(), int(message.size())) >= 0) {
		last = message;
	}
}

// Sends what changed since the last call; the relay draws it every frame.
inline void show(const Mark cells[9], Result result) {
	if (!active()) {
		return;
	}
	Browser& b = browser();
	sendIfChanged(b.board, encodeBoard());
	sendIfChanged(b.view, encodeView(camera()));
	sendIfChanged(b.pieces, encodePieces(cells));
	sendIfChanged(b.hud, encodeHud(resultHud(result, float(b.width), float(b.height))));
}

inline int pollEvent(SDL_Event* e) {
	float f[8];
	while (view3d_js_next(f)) {
		const int type = int(f[0]);
		if (type == 8 && f[6] > 0 && f[7] > 0) {  // STRATA_EVENT_RESIZED
			browser().width = int(f[6]);
			browser().height = int(f[7]);
		} else if (type == 2) {  // STRATA_EVENT_KEY_DOWN
			keyEvent(int(f[1]), int(f[2]), e);
			return 1;
		} else if (type == 5) {  // STRATA_EVENT_MOUSE_BUTTON_DOWN (a tap's too)
			const Browser& b = browser();
			clickEvent(camera(), int(f[3]), f[4], f[5], float(b.width), float(b.height), e);
			return 1;
		}
	}
	return 0;
}

inline bool savePng(const char*) { return false; }

inline void close() {}

#endif

}  // namespace view3d

#endif
