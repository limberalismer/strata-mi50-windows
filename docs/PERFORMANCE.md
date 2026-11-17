# 实测性能、口径和瓶颈

2026-11-17。Windows 10 x64、MI50 16 GiB、i5-9400F（6 核 / 6 线程，AVX2）、64 GiB RAM、PCIe 3.0 x16。IQ3_S 主模型、单请求、DFlash2、spec 6、4096 prefill chunk、262144 容量、INT8 KV、32768 resident KV、42 GiB resident RAM 预算。部署引擎 SHA-256：`08d17dd1964a649f23da0e98c1891908364c4c83acf983444cbe83d1670fc588`。

## 已记录结果

- P28 发布目录，107 输入、512 输出、三个正式代码请求：40.3、43.1、41.8 tok/s；中位 41.8。先做六次 64-token 预热，固定空闲 30 秒，再串行测三次；正式请求没有 prompt cache 复用。后续 P29 同日对照中位 39.4，因此短代码水平按 39–42 理解。
- P26 无复用长输入：65479 / 261847 token，prefill 470.1 / 415.0 tok/s；分别输出 46 / 49 token，decode 28.5 / 28.8。完整读取，无 KV overflow，前/中/后控制码 3/3 正确。它是基本长上下文召回验证，不是完整智力或代码生成测评。
- P26 长输入引擎 SHA：`8f7a2cd8c05eb19b34f9d0cd5293a47dad4dcb3e096c6dddb0b9e36f51ac7b5b`。后续 P27 增加专家 decode 融合分派；P28 发布引擎完成全部七种专家格式检查和短代码发布复测，未再完整重跑另一条 261K 输入。
- 2026-11-17 当前会话截至 13:13 的 16 条完整异构请求：decode 中位 26.0，按总输出 / 总 decode 时间汇总 25.0；其中 6 条新增输入至少 4K，prefill 中位 413.45，范围 371.8–471.9。
- 最近 20 条输入至少 8192、输出至少 128 的完整请求：decode 中位 24.85、范围 22.1–32.8、按时间汇总 24.03；draft 接受比例 50.31%。其中 14 条新增输入至少 4K，prefill 中位 404.35。输入、复用、路由和输出长度不同，不能当作受控 A/B。

机器可读计数和阶段摘录在 `../evidence/bottleneck-evaluation-20261117-evidence.json`。不包含用户提示或完整输出。短代码、长文短答与真实 agent 长输出是不同负载，不能混用峰值。

## 瓶颈

P28 三条正式代码请求约 108–115 ms/window、4.6 token/window。CPU 专家区间约 54–60 ms（50%–52%），GPU-reach wait 41–43 ms（37%–39%），draft 约 5 ms，plan 约 0.3 ms。CPU 区间包含计算和线程等待；GPU-reach 是等待 GPU 到达下一层，包含执行与交接，不是纯 PCIe 传输时间。首要优化是 IQ2_S / IQ3_XXS CPU 专家及 CPU/GPU/PCIe 分工。

无 overflow 的 P26 65K / 262K GPU 时间线：

| 路径 | 65K | 262K |
| --- | ---: | ---: |
| 专家 GEMM gate/up + down | 32.63% | 29.31% |
| 完整 GDN | 26.41% | 23.30% |
| QSA 投影、选择、attention 等 | 17.53% | 26.24% |
| copy wait + dequant | 9.51% | 8.17% |

`gdn` 计时只含输入投影，应加上 conv/gates、recurrence、out projection 才是完整链路。QSA select 从 4.2% 增至 14.4%，长上下文下不可忽略。各阶段还可能包含 host 提交空隙。host staging 是累计且与 GPU 重叠，不能加到 GPU timeline 上。

## 不计为成功的数据

P29 的约 770.9 / 751.1 tok/s 实验，保存 JSON 与实际启动参数不同；实际 resident KV 仅 4096，发生 `OVERFLOW (too few resident cells)`。即使控制码仍能答对，也不能验收为完整注意力或无损提速。当前生产服务快照没有 OVERFLOW，不应将实验错误说成当前服务错误。

80–90 tok/s decode、1000+ tok/s prefill 尚未达到。双卡和 Linux 的数值属于其他环境的估算/外部测量，不能与本单卡 Windows 数据混在一起。

## 复测要求

记录 EXE/DLL 哈希、GPU/CPU/RAM、驱动、实际进程参数、chunk、KV resident、cache slots、RAM 注册量、预热与空闲间隔。用带唯一标记的输入排除 prompt cache，每组重复至少三次；分别记录全部样本、中位数和范围。长输入要求输入全部读取、无 nonfinite、无 KV overflow，再检查多点召回。改累加或分派时，另做 logits/算子正确性和多类任务质量检查。
