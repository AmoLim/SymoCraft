---
type: 类设计
status: 草稿
project: Symocraft
module: ecs
class_name: "SymoCraft::ECS::Internal::ComponentContainer"
inheritance: []
created: 2026-10-06
updated: 2026-10-06
tags:
  - area/architecture
---

# ComponentContainer 私有组件池

## 当前设计

### 职责与成员

Registry::Storage 内部每类型一个容器，拥有 SparseSetPool* pools、EntityIndex* entities、char* data；component_type/component_size、num_pools、num_components/max_num_components 描述容量。Registry 注册以 {} 清零局部容器再 InitRaw。普通默认构造未自动初始化裸成员，不供外部直接使用。

### 接口与状态

InitRaw(size,entity_index,type) 设置类型/大小，分配一个 8 项稀疏段、8 个 dense payload/实体索引；不检查全部初始分配失败再恢复。
GetPool 线性查找索引所属段；Get（类型/字节）用映射算 data+dense*size，无池返回 null 并记录错误，断言不是通用可恢复检查。
Add（类型/字节）必要时新增 sparse 段、dense 容量翻倍并写数据/实体映射；AddOrGet 用已有映射或新增默认值再取指针。IsComponentExist 查询段映射；GetComponentSize 取记录大小；GetPoolAlignedIndex 将实体索引按 8 向下对齐。
Remove 修改映射、尝试末项搬运并减计数；**当前删除实现有缺陷**，不能按预期 swap-remove 算法写成保证。
Free 显式释放非空三缓冲、置空并清数量，可重复清理，类型/记录大小不是重新注册证明。

### 所有权与特殊成员

没有 RAII 析构，也没有显式删除 shallow copy；Registry 注册时把局部容器复制进 vector 后由 Storage/Registry 唯一执行 Free，局部析构不释放。所有权靠内部协议而非类型强制，**不能向外复制得到两个独立所有者**。扩容使组件/稀疏段引用失效，主线程同步，不因分配器锁支持并发/重入。

### SparseSetPool 关联

专用数据小节与注册/失效原编号见 [ECS 数据 I2/I3](ECS-组件存储-数据设计.md#不变量)、[SparseSetPool](ECS-组件存储-数据设计.md#sparsesetpool)。本对象不重复维护它们。

### 审核风险与证据

- Remove 以 entity_index>=num_components 判断，混淆稀疏实体索引与 dense 数量；搬运 data[dense_index]=data[last_index] 仅一字节，不是 component_size payload。
- 两次 realloc 单独进行，任一失败不能原子恢复旧指针/容量；InitRaw 未完整处理 null，不承诺 OOM 安全。
- POD/类型号前提并不解决代数、删除、对齐与序列化全部风险；T5 保留未完成。

[头](../../../../game/modules/ecs/src/component_container.h)、[实现](../../../../game/modules/ecs/src/internal.cpp)、[Registry](ECS-存储边界-类设计.md)、[历史验收](ECS-存储边界.md#验收案例)、[覆盖清单](../对象笔记覆盖清单.md)。2026-10-06 源码核对，未重跑失败/删除测试，不擅自修复实现。

## 本次变更

无。

## 后续考虑

先完整修复与测试所有权、删除与失败恢复，再改变布局/扩容策略。

