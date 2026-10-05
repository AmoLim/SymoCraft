---
type: 功能
status: 已验证（T0范围）
project: Symocraft
module: simulation
created: 2026-10-05
tags:
  - area/architecture
---

# Simulation 玩家与玩法推进

关联类：[模拟数据与系统](Simulation-类设计.md)、[Camera](Camera-类设计.md)。关联规格：[M3-T0](../../../spec/M3-T0-模块软硬边界.md)。

最终验证：[M3-T0 交付与验收报告](../../../milestones/m3-t0/README.md)。

## 当前设计

`symocraft_simulation` 负责玩家初始化、输入意图解释、角色运动、固定步碰撞、相机姿态和射线交互规则。它借用 ECS 存储，通过 world 公开能力查询方块，不接收 GLFW 对象、Application 单例或 GPU 批次。

组件与系统由旧 ECS/Systems、camera、playercontroller 和 World::CreatePlayer 迁入；存储实现仍由 ecs 拥有。T0 不重写 DDA、碰撞算法或旧容器，也不声称完成后续 T3/T4 的完整生命周期设计。

## 本次变更

### 目标与流程

```text
platform 原始键鼠快照 -> app 映射 -> PlayerIntent
platform 有序指针事件 -> app 逐项传递 -> ApplyPointerInput
ApplyInput -> 组件移动轴/重力/跳跃/视角/选材状态
TransformSystem + Character::Player + Physics -> 更新位置/速度
SyncCamera(registry,cameraEntity) -> 相机实体姿态
DoRayCast -> Selection + 可选 BlockEdit 值
app -> world 提交编辑 -> renderer 接收选择框位置
```

`CreatePlayer(registry,camera)` 返回实体 ID，由 app 保存。Camera 显式借用 Registry；相机同步显式指定目标 ID，消除内部寻找全局应用的行为。`ApplyInput` 管理选材 0.2 秒间隔；`DoRayCast` 对外部传入的放置/移除冷却值设置原 0.2 秒间隔，应用仍每帧递减一次。

### 关键约束与取舍

- 玩家、相机组件均归 Registry 持有。模拟只在调用期间借用组件引用，Camera 仅保存 Registry 引用和实体 ID。
- 平台只产生输入事实；跑步、无碰撞模式、接地跳跃、选材、俯仰角约束全部留在 simulation，窗口回调不再直接修改 ECS。
- 鼠标和滚轮保持事件顺序逐项应用，保留原逐事件 pitch/FOV 裁剪；不能把相反方向事件相加后只裁剪一次，否则到达视角上限时会改变结果。
- 射线交互返回自有值，不在内部写世界或调用 renderer。应用必须在同帧、世界未被其他流程改变时提交编辑；这是当前单线程主循环的契约，不是异步编辑队列。
- 右键与左键优先级、3 格射程、实体碰撞盒放置拒绝、0.2 秒冷却保持原逻辑；基准模式传入 `allow_input=false`，仍可产生选择框。
- 不产生每帧动态任务或分配网格。输入和交互结果为小型值类型，组件就地更新。
- 固定步仍为 120Hz，每帧最多 8 步；预算为原模块私有状态，不保证多个世界/会话同时运行或并发调用。

### 验收案例

| 状态 | 场景 | 预期行为 |
| --- | --- | --- |
| [x] | FixedStepBudget、移动方向、地面检测、DDA | 保留已有数学边界与输出 |
| [x] | 玩家/相机构造，视角与缩放输入 | 显式 Registry 能独立驱动，不需要窗口；反向指针事件保持逐项裁剪 |
| [x] | 无输入/选块/放置撞人/移除 | selection/edit 值正确，DoRayCast 不直接改世界 |
| [x] | 窗口失焦再恢复后真实玩法 | 用户确认本轮安装包移动跳跃、视角、放破、切出切回及最小化恢复正常 |
| [x] | CPU-only、公开头与依赖负向检查 | 不可依赖 platform、renderer 或 app |

实现：[模块构建](../../../../game/modules/simulation/CMakeLists.txt)、[输入与玩家](../../../../game/modules/simulation/src/player.cpp)、[交互](../../../../game/modules/simulation/src/playercontroller.cpp)。2026-10-05 最终 Debug、Release、CPU-only 的数学与模拟契约测试通过，日志入口见 [world-simulation-debug.log](../../../milestones/m3-t0/evidence/world-simulation-debug.log)及最终报告。用户另对 `out/m3-t0/install/full-release/SymoCraft.exe` 确认移动跳跃、视角、放破、切出切回、最小化恢复和正常退出均正常；这是迁移后人工反馈，不是自动输入回放或长时稳定性证明。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| M3-T3 | 实例化模拟会话/时间预算，完善输入事件与相机生命周期设计 |
| M3-T4 | 审计 EntityId 版本和组件引用失效，不能由 Camera 借用规则代替存储正确性 |
| 异步编辑或回放 | 增加世界版本/前置条件，不让旧编辑结果在新世界静默执行 |
