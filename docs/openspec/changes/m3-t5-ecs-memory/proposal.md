---
tags:
  - area/spec
---

# M3-T5 ECS 与内存契约收口

## Why

[T5 范围](../../../scope/project-scope.md#m3-t5-完成-ecs内存契约与整体验收)要求解决实体身份、组件生命周期、遍历、分配失败与残留基础设施，并汇总 M3 整体依据。当前旧实现及 T4 限定修补仍有通用删除、跨 Registry/Clear 身份、代数耗尽和字节池对象生命周期缺口，见[当前 ECS 机制](../../../architecture/ecs.md)。继续修补或自研 typed 稀疏池均需长期维护这些机制；本聊天已选择采用成熟 EnTT 加项目 typed 门面，保留项目自己的公开契约。

本轮是从既有[探索决策](decisions.md)与聊天确认整理正式规划的授权，不是依赖版本冻结、生产实施、删除、Git 写入、规格同步或阶段验收授权。2026-10-17 仅编写规划，尚未运行候选适配实验或 C++ 验证。

## What Changes

- 以 EnTT 替换 `symocraft_ecs` 的旧字节池；保留通用 typed 项目接口，使用项目 EntityId、显式注册、统一对象组件语义、受检查的结构操作及同步回调查询。
- 明确跨实例/Clear 身份、耗尽不回绕、RAII、短期借用、逐组件 const、缺失处理、具名异常与 Failed 状态。默认不保序，显式有序查询使用完整自有键，平局拒绝。
- 以 T4 交付后的调用边界迁移装配、simulation 适配、app 残留 ECS 操作和测试；保持 T4 时间、输入、逐实体提交与结果所有权契约，不保留双 ECS 或无消费者的兼容空壳。
- 默认标准分配，不接 AmoMemory 适配器；核对旧分配器、线程池、输入及私有序列化的真实依赖，先提出准确退役清单，获确认后才删除，不按目录整体清理。
- 组织 ECS 契约、调用方回归、受影响集成与 M3 收口三层验证；局部交付和整体批准分开，未执行、禁用、失败和历史风险如实保留。

不包括 app 与 ECS 的独立解耦变更、NPC/AI/村落演化、多分辨率模拟、计算线程或进程接入、物理正式模块搬迁、新渲染能力、GL 最终退役或恢复暂停 Fix1。

## Capabilities

- **新增 `ecs`**：首次形成独立公开行为规格；这是对既有 ECS 行为的有意替换，不是声称项目原先没有 ECS。目标覆盖身份、生命周期、typed 组件、注册、读取、查询、借用、失败与串行交接。
- **扩展 `module-boundaries`**：追加 typed 第三方依赖、窄私有测试适配及 M3 组合验收/定向退役要求，沿用其余[现役边界](../../specs/module-boundaries/spec.md)，不修改已有 requirement。
- **保持行为**：[world](../../specs/world/spec.md)、[renderer](../../specs/renderer/spec.md)、[platform](../../specs/platform/spec.md)、[foundation-files](../../specs/foundation-files/spec.md)不另造平行 delta；[simulation 现役基线](../../specs/simulation/spec.md)及其[T4 增量](../archive/2026-10-19-m3-t4-application-simulation/specs/simulation/spec.md)的业务语义不由 T5 重写。

## Impact

直接影响 ECS 公开头/实现/CMake、simulation 的装配与遍历、app 的必要调用、根测试和 EnTT 许可部署。仅当核对确认依赖变化时定向调整 foundation 或边界规则；不将整个 foundation 纳入重写。

T4 的[设计与交接](../archive/2026-10-19-m3-t4-application-simulation/design.md)、[候选验证身份](../archive/2026-10-19-m3-t4-application-simulation/verification.md#2026-10-15-完整生产迁移)是必要依赖。F 审核目录尚未合入 E 的完整 T4 生产代码；独立 Vulkan 候选也不等于本目录已具备双现代组合。实施前须确认采用的源码/配置身份和交接授权，不自动合并其他工作树。T3 仍按其[接管目标](../archive/2026-10-19-m3-t3-renderer/specs/renderer/spec.md)独立收口；T5 不接管或豁免 R4/T3 的剩余批准。

采用本地固定官方发布源码，初步候选 EnTT `v4.0.0`，不修改上游源码、不在配置时追踪远端最新版。完整 commit/摘要/许可证、统一配置及关键适配结果须在生产替换前核对并冻结；候选不兼容时报告冲突，不静默削弱契约。目标机制、具体检查和唯一活动任务分别见 [design](design.md) 与 [tasks](tasks.md)。
