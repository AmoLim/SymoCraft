---
tags:
  - area/architecture
---

# Simulation

`symocraft_simulation`是CPU静态模块。[SimulationSession](../../game/modules/simulation/src/session.cpp)及内部物理核心适配已随获批准的T4实现于2026-10-19同步F主工作区，不是Git历史合并或T5实施。现役行为见[simulation spec](../openspec/specs/simulation/spec.md)/[world spec](../openspec/specs/world/spec.md)，历史设计与选择保留于[T4归档](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/design.md)。

## 所有权与组合

模块仅公开CPU值/Registry/World参数，依赖foundation/scene/ecs/world，PRIVATE链接真实symocraft_physics，不依赖Window/Renderer/app/SDK。Session借稳定专属Registry及const World，拥有固定步预算/控制冷却/Lens/状态，不保存平台/观测指针或组件地址。

caller先注册五类组件，Session工厂检查注册与中立初态，在该Registry完整创建一个玩家后才发布；不创建Camera实体。失败可留下部分装配，caller丢弃专属范围，无共享Registry事务；Session析构不删除借用存储/实体。独立Session预算/选材/冷却隔离，不推出共享Registry、多窗口或并发安全。
AdvanceInput/有序PointerInput均为值；AdvanceResult自有身体pose、lens、selection、整数edit请求和本轮统计。普通caller决定是否派生视图/提交World，不借结果字段跨推进作第二运动权威。请求不带World版本条件，不代表已提交或异步队列。

组件都是旧 ECS POD：Transform 保存 position/scale、以度为单位的 yaw/pitch 和 front/right/up；RigidBody 保存 velocity/acceleration 与 grounded/sensor/gravity；HitBox 保存绝对 size/offset，不由 Transform.scale 缩放。`zero_forces` 同时清 velocity 和 acceleration，名字不表示只清加速度。零填充组件不是所有算法都可直接使用的有效姿态/形状，创建者仍须完成初始化。

CreatePlayer中立position/yaw/pitch重载初始化五组件，Camera重载仅保旧低层兼容；身体0.55×1.8×0.55、眼偏移0.65、速度4.4/6.2、起跳7.6、下落加速度-25不变。最小[ECS修补](ecs.md)支持检测拒绝/展开清理，不替代一般Remove、对齐、跨Registry/Clear身份等T5审计。

依据：[目标](../../game/modules/simulation/CMakeLists.txt)、[组件](../../game/modules/simulation/include/symocraft/simulation/component.h)、[玩家值与创建](../../game/modules/simulation/src/player.cpp)、[交互值](../../game/modules/simulation/include/symocraft/simulation/playercontroller.h)。

## 每帧规则与零步语义

一次Advance顺序：全量预检→一次冷却/有序指针/控制→Transform/Character各一次→独立预算Consume一次/零至八子步→安全出生恢复→核心射线/交互请求→自有结果。Benchmark edit由app在前提交，普通edit在后；不为统一换序。没有SyncCamera或图形相机步骤。

指针事件保持 right/up 正向；每项 yaw += dx × 0.05，pitch += dy × 0.05 后裁剪 -89..89，滚轮逐项更新 FOV。先裁剪后接受下一项，不能合并成总量而改变手感。app 已逐项处理后 MapInput 不再填汇总指针，避免重复。ApplyInput 写 run/sensor/gravity/方向与起跳意图，减选材冷却，0..7 环回，到期切换设置 0.2 秒；对向键优先 forward/right，同时 next/previous 时先 next 生效。

TransformSystem 从角度计算并归一化方向，再 cross/normalize 得 right/up；不验证任意非有限值或极点姿态。Character 的水平期望速度按 yaw 旋转、对角归一；有重力时保留竖向物理速度，sensor 模式可使用竖向轴。只有竖向 sensor 输入时原速度仍是 ±1，而不是额外乘基本速度；混合横向时按向量长度整体归一，属于已存在行为。

