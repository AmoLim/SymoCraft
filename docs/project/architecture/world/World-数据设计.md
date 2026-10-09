---
type: 数据设计
status: 自动验证完成（T1范围）
project: Symocraft
module: world
created: 2026-10-06
updated: 2026-10-06
tags:
  - area/architecture
---

# World 坐标、编辑与发布值

所有者与操作：[VoxelWorld](World-类设计.md)。CPU 顶点/自有网格：[scene 数据](../scene/Scene-共享场景数据-数据设计.md)。规则：[Block/BlockDefinitionEntry](Block-数据设计.md)、[BlockDefinition](BlockDefinition-类设计.md)。

## 当前设计

职责：完整登记 world 公共轻量交换值；不创建新存储 owner、逐格对象或通用 Result。实际头：[world.h](../../../../game/modules/world/include/symocraft/world/world.h)、[block_definition.h](../../../../game/modules/world/include/symocraft/world/block_definition.h)、[test_scene.h](../../../../game/modules/world/include/symocraft/world/test_scene.h)。

<a id="blockcoord"></a>
<a id="chunkcoord"></a>
<a id="blockqueryresult"></a>
<a id="editrequest"></a>
<a id="editresult"></a>
<a id="meshidentity"></a>
<a id="worldmeshrecord"></a>
<a id="settings"></a>
<a id="timings"></a>
<a id="testscene-pose"></a>
<a id="testscene-edit"></a>
### 数据语义

