---
tags:
  - area/architecture
---

# Simulation

`symocraft_simulation`是CPU静态模块。[SimulationSession](../../game/modules/simulation/src/session.cpp)及内部物理核心适配已随获批准的T4实现于2026-10-19同步F主工作区，不是Git历史合并或T5实施。现役行为见[simulation spec](../openspec/specs/simulation/spec.md)/[world spec](../openspec/specs/world/spec.md)，历史设计与选择保留于[T4归档](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/design.md)。

本工作树的 NPC 实现机制见下文；现役行为由 [NPC 实体](../openspec/specs/npc-entities/spec.md)、[NPC AI](../openspec/specs/npc-ai/spec.md)及 [simulation spec](../openspec/specs/simulation/spec.md)维护。[已确认决策](../openspec/changes/archive/2026-11-19-npc-framework-harvest-mvp/decisions.md)保留选型依据，验证与批准见该 change 的记录。

2026-10-25 源码适用范围：用户已独立授权将验收后的 T5 实现同步 F，本页相对源码链接现指向采用新 ECS 的主工作区。下方 T5 适配与 [ECS 机制](ecs.md#t5-实现机制)均已落实；其余时间/物理/输入/结果语义不变，主工作区检查与 E 的历史运行成绩分别记录。

## T5 调用适配

`session.cpp` 工厂要求注册六类组件（原五类加 PhysicsStepOrder），严格 Emplace 装配玩家并显式赋顺序键 0。组件按 C++ 对象初始化，调用方在全部结构新增后才重取引用并填完整姿态/形状/控制；不依赖旧字节清零。Camera 到玩家兼容工厂先复制原 Transform 值再创建玩家，避免构造输入借用跨结构修改。低层 CreatePlayer 仍是多步装配，失败整体丢弃 caller 的专属范围，不自动 Destroy 回滚半装配。

`physics_adapter.cpp` 仍匹配 Transform/RigidBody/HitBox，EachOrdered 的只读 selector 再读取 PhysicsStepOrder，不能把缺键身体过滤掉。装配方保证本步完整集合有唯一 uint64 业务键；额外身体不能沿用玩家键 0，不按 native ID/地址/偶然顺序补序。键准备、暂存/排序、缺键或平局在任何本步物理 visitor 前拒绝；已经执行的 Character、预算或此前子步不回滚，“零本步身体提交”不等于整次 Advance 零变化。

按键逐体复制 BodyState/shape，正式核心完整求解成功后只写该体 position/velocity/acceleration/grounded，再增加提交计数；后续坏体保留先前体/子步，不提前全体预检抹去既有前序提交语义。排序不改求解算法、World 规则或 D12 固定字段/绝对阈值，真实时间不同的 GPU 轨迹不要求相等。

character/transform 使用 Each 的声明顺序引用与逐组件 const；只在当前回调借用，不保原生 view/iterator。Session 私有诊断先看 Registry::GetStatus，Registry Failed 时不扫描 native，motion 明确缺失而非补 0/缓存旧成功姿态。Session 因业务失败进入 Failed 不表示 Registry 一定损坏；两者失败域独立。原 S1-S4/C1-C3 继续，S5 对应 [T5 短借用/查询 guard](ecs.md#借用查询与同步)，不再使用 Free。

## 所有权与组合

模块仅公开CPU值/Registry/World参数，依赖foundation/scene/ecs/world，PRIVATE链接真实symocraft_physics，不依赖Window/Renderer/app/SDK。Session借稳定专属Registry及const World，拥有固定步预算/控制冷却/Lens/状态，不保存平台/观测指针或组件地址。

caller 先注册上节六类组件；Session 工厂检查注册与中立初态，在该 Registry 完整创建一个玩家后才发布，不创建 Camera 实体。失败可留下部分装配，caller 丢弃专属范围，无共享 Registry 事务；Session 析构不删除借用存储/实体。独立 Session 预算/选材/冷却隔离，不推出共享 Registry、多窗口或并发安全。
AdvanceInput/有序PointerInput均为值；AdvanceResult自有身体pose、lens、selection、整数edit请求和本轮统计。无活动 NPC 的既有 caller 决定是否派生视图并直接提交 World；有活动 NPC 时走下文的一次性帧提交入口，不借结果字段跨推进作第二运动权威。玩家请求本身不带 World 版本条件，不代表已提交或异步队列。

Transform 保存 position/scale、以度为单位的 yaw/pitch 和 front/right/up；RigidBody 保存 velocity/acceleration 与 grounded/sensor/gravity；HitBox 保存绝对 size/offset，不由 Transform.scale 缩放。组件按正常对象初始化，不再依赖旧 POD/零填充；创建者须完成合法姿态/形状。`zero_forces` 同时清 velocity 和 acceleration，名字不表示只清加速度。

CreatePlayer 的 position/yaw/pitch 重载装配原五组件并显式初始化 PhysicsStepOrder；Camera 重载仅保低层兼容。身体 0.55×1.8×0.55、眼偏移 0.65、速度 4.4/6.2、起跳 7.6、下落加速度 -25 不变。当前使用上节 T5 门面，实际结果不能从[旧 ECS 限定修补](ecs.md#f-主工作区旧实现)继承。

依据：[目标](../../game/modules/simulation/CMakeLists.txt)、[组件](../../game/modules/simulation/include/symocraft/simulation/component.h)、[玩家值与创建](../../game/modules/simulation/src/player.cpp)、[交互值](../../game/modules/simulation/include/symocraft/simulation/playercontroller.h)。

## NPC 权威与动作接口

`SimulationSession` 首次 `AddNpc` 时创建私有 `NpcRuntime` 并注册 `NpcComponent`。运行器仅借 Registry/World，持有活动实体身份表和动作编号；`Transform`、`RigidBody`、`HitBox`、`CharacterComponent` 与 `PhysicsStepOrder` 仍在 typed ECS 中。NPC 配置、动作阶段/进度、原目标标识、已接受规则、工作点、受阻计时和按资源 ID 排列的计数由该实体的 `NpcComponent` 唯一保存，不在行为树或显示列表中复制成可写权威。`LivingEntity` → `Player`/`Mob` 是借 Session 与实体身份的能力门面，玩家不接受 NPC 耗时采集。

装配先检查有限参数、保护坐标域和唯一非零顺序键，预留活动表并完成全部组件初始化后才发布身份。玩家物理键保留 0；NPC 显式配置键同时填入当前的 `PhysicsStepOrder`，提交排序与物理排序的用途仍各自独立。失败未发布的实体在 Registry 健康时销毁，既有活动实体保留；不在 ECS 查询回调中执行结构修改。`RemoveNpc` 销毁该身份及动作资格，后续显示快照不再包含它；应用须先停止并释放对应树实例，避免活树继续调用旧身份。

公开 `StartHarvest(entity, target)` 返回会话签发的 `ActionId`，`CancelAction(entity, action)` 成对校验身份。`Moving`、`Harvesting`、`Ready` 是活动阶段；达到耗时只变成 Ready，成功只能由帧提交发布。替换任务或取消清除旧进度，终态不会重复入账。`InspectNpc` 返回自有动作/资源/位姿快照；`NpcDisplays` 复制当前活动实例的身份/种类/位姿、会话与推进标识、累计实际水平路程、动作及资源值。路程在每个真实物理步前后量取，不随显示读取、现实时间或暂停增长。消费者据会话/实体/动作及样本身份隔离姿态，重复绘制同一样本不消费步态；同帧提交或取消仍能从新的动作阶段识别终态。两种读取均不推进玩法、不保存组件地址，返回值不借资源容器。新增/启动要求 Ready 且没有待提交帧；取消/移除可在暂停或待提交边界撤销动作资格，Failed 后这些动作和 NPC 读取均拒绝。

依据：[公开 NPC 值与门面](../../game/modules/simulation/include/symocraft/simulation/npc.h)、[会话接口](../../game/modules/simulation/include/symocraft/simulation/session.h)、[私有权威组件](../../game/modules/simulation/src/npc_runtime.h)、[动作实现](../../game/modules/simulation/src/npc.cpp)。

行为树的 [NpcAiSnapshot](../../game/modules/simulation/include/symocraft/simulation/npc_ai.h) 另提供只读 `idling`：当前至少一个项目 Idle 动作为 RUNNING，且没有其他 RUNNING 动作时为真。该值在读取时从树的实际节点类型和状态派生，未 tick 或已 halt 时为假；不推进树、不依赖节点显示名称，也不复制成可写玩法权威。应用用它和真实动作终态共同判断演示完成，诊断路径仍仅供显示。

## NPC 帧提交与 S1/F1

存在活动 NPC 的成功 `Advance` 在 Session 内保存本轮原始玩家编辑，并在结果中给出 `frame_owner`/`frame_id`。`CommitFrame(World&, result)` 在任何写入前核对 Ready、待提交状态、同一个 World 对象及会话/推进身份；结果副本中的玩家请求不能替换内部原请求。批次在开始提交时即消费，旧批次、重复提交和跨会话/World 提交拒绝。下一次 Advance 必须等本批次提交或 Pause 作废；无活动 NPC 的旧路径继续由 caller 直接提交玩家请求。

S1 在一个串行、非重入入口先提交原玩家 `TryEdit`，再按场景键升序处理 Ready NPC。每位候选都重新核对活身份、动作、当前工作位置和原目标 token。工作位置复核同时检查到达容差、工作距离、接地事实及当前地形的身体空间/脚下支撑，因此玩家同帧移除支撑或占用工作空间也会使后续 NPC 中断。先前候选的 World 变化会使后续争抢者失效；取消或移除的动作不能靠旧批次恢复资格。

F1 先复制资源计数形成候选，完成资源项构造、查找及 uint64 加法溢出检查，之后才调用 World 条件移除。溢出或可识别的分配准备失败只结束该动作，保留目标并继续下一候选。仅 World 返回 Changed 后，以无异常 `swap` 发布已准备计数并写成功终态；这一段没有分配、用户回调、树 tick、日志格式化或 ECS 结构修改。World 拒绝/无变化时丢弃计数候选。

提交异常使 Session Failed，保留已完成的前序提交并禁止重试，不回滚已移除方块。`HarvestCommitFailed` 用固定枚举与数值携带 WorldRemove/Credit 阶段、实体/动作身份及已知的 `world_changed`/`resource_credited` 事实；World 移除异常保留嵌套原因。移除后约束破坏的窄测试入口只注入一次固定故障，不开放回调。应用随后停止推进/提交，先释放不再访问存储的树状态，再结束 Session、Registry 与 World；网格/GPU 故障属于后续边界，不能把已入账玩法结果改回未提交。

Q01 的原目标有效性由 [World 稀疏单格版本](world.md#采集规则与单格目标标识q01)负责，Session 不以区块 mesh 版本或方块种类相等代替身份。真实 World 的争抢、原位替换、资源准备失败、移除后故障和提交时地形变化回归见 [npc_tests](../../test/unit/simulation/npc_tests.cpp)；覆盖结果与验收结论见[本变更验证](../openspec/changes/archive/2026-11-19-npc-framework-harvest-mvp/verification.md)。

## NPC 规则时钟与物理复用

Q02 直接使用本次 `AdvanceStatistics::rule_seconds` 对应的裁剪后 float 有效时间：最多 0.1 秒，恢复后的首次成功 Advance 为零。推进开始前已在合法 Harvesting 阶段的动作才消费它；本轮刚抵达只转入 Harvesting，从下次推进开始累计。目标与工作位置持续有效时，正规则时间即使零物理步也累计，进度达到 `base_seconds / efficiency` 后饱和为 Ready。Pause 保留进度、清 NPC 水平速度并作废待提交批次，不积累暂停时间。

移动从目标四个水平相邻格选出有支撑、身体空间及工作距离合法的工作点，以距离及固定候选次序稳定裁决。每个实际物理子步前仅设置 NPC 水平速度/朝向，并限制最后一步速度避免跨过工作点；Session 的同一预算和有序 `StepBodies` 通过正式 `symocraft_physics` target 的 `Physics::StepBody` 对玩家/NPC 读取真实 World 地形、求解并提交身体值。NPC 不瞬移抵达、不增加第二套碰撞或固定步预算，也不增加实体间碰撞。抵达与受阻由实际物理结果判断，受阻时限仅由没有取得进展的已完成物理步消耗；零步和暂停不消耗它。后续采集中失去有效位置即中断。

正式核心所有权的独立迁移验收只确立复用前置；实际 NPC 移动和采集仍需本变更自己的联合验证，不继承迁移的运行结论。

## 每帧规则与零步语义

一次Advance顺序：全量预检→一次冷却/有序指针/控制→NPC 规则推进（如有）→Transform/Character各一次→独立预算Consume一次/零至八子步→安全出生恢复→核心射线/交互请求→自有结果。NPC 的移动驱动/抵达检查包围每个实际物理子步。Benchmark edit由app在前提交，普通edit在后；NPC 场景的后提交由上述 CommitFrame 保持玩家优先。没有SyncCamera或图形相机步骤。

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

`game/modules/physics` 唯一拥有正式 `symocraft_physics` STATIC target、核心源码与公开 [physics.h](../../game/modules/physics/include/symocraft/physics/physics.h)，仅依赖标准库与 foundation/math。simulation 只 PRIVATE 链接该库，不取得其私有 src；固定步预算、ECS 有序遍历、World 查询适配和逐实体提交仍由 simulation 负责。BodyState/AabbShape/PhysicsSettings 是值；StepBody 只推进调用方提供的一步，先验证参数，再以局部副本求解并返回自有结果，不写输入、不累计预算、不保存查询或身体。查询是 `const void*` 加函数指针的同步只读借用，回调必须有效，空上下文可供无状态实现；调用方保证使用期存活，串行非重入，不在查询中改变地形。

默认步长/重力/逐轴限速/接地容差与旧算法一致，合法数据保留积分、扫描、严格平局与 DDA 顺序。参数错误、求解产生的非有限/不可表示值及查询失败分别具名；查询错误保留嵌套原因，失败不返回可提交结果。扫描端点先验证整数范围、循环使用宽计数；RayCast 延续旧一百万坐标/距离保护域，但非法输入具名抛错而不是普通 miss。无命中返回中立 RayHit，无 draw/SDK 副作用。

physics_system.cpp现仅薄World/ECS适配并调用核心，原碰撞/射线算法和全局预算不再是活动生产路径。PlayerMath旧纯数学/DDA helper与Camera兼容API留低层测试，不是第二活动游戏系统。核心测试、真实World Session对照和SDL/GPU组合分别取证，不相互替代。

## 射线、编辑与相机借用

核心DDA归一化方向/先查起点，负方向整边界先入负格；平局跨轴同时推进、法线X/Y/Z优先，端点含等号、起点实心可零normal。非有限、零方向、负距离或超百万保护域按D13具名拒绝，不当普通miss；RayCastStatic薄包装同规则，draw仍无作用，命中字段仅hit=true有语义。

DoRayCast 在眼位置查 3 米，禁用输入仍可返回 selection；允许且冷却到期时 place 优先 remove。放置用 TryToBlockCoord 形成整数请求，检查库存范围、法线非零、高度/域内、目标 nonsolid 与实体 AABB 不重叠；缺定义抛错。放置正常拒绝（如重叠/非法库存）仍设置 0.2 秒，不退回 remove；查询异常发生在冷却赋值前则不保证该赋值已完成。miss 不改变冷却。app 在同帧同 World 上 TryEdit，随后才更新 mesh；不能保存请求后假定世界未变。

Camera 自有 FOV=45，但只借用 Registry 与其中 Transform 的 ID。构造创建相机实体，width/height 参数不缓存；析构不销毁实体，隐式复制/移动构造仍别名同 Registry/实体，引用成员使赋值不可用。FOV 对有限 Scroll 裁剪 1..45；姿态 setter 直接写值，不重新计算方向，也不统一防 NaN。view 返回 lookAt 值，projection 用调用者 aspect、0.1..2000 clipping；不能用 Window 的保护性 aspect 推定零像素可渲染。

上述Camera/SyncCamera只保旧低层兼容测试，不被Application/Session调用。活动视图从同一身体pose+Lens派生自有PlayerView，可省略/重复，非法aspect只拒派生不推进会话；现代app转换角度及clip，后端用真实extent，GL派生RH_NO矩阵。Renderer不读ECS/Camera，见[renderer](renderer.md)。

## 私有只读诊断

正式simulation库内SessionDiagnosticsAdapter通过单独窄私有diagnostics头交接自有状态/预算/冷却/材料/运动值，根session测试只获该子目录，不开放整个src或PUBLIC传播。采集不推进预算/改状态/保组件地址；无法安全读取的Failed运动字段为optional缺失，不补0。组合AdvanceResult/AdvanceFailed及World公开查询/提交完成历史 D12。T4 当时没有公开通用 Inspect；本次仅新增上述有明确 NPC 值边界的 InspectNpc/NpcDisplays，不把私有 Session 诊断整体公开。
AdvanceResult返回处已写D09理由/所有权/演进触发：当前小自有结果处理简单，只有测得持续转交成本才考虑caller buffer，不为未来预先暴露缓冲协议。

依据：[交互](../../game/modules/simulation/src/playercontroller.cpp)、[Camera](../../game/modules/simulation/src/Camera.cpp)、[现代值转换](../../game/app/src/render_conversion.cpp)。

## 保留的不变量与验证边界

| 原编号 | 当前成立前提 |
| --- | --- |
| S1 | PlayerIntent/InteractionResult 是值，不持平台、世界或 GPU 指针。 |
| S2 | 有效玩家、正常初始状态和有限输入下，选材 0..7、pitch -89..89。 |
| S3 | DoRayCast 只形成 selection/edit 值，app 负责触发 World 提交；有活动 NPC 时经 CommitFrame 保持同帧次序。 |
| S4 | 一次预算 Consume 返回 0..8、步长 1/120；非有限/非正输入不改余数。 |
| S5 | 组件引用只在当前同步查询、结构稳定时使用；禁止并发、遍历中结构修改/Free。 |
| C1 | Camera 使用期 Registry 存活，所属相机 Transform 未移除。 |
| C2 | Camera FOV 在构造及有限滚轮输入后保持 1..45 度。 |
| C3 | 每次 projection 的当前有效 aspect 由调用者提供，不缓存窗口尺寸。 |

[player_math_tests](../../test/unit/simulation/player_math_tests.cpp)、[simulation_contract_tests](../../test/unit/simulation/simulation_contract_tests.cpp)、15物理+9玩法基线保旧字面期望；夹具改各自预算并调用正式核心。Session测试另验双会话、拒绝/部分提交、每Advance/World提交固定阈值对照及自有视图/旧结果，不能从旧基线单独推定这些保证。

T4 完整授权与当时候选结果见 [T4 verification](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/verification.md#2026-10-15-完整生产迁移)，不由机制文字代替人工体验/整体验收。D02 正式 physics 所有权已按独立授权在本工作树迁移，A23 方案 A 的结果与未覆盖项见 [归档记录](../openspec/changes/archive/2026-11-16-promote-physics-module/verification.md)；2026-11-16 用户已独立验收，该交付的 NPC 物理复用前置成立，主工作区传播单独记录；不将结果回写为 T4 当时 A23 已通过。T4 的 Q01–Q04 按其 [decisions](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/decisions.md)落实，当时没有新增公开 Inspect 或一般 ECS 重构；与本次 NPC 变更的同名决策编号及专用快照接口分开解读。
