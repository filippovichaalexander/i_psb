#!/usr/bin/env python3
"""Development server for the i_psb companion SPA.

Serves web/ and a mock implementation of the HTTP API documented in
docs/12-web-api.md. It is intentionally dependency-free (stdlib only) and
never writes uploads to disk: the goal is to exercise the UI, not the SD card.

Run:
    python -I tools/devserver.py --host 127.0.0.1 --port 8080
"""

from __future__ import annotations

import argparse
import json
import posixpath
import time
import urllib.parse
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent / "web"
START = time.time()

FILES: dict[str, dict] = {
    "/Music": {"is_dir": True, "size": 0, "mtime": START},
    "/Music/Album A": {"is_dir": True, "size": 0, "mtime": START},
    "/Music/Album A/01 - Intro.flac": {"is_dir": False, "size": 28_400_000, "mtime": START},
    "/Music/Album A/02 - Theme.mp3": {"is_dir": False, "size": 8_900_000, "mtime": START},
    "/Music/Демо.wav": {"is_dir": False, "size": 41_200_000, "mtime": START},
    "/Music/Новый альбом": {"is_dir": True, "size": 0, "mtime": START},
    "/config": {"is_dir": True, "size": 0, "mtime": START},
    "/logs": {"is_dir": True, "size": 0, "mtime": START},
    "/system": {"is_dir": True, "size": 0, "mtime": START},
}

STATE = {
    "playback": {
        "state": "paused",
        "title": "01 - Intro",
        "artist": "Mock Artist",
        "album": "Mock Album",
        "pos": 42,
        "dur": 210,
    },
    "wifi": {"ssid": "home", "mode": "sta", "rssi": -55},
    "logs": [
        "[0][i][boot] i_psb 0.1.0",
        "[1][i][boot] reset=power-on chip_rev=0 flash=16777216 psram=8388608 heap=392000",
        "[120][i][sd] mounted: total=61000.0 MB used=1200.0 MB",
        "[121][i][epd] panel 250x122 rotation=1",
        "[122][i][audio] I2S ready: BCLK=18 LRCK=16 DIN=17 MCLK=-1 volume=4/21",
        "[123][i][wifi] not implemented in M0 (planned M3)",
        "[124][w][app] mock API: uploads are discarded",
    ],
}

ROOTS = ("/Music", "/config", "/logs", "/system")


def now() -> float:
    return time.time()


def normalise(path: str) -> str:
    if not path:
        return "/"
    path = urllib.parse.unquote(path)
    path = posixpath.normpath("/" + path.lstrip("/"))
    if path == ".":
        return "/"
    return path


def allowed(path: str) -> bool:
    if path == "/":
        return True
    return any(path == root or path.startswith(root + "/") for root in ROOTS)


def json_bytes(payload: object) -> bytes:
    return json.dumps(payload, ensure_ascii=False).encode("utf-8")


