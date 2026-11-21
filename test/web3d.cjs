// Plays the browser build in headless Chromium (WebGL 2 on SwiftShader) and
// checks the 3D board: it starts in 3D, a click or tap on a tile and the keys
// play the box the 2D board would, the 3D view can be switched off and on
// mid-game with the board kept, ?view=2d keeps to the 2D board without
// downloading the 3D one, and nothing is logged as an error.
//
//   npm install --no-save playwright && npx playwright install chromium
//   node test/web3d.cjs web/build <screenshot directory>
//
// Exits 1 on any failed check.
'use strict';
const fs = require('fs');
const http = require('http');
const path = require('path');
const { chromium } = require('playwright');

const site = path.resolve(process.argv[2] || 'web/build');
const shots = path.resolve(process.argv[3] || 'web/build/shots');
fs.mkdirSync(shots, { recursive: true });

const TYPES = { '.html': 'text/html', '.js': 'text/javascript', '.wasm': 'application/wasm', '.data': 'application/octet-stream' };
function serve() {
  const server = http.createServer((req, res) => {
    const name = decodeURIComponent(new URL(req.url, 'http://x').pathname).replace(/^\/+/, '') || 'index.html';
    const file = path.join(site, name);
    if (!file.startsWith(site) || !fs.existsSync(file)) { res.writeHead(404); res.end(); return; }
    const body = fs.readFileSync(file);
    res.writeHead(200, { 'Content-Type': TYPES[path.extname(file)] || 'application/octet-stream', 'Content-Length': body.length });
    res.end(req.method === 'HEAD' ? undefined : body);
  });
  return new Promise((resolve) => server.listen(0, '127.0.0.1', () => resolve(server)));
}

// view3d.h's camera and projection: where box (column, row) is on a w x h canvas.
function tile(column, row, w, h) {
  const aspect = w / h;
  const size = 4 * Math.max(1, 1 / Math.max(aspect, 0.5));
  const d = 0.86 * size + 0.9;
  const eye = [0, 1.35 * d, 0.95 * d], target = [0, -0.3, -0.04 * size];
  const sub = (a, b) => a.map((x, i) => x - b[i]);
  const dot = (a, b) => a.reduce((s, x, i) => s + x * b[i], 0);
  const unit = (a) => { const n = Math.sqrt(dot(a, a)); return a.map((x) => x / n); };
  const f = unit(sub(target, eye));
  const r = unit([-f[2], 0, f[0]]);
  const u = [r[1] * f[2] - r[2] * f[1], r[2] * f[0] - r[0] * f[2], r[0] * f[1] - r[1] * f[0]];
  const o = sub([column - 1, 0, row - 1], eye);
  const depth = dot(o, f), focal = 1 / Math.tan((42 * Math.PI) / 360);
  return [((focal / aspect * dot(o, r) / depth) + 1) / 2 * w, (1 - focal * dot(o, u) / depth) / 2 * h];
}

let failures = 0;
function check(ok, what) {
  console.log((ok ? 'ok   ' : 'FAIL ') + what);
  if (!ok) failures++;
}

// The marks on the board, as the game last sent them to the relay: the
// anchors of its last PIECES message, one per mark.
async function marks(page) {
  return page.evaluate(() => window.__pieces);
}

async function open(browser, url, device, name) {
  const context = await browser.newContext(device);
  const page = await context.newPage();
  const errors = [];
  const wasm = [];
  page.on('console', (m) => { if (m.type() === 'error') errors.push(m.text()); });
  page.on('pageerror', (e) => errors.push(String(e)));
  page.on('request', (r) => { if (r.url().endsWith('grid-relay-scene.wasm')) wasm.push(r.url()); });
  // Count the marks in each PIECES message the game sends (anchors: u32 at byte 12).
  await page.addInitScript(() => {
    window.__pieces = -1;
    const hook = () => {
      const api = window.TicTacToe3D;
      if (!api || api.__hooked) return;
      const send = api.send;
      api.send = (bytes) => {
        if (bytes[4] === 2) window.__pieces = bytes[12] | (bytes[13] << 8);
        return send(bytes);
      };
      api.__hooked = true;
    };
    document.addEventListener('DOMContentLoaded', hook);
  });
  await page.goto(url);
  await page.waitForFunction(() => document.getElementById('status').textContent === '', null, { timeout: 60000 });
  return { page, context, errors, wasm, name };
}

async function press(page, how, column, row) {
  const box = await page.locator('#relay').boundingBox();
  const [x, y] = tile(column, row, box.width, box.height);
  if (how === 'tap') await page.touchscreen.tap(box.x + x, box.y + y);
  else if (how === 'click') await page.mouse.click(box.x + x, box.y + y);
  else await page.keyboard.press(how);
  await page.waitForTimeout(2600);  // the computer waits a second before its move
}

