---
tags:
  - area/architecture
---

# World

## 边界与权威

`symocraft_world` 拥有固定有限世界、定义、确定性生成、受控编辑及 CPU 网格；不持有 Registry、Window、GPU、telemetry 会话或 app 配置。公开值/坐标/编辑/版本/发布及 D01–D09 已选理由以 [world spec](../openspec/specs/world/spec.md) 为唯一权威，本文只解释实现正确性所依赖的机制。
公开 foundation/scene，YAML/noise 私有；[CMake](../../game/modules/world/CMakeLists.txt)、[world.h](../../game/modules/world/include/symocraft/world/world.h)、[私有 owner](../../game/modules/world/src/world_internal.h) 是核对入口。现代缓存/上传属于 app/[renderer](renderer.md)，独立 Session/physics 的 [T4 目标](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/proposal.md) 尚未实现，不改变本模块当前寿命。

## 实例、定义与存储

工厂按值接管已校验 BlockDefinition，创建局部 Generator 和完整 Impl/Chunk，完成全部地形及植被后才返回 unique_ptr。World、Chunk、Mesher、Generator 均不可复制/移动；指针 owner 可以转移但对象/身份不迁移。失败局部 RAII 不发布半世界、不影响另一实例；可选 Timings 只借到返回，失败可能已写部分阶段耗时，不承诺输出参数事务。

Impl 持自有定义、const 实际 Settings/WorldId、按 x 外层 z 内层排列的 `vector<unique_ptr<Chunk>>`、一个串行 Mesher 和实际派生 noise seeds；不常驻报告树。radius 2..10，默认 441 存储块/361 drawable；fringe 保存方块并参与摘要，但不发布网格。固定二维槽按坐标查找，既不借另一 World，也不保存长期邻接指针。无独立卸载/流式区块机制。

每 Chunk 独占 16×16×256 个完整初始化 Block 的连续 AoS 与最终 MeshData；索引 `(y*16+x)*16+z` 仅用于已检查内部坐标，无逐格堆对象或可公开 resize/free 入口。当前 Block 四个 16 位字段为 8 字节：ID、光强、压缩色和 flags。ID 0 是域外内部哨兵，1 是合法空气；材料三个位与其余保留位分开，普通 Block 相等**只比 ID**，不能用于完整内容变化判断。光色当前先转整数再乘 7 的量化风险保留，不借重构宣称修复光照。

定义私有存储为按 ID 排序规则 vector、按名称排序的自有字符串 vector，lower_bound 查询返回 optional **副本**。解析完成候选后才发布，拒绝非法/重复 ID/名称并检查必需映射/uint16 纹理层；运行中无公开热替换。拷贝深复制，copy assignment 先复制再 swap，失败保原目标；移动源可空，缺失查询为空，但纹理校验拒绝空源。规则解析/名称分配是冷路径；实际 atlas 层数校验不由配置解析替代。

## 编辑与版本提交

浮点入口统一 floor 后 finite/i32 检查；核心 Query/Edit 使用整数，负 x/z 的 floor-div16 映射与正本地索引分开。查询返回自有 Block/规则值；修改副本不能回写存储。域外与未知定义不等于空气，非法 edit operation 是异常而非普通 Rejected。

TryEdit 先检查 visit/build guard，再形成完整候选：Set 改 ID/三个材料位，Remove 恢复空气材料位，均保不受影响的光及高位。显式比较四字段而非 padding 或 Block 相等；只 Changed 准备版本推进。固定 array 最多包含目标加两个直接 x/z 邻块，角点不含对角块，不分配动态编辑队列。
提交之前检查目标 content 和全部受影响 mesh-input 是否能加一；任何准备/耗尽失败不写内容或任何版本。真实提交只赋 Block、递增版本；Unchanged 保持已有 pending。content/input 初始 1，published revision 初始无值，内容变化与网格输入变化分别计量，不能把 published 假装成最新 input。

## Mesher 与按块发布

