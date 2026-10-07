---
type: 类设计
status: 自动验证完成（T1范围）
project: Symocraft
module: world
class_name: "SymoCraft::World::BlockDefinition"
inheritance: []
created: 2026-10-06
updated: 2026-10-06
tags:
  - area/architecture
---

# BlockDefinition 自有只读规则集合

关联：[World功能/统一验收](World-功能.md)、[规则值](Block-数据设计.md)、[VoxelWorld](World-类设计.md)。源码：[公开头](../../../../game/modules/world/include/symocraft/world/block_definition.h)、[实现](../../../../game/modules/world/src/block.cpp)。

## 当前设计

职责：从配置文本构建完整ID/名称集合，查询返回副本，校验纹理层。app/assets读取文件；renderer建立纹理数组；本类不拥有路径、纹理、World或GPU。

### 数据成员

| 类型 | 成员 | 初值 / 范围 | 含义与所有权 |
| --- | --- | --- | --- |
| `unique_ptr<Storage>` | storage_ | FromConfig成功非空；移动源可空 | 独占私有集合 |
| `vector<pair<BlockId,BlockDefinitionEntry>>` | Storage::rules | 按ID排序，ID非0且唯一 | 自有小规则值，binary search |
| `vector<pair<string,BlockId>>` | Storage::names | 名称非空唯一，字典序 | 自有名称，不借配置文本 |

### 不变量

| 编号 | 条件 | 成立边界 |
| --- | --- | --- |
| BC-I1 | rules/names对应且唯一；必需11名称与ID1..11兼容；纹理层能表示为uint16 | FromConfig成功、所有只读操作 |
| BC-I2 | 返回规则/ID值不依赖storage_寿命；集合运行中无公开修改入口 | 查询返回 |
| BC-I3 | copy独立存储；赋值失败不改目标；移动源空状态可销毁/赋值/缺失查询 | 特殊成员结束 |

### 公开接口预期行为

| 签名 / 入口 | 调用方与可见范围 | 预期行为：输出及状态变化 | 前提 / 边界 | 失败反馈及失败后状态 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| `static BlockDefinition FromConfig(std::string_view)` | app/CPU | 本地解析、校验、排序后返回自有集合 | 文本借用仅调用内；必需字段严格转换，optional材料/光字段沿用缺省 | YAML诊断转runtime_error；非法/重复明确报错；bad_alloc传播，候选RAII | block.cpp / BC-I1 |
| `~BlockDefinition()` | owner | 释放Storage/字符串/规则 | 可为空移动源 | 不抛；不改World | block.cpp |
| `BlockDefinition(const BlockDefinition&)` | 值消费者 | 深复制Storage，空源仍为空 | 不借源容器 | 分配失败清理新候选 | block.cpp / BC-I3 |
| `operator=(const BlockDefinition&)` | 值消费者 | copy后swap，self-assignment无变化 | 独立值 | 复制失败目标保留原集合 | block.cpp / BC-I3 |
| move ctor/assign noexcept | 工厂/值消费者 | 转移unique_ptr，源变空 | 不移动World内实际定义 | 不抛；旧目标内容销毁 | block.cpp / BC-I3 |
| `std::optional<BlockDefinitionEntry> Find(BlockId) const` | World/mesher | lower_bound后规则副本 | 不泄露容器/名称借用 | 缺失/移动源为空optional | block.cpp / BC-I2 |
| `std::optional<BlockId> FindId(std::string_view) const` | app/World | lower_bound后ID副本 | 参数借用仅调用内 | 未知/移动源为空optional | block.cpp / BC-I2 |
| `void ValidateTextureLayers(std::size_t) const` | app首次绘制前/CPU | 遍历三纹理索引验证<count | count>0，与实际资产层数对照 | 移动源logic_error；零/越界runtime_error；集合不变 | block.cpp / BC-I1 |

### 私有函数预期行为

| 签名 / 入口 | 内部调用方 | 预期行为：处理规则及副作用 | 前提 / 边界 | 失败传播及清理责任 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| `BlockDefinition(unique_ptr<Storage>)` | FromConfig | 接管候选存储 | 只内部发布 | unique_ptr清理 | block.cpp |
| texture(field) lambda | FromConfig每条规则 | 读取top/side/bottom，检查0..uint16max | 实际必需字段 | 类型/越界异常，不发布候选 | block.cpp / BC-I1 |
| lower_bound/sort比较lambda | 查询/构建 | ID或字典序比较 | 已构建候选/排序集合 | 不修改查询集合 | block.cpp |

### 生命周期与成本

FromConfig局部候选 → 成功自有集合 → copy或move入World → 只读查询 → 销毁。解析/名称分配属于冷路径；查ID为连续规则表binary search，未从表示推定热路径收益。YAML类型只在cpp，公开无重载全局表。

## 本次变更

替代两份全局map及路径读取；未知规则/名称从退回空值改明确optional，失败不替换另一World定义。BC-I1-I3对应T1-A01/A02/A09/A15，自动结果及故障扫描配置见 [唯一验收](World-功能.md#验收案例)。

## 后续考虑

规则规模/查找实测成瓶颈再比较索引结构；热重载需另定义版本，不能修改活动World规则。