(async () => {
  const server = await serve();
  const base = `http://127.0.0.1:${server.address().port}/`;
  const browser = await chromium.launch({
    args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'],
  });
  try {
    // A desktop window, played with clicks and keys, switched to 2D and back.
    {
      const t = await open(browser, base, { viewport: { width: 700, height: 900 } }, 'desktop');
      const { page } = t;
      await page.waitForFunction(() => window.TicTacToe3D.active() === 1, null, { timeout: 60000 });
      await page.waitForTimeout(800);
      check(await page.evaluate(() => getComputedStyle(document.getElementById('canvas')).visibility) === 'hidden',
        'desktop: starts in 3D, the 2D board hidden under it');
      check(await page.locator('#view3d').getAttribute('aria-pressed') === 'true', 'desktop: the 3D view button is on');
      await page.screenshot({ path: path.join(shots, 'tic-tac-toe-sdl-web3d-desktop-start.png') });
      check(await marks(page) === 0, 'desktop: the 3D board starts empty');
      await press(page, 'click', 1, 1);
      check(await marks(page) === 2, 'desktop: a click on the middle tile plays X there, and the computer answers');
      await page.screenshot({ path: path.join(shots, 'tic-tac-toe-sdl-web3d-desktop-after-click.png') });
      const before = await marks(page);
      await press(page, 'click', 1, -1);  // the frame's far side and beyond: no box
      check(await marks(page) === before, 'desktop: a click off the board plays nothing');
      // switched to 2D, the game goes on in 2D with the same board
      await page.click('#view3d');
      await page.waitForTimeout(500);
      check(await page.evaluate(() => document.getElementById('relay').hidden &&
        getComputedStyle(document.getElementById('canvas')).visibility === 'visible'), 'desktop: 3D off shows the 2D board');
      check(await page.locator('#view3d').getAttribute('aria-pressed') === 'false', 'desktop: the button says 3D is off');
      await page.screenshot({ path: path.join(shots, 'tic-tac-toe-sdl-web3d-desktop-switched-to-2d.png') });
      await page.click('#view3d');
      await page.waitForFunction(() => window.TicTacToe3D.active() === 1, null, { timeout: 30000 });
      check(true, 'desktop: 3D on again');
      // the keys, Q W E / A S D / Z X C, until the game ends
      for (const key of ['q', 'e', 'z', 'c', 'w', 'a', 'd', 'x']) {
        await press(page, key);
      }
      await page.waitForTimeout(1500);
      await page.screenshot({ path: path.join(shots, 'tic-tac-toe-sdl-web3d-desktop-end.png') });
      const hud = await page.evaluate(() => JSON.parse(window.StrataGridRelay.stats()));
      check(hud.state === 'ready' && hud.messages >= 6, `desktop: the relay took the game's messages (${hud.messages})`);
      check(t.errors.length === 0, 'desktop: no errors logged' + (t.errors.length ? ': ' + t.errors.join(' | ') : ''));
      await t.context.close();
    }
    // A phone held upright, played by tapping tiles.
    {
      const t = await open(browser, base, { viewport: { width: 390, height: 844 }, deviceScaleFactor: 2, isMobile: true, hasTouch: true }, 'phone');
      const { page } = t;
      await page.waitForFunction(() => window.TicTacToe3D.active() === 1, null, { timeout: 60000 });
      await page.waitForTimeout(800);
      await page.screenshot({ path: path.join(shots, 'tic-tac-toe-sdl-web3d-phone-start.png') });
      await press(page, 'tap', 0, 0);
      check(await marks(page) === 2, 'phone: a tap on the top left tile plays X there, and the computer answers');
      for (const [c, r] of [[2, 2], [2, 0], [0, 2], [1, 1], [1, 0], [0, 1], [2, 1], [1, 2]]) {
        await press(page, 'tap', c, r);
      }
      await page.waitForTimeout(1500);
      await page.screenshot({ path: path.join(shots, 'tic-tac-toe-sdl-web3d-phone-end.png') });
      check(await marks(page) >= 5, 'phone: tapping every tile plays the game to its end');
      check(t.errors.length === 0, 'phone: no errors logged' + (t.errors.length ? ': ' + t.errors.join(' | ') : ''));
      await t.context.close();
    }
    // ?view=2d: the 2D board, and the 3D one never downloaded.
    {
      const t = await open(browser, base + '?view=2d', { viewport: { width: 700, height: 900 } }, '2d');
      const { page } = t;
      await page.waitForTimeout(1500);
      check(t.wasm.length === 0, '?view=2d: the 3D board is not downloaded');
      check(await page.evaluate(() => window.TicTacToe3D.active()) === 0 &&
        await page.evaluate(() => getComputedStyle(document.getElementById('canvas')).visibility) === 'visible',
        '?view=2d: the 2D board is shown');
      const canvas = await page.locator('#canvas').boundingBox();
      await page.mouse.click(canvas.x + canvas.width / 2, canvas.y + canvas.height / 2);
      await page.waitForTimeout(2600);
      await page.screenshot({ path: path.join(shots, 'tic-tac-toe-sdl-web3d-view-2d.png') });
      check(t.errors.length === 0, '?view=2d: no errors logged' + (t.errors.length ? ': ' + t.errors.join(' | ') : ''));
      await t.context.close();
    }
  } finally {
    await browser.close();
    server.close();
  }
  console.log(failures ? `${failures} check(s) failed` : 'all checks passed');
  process.exit(failures ? 1 : 0);
})().catch((e) => { console.error(e); process.exit(1); });
