---
type: 类设计
status: 已验证（T0范围）
project: Symocraft
module: world
class_name: Chunk / ChunkManager
inheritance: []
created: 2026-10-05
tags:
  - area/architecture
---

# Chunk 与世界存储类设计

关联功能：[世界功能](World-功能.md)。`ChunkManager` 当前仍是命名空间门面，不伪称为已实例化的世界类。

最终验证：[M3-T0 交付与验收报告](../../../milestones/m3-t0/README.md)。

## 当前设计

职责：`Chunk` 保存一个 16×16×256 方块区域、四向邻接和该区块 CPU 网格；`ChunkManager` 拥有全部区块并协调查询、更新及受限网格访问。图形分配和玩家生命周期分别归 renderer、simulation/app。

### 数据成员

| 类型 | 成员 | 初值 / 范围 | 含义与所有权 |
| --- | --- | --- | --- |
| `vector<Block>` | `m_local_blocks` | 构造为 65536 个 AIR_BLOCK | 区块独占方块，索引顺序 y,x,z |
| `vector<BlockVertex3D>` | `m_vertex_data` | 空 | 区块独占 CPU 三角形顶点 |
| `glm::ivec2` | `m_chunk_coord` | 创建时指定 | 世界区块坐标 |
| `ChunkState` | `state` | `ToBeUpdated` | 网格失效/完成状态 |
| `Chunk*` ×4 | `front/back/left/right_neighbor` | null 或同一存储内邻居 | 借用，不负责释放 |
| `bool` | `m_is_fringe_chunk` | false，邻接重排时刷新 | 有缺失邻接的有限世界边缘，不生成显示网格 |
| 私有 `unordered_node_map` | `chunks` | 空 | ChunkManager 的唯一所有者，不向模块外暴露 |

### 不变量

| 编号 | 条件 | 成立边界 |
| --- | --- | --- |
| I1 | 有效存储的 `m_local_blocks.size()==65536`；`Free` 后允许为空 | 构造、生成完成、显式释放 |
| I2 | `VertexCount()==m_vertex_data.size()`，没有第二份 GPU/count 状态 | 所有方法返回后 |
| I3 | 邻居指针只指向当前 `chunks` 中节点，缺失为 null | 创建/邻接重排后；清空后全部引用失效 |
| I4 | 编辑改变当前区块和必要邻居的 `state` 为 `ToBeUpdated` | 成功写入返回后 |
| I5 | `VisitMeshes` 的只读 span 不越过回调生命周期 | 调用者必须遵守；期间禁止编辑、生成、清空及重入修改 |

### 接口与生命周期

| 接口 | 行为 | 前提 / 边界 |
| --- | --- | --- |
| `GetBlock(position)` | 返回 Block 值，缺失返回 NULL_BLOCK | 不暴露内部地址 |
| `TrySetBlock(position,id)` | 返回底层区块写入成功与否 | 沿用原坐标/高度边界；调用方先应用玩法约束 |
| `UpdateAllChunks()` | 重建需要更新且非边缘区块，返回重建数量 | 单线程；保持 I2/I4 |
| `VisitMeshes(visitor)` | 按原 map 顺序提供非空、非边缘网格 | 保持 I5；回调不拥有数据 |
| `ChunkCount()` | 返回当前区块总数 | 不需要公开容器类型 |
| `FreeAllChunks()` | 销毁集合及方块/网格 | 所有借用必须已经结束 |
| 私有 `GetChunk/GetAllChunks` | 模块内生成、摘要和白盒测试访问 | 不属于 app/simulation 的接口 |

持有与销毁：应用启动生成世界，主循环使用，退出调用 `FreeAllChunks`。Chunk 删除拷贝、保留 noexcept 移动；map 采用原节点型容器，不能把已有邻接关系搬到另一个世界再沿用。`Free` 释放 vector 并清邻接，重复调用安全。内存分配异常向上抛出，当前生成/编辑不是事务式恢复接口。

实现位置：[私有 Chunk](../../../../game/modules/world/src/chunk.h)、[ChunkManager](../../../../game/modules/world/src/chunk_manager.cpp)。没有新异步任务、互斥量或并发保证。

## 本次变更

删除区块中的 `DrawArraysIndirectCommand` 与索引，隐藏具体 Chunk/map，新增 I5 同步借用规则。测试通过正式 `symocraft_world` 库运行；仅网格/生成白盒测试获得 `world/src` 的窄白名单。

验收位置：[世界功能验收](World-功能.md#验收案例)，重点对应 I1-I5 的已列同步路径。网格/配置/生成测试、CPU-only、公开头与实际 static/edit 对照已通过；旧新摘要和 static 截图字节一致。仍不将本次验证视为 T1 实例化世界、卸载或并发失效规则已经完成。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| T1 | 限制 Chunk 字段直接写入、引入显式世界实例与网格版本 |
| 区块卸载 | 在拥有者中先断邻接，再销毁节点；不让旧回调视图跨帧存活 |
