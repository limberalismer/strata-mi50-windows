"""Deployment regression checks; no GPU/model downloads or system changes."""
import contextlib
import hashlib
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import importlib.util
import json
from pathlib import Path
import tempfile
import threading
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("deployment_setup", ROOT / "scripts/setup.py")
setup = importlib.util.module_from_spec(spec)
spec.loader.exec_module(setup)


@contextlib.contextmanager
def endpoint(payload, range_mode):
    class Handler(BaseHTTPRequestHandler):
        def do_GET(self):
            start = int(self.headers.get("Range", "bytes=0-").split("=")[1].split("-")[0])
            if range_mode == "ignore":
                start = 0
            self.send_response(206 if start else 200)
            if start:
                self.send_header("Content-Range", f"bytes {start if range_mode != 'wrong' else start+1}-{len(payload)-1}/{len(payload)}")
            self.end_headers()
            self.wfile.write(payload[start:])
        def log_message(self, *_):
            pass
    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        yield f"http://127.0.0.1:{server.server_port}/asset"
    finally:
        server.shutdown()
        server.server_close()
        thread.join()


class DeploymentTests(unittest.TestCase):
    def test_resume_and_ignored_range(self):
        payload = b"known-byte-content" * 1000
        for mode in ["resume", "ignore"]:
            with self.subTest(mode=mode), tempfile.TemporaryDirectory() as temp, endpoint(payload, mode) as url:
                p = Path(temp) / "file"
                p.with_suffix(".part").write_bytes(payload[:100])
                setup.download({"url": url, "size": len(payload), "sha256": hashlib.sha256(payload).hexdigest()}, p)
                self.assertEqual(p.read_bytes(), payload)

    def test_wrong_hash_never_publishes(self):
        with tempfile.TemporaryDirectory() as temp, endpoint(b"bad bytes", "ignore") as url:
            p = Path(temp) / "file"
            with self.assertRaises(RuntimeError):
                setup.download({"url": url, "size": 9, "sha256": "0" * 64}, p)
            self.assertFalse(p.exists())

    def test_wrong_resume_range_rejected(self):
        payload = b"sample bytes"
        with tempfile.TemporaryDirectory() as temp, endpoint(payload, "wrong") as url:
            p = Path(temp) / "file"
            p.with_suffix(".part").write_bytes(payload[:2])
            with self.assertRaises(RuntimeError):
                setup.download({"url": url, "size": len(payload), "sha256": hashlib.sha256(payload).hexdigest()}, p)
            self.assertFalse(p.exists())

    def test_pins_and_paths(self):
        pins = setup.read(ROOT / "configs/runtime-files.json")
        for name in pins:
            self.assertFalse(Path(name).is_absolute())
            self.assertNotIn("..", Path(name).parts)
        preset = setup.read(ROOT / "configs/preset.json")
        args = preset["args"]
        for key, value in [("--max-context", "262144"), ("--kv-resident", "32768"), ("--prefill", "4096")]:
            self.assertEqual(args[args.index(key)+1], value)
        self.assertEqual(preset["env"]["STRATA_ROUTE_TAIL_SKIP"], "0")
        self.assertNotIn("C:\\Users\\", json.dumps(preset))
        for name, expected in setup.read(ROOT / "configs/metadata-sha256.json").items():
            self.assertEqual(setup.sha(ROOT / name), expected)


if __name__ == "__main__":
    unittest.main()
