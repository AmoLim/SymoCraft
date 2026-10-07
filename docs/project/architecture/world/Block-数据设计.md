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

# Block 方块值与规则条目

关联：[Chunk存储](Chunk-类设计.md)、[BlockDefinition集合](BlockDefinition-类设计.md)、[World编辑](World-类设计.md)、[统一验收](World-功能.md#验收案例)。源码：[block.h](../../../../game/modules/world/include/symocraft/world/block.h)、[规则头](../../../../game/modules/world/include/symocraft/world/block_definition.h)、[常量](../../../../game/modules/world/include/symocraft/world/constants.h)。

## 当前设计

<a id="block"></a>
<a id="blockformat"></a>
### 工作负载与数据语义

Block是每格AoS普通值，不是堆对象/材质/虚函数体系；生成/编辑原地写ID和三材料位，mesher/碰撞读取。每块65,536个8字节记录；光照算法/字段热度和缓存瓶颈未测。

| 记录 / 类型 | 字段 | 初值 / 范围 / 单位 | 含义与来源 |
| --- | --- | --- | --- |
| Block | uint16 block_id | 无统一默认初值；合法定义ID | 每格权威ID；AIR_BLOCK等完整常量构造 |
| Block | uint16 lightLevel、int16 lightColor | 完整常量或已有内容 | 光强/三个3-bit色段，T1不修光照算法 |
| Block | uint16 bitwise_compressed_data | 完整常量或已有内容 | 位0透明、1混合、2光源；其余保留 |
| World::BlockDefinitionEntry | uint16 m_top_texture/m_side_texture/m_bottom_texture | 配置解析后可表示层索引 | 自有单条规则；顶/侧/底纹理层，不是GPU句柄 |
| World::BlockDefinitionEntry | bool m_is_transparent/m_is_solid/m_is_blendable/m_is_lightSource | 配置/既有缺省 | 表面/碰撞规则 |
| World::BlockDefinitionEntry | int16 m_light_level | 配置或0 | 保留既有规则字段 |

### 公开接口预期行为

| 签名 / 入口 | 调用方与可见范围 | 预期行为：输出及状态变化 | 前提 / 边界 | 失败反馈及失败后状态 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| 聚合字段构造/普通复制/移动 | World/值消费者 | 自有数值，无外部资源 | 必须完整初始化；查询副本不能回写World | 无自有清理/异常恢复 | block.h |
| operator== / != (const Block&) const | 既有值消费者 | **仅比较block_id** | 不是完整属性等价判据 | 不改状态；TryEdit不使用它判Unchanged | block.h / D1 |
| IsTransparent / IsBlendable / IsLightSource const | mesh/玩法 | 读取位0/1/2为bool | 已初始化记录 | 不改状态 | block.h |
| IsLightPassable const | 旧光照消费者 | source或transparent | 不推定新的传播算法 | 不改状态 | block.h |
| SetTransparency / SetBlendability / SetLightSource(bool) | World私有写入/值消费者 | 只清/写对应一位 | World存储写仅受控阶段 | 不重置其余位/光字段 | block.h / D2 |
| SetLightColor(const ivec3&) | 既有生成 | 依当前表达式写3段压缩色 | 原0..255注释不构成校验 | 无范围诊断；原先转int再乘7精度问题仍存在 | block.h |
| GetLightColor() const | 值消费者 | 色段按7→255恢复整数RGB | 旧压缩表示 | 不改状态，非高精度可逆保证 | block.h |
| GetCompressedLightColor() const | 值消费者 | 返回三个0..7值 | 已初始化记录 | 不改状态 | block.h |

### 私有函数预期行为

不适用：这些是值记录，无私有函数/容器。FromConfig/Find/FindId/纹理层校验由 [BlockDefinition](BlockDefinition-类设计.md) 唯一维护，不再存在get_block借用全局表。

### 所有权与不变量

| 编号 | 可检查条件 | 成立边界 |
| --- | --- | --- |
| D1 | Block::operator==不用于完整Unchanged判断；World显式比较四字段 | TryEdit完整候选比较 |
| D2 | Set同步ID与三材料位，保留lightLevel/lightColor及未受操作影响位 | TryEdit/WriteGenerated；Remove按既有空气标记 |
| D3 | Find/DescribeBlock返回自有规则副本，定义销毁/移动不使副本悬空 | BlockDefinition/World查询返回 |

Block/规则自有副本可复制移动，无独立身份/存档版本；完整规则集合按值归World。公开Block字段只允许改副本，最终存储不可从公开API借为可写引用。主线程串行，不推定并发/异步光照能力。

## 本次变更

T1在block_definition.h定义实际World::BlockDefinitionEntry，删除旧全局BlockFormat，不保留兼容alias；集合所有权/缺失语义归BlockDefinition，完整编辑比较取代ID相等。D1-D3对应T1-A02/A04/A15，自动结果见 [统一验收](World-功能.md#验收案例)。光照量化风险不在T1修复或验收。

## 后续考虑

光照/块格式改变需明确数值测试及版本；不随实例重构修改旧生成摘要。

