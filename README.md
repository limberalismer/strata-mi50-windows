# MI50 + Strata：Windows 原生部署

用 **MI50 16GB 在 Windows 下运行 Strata**。

这套环境已经跑通 Qwen3.8-Flash-Next IQ3_S、DFlash2 加速、OpenAI / Anthropic API 和 256K 长上下文。走原生 HIP，直接在 Windows 中运行。

仓库包含引擎、HIP/hipBLAS 兼容层、源码和部署脚本。驱动从 AMD 下载，主模型使用标准 GGUF，运行库由 Release 提供。

如果让 agent 帮你安装，把仓库地址交给它即可，也可以直接复制：

> 帮我部署 https://github.com/limberalismer/strata-mi50-windows 。先读 README.md 和 AGENTS.md，检查本机环境，优先使用 Release 运行包，复用已有模型。安装完成后启动服务，验证 API，并告诉我日常启动方式。

## 实测性能

测试机器：**Windows 10 Pro 22H2、MI50 16 GiB、i5-9400F、64 GiB RAM、PCIe 3.0 x16**。

模型为标准 IQ3_S，单请求，DFlash2，4096 prefill chunk，256K 上下文容量，INT8 KV，32K KV 驻留。记录时间为 2026 年 11 月 13–17 日。

| 场景 | Prefill | Decode |
| --- | ---: | ---: |
| 短代码：107 输入 / 512 输出，多组三次测试 | — | 各组中位 **39.4–41.8 tok/s** |
| 日常长输入、长输出：最近 20 条请求 | **404.4 tok/s**¹ | 中位 **24.85 tok/s** |
| 11 月 17 日：6 条新增输入至少 4K 的请求 | 中位 **413.45 tok/s** | — |
| 65,479 token 全新输入，控制码召回 | **470.1 tok/s** | 28.5 tok/s，46 输出 |
| 261,847 token 全新输入，控制码召回 | **415.0 tok/s** | 28.8 tok/s，49 输出 |

¹ 20 条日常请求中，有 14 条新增输入至少 4K，prefill 取这 14 条的中位数。decode 的 20 条请求均为输入至少 8192、输出至少 128。

短代码其中一组三次成绩为 **40.3 / 43.1 / 41.8 tok/s**。日常长任务的速度更接近 **25 tok/s**。两条长上下文测试都完整读取输入，前、中、后三个控制码全部召回，KV 无溢出。

数据摘录在 [evidence/](evidence/)，测试方法、版本和阶段耗时见 [性能记录](docs/PERFORMANCE.md)。

## 准备环境

