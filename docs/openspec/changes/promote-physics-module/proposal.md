---
tags:
  - area/spec
---

# 正式 physics 模块迁移

## Why

落实 [T4 D01/D02](../archive/2026-10-19-m3-t4-application-simulation/decisions.md) 的分阶段路线：T5 审计及生产传播已经完成，正式 physics 所有权迁移须在 NPC 复用前独立验收。提案起点的独立静态库已形成中立核心，但源码和构建定义仍嵌于 simulation，检查器为旧位置维护特殊规则。实施后的 [独立静态库](../../../../game/modules/physics/CMakeLists.txt) 由正式模块拥有；将所有权正式化并验证边界，可在新增 NPC 行为前建立清楚的回归基线。

本轮已盘点并预读全部 11 份现役规格及场景，依据 [scope](../../../scope/project-scope.md)、[physics-core](../../specs/physics-core/spec.md)、[module-boundaries](../../specs/module-boundaries/spec.md) 和 [simulation](../../specs/simulation/spec.md) 确定增量。T5 当前状态见 [验收与生产传播](../archive/2026-10-25-m3-t5-ecs-memory/verification.md)。本 change 承接 M3-T4-A23 的后续独立节点，保留 T4 归档中当时“不适用”的原义。

## What Changes

- 将唯一生产实现、公开头和构建定义从 `game/modules/simulation/physics` 迁到 `game/modules/physics`，由游戏模块组合直接注册正式 owner。
- 保留 `symocraft_physics` STATIC target、`symocraft/physics/physics.h`、`SymoCraft::Physics`、现有公开类型/签名/默认值和物理行为；核心源码与公开头按原字节搬迁。
- simulation 仍是唯一直接链接该库的生产模块，继续负责固定步预算、ECS 有序遍历、World 查询适配和逐实体成功后提交。独立根测试可通过公开头链接同一正式库。
- 更新所有权、include/link 与依赖闭包检查，以及公开头消费者、负向边界夹具和 profiling 禁止引用检查，使它们覆盖新位置。
- 用户于本聊天确认 A23 采用方案 A：构建与 CPU 回归，包含生产游戏链接；本节点不要求 D3D12/Vulkan 实机短测。

规划阶段仅生成提案与规格增量。2026-11-13 用户已在聊天授权当前托管工作树按 P1–P5 实施，完成后交用户独立验收；不新增 NPC 刚体接入、算法优化、CCD、实体间碰撞、并行调度、计算服务或新公开 API，不改 T4/T5 历史证据与批准，不做 Git 提交、主规格同步或归档。

## Capabilities

- **修改 `physics-core`：** 完整修改 `Independent neutral physics core`，明确正式唯一 owner，并将原 M3-T4-A23 场景落实为迁移后的验收要求；其余借用、错误、算法及回归场景沿用。
- **修改 `module-boundaries`：** 将 physics 纳入正式所有权，补充独占生产消费方、新路径私有保护和禁止依赖的可检查场景；保留现有 requirement 的全部有效场景。
- **复用 `simulation`：** 固定步、碰撞、射线、Rejected/Failed、部分提交、玩家编辑时点及 D12 对照原义回归，无该能力的规格增量。
- **无新增能力：** ECS、World、应用/平台/渲染、foundation 文件退役及观测契约保持；玩法不变但模块所有权及依赖契约改变，因此保留两份增量，不设置 `skip_specs: true`。

## Impact

代码影响限正式 physics 的三文件迁移、`game/CMakeLists.txt`、simulation 的 CMake 接线、`cmake/ModuleBoundaries.cmake`、`cmake/PublicHeaderTests.cmake`、`test/integration/CheckNegativeBoundaries.cmake`、`test/unit/profiling/check_configuration.py` 及核对后确需更新的活动路径引用。既有核心、玩家与会话测试继续链接生产库，不复制实现。实施时同步受影响的架构所有权说明和当前导航；历史命令、旧缓存和冻结材料不批量替换路径。

与相关 change 的交接：

