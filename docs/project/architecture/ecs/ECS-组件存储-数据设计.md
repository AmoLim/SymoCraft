---
type: 数据设计
status: 草稿
project: Symocraft
module: ecs
created: 2026-10-06
updated: 2026-10-06
tags:
  - area/architecture
---

# ECS 组件存储数据

从原 ECS 类设计拆出，2026-10-06 核对当前源码，未独立运行测试。
关联：[Registry](ECS-存储边界-类设计.md)、[ComponentContainer](ComponentContainer-类设计.md)、[功能验收](ECS-存储边界.md#验收案例)、[覆盖清单](../对象笔记覆盖清单.md)。

## 当前设计

### 工作负载与布局

Registry 按类型存储 POD 组件，查询逐实体扫描并按类型取指针。每类型使用稀疏索引池、连续实体索引与连续字节 payload；不是按组件组合存储的 archetype。Add/扩容/删除/Free 属结构变化；系统同步原地写字段。类型上限 256，稀疏池分段大小 8，dense 初容量 8。缓存命中、负载分布、优化收益未测量。

### Storage

Registry::Storage 私有：vector<EntityId> entities、vector<ComponentContainer> component_set、vector<EntityId> free_entities（实际保存回收索引）、vector<string> debug_component_names。Registry 独占 unique_ptr；没有跨 Registry 共享组件池。

EntityId 为索引+版本编码；实体表保存当前编码，销毁写入 UINT32_MAX 空索引和递增版本，创建优先复用索引。当前 IsEntityValid **只检查传入 ID 的低位索引小于表长度且非 UINT32_MAX，不核对槽内存活 ID 或版本**；编码不等于已实现 stale-ID 防护。

### 类型注册与查询

ComponentType<T> 使用进程内静态类型号，所有 Registry 须相同顺序注册。RegisterType 通过诊断断言比对槽位与 256 上限，不能宣称所有构建/日志配置提供异常恢复保护。Clear 不重置进程静态类型号，不能当作任意重注册方案。

[RegistryViewer](RegistryViewer-类设计.md)构造 256 位过滤条件，[Iterator](Iterator-类设计.md)逐槽位匹配；不缓存查询结果。GetComponent 的断言不是安全的可恢复错误返回。

### SparseSetPool

Internal::SparseSetPool：start_entity_index 与 ComponentIndex component_index_in_data[8]；索引段起点按 8 对齐，Init 将映射设为 UINT_MAX。ComponentContainer 拥有池数组，查找会遍历池。非资源主对象，池生命周期随容器，结构变化不保留外部指针。

### RawMemory

全局旧 RawMemory（ecs/src/legacy）：data/size/offset 裸缓冲、显式 Init/Free/Cursor/Read/Write/Shrink。只服务私有保留的 Serialize/Deserialize，未启用为产品存档。没有 RAII 或可靠边界/写入实现保证，WriteDangerous 的扩容分支未完成，不新增用户调用入口。

### SizedMemory

旧 SizedMemory 数据/大小辅助结构，位于 MemoryHelper.h；仅旧内存转换/序列化辅助，未发现活动产品调用。不能将声明/编译保留算已支持的保存读取功能。

### 不变量

| 原编号 | 条件与成立边界 |
| --- | --- |
| I2 | 类型号与注册顺序一致，组件满足原 POD 要求；调用者前提，注册诊断不提供通用恢复保证 |
| I3 | Get/View 的组件引用仅在池未扩容、删除、释放期间有效；结构变化可使地址/语义失效 |

[I1/I4](ECS-存储边界-类设计.md#不变量)仍归 Registry，分别约束唯一所有者和清理，不复制完整条件。

### 已知风险

保留 T5：版本校验缺失；Iterator 解引用返回 EntityIndex，可能截断 EntityId 版本；ComponentContainer 删除以实体索引与 dense 数量比较，非末尾删除搬运只复制单字节而非完整 payload；分配失败可能留下部分更新/旧指针问题；私有序列化未完成。详见独立对象笔记，不把正常玩家路径通过写成可靠 ECS 证明。

没有并发、callback 重入或分配失败事务保证；私有 Amo 分配器锁不保护 Registry 操作。Free 后不能继续添加；Clear 重注册限制如上。

源码：[registry.cpp](../../../../game/modules/ecs/src/registry.cpp)、[component_container.h](../../../../game/modules/ecs/src/component_container.h)、[internal.cpp](../../../../game/modules/ecs/src/internal.cpp)、[legacy](../../../../game/modules/ecs/src/legacy)。
原 T0 公开头/玩家路径结论见 [报告](../../../milestones/m3-t0/README.md)，不改变 T5 未完成状态。

## 本次变更

无。

## 后续考虑

先修复并测试 ID、删除、失效与失败恢复，再讨论布局/查询性能优化；本轮不修改实现。

