---
type: 类设计
status: 自动验证完成（T1范围）
project: Symocraft
module: world
class_name: "SymoCraft::World::VoxelWorld"
inheritance: []
created: 2026-10-05
updated: 2026-10-06
tags:
  - area/architecture
---

<a id="world-存储门面与协作"></a>
# VoxelWorld 世界实例与发布

关联：[功能/统一验收](World-功能.md)、[值与版本](World-数据设计.md)、[定义](BlockDefinition-类设计.md)、[Chunk](Chunk-类设计.md)、[Mesher](ChunkMesher-类设计.md)。历史文件名与旧锚点保留，不再表示 ChunkManager namespace。

## 当前设计

职责：独占有限世界，校验/查询/提交编辑，调度同步 Mesher 与按块发布。生成算法见 [Generator](Generator-类设计.md)，GPU/输入/玩家归相邻模块。

### 数据成员

| 类型 | 成员 | 初值 / 范围 | 含义与所有权 |
| --- | --- | --- | --- |
| `unique_ptr<Impl>` | `impl_` | 工厂成功非空 | World 独占完整私有状态 |
| `BlockDefinition` | `Impl::definition` | 已校验完整集合 | 按值自有，运行中不替换 |
| `const Settings / const WorldId` | `settings / identity` | radius 2..10；进程身份 | 实际生成配置/稳定身份 |
| `vector<unique_ptr<Detail::Chunk>>` | `chunks` | (2r+1)² 个 | x 外层、z 内层；独占不可移动 Chunk |
| `Detail::ChunkMesher` | `mesher` | 一个 | World 独占串行工作区 |
| `std::array<int,3>` | `noise_seeds` | 初始0，Populate末复制实际Generator值 | 自有派生seed；Describe按settings/seeds/definition构造报告，不常驻树 |
| `mutable bool / bool` | `visiting / rebuilding` | false | 运行时同步重入 guard；不是锁 |

### 不变量

| 编号 | 条件 | 成立边界 |
| --- | --- | --- |
| W-I1 | 私有 Chunk/定义只归本实例；坐标索引/存储 ID 有效 | Create 成功、公开操作结束 |
| W-I2 | 发布版本只对应完整网格；输入/内容版本无回绕，失败不伪发布 | TryEdit/重建返回或异常 |
| W-I3 | chunks 固定 x,z 顺序，fringe 留在存储但不发布网格 | Create 成功以后 |
| W-I4 | 重入修改/访问在状态变化前拒绝，guard 在异常后恢复 | VisitMeshes/RebuildDirtyMeshes 作用域 |
| I5（沿用） | MeshView 仅本回调有效；不得销毁正在借用的 World | 调用方寿命前提，运行时不追踪悬空 span |

### 公开接口预期行为

| 签名 / 入口 | 调用方与可见范围 | 预期行为：输出及状态变化 | 前提 / 边界 | 失败反馈及失败后状态 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| `static std::unique_ptr<VoxelWorld> Create(BlockDefinition, Generation::Settings, Generation::Timings* = nullptr)` | app/CPU；工厂唯一入口 | 按值接管定义，完成方块/settings/seeds，网格未发布 | 定义1..11；半径2..10 | 参数/资源异常；局部 RAII，无返回半对象，不改另一世界 | world.cpp / W-I1 |
| `~VoxelWorld()` | owner | 释放 Impl/Chunk/mesher | 所有 callback 已结束 | 不抛，不生成/导出；借用失效 | world.cpp / I5 |
| copy ctor/assign、move ctor/assign = delete | 全部消费者 | 对象与身份不迁移 | 独占指针可移动，不是 World 可移动 | 编译期拒绝 | world.h / W-I1 |
| `QueryBlock(BlockCoord) const noexcept` | simulation/工具 | Found 自有 Block 或 OutsideWorld | 整数格，负坐标 floorDiv16 | 域外为正常结果；不改世界 | world.cpp / W-I1 |
| `std::optional<BlockDefinitionEntry> DescribeBlock(BlockId) const` | 规则消费者 | optional 规则副本 | 不借用定义存储 | 缺失为空 optional | BlockDefinition::Find |
| `std::optional<BlockId> FindBlockId(std::string_view) const` | app/工具 | optional ID | 名称只在调用内借用 | 未知为空 optional | BlockDefinition::FindId |
| `EditResult TryEdit(EditRequest)` | app/固定场景 | Rejected/Unchanged/Changed；完整准备后原地写回 | 非 visit/build；Set 同步 ID/三材料位，保留光字段；Remove 空气标记 | 域外/未知ID拒绝；非法operation抛；版本耗尽/准备失败无任何提交 | world.cpp / W-I1/W-I2/W-I4 |
| `std::size_t RebuildDirtyMeshes()` | app 同步阶段 | x,z 顺序发布脏且非边缘块，空发布计数 | 非 visit/build；已有存储 | bad_alloc保留类型，其余构建异常补充阶段/Chunk坐标；已成功块保留，失败/后续待更新 | world.cpp / W-I2/W-I3 |
| `void VisitMeshes(const std::function<void(const WorldMeshRecord&)>&) const` | app/CPU | 已发布非边缘块，包含空/旧发布结果；不重建 | callback 限时借用 | callback 异常传播，guard 恢复；重入/修改 logic_error | world.cpp / W-I4/I5 |
| `std::string Digest() const` | 工具/报告 | 当前全部块的旧 schema FNV1a64 | 不遍历网格 | 输出分配异常，不改实例 | world.cpp |
| `glm::vec3 FindSpawn() const` | app | 保留近中心安全出生规则 | 本实例规则/方块 | 找不到抛 runtime_error | world.cpp |
| `Data::Value Describe() const` | app/telemetry | 按实际settings/seeds/definition生成自有元数据 | 冷路径；不重造Generator，不并入terrain/vegetation计时 | 分配异常；不暴露YAML，不修改World | generation.cpp |
| `void ValidateTextureLayers(std::size_t) const` | app 首次绘制前 | 对照实际层数校验定义 | count>0 | 异常不改世界 | BlockDefinition 笔记 |
| `std::size_t ChunkCount() const noexcept / WorldId Identity() const noexcept` | app/CPU | 总块数/WorldId 值 | 有效实例 | 不分配、不修改 | world.cpp |

