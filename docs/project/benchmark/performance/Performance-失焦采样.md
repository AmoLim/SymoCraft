---
type: 功能
status: 已验证
project: SymoCraft
module: performance
created: 2026-10-04
tags:
  - area/benchmark
  - topic/performance
---

# Performance 失焦采样

关联类 / 协作者：[[Performance-Session类]]、[[Benchmark-文件协议]]、游戏 `Application::Run`。

## 当前设计

已有行为：普通游戏失焦暂停，采样循环本来就继续渲染，但旧 Session 会把任意失焦帧标为无效。笔记本历史失败是 `window-not-focused`，不代表程序崩溃。本轮是显式扩展采样策略，不把历史无效结果追认成有效基线。

```text
StartupOptions.focus_policy
  strict → 正常创建窗口，出现失焦帧即记录 invalid reason
  allow-unfocused → 不主动抢焦点，失焦帧继续测量并记录
  非 benchmark → 拒绝焦点策略参数；保持原有暂停逻辑

每帧 PollEvents → 检查最小化/帧缓冲 → 无效则退出
  → benchmark 不因焦点恢复重置物理时序
  → 同一模拟/网格/上传/绘制路径 → CPU/GPU 观测 → Session.Add
退出后 → 完整 CSV、焦点变化、summary、额外游戏截图
```

### 关键约束与取舍

2026-10-05 基准采用约定：按用户确认，当前低端/中端/开发机统一使用既有 allow-unfocused，窗口初始未获焦和轮内失焦只记入分析，不列失败，也不要求前台补测才能冻结。laptop / this 的 18 轮首帧均未获焦，用户报告每轮新建窗口会失焦；不能由此断言全部后台时间的原因或遮挡面积。详见 [[Benchmark-基准冻结-20261005]]。本次只更新文档，没有修改下面两种策略的实现或普通游戏行为。

- 焦点与最小化不是一回事。两种策略都拒绝最小化、零尺寸和帧缓冲改变，不暂停后拼接结果。
- `GLFW_FOCUSED=false`、`GLFW_FOCUS_ON_SHOW=false` 只在 allow-unfocused benchmark 创建时设置；不会定时调用抢焦点接口。
- 普通游戏的失焦暂停、鼠标锁定/恢复不变；benchmark 不注册手动视角输入，原有 Escape 退出仍可用。
- 焦点状态按事件处理后的帧取样；变化表记录首帧及状态改变。失焦秒数为相关帧间隔累加，不能声称是逐事件精确计时，故字段标 estimate。
- 确定性工作负载仍依赖实际时间推进；物理 dt 仍按既有范围截断。本轮不改游戏的移动算法、地形、网格算法或上传路径。
- workload_version 从 1 升到 2，数据比较必须分组；新策略改变了有效性边界，不得把旧 9 轮直接当新基线。
- 背景程序竞争、驱动后台限帧、锁屏、自动休眠均不是该策略可以屏蔽的干扰。程序不修改系统电源/驱动设置。

### 验收案例

| 状态 | 场景 | 预期行为 |
| --- | --- | --- |
| [x] | 参数：省略策略、显式 allow、未知值、非 benchmark 带策略 | 默认 strict；合法值接受，其他拒绝 |
| [x] | 同样的失焦帧写入两种 Session | strict 无效，allow 有效；两者都保留原帧 |
| [x] | 焦点 false→true | 转换数、失焦帧数和估算秒数导出 |
| [x] | 显式最小化无效原因写入 allow Session | 仍判无效 |
| [x] | RTX 5070 Ti，三场景全程失焦，10+10 秒 | 3/3 有效；每轮约 20 秒失焦；walk 有位置推进，edit 有 40 次采样修改 |
| [x] | 真实游戏测试中点击最小化 | 提前退出，completed=false，valid_run=false，原因 framebuffer-changed-or-minimized，退出码 4 |
| [x] | Y9000P 声明混合显卡、实际 NVIDIA GL 设备 | 新修复包九轮有效，失焦保留；见 [[Benchmark-Y9000P中端机基线]]，不是核显切换或 AMD/Intel 驱动验证 |
| [ ] | AMD/Intel Windows 真实设备 | 驱动端确认，无厂商假设 |
| [ ] | 普通游戏连续人工游玩/焦点恢复 | 完整玩法回归仍按 M2 人工节点验收，不以自动化替代 |

验证版本：本次 Release 游戏 SHA256 `30e69da18ec62731b8081a5aea4c2b3ffaf0a955168c9bffb9d0ed0783ce73eb`。短测为流程证据，不是正式性能评价；完整原始 CSV 在 `out/benchmark-native`。摘要和日志保存于 `docs/project/benchmark/evidence`。

实现位置：`include/core/startup_options.h`、`src/core/startup_options.cpp`、`src/core/application.cpp`、`src/core/performance.cpp`；自动化入口 `core.startup_options`、`performance.export`。

## 本次变更

2026-10-05 仅记录当前基准采用 allow-unfocused 和回收笔记本证据，不改采集/窗口代码、默认参数或原始文件。旧 strict 失败留作历史，未执行硬件与人工案例保留复核状态。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| 必须支持最小化/无桌面运行 | 独立离屏基准协议和图形上下文方案，不复用窗口基线名义 |
| 要消除后台竞争 | 专用测试机/隔离测量，而不是删除失焦慢帧 |
