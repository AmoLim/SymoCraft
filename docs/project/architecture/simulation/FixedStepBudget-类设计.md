---
type: 类设计
status: 草稿
project: Symocraft
module: simulation
class_name: "SymoCraft::PlayerMath::FixedStepBudget"
inheritance: []
created: 2026-10-06
updated: 2026-10-06
tags:
  - area/architecture
---

# FixedStepBudget 固定步预算

## 当前设计

### 职责与成员

只有 double accumulated_=0 秒；Step=1/120 秒，MaxSteps=8。计算当前帧可执行步数，不推进物理或拥有 Registry/world。Physics 私有静态 step_budget 持有一个，当前单会话，不是每实体预算。

### 不变量

原 Simulation S4：单次 Consume 返回 0..8；步长 1/120 秒。非有限/非正输入返回 0，不更改余数；长帧本次输入贡献上限 Step*8，并非无限追赶总丢失时间。

### 接口与生命周期

Consume(float frame_seconds) 将有效输入裁剪后累加，以 (accumulated_+1e-9)/Step 取步数且最多 8，减去已消费时间，余数以 max(0,...) 防负值。没有时间源、不等待、不分配。
Reset 清零；启动、暂停/失焦、重生由 app 经 Physics::ResetTiming 调用。
普通默认构造与析构；隐式复制/移动复制数值，不绑定独占时钟。仅主线程串行调用，不安全共享多线程推进。

[系统调用顺序](Simulation-系统设计.md)要求 Physics 每帧 Consume 一次再做子步；不能在每个实体/子步重复消费。浮点余数/epsilon 为当前实现，不作为绝对墙钟精度保证。

### 依据与限制

[声明与实现](../../../../game/modules/simulation/include/symocraft/simulation/player_math.h)、[Physics](../../../../game/modules/simulation/src/physics_system.cpp)、[历史功能验收](Simulation-功能.md#验收案例)、[T0 报告](../../../milestones/m3-t0/README.md)、[覆盖清单](../对象笔记覆盖清单.md)。2026-10-06 仅源码核对，未独立运行长时累积/多会话测试。

### 公开接口预期行为

| 签名 / 入口 | 调用方与可见范围 | 预期行为：输出及状态变化 | 前提 / 边界 | 失败反馈及失败后状态 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| `Consume(float frame_seconds)` | 模块公开类；Physics / CPU 测试 | 正有限输入最多贡献 Step*8，返回 0..8 并减去消耗时间 | 每帧调用一次，实例串行推进；epsilon=1e-9 | 非有限/非正输入返回 0，余数不变；无分配/异常恢复流程 | [player_math.h](../../../../game/modules/simulation/include/symocraft/simulation/player_math.h) / S4 |
| `Reset()` | Physics::ResetTiming | accumulated_=0 | 不推进物理，不读取墙钟 | 无分配或错误返回 | player_math.h |

### 私有函数预期行为

| 签名 / 入口 | 内部调用方 | 预期行为：处理规则及副作用 | 前提 / 边界 | 失败传播及清理责任 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| 不适用 | FixedStepBudget | private 只有 accumulated_，无 helper | 隐式构造/复制/移动见生命周期 | 无资源清理 | player_math.h |

## 本次变更

无。

## 后续考虑

多会话必须独立预算；补步/丢时间策略改变需单独 spec 与数值回归。

