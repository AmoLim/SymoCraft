---
type: 类设计
status: 已验证（T0范围）
project: Symocraft
module: foundation
class_name: "Value"
inheritance: []
created: 2026-10-05
tags:
  - area/architecture
---

# Foundation-基础契约-类设计

关联功能：[Foundation-基础契约](Foundation-基础契约.md)。不存在的管理类不为模板而增造；自由函数与数据结构按实际实现记录。

最终验证：[M3-T0 交付与验收报告](../../../milestones/m3-t0/README.md)。

## 当前设计

职责：基础模块提供显式数学配置、整数类型、最小诊断、旧分配器适配和拥有数据的元数据值。没有世界、窗口、Registry 或图形服务定位器。

### 数据成员

| 类型 | 成员 | 初值 / 范围 | 含义与所有权 |
| --- | --- | --- | --- |
| Storage | Value::value_ | monostate | 拥有标量、字符串、序列或有序映射 |
| bool | Value::flow_style_ | false | 序列化样式提示，不是解析器枚举 |
| 私有静态状态 | AmoBase allocations / memory_mtx | 空 / 未锁定 | 旧分配器跟踪，由 foundation 独占 |

### 不变量

| 编号 | 条件 | 成立边界 |
| --- | --- | --- |
| I1 | Value 的副本不与原文档共享子节点 | 完成复制后 |
| I2 | 已有标量不能以 [] 或 push_back 静默变成另一种结构 | 修改入口 |
| I3 | Files::Publish 只处理已关闭、同卷的临时文件 | 调用前提 |

### 接口与生命周期

| 接口 | 行为 | 前提 / 边界 |
| --- | --- | --- |
| operator[] / push_back | 构造拥有数据的树；类型不兼容时抛异常 | 增长容器会使先前子元素引用失效，不跨后续扩容保存引用 |
| as<T> | 读取明确的标量类型 | 数值转换沿用 C++ 转换规则，不作为输入校验器 |
| Files::Publish | 原子替换目标 | 失败以异常报告；不建立窗口依赖 |

- Value 是世界描述和 telemetry 元数据的共同词汇，不是配置系统或对象注册表。
- Mapping 保持插入顺序，float 与 double 分别保存，避免迁移改变已有 YAML 精度。
- 拷贝深拷贝，不共享 YAML 节点；序列化只在 telemetry 中进行。
- 元数据构建、深拷贝和序列化不进入逐帧网格与绘制路径。旧分配器风险保留到 M3-T4，不宣称已经安全重写。

公开头：[include](../../../../game/modules/foundation/include/symocraft/foundation)；实现：[src](../../../../game/modules/foundation/src)。除明确的私有分配器锁外，不由本次迁移推定支持多线程或回调重入。

## 本次变更

本次将真实实现归入 foundation，用公开契约替代旧聚合头依赖。成员、所有权与失效约束以上表为准；尚未完成验证的风险不以“拆库完成”代替。

### 验收案例

验收位置：[关联功能的验收案例](Foundation-基础契约.md#验收案例)。自有值复制/类型边界、YAML 往返与状态文件发布已由正式库测试和集成运行验证，Debug/Release/CPU-only 均通过。旧分配器全面正确性、并发及分配失败证明不在本轮结论内；整体交付待用户验收。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| 新调用者需要改变生命周期 | 先修改契约与测试，再修改接口，不暴露存储布局解决临时需求 |

