---
type: 类设计
status: 草稿
project: Symocraft
module: ecs
class_name: "SymoCraft::ECS::Iterator"
inheritance: []
created: 2026-10-06
updated: 2026-10-06
tags:
  - area/architecture
---

# Iterator 实体过滤迭代

## 当前设计

### 成员与借用

Registry& registry；EntityIndex entity_index；bitset<256> components_need；bool _is_searching_all。构造借用 Registry 和复制过滤状态，不拥有组件/实体/Registry，也不延长其寿命。

### 接口、状态与失效

operator++ 递增槽位，跳过无效实体/不匹配类型直至 end；IsIndexValid 私有检查当前槽和过滤条件。operator==/!= **仅比较 entity_index**，忽略 Registry/过滤器，参数是非 const 引用，不支持通用跨查询比较语义。

operator* 声明返回 **EntityIndex(uint32)**，实现取 EntityAt（EntityId(uint64)）再转换，可能丢失版本；不能写成安全完整 EntityId 返回。不得解引用 end，不能在遍历期间插入/删除/Free 或并发结构修改。

隐式拷贝复制借用；含引用导致赋值不可用，移动构造也不转移资源所有权，只产生别名。不需释放，但 Registry 必须覆盖迭代器使用期。[ECS I3](ECS-组件存储-数据设计.md#不变量)说明组件指针失效，end 也不是可跨结构变化持有的稳定身份。

### 依据与审核缺口

[头](../../../../game/modules/ecs/include/symocraft/ecs/registry.h)、[实现](../../../../game/modules/ecs/src/registry.cpp)、[RegistryViewer](RegistryViewer-类设计.md)、[Registry](ECS-存储边界-类设计.md)、[功能验收](ECS-存储边界.md#验收案例)、[覆盖清单](../对象笔记覆盖清单.md)。
2026-10-06 源码核对，未独立运行版本/跨查询比较/边界测试。T0 正常玩家遍历不证明删除重用或 stale-ID 正确性。

## 本次变更

无。

## 后续考虑

完整 EntityId/迭代范围身份及结构变化规则属 T5，另立修复与测试。

