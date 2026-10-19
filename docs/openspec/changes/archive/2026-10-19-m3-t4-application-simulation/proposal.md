---
tags:
  - area/spec
---

> 2026-10-19：T4已获整体验收，五份增量规格已同步，本change已归档；下文保留目标和授权的历史时点，不再是活动实施计划。风险与交付依据见[verification](verification.md#2026-10-19-t4整体完成批准)。

## Why

将已有静态应用组合、共享物理预算和混合观测责任收束为明确的生命周期与 CPU 推进入口，保持单机游戏和既有算法/协议。
范围依据 [scope](../../../../scope/project-scope.md)；现役 [simulation](../../../specs/simulation/spec.md)、[platform](../../../specs/platform/spec.md)、[world](../../../specs/world/spec.md)、[renderer](../../../specs/renderer/spec.md)、[module-boundaries](../../../specs/module-boundaries/spec.md)及[文件发布](../../../specs/foundation-files/spec.md)已含场景预读。

## What Changes

迁移既有 T4 目标至这一份 change：实例 Application、受控平台资源、自有输入、SimulationSession、内部独立 physics target、观测封闭/导出及场景分离。
D01-D13保持已选；[Q01](decisions.md#q01-实体装配入口)已由用户确认方案2，[Q02](decisions.md#q02-原始键盘事实)当前采用方案1并保留向方案3演进方向，[Q03](decisions.md#q03-关闭报告交接)已选方案2，[Q04](decisions.md#q04-只读诊断可见范围)已选方案3并仅保留公开Inspect未来方向。四项选型已确认，不推定完整接口设计冻结。P01静态准备和旧CPU基线已交付；2026-10-13按本聊天局部授权在新worktree交付独立physics核心/单测，新会话/生产适配尚未实现，不代表完整T4生产迁移授权或通过。局部结果见[verification](verification.md#2026-10-13-局部核心与交接核对)。
上述为局部授权时点；2026-10-15完整入口已覆盖这些实施限制，E候选现已接通Session/生产适配/实例应用。当前交付与实际CPU/SDL/GPU验证统一见[verification](verification.md#2026-10-15-完整生产迁移)，不将实现状态变成验收或current specs sync，不提交/合并/归档。

不包含正式 physics 模块搬迁（T5审计后、NPC复用前独立节点）、新 ECS/SoA/并行、CCD/实体碰撞、输入缓冲产品、Engine/GameSession、Off/Basic/Deep/裁剪包、GL退役、拆进程/通信或服务器产品。

2026-10-18用户确认将正式编译器切换纳入S06交付基线：最终候选改用Visual Studio 2026配套MSVC工具链，范围包含必要构建入口适配、候选重建及受影响验证。保Windows x64、C++20、CMake/Ninja与既定游戏行为，不附带升级SDL、EnTT、图形SDK或开展T5重构。此项细化现有application-lifecycle交付要求，沿用上述现役构建/模块、World数值环境及平台部署契约，不新增玩法能力；精确版本在实施前核实固定。

## Capabilities

- `simulation`：MODIFIED 共享时间边界为会话预算；ADDED 自有结果、显式暂停/失败边界及同一生产 CPU 逐次对照。
- `platform`：MODIFIED 静态生命周期、借用快照及平台内基准筛选为资源 owner/自有交接/app策略；保留 T2 全部继续有效行为与场景。
- `physics-core`：ADDED simulation 内部独立中立运动/碰撞/查询核心，不提前宣称正式第十模块。
- `application-lifecycle`：ADDED 私有组合根、一次 Run、有界关闭、帧顺序与适用组合验收。
- `telemetry-lifecycle`：ADDED 受控 Recording→Sealed 与独立导出结果，失败可显式重试、成功幂等；不替代尚开放的文件发布风险。

## Impact

2026-10-15用户已明确授权按已确认方案推进完整T4生产迁移，组合核对由实施方承担，新设计冲突/范围扩大再确认；入口以[关键节点](../../../../verification/VERIFICATION.md#关键节点)为准，覆盖前文局部授权时的限制。不是T4验收、T3候选合并或T5授权，未实现目标不因此成为现役规格。
2026-10-17用户追加授权将上游已批准T3提交604c2b58的源码同步到E目录T4，作为整合基线并核对过时数据；此项覆盖此前尚未选择T3候选的限制，不授权Git合并/提交、规格sync/archive或继承旧性能/人工成绩。实际交接及刷新依据见[本次整合](verification.md#2026-10-17-t3批准基线整合)。

涉及 game/app、platform、simulation/physics、telemetry 及根 test；依赖 T1 World、T2 实际输入/窗口、[T3候选与交接](../../m3-t3-renderer/design.md)。T3 R1冻结/R2局部交付不代替R3/Vulkan及完整前序核对。
T4 不恢复[暂停 Fix1](../../m3-t2-publication-fix1/proposal.md)，不将 Seal 当 Win32 1175 根因修复。
目标机制见 [design](design.md)，唯一任务及 A01-A27 身份见 [tasks](tasks.md)；批准/历史基线及证据仅从 [VERIFICATION](../../../../verification/VERIFICATION.md) 进入。原 [T4 目标设计](design.md)及 decisions 维护 P01 目标与未决项，旧目录已删除，不维护第二份计划。
