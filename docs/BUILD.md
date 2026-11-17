# 对应源码与重建

正常使用预编译运行包。这里用于审计和修改，不承诺新编译文件与已验收二进制逐字节相同。

## 来源

- `vendor/strata/src`、`include`、`CMakeLists.txt` 等：用于 P28 及后续 MI50 工作的本地 Strata-mi50-int6 源码树当前快照，包含 MI50/GFX906 优化和 Windows 适配。缺少 Git 历史，无法证明每个文件都与原二进制构建时完全一致；本发布分别固定原已验证二进制哈希与当前源码摘要，不宣称逐字节可复现构建。
- `vendor/strata/serve`、`strata_control`、`strata_cli`：生产运行中的 Strata-mi50 Python 服务源码快照。两棵本地树均无 `.git` 历史，不能伪造精确上游 commit；完整文件摘要在 `evidence/source-sha256.json`。
- `vendor/strata/third_party/llama.cpp`：实际构建使用的本地 ggml、gguf-py 和 cmake 文件；上游基准 pin `3cf03257f219afbe7334045ff7c6a06ac68c627d`，保留本地版本，而不是构建时隐式切换最新版。
- `scripts/hip57_shim.cpp`：对应固定 24 GiB 连续注册上限的 shim 源码。
- `scripts/hipblas57_adapter.cpp`：hipBLAS 新旧数据类型转换及转发源码。

Strata 和这些第三方组件各自保留 LICENSE。完整应用/桌面壳和无关后端资源不在本部署快照的构建目标中。构建只指定 `strata`，不是整个上游的全部目标。

## 前置依赖

Visual Studio 2022 Build Tools 的 C++ x64 workload / Windows SDK，Python x64，CMake，Ninja。TheRock 四个 Windows wheels：`rocm_sdk_core`、`rocm_sdk_devel`、`rocm_sdk_libraries`、`rocm_sdk_device_gfx906`，固定 `10.3.0a20261111`。

原来源为 [AMD nightly wheel index](https://nightly.repo.amd.com/rocm/whl-next/)。夜间文件可能过期，优先保留原 wheel 文件并校验；若已无法获取该版本，应明确报告重建受阻，不以新版编译结果冒充原发布。发布预编译运行包不依赖 wheel 仍在线。

原四个 wheel 的文件名、大小与发布前实际计算的 SHA-256 在 [configs/toolchain.json](../configs/toolchain.json)。源码摘要不包含模型 GGUF（包括上游的小实验 GGUF fixture）、缓存或 Python 字节码。

新建独立编译 venv 并安装工具链：

```powershell
py -3 -m venv build\rocm-venv
build\rocm-venv\Scripts\python.exe -m pip install --index-url https://nightly.repo.amd.com/rocm/whl-next/ 'rocm[libraries,devel,device-gfx906]==10.3.0a20261111'
build\rocm-venv\Scripts\rocm-sdk.exe init
```

若使用离线 wheels，先 `pip install D:\Wheels\rocm_sdk_*.whl`（PowerShell 下逐个枚举传给 pip），再安装同版本 `rocm[...]` 元包。编译器路径应指向安装后的 `_rocm_sdk_devel/lib/llvm/bin/clang++.exe`，不是安装机器上的绝对旧路径。

在 VS x64 Native Tools 命令行调用：

```bat
scripts\build-engine.bat
```

脚本从本仓库默认 `build/rocm-venv` 发现工具链，可设置 `MI50_ROCM_DEVEL` 覆盖。输出在 `build/engine`，**不会替换 runtime/**。需在 VS x64 命令行运行，因为它需要 MSVC 头文件、链接器和 SDK。

核心配置：`STRATA_HIP_GFX906=ON`、`STRATA_ENABLE_HIP=OFF`、`STRATA_ENABLE_CUDA=OFF`、`CMAKE_HIP_ARCHITECTURES=gfx906`、`STRATA_PORTABLE=ON`、`STRATA_NATIVE_EXPERTS=ON`、`STRATA_MMQ_KQUANTS=ON`、HIP flags 中 `-mcode-object-version=4`。gfx906 是独立兼容后端，不能简单打开 wave32 的 STRATA_ENABLE_HIP。

显式指定 `CMAKE_HIP_FLAGS_RELEASE=-O3 -DNDEBUG`，与原有成功构建一致。本次干净配置发现 CMake 的 HIP Release flags 默认为空，导致 GDN 的一个设备函数出现 LLVM `illegal VGPR to SGPR copy` 错误；不能依赖旧构建缓存替你保留优化参数。

## 重建 ABI 兼容库

同样使用 VS x64 命令行与 TheRock clang++：

```bat
scripts\build-adapters.bat
```

输出到 `build/adapters`。适配器的 hipblas57.dll 是原厂 SDK 文件，不是从此 C++ 编译出来的；使用 Release 中原文件及许可证，或从上述 SDK 5.7.1 获取相同哈希文件。

## 验证后再替换

先检查 PE imports 和嵌入设备代码，再测设备枚举、HIP/BLAS 数值、全部主机注册页读写、专家算子逐位比较、KV streaming/window parity、完整服务加载与真实输出。保留旧运行目录和哈希，对候选建立单独配置和端口；受内存限制时顺序运行，不能同时加载两个 41 GiB arena。

更改归约/累加或分派时增加 logits 和多种任务质量检查；不得以六条简单问答代表完整能力保证。性能比较固定权重、实际参数、全新输入、输出长度和预热，排除 KV overflow。本项目未对 CUDA、wave32 HIP、SYCL 或其他 GPU 的新改动做构建验收。