| 项目 | 本套配置 |
| --- | --- |
| 显卡 | MI50 16 GiB，gfx906；系统中可能显示 Radeon Pro VII |
| 系统 | Windows x64，实测 Windows 10 Pro 22H2 |
| 内存 | 64 GiB |
| 磁盘 | 建议预留 160 GiB SSD 空间，存放模型和 native pack |
| Python | 3.11+ x64，含 `py` launcher；部署流程使用 3.13.6 验证 |
| 其他 | Git、[Visual C++ x64 Runtime](https://aka.ms/vs/17/release/vc_redist.x64.exe) |

使用预编译运行包时，只需要 Python 和 VC++ Runtime；源码重建另需 VS2022 Build Tools 和 TheRock 工具链。

### 1. 显卡驱动

本机使用 **Adrenalin 25.3.1**，其系统 HIP 运行库可以识别 gfx906。

- [AMD 25.3.1 版本说明](https://www.amd.com/en/resources/support-articles/release-notes/RN-RAD-WIN-25-3-1.html)
- [AMD 原厂 combined 安装包](https://drivers.amd.com/drivers/whql-amd-software-adrenalin-edition-25.3.1-win10-win11-march-rdna-combined.exe)
- [驱动安装、校验值和运行库说明](docs/DRIVERS.md)

驱动安装后按提示重启。已有可用驱动的机器，可以先运行后面的自检。部署脚本只准备项目环境，显卡驱动单独安装。

### 2. 模型

主模型使用标准的 **[ISTA-DASLab/Qwen3.8-Flash-Next-GSQ-RCO-GGUF](https://huggingface.co/ISTA-DASLab/Qwen3.8-Flash-Next-GSQ-RCO-GGUF)**，量化为 **IQ3_S**。本项目没有修改主模型权重。

固定版本：`ed59f92082b1e93c0e96d60a8b11aab089b52f09`。两个文件合计约 **83.6 GB**：

- [Qwen3.8-Flash-Next-GSQ-RCO-IQ3_S-00001-of-00002.gguf](https://huggingface.co/ISTA-DASLab/Qwen3.8-Flash-Next-GSQ-RCO-GGUF/resolve/ed59f92082b1e93c0e96d60a8b11aab089b52f09/Qwen3.8-Flash-Next-GSQ-RCO-IQ3_S-00001-of-00002.gguf)
- [Qwen3.8-Flash-Next-GSQ-RCO-IQ3_S-00002-of-00002.gguf](https://huggingface.co/ISTA-DASLab/Qwen3.8-Flash-Next-GSQ-RCO-GGUF/resolve/ed59f92082b1e93c0e96d60a8b11aab089b52f09/Qwen3.8-Flash-Next-GSQ-RCO-IQ3_S-00002-of-00002.gguf)

文件大小和 SHA-256 见 [configs/models.json](configs/models.json)。安装脚本会检查模型，并在本机构建 native pack、导出 tokenizer。专家打包保留原量化权重，额外占用约 52 GB 磁盘。

草稿模型使用 **Strata tuned DFlash2**，339.7 MB，由脚本从 [Strata v0.2.12](https://github.com/Niko1221/Strata/releases/download/v0.2.12/qwen3.8-flash-next-dflash2.strata1.q4_0-selq4.pack) 获取。它与主模型分开加载，来源见 [DFlash2 说明](licenses/dflash2-tuned/NOTICE.md)。

## 安装

在 PowerShell 中克隆仓库：

```powershell
git clone https://github.com/limberalismer/strata-mi50-windows.git
cd strata-mi50-windows
```

### 已有 GGUF

将两个模型文件放在同一个目录，例如 `D:\Models\IQ3_S`，执行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\setup.ps1 -ModelDir D:\Models\IQ3_S
```

脚本会创建 Python 环境，下载约 54 MiB 的运行包和 DFlash2，校验文件，构建 native pack，并生成本机配置。首次校验和打包需要读取大文件，请等待完成。

### 自动下载模型

加上 `-DownloadModel`，脚本会下载上述固定版本的两个 GGUF：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\setup.ps1 `
  -ModelDir D:\Models\IQ3_S -DownloadModel
```

下载支持断点续传；模型保存在 `-ModelDir` 指定的位置。

### 复用已有 Strata 数据

已有同一模型的完整 native pack 和草稿文件时，可以直接复用：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\setup.ps1 `
  -ModelDir D:\Models\IQ3_S `
  -PackDir D:\StrataData\packs\iq3_s `
  -Draft D:\StrataData\dflash2.strata1.pack
```

其他参数：

| 参数 | 用途 |
| --- | --- |
| `-Port 8082` | 修改服务端口，默认 8081 |
| `-Gpu 1` | 选择 GPU，默认 0 |
| `-RuntimeArchive D:\Downloads\strata-mi50-windows-runtime-v1.0.0.zip` | 使用已下载的 [Release 运行包](https://github.com/limberalismer/strata-mi50-windows/releases/tag/v1.0.0) |
| `-SkipModelHash` | 复用已经独立校验过的 GGUF，省去再次读取全文件计算哈希 |

本机路径写入 `local/config.json`。移动项目或模型目录后，重新执行 setup 即可更新配置。

## 启动与使用

先运行自检，再启动服务：

```powershell
.\.venv\Scripts\python.exe scripts\doctor.py --gpu-probe
.\run.bat
```

首次加载会在 RAM 中准备约 41 GiB 专家数据。启动窗口保留运行；日志出现模型就绪后，在另一个终端验证生成：

```powershell
.\.venv\Scripts\python.exe scripts\smoke.py
```

通过后即可打开聊天页面，或接入客户端：

| 接口 | 地址 |
| --- | --- |
| Web 聊天 | <http://127.0.0.1:8081/> |
| OpenAI base URL | `http://127.0.0.1:8081/v1` |
| Anthropic endpoint | `http://127.0.0.1:8081/v1/messages` |
| 模型 ID | `qwen3.8-flash-next-gsq-rco-iq3_s-mi50` |
| 日志 | `logs/engine.log` |

PowerShell 请求示例：

```powershell
$body = @{
  model = 'qwen3.8-flash-next-gsq-rco-iq3_s-mi50'
  messages = @(@{ role = 'user'; content = '写一个 Python 快速排序函数。' })
  max_tokens = 512
} | ConvertTo-Json -Depth 6

Invoke-RestMethod http://127.0.0.1:8081/v1/chat/completions `
  -Method Post -ContentType 'application/json; charset=utf-8' `
  -Body ([System.Text.Encoding]::UTF8.GetBytes($body))
```

默认监听本机地址。日常使用只需运行 `run.bat`；按 Ctrl+C 停止服务。

## 运行配置

默认参数在 [configs/preset.json](configs/preset.json)：

```text
prefill chunk       4096
context             262144
KV                  INT8，显存驻留 32768 token
resident RAM budget 42 GiB
PCIe fraction       0.55
draft               DFlash2，spec 6，confidence 0.35
```

专家缓存实测占用约 **8.07 GiB 显存**。显存中还放有约 2.59 GiB 投影权重、主输出头、DFlash2、KV 和计算工作区。其余专家保存在 RAM，由 GPU 经 PCIe 访问或交给 CPU 计算。

## Windows 适配

这套运行环境由三部分组成：

```text
TheRock 10.3 编译的 gfx906 引擎，code object v4
  ├─ HIP ABI 兼容层 → 驱动自带的 HIP 5.7 系运行库
  └─ hipBLAS ABI 兼容层 → hipBLAS / rocBLAS 5.7.1 + gfx906 内核
```

兼容层处理新旧 HIP 属性结构、API 和 BLAS 数据类型差异。主机内存使用单次连续注册，当前上限为 24 GiB；其余专家数据通过 VirtualLock 保留在 RAM。启动器为 HIP 的内部编译准备了独立的可写临时目录。

## 源码与排错

| 内容 | 位置 |
| --- | --- |
| 引擎和 Python 服务源码 | [vendor/strata](vendor/strata/) |
| HIP 兼容层 | [scripts/hip57_shim.cpp](scripts/hip57_shim.cpp) |
| hipBLAS 兼容层 | [scripts/hipblas57_adapter.cpp](scripts/hipblas57_adapter.cpp) |
| 源码重建 | [docs/BUILD.md](docs/BUILD.md) |
| 驱动、下载和校验 | [docs/DRIVERS.md](docs/DRIVERS.md) |
| 启动崩溃、DLL、锁页、KV 和速度问题 | [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) |
| 性能数据与瓶颈 | [docs/PERFORMANCE.md](docs/PERFORMANCE.md) |
| 发布验证记录 | [docs/VALIDATION.md](docs/VALIDATION.md) |

基于 [Strata](https://github.com/Niko1221/Strata) 和 [llama.cpp / ggml](https://github.com/ggml-org/llama.cpp)。项目代码采用 MIT 许可；第三方组件、模型及草稿模型的来源与许可见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
