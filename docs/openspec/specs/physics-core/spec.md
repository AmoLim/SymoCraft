---
tags:
  - area/spec
---

# physics-core Specification

> 2026-10-19：T4已验收并同步本页要求；Purpose保留原编写时点。交付范围与保留风险见[T4记录](../../changes/archive/2026-10-19-m3-t4-application-simulation/verification.md#2026-10-19-t4整体完成批准)。后续用户明确授权后，T4生产代码及必要构建、测试配套已同步F主工作区；不是Git提交/合并或T5实施授权。

> 2026-11-16：用户已独立验收正式 physics 迁移，并授权同步本页增量；实现与验证代码根为 `G:/Sandbox/Worktrees/promote-physics-module/Symocraft`，主工作区生产代码尚未传播。当前 owner/依赖契约见下方 Requirements，历史 T4 结论及 Purpose 起点描述保留，证据与未测范围见 [physics 验收记录](../../changes/promote-physics-module/verification.md)。

## Purpose

建立 D01 方案2的真实内部 CPU 核心，不是已实现的正式 physics 模块。算法现状见[simulation基线](../simulation/spec.md)，机制见[design](../../changes/archive/2026-10-19-m3-t4-application-simulation/design.md)。

## Requirements

### Requirement: Independent neutral physics core

symocraft_physics MUST 由 `game/modules/physics` 唯一拥有生产实现、公开头及 STATIC target 定义，以 `symocraft/physics` 公开路径及 `SymoCraft::Physics` 命名空间仅依赖标准库/foundation 数学；不得直接/传递依赖 ECS/World/scene/platform/renderer/assets/telemetry/app/图形 SDK、profiling/Tracy 或 document_io/YAML，不取得 simulation include/src 根。核心 SHALL 不遍历实体、不编辑世界、不累计帧预算，不建立第二运动权威或两套算法。simulation SHALL 继续负责固定步预算、ECS 有序遍历、World 只读查询适配和单体求解成功后的逐实体提交，保持既有借用寿命、自有结果及失败部分提交语义。正式迁移 MUST 保持 target 名、公开 include、命名空间、类型/签名/默认值及物理行为，旧路径不得保留活动生产 owner、实现副本或兼容转发层。

D02 的正式模块搬迁 SHALL 在 T5 审计后、NPC 复用前作为独立授权/验收节点完成；本节点承接 M3-T4-A23，不改变 T4 当时的完成门槛或归档中 A23 后续不适用的历史结论。

#### Scenario: M3-T4-A01 A02 A03 independent consumer
- **WHEN** Debug/Release 独立核心消费者、CPU-only/现役独立消费者/关闭测试或负向 include/link 检查执行
- **THEN** 实际核心可无 World/Registry/窗口运行，越界依赖被拒绝，测试链接唯一生产实现而非复制 cpp；关闭测试不引入 fixture，旧工具的历史验证身份保留而不恢复已退役 benchmark 产品

#### Scenario: M3-T4-A23 later formal ownership move
- **WHEN** 在 T5 后、NPC 复用前验证正式 physics 迁移独立节点
- **THEN** 唯一源码及 target 定义 owner 为 `game/modules/physics`，namespace/include/target/接口及行为保持；核对迁移前后核心源码与公开头字节，使用同工具链及等价配置/输入分别运行既有核心、玩家与会话回归，并通过 Debug/Release、CPU-only、关闭测试、公开头、独立消费者、负向边界与生产游戏最终链接验证
- **AND** 方案 A SHALL 保持 simulation 的固定步、碰撞、射线、失败部分提交及玩家编辑时点场景和 D12 阈值；同构建两会话配对不单独作为迁移前后等价证明。失败/禁用/未执行及 GPU 运行未覆盖分别报告，用户独立验收后才解除 NPC 真实刚体复用前置；不把本节点结果回写为 T4 当时 A23 已通过

### Requirement: Borrowed query and per-body result

核心 SHALL 接受有限 BodyState、正尺寸/有限偏移的世界轴对齐 AabbShape、按值配置/固定步长和 D03 const上下文+函数指针窄查询；查询回调必须有效，可空上下文用于无状态实现，只同步借用。核心 SHALL 返回小型自有 StepResult/RayHit，失败不交可误提交结果；错误中立具名且不携带 ECSID/存储指针。false SHALL 仅表示正常非碰撞，不吞查询错误。simulation 适配 SHALL 保留单步成功后才提交该实体的 D04边界。

#### Scenario: M3-T4-A13 A14 validation and fault
- **WHEN** 数据/形状/步长/绑定无效或查询抛错
- **THEN** 具名拒绝/失败并保持输入值，适配不提交失败实体；前序实体/子步仍保留，不假设全批暂存或整帧回滚

### Requirement: Preserved integration collision and ray behavior

核心 SHALL 保留位置→加速度→重力→逐轴限速→碰撞/接地、sensor清接地/跳碰撞、旧扫描/平局/容差及射线DDA边界/法线；Aabb 不静默使用完整Transform缩放。World适配 SHALL 将域外/高度越界为非碰撞、未定义方块为错误，读取当前方块不等网格更新；无命中为中立值且无 draw 副作用。不借提取改积分器、CCD或高速穿透能力。

#### Scenario: M3-T4-A10 A11 A12 existing algorithm fixtures
- **WHEN** 核心夹具验证 sensor、地面/墙角/低顶、支撑移除、负坐标/区块边界及射线
- **THEN** 按旧基线核对积分/纠正/速度清理/接地/命中，真实核心与 World适配分别验证；现有算法局限明示，不以重构声明性能或新碰撞能力
