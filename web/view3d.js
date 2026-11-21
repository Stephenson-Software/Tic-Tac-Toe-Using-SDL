/* view3d.js: the page's side of the 3D board (window.TicTacToe3D).

   The game (index.wasm) keeps its rules and turns. Here the page loads
   Strata's grid relay (grid-relay.js, from Strata's release) into the
   #relay canvas, which lies over the game's own #canvas, and starts in 3D
   where it can run: the relay checks its size and WebGL 2 first, and the 2D
   canvas stays in place underneath while it loads, whenever it can't run, and
   when the player switches 3D off (the "3D view" button, remembered on this
   device; ?view=2d or ?view=3d in the address picks for one visit).

   The game asks three things of it (view3d_strata.h):
     active()      1 while the board is shown in 3D
     send(bytes)   one wire message for the relay (a Uint8Array)
     next()        the relay's next input event, or null

   Everything works without it: the 2D game never calls it unless 3D is on. */
(function () {
  "use strict";
  var STORE = "tic-tac-toe-sdl:view";
  var want = true;
  var queue = [];
  var button = null, board2d = null, board3d = null, note = null;

  function stored() {
    try { return window.localStorage.getItem(STORE); } catch (e) { return null; }
  }
  function store(view) {
    try { window.localStorage.setItem(STORE, view); } catch (e) { /* private window: not remembered */ }
  }
  function relay() { return window.StrataGridRelay || null; }
  function state() { return relay() ? relay().state() : "failed:the 3D view did not load"; }
  function showing() { return want && state() === "ready"; }

  // The canvases, the button and its note, from the relay's state.
  function render() {
    var s = state(), failed = s.indexOf("failed:") === 0, on = showing();
    if (board2d) board2d.style.visibility = on ? "hidden" : "visible";
    // (while it loads the relay shows its canvas itself, still clear, over the 2D board)
    if (board3d && s !== "loading") board3d.hidden = !on;
    if (button) {
      button.setAttribute("aria-pressed", want && !failed ? "true" : "false");
      button.disabled = failed;
      button.textContent = failed ? "3D unavailable" : "3D view";
      button.title = failed ? s.slice(7) : (want ? "Show the flat board" : "Show the board in 3D");
    }
    if (note) {
      note.textContent = failed && want ? "Playing on the flat board: " + s.slice(7) + "." :
        (want && s === "loading" ? "Loading the 3D board…" : "");
    }
  }

  // Until the relay has loaded (or failed); switched off meanwhile, it is
  // closed as soon as it is ready.
  function watch() {
    var s = state();
    if (s === "ready" && !want) relay().close();
    render();
    if (s === "loading") setTimeout(watch, 150);
  }

  function start() {
    if (!relay()) { render(); return; }
    relay().request(JSON.stringify({canvas: "relay", title: "the 3D board", maxBytes: 4000000}));
    watch();
  }

  function setWant(on) {
    want = on;
    queue = [];
    store(on ? "3d" : "2d");
    if (on) start();
    else watch();
  }

  window.TicTacToe3D = {
    active: function () { return showing() ? 1 : 0; },
    send: function (bytes) { return relay() ? relay().sendBytes(bytes) : -1; },
    next: function () {
      if (!queue.length && showing()) {
        try { queue = JSON.parse(relay().poll()); } catch (e) { queue = []; }
      }
      return queue.shift() || null;
    },
    toggle: function () { setWant(!want); },
    state: state,
  };

  function init() {
    button = document.getElementById("view3d");
    board2d = document.getElementById("canvas");
    board3d = document.getElementById("relay");
    note = document.getElementById("view3d-note");
    var asked = /[?&]view=(2d|3d)\b/.exec(window.location.search);
    want = (asked ? asked[1] : stored()) !== "2d";
    if (button) button.addEventListener("click", function () { setWant(!want); });
    if (want) start();
    render();
  }

  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", init);
  else init();
})();
