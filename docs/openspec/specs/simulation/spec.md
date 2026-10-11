---
tags:
  - area/spec
---

# Simulation

## Purpose

最小现役基线，只覆盖现有生产 CPU 入口，不把 T4 的 SimulationSession/独立 physics/中立异常写成已实现。
已核对 [Physics](../../../../game/modules/simulation/src/physics_system.cpp)、[预算/射线数学](../../../../game/modules/simulation/include/symocraft/simulation/player_math.h)、[角色规则](../../../../game/modules/simulation/src/character_system.cpp)、[输入](../../../../game/modules/simulation/src/player.cpp)、[交互](../../../../game/modules/simulation/src/playercontroller.cpp)和[应用顺序](../../../../game/app/src/application.cpp)。
历史 CPU 基线为 15 物理 + 9 玩法组、Debug/Release 各重复三次；身份、局限及批准从 [VERIFICATION](../../../verification/VERIFICATION.md) 进入。本轮仅源码核对，未重跑构建/测试/游戏。
目标只在 [T4 change](../../changes/m3-t4-application-simulation/proposal.md)；覆盖不完整不证明其他行为不存在。

## Requirements

### Requirement: Explicit simulation storage and timing

当前 Physics::Update SHALL 同步消费调用方提供的 Registry、只读 World 和 float 秒，不自行查找 Application/窗口。物理 SHALL 使用现有共享静态固定步预算：120 Hz、单次最多 8 步，正有限时间按最多 8/120 秒累计并保留余数；非有限/非正时间不消费预算。ResetTiming SHALL 清共享余数。当前入口不提供会话隔离或整次推进回滚保证，调用方 MUST 串行使用并维持借用对象寿命。

#### Scenario: Existing zero and long frame budget
- **WHEN** Physics::Update 收到零时间、正长帧或不足一固定步的时间
- **THEN** 零时间不强行补步或清余数；正长帧最多执行 8 步，短时间余数留到后续；预算不是每个实体各消费一次

#### Scenario: Existing shared timing reset
- **WHEN** 普通 app 在加载结束、暂停/恢复或安全出生点恢复时调用 ResetTiming
- **THEN** 旧共享余数清零；不能据此声称不同 Registry 的同时会话独立

### Requirement: Existing character and pointer rules

角色规则 SHALL 与物理预算分开：应用每帧先处理有序指针/意图、更新朝向与角色规则，再执行零到多物理步。指针 SHALL 逐事件修改 yaw/pitch/FOV，保持 pitch [-89,89]、原灵敏度和镜头限制，不把相反事件汇总后再裁剪。有效跳跃 SHALL 在角色规则阶段立即改向上速度、清接地/待跳并设跳跃状态，即使本次物理为零步；位置只由后续正常物理步推进。当前镜头仍有 ECS 相机实体，尚非 T4 单一姿态模型。

#### Scenario: Accepted jump with zero physical steps
- **WHEN** 重力角色已接地、非 sensor 且接受一次有效跳跃，本次预算为零步
- **THEN** 跳跃状态/速度立即生效且请求消费，不延迟或补物理步，不新增落地前预输入/离地宽限

#### Scenario: Ordered pointer clipping and input priority
- **WHEN** 多个正反指针事件依次输入，或前后/左右同时按下
- **THEN** 指针按次序裁剪，意图保留前优先于后、右优先于左的既有规则，不重复同时应用事件和汇总增量

### Requirement: Existing voxel movement and query semantics

每物理步 SHALL 保持位置推进→加速度→重力→逐轴限速→碰撞/接地的顺序；sensor SHALL 跳过碰撞并清接地。碰撞 SHALL 保留 y 降序、x/z 升序扫描、严格重叠与既有轴平局处理、速度/加速度清理及接地容差，不宣称 CCD 或高速穿透已解决。世界适配 SHALL 将 OutsideWorld/高度越域解释为非碰撞，未定义方块抛错误而不是空气。当前运动可能在后续查询失败前已原地改写，不具有 T4 单实体值提交保证。

RayCastStatic SHALL 产生自有命中值，保持单位格 DDA 的等边界同时跨越、法线与最大距离规则；非法射线参数/无命中返回无命中值。旧 draw 参数不产生绘制副作用，不把这个旧无命中出口当未来具名参数拒绝保证。

#### Scenario: Sensor and current support
- **WHEN** sensor 运动，或当前方块支撑被移除后执行真实物理步
- **THEN** sensor 不碰撞且不保留接地；非 sensor 按当前 World 重新查支撑，不依赖 GPU 网格重建

#### Scenario: Ray boundaries and query failure
- **WHEN** 射线从整数边界向负方向出发、同时跨越多个轴、无命中或读取未定义方块
- **THEN** 保持现有格/法线/距离及无命中语义，查询错误传播，不伪造命中或将坏定义吞为空气

### Requirement: Deferred ordinary world edits

DoRayCast SHALL 读取指定 World/Registry 并返回选择及可选自有 EditRequest，不直接修改 World。普通 app SHALL 在物理及相机同步后生成请求，再由 World 校验提交；benchmark edit SHALL 保持模拟前提交。交互 SHALL 保留射线距离 3、材料库存、放置与玩家 AABB 重叠保护、放置优先于移除及既有 0.2 秒冷却；已有命中且进入编辑分支的拒绝仍消费冷却，不把冷却仅限成功 Changed。

#### Scenario: Rejected placement and world authority
- **WHEN** 命中后尝试非法材料/重叠放置，或 World 拒绝请求
- **THEN** 交互不绕过 World 提交，原编辑冷却仍成立；无命中/不允许交互不伪造编辑请求

#### Scenario: Ordinary and benchmark edit order
- **WHEN** 普通游玩或既有 edit workload 执行一次编辑
- **THEN** 前者在模拟后，后者在模拟前；接口整理不统一到另一时点或新增模拟内 World 编辑
