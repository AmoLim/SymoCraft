---
tags:
  - area/architecture
---

# Simulation

`symocraft_simulation` 是当前 CPU 静态模块，包含玩家输入规则、Camera 借用门面、Transform/Character 系统、旧 Physics 和射线交互。公开行为唯一正文为 [simulation spec](../openspec/specs/simulation/spec.md)，世界查询归 [world spec](../openspec/specs/world/spec.md)。本文保留支持这些行为的机制与未覆盖限制，不把 [T4 design](../openspec/changes/m3-t4-application-simulation/design.md) 的会话、physics-core、事务预检和自有输出反填为现役。

## 所有权与组合

模块仅公开 CPU 值/Registry/显式 World 参数，依赖 foundation、scene、ecs、world，不依赖 Window、Renderer、app 或 SDK。命名空间函数同步使用参数，不保存 World/平台指针，也不承担应用调度。当前 Physics 仍编入 simulation，没有独立 physics target；不存在 SimulationSession 或已实现多会话预算隔离。

Registry 拥有 Transform、RigidBody、HitBox、CharacterComponent、PlayerComponent 及玩家/相机实体。app 拥有选材状态 `PlayerInputState`、选材与放置冷却并执行系统顺序。PlayerIntent 是按键/指针值；InteractionResult 的 selection 与整数 World::EditRequest 是自有 optional 值，不借用世界、组件或 GPU。编辑请求没有世界版本前置条件，不代表已经提交成功，也不是异步队列。

组件都是旧 ECS POD：Transform 保存 position/scale、以度为单位的 yaw/pitch 和 front/right/up；RigidBody 保存 velocity/acceleration 与 grounded/sensor/gravity；HitBox 保存绝对 size/offset，不由 Transform.scale 缩放。`zero_forces` 同时清 velocity 和 acceleration，名字不表示只清加速度。零填充组件不是所有算法都可直接使用的有效姿态/形状，创建者仍须完成初始化。

CreatePlayer 创建五类组件，从同 Registry 的 Camera Transform 初始化位置/姿态；身体尺寸 0.55 × 1.8 × 0.55，眼偏移 0.65，正常速度 4.4、跑速 6.2、起跳 7.6、下落加速度 -25。Registry/Camera/玩家必须属于同一活调用链，类型按约定预注册。分配或中途失败可能留下实体/部分组件，不提供完整创建回滚；底层 [ECS 限制](ecs.md) 仍适用。

依据：[目标](../../game/modules/simulation/CMakeLists.txt)、[组件](../../game/modules/simulation/include/symocraft/simulation/component.h)、[玩家值与创建](../../game/modules/simulation/src/player.cpp)、[交互值](../../game/modules/simulation/include/symocraft/simulation/playercontroller.h)。

## 每帧规则与零步语义

普通游戏的当前顺序由 app 控制：Capture 键状态 → 逐项 ApplyPointerInput → ApplyInput 一次 → TransformSystem 一次 → Character 一次 → Physics 的零至八子步 → 越界重生（若需要）→ SyncCamera → 射线/编辑提交 → mesh/cache → 渲染。放置冷却由 app 每帧递减；Benchmark 脚本编辑在物理前，普通交互编辑在物理后，不可为了“统一”而换序。

指针事件保持 right/up 正向；每项 yaw += dx × 0.05，pitch += dy × 0.05 后裁剪 -89..89，滚轮逐项更新 FOV。先裁剪后接受下一项，不能合并成总量而改变手感。app 已逐项处理后 MapInput 不再填汇总指针，避免重复。ApplyInput 写 run/sensor/gravity/方向与起跳意图，减选材冷却，0..7 环回，到期切换设置 0.2 秒；对向键优先 forward/right，同时 next/previous 时先 next 生效。

TransformSystem 从角度计算并归一化方向，再 cross/normalize 得 right/up；不验证任意非有限值或极点姿态。Character 的水平期望速度按 yaw 旋转、对角归一；有重力时保留竖向物理速度，sensor 模式可使用竖向轴。只有竖向 sensor 输入时原速度仍是 ±1，而不是额外乘基本速度；混合横向时按向量长度整体归一，属于已存在行为。

Character 在物理前即消费 `apply_jump_force`：有效 grounded/gravity/非 sensor 起跳立即写上升速度、清竖向加速度、清 on_ground、置 is_jumping，然后清 pending。这在 Physics 返回零步时也发生；位置积分仍不推进。每帧多物理步不重复输入、选材、指针或起跳。下落且竖向速度非正时设置 down_jump_force 并清 is_jumping；sensor 清 grounded/jump/竖向加速度。

依据：[玩家规则](../../game/modules/simulation/src/player.cpp)、[Transform](../../game/modules/simulation/src/transform_system.cpp)、[Character](../../game/modules/simulation/src/character_system.cpp)、[现行调用顺序](../../game/app/src/application.cpp)。

## 共享预算与碰撞机制

`physics_system.cpp` 的 TU 静态 FixedStepBudget 是进程共享状态；一次 Update 先 Consume 一次，再按“子步 → 匹配实体”遍历。不能每实体 Consume，也不能同时或交错推进两个独立会话而声称隔离。ResetTiming 只清这一共享余数，不清组件，不是异步取消。

FixedStepBudget 余数为 double，入口为 float，步长 1/120 秒、最多 8 步。非有限/非正 dt 返回 0 且不改余数；正有限输入只贡献 `min(dt, Step*8)`，加 1e-9 计算步数后减消耗、余数下界为 0。长帧不留下无界追赶积压，也不补足全部丢失时间。app 另将帧 dt 裁剪 0..0.1 秒；暂停/恢复/重生清共享预算，恢复首帧 dt=0。

