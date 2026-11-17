# 故障处理

## HIP 设备为零 / 运行库找不到 gfx906

先核对显卡实际硬件 ID、驱动与 `C:\Windows\System32\amdhip64.dll`。本机实际文件版本 10.0.3592.0（此前也测过 10.0.3584），属于驱动自带旧 HIP 路线；它不是应当被新版 SDK DLL 覆盖的文件。新 ROCm Windows runtime 存在不能枚举 gfx906 的版本。仓库 `amdhip64_7.dll` 是转发适配层，不是完整 runtime。

doctor 的 HIP/BLAS 小探针在子进程运行。失败时先解决版本和 DLL 解析问题，不要不断重启完整模型。另一张 AMD 卡存在时，用 `setup.ps1 -Gpu N` 明确选择 MI50。

## 加载初期访问异常 0xC0000005 / COMGR 编译失败

原机器曾在 embedding 后、首次权重上传前崩溃。HIP 内部 blit kernel 编译日志显示 `Couldn't create blit kernels` / `Could not create BlitManager`。已验证的修复是为引擎设置项目内可写 `TEMP/TMP/TMPDIR`，见 scripts/launch.py 的 `local/hip-tmp`。这不改变全局环境变量。

必须从 run.bat 启动，或者手工为子进程设置上述环境。简单 hipMalloc/小拷贝通过不足以证明 COMGR 大模型启动链路正常。不要用更换驱动掩盖一个临时目录问题。

## page-locking refused

先看后续日志，而不是只看第一条失败。当前成功模式是约 40.93 GiB RAM 副本、24 GiB 连续 GPU 注册、剩余约 17 GiB 通过工作集与 VirtualLock 锁定。留意最终 `resident RAM mode` 和推理期 `0 blob reads from the file`。

管理员身份、锁页权限、可用 RAM、commit limit/分页文件与 Windows/HIP 限制都可能影响结果。不应一开始就扩大注册上限。本发布固定 24 GiB 上限是已验证组合的一部分。若需要修改本地安全策略，先核对是否必要、说明影响并获得用户授权；不要自动批量改注册表或系统 ACL。

## 显存不足 / WDDM 换页 / 图捕获失败

先关闭与本任务无关但占显存的程序，确认选对 GPU，保留当前 reserve。不要关掉正在服务的其他任务。不同显示连接、VBIOS 和驱动会改变可用预算。不能以关闭 KV、缩短上下文或漏算专家代替故障解决。若要做较小配置对照，应新建独立配置并明确能力变化。

## KV OVERFLOW

`OVERFLOW (too few resident cells)` 是失败信号；该请求的速度不计为能力完整的成绩。核对日志里的实际 `--kv-resident 32768` 与 4096 chunk，而不是只看 JSON。不能因为一个 needle 答对就忽略 overflow。

## prefill / decode 低于表格

区分短输入固定开销与真正长 prefill；只计算新输入 token。真实长上下文、长输出的 decode 约 25 tok/s，不能要求每条请求都复现短代码专项 42。检查 draft 接受率、专家缓存、CPU 区间、磁盘回读、其他 CPU/RAM 竞争。

原机器 GPU-Z 的 0MHz/Unknown 曾源于组件/字段兼容问题，不能用主页面静态字段证明锁频。不要把未经校验的 ADL DLL 替换进系统；该修复不是推理必需步骤。

## 配置路径/端口错误

重跑 setup 生成本机 `local/config.json`。不要编辑另一个未被启动器读取的历史 JSON。端口冲突时改 `-Port`，不要终止不属于本项目的服务。配置默认只监听 127.0.0.1。

## 换模型/双卡/Linux

本预设只针对标准 IQ3_S、单 MI50 的 Windows 链路。其他量化和平台需要独立打包、配置和验收。多卡不是将 gpu 改成数组就保证成功；旧 HIP 多设备内存映射尚未在本机验收。
