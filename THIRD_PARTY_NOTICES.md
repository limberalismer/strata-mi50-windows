# Third-party notices and provenance

This project contains a modified local snapshot of [Strata](https://github.com/Niko1221/Strata), MIT, Copyright 2026 Niko1221 and contributors. Original notice: vendor/strata/LICENSE and licenses/Strata-LICENSE. Local deployment scripts and documentation are MIT under LICENSE. No claim of upstream endorsement is made.

The engine uses [llama.cpp / ggml](https://github.com/ggml-org/llama.cpp), MIT, base revision `3cf03257f219afbe7334045ff7c6a06ac68c627d`, with the exact local source snapshot retained. Notices are preserved in vendor/strata/third_party/llama.cpp/LICENSE and licenses/llama.cpp-LICENSE. Strata's additional ggml/repack code retains notices in the corresponding vendored files.

The runtime release includes AMD hipBLAS and rocBLAS 5.7.1 binary files and gfx906 device-library data from the original HIP SDK 23.Q4 package. They are distributed with their original licenses in licenses/hipBLAS-LICENSE.md and licenses/rocBLAS-LICENSE.md, including the latter's additional third-party notices. Original package download and hash: configs/drivers.json, docs/DRIVERS.md. No proprietary display-driver installer, Windows system HIP DLL, Microsoft C++ runtime, VBIOS or driver-store backup is redistributed here.

The model weights are **not** included. The standard ISTA-DASLab Qwen3.8-Flash-Next GSQ-RCO IQ3_S model, its tokenizer, chat template, and any resulting local pack are governed by the model provider's terms. Main-model quantized expert bytes are not changed by this project; see configs/models.json for links and pins.

The optional downloaded DFlash2 tuned pack is Apache-2.0, modified and published by Strata from Inco AI's DFlash2 model. Attribution, description of modified draft weights and the original Apache license are retained in licenses/dflash2-tuned/. The original main Qwen model and this tuned draft model are distinct. This repository links the upstream release; it does not host another copy of those weights.

The committed draft vocabulary and routing profile are Strata runtime metadata, retained from the compatible production installation. They contain token IDs and expert ranks, not prompts, completions or model matrix weights. Their file hashes are recorded in configs/metadata-sha256.json. See upstream Strata's license.

The Python server web assets include vendored components such as KaTeX; their original license files are retained under vendor/strata/serve/web/vendor/. Python package dependencies are installed from their own distributions under their respective licenses, not copied into this Git repository.

The source trees were provided as local directories without .git history. We therefore publish a source file hash inventory and describe their provenance honestly rather than inventing an upstream commit for the modified snapshot. Known upstream base identifiers are documented separately in evidence/provenance.json.