| 记录 / 类型 | 字段 | 初值 / 范围 / 单位 | 含义与来源 |
| --- | --- | --- | --- |
| `BlockId = uint16` | 值 | 合法存储1..65535，1为空气；0仅域外内部哨兵 | 定义集合校验；不把未知ID当空气 |
| `BlockDefinitionEntry` | [完整字段表](Block-数据设计.md#工作负载与数据语义) | 自有规则struct | BlockDefinition集合的单条记录 |
| `BlockCoord = glm::ivec3` | x/y/z | i32 格坐标；y合法0..255 | Query/Edit核心输入，负x,z用floorDiv16 |
| `ChunkCoord = glm::ivec2` | x/y | i32；y字段表示区块z | 本World范围[-radius,radius]² |
| `WorldId / Revision = uint64_t` | 值 | 身份1起，版本1起，不回绕 | 进程身份非存档ID；版本所有者为私有Chunk |
| `BlockQueryStatus` | Found / OutsideWorld | 枚举 | Found含合法空气；域外不是Found的ID0 |
| `BlockQueryResult` | status、block | 显式填充，无默认成员初值 | 自有Block副本；OutsideWorld时不读取为合法方块 |
| `EditOperation` | Set / Remove | 枚举 | Remove忽略请求id，以既有空气状态提交 |
| `EditRequest` | operation、position、id | operation/position显式填；id默认0 | 自有输入；非法枚举抛异常，不作为普通拒绝 |
| `EditStatus` | Rejected / Unchanged / Changed | 枚举 | 接受不等于内容变化 |
| `EditRejection` | None / OutsideWorld / UnknownBlockId | 枚举 | Rejected原因；资源/版本异常不伪装拒绝 |
| `EditResult` | status、rejection | status显式填；rejection默认None | Accepted()仅判断status!=Rejected |
| `MeshIdentity` | WorldId world、ChunkCoord chunk | 显式填充 | 同实例同区块稳定；重建不换身份 |
| `WorldMeshRecord` | identity、revision、mesh_input_revision、vertices | 显式填充；vertices为MeshView | revision是已发布版本，input可能更新；记录仅callback内可用 |
| `MeshData / MeshView` | [完整CPU字段/视图契约](../scene/Scene-共享场景数据-数据设计.md) | scene类型 | 自有网格 / 限时借用，不向renderer泄露world私有类型 |
| `Generation::Settings` | seed/radius/vegetation | uint32=0/int=10/bool=true | 自有配置；实际校验/所有权见Generator/World |
| `Generation::Timings` | allocation_ms/terrain_ms/vegetation_ms | double=0，毫秒 | 同步阶段输出，不保存指针 |
| `TestScene::Pose` | string_view name、vec3 position、float yaw/pitch | 显式填充；角度为度 | 姿态数值自有；name只借静态文本，checkpoint表另有静态寿命 |
| `TestScene::Edit` | ivec3 position、unsigned short before/after | 显式填充；ID | 静态有序计划值，不是异步事务 |

### 工作负载、表示与所有权

Query/DescribeBlock 每次复制小型值；TryEdit 原地写一个 Block，固定数组最多准备自身与两水平邻块，不创建动态编辑队列。VisitMeshes 按块同步交接元数据/span，不复制所有世界顶点；消费者需长期保留时在callback内取得自有顶点。Settings/Timings/Describe 属冷路径，热网格循环不使用 Data::Value。

| 数据 | 拥有者 / 表示 | 有效期 / 结构变化 |
| --- | --- | --- |
| 坐标、规则、编辑、身份/版本值 | 自有普通值 | 原owner销毁不影响副本；MeshIdentity不是跨进程永久身份 |
| MeshData::vertices | Chunk最终网格或Mesher候选vector | 扩容/释放可使借用悬空；移动/交换改变owner，不保证原契约继续有效，vector交换本身不使元素引用悬空 |
| WorldMeshRecord::vertices | 借用对应Chunk已发布网格 | 最长当前callback；span复制不延寿 |
| Pose/Edit span、Checkpoint引用 | 静态场景表 | 当前内置静态表寿命；不推广至任意动态场景 |
| Describe返回值 | 调用者自有Data::Value，按World事实冷路径构造 | World不常驻报告树，无共享可写状态 |

### 公开接口预期行为

| 签名 / 入口 | 调用方与可见范围 | 预期行为：输出及状态变化 | 前提 / 边界 | 失败反馈及失败后状态 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| 值聚合构造/复制/移动、字段访问 | world消费者 | 复制自有字段；仅vertices/name保持借用 | 显式填无默认字段；不虚构数据校验器 | 不能以失效span测试安全 | world.h/test_scene.h / D1-D3 |
| `EditResult::Accepted() const noexcept` | benchmark/app | Changed/Unchanged均true，Rejected为false | 不吞异常，不代表重新mesh | 不修改状态 | world.h / D2 |
| `MeshIdentity::operator==(const MeshIdentity&) const = default` | CPU消费者 | world+chunk共同相等 | 坐标相同但world不同不等 | 不修改状态 | world.h / D1 |

### 私有函数预期行为

不适用：这些是值/视图，无私有owner函数。创建/索引/编辑/版本提交的唯一行为表位于 [VoxelWorld](World-类设计.md)，固定场景函数位于 [namespace API](World-namespace-API.md)。

### 不变量

| 编号 | 可检查条件 | 成立边界 |
| --- | --- | --- |
| D1 | MeshIdentity使用WorldId+ChunkCoord，不取地址；身份不随重建变化 | World产生的记录 |
| D2 | Unchanged不推进/清除版本；Accepted同时包含Changed/Unchanged | World返回；调用方统计合法应用数 |
| D3 | 发布记录revision不冒用更新的input_revision；首次未发布无记录，空发布有记录 | VisitMeshes；寿命见World I5 |

目标Windows x64；这里的值交接无独立吞吐目标。缺失统计不补0，候选/工作区峰值与重建成本由统一T1测量留证。

## 本次变更

T1新增显式坐标、三态编辑、身份/版本和发布记录；MeshData归scene。S1字段清单即上表，自动契约已验证；唯一运行验收见 [world功能](World-功能.md#验收案例)。

## 后续考虑

新增流式身份、异步快照或存档必须另定义格式/复用/失效；不延长当前span契约。
