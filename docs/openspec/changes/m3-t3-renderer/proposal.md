---
tags:
  - area/spec
---

# M3-T3 Renderer 接管

## Why

将已批准的 D3D12 先行、随后 Vulkan 的接管意图与未关闭验收迁入一份活动 change，避免旧流水账把实验、设计冻结、候选交付和整体验收混为一谈。
依据为 [项目范围](../../../scope/project-scope.md)、[现役 renderer](../../specs/renderer/spec.md)、[renderer 机制](../../../architecture/renderer.md) 与 [验证/批准索引](../../../verification/VERIFICATION.md)。
已预读含 scenarios 的现役六能力：[renderer](../../specs/renderer/spec.md)、[module-boundaries](../../specs/module-boundaries/spec.md)、[world](../../specs/world/spec.md)、[platform](../../specs/platform/spec.md)、[simulation](../../specs/simulation/spec.md)、[foundation-files](../../specs/foundation-files/spec.md)。最小覆盖不证明其他行为不存在。
[文档迁移](../archive/2026-10-11-docs-openspec-migration/design.md)、[暂停 Fix1](../m3-t2-publication-fix1/proposal.md)及[T4](../m3-t4-application-simulation/proposal.md)未与 T3 交接冲突；T4 生命周期目标、文件发布缺口及其他任务仍由各自 change 管理。

## What Changes

保留 R1–R4 及 R1-E01–E03、R-I1–R-I6、A01–A19、T3-M01–M15 的原义，按现役行为与接管目标分开表达。当前已实现 CPU PImpl、私有 D3D12、统一游戏转接/持久缓存和 D/R `002` 候选；R3 整组、人工玩法、默认切换与 Vulkan 尚未完成。

继续由稳定游戏侧 Renderer v1 接收 CPU 场景输入，落实现代后端的真实设备/资源/同步/呈现/诊断；D3D12 接管获批后才能切默认，再实施与独立验证 Vulkan，双现代均完成才申请 T3 通过。GL 只保留必要过渡回归，最终退役另获授权。

不新增 D3D11、热切换、插件 ABI、渲染线程、通用 RHI/Render Graph、光追、新透明排序、流式世界或 micro voxel 实体外表。此次只迁移文档，不执行实现、sync/archive、GPU、人工验收或 Git 操作。

## Capabilities

- Modified `renderer`：复用现役公开行为，delta 完整修改后端可用性 requirement，增加逐后端实验、A01–A19 接管门槛及双现代完成边界；不把 Vulkan/默认切换发布到现役 specs。
- 现役 `renderer` 基线是经静态核对的最小已实现契约，不是由本 change sync 得到，也不宣称 R3 通过。

## Impact

后续代码范围为 `game/modules/renderer`、必要 platform 私有桥接、`game/app` 转接、中立 telemetry、根级 `test`、后端 shader/CMake/安装与 `tools/benchmark` 兼容消费者；CPU world/scene 已交付语义保持。
依赖 T0 模块边界、T1 callback-only 网格交接、T2 SDL3/像素/桥接与节点批准；这些能力不因本 proposal 成立而重验通过。R1 冻结的是公开 v1 与 target 设计，不是完整私有 RHI。
当前缺口及唯一计划见 [tasks](tasks.md)，选择及代价见 [decisions](decisions.md)，原验收意图恢复来源为 [保留旧 T3](../../../spec/M3/M3-T3-渲染器重构.md) 和 [CPU 交接清单](../../../spec/M3/M3-T1-T3-CPU契约清单.md)，二者不再作为平行活动计划。