### 私有函数预期行为

| 签名 / 入口 | 内部调用方 | 预期行为：处理规则及副作用 | 前提 / 边界 | 失败传播及清理责任 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| `VoxelWorld(unique_ptr<Impl>)` | Create | 接管完成 Impl | 非空候选 | unique_ptr 清理 | world.cpp |
| `Impl(BlockDefinition, Settings, WorldId)` | Create | 接管定义/配置/身份 | 工厂已校验 | 成员构造 RAII | world.cpp |
| `Impl::Lookup(ChunkCoord) const noexcept` | 查询/生成/mesh | 范围检查后固定二维槽查找 | 完成 chunks 范围 | 域外 nullptr，不借另一世界 | world.cpp / W-I1 |
| `Impl::Read(BlockCoord) const noexcept` | Query/Digest/Spawn | floorDiv16 + 本地索引读 Block 副本 | y0..255 | 域外 NULL_BLOCK 仅内部适配 | world.cpp |
| `Impl::EnsureMutable() const` | 编辑/重建 | visiting/rebuilding 时先拒绝 | 修改前调用 | logic_error，无状态变化 | world.cpp / W-I4 |
| `Impl::Populate(const Generator&, Timings*)` | Create | 全地形后按x,z植被，记录阶段耗时，末尾复制实际NoiseSeeds | 仅工厂未发布世界；显式目标 | 异常由Create清理全部候选 | generation.cpp / Generator笔记 |
| `Impl::WriteGenerated(BlockCoord, BlockId)` | Populate 植被 | 本实例坐标查找后写 ID/标记 | 仅构建期间，不推进初始版本 | 域外忽略；未知ID logic_error | generation.cpp |
| `FloorDiv16(int) noexcept`（TU） | Read/TryEdit | 负数余数修正，不使用浮点 | 任意 i32 | 不抛 | world.cpp |
| `NextIdentity()`（TU） | Create | relaxed atomic CAS 发放1起不复用身份 | 进程唯一序列；不证明世界线程安全 | uint64 用尽 overflow_error，不回绕；失败世界可消耗身份 | world.cpp |
| `Digest::append(uint32_t,unsigned)`（局部lambda） | Digest | 逐字节little-endian FNV1a64累加 | 固定坐标4字节、Block字段2字节；x,z块/y,x,z格顺序 | 不分配/不改世界 | world.cpp |
| `Restore`（局部 guard 析构） | visit/build | 异常/返回均恢复对应 bool | World 活着 | 不抛，不回滚已发布块 | world.cpp / W-I4 |

### 生命周期

app/CPU 取得定义 → Create 局部 Generator/完整 Impl → 场景编辑 → 首网格 → 查询/编辑/重建/访问 → 结束借用 → unique_ptr 销毁。Chunk 地址稳定来自独占节点，公开身份不取地址。当前只承诺串行调用；被借用 World 的销毁属于调用方违规，不通过运行时访问悬空指针检测。

## 本次变更

T1 从隐式 ChunkManager/全局定义转为显式独占实例，新增三态编辑、三版本及限时发布记录。九项已选决定及唯一验收/证据入口见 [功能](World-功能.md#本次变更)；自动契约已验证，人工玩法和节点批准仍以阶段报告为准。

## 后续考虑

流式/并发/存档需要新寿命与版本契约；T3 GPU 映射属于 app/renderer，不加入 World。