Character 在物理前即消费 `apply_jump_force`：有效 grounded/gravity/非 sensor 起跳立即写上升速度、清竖向加速度、清 on_ground、置 is_jumping，然后清 pending。这在 Physics 返回零步时也发生；位置积分仍不推进。每帧多物理步不重复输入、选材、指针或起跳。下落且竖向速度非正时设置 down_jump_force 并清 is_jumping；sensor 清 grounded/jump/竖向加速度。

依据：[玩家规则](../../game/modules/simulation/src/player.cpp)、[Transform](../../game/modules/simulation/src/transform_system.cpp)、[Character](../../game/modules/simulation/src/character_system.cpp)、[现行调用顺序](../../game/app/src/application.cpp)。

## 独立预算、预检与失败

Session Ready才可Advance；double时间必须有限非负，所有有序指针分量必须有限且可表示float，整列在任何写入前检查。非空有序列不能同时给非零汇总，避免重复应用。Rejected不改预算/冷却/组件/resume标志；有限长帧显式裁到规则0.1秒后转float。
推进开始后的组件/物理/查询/结果构建故障置Failed，再携Input/Character/Physics/Recovery/Interaction/Result阶段、完整子步数及单体提交数嵌套抛AdvanceFailed。先前规则/预算/实体提交不回滚，Failed拒绝后续Advance/Pause；不是帧事务或可恢复会话。

FixedStepBudget余数double、入口float，1/120秒最多8步，正输入贡献min(dt,Step*8)、1e-9算步后扣消耗/余数下界0，保旧float转换语义；低层非有限/非正Consume仍不改余数。每Session独占预算，旧Physics::Update必须显式传caller预算，TU全局/ResetTiming已撤除。Pause清movement/pendingjump/水平速度/预算，保竖直运动、镜头、选材/冷却；Resume要求Paused并使首次成功Advance规则dt=0；恢复出生也清力/接地/跳跃/预算。

每子步按旧速度积分位置→加acceleration→减重力(0,20,0)→逐轴限速±50→碰撞；sensor仍积分限速、清on_ground跳碰撞，不是CCD。核心参数/中间数值预检不保证任意高速/形状的连续接触。

静态体素扫描按 y 降序、x/z 升序，中心来自 position+offset，使用单位块 AABB。严格 overlap 不把仅接触面当作穿透；膨胀静态盒按象限求各轴修正，严格最小 x/y 分支之外落到 z，包括 x/y 最小值相等而 z 并非最小的旧行为。修正与 velocity 点积小于 0 表示已移出，跳过修正；否则扣重叠并清碰撞轴 velocity/acceleration，BOTTOM 接地。随后向下/静止身体以 0.002 容差查询脚下支撑，保持精确贴地，移除支撑后下一物理步失去接地。

World适配同步只读，域外/高度外/空气/nonsolid不碰撞，Found坏定义抛错不伪空气，callback不逃逸。每实体先复制Body/shape到核心，完整求解成功后才无分配提交位置/速度/加速度/接地；失败实体本次候选不写，之前实体/子步保留。结构稳定串行前提仍不可省略。

依据：[预算/DDA/纯数学](../../game/modules/simulation/include/symocraft/simulation/player_math.h)、[旧物理实现](../../game/modules/simulation/src/physics_system.cpp)。

## 唯一活动物理核心

`simulation/physics` 的 `symocraft_physics` 仅依赖 foundation/math，公开 [physics.h](E:/Codex/Worktrees/refactor-application-simulation/Symocraft/game/modules/simulation/physics/include/symocraft/physics/physics.h)。BodyState/AabbShape/PhysicsSettings 是值；StepBody 只推进调用方提供的一步，先验证参数，再以局部副本求解并返回自有结果，不写输入、不累计预算、不保存查询或身体。查询是 `const void*` 加函数指针的同步只读借用，回调必须有效，空上下文可供无状态实现；调用方保证使用期存活，串行非重入，不在查询中改变地形。

