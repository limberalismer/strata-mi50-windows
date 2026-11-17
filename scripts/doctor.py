"""Read-only installation checks; optional small HIP probe isolated in a subprocess."""
import argparse
import ctypes
import json
import os
from pathlib import Path
import subprocess
import sys

from setup import ROOT, read, sha


def probe(gpu):
    handles = [os.add_dll_directory(str(ROOT / "runtime"))]
    hip = ctypes.WinDLL(str(ROOT / "runtime/amdhip64_7.dll"))
    n = ctypes.c_int()
    def check(code, step):
        if code:
            raise RuntimeError(f"{step}: HIP status {code}")
    check(hip.hipGetDeviceCount(ctypes.byref(n)), "hipGetDeviceCount")
    if n.value <= gpu:
        raise RuntimeError(f"GPU {gpu} is not available; HIP enumerated {n.value}")
    check(hip.hipSetDevice(gpu), "hipSetDevice")
    # Oversized output buffer avoids relying on Python's reconstruction of the HIP property ABI.
    props = ctypes.create_string_buffer(8192)
    check(hip.hipGetDevicePropertiesR0600(props, gpu), "hipGetDevicePropertiesR0600")
    if b"gfx906" not in props.raw:
        raise RuntimeError("Selected GPU does not report gfx906; this engine is wave64-only")
    blas = ctypes.WinDLL(str(ROOT / "runtime/hipblas.dll"))
    h = ctypes.c_void_p()
    check(blas.hipblasCreate(ctypes.byref(h)), "hipblasCreate")
    check(blas.hipblasDestroy(h), "hipblasDestroy")
    subprocess.run([sys.executable, str(ROOT / "scripts/blas_probe.py"), str(gpu)], check=True, timeout=60)
    print(f"HIP enumerated {n.value} GPU(s); selected gfx906 GPU {gpu}; numeric hipBLAS checks passed. This is not full model validation.")


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--gpu-probe", action="store_true")
    p.add_argument("--probe-child", type=int, help=argparse.SUPPRESS)
    args = p.parse_args()
    if args.probe_child is not None:
        probe(args.probe_child)
        return
    errors = []
    pins = read(ROOT / "configs/runtime-files.json")
    for name, pin in pins.items():
        if not name.startswith("runtime/"):
            continue
        f = ROOT / name
        if not f.is_file() or sha(f) != pin["sha256"]:
            errors.append("Missing/wrong runtime component: " + name)
    system_hip = Path(os.environ.get("SystemRoot", "C:/Windows")) / "System32/amdhip64.dll"
    print("System HIP:", system_hip, "exists:", system_hip.is_file())
    if not system_hip.is_file():
        errors.append("Install the documented display driver; the bundled shims need the system HIP runtime")
    cfg_path = ROOT / "local/config.json"
    cfg = read(cfg_path) if cfg_path.is_file() else None
    if not cfg:
        errors.append("Missing local/config.json; run setup.ps1")
    else:
        for k in ["exe", "cwd", "tokenizer"]:
            if not Path(cfg[k]).exists():
                errors.append("Missing " + k + ": " + cfg[k])
        av = cfg["args"]
        for key in ["--pack", "--native", "--expert-profile", "--mtp", "--draft-model"]:
            if not Path(av[av.index(key) + 1]).exists():
                errors.append("Missing path for " + key)
        print("Prepared GPU", cfg["gpu"], "port", cfg["port"], "context", av[av.index("--max-context") + 1])
    print("Runtime integrity:", "FAILED" if errors else "PASS")
    for error in errors:
        print("ERROR:", error)
    if errors:
        raise SystemExit(1)
    if args.gpu_probe:
        env = os.environ.copy()
        tmp = ROOT / "local/hip-tmp"
        tmp.mkdir(parents=True, exist_ok=True)
        env.update({k: str(tmp) for k in ["TEMP", "TMP", "TMPDIR"]})
        result = subprocess.run([sys.executable, __file__, "--probe-child", str(cfg["gpu"])], env=env, timeout=90)
        if result.returncode:
            raise SystemExit(f"Probe failed ({result.returncode}); see docs/TROUBLESHOOTING.md")


if __name__ == "__main__":
    main()