class Handler(SimpleHTTPRequestHandler):
    server_version = "i_psb-mock/0.1"

    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(ROOT), **kwargs)

    def log_message(self, fmt, *args):
        if self.path.startswith("/api/"):
            super().log_message(fmt, *args)

    # -- helpers ---------------------------------------------------------
    def send_json(self, payload, code=200):
        body = json_bytes(payload)
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def authorised(self) -> bool:
        return bool(self.headers.get("X-IPSB-Token", "").strip())

    def body_json(self):
        length = int(self.headers.get("Content-Length") or 0)
        if length <= 0 or length > 1_000_000:
            return None
        raw = self.rfile.read(length)
        try:
            return json.loads(raw.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError):
            return None

    def query(self):
        return urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query)

    def list_items(self, path: str):
        prefix = path.rstrip("/") + "/"
        items = []
        for name, meta in FILES.items():
            if not name.startswith(prefix):
                continue
            rest = name[len(prefix):]
            if not rest or "/" in rest:
                continue
            items.append({"name": rest, **meta})
        items.sort(key=lambda item: (not item["is_dir"], item["name"].lower()))
        return items

    # -- GET -------------------------------------------------------------
    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        if not parsed.path.startswith("/api/"):
            super().do_GET()
            return

        if parsed.path == "/api/ping":
            self.send_json({"ok": True})
            return

        if parsed.path == "/api/status":
            used = sum(meta["size"] for meta in FILES.values() if not meta["is_dir"])
            total = 64 * 1024**3
            self.send_json(
                {
                    "fw": "0.1.0-mock",
                    "uptime_s": int(now() - START),
                    "battery": {"mv": 3920, "pct": 78, "charging": False, "valid": True},
                    "storage": {"total": total, "used": used, "free": total - used},
                    "playback": STATE["playback"],
                    "wifi": {"mode": STATE["wifi"]["mode"], "ssid": STATE["wifi"]["ssid"],
                             "rssi": STATE["wifi"]["rssi"]},
                    "sd": True,
                    "errors": [],
                }
            )
            return

        if parsed.path == "/api/files":
            if not self.authorised():
                self.send_json({"error": "unauthorized"}, 401)
                return
            query = self.query()
            path = normalise((query.get("path") or ["/"])[0])
            if not allowed(path):
                self.send_json({"error": "forbidden"}, 403)
                return
            if path not in FILES or not FILES[path]["is_dir"]:
                self.send_json({"error": "not_found"}, 404)
                return
            items = self.list_items(path)
            self.send_json({"path": path, "total": len(items), "items": items})
            return

        if parsed.path == "/api/logs":
            if not self.authorised():
                self.send_json({"error": "unauthorized"}, 401)
                return
            lines = int((self.query().get("lines") or ["200"])[0])
            lines = max(1, min(lines, 200))
            self.send_json({"lines": STATE["logs"][-lines:]})
            return

        if parsed.path == "/api/wifi":
            if not self.authorised():
                self.send_json({"error": "unauthorized"}, 401)
                return
            wifi = STATE["wifi"]
            self.send_json({"ssid": wifi["ssid"], "mode": wifi["mode"],
                            "rssi": wifi["rssi"]})
            return

        self.send_json({"error": "not_found"}, 404)

    # -- POST ------------------------------------------------------------
    def do_POST(self):
        parsed = urllib.parse.urlparse(self.path)
        if not parsed.path.startswith("/api/"):
            self.send_json({"error": "not_found"}, 404)
            return
        if not self.authorised():
            self.send_json({"error": "unauthorized"}, 401)
            return

        if parsed.path == "/api/upload":
            query = self.query()
            path = normalise((query.get("path") or ["/Music"])[0])
            name = Path((query.get("name") or ["upload.bin"])[0]).name
            if not allowed(path) or path not in FILES or not FILES[path]["is_dir"]:
                self.send_json({"error": "bad_request"}, 400)
                return
            if not name or name in {".", ".."}:
                self.send_json({"error": "bad_request"}, 400)
                return

            length = int(self.headers.get("Content-Length") or 0)
            if length <= 0 or length > 4 * 1024**3:
                self.send_json({"error": "too_large"}, 413)
                return
            remaining = length
            while remaining > 0:
                chunk = self.rfile.read(min(remaining, 64 * 1024))
                if not chunk:
                    break
                remaining -= len(chunk)
            full = posixpath.join(path, name)
            FILES[full] = {"is_dir": False, "size": length - remaining, "mtime": now()}
            STATE["logs"].append(f"[mock] upload {full} ({length} bytes)")
            self.send_json({"ok": True, "path": full, "bytes": length})
            return

        if parsed.path == "/api/mkdir":
            payload = self.body_json() or {}
            path = normalise(str(payload.get("path", "")))
            if not allowed(path) or path == "/":
                self.send_json({"error": "bad_request"}, 400)
                return
            parent = posixpath.dirname(path)
            if parent not in FILES or not FILES[parent]["is_dir"]:
                self.send_json({"error": "not_found"}, 404)
                return
            if path in FILES:
                self.send_json({"error": "exists"}, 409)
                return
            FILES[path] = {"is_dir": True, "size": 0, "mtime": now()}
            self.send_json({"ok": True, "path": path})
            return

        if parsed.path == "/api/playback":
            payload = self.body_json() or {}
            action = str(payload.get("action", ""))
            playback = STATE["playback"]
            if action == "play":
                path = normalise(str(payload.get("path", "")))
                if path not in FILES or FILES[path]["is_dir"]:
                    self.send_json({"error": "not_found"}, 404)
                    return
                playback.update(
                    {"state": "playing", "title": posixpath.basename(path), "pos": 0,
                     "dur": 210}
                )
            elif action == "pause":
                playback["state"] = "paused"
            elif action == "resume":
                playback["state"] = "playing"
            elif action == "volume":
                pass
            elif action == "seek":
                playback["pos"] = max(0, int(payload.get("value", 0)))
            elif action in {"next", "prev", "stop"}:
                playback["state"] = "stopped" if action == "stop" else playback["state"]
            else:
                self.send_json({"error": "bad_request"}, 400)
                return
            self.send_json({"ok": True, "playback": playback})
            return

        if parsed.path == "/api/wifi":
            payload = self.body_json() or {}
            ssid = str(payload.get("ssid", "")).strip()
            mode = str(payload.get("mode", "sta"))
            if not ssid or mode not in {"sta", "ap"}:
                self.send_json({"error": "bad_request"}, 400)
                return
            STATE["wifi"].update({"ssid": ssid, "mode": mode})
            STATE["logs"].append(f"[mock] wifi saved: ssid={ssid} mode={mode}")
            self.send_json({"ok": True})
            return

        self.send_json({"error": "not_found"}, 404)

    # -- DELETE ----------------------------------------------------------
    def do_DELETE(self):
        parsed = urllib.parse.urlparse(self.path)
        if parsed.path != "/api/file":
            self.send_json({"error": "not_found"}, 404)
            return
        if not self.authorised():
            self.send_json({"error": "unauthorized"}, 401)
            return
        path = normalise((self.query().get("path") or ["/"])[0])
        if path not in FILES or path == "/":
            self.send_json({"error": "not_found"}, 404)
            return
        meta = FILES[path]
        if meta["is_dir"] and self.list_items(path):
            self.send_json({"error": "not_empty"}, 409)
            return
        del FILES[path]
        STATE["logs"].append(f"[mock] deleted {path}")
        self.send_json({"ok": True})


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8080)
    args = parser.parse_args()

    if not ROOT.is_dir():
        print(f"error: web root {ROOT} not found", flush=True)
        return 1

    server = ThreadingHTTPServer((args.host, args.port), Handler)
    print(f"i_psb mock: http://{args.host}:{args.port} (root={ROOT})", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
