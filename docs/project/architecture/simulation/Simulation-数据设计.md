---
type: 数据设计
status: 自动验证完成（T1适配）
project: Symocraft
module: simulation
created: 2026-10-05
updated: 2026-10-06
tags:
  - area/architecture
---

# Simulation 输入、组件与交互数据

关联：[功能](Simulation-功能.md)、[系统流程](Simulation-系统设计.md)、[对象覆盖清单](../对象笔记覆盖清单.md)。

## 当前设计

组件保存状态，Registry 拥有组件池；app 拥有输入状态及交互冷却，系统只同步借用。原平台无关输入/自有交互结果的 T0 迁移已实现，不再放本次变更。没有 System 基类或隐含存档格式。

### 工作负载与布局

每帧将平台快照映射为值，系统按 RegistryViewer 查询多组件并原地更新；Physics 先消费帧预算，再执行固定子步。组件按类型存在私有连续字节池，并以稀疏索引查找，不能据此称为 archetype 或经过 SoA 优化。各值的规模、缓存代价和布局瓶颈未测量。

### PlayerIntent

bool：run/sensor/descend/forward/backward/right/left/jump/next_block/previous_block，默认 false；double mouse_dx/mouse_dy/scroll_y 默认 0，向右/向上为正。自有输入事实，不保存平台 handle。普通输入的有序指针事件逐项应用，汇总量留零避免双算。

### PlayerInputState

int selected_block=0；float block_change_debounce=0 秒；app 持有，ApplyInput 更新。正常有效路径选材索引 0..7，不能把任意外部非法值写成已验证输入。

### Transform

position/scale、yaw/pitch（角度）、front/up/right 为自有数学值；创建玩家时显式初始化，TransformSystem 更新方向。头文件未提供普遍默认初始化保证。

### RigidBody

Physics::RigidBody：velocity/acceleration、on_ground/is_sensor/use_gravity。Physics 原地写回；zero_forces 同时清速度与加速度，不只是清力。

### HitBox

Physics::HitBox：size/offset；由玩家创建初始化，碰撞遍历读取。没有世界或 GPU 指针。

### CharacterComponent

Character::CharacterComponent：base_speed/run_speed/jump_force/down_jump_force、movement_axis（前后/上下/左右）、is_running/apply_jump_force/is_jumping。Character 每帧计算速度/跳跃意图；沿用旧玩法参数，非独立资源对象。

### PlayerComponent

Character::PlayerComponent：movement_sensitivity、camera_offset；输入与 SyncCamera 消费。与其他组件一样由 Registry 拥有，系统不析构它。

### InteractionInput

PlayerController::InteractionInput：allow_input 默认 true，place/remove 默认 false，selected_block=0；自有值，禁输入路径不生成编辑请求。

### World EditRequest

`World::EditRequest`：EditOperation Set/Remove、整数 BlockCoord position、BlockId id（Remove 不使用 Set ID）。由 world 公共值定义，simulation 只形成自有请求；它不是已提交事务或携带前置版本的命令。浮点命中中心经统一 TryToBlockCoord 转换，转换失败不生成有效编辑。

### InteractionResult

两个 optional：selection（vec3 选择框中心）、edit（World::EditRequest）；默认空，无世界内部地址。app 在传给 simulation 的同一实例上同帧 TryEdit，renderer 消费 selection 副本；拒绝与 Unchanged/Changed 状态由 world 返回，不由请求本身保证成功。

### RaycastStaticResult

point/block_center/block_size/hit_normal 与 bool hit；Physics::RayCastStatic 返回值，不借用 Block。使用命中字段以前检查 hit；不假定未命中所有字段均具意义。

### VoxelRayHit

PlayerMath::VoxelRayHit：hit=false、cell=0、normal=0、distance=0；单位网格 DDA 结果，距离按归一化方向计算，非法/非有限输入返回未命中。边/角同时跨越不算仅边接触格命中。

### CollisionInfo

