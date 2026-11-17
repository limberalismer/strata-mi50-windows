# Strata on MI50 — Windows 原生推理实测与部署

在 **Windows 10 + AMD Instinct MI50 16 GiB（gfx906）** 上，通过原生 HIP 跑通 Strata / Qwen3.8-Flash-Next IQ3_S：提供已验证引擎、HIP/hipBLAS ABI 兼容层、完整对应源码快照、固定版本下载清单、自动生成本机配置的脚本，以及实际性能和失败经验。

**无 ZLUDA，不需要 WSL/Linux。** 本项目是第三方实验适配，不是 AMD 官方支持，也不是 Strata 官方 Windows gfx906 发行版。

实测机器：Windows 10 x64、MI50 16 GiB（驱动显示 Radeon Pro VII，`gfx906:sramecc-:xnack-`）、i5-9400F 六核 AVX2、64 GiB RAM、PCIe 3.0 x16。主模型与量化没有修改。默认单请求，DFlash2，262144-token 上限，INT8 KV，32K KV 显存驻留。

> 想把仓库交给 agent？复制以下整段即可。它会先读根目录 [AGENTS.md](AGENTS.md)，识别依赖，再按真实验证结果报告。
>
> 请部署 https://github.com/limberalismer/strata-mi50-windows 。先阅读 README.md、AGENTS.md 和 docs/DRIVERS.md。检测我的 Windows、GPU、RAM、磁盘和现有驱动；保留已有可用环境。优先使用仓库中哈希固定的已验证引擎，获取 Release 的运行库，用标准 IQ3_S GGUF 构建或复用 native pack，使用固定版本 Strata tuned DFlash2。生成本机路径配置，运行 doctor 和真实 API 冒烟验证。遇到需要安装/替换系统显卡驱动或重启时，先说明具体原因；不要自动刷 BIOS、禁用驱动签名或覆盖 System32 DLL。不要把设备枚举成功当成整模型成功，也不要跳过失败检查。最后告诉我启动方式、API 地址、通过和未通过的验证。

## 实际性能：不要只看最高分

下表是本机已有记录，**不是其他机器的性能保证**。日期、工作负载不同的行不能当作受控 A/B。prefill 仅统计新读 token，不把缓存命中 token 算进吞吐。

| 测试 | Prefill（tok/s） | Decode（tok/s） | 说明 |
| --- | ---: | ---: | --- |
| P28 部署复测，107 输入 / 512 输出，3 次 | 短输入不用于长 prefill 评价 | **40.3 / 43.1 / 41.8** | 短代码专项，中位 41.8；每组预热后串行测量，无 prompt cache 复用 |
| 后续 P29 同配置短代码对照，3 次 | 同上 | 中位 **39.4** | 短代码应按约 39–42 理解 |
| 65,479 全新输入 | **470.1** | 28.5 | 输出仅 46 token，前/中/后控制码全部召回，无 KV overflow |
| 261,847 全新输入 | **415.0** | 28.8 | 输出仅 49 token，完整读取，三点召回，无 KV overflow |
| 2026-11-17 当前会话中 6 条新增输入至少 4K 的请求 | 中位 **413.45**，371.8–471.9 | — | 真实异构请求，非受控基准 |
| 截至 2026-11-17 的最近 20 条长输入/长输出请求 | 其中 14 条新增输入至少 4K：中位 **404.35** | 中位 **24.85**，22.1–32.8 | 输入至少 8192、输出至少 128；汇总 decode 24.03；draft 接受约 50.31% |

原始数值摘录：[evidence/](evidence/)，方法和限制见 [docs/PERFORMANCE.md](docs/PERFORMANCE.md)。未上传用户提示、回复或服务完整日志。

**没有达到 decode 80–90 或 prefill 1000+。** 旧实验约 770 tok/s 的 prefill 存在 KV overflow，不作为能力完整的验收成绩。不能通过缩小上下文、漏读输入、跳过专家或关闭目标模型验证来“复现”高分。

