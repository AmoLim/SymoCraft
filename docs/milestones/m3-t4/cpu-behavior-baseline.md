---
type: 行为基线
status: CPU现有行为基线与夹具双配置验证完成，未人工批准
project: Symocraft
module: M3-T4-App-Platform-Simulation
created: 2026-10-09
updated: 2026-10-09
tags:
  - area/milestones
  - topic/simulation
---

> 2026-10-11 文档浓缩后已退役复核源：现行入口见 [权威位置](../../verification/VERIFICATION.md#原位置保留的历史材料)；原文保留点时反馈、历史身份和复核依据，不再维护为当前机制或活动计划，旧勾选不产生新批准。

# T4 现有 CPU 行为基线

本页记录本批现有生产函数的行为及 CPU 夹具实测，不是新接口契约或完整 T4 验收。2026-10-09 双配置验证完成，权威汇总见 [summary.json](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/summary.json)；来源以实际 dirty 工作区 SHA-256 为准，未运行条目不记通过。[阶段入口](README.md) 和 [唯一人工核查入口](manual-verification.md) 分别管理阶段状态与用户结论。

## 要保持的行为

| 领域 | 现有生产行为与对照重点 | 来源 |
| --- | --- | --- |
| 固定步预算 | 120 Hz、每次最多 8 步；预算用 double 余数，接受 float 帧时间，现有 `1e-9` 取步容差及 Reset 保持。非有限或非正时间现有预算返回零步 | [player_math.h](../../../game/modules/simulation/include/symocraft/simulation/player_math.h) |
| 时间与规则顺序 | 普通 app 先将实际非负时间裁剪至 0.1 s 并转 float；角色规则每帧一次，物理零至多步。规则/冷却的 0.1 s 上限与预算的 8/120 s 上限分开，不把角色规则放入每个物理子步 | [application.cpp](../../../game/app/src/application.cpp) |
| 单步积分 | 位置先加旧速度乘固定步，再加加速度、减重力，最后各轴限速至 ±50，再碰撞/接地；重力为 `(0,20,0)`。不改积分器或增加 CCD | [physics_system.cpp](../../../game/modules/simulation/src/physics_system.cpp) |
| 碰撞扫描及平局 | 扫描 y 递减、x 递增、z 递增；严格重叠才解算，正在向外运动的 `dot < 0` 接触跳过。仅 x/y 是严格最小穿透时选对应轴，其余落 z 分支，包括平局；不顺便改 tie-z 或扫描范围 | [physics_system.cpp](../../../game/modules/simulation/src/physics_system.cpp) |
| 接地与 sensor | 执行子步时 sensor 跳过碰撞并清接地；普通体每次碰撞先清接地，非正垂直速度且脚下 `0.002` 容差内存在支撑时接地并清垂直速度/加速度。支撑移除后下一次实际子步重新查询；零子步不会单由 Physics::Update 刷新支撑 | [物理](../../../game/modules/simulation/src/physics_system.cpp)、[支撑数学](../../../game/modules/simulation/include/symocraft/simulation/player_math.h) |
| 世界查询 | 当前 World 定义解释可碰撞性；负坐标与区块边界按真实 World 查询，y 域外、OutsideWorld 及 AIR 为非碰撞，已存未定义方块抛错而非当空气。无需重建渲染网格才看到编辑后支撑变化 | [physics_system.cpp](../../../game/modules/simulation/src/physics_system.cpp) |
| 旧零步跳跃 | 已接受跳跃由角色规则立即改垂直速度、清接地、设跳跃并消费请求，即使本帧零物理步；位置等待后续正常子步。未满足接地等条件的跳跃不缓冲，不增加落地前预输入或离地宽限 | [character_system.cpp](../../../game/modules/simulation/src/character_system.cpp)、[player.cpp](../../../game/modules/simulation/src/player.cpp) |
| 有序指针 | app 逐事件调用现有 ApplyPointerInput，再应用不含重复指针汇总的键盘意图；每个事件独立裁剪 pitch 与 FOV。不能先合并事件，否则饱和后反向事件的结果可能改变 | [application.cpp](../../../game/app/src/application.cpp)、[player.cpp](../../../game/modules/simulation/src/player.cpp) |
| 冷却 | 选材和放置各按本帧有效规则时间减一次；选材 0.2 s、防抖期间不重复选材。命中且进入放置/移除分支后置 0.2 s，即使放置被保护规则拒绝；无命中或不允许输入不消费新交互冷却。place 与 remove 同时满足时 place 优先 | [player.cpp](../../../game/modules/simulation/src/player.cpp)、[playercontroller.cpp](../../../game/modules/simulation/src/playercontroller.cpp) |
| 延迟编辑 | DoRayCast 返回自有请求而不直接修改 World；普通 app 在模拟后提交，benchmark edit 在模拟前。World 拒绝不回滚已消费放置冷却；测试分开记录请求产生、提交与当前方块结果 | [playercontroller.cpp](../../../game/modules/simulation/src/playercontroller.cpp)、[application.cpp](../../../game/app/src/application.cpp) |

## 现状限制，不冻结为目标

旧 Physics::Update 的 `step_budget` 是进程静态共享状态，ResetTiming 影响全局预算；夹具须每个独立用例显式 Reset 并串行调用，不能把它包装后声称双 SimulationSession 隔离。T4 目标仍是会话唯一预算及独立核心一次明确子步，不二次 Consume。

旧入口在消耗预算后原地修改组件，World 查询异常可能发生在已推进位置/速度之后，且不保证整步或本实体回滚。旧预算对非法时间的静默零步、旧查询/错误路径只作为现状记录，不能视为 D04/D11/D13 已成立；新设计的参数拒绝不改状态、单实体成功后提交及 Failed 状态须另实施留证。

无窗口夹具调用的是现有生产函数。它不是新 SimulationSession，不证明可选视图、旧结果寿命、暂停恢复的完整平台端到端行为，也不替代普通 Application 组合的 D12 逐次对照或 A24-A27。

## 夹具及证据

两项测试共 **24 组案例**，不是断言数量；使用真实生产 `symocraft_simulation`、World/ECS 及方块定义，不复制物理实现。每个场景串行 Reset 旧全局预算。源码快照为 [共享夹具](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/sources/test/unit/simulation/cpu_baseline_fixture.h)、[物理测试](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/sources/test/unit/simulation/legacy_physics_baseline_tests.cpp)、[玩法测试](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/sources/test/unit/simulation/legacy_gameplay_baseline_tests.cpp)。夹具不创建窗口、不加载 renderer，也不生成 GPU 或性能成绩。

| 验证 | 最终真实结果 | 查验材料 |
| --- | --- | --- |
| `simulation.legacy_physics_baseline` | 15 组；Debug / Release 各连续重复 3 次通过。覆盖显式 dt=0、固定步/重置、积分/限速、地面/低顶/墙角、非最小平局与多块扫描、offset/负坐标/区块边界、支撑容差/移除、只读查询/域外、射线和 Registry 遍历 | 案例输出：[Debug](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/raw/debug-final/legacy_physics_baseline_tests-output.log)、[Release](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/raw/release-final/legacy_physics_baseline_tests-output.log) |
| `simulation.legacy_gameplay_baseline` | 9 组；Debug / Release 各连续重复 3 次通过。覆盖零步跳跃、多步一次规则、有序指针逐事件裁剪、按住/选材冷却、sensor、延迟编辑、拒绝放置耗冷却、禁用输入/射线未命中及模拟后移除支撑 | 案例输出：[Debug](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/raw/debug-final/legacy_gameplay_baseline_tests-output.log)、[Release](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/raw/release-final/legacy_gameplay_baseline_tests-output.log)；重复记录：[Debug](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/raw/debug-final/baseline-repeat.log)、[Release](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/raw/release-final/baseline-repeat.log) |
| 同配置全部 CPU CTest | Debug **58/58 启用项通过**，`world.allocations` 1 项禁用；Release **59/59 通过**。禁用项不计通过，不借用 T3 旧成绩 | [Debug 日志](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/raw/debug-final/ctest.log) / [JUnit](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/raw/debug-final/ctest.xml)；[Release 日志](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/raw/release-final/ctest.log) / [JUnit](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/raw/release-final/ctest.xml) |
| 真实 CPU 构建闭包 | 两配置实际 Ninja 命令不依赖 renderer、app、window 或图形 SDK；`physics_system.cpp` 仅由生产 target 编译一次，不向夹具重复编译；CPU 配置无 native targets | 物理命令：[Debug](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/raw/debug-final/legacy_physics_baseline_tests-commands.log)、[Release](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/raw/release-final/legacy_physics_baseline_tests-commands.log)；玩法命令：[Debug](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/raw/debug-final/legacy_gameplay_baseline_tests-commands.log)、[Release](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/raw/release-final/legacy_gameplay_baseline_tests-commands.log) |
| `BUILD_TESTING=OFF` | Debug / Release 均独立构建生产 `symocraft_simulation` 通过；无测试 target 或测试编译，不靠 `BUILD_TESTING` 才生成生产模块 | [Debug 构建](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/raw/debug-final/no-tests-build.log) / [缓存](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/raw/debug-final/no-tests/CMakeCache.txt) / [命令描述](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/raw/debug-final/no-tests/build.ninja)；[Release 构建](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/raw/release-final/no-tests-build.log) / [缓存](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/raw/release-final/no-tests/CMakeCache.txt) / [命令描述](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/raw/release-final/no-tests/build.ninja) |

最终构建/运行记录：[Debug result.json](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/raw/debug-final/result.json)、[Release result.json](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/raw/release-final/result.json)。实际使用 MSVC `14.38.33145.0` 与 Windows SDK `10.0.22621.0`；上述图形依赖检查不表示不使用编译工具链的 Windows SDK。最终原始构建位于 `out/m3-t4/cpu-b02-debug`、`out/m3-t4/cpu-b02-release`，初轮/身份位于 `out/m3-t4/cpu-baseline-20261009-001`；查验归档为本阶段 `evidence/cpu-baseline-20261009-001`，不是可运行包或新运行。

## 来源、原失败与缺口

来源链为 [before](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/identity/before.json) → [tested](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/identity/tested.json) → [final](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/identity/final.json) → [after](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/identity/after.json)。选定 `production/dependencies/assets` 相对 before 未变；最终运行后复核 `production/dependencies/assets/build/fixtures` 与 final 的 SHA-256 一致。该身份固定的是选定 CPU 闭包及测试/构建来源，不是完整仓库或全部 CPU 回归的可重现封存；第三方依赖原位哈希。`application.cpp` 仅属 `reference_only` 调度背景，不链接、不冻结，也不把 fixture 顺序等同真实 Application 装配对照。

首次 Debug 沙箱配置出现 MSVC C1902，原始 [configure.log](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/raw/debug-sandbox-failure/configure.log) 保留；获准在正常开发环境重试后通过，未改 PDB 选项规避。初轮成功的 [Debug](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/raw/debug-first/result.json)、[Release](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/raw/release-first/result.json) 原样保留。最终加强显式 dt=0、射线法线/格坐标精确比较及 app 禁止依赖门禁，改用新的短路径目录复跑；浮点阈值未因结果扩大。

本批没有独立新物理核心、SimulationSession、全局预算迁移或 D12 对照，不记 T4-S04、D04/D11/D13 新保证及 A24-A27 通过。`world.allocations` 普通 Debug 禁用状态保持；CPU 本轮通过不关闭 Win32 1175 / Fix1 遗留风险，也不补造窗口、GPU、完整玩法、性能或用户人工批准。
