---
type: 实验
status: 已归档
project: SymoCraft
module: benchmark
created: 2026-10-05
tags:
  - area/benchmark
  - topic/performance
---

# Benchmark 本机基线 20261005

**当前开发对照 `this-desktop-20261005`：9/9 有效，退出码均为 0，无重试、取消、超时或强制结束。** run 与 export 是同一会话，只计九轮。共同身份、协议与统计见 [基准冻结](Benchmark-基准冻结-20261005.md)，不与 [2026-10-04 历史记录](Benchmark-修复版桌面日常使用基线.md) 合并。

## 实验条件

| 项目 | 记录 |
| --- | --- |
| 硬件 | Ryzen 7 9700X / RTX 5070 Ti；64 GB 档，系统可见 66,190,954,496 字节 |
| 系统 / 驱动 | Windows build 26200；OpenGL 4.6.0 NVIDIA 616.64；Release |
| 声明 | AC=1；接电、平衡、独显直连、默认无超频降压 |
| 采样 | 1920×1080、4x MSAA、VSync 请求关闭、普通无边框；v2、allow-unfocused、seed 424242、441 区块；三场景各三轮，60 秒预热 + 180 秒采样 |
| 时间 / 缺失 | 2026-10-05 09:59:45.732 至 10:36:02.374，UTC+8；温度、实际频率和后台负载未采集 |

## 核心结果

FPS / P95 / P99 为逐轮指标的三轮中位数；最大帧取该场景三轮最大值。逐轮明细见 [analysis.json](../evidence/this-desktop-20261005/analysis.json)。

| 场景 | FPS | P95 ms | P99 ms | 最大帧 ms |
| --- | ---: | ---: | ---: | ---: |
| static | 187.02 | 5.9012 | 6.1970 | 15.6983 |
| walk | 180.29 | 6.1862 | 6.5328 | 15.8782 |
| edit | 190.07 | 5.8842 | 6.5603 | 8.8375 |

正式 300,466 帧，GPU 查询缺失 8 帧，无超过 16.7 ms 的帧。有限样本的最大 15.8782 ms 不证明未来无卡顿，也不替代玩法或长期稳定性验收。

- 正式失焦估计 1,440.003 秒（88.89%）。九轮首帧失焦，static 1 在 0.4486919 秒获焦并保持，其余八轮全程失焦；含预热仅一次转换。按用户确认保留全部帧，不追加前台基准门槛，不能从焦点状态推断背景负载或遮挡一致。
- static / walk 无重建，仍每帧整理上传约 55.09 MiB；edit 各 720 次编辑、2,160 次重建，编辑帧 P95 为 8.2366 / 7.5217 / 7.5232 ms。
- static / walk / edit 的 pack 均值中位数为 2.2857 / 2.4669 / 2.3755 ms，upload 为 2.8581 / 2.9782 / 2.7593 ms；GPU draw 为 0.1215 / 0.1084 / 0.1216 ms。pack 已计入 submit 不含 upload；CPU/GPU 区间可能重叠，GPU draw 不含上传、clear、swap。
- 含预热低频内存采样：工作集峰值 607.41 MiB，private bytes 峰值 1,212.95 MiB；main 至首次交换返回中位数为 940.17 / 937.28 / 930.28 ms。不是长期泄漏判定或用户输入延迟。

## 核验与归档

82/82 导出清单文件哈希匹配；ZIP 内 83 文件与导出目录一致，run/export 的 session 相同且 54 个 capture 文件哈希一致。九轮 CSV 的帧时、阶段、焦点、编辑、重建与 GPU 缺失统计复算一致；静态截图只反映采样后额外渲染。此归档与本次精简均未重跑游戏。

- [会话与资源身份](../evidence/this-desktop-20261005/session.yaml)、[逐轮分析](../evidence/this-desktop-20261005/analysis.json)、[归档核验](../evidence/this-desktop-20261005/archive-verification.json)、[最终截图](../evidence/this-desktop-20261005/static-1-attempt-1/capture/final-frame.png)。
- 原回传 `.temporary/this-result` 不变；完整副本 `out/benchmarks/archive/this-desktop-20261005` 保留 run、export、ZIP，共 170 文件。ZIP SHA256：`7286dc7bc7a2187da70aa001fb4eb5015c7a6dc91c1924693321821319eab401`。
- docs 精选 57 个原文件及派生分析/索引；完整 CSV、其余截图和 ZIP 留在 out。原清单描述完整导出，不是 docs 子集，本地副本不是异地备份。
- [x] 归档与复算完成，纳入当前开发机冻结基准。
- 与 2026-10-04 成绩差异不能称为代码优化：游戏身份相同，环境和焦点暴露不受控一致；M4 优化验收不随之通过。
