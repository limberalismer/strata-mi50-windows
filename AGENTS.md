# Agent deployment entry point

This repository is a Windows-native MI50/gfx906 deployment kit, not a generic Strata checkout. Read README.md, docs/DRIVERS.md, docs/TROUBLESHOOTING.md and docs/VALIDATION.md first. The user's current instructions take precedence. Do the authorized preparation and reversible checks without asking redundant questions.

## Deployment workflow

1. Inspect Windows x64, available GPU ordinal, RAM, free disk, installed Python and the existing display driver. Target baseline: MI50 16 GiB, 64 GiB RAM, AVX2 CPU. Do not substitute another quantization or shorten context to fit silently.
2. Check system `amdhip64.dll` and driver. Preserve an already working installation. Driver replacement, reboot and firmware changes are not part of setup.ps1. If needed and not already authorized, identify the exact installer and effects for the user. Never flash VBIOS, disable signature enforcement or replace System32 DLLs as a shortcut.
3. Prefer the committed, hash-pinned runtime. Fetch the v1.0.0 runtime ZIP through configs/downloads.json; verify archive and member hashes. Install VC++ runtime if missing. Do not add an unrelated amdhip64.dll next to the application.
4. Use the two standard IQ3_S GGUF shards in configs/models.json. Reuse verified local files, otherwise use `setup.ps1 -ModelDir <SSD path> -DownloadModel`. This is about 83.6 GB. Need space for another ~52 GB pack. Respect any user download/storage constraints.
5. Reuse only a complete pack for these exact shards. Otherwise run the bundled iq_pack.py through setup. It preserves the main model's quantized expert bytes. Use the exact Strata tuned DFlash2 pin, not an arbitrary stock/tuned/BF16-selector file.
6. Run doctor.py, then doctor.py --gpu-probe. Set a free port and correct GPU using setup arguments. Use run.bat; it sets a dedicated writable HIP TEMP/TMP/TMPDIR, then starts the production server with local/config.json.
7. Wait for actual model readiness. Run scripts/smoke.py against that port and check engine.log for errors, nonfinite and KV OVERFLOW. Basic arithmetic is a smoke check, not a full quality guarantee.
8. Report exact entry point/API URL, component hashes, passed/failed/not-run checks and remaining limitations. Never claim a benchmark you did not run.

## Important invariants

- Native Windows, no ZLUDA. New TheRock compiler + old system HIP 5.7-line runtime + HIP SDK 5.7.1 BLAS + the two ABI adapters. Compiler emits code object v4. Do not mix DLL generations.
- Fixed 24 GiB contiguous registration cap in scripts/hip57_shim.cpp. Never resurrect the incorrect fragmented-register/address-arithmetic implementation.
- Default is 262144 capacity, INT8 KV, 32768 KV resident, 4096 prefill chunk, full expert route and DFlash2 target verification. KV overflow is a failure even if an answer looks plausible.
- Do not kill existing inference servers, change other GPU projects, overwrite production configuration, or claim a port collision is solved by terminating an unrelated process.
- Record actual process arguments, not just saved configuration. Preserve user prompts/history; do not publish private logs. New timings must distinguish fresh versus reused input and short versus long output.
- Current real performance is in README.md. 80–90 decode and 1000+ prefill have not been attained under these requirements.
- Upstream source is in vendor/strata. It is a historical snapshot with local MI50 modifications, not current upstream main. Do not run its generic updater. Build instructions are in docs/BUILD.md.
- Root deployment scripts are maintained separately from vendored code. Do not use the experimental source rebuild to overwrite the verified runtime before separate validation.

## Publication hygiene

Do not commit local/, downloads/, .venv/, logs/, model GGUFs, generated packs, device-login tokens, crash dumps, user prompts/outputs, proprietary display drivers or machine-specific config. Proprietary driver download links and hashes belong in configs/drivers.json. Keep existing third-party licenses and runtime hashes.
