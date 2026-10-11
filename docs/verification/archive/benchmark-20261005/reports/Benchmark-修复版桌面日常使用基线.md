---
type: 实验
status: 已验证
project: SymoCraft
module: benchmark
created: 2026-10-04
tags:
  - area/benchmark
  - topic/performance
---

# Benchmark 修复版桌面日常使用基线

**2026-10-04 历史对照：快速检查 3/3、正式基线 9/9 有效，无重试、取消、超时或强制结束。** 当前开发对照已改为 [2026-10-05 本机会话](Benchmark-本机基线-20261005.md)，两次结果不合并。按 [冻结约定](Benchmark-基准冻结-20261005.md)，失焦不否决基准，也不追加纯前台重测门槛；不追溯改写本次原始数据。

## 实验条件与身份

Ryzen 7 9700X / RTX 5070 Ti / 64 GB 档，系统可见 66,190,954,496 字节；Windows build 26200、OpenGL 4.6.0 NVIDIA 616.64、Release / MSVC 19.38.33145。AC=1，电源 GUID `381b4222-f694-41f0-9685-ff5bb260df2e`；普通性能、无超频降压沿用此前用户声明，并非重新测量，GPU 模式未记录。

1920×1080、4x MSAA、VSync 关闭、普通无边框、preserve-shell-z-order；协议/工作负载 v2、allow-unfocused、seed 424242、441 区块、摘要 `fnv1a64:bddd435ea737fa62`。static / walk / edit 各三轮，60 秒预热 + 180 秒采样；此前快速检查每场景 10 秒预热 + 10 秒采样，不并入正式统计。

正式时间：2026-10-04 16:46:40.847 至 17:22:58.323（Asia/Shanghai）。温度和后台活动未采集，焦点不能证明遮挡面积，NVX 是设备级估计。

| 对象 | SHA256 |
| --- | --- |
| 修复包 ZIP | `12771D0DB58308D28E015F27BBDF4C076998D56AC0CF14D52DEED712FC314BBF` |
| 游戏 | `F73285A0AC164073DA7B8DFB912EF2FAE558061DF433A69A2FDBFA7B25729AF1` |
| 执行器 | `2E2210D1A938790A3251228F4D5DCAE02CE786EE865F4C254B86BB2F17225709` |
| 结果 ZIP | `09dc29e882caf146b4805f17e315a8548119119ae1ae5c98f1cedb58ef9cd83e` |

构建来源见 [窗口修复身份](../evidence/window-fix/build-identity.json)，资源身份见 session.yaml，不用当前 HEAD 代替二进制身份。

## 核心结果

以下为各场景逐轮指标的三轮中位数，不是合并帧分布。帧时取相邻 SwapBuffers 返回间隔，nearest-rank 百分位，不剔除异常帧。

| 场景 | FPS 中位数（范围） | P95 ms | P99 ms |
| --- | --- | ---: | ---: |
| static | 166.29（163.32–167.84） | 7.0083 | 7.6334 |
| walk | 172.35（166.03–173.80） | 6.6310 | 7.0008 |
| edit | 166.24（165.94–171.17） | 7.0625 | 7.9512 |

- 九轮退出码均为 0，`continuous_full_protocol=true`、`retried=false`；正式 272,335 帧，GPU 查询缺失 2 帧，不补零。
- 229,818 个正式帧失焦，估计 1,368.339 秒（84.47%），含预热 6 次焦点转换。允许切换窗口但未记录完整后台负载，只能作为日常使用对照，不是受控前台成绩或焦点因果实验。
- edit 各轮 720 次世界写入、2,160 次重建；static / walk 无编辑。不是鼠标输入链路验收。
- static / walk / edit 的 upload 均值中位数为 2.9613 / 2.8623 / 2.9492 ms，submit 不含 upload 为 2.6983 / 2.5892 / 2.6719 ms，GPU draw 为 0.1230 / 0.1093 / 0.1229 ms。CPU 墙钟不是利用率，GPU 区间不含上传、clear、swap，不能据此确定瓶颈。
- main 至首次交换返回中位数为 982.64 / 967.18 / 989.43 ms，不是双击到显示延迟。含预热、每轮 240 条低频内存记录的工作集峰值 520.62 MiB，private bytes 峰值 1,128.75 MiB，不证明无长期泄漏。

## 证据与复现

[快速会话](../evidence/desktop-windowfix-20261004/quick-session.yaml)、[正式会话](../evidence/desktop-windowfix-20261004/session.yaml)、[运行声明](../evidence/desktop-windowfix-20261004/launch-record.json)、[逐轮摘要](../evidence/desktop-windowfix-20261004/analysis-rounds.json)、[内存摘要](../evidence/desktop-windowfix-20261004/analysis-memory.json)、[导出核验](../evidence/desktop-windowfix-20261004/export-verification.json)。

当时从新目录解压修复包，先快速检查再正式采样，结束后用同一原生执行器导出；ZIP 退出码 0，82 个清单文件逐项哈希通过，另含清单本身。派生结果由 yaml-cpp 和结构化 CSV 读取汇总，没有重筛逐帧数据或修改原结果。

原始根目录 `out/benchmarks/desktop-windowfix-20261004-0844Z`，正式 / 快速结果在 `baseline` / `quick`；导出 `export-20261004-092619-002Z/results.zip` 为 21,058,742 字节，不含程序或脚本。docs 仅保留约 0.21 MB 小型证据，完整 CSV、焦点记录及截图留在 out。

复现使用 [原生执行器](../使用与交付.md) 的 `--run --output <新目录> --focus-policy allow-unfocused`，快速另加 `--quick`，导出用 `--export --output <正式结果目录>`；不覆盖旧结果，不自动运行采样。

## 结论边界

- [x] 已记录环境、版本、连续协议与导出核验，并回写执行器正式采样验收项；“已验证”限本次实验。
- 不与旧协议、窗口模式、strict 策略或其他会话合并，也不据此宣称修复提升性能。
- 本次不覆盖朋友原机、其他硬件、人工玩法、长期稳定性或 M2/M4；后续三机证据见冻结记录。AMD/Intel Windows 真驱动仍待补齐，历史范围不因后续归档而扩张。
