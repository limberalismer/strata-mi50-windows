# 驱动、运行库与可核对的下载

此项目不是通用驱动安装器。显示驱动由 AMD 分发；本仓库不重新打包、修改 INF 或签名。不要刷 VBIOS、禁用 Windows 驱动签名检查、替换 System32 DLL。

## 原机器组合

| 组件 | 已用版本/来源 |
| --- | --- |
| 系统 | Windows 10 x64；GPU 驱动显示 Radeon Pro VII / MI50 16 GiB |
| 显示驱动 | Adrenalin 25.3.1，原厂 combined 安装包中的 Vega20 支持 |
| 系统 HIP | 驱动自带 `C:\Windows\System32\amdhip64.dll`；最近成功启动文件版本 10.0.3592.0 |
| BLAS | HIP SDK 5.7.1，23.Q4 For-HIP 中的 hipBLAS / rocBLAS / gfx906-xnack- 设备库 |
| ABI 适配 | `runtime/amdhip64_7.dll` 和 `runtime/hipblas.dll`，源码在 scripts/ |
| 编译工具链 | TheRock Windows wheels 10.3.0a20261111，仅用于编译；不是实际使用的新版 HIP runtime |

显示驱动、HIP runtime 和完整 SDK 是不同组件；不需要为了运行本发布安装整个 SDK。

## 25.3.1 显示驱动

- [AMD 官方版本说明](https://www.amd.com/en/resources/support-articles/release-notes/RN-RAD-WIN-25-3-1.html)
- [原厂 combined 安装包](https://drivers.amd.com/drivers/whql-amd-software-adrenalin-edition-25.3.1-win10-win11-march-rdna-combined.exe)
- 文件名：`whql-amd-software-adrenalin-edition-25.3.1-win10-win11-march-rdna-combined.exe`
- 本地留存文件：1,527,561,768 字节；SHA-256 `37a92c73b2ac980ed48ddf50708d8e889986b2a75907e44414a71385d9bf4e37`。

AMD 下载可能要求浏览器访问和接受条款。先打开官方下载页，不要因为直接请求返回 403 就换不明修改版。此 SHA 是实际留存文件的摘要；若 AMD 后续替换下载，不应自动接受新的字节作为同一已测文件，先查明来源和版本并另行验证。

发布前在原机器重新检查了两个留存安装包的 Authenticode：均为 `Valid`，签名者为 Advanced Micro Devices。下载者仍需校验自己收到的文件。

```powershell
Get-FileHash 'D:\Downloads\whql-amd-software-adrenalin-edition-25.3.1-win10-win11-march-rdna-combined.exe' -Algorithm SHA256
Get-AuthenticodeSignature 'D:\Downloads\whql-amd-software-adrenalin-edition-25.3.1-win10-win11-march-rdna-combined.exe'
```

安装前保存当前驱动信息和回退包；尤其存在其他 AMD GPU 或承担显示输出时。确认你的硬件 ID 在安装包中受支持后，使用正常安装界面并按提示重启。驱动替换可能影响其他显卡/应用，应由用户明确授权。不要因原机器可以安装就对其他卡强制套 INF。

## HIP SDK 5.7.1 原始来源

- [AMD 许可及下载入口](https://www.amd.com/en/developer/resources/rocm-hub/eula/licenses.html?filename=AMD-Software-PRO-Edition-23.Q4-Win10-Win11-For-HIP.exe)
- 文件：`AMD-Software-PRO-Edition-23.Q4-Win10-Win11-For-HIP.exe`
- 大小：1,259,878,656 字节。
- SHA-256：`ec167eb267d3f331edd65ecfaedd292f407e7ea00163e7758a9c3c3711c1da77`。

用于 BLAS 来源复核或源码重建。正常使用优先下载本项目固定哈希的 Release 运行包，无需重新安装 SDK、更换 PRO 显示驱动或复制系统 HIP。Release 只含实际需要的 MIT/BSD 授权 BLAS 运行文件，附 `licenses/rocBLAS-LICENSE.md` 和 `licenses/hipBLAS-LICENSE.md`。

## 运行包布局

```text
runtime/
  strata.exe                  已验证的 gfx906 v4 引擎
  amdhip64_7.dll               HIP 6+ 名称/属性到旧系统 HIP 的适配器
  hipblas.dll                 新旧 datatype / ABI 适配器
  hipblas57.dll               SDK 5.7.1 的原 hipBLAS（改名以免与适配器冲突）
  rocblas.dll                 SDK 5.7.1
  rocblas/library/*gfx906*    gfx906-xnack- 内核及对应 Tensile 元数据
```

完整逐文件大小与 SHA-256 在 `configs/runtime-files.json`。setup 同时验证下载 ZIP 和文件摘要。Windows VC++ 运行库从 [Microsoft 官方下载](https://aka.ms/vs/17/release/vc_redist.x64.exe) 获取，按微软条款安装。

`runtime/` 中不应有另一份 amdhip64.dll、AMD COMGR 或 OpenCL runtime；这些由匹配的显示驱动提供。不要把两个独立 ROCm 目录全部塞进 PATH，制造 DLL 解析顺序问题。启动器和配置只为子进程添加本仓库 runtime 目录。

## 驱动兼容性的实际边界

系统 HIP 能枚举 gfx906只是第一关，还需要 BLAS 与设备 code object 可加载、主机/GPU 地址一致、全部权重上传成功、真实 API 输出正确。doctor 的小探针与完整模型验收要分开报告。Linux 社区 ROCm 7.x 不是此 Windows runtime 的替代 DLL。
