---
type: 功能
status: 已验证（T0范围）
project: Symocraft
module: ecs
created: 2026-10-05
updated: 2026-10-06
tags:
  - area/architecture
---

# ECS-存储边界

关联设计：[Registry](ECS-存储边界-类设计.md)、[存储数据](ECS-组件存储-数据设计.md)、[Iterator](Iterator-类设计.md)、[RegistryViewer](RegistryViewer-类设计.md)、[ComponentContainer](ComponentContainer-类设计.md)。统一验收：[M3-T0](../../../spec/M3-T0-模块软硬边界.md)。

最终验证：[M3-T0 交付与验收报告](../../../milestones/m3-t0/README.md)。

## 当前设计

ECS 仅承担实体与组件存储。游戏组件与系统迁到 simulation；公开 Registry 模板不再包含 ComponentContainer、Amo 内存模板或世界 / 图形头。模板将类型与大小交给私有字节操作，底层保持原稀疏池算法。

实现位置：[模块源码](../../../../game/modules/ecs/CMakeLists.txt)。当前实现与历史验证见下文；不将已有 M2 结果冒充新实现证据。

### 既有 T0 实现与验收

### 目标与流程

T0 仅建立真实模块、公开数据契约及测试边界，维持原正常运行路径，不提前做领域算法重写。

```text
组件类型 → RegisterComponent → 私有类型注册与池初始化
实体 ID → Add/Get/View → 已登记组件引用
app 关闭 → Registry::Free → 私有池释放 → Registry 析构
```

### 关键约束与取舍

- Registry 独占 Storage，禁止复制。公开接口不提供 entities 向量或稀疏池布局。
- 按现有约定，各 Registry 必须以同样顺序注册组件；组件只支持 POD。
- Storage 的一次分配发生在 Registry 构造，转发调用不引入逐帧分配；原池扩容仍会使组件引用失效。
- 本轮隐藏实现并维持正常玩家路径，不把旧容器升级为完整可靠 ECS。版本校验、删除压缩、分配失败原子性和未使用的序列化缺陷明确留到 T5。
- RawMemory 的旧序列化仅留在私有实现，未作为可用存档接口暴露。

### 验收案例

| 状态 | 场景 | 预期行为 |
| --- | --- | --- |
| [x] | 独立公开头消费者 | 不需要聚合 core.h、SDK 或私有 include |
| [x] | ecs.registry | 已列注册、查询、清理等基本契约通过正式库验证，不代表全部存储正确性 |
| [x] | Debug / Release 与 CPU-only | 相同源码所有者；CPU 库无需窗口 |

2026-10-05 最终 Debug、Release、CPU-only 及玩家正常路径验证通过，证据统一见阶段报告。原 T4（现 T5）的版本校验、删除压缩、分配失败及序列化审计仍未执行；本轮没有将其标为通过。

## 本次变更

无。2026-10-06 仅整理文档；以上为原 T0 实现与验收记录，不是本次重新运行测试。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| 边界成为实际瓶颈 | 先测量分配、复制或调用开销，再调整接口；不恢复私有跨模块访问 |
| 后续阶段修改内部算法 | 同步更新本笔记、类不变量与回归证据 |

