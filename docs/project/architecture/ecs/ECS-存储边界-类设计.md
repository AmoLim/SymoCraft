---
type: 类设计
status: 已验证（T0范围）
project: Symocraft
module: ecs
class_name: "Registry / Storage / ComponentContainer"
inheritance: []
created: 2026-10-05
tags:
  - area/architecture
---

# ECS-存储边界-类设计

关联功能：[ECS-存储边界](ECS-存储边界.md)。不存在的管理类不为模板而增造；自由函数与数据结构按实际实现记录。

最终验证：[M3-T0 交付与验收报告](../../../milestones/m3-t0/README.md)。

## 当前设计

职责：ECS 仅承担实体与组件存储。游戏组件与系统迁到 simulation；公开 Registry 模板不再包含 ComponentContainer、Amo 内存模板或世界 / 图形头。模板将类型与大小交给私有字节操作，底层保持原稀疏池算法。

### 数据成员

| 类型 | 成员 | 初值 / 范围 | 含义与所有权 |
| --- | --- | --- | --- |
| unique_ptr<Storage> | Registry::storage_ | 构造创建 | 独占存储，消费者不能访问布局 |
| vector<EntityId> | Storage::entities / free_entities | 空 | 私有实体表与回收索引 |
| vector<ComponentContainer> | Storage::component_set | 空 | 旧组件池，实现头私有 |
| 私有裸内存 | ComponentContainer::pools / entities / data | InitRaw 分配 | 由 Free 释放，非外部所有权 |

### 不变量

| 编号 | 条件 | 成立边界 |
| --- | --- | --- |
| I1 | Registry 的存储所有者唯一 | 构造后至析构 |
| I2 | 类型 ID 与注册顺序一致，组件为 POD | 使用者前提；注册入口检查 |
| I3 | Get / View 引用只在相应组件池未扩容、删除或释放时有效 | 借用期间 |
| I4 | Free 可重复执行并使旧池指针为空 | 清理完成后；Free 后不继续添加组件 |

### 接口与生命周期

| 接口 | 行为 | 前提 / 边界 |
| --- | --- | --- |
| RegisterComponent<T> | 类型检查后调用私有 RegisterType | 不暴露模板容器布局 |
| AddComponent / GetComponent | 返回存储内引用 | 类型已注册、实体有效；不承诺线程安全 |
| View<...> | 借用 Registry 的过滤迭代器 | 遍历中不能改变影响结构的实体 / 组件集合 |
| Free / 析构 | 释放私有池 | Free 后只允许继续清理 / 销毁，不把它当作重置重开 |
| Serialize / Deserialize（私有） | 保存迁移前未启用实现 | 不允许作为产品存档能力调用 |

- Registry 独占 Storage，禁止复制。公开接口不提供 entities 向量或稀疏池布局。
- 按现有约定，各 Registry 必须以同样顺序注册组件；组件只支持 POD。
- Storage 的一次分配发生在 Registry 构造，转发调用不引入逐帧分配；原池扩容仍会使组件引用失效。
- 本轮隐藏实现并维持正常玩家路径，不把旧容器升级为完整可靠 ECS。版本校验、删除压缩、分配失败原子性和未使用的序列化缺陷明确留到 T4。
- RawMemory 的旧序列化仅留在私有实现，未作为可用存档接口暴露。

公开头：[include](../../../../game/modules/ecs/include/symocraft/ecs)；实现：[src](../../../../game/modules/ecs/src)。不由私有分配器锁或本次迁移推定 Registry 支持多线程或回调重入。

## 本次变更

本次将真实实现归入 ecs，用公开契约替代旧聚合头依赖。成员、所有权与失效约束以上表为准；尚未完成验证的风险不以“拆库完成”代替。

### 验收案例

验收位置：[关联功能的验收案例](ECS-存储边界.md#验收案例)。正式 Registry 基本契约、公开头边界和真实玩家路径已通过本轮验证；I2/I3 仍是调用者必须遵守的前提，不因迁移完成就消除旧实体版本、删除压缩及失效引用风险。T4 深度审计保持未完成。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| 新调用者需要改变生命周期 | 先修改契约与测试，再修改接口，不暴露存储布局解决临时需求 |

