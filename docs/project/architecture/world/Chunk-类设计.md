---
type: 类设计
status: 自动验证完成（T1范围）
project: Symocraft
module: world
class_name: "SymoCraft::World::Detail::Chunk"
inheritance: []
created: 2026-10-06
updated: 2026-10-06
tags:
  - area/architecture
---

# Chunk 方块与已发布网格存储

关联：[World owner](World-类设计.md)、[Block](Block-数据设计.md)、[Mesher](ChunkMesher-类设计.md)、[统一验收](World-功能.md#验收案例)。源码：[私有头](../../../../game/modules/world/src/chunk.h)、[构造](../../../../game/modules/world/src/chunk.cpp)。

## 当前设计

职责：保存一个固定坐标区块的连续方块、最终CPU网格与版本。World通过私有独占指针拥有；Chunk不拥有邻块、定义、Generator、GPU或任务调度，不向外暴露地址/可写数组。

### 数据成员

| 类型 | 成员 | 初值 / 范围 | 含义与所有权 |
| --- | --- | --- | --- |
| `const ChunkCoord` | coordinate | 构造指定 | 固定x,z坐标；不可移动 |
| `const bool` | fringe | 构造指定 | 有限世界边缘，只存方块/摘要不绘制 |
| `vector<Block>` | blocks | 65,536个完整AIR_BLOCK | 本块自有AoS，索引(y*16+x)*16+z |
| `MeshData` | mesh | 空vertices | 最终已发布网格独占；未发布空与已发布空由版本区分 |
| `Revision` | content_revision | 1 | 当前方块完整内容版本 |
| `Revision` | mesh_input_revision | 1 | 自身/邻块影响的网格输入版本 |
| `optional<Revision>` | published_mesh_revision | 无值 | 最后完整发布；由World提交 |

### 不变量

原T0编号保留历史关联，含义随T1表示更新；World版本/访问约束见其权威表。

| 编号 | 条件 | 成立边界 |
| --- | --- | --- |
| I1 | blocks.size()==16*16*256，无公开Free/resize入口 | 构造成功至析构前 |
| I2 | 最终顶点数量由mesh.vertices.size()唯一决定；无GPU/count副本 | World发布前后 |
| I3 | 无长期邻居成员；Mesher借用只来自同World当前Build | World查找/同步构建 |
| I4 | Changed推进自身内容和全部受影响输入；公布版本仅成功交换后更新 | World TryEdit/RebuildDirtyMeshes |

### 公开接口预期行为

public仅指world私有实现类，不是模块消费者API。

| 签名 / 入口 | 调用方与可见范围 | 预期行为：输出及状态变化 | 前提 / 边界 | 失败反馈及失败后状态 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| `explicit Chunk(ChunkCoord, bool edge)` | World工厂 | 固定坐标/边缘，完整空气数组，无发布网格 | 工厂控制合法坐标/边缘 | 分配异常，完成成员RAII清理 | chunk.cpp / I1 |
| 隐式析构 | unique_ptr owner | 释放blocks/mesh | 结束所有借用后 | 不抛，不删除邻块 | chunk.h / I3 |
| copy/move ctor/assign = delete | 私有实现/测试 | 禁止浅复制或搬迁活动Chunk | 稳定独占节点 | 编译期拒绝 | chunk.h |
| `static size_t Index(int x,int y,int z) noexcept` | World/mesher | 返回(y*16+x)*16+z宽索引 | 内部前提x,z0..15、y0..255，函数不重复校验 | 非法参数不构成公开安全查询 | chunk.h / I1 |

### 私有函数预期行为

不适用：Chunk是受World控制的存储，无私有方法。查询、生成、编辑、邻接和发布算法已移至 [World](World-类设计.md) / [Mesher](ChunkMesher-类设计.md)。

### 生命周期与布局

World工厂以x,z次序make_unique节点 → 全地形/植被 → 受控修改/完整网格交换 → 终止借用 → World销毁。单块65,536个8字节Block，连续访问索引兼容旧摘要；不分配逐格对象。固定square索引向量只搬独占指针，不搬Chunk；World也禁止移动。当前无独立卸载或并发读写。

## 本次变更

T1删除ChunkState、长期front/back/left/right_neighbor、HashFunction与Free/生成/编辑方法；原T0仅删除GPU命令/重复计数及隐藏私有头，历史依据见 [T0报告](../../../milestones/m3-t0/README.md)。I1-I4、禁止特殊成员和按块发布的自动结果见 [T1唯一验收](World-功能.md#验收案例)。

## 后续考虑

加载/卸载与并发不由当前地址稳定推定；需另定义身份/邻接输入寿命及消费者失效。

