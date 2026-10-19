---
tags:
  - area/spec
---

# physics-core Specification

> 2026-10-19：T4已验收并同步本页要求；Purpose保留原编写时点。交付范围与保留风险见[T4记录](../../changes/archive/2026-10-19-m3-t4-application-simulation/verification.md#2026-10-19-t4整体完成批准)。后续用户明确授权后，T4生产代码及必要构建、测试配套已同步F主工作区；不是Git提交/合并或T5实施授权。

## Purpose

建立 D01 方案2的真实内部 CPU 核心，不是已实现的正式 physics 模块。算法现状见[simulation基线](../simulation/spec.md)，机制见[design](../../changes/archive/2026-10-19-m3-t4-application-simulation/design.md)。

## Requirements

### Requirement: Independent neutral physics core

symocraft_physics MUST 在 simulation/physics 内成为真实独立 STATIC owner，以 symocraft/physics 公开路径及 SymoCraft::Physics 命名空间仅依赖标准库/foundation数学；不得直接/传递依赖 ECS/World/scene/platform/renderer/assets/telemetry/app/图形SDK，不取得 simulation include/src 根。核心 SHALL 不遍历实体、不编辑世界、不累计帧预算，不建立第二运动权威或两套算法。D02 的正式模块搬迁 SHALL 在 T5审计后、NPC复用前另立授权/验收节点，不作为 T4门槛。

#### Scenario: M3-T4-A01 A02 A03 independent consumer
- **WHEN** Debug/Release 独立核心消费者、CPU-only/工具/关闭测试或负向 include/link 检查执行
- **THEN** 实际核心可无 World/Registry/窗口运行，越界依赖被拒绝，测试链接唯一生产实现而非复制cpp；关闭测试不引入fixture

#### Scenario: M3-T4-A23 later formal ownership move
- **WHEN** 到达 T5后、NPC复用前正式迁移独立节点
- **THEN** 搬 target 定义所有者而保持 namespace/include/target/行为及单实现，更新真实边界并重跑矩阵；本次 A23 标后续不适用，不勾通过

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