- [NPC MVP](../npc-framework-harvest-mvp/design.md#正式-physics-迁移前置) 的 N1.3 及后续生产联调依赖本节点验收；设计、World 规则、动作替身和角色静态显示按各自授权推进。本轮不修改 NPC 计划或解除其门槛。
- [RenderDoc/Tracy](../integrate-renderdoc-tracy/proposal.md) 已有边界增量涉及相同所有权 requirement。以当前现役规格为底，保留 profiling、YAML 适配与旧产品退役约束；后续同步须避免旧增量覆盖新增 physics owner。埋点留在 simulation 适配层，核心不增加 Tracy 依赖。
- [环境恢复计划](../restore-windows-development-environment/proposal.md) 仍有 SDK `10.0.22621.0` 目标，与当前 AGENTS 的 `10.0.28000.0` 不同。本迁移按当前 AGENTS 的 MSVC `14.51.36231`、SDK `10.0.28000.0` 规划同工具链前后验证；如实施时权威要求变化，先明确身份并重建可比基线，不混用缓存或修改另一计划。

## Implementation

### Ownership and compatibility

迁移集合为旧根下 `CMakeLists.txt`、`src/physics.cpp`、`include/symocraft/physics/physics.h`。新模块在 foundation 之后、simulation 之前装配；simulation 去掉嵌套 `add_subdirectory(physics)`，保留 PRIVATE 链接。旧位置不保留第二 target、转发头或算法副本。

将 physics 纳入正式模块清单时，app 默认允许列表会随清单扩大，必须继续显式拒绝 app/其他生产模块直接消费 physics，包括 alias、生成器表达式绕过。最终游戏经 simulation 的静态链接闭包取得 physics 是合法组合，不等于 app 直接获得公开头使用权限。核心只使用标准库及 foundation 数学；不取得 simulation include/src、World/ECS、图形 SDK、profiling/Tracy 或 document_io/YAML 闭包。

会话串行、非重入推进；World/Registry 覆盖会话寿命。核心只同步借用查询，返回自有值；适配求解成功才提交当前实体，失败实体不提交，前序实体/子步和规则结果保留。`PhysicsStepOrder`、预算和状态所有者均留在 simulation，不因目录移动重选这些机制。

### M3-T4-A23 acceptance — option A

实施前固定工作区实际源码身份（含相关未提交输入）、工具链、配置和测试集合；前后分别使用新构建目录并保留旧结果。Debug 全依赖闭包遵守 DBG01，不能混入旧 level-2 对象。

| 范围 | 必需依据 |
| --- | --- |
| 迁移前后行为基线 | 同一 MSVC/SDK、构建选项、资产、初态和测试输入，分别运行现有核心、玩家和会话 CPU 回归；逐项对账通过、失败、禁用与未执行。核对核心 cpp/公开头的前后字节及其他差异，CMake 差异仅为所有权/接线所需。 |
| Debug / Release | 两配置均构建正式库、独立消费者及当前默认 D3D12 游戏，完成最终生产游戏链接；运行受影响 CPU 测试、公开头与负向边界检查，不启动游戏。 |
| CPU-only | 两配置独立目录验证正式核心、simulation 与独立消费者可无窗口运行，无 SDL/原生图形 SDK/Tracy 发现与链接。 |
| `BUILD_TESTING=OFF` | 两配置验证普通游戏和 CPU-only 的生产构建；无测试注册、fixture 或生产反向依赖，游戏配置完成最终链接。 |
| 边界有效性 | 正向消费者真正编译；新路径的跨 owner、重复实现、私有头/宽 include 根、app/World 直接链接、alias/表达式绕过、直接/传递非法依赖及 physics 引用 profiling 均按预期拒绝，不能因扫描为空或夹具仍测旧路径而通过。 |

行为覆盖复用 M3-T4-A10–A14、A24–A26 及 simulation 的零/多步/长帧、跳跃、sensor、墙/地/角/低顶、支撑移除、负坐标/区块边界、射线、输入裁剪、冷却/编辑拒绝、暂停恢复、双会话和失败部分提交场景。普通编辑在模拟后、既有场景编辑在模拟前；D12 离散精确与浮点固定阈值保持，不通过放宽阈值或删测试取得通过。

现有 `simulation.session` 的配对用例比较同一构建内两份会话，不能单独证明跨版本等价。方案 A 以源码/公开头未变核对、同工具链前后分别回归、当前生产边界与游戏链接共同提供迁移依据，不宣称跨版本逐帧轨迹对照或二进制逐字节相同。测试基线失败或出现行为差异须先定位并报告，不混入算法修复或隐去失败。

方案 A 不包含 GPU 运行、人工玩法短测、性能预算、长时或其他机器验证；这些未覆盖事实明确保留，不恢复旧 benchmark 产品或重跑历史 T4/T5 全套。结果在本 change 的一份 `verification.md` 记录必要身份、结论及限制，执行时再建立，不预填通过，不新增原始数据附件。

2026-11-16 用户已确认独立验收通过，授权同步对应主规格及提交本次修改；该交付的 physics 复用前置成立，主工作区生产传播、NPC 实施、推送/合并及归档不在本轮授权内。

### Tasks

- [x] **P1 建立迁移前基线。** 在取得实施授权后核对代码根、三文件集合、实际工具链及相关并发变更，按方案 A 运行前置核心/玩家/会话回归并记录身份与现有失败；保留可核对的前后源码依据。
- [x] **P2 迁移唯一生产 owner。** 原字节迁移核心与公开头、迁移构建定义并更新游戏/simulation 接线；核对接口、target、单实现及 simulation 职责保持，旧路径无活动定义或兼容副本。
- [x] **P3 更新边界与消费者覆盖。** 修改正式所有权及新路径检查，保留仅 simulation 直接消费的限制；迁移公开头、负向夹具和 profiling 禁止引用测试，逐类核对正向接受与负向拒绝确实触达新 owner。
- [x] **P4 完成 A23 方案 A。** 完成上表 Debug/Release、CPU-only、关闭测试和生产游戏链接，重跑既有核心/玩家/会话回归；对账迁移前后源码及结果，记录失败/禁用/未执行与 GPU 未覆盖，不用同构建配对冒充前后轨迹比较。
- [x] **P5 整理交接并申请独立验收。** 按实现更新受影响架构说明及活动引用，在同一 verification.md 汇总 A23 证据和限制。取得用户验收后才解除 NPC N1.3 的 physics 前置；主规格同步、归档、Git 操作按届时独立授权执行。