私有 Physics::CollisionInfo：vec3 overlap_part、Direction force、bool did_collision；Direction 为 NONE/TOP/BOTTOM/FRONT/BACK/LEFT/RIGHT。仅当前碰撞计算有效，不持有组件/世界所有权，规则见[实现](../../../../game/modules/simulation/src/physics_system.cpp)。

### 不变量与失效

| 原编号 | 条件与边界 |
| --- | --- |
| S1 | PlayerIntent/InteractionResult 不持有平台、世界或 GPU 指针；类型与跨模块传递边界 |
| S5 | 组件引用仅当前同步查询内使用；调用者禁止并发/遍历中结构修改；底层池未扩容、删除或释放 |

S2/S3 归[系统笔记](Simulation-系统设计.md#不变量)，S4 归[FixedStepBudget](FixedStepBudget-类设计.md#不变量)。底层失效与注册前提见 [ECS I2/I3](../ecs/ECS-组件存储-数据设计.md#不变量)。

输入、结果可按值复制；组件不加入析构所有权。组件注册/创建分配失败没有完整实体事务回滚，进入 app 统一清理；单线程单会话，不因私有分配器锁产生并发保证。

源码：[component.h](../../../../game/modules/simulation/include/symocraft/simulation/component.h)、[player.h](../../../../game/modules/simulation/include/symocraft/simulation/player.h)、[playercontroller.h](../../../../game/modules/simulation/include/symocraft/simulation/playercontroller.h)、[physics_system.h](../../../../game/modules/simulation/include/symocraft/simulation/physics_system.h)、[player_math.h](../../../../game/modules/simulation/include/symocraft/simulation/player_math.h)。
历史验证：[功能验收](Simulation-功能.md#验收案例)、[T0 报告](../../../milestones/m3-t0/README.md)：Debug/Release/CPU-only 契约、S1/S3 边界和用户人工玩法复查；未做多会话、异步编辑或原 T4（现 T5）引用失效审计。本次 T1 整数请求和显式查询适配的自动结果见 [T1报告](../../../milestones/m3-t1/README.md)。

### 公开接口预期行为

| 签名 / 入口 | 调用方与可见范围 | 预期行为：输出及状态变化 | 前提 / 边界 | 失败反馈及失败后状态 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| `Physics::RigidBody::zero_forces()` | struct public；app 重生等 | 同时将 velocity/acceleration 置零，不改三项 bool | 有效组件引用；没有 Registry 查找 | 无分配/错误值，不替代 timing reset | [component.h](../../../../game/modules/simulation/include/symocraft/simulation/component.h) / S5 |
| 其他本篇数据类型：不适用 | app / 系统 | 无显式成员函数；聚合/隐式复制移动数学值、bool、optional | 无资源 handle；没有字段初值的组件需显式初始化；引用失效见 S5 | 无普遍字段校验；不能以默认构造当作合法玩家 | component.h / player.h / playercontroller.h / physics_system.h / player_math.h |
| 创建/输入/交互/数学算法（入口索引） | 模块公开函数 | [系统权威行为表](Simulation-系统设计.md#公开接口预期行为) | 算法不属于这些值的方法 | 同目标表 | S1/S5 |

### 私有函数预期行为

| 签名 / 入口 | 内部调用方 | 预期行为：处理规则及副作用 | 前提 / 边界 | 失败传播及清理责任 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| 不适用：本篇数据类型无 private 函数 | 系统 | CollisionInfo 为 TU 数据，不自带算法；碰撞 helper 见 [系统表](Simulation-系统设计.md#私有函数预期行为) | 值/组件状态与算法分开维护 | Registry 负责池寿命，app 负责上层失败清理 | component.h / physics_system.cpp |

## 本次变更

T1 将原浮点 BlockEdit 替换为 world 的整数 EditRequest 值；组件、输入、命中中心和其他数学布局未改变。验证记录统一见[功能](Simulation-功能.md#本次变更)，不在数据笔记重复勾选历史测试。

## 后续考虑

多世界/回放需显式会话、编辑序号与世界版本，另立 spec；不改变本文当前数据形态。

