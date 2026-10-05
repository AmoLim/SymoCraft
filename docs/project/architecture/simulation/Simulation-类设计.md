---
type: 类设计
status: 已验证（T0范围）
project: Symocraft
module: simulation
class_name: PlayerIntent / PlayerInputState / InteractionResult / FixedStepBudget
inheritance: []
created: 2026-10-05
tags:
  - area/architecture
---

# 模拟数据与系统类设计

关联功能：[Simulation](Simulation-功能.md)。本模块当前由 POD 组件、值类型和命名空间系统函数组成，不额外建立空的 System 基类。

最终验证：[M3-T0 交付与验收报告](../../../milestones/m3-t0/README.md)。

## 当前设计

职责：组件保存状态，系统实现规则；Registry 保存组件但不知道玩法。world 解释/保存方块，renderer 解释绘制，不进入本模块的输入类型。

### 数据成员

| 类型 | 成员 | 初值 / 范围 | 含义与所有权 |
| --- | --- | --- | --- |
| `PlayerIntent` | 跑步/模式/方向/跳跃/选材 bool | 默认 false | app 映射的当前输入事实副本 |
| `double` | `mouse_dx,mouse_dy,scroll_y` | 默认 0 | 向右/向上为正的相对运动；不保存原生鼠标句柄 |
| `PlayerInputState` | `selected_block,block_change_debounce` | 0，0 秒 | app 持有，ApplyInput 更新；选材索引 0..7 |
| `InteractionInput` | `allow_input,place,remove,selected_block` | true,false,false,0 | 当前交互意图值 |
| `InteractionResult` | `optional<vec3> selection` | 无 | 自有选择框中心，不借用世界对象 |
| `InteractionResult` | `optional<BlockEdit> edit` | 无 | 自有坐标、方块 ID、remove 标识 |
| `FixedStepBudget` | `accumulated_` | 0 秒 | 私有时间余数；当前物理系统持有单个实例 |
| `Transform` | 位置/缩放/角度/front/up/right | 玩家创建时显式初始化 | Registry 拥有；方向由 TransformSystem 计算 |
| `RigidBody/HitBox` | 速度/加速度/模式/尺寸偏移 | 玩家创建时显式初始化 | Registry 拥有；Physics 更新 |
| `CharacterComponent/PlayerComponent` | 速度、跳跃、移动轴、相机偏移 | 原玩法参数 | Registry 拥有；Character/ApplyInput 更新 |

### 不变量

| 编号 | 条件 | 成立边界 |
| --- | --- | --- |
| S1 | `PlayerIntent` 和 `InteractionResult` 不持有平台、世界或 GPU 指针 | 类型构造与跨模块传递 |
| S2 | `ApplyInput` 选材按 0..7 环回，pitch 限制 -89..89 | 有效玩家/初始状态的正常输入路径 |
| S3 | `DoRayCast` 不修改方块，只返回可提交的编辑请求 | 每次返回后 |
| S4 | 单次 `FixedStepBudget::Consume` 返回 0..8，步骤 1/120 秒 | 非有限/非正输入返回 0；超长帧限制补步 |
| S5 | 系统查询的组件引用只在当前同步调用内有效 | Registry 结构不在遍历中被另一个任务修改 |

### 接口与生命周期

| 接口 | 行为 | 前提 / 边界 |
| --- | --- | --- |
| `CreatePlayer(registry,camera)` | 创建并初始化五类玩家组件，返回 EntityId | app 已按约定注册组件，相机属于同一 Registry |
| `ApplyInput(...,intent,state,delta)` | 更新移动/模式/相机/选材 | delta 为应用裁剪后的帧间隔；只递减选材冷却 |
| `ApplyPointerInput(...,dx,dy,scroll)` | 应用一个中立指针事件并裁剪视角 | app 按原事件顺序调用；普通键盘映射将 PlayerIntent 的对应汇总值留零，避免重复应用 |
| `Character::Player::Update(registry)` | 产生速度和跳跃力 | 保留旧组件遍历/规则 |
| `Physics::Update(registry,delta)` | 固定步推进与方块碰撞 | 原私有预算，单线程单会话 |
| `Physics::ResetTiming()` | 清除补步余量 | 启动、失焦暂停、重生时由 app 调用 |
| `SyncCamera(registry,cameraEntity)` | 复制玩家姿态与相机偏移 | 无有效相机 Transform 则返回；沿用已有单玩家遍历 |
| `DoRayCast(...,float& placement_debounce)` | 选择值和编辑请求，更新冷却 | app 在同帧提交；禁输入模式不产生编辑 |

输入/结果为普通可拷贝值；组件保持原存储要求的 POD，不加入析构所有权。系统不拥有 Registry 或 world，游戏退出时由 app 先停止循环再销毁对象。组件注册/创建分配失败不提供完整实体事务回滚，应用进入统一清理；此风险留给 ECS/生命周期专项，不隐瞒为已解决。

实现：[组件](../../../../game/modules/simulation/include/symocraft/simulation/component.h)、[输入](../../../../game/modules/simulation/include/symocraft/simulation/player.h)、[交互](../../../../game/modules/simulation/include/symocraft/simulation/playercontroller.h)、[数学](../../../../game/modules/simulation/include/symocraft/simulation/player_math.h)。

## 本次变更

新增平台无关输入与自有交互结果，迁移玩家创建和输入规则；保留数值算法，删除对 Application、GLFW 和 Renderer 的反向调用。验收引用[模块功能](Simulation-功能.md#验收案例)：Debug/Release/CPU-only 模拟契约、S1/S3 的边界检查及安装包人工玩法复查均完成。人工复查来自用户明确反馈；没有进行多会话、异步编辑或 T4 引用失效审计。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| 多玩家或多世界 | 显式玩家/相机会话和每会话预算，不能沿用隐式全局步进 |
| 回放/异步编辑 | 明确命令序号、世界版本及执行失败反馈 |
