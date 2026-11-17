# 发布验证记录

日期：2026-11-17。此文件区分原生产组合与本次新整理的可移植部署包。

## 原生产组合已有的硬件证据

- Windows 10 Pro 22H2（19045），MI50 16 GiB / gfx906，i5-9400F，64 GiB RAM。
- 主模型完整加载、OpenAI API 与中文/算术生成成功。
- P28 短代码三次发布复测、P26 65K / 近 262K 全新输入基本召回及无 overflow，详见 PERFORMANCE.md。后续部署版本未再次跑另一条 261K 请求。
- CPU/GPU 专家格式、KV streaming / window parity 等历史验证，不在本次重新声称全部重跑。

## 本次在独立发布目录实际通过

- 解压固定哈希的 Release 运行包；每个 runtime 文件校验通过。系统 HIP 和生产目录没有被覆盖。
- 用现有标准 GGUF、完整 pack 和固定 DFlash2 生成新的本机配置，端口 18081。主模型的完整 83.6GB 哈希在本次复用检查中显式跳过，仍检查两片大小和 pack 来源；这是已有生产数据，不是重新下载测试。
- `scripts/doctor.py --gpu-probe`：枚举 1 张 GPU，选中 gfx906；hipBLAS 初始化；FP16 输入、FP32 累加的 FP32 / FP16 输出 2×2 GEMM 都正确得到四个 2.0；未知 dtype 被拒绝。只使用发布目录中精简的 gfx906 BLAS 子集。
- `vendor/strata/serve/server.py --help` 成功，Python 服务和控制模块可导入。
- 独立 mock 服务 127.0.0.1:18082：health=ok，Web 首页 HTTP 200，OpenAI chat/completions 返回合法 choices。mock 不调用模型，不作为新的 GPU 整模型成绩。
- 四项部署回归测试：HTTP 断点续传、服务忽略 Range 时重下、错误 SHA 不发布最终文件、错误 Content-Range 拒绝，以及固定元数据/路径配置检查。
- PowerShell setup.ps1 语法检查通过。
- 在全新的 `.venv` 中安装 requirements.txt 的固定依赖成功（Python 3.13.6），完整执行 setup.ps1 的已有模型/pack 复用分支成功；在该 venv 中重新执行 doctor、数值探针、四项部署测试和服务导入检查均通过。下载依赖时使用项目可写 TEMP 和本地 pip cache，避开原机器默认目录权限问题。
- 两个 ABI 适配器从 scripts/ 的源码独立重建成功；未用新 DLL 覆盖已验证 runtime。
- 从仅含 Git 提交文件的另一个全新 clone 目录再次准备本机配置、解压固定 Release ZIP；doctor 数值探针和全部四项部署测试通过。运行库不依赖作者原 rocm571 目录。
- 留存两个原厂驱动安装包 Authenticode 均为 Valid、签名者 AMD；完整大小与 SHA 已记录。
- 发布脚本、文档和 vendored 文本扫描未发现 token 或私钥；本机路径只允许存在于被忽略的 local/、build/。移除一个带机器固定路径的上游研究 runner，不包含用户提示/输出日志。

## 未作为通过的项目

- 全新 Windows 系统上的驱动安装、重启、下载整套 83.6GB GGUF 并从零打包：未重跑。本次验证复用了原机器已有模型数据。
- 可移植目录第二次完整加载 41 GiB RAM arena 并重新跑 256K：未做，避免同时启动两个大内存模型或中断正在使用的生产服务。运行 EXE/DLL 与原验收组合相同；新运行库子集额外通过上述数值检查。
- 新源码重建二进制的完整模型质量/性能验收：尚未进行，发布使用原有已验证 EXE。重建编译状态会在下面单独更新。
- Linux、双卡、其他 Windows 版本、其他 GPU/VBIOS、MTP 模式：本发布没有重新验收。

## 可执行复核

```powershell
py -m unittest discover -s tests -v
.\.venv\Scripts\python.exe scripts\doctor.py --gpu-probe
.\run.bat
# 另一个终端：
.\.venv\Scripts\python.exe scripts\smoke.py
```

后两项需已经准备好本机模型/配置及驱动。CI 的无 GPU 检查不代表 MI50 推理实测。任何新的全机复现都应另记日期、硬件、组件哈希和失败项。
