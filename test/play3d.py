#!/usr/bin/env python3
"""Plays the real game in 3D through Strata's drive channel and saves what it
shows: the start, the board after each move, and the end screen.

    python3 test/play3d.py <game directory> <output directory> [window]

The game (./tictactoe beside its PNG files and libstrata.so.0.10) is started
with STRATA_DRIVE pointing at a socket this script listens on (Strata's
docs/DRIVE.md); Strata draws it headless (or, with `window`, in a window on
$DISPLAY) and takes the moves from here: keys for some boxes and clicks on
the tiles, projected through the same camera the game draws with, for the
others. The computer's replies are random, so every box is tried in turn
until the game ends; a box already taken ignores its key and its click, as
in 2D. Exits 1 when the game does not reach its end screen.
"""

import json
import math
import os
import socket
import subprocess
import sys
import tempfile
import time

WIDTH = HEIGHT = 600

# USB HID usage ids of the keys for each box, in reading order: Q W E / A S D / Z X C.
KEYS = [20, 26, 8, 4, 22, 7, 29, 27, 6]


def camera(aspect):
    """view3d.h's fitCamera."""
    size = 4.0 * max(1.0, 1.0 / max(aspect, 0.5))
    distance = 0.86 * size + 0.9
    return (0.0, 1.35 * distance, 0.95 * distance), (0.0, -0.3, -0.04 * size), 42.0


def project(point, width, height):
    """view3d.h's projectToScreen."""
    eye, target, fov = camera(width / height)
    sub = lambda a, b: tuple(x - y for x, y in zip(a, b))
    dot = lambda a, b: sum(x * y for x, y in zip(a, b))

    def unit(a):
        n = math.sqrt(dot(a, a))
        return tuple(x / n for x in a)

    forward = unit(sub(target, eye))
    right = unit((forward[1] * 0 - forward[2] * 1, forward[2] * 0 - forward[0] * 0, forward[0] * 1 - forward[1] * 0))
    up = (right[1] * forward[2] - right[2] * forward[1], right[2] * forward[0] - right[0] * forward[2],
          right[0] * forward[1] - right[1] * forward[0])
    offset = sub(point, eye)
    depth = dot(offset, forward)
    focal = 1.0 / math.tan(math.radians(fov) / 2)
    x = focal / (width / height) * dot(offset, right) / depth
    y = focal * dot(offset, up) / depth
    return (x + 1) / 2 * width, (1 - y) / 2 * height


def tile(box):
    """Where box 0..8 (reading order) is on screen."""
    return project((box % 3 - 1.0, 0.0, box // 3 - 1.0), WIDTH, HEIGHT)


class Drive:
    def __init__(self, conn):
        self.conn = conn
        self.buf = b""

    def ask(self, **request):
        self.conn.sendall((json.dumps(request) + "\n").encode())
        while b"\n" not in self.buf:
            chunk = self.conn.recv(65536)
            if not chunk:
                raise RuntimeError("the game closed the drive channel")
            self.buf += chunk
        line, self.buf = self.buf.split(b"\n", 1)
        reply = json.loads(line)
        if not reply.get("ok"):
            raise RuntimeError("drive: %r" % reply)
        return reply


def main():
    game, out = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
    windowed = len(sys.argv) > 3 and sys.argv[3] == "window"
    os.makedirs(out, exist_ok=True)
    sock_dir = tempfile.mkdtemp()
    path = os.path.join(sock_dir, "drive.sock")
    server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    server.bind(path)
    server.listen(1)
    server.settimeout(30)
    env = dict(os.environ, STRATA_DRIVE=path, DO_NOT_TRACK="1", TRACE_USAGE_REPORTING="off")
    if not windowed:
        # no display at all: Strata draws offscreen, SDL2's hidden window is a dummy
        env.update(STRATA_DRIVE_SURFACE="headless", SDL_VIDEODRIVER="dummy")
    log = open(os.path.join(out, "play3d.log"), "w")
    proc = subprocess.Popen(["./tictactoe"], cwd=game, env=env, stdout=log, stderr=subprocess.STDOUT)
    try:
        conn, _ = server.accept()
        drive = Drive(conn)
        hello = drive.ask(op="hello", protocol=1)
        print("driving", hello)
        time.sleep(1.0)
        n = 0

        def shot(name):
            nonlocal n
            target = os.path.join(out, "tic-tac-toe-sdl-3d-play-%02d-%s.png" % (n, name))
            drive.ask(op="shot", path=target)
            print(target)
            n += 1

        shot("start")
        # keys for the boxes on the left and right, clicks for the middle column
        order = [4, 0, 8, 2, 6, 1, 7, 3, 5]
        for box in order:
            if box % 3 == 1:
                x, y = tile(box)
                drive.ask(op="event", type="mouse_motion", x=x, y=y)
                drive.ask(op="event", type="mouse_button_down", button=1, x=x, y=y)
                drive.ask(op="event", type="mouse_button_up", button=1, x=x, y=y)
                how = "click"
            else:
                drive.ask(op="event", type="key_down", scancode=KEYS[box])
                drive.ask(op="event", type="key_up", scancode=KEYS[box])
                how = "key"
            # the computer thinks for a second; a win shows its end screen a second later
            time.sleep(2.6)
            shot("%s-box%d" % (how, box))
            if proc.poll() is not None:
                raise RuntimeError("the game exited")
        time.sleep(1.5)
        shot("end")
        drive.ask(op="event", type="quit")
        proc.wait(timeout=10)
        print("the game closed with status", proc.returncode)
        return 0 if proc.returncode == 0 else 1
    finally:
        if proc.poll() is None:
            proc.kill()
        server.close()
        os.unlink(path)
        os.rmdir(sock_dir)


if __name__ == "__main__":
    sys.exit(main())