World 私有持有一个具体有状态 Mesher（D08 实际选择，不是原草稿的局部无状态函数）。Build 同步借当前 Chunk、按 +x/-x/+z/-z 取得的四邻块及只读定义；缺邻/高度域外返回 NULL，六邻格非 NULL 且透明才出面。保留 y,x,z 扫描、+x/+z/-x/-z/+y/-y 面序、每面六顶点、绕序/UV/层及 normal=0，不引入 greedy meshing、相机 culling 或新透明/树叶规则。

Mesher 的独占 Face 列只记世界位置/层/方向，不留 Chunk/Block 指针；退出正常/异常均 clear size、恢复 building guard、保留 capacity。宽类型数量检查后候选一次 reserve 并展开，自有候选不别名 scratch。复用增加 Face 写读和内存、仍分配候选；不保证零分配、并行或变快。
World 按固定 x,z 处理 pending drawable，每块 Build 完整后再检查顶点数量为六的倍数/字节数可表示，最终无异常 swap 后才发布当前 input revision。旧网格归候选局部析构。首错停止，前序成功已发布不回滚；当前/后续保旧结果和 pending，可后续重试，不是整批或整世界事务。bad_alloc 原类型传播，其余构建异常补坐标/阶段；候选非法尺寸也不能发布。

内部统计是有边界的诊断：builds 计 Mesher 完整返回，不等于 World 发布；scratch_growths 计一次调用退出 capacity 变化（含异常），不是分配总数；candidate_bytes 是最近 reserve 容量；peak_mesh_bytes 只覆盖单次 scratch+候选+该块旧 mesh，不含其他块/OS/驱动。测试注入 probe 默认空且只有窄白盒入口，不是 app 公共服务。

## 身份、借用与线程

WorldId 由 relaxed atomic CAS 进程序列发放，不复用、不回绕；失败工厂可消耗身份。它加 ChunkCoord 构成稳定 mesh 身份，不是地址、跨进程存档 ID 或线程安全证明。
VisitMeshes 不隐式重建，按顺序给全部已发布 drawable，包括空发布和仍 pending 的旧完整网格；首次无 published 不输出。record 的发布 revision 与当前 input 并列，不伪装 fresh。身份/版本可复制，但 MeshView 只当前 callback 有效，消费端需在 callback 内取得自有顶点，禁止在借用中终结 World。

visiting/rebuilding/building 是同步重入 guard，作用域析构正常/异常均复位，不是锁。callback 内同实例编辑/重建/重入访问在变化前拒绝，读取和另一 World 操作不受该 guard 阻止；callback 异常原样传播，不回滚以前外部行为。当前仅串行使用，不承诺 const 方法与写入并行安全。后台生成/meshing 须另定每任务独占工作区、输入版本/寿命、取消/过期结果与退出等待，不能沿用短借用。

## 生成、摘要与固定场景

Generator 只在构建期拥有三路 FastNoiseLite/派生 seeds/设置，不拥有 World；Config/NoiseSeeds 引用只到 Generator 销毁，World 在生成末尾复制事实。噪声库类型只在 cpp；const 采样不证明库线程安全，植被每块从 seed+坐标派生自有 mt19937，无全局随机游标。
派生采用 [generation.cpp](../../game/modules/world/src/generation.cpp) 固定 uint32 Mix 的定义回绕、负坐标到 unsigned 的转换及噪声 seed 的非负掩码，不是 std::hash 或外部随机源。每块独立游标不保证 seed 无碰撞或统计独立；RandomSeed 只取 random_device 输出，不保证唯一，也不提供密码学身份。
版本 1 的噪声配置、浮点算法、候选/随机消耗及写入顺序维持旧基线：全部地形先完成，再按 chunk x,z 植被跨块 last-write-wins。terrain 的 fringe 先置空气，**后续跨块植被可写 fringe**，不能宣称最终边缘始终空气。工厂期直接写不推进初始版本，首网格必须另外显式构建。

Digest 是固定小端 FNV1a64 schema，含全部块坐标与四个 Block 字段，按 chunk x,z / block y,x,z，不遍历网格，也不把配置/版本混入 hash；比较须同时保留 Describe 的配置与版本事实。它是可碰撞的内容摘要，不是密码学身份、存档格式或源码版本。相同有效定义/配置/seed/版本及数值构建环境才有确定性承诺，非跨编译器/平台逐位一致。Describe 按实例实际 facts 冷路径构建自有 Value，不重造 Generator/噪声或把报告分配并入生成计时；Timings 的 allocation/terrain/vegetation 毫秒不含首网格。FindSpawn 保原近中心安全出生规则，找不到抛异常，不修改世界。

