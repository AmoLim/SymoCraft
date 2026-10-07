---
type: 类设计
status: 草稿
project: Symocraft
module: ecs
class_name: "SymoCraft::ECS::RegistryViewer<Components...>"
inheritance: []
created: 2026-10-06
updated: 2026-10-06
tags:
  - area/architecture
---

<a id="registryviewer-组件查询视图"></a>

# RegistryViewer 组件查询 view

## 当前设计

### 成员与职责

Registry& registry、bitset<MaxComponents=256> components_need、bool _is_searching_all（空类型包为 true）。构造以进程 ComponentType<T> 构建过滤位；不拥有组件、不物化结果、不形成 archetype 批块。

### 接口与工作负载

begin 扫实体槽，HasRequiredComponents 按需要位调用 HasComponentByType，同时检查实体槽有效；end 为 EntitySlots。空集合 begin==end。类型号超 bitset 范围可抛 out_of_range，类型未按共同顺序注册没有自动注册/重排保证。

[Iterator](Iterator-类设计.md)递增筛选；逐槽位/逐类型查询成本随槽数与匹配类型变化，规模/瓶颈未测量，不宣称 dense 交集优化。
复制/移动构造保留 Registry 引用别名，不转移所有权；引用成员使赋值不可用。无自有资源释放。使用期 Registry 必须存活且相关实体/组件结构稳定，禁止遍历中修改集合、并发或重入 Free。

### 契约与风险

沿用 [I2/I3](ECS-组件存储-数据设计.md#不变量)注册/POD/借用前提；Iterator 结果版本截断、Registry 校验版本不足都未由此 view 解决，不能标为可靠多代实体查询。

[源码](../../../../game/modules/ecs/include/symocraft/ecs/registry.h)、[历史功能验收](ECS-存储边界.md#验收案例)、[覆盖清单](../对象笔记覆盖清单.md)。2026-10-06 仅源码核对，未新增查询正确性/性能验收。

## 本次变更

无。

## 后续考虑

结构变化身份、版本及查询优化先补正确性验证，再立性能实验。