## 为什么这套组合能工作

```text
runtime/strata.exe（TheRock 10.3 编译，gfx906，code object v4）
 ├─ amdhip64_7.dll：本项目兼容层
 │   └─ Windows System32/amdhip64.dll：驱动自带的 HIP 5.7 系运行库
 └─ hipblas.dll：本项目 ABI 适配器
     └─ hipblas57.dll + rocblas.dll + rocblas/library/*gfx906*
        └─ HIP SDK 5.7.1 / 23.Q4 For-HIP 的 BLAS 与 gfx906 内核
```

新工具链能编译 gfx906，不等于其新版 Windows runtime 能枚举 MI50。这里保留已验证的旧运行链路，转换属性结构、GEMM datatype 和旧 API，使用 v4 设备代码。连续内存注册经过修复并限制为 24 GiB；旧的“分成多个 8 GiB 注册后假设 GPU 地址连续”的实现不可使用。

这是一份有完整模型加载、API 生成与长上下文证据的 Windows MI50 案例。不能证明“全球唯一”：更早的 [Strata PR #1730](https://github.com/Niko1221/Strata/pull/1730) 已报告 Windows HIP 5.7 编译和算子验证，但明确未声称该二进制完成整模型推理或性能 A/B。本项目的端到端记录与它的编译验证应区分。

## 1. 硬件和系统要求

- **Windows x64 + gfx906 单卡**。本发布只实测上述 MI50 16 GiB；MI60、其他 VBIOS、32GB 卡、其他驱动、Windows 11 均未在本机验收。
- **64 GiB RAM** 是当前预设的实测配置。约 41 GiB 专家常驻 RAM，另有 KV、副本、Python 和系统开销。不应在 32GB RAM 上照搬 42 GiB 预算。
- 建议至少 **160 GiB 空闲 SSD**：标准 GGUF 合计约 83.6 GB、native pack 约 52 GB，另有下载与环境。源码重建另留 10–20 GiB。
- Python **3.11+ x64**（含 `py` launcher，发布流程实测 3.13.6）、Git、[Visual C++ x64 Runtime](https://aka.ms/vs/17/release/vc_redist.x64.exe)。使用预编译引擎无需 Visual Studio 或完整 ROCm SDK。
- 有效系统显卡驱动及其 `System32/amdhip64.dll`。本机实测 Adrenalin 25.3.1。详见下一节。
- 散热与供电按你的实际 MI50 卡和整机确认；仓库不提供刷 BIOS 或超频流程。

## 2. 驱动与运行文件

完整步骤、固定文件 SHA-256 和回退原则见 **[docs/DRIVERS.md](docs/DRIVERS.md)**。

1. 优先保留已有能正确枚举 gfx906 的驱动。不要仅因版本新就更换驱动。
2. 需要复现本机时，从 AMD 原厂获取 25.3.1 安装包，核对 `configs/drivers.json` 中的 SHA-256。**驱动安装与重启需要用户操作/授权，setup 不会静默替换系统驱动。**
3. 本仓库 Git 内已有已验证 `strata.exe`、两层 ABI 兼容 DLL 和 `hipblas57.dll`。`setup.ps1` 再下载 [v1.0.0 Release](https://github.com/limberalismer/strata-mi50-windows/releases/tag/v1.0.0) 的约 54 MiB 运行包，补齐 `rocblas.dll` 和 gfx906 设备库，逐文件校验后安装到 `runtime/`。
4. 不把其他版本的 `amdhip64.dll` 放进 `runtime/`。引擎与 BLAS 必须解析到同一个已验证的系统 HIP runtime。不要把仓库 DLL 覆盖到 System32。

Release 的 BLAS 文件来自 HIP SDK 5.7.1，附原有许可证。显示驱动不在仓库或 Release 中重新分发，提供原厂链接和校验方法。无网络时可在另一台机器下载 Release ZIP，传 `-RuntimeArchive`。

## 3. 标准模型：没有本项目特制权重

主模型使用 **[ISTA-DASLab/Qwen3.8-Flash-Next-GSQ-RCO-GGUF](https://huggingface.co/ISTA-DASLab/Qwen3.8-Flash-Next-GSQ-RCO-GGUF)**，固定 revision `ed59f92082b1e93c0e96d60a8b11aab089b52f09`，IQ3_S：

- [第 1 片，54,817,524,224 字节](https://huggingface.co/ISTA-DASLab/Qwen3.8-Flash-Next-GSQ-RCO-GGUF/resolve/ed59f92082b1e93c0e96d60a8b11aab089b52f09/Qwen3.8-Flash-Next-GSQ-RCO-IQ3_S-00001-of-00002.gguf)
- [第 2 片，28,800,138,432 字节](https://huggingface.co/ISTA-DASLab/Qwen3.8-Flash-Next-GSQ-RCO-GGUF/resolve/ed59f92082b1e93c0e96d60a8b11aab089b52f09/Qwen3.8-Flash-Next-GSQ-RCO-IQ3_S-00002-of-00002.gguf)

完整 SHA-256 在 [configs/models.json](configs/models.json)。主模型遵循模型仓库自己的许可；不上传 GGUF 权重。专家 pack 保留原量化字节，转换文件布局；tokenizer 和 chat template 从 GGUF 导出。

**DFlash2 是另一份草稿模型**：当前实测使用标准 Strata 微调版 `qwen3.8-flash-next-dflash2.strata1.q4_0-selq4.pack`，339,694,592 字节，SHA `4df2e5ab2b47c064afbe662e16e4f06569877125c2d0e2123a4ac755266c664c`。它不是本项目修改的主模型。setup 从 [Strata v0.2.12 Release](https://github.com/Niko1221/Strata/releases/download/v0.2.12/qwen3.8-flash-next-dflash2.strata1.q4_0-selq4.pack) 下载。来源和修改说明见 [原许可证/NOTICE](licenses/dflash2-tuned/NOTICE.md)。

## 4. 准备环境：两条路线

先在 PowerShell 执行：

```powershell
git clone https://github.com/limberalismer/strata-mi50-windows.git
cd strata-mi50-windows
```

**已有两个 GGUF，第一次构建 native pack：**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\setup.ps1 -ModelDir D:\Models\IQ3_S
```

这里只对本次脚本进程设置 ExecutionPolicy，不修改系统执行策略。下载运行包和 DFlash2；完整校验两个 GGUF；创建 `.venv`，构建 `data/pack/iq3_s`，导出 tokenizer，生成 `local/config.json`。大文件哈希和专家 pack 构建可能需要较长时间，请等待。

**尚无主模型，自动下载固定版本后打包：**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\setup.ps1 -ModelDir D:\Models\IQ3_S -DownloadModel
```

这会下载约 **83.6 GB** 主模型。脚本支持 `.part` 断点续传，校验通过后才发布最终文件；空间不足或校验失败会停止。

**已有同一个模型的完整 native pack、草稿文件和离线运行 ZIP：**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\setup.ps1 `
  -ModelDir D:\Models\IQ3_S -PackDir D:\StrataData\packs\iq3_s `
  -Draft D:\StrataData\dflash2.strata1.pack `
  -RuntimeArchive D:\Downloads\strata-mi50-windows-runtime-v1.0.0.zip
```

可选 `-Port 8082 -Gpu 1`。默认 GPU 0、localhost:8081。`-SkipModelHash` 仅供已经独立校验过 GGUF 的复用环境；默认检查完整哈希。不同量化、Swift/Unsloth 或其他 pack 不适用这一固定预设。

所有绝对路径只写进被 Git 忽略的 `local/config.json`；换目录后重跑 setup。仓库不依赖作者的 `C:\Users\...`、其他 GPU 项目目录或本机 Python 路径。

## 5. 自检、启动和实际生成验证

```powershell
.\.venv\Scripts\python.exe scripts\doctor.py --gpu-probe
.\run.bat
```

doctor 检查运行组件哈希、本机配置路径、系统 HIP，并可在独立子进程中执行小型 HIP/BLAS 检查。**它通过不代表整个模型一定能加载。**

保持启动窗口打开。第一次启动需要读取并准备约 41 GiB 专家 RAM 副本，通常几十秒到数分钟。正常日志应有 `resident RAM mode`、`session is up`、DFlash2 加载，以及合理的显存余量。

在另一个终端执行：

```powershell
.\.venv\Scripts\python.exe scripts\smoke.py
```

它检查 `/health` 的 loaded、262144 容量，并实际发送算术请求，要求回答 42。完整的 256K 能力还需另外检查长输入完整读取、KV 无 overflow 和召回，不能只看 `/health` 的容量字段。

- Web 聊天页：<http://127.0.0.1:8081/>
- OpenAI base URL：`http://127.0.0.1:8081/v1`
- Anthropic endpoint：`http://127.0.0.1:8081/v1/messages`
- 模型 ID：`qwen3.8-flash-next-gsq-rco-iq3_s-mi50`
- 日志：`logs/engine.log`；Ctrl+C 停止该启动窗口的服务。

默认只监听 localhost。远程访问需要明确配置认证和网络边界；不要直接把无认证服务暴露到公网。若端口已占用，启动器会退出，不会杀掉别的服务。

## 6. 默认预设与显存用途

预设在 [configs/preset.json](configs/preset.json)：`--prefill 4096 --spec 6 --max-context 262144 --kv int8 --kv-resident 32768 --resident-budget-gib 42 --pcie-frac 0.55 --draft dflash2 --draft-conf 0.35`。保留完整专家路由，禁用 tail skip。

16GB 显存不全是专家缓存：实测专家 8.07 GiB，其他投影权重约 2.59 GiB，其他权重约 266 MiB，主输出头约 497 MiB，DFlash2 约 625 MiB，以及 KV、GDN 状态、prefill/verify 工作区和运行库。捕获计算图后余量约 1.58 GiB。完整 256K INT8 KV 的 RAM 副本约 3.09 GiB，显存只缓存其中 32K。

不要盲目把 reserve 降到零或强行注册全部 RAM。旧 HIP 下安全性必须按全部页读写及真实推理验证，不以“API 返回成功”作依据。

## 7. 排错与重建

- 启动崩溃、HIP 枚举失败、DLL 混用、锁页、KV overflow、速度偏低：[docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md)
- 完整源码、工具链、兼容库和设备 code object 重建：[docs/BUILD.md](docs/BUILD.md)
- 性能口径与当前瓶颈：[docs/PERFORMANCE.md](docs/PERFORMANCE.md)
- 第三方来源和许可：[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)

不使用上游通用 `setup.py` 自动更新引擎来替换本发布；它可能下载不支持此 Windows gfx906 链路的产物。更新任一 EXE/DLL 后需重新验证整个组合。

## 发布验证范围

原始机器已实测：引擎加载、OpenAI API 生成、中文/算术基本检查、短代码性能、原生 256K 输入召回、KV 算子检查、CPU/GPU 专家格式算子检查。长输入成绩使用 P26 验证引擎；P27/P28 后续修改专家 decode 分派并完成发布复测，未再次运行另一条 261K 输入，详见性能说明。

当前可移植安装脚本、运行库子集和服务的发布验证记录在 [docs/VALIDATION.md](docs/VALIDATION.md)。原始机器能跑不等于全新机器已经验收。双卡、Linux、ROCm 7.x 是独立路线，不属于本 Windows 单卡发行的已验证能力。
