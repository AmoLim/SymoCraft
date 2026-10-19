---
tags:
  - area/spec
---

# M3-T5 ECS 与内存契约收口

## Why

[T5 范围](../../../scope/project-scope.md#m3-t5-完成-ecs内存契约与整体验收)要求解决实体身份、组件生命周期、遍历、分配失败与残留基础设施，并汇总 M3 整体依据。当前旧实现及 T4 限定修补仍有通用删除、跨 Registry/Clear 身份、代数耗尽和字节池对象生命周期缺口，见[当前 ECS 机制](../../../architecture/ecs.md)。继续修补或自研 typed 稀疏池均需长期维护这些机制；本聊天已选择采用成熟 EnTT 加项目 typed 门面，保留项目自己的公开契约。

本轮是从既有[探索决策](decisions.md)与聊天确认整理正式规划的授权，不是依赖版本冻结、生产实施、删除、Git 写入、规格同步或阶段验收授权。2026-10-17 仅编写规划，尚未运行候选适配实验或 C++ 验证。

2026-10-18 前置更新：S01.2 身份核对及 S01.3 有界 CPU 关键适配已完成，结果仅见 [verification](verification.md)；[DBG01](decisions.md#m3-t5-dbg01-统一-debug-abi-配置) 已获配置方向批准。用户本次仅授权保存 [S01.4 待审冻结稿](design.md#s014-待审冻结候选)，尚未批准依赖/完整设计冻结或生产替换。该准备细化既有 ECS/module-boundaries 增量，不新增能力或改写现役行为，也不重开 CT01 等已选方向。

2026-10-19 上游状态更新：T4 已获本机范围整体验收，规格已同步、change 已归档；其生产源码及必要构建/测试配套随后按用户明确授权同步至 F 主工作区。本次仅更新 T5 的输入与依赖描述，不新增行为变化、能力或批准；既有 ECS/module-boundaries 增量及 S01.1/S01.4 门槛不变。

## What Changes

- 以 EnTT 替换 `symocraft_ecs` 的旧字节池；保留通用 typed 项目接口，使用项目 EntityId、显式注册、统一对象组件语义、受检查的结构操作及同步回调查询。
- 明确跨实例/Clear 身份、耗尽不回绕、RAII、短期借用、逐组件 const、缺失处理、具名异常与 Failed 状态。默认不保序，显式有序查询使用完整自有键，平局拒绝。
- 以 T4 已交付的调用边界迁移装配、simulation 适配、app 残留 ECS 操作和测试；保持 T4 时间、输入、逐实体提交与结果所有权契约，不保留双 ECS 或无消费者的兼容空壳。
- 默认标准分配，不接 AmoMemory 适配器；核对旧分配器、线程池、输入及私有序列化的真实依赖，先提出准确退役清单，获确认后才删除，不按目录整体清理。
- 组织 ECS 契约、调用方回归、受影响集成与 M3 收口三层验证；局部交付和整体批准分开，未执行、禁用、失败和历史风险如实保留。

不包括 app 与 ECS 的独立解耦变更、NPC/AI/村落演化、多分辨率模拟、计算线程或进程接入、物理正式模块搬迁、新渲染能力、GL 最终退役或恢复暂停 Fix1。

## Capabilities

- **新增 `ecs`**：首次形成独立公开行为规格；这是对既有 ECS 行为的有意替换，不是声称项目原先没有 ECS。目标覆盖身份、生命周期、typed 组件、注册、读取、查询、借用、失败与串行交接。
- **扩展 `module-boundaries`**：追加 typed 第三方依赖、窄私有测试适配及 M3 组合验收/定向退役要求，沿用其余[现役边界](../../specs/module-boundaries/spec.md)，不修改已有 requirement。
- **保持行为**：[world](../../specs/world/spec.md)、[renderer](../../specs/renderer/spec.md)、[platform](../../specs/platform/spec.md)、[foundation-files](../../specs/foundation-files/spec.md)不另造平行 delta；T4 已同步的 [simulation](../../specs/simulation/spec.md)、[physics-core](../../specs/physics-core/spec.md)、[application-lifecycle](../../specs/application-lifecycle/spec.md)、[telemetry-lifecycle](../../specs/telemetry-lifecycle/spec.md)业务语义不由 T5 重写。历史增量及设计从 T4 归档读取，不再当作尚待交付的活动目标。

## Impact

直接影响 ECS 公开头/实现/CMake、simulation 的装配与遍历、app 的必要调用、根测试和 EnTT 许可部署。仅当核对确认依赖变化时定向调整 foundation 或边界规则；不将整个 foundation 纳入重写。

T4 的[设计与交接](../archive/2026-10-19-m3-t4-application-simulation/design.md)及[完成批准与保留风险](../archive/2026-10-19-m3-t4-application-simulation/verification.md#2026-10-19-t4整体完成批准)是上游依据。T3/T4 均已批准并归档；当前 [scope](../../../scope/project-scope.md#m3-t4-完成应用平台与模拟模块接口)和[工程状态](../../../architecture/engineering.md)确认 F 的 feature-refactoring 已取得 T4 生产代码及配套，不再仅有 T3 或审核文档。主工作区工具链为 VS2026/MSVC tools `14.51.36231`（cl `19.51.36260.0`）及 SDK `10.0.22621.0`，默认 D3D12、Vulkan 显式可选、GL 保留。

源码同步不是 Git 历史合并，也不是 T5 生产入口。S01.1 仍须绑定 T5 实际采用的源码/构建身份、统一 ABI 和代码根/实施范围；早先的 T5 独立实验树不能直接当作最新 T4 输入。既有上游批准不要求重新验收，但其历史成绩不自动成为 T5 替换后的组合成绩，DBG01 也不能仅凭工具集相同就视为已在生产全链路落实。

采用本地固定官方发布源码，初步候选 EnTT `v4.0.0`，不修改上游源码、不在配置时追踪远端最新版。完整 commit/摘要/许可证、统一配置及关键适配结果须在生产替换前核对并冻结；候选不兼容时报告冲突，不静默削弱契约。目标机制、具体检查和唯一活动任务分别见 [design](design.md) 与 [tasks](tasks.md)。
