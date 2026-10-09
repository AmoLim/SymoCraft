---
type: 类设计
status: 已验证（T0范围）
project: Symocraft
module: ecs
class_name: "SymoCraft::ECS::Registry"
inheritance: []
created: 2026-10-05
updated: 2026-10-06
tags:
  - area/architecture
---

# Registry 存储所有者

关联：[功能](ECS-存储边界.md)、[存储数据](ECS-组件存储-数据设计.md)、[覆盖清单](../对象笔记覆盖清单.md)。

## 当前设计

### 职责与成员

Registry 独占 unique_ptr<Storage>，构造 make_unique 创建；具体字段见 [Storage](ECS-组件存储-数据设计.md#storage)。只存储实体/POD 组件，不懂玩法，公开模板不泄漏 ComponentContainer/Amo/world/图形私有头。

### 不变量

| 原编号 | 条件与边界 |
| --- | --- |
| I1 | storage_ 唯一所有者，构造完成至析构 |
| I4 | Free 可重复执行，旧池指针置空；之后只继续清理/销毁，不继续 Add |

I2/I3 唯一维护于 [ECS 数据](ECS-组件存储-数据设计.md#不变量)，此处引用注册/借用前提。

### 接口与生命周期

| 入口 | 前提 / 输出 / 失败 |
| --- | --- |
| RegisterComponent<T> | 编译期 POD static_assert；RegisterType 接受类型/大小/名称，诊断检查共同顺序与 256 上限，不提供通用异常恢复 |
| CreateEntity / DestroyEntity | 建立/回收索引和版本编码；Destroy 先移除组件再标空，无完整失败事务 |
| IsEntityValid | 只检查传入索引小于表长度且非 UINT32_MAX，不核对槽内存活 ID/版本，不是 stale-ID 保证 |
| AddComponent / AddOrGetComponentByType | 已注册/有效实体前提；返回池内借用，失败 null 可能被模板解引用 |
| GetComponent / GetComponentByType | 模板断言有组件再解引用；部分字节接口非法时返回 null，不能概括为安全错误返回 |
| HasComponent / HasComponentByType | 匹配现有池；不使旧代 EntityId 安全 |
| NumComponents / RemoveComponent / RemoveAllComponent | 计数/移除池组件，继承容器删除风险 |
| View<...> | 返回 [RegistryViewer](RegistryViewer-类设计.md)，通过 [Iterator](Iterator-类设计.md)过滤 |
| Free / 析构 | 逐池 Free；Free 不清空全部实体/组件 vectors；析构再销毁 Storage |
| Clear | 清实体、Free、清组件/名称/回收列表；不重置全局 ComponentType，非任意重注册重开保证 |
| Serialize / Deserialize（私有保留） | 未启用旧实现，不是产品存档 API |

不可复制，用户声明析构而未定义移动，实际不可移动。app unique_ptr 持有；Camera/系统借用期必须被 Registry 覆盖。无线程/callback 重入保证，遍历中禁止结构修改或 Free。构造一次 Storage，不增加逐帧 Storage 分配；原池扩容仍分配/使引用失效，不能称全接口无分配。

### 既有迁移与审核风险

T0 将真实实现归 ecs，公开模板经字节入口转发，保留旧池算法；公开头隔离不是完整可靠 ECS 升级。版本校验、迭代 ID 截断、删除压缩、OOM 原子性与私有序列化仍留 [T5 风险](ECS-组件存储-数据设计.md#已知风险)。

[头](../../../../game/modules/ecs/include/symocraft/ecs/registry.h)、[实现](../../../../game/modules/ecs/src/registry.cpp)、[ComponentContainer](ComponentContainer-类设计.md)。
历史 [功能验收](ECS-存储边界.md#验收案例)、[T0 报告](../../../milestones/m3-t0/README.md)支持基本契约/公开头/玩家路径；2026-10-06 没有重跑游戏测试，没有完成原 T4（现 T5）。

## 本次变更

无。

## 后续考虑

新生命周期需求先修改契约/测试，不公开底层布局兜底；T5 先审计版本、删除、借用和失败恢复。

