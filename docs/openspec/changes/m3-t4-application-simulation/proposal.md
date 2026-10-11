---
tags:
  - area/spec
---

## Why

将已有静态应用组合、共享物理预算和混合观测责任收束为明确的生命周期与 CPU 推进入口，保持单机游戏和既有算法/协议。
范围依据 [scope](../../../scope/project-scope.md)；现役 [simulation](../../specs/simulation/spec.md)、[platform](../../specs/platform/spec.md)、[world](../../specs/world/spec.md)、[renderer](../../specs/renderer/spec.md)、[module-boundaries](../../specs/module-boundaries/spec.md)及[文件发布](../../specs/foundation-files/spec.md)已含场景预读。

## What Changes

迁移既有 T4 目标至这一份 change：实例 Application、受控平台资源、自有输入、SimulationSession、内部独立 physics target、观测封闭/导出及场景分离。
D01-D13保持已选；[Q01-Q04](decisions.md#未决接口)各三候选保留，尚未代选或设计冻结。当前新会话/physics 核心未实现；P01静态准备已交付，另有 CPU 基线/独立核心单测局部授权但仅基线已交付，不代表完整 T4 生产迁移授权或通过。
本轮只迁移文档，不实施源码功能，不真实 sync/archive、不提交/删除、不跑游戏/GPU。

不包含正式 physics 模块搬迁（T5审计后、NPC复用前独立节点）、新 ECS/SoA/并行、CCD/实体碰撞、输入缓冲产品、Engine/GameSession、Off/Basic/Deep/裁剪包、GL退役、拆进程/通信或服务器产品。

## Capabilities

- `simulation`：MODIFIED 共享时间边界为会话预算；ADDED 自有结果、显式暂停/失败边界及同一生产 CPU 逐次对照。
- `platform`：MODIFIED 静态生命周期、借用快照及平台内基准筛选为资源 owner/自有交接/app策略；保留 T2 全部继续有效行为与场景。
- `physics-core`：ADDED simulation 内部独立中立运动/碰撞/查询核心，不提前宣称正式第十模块。
- `application-lifecycle`：ADDED 私有组合根、一次 Run、有界关闭、帧顺序与适用组合验收。
- `telemetry-lifecycle`：ADDED 受控 Recording→Sealed 与独立导出结果，失败可显式重试、成功幂等；不替代尚开放的文件发布风险。

## Impact

涉及 game/app、platform、simulation/physics、telemetry 及根 test；依赖 T1 World、T2 实际输入/窗口、[T3候选与交接](../m3-t3-renderer/design.md)。T3 R1冻结/R2局部交付不代替R3/Vulkan及完整前序核对。
T4 不恢复[暂停 Fix1](../m3-t2-publication-fix1/proposal.md)，不将 Seal 当 Win32 1175 根因修复。
目标机制见 [design](design.md)，唯一任务及 A01-A27 身份见 [tasks](tasks.md)；批准/历史基线及证据仅从 [VERIFICATION](../../../verification/VERIFICATION.md) 进入。原 [T4来源](../../../spec/M3/M3-T4-应用平台与模拟模块接口.md)及P01材料只保留复核，不继续维护第二份计划。