默认步长/重力/逐轴限速/接地容差与旧算法一致，合法数据保留积分、扫描、严格平局与 DDA 顺序。参数错误、求解产生的非有限/不可表示值及查询失败分别具名；查询错误保留嵌套原因，失败不返回可提交结果。扫描端点先验证整数范围、循环使用宽计数；RayCast 延续旧一百万坐标/距离保护域，但非法输入具名抛错而不是普通 miss。无命中返回中立 RayHit，无 draw/SDK 副作用。

physics_system.cpp现仅薄World/ECS适配并调用核心，原碰撞/射线算法和全局预算不再是活动生产路径。PlayerMath旧纯数学/DDA helper与Camera兼容API留低层测试，不是第二活动游戏系统。核心测试、真实World Session对照和SDL/GPU组合分别取证，不相互替代。

## 射线、编辑与相机借用

核心DDA归一化方向/先查起点，负方向整边界先入负格；平局跨轴同时推进、法线X/Y/Z优先，端点含等号、起点实心可零normal。非有限、零方向、负距离或超百万保护域按D13具名拒绝，不当普通miss；RayCastStatic薄包装同规则，draw仍无作用，命中字段仅hit=true有语义。

DoRayCast 在眼位置查 3 米，禁用输入仍可返回 selection；允许且冷却到期时 place 优先 remove。放置用 TryToBlockCoord 形成整数请求，检查库存范围、法线非零、高度/域内、目标 nonsolid 与实体 AABB 不重叠；缺定义抛错。放置正常拒绝（如重叠/非法库存）仍设置 0.2 秒，不退回 remove；查询异常发生在冷却赋值前则不保证该赋值已完成。miss 不改变冷却。app 在同帧同 World 上 TryEdit，随后才更新 mesh；不能保存请求后假定世界未变。

Camera 自有 FOV=45，但只借用 Registry 与其中 Transform 的 ID。构造创建相机实体，width/height 参数不缓存；析构不销毁实体，隐式复制/移动构造仍别名同 Registry/实体，引用成员使赋值不可用。FOV 对有限 Scroll 裁剪 1..45；姿态 setter 直接写值，不重新计算方向，也不统一防 NaN。view 返回 lookAt 值，projection 用调用者 aspect、0.1..2000 clipping；不能用 Window 的保护性 aspect 推定零像素可渲染。

上述Camera/SyncCamera只保旧低层兼容测试，不被Application/Session调用。活动视图从同一身体pose+Lens派生自有PlayerView，可省略/重复，非法aspect只拒派生不推进会话；现代app转换角度及clip，后端用真实extent，GL派生RH_NO矩阵。Renderer不读ECS/Camera，见[renderer](renderer.md)。

## 私有只读诊断

正式simulation库内SessionDiagnosticsAdapter通过单独窄私有diagnostics头交接自有状态/预算/冷却/材料/运动值，根session测试只获该子目录，不开放整个src或PUBLIC传播。采集不推进预算/改状态/保组件地址；无法安全读取的Failed运动字段为optional缺失，不补0。组合AdvanceResult/AdvanceFailed及World公开查询/提交完成D12，公开Inspect仅未来方向。
AdvanceResult返回处已写D09理由/所有权/演进触发：当前小自有结果处理简单，只有测得持续转交成本才考虑caller buffer，不为未来预先暴露缓冲协议。

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

[player_math_tests](../../test/unit/simulation/player_math_tests.cpp)、[simulation_contract_tests](../../test/unit/simulation/simulation_contract_tests.cpp)、15物理+9玩法基线保旧字面期望；夹具改各自预算并调用正式核心。Session测试另验双会话、拒绝/部分提交、每Advance/World提交固定阈值对照及自有视图/旧结果，不能从旧基线单独推定这些保证。

本轮完整授权与实际候选结果见[T4 verification](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/verification.md#2026-10-15-完整生产迁移)，不由机制文字代替人工体验/整体验收。D02正式physics搬迁在T5后/NPC前独立授权；既定Q01-Q04按[decisions](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/decisions.md)落实，无新公开Inspect或一般ECS重构。
