"""Launch only this repository's prepared configuration, without global PATH changes."""
import json
import os
from pathlib import Path
import socket
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
config = ROOT / "local/config.json"
if not config.is_file():
    raise SystemExit("Run setup.ps1 first; see README.md")
cfg = json.loads(config.read_text(encoding="utf-8"))
with socket.socket() as s:
    try:
        s.bind(("127.0.0.1", int(cfg["port"])))
    except OSError:
        raise SystemExit(f"Port {cfg['port']} is occupied. Keep the existing service or prepare a different --port; no process was stopped.")
env = os.environ.copy()
tmp = ROOT / "local/hip-tmp"
tmp.mkdir(parents=True, exist_ok=True)
for key in ["TEMP", "TMP", "TMPDIR"]:
    env[key] = str(tmp)
env["PYTHONUTF8"] = "1"
raise SystemExit(subprocess.call([sys.executable, str(ROOT / "vendor/strata/serve/server.py"),
                                  "--engine", "strata", "--config", str(config),
                                  "--host", "127.0.0.1", "--port", str(cfg["port"])], cwd=ROOT, env=env))
