"""Prepare a relocatable, hash-checked MI50 installation. Never installs drivers."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "vendor" / "strata"


def sha(path):
    with Path(path).open("rb") as f:
        return hashlib.file_digest(f, "sha256").hexdigest()


def read(path):
    return json.loads(Path(path).read_text(encoding="utf-8"))


def download(pin, target):
    """Resume large public assets; publish the file only after size and SHA checks."""
    target = Path(target)
    target.parent.mkdir(parents=True, exist_ok=True)
    if target.is_file() and target.stat().st_size == pin["size"] and sha(target) == pin["sha256"]:
        print("Verified existing", target)
        return target
    part = target.with_name(target.name + ".part")
    offset = part.stat().st_size if part.exists() else 0
    if offset > pin["size"]:
        raise RuntimeError(f"Partial download is too large: {part}; move it aside and retry")
    if offset < pin["size"]:
        headers = {"User-Agent": "strata-mi50-windows/1.0"}
        if offset:
            headers["Range"] = f"bytes={offset}-"
        print("Downloading", pin["url"], flush=True)
        with urllib.request.urlopen(urllib.request.Request(pin["url"], headers=headers), timeout=120) as response:
            if offset and response.status != 206:
                offset = 0
            if offset:
                content_range = response.headers.get("Content-Range", "")
                if not content_range.startswith(f"bytes {offset}-"):
                    raise RuntimeError("Server returned an unexpected download range")
            with part.open("ab" if offset else "wb") as out:
                shutil.copyfileobj(response, out, length=8 << 20)
    if part.stat().st_size != pin["size"] or sha(part) != pin["sha256"]:
        raise RuntimeError(f"Size/SHA-256 mismatch: {part}; no unverified file was installed")
    part.replace(target)
    return target


def runtime_ok():
    pins = read(ROOT / "configs/runtime-files.json")
    return all((ROOT / name).is_file() and sha(ROOT / name) == p["sha256"]
               for name, p in pins.items() if name.startswith("runtime/"))


def install_runtime(archive):
    pins = read(ROOT / "configs/runtime-files.json")
    with zipfile.ZipFile(archive) as z:
        # Install only the explicit manifest allowlist, never arbitrary archive paths.
        for name, pin in pins.items():
            info = z.getinfo(name)
            if info.file_size != pin["size"]:
                raise RuntimeError(f"Wrong archive member size: {name}")
            target = ROOT / name
            target.parent.mkdir(parents=True, exist_ok=True)
            tmp = target.with_name(target.name + ".installing")
            with z.open(info) as src, tmp.open("wb") as out:
                shutil.copyfileobj(src, out, length=8 << 20)
            if sha(tmp) != pin["sha256"]:
                raise RuntimeError(f"Wrong archive member hash: {name}")
            tmp.replace(target)


def prepare(args):
    for name, expected in read(ROOT / "configs/metadata-sha256.json").items():
        if not (ROOT / name).is_file() or sha(ROOT / name) != expected:
            raise RuntimeError(f"Runtime metadata SHA-256 mismatch: {name}")
    pins = read(ROOT / "configs/downloads.json")
    if not runtime_ok():
        archive = args.runtime_archive
        if archive is None:
            if not args.fetch_runtime:
                raise RuntimeError("Runtime incomplete: pass --fetch-runtime or --runtime-archive <verified zip>")
            archive = download(pins["runtime"], ROOT / "downloads/runtime.zip")
        if sha(archive) != pins["runtime"]["sha256"]:
            raise RuntimeError("Runtime archive SHA-256 does not match the release pin")
        install_runtime(archive)
    model_dir = args.model_dir.resolve()
    models = read(ROOT / "configs/models.json")
    for pin in models["files"]:
        path = model_dir / pin["name"]
        if args.download_model:
            download(pin, path)
        if not path.is_file() or path.stat().st_size != pin["size"]:
            raise RuntimeError(f"Missing or truncated standard model: {path}")
        if not args.skip_model_hash and sha(path) != pin["sha256"]:
            raise RuntimeError(f"Model SHA-256 mismatch: {path}")
    first = model_dir / models["files"][0]["name"]
    pack = args.pack_dir.resolve() if args.pack_dir else ROOT / "data/pack/iq3_s"
    needed = ["index.txt", "dense.bin", "native_experts.txt", "experts.bin", "tokenizer/vocab.json", "tokenizer/chat_template.jinja"]
    if args.repack or not all((pack / n).is_file() for n in needed):
        if pack.exists() and any(pack.iterdir()) and not args.repack:
            raise RuntimeError("Pack directory is incomplete. Use a fresh directory or explicitly pass --repack")
        pack.mkdir(parents=True, exist_ok=True)
        if shutil.disk_usage(pack).free < 55 * 2**30:
            raise RuntimeError("Need at least 55 GiB free for the native expert pack")
        subprocess.run([sys.executable, str(SOURCE / "tools/iq_pack.py"), "--gguf", str(first),
                        "--out", str(pack), "--experts-bin"], cwd=SOURCE, check=True)
    source_meta = read(pack / "experts.bin.src.json")
    if source_meta["shards"] != [{"name": p["name"], "size": p["size"]} for p in models["files"]]:
        raise RuntimeError("Existing pack belongs to another model; select a fresh pack directory")
    if (pack / "experts.bin").stat().st_size != 50292326400:
        raise RuntimeError("Experts file size is wrong for this IQ3_S preset")
    draft = args.draft.resolve() if args.draft else ROOT / "data/dflash2.strata1.pack"
    if not draft.is_file() and args.fetch_draft:
        download(pins["draft"], draft)
    if not draft.is_file() or sha(draft) != pins["draft"]["sha256"]:
        raise RuntimeError("Missing/wrong DFlash2 pack: pass --fetch-draft or --draft <verified file>")
    cfg = read(ROOT / "configs/preset.json")
    replacements = {
        "${ROOT}": str(ROOT), "${PACK}": str(pack), "${MODEL}": str(first),
        "${DRAFT}": str(draft), "${TOKENIZER}": str(pack / "tokenizer"),
    }
    def expand(value):
        if isinstance(value, str):
            for key, replacement in replacements.items():
                value = value.replace(key, replacement)
            return value
        if isinstance(value, list):
            return [expand(v) for v in value]
        if isinstance(value, dict):
            return {k: expand(v) for k, v in value.items()}
        return value
    cfg = expand(cfg)
    cfg["port"] = args.port
    cfg["gpu"] = args.gpu
    (ROOT / "local").mkdir(exist_ok=True)
    (ROOT / "logs").mkdir(exist_ok=True)
    (ROOT / "local/config.json").write_text(json.dumps(cfg, indent=2), encoding="utf-8")
    print("Prepared local/config.json; next: py scripts/doctor.py --gpu-probe, then run.bat")


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--model-dir", type=Path, required=True, help="directory containing the two standard IQ3_S GGUF shards")
    p.add_argument("--pack-dir", type=Path, help="reuse a complete native IQ3_S pack, otherwise create data/pack/iq3_s")
    p.add_argument("--draft", type=Path, help="existing Strata tuned selector-Q4 DFlash2 pack")
    p.add_argument("--runtime-archive", type=Path)
    p.add_argument("--fetch-runtime", action="store_true")
    p.add_argument("--download-model", action="store_true", help="download about 83.6 GB of standard GGUF weights")
    p.add_argument("--fetch-draft", action="store_true", help="download the 339.7 MB upstream tuned drafter")
    p.add_argument("--skip-model-hash", action="store_true", help="reuse model files already verified independently; size is still checked")
    p.add_argument("--repack", action="store_true")
    p.add_argument("--port", type=int, default=8081)
    p.add_argument("--gpu", type=int, default=0)
    args = p.parse_args()
    if sys.version_info < (3, 11) or sys.maxsize < 2**32:
        p.error("Install Python 3.11+ x64")
    if os.name != "nt":
        p.error("This release is for native Windows x64; Linux needs a separate build")
    if not 1 <= args.port <= 65535 or args.gpu < 0:
        p.error("Invalid port or GPU ordinal")
    prepare(args)


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, OSError, subprocess.CalledProcessError) as exc:
        raise SystemExit(str(exc))