每个物理子步依次按旧速度积分位置 → 加 acceleration → 减重力 (0,20,0) → 各轴速度裁剪 ±50 → 碰撞。sensor 仍积分和限速，只清 on_ground 并跳过碰撞。不是连续碰撞检测，也没有对任意形状、超大/非有限坐标的完整预检。

静态体素扫描按 y 降序、x/z 升序，中心来自 position+offset，使用单位块 AABB。严格 overlap 不把仅接触面当作穿透；膨胀静态盒按象限求各轴修正，严格最小 x/y 分支之外落到 z，包括 x/y 最小值相等而 z 并非最小的旧行为。修正与 velocity 点积小于 0 表示已移出，跳过修正；否则扣重叠并清碰撞轴 velocity/acceleration，BOTTOM 接地。随后向下/静止身体以 0.002 容差查询脚下支撑，保持精确贴地，移除支撑后下一物理步失去接地。

World 查询显式同步只读：域外、高度外、空气或 nonsolid 不碰撞；Found 块若缺定义则抛 logic_error，不伪装空气。查询 callback 不逃逸。Update 已先消费预算并原地更新组件；世界查询或后续实体失败前，预算、先前实体/子步和当前实体都可能已改变，没有 T4 的单实体候选/整体预检失败保证。

依据：[预算/DDA/纯数学](../../game/modules/simulation/include/symocraft/simulation/player_math.h)、[旧物理实现](../../game/modules/simulation/src/physics_system.cpp)。

## 射线、编辑与相机借用

单位网格 DDA 归一化方向，先查起点；负方向整边界先进入相邻负格。等距离跨轴同时推进，不命中仅边/角接触格，法线按 X、Y、Z 优先取首个跨轴；距离端点含等号，起点已在实心块时 normal 可为零。非有限 origin/direction/长度、零方向、负/非有限距离，或超出 1,000,000 坐标/距离保护界返回未命中，不查询世界。公开旧 RayCastStatic 的 `draw` 参数无作用；结果仅 hit=true 时命中字段有意义。

DoRayCast 在眼位置查 3 米，禁用输入仍可返回 selection；允许且冷却到期时 place 优先 remove。放置用 TryToBlockCoord 形成整数请求，检查库存范围、法线非零、高度/域内、目标 nonsolid 与实体 AABB 不重叠；缺定义抛错。放置正常拒绝（如重叠/非法库存）仍设置 0.2 秒，不退回 remove；查询异常发生在冷却赋值前则不保证该赋值已完成。miss 不改变冷却。app 在同帧同 World 上 TryEdit，随后才更新 mesh；不能保存请求后假定世界未变。

Camera 自有 FOV=45，但只借用 Registry 与其中 Transform 的 ID。构造创建相机实体，width/height 参数不缓存；析构不销毁实体，隐式复制/移动构造仍别名同 Registry/实体，引用成员使赋值不可用。FOV 对有限 Scroll 裁剪 1..45；姿态 setter 直接写值，不重新计算方向，也不统一防 NaN。view 返回 lookAt 值，projection 用调用者 aspect、0.1..2000 clipping；不能用 Window 的保护性 aspect 推定零像素可渲染。

SyncCamera 对相机 HasTransform 检查不是完整代数 ID 防护；遍历所有 PlayerComponent 将姿态/位置+眼偏移写同一个相机，多玩家会由后遍历者覆盖，不提供玩家选择协议。现代渲染由 app 显式转换为 position/front/up/FOV 弧度及 clipping 值，后端用实际 extent 求 aspect；旧 GL 则接收矩阵值。Renderer 不读取 ECS 或 Camera 对象，见 [renderer](renderer.md)。

依据：[交互](../../game/modules/simulation/src/playercontroller.cpp)、[Camera](../../game/modules/simulation/src/Camera.cpp)、[现代值转换](../../game/app/src/render_conversion.cpp)。

## 保留的不变量与验证边界

| 原编号 | 当前成立前提 |
| --- | --- |
| S1 | PlayerIntent/InteractionResult 是值，不持平台、世界或 GPU 指针。 |
| S2 | 有效玩家、正常初始状态和有限输入下，选材 0..7、pitch -89..89。 |
| S3 | DoRayCast 只形成 selection/edit 值，World 提交由 app 负责。 |
| S4 | 一次预算 Consume 返回 0..8、步长 1/120；非有限/非正输入不改余数。 |
| S5 | 组件引用只在当前同步查询、结构稳定时使用；禁止并发、遍历中结构修改/Free。 |
| C1 | Camera 使用期 Registry 存活，所属相机 Transform 未移除。 |
| C2 | Camera FOV 在构造及有限滚轮输入后保持 1..45 度。 |
| C3 | 每次 projection 的当前有效 aspect 由调用者提供，不缓存窗口尺寸。 |

[player_math_tests](../../test/unit/simulation/player_math_tests.cpp) 与 [simulation_contract_tests](../../test/unit/simulation/simulation_contract_tests.cpp) 检查纯数学、显式输入、逐事件裁剪、Camera 和延后编辑；[15 项旧物理基线](../../test/unit/simulation/legacy_physics_baseline_tests.cpp)、[9 项旧玩法基线](../../test/unit/simulation/legacy_gameplay_baseline_tests.cpp) 固定积分/扫描/平局/零步起跳等现有语义。夹具刻意在用例间重置共享预算，不能作为双会话实现证据。

本轮只静态核对源码、CMake、公开头与这些测试，没有执行 CPU 或 GPU。历史基线由 [VERIFICATION](../verification/VERIFICATION.md) 定位；完整 T4 未获实施入口，局部 CPU 授权/基线不等于会话迁移交付，D02 独立物理 target 与 Q01-Q04 等目标状态只在 [T4 decisions](../openspec/changes/m3-t4-application-simulation/decisions.md) 与 tasks 维护。