TestScene 的 Pose/Edit 表及名称是静态只读借用；姿态数值/编辑记录可复制，span 不变成任务 owner。Install 先核对 radius≥3 覆盖，再顺序 TryEdit；ApplyEdits 检查每步 before，首错停止保已完成写入，两者不承诺整场景事务。Changed/Unchanged 都算 Accepted，但不代表新网格已发布。固定基准 DueEdits 使用合法实际秒数/0.25 的 floor 总数，CycleEdit 为正向16步再逆向16步，不保存当前 World 或自动应用编辑；场景/基准元数据和输入回放不是一回事。

改变噪声/seed 派生、树形、随机消耗、写入顺序或边缘语义须审查 [Generation::Version](../../game/modules/world/include/symocraft/world/generation.h)；改变固定几何/姿态/路线/编辑序列须审查 [TestScene::Version](../../game/modules/world/include/symocraft/world/test_scene.h)；摘要序列化变化须更新 metadata 的 digest_schema 并重建适用基线。当前版本均为 1，这些标识靠显式维护，不会由实现自动检测变化。

## 稳定不变量与验证边界

编号按原对象/任务限定，避免旧各篇 I1/D1 重名被错误合并；现役五个任务不变量身份保持不变。

| 原身份 | 继续成立的边界 |
| --- | --- |
| M3-T1-BC-I1；BC-I1–BC-I3 | 定义规则/名称对应且唯一、完整校验、只读自有；查询副本独立，拷贝赋值失败保目标/移动源状态明确。 |
| M3-T1-W-I1；W-I1/W-I3；Chunk I1/I3 | 坐标/索引/ID 合法、两实例隔离、固定块顺序/AoS 容量，无长期邻接或可公开存储修改。 |
| M3-T1-W-I2；W-I2；Chunk I2/I4；World 数据 D2/D3 | 内容/输入/发布分离、受影响推进不回绕，最终顶点唯一计数、完整按块发布及失败 pending/空结果保留。 |
| W-I4/沿用 I5；World 数据 D1 | guard 拒绝并恢复，身份不用地址；callback 借用不延寿。 |
| M3-T1-ME-I1；ME-I1–ME-I3；Scene I4 | 工作区独占/退出清状态，输入只本次借用，候选自有并保固定几何顺序。 |
| M3-T1-GE-I1；Generator G1–G3 | 半径/版本和有效环境固定，独立随机流/确定顺序，噪声只本 owner 拥有；引用受其寿命限制。 |
| Block 数据 D1–D3 | ID 相等不代替完整变化判断；保未受影响光/flags，规则返回自有副本。 |
| API-I1–API-I3 | 场景只改显式 World，经 TryEdit/Accepted；表及名称仅静态借用。 |

[world_contract_tests](../../test/unit/world/world_contract_tests.cpp)、[mesh_safety_tests](../../test/unit/world/mesh_safety_tests.cpp)、[block_config_tests](../../test/unit/world/block_config_tests.cpp)、[world_generation_tests](../../test/unit/world/world_generation_tests.cpp) 使用正式库，覆盖隔离/版本/故障/回调/定义与旧摘要对照，不是影子实现。
真实 C++ 分配扫描历史只在 Release 与全库 `_ITERATOR_DEBUG_LEVEL=0` 专用 Debug；[测试 CMake](../../test/unit/world/CMakeLists.txt) 禁用普通 MSVC Debug 扫描，不能据此称全部 Debug OOM 安全。cost probe 只计 C++ new 请求，非 malloc/OS/全部内存；历史测量不是性能预算通过。
T1 人工 GL 玩法、矩阵/成本及延期树叶剔除/视角问题按 [world spec 的证明边界](../openspec/specs/world/spec.md#rationale-and-proof-boundary) 与 [VERIFICATION](../verification/VERIFICATION.md) 追溯，技术结果不补造节点批准。本轮未重新运行 C++、分配故障、游戏/GPU 或性能测试。
