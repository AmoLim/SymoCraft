---
type: 类设计
status: 草稿
project: Symocraft
module: foundation
class_name: "SymoCraft::Data::Value"
inheritance: []
created: 2026-10-06
updated: 2026-10-06
tags:
  - area/architecture
---

# Value 自有元数据树

## 当前设计

### 职责与成员

拥有协议元数据，供 world 描述与 telemetry 序列化；不是配置服务、对象注册表或逐帧热路径容器。
value_ 为 Storage variant：monostate/bool/int64/uint64/float/double/string/Sequence/Mapping；
Sequence=vector<Value>；Mapping=vector<pair<string,Value>> 保持插入顺序；flow_style_=false 为序列化样式提示。默认未定义，float/double 区分以保留 YAML 精度。

### 不变量

原 Foundation I1：完成复制后不与原文档共享子节点。
原 Foundation I2：已有不兼容结构不能以键 [] / push_back 静默换类型；不兼容抛 invalid_argument。

### 接口与状态变化

### 公开接口预期行为

定义见 [document.h](../../../../game/modules/foundation/include/symocraft/foundation/document.h)、[document.cpp](../../../../game/modules/foundation/src/document.cpp)。全部为公开类成员；子节点引用只在 owner 未增长、替换、销毁期间短期借用。

| 签名 / 入口 | 调用方与可见范围 | 预期行为：输出及状态变化 | 前提 / 边界 | 失败反馈及失败后状态 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| `Value()` | Value 消费者 | monostate，flow=false | 无 parser 节点 | 默认构造语义 | document.h |
| `Value(bool)` | Value 消费者 | 存 bool | 与“已定义”判据不同 | 无分配 | document.h |
| `Value(const char*)` | Value 消费者 | 复制 C 字符串 | 非空、有结束符 | 分配异常；非法指针无检查 | document.h |
| `Value(string)` | Value 消费者 | 移动值参数到存储 | 不借用原字符串 | 参数构造可能分配 | document.h / I1 |
| `Value(string_view)` | Value 消费者 | 复制为自有 string | view 至构造返回有效 | 分配异常 | document.h / I1 |
| `Value(Sequence)` | Value 消费者 | 移动入参，持有自有子节点 | lvalue 先复制参数 | 复制/分配异常 | document.h / I1 |
| `Value(Mapping)` | Value 消费者 | 移动入参，保留顺序 | 不检查重复 key | 复制/分配异常 | document.h / I1 |
| `template<class T> Value(T)` requires arithmetic 且非 bool | Value 消费者 | float 保留；其他浮点转 double，有符号转 int64，无符号转 uint64 | C++ cast，无全面范围校验 | cast 合法性由调用方保证 | document.h |
| `template<class T> Value(const vector<T>&)` | Value 消费者 | 空 sequence 逐项 Value 转换并 push | T 可构造 Value | 转换/分配异常，失败构造的成员自动清理 | document.h / I1 |
| `Value& operator[](string_view)` | 元数据写入 | 未定义转 map，线性找键，缺失追加；返回借用子项 | 增长使旧子引用失效 | 非 map invalid_argument；分配失败可能已转空 map，无完整事务 | document.cpp / I2 |
| `const Value& operator[](string_view) const` | 元数据读取 | 命中借用项，未命中/非 map 返回静态 missing | 命中依 owner；missing 进程期有效 | 不插入，不抛缺键错误 | document.cpp |
| `Value& operator[](size_t)` | sequence 写入 | at 返回借用项 | sequence 且索引有效 | bad_variant_access / out_of_range，不自动增长 | document.h |
| `const Value& operator[](size_t) const` | sequence 读取 | const at 返回借用项 | sequence 且索引有效 | bad_variant_access / out_of_range | document.h |
| `push_back(Value)` | sequence 构建 | 未定义转 sequence，移动追加 | 不静默转换其他类型 | invalid_argument；分配失败可能已转空 sequence | document.cpp / I2 |
| `IsDefined() const` | Value 消费者 | 非 monostate true | 不转换标量 | 无写入 | document.h |
| `IsMap() const` | Value 消费者 | Mapping 分支 true | 不校验内容 | 无写入 | document.h |
| `IsSequence() const` | Value 消费者 | Sequence 分支 true | 不校验内容 | 无写入 | document.h |
| `IsScalar() const` | Value 消费者 | 已定义且非 map/sequence | 不校验 scalar 值 | 无写入 | document.h |
| `explicit operator bool() const` | 条件判断 | 返回 IsDefined；Value(false) 仍已定义 | 非存储 bool 数值 | 无写入 | document.h |
| `size() const` | Value 消费者 | map/sequence 长度，其他 0 | 不返回 string 字符数 | 无错误值 | document.cpp |
| `SetFlowStyle(bool = true)` | YAML 写作 | 改 flow_style 提示 | 不改 Storage | 无分配 | document.h |
| `FlowStyle() const` | YAML Encode | 返回样式提示 | 不表示已序列化 | 无写入 | document.h |
| `const Storage& storage() const` | YAML Encode / 消费者 | 借用 const variant | owner 存活；变更影响借用内容/子引用 | 无寿命检查 | document.h |
| `template<class T> T as() const` | 类型提取 | 同类型按值复制；算术类型间 cast | 无范围/溢出校验 | 不兼容 invalid_argument；复制可分配；valueless variant 可 bad_variant_access | document.h / I1 |

### 私有函数预期行为

无命名 private 方法，private 两项为数据成员；此表覆盖公开 as 的内部 lambda，不增加 API。

| 签名 / 入口 | 内部调用方 | 预期行为：处理规则及副作用 | 前提 / 边界 | 失败传播及清理责任 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| `as<T>` 内 visit lambda `(const auto&) -> T` | as<T> | 同类型 return；两端算术 cast；否则 throw | T/U 为实际模板类型；不写 owner | 异常上抛；返回副本按标准语义清理 | document.h |

标准容器/variant 自动销毁；隐式 deep copy，隐式移动遵循标准有效但未指定源内容，不保证源为空。增长、替换、销毁可使子节点引用失效。不同文档不共享节点不等于对同一文档并发写安全；无线程/重入保证。

### 成本、来源与风险

映射线性查找，deep copy/增长会分配；构建和 YAML 转换在启动/采样之后，不宣称 DOD 优化或无分配。规模/瓶颈未测量。
[声明](../../../../game/modules/foundation/include/symocraft/foundation/document.h)、[实现](../../../../game/modules/foundation/src/document.cpp)、[基础契约](Foundation-基础契约-类设计.md)、[功能验收](Foundation-基础契约.md#验收案例)、[T0 报告](../../../milestones/m3-t0/README.md)、[覆盖清单](../对象笔记覆盖清单.md)。
2026-10-06 从汇总抽取，未重跑值复制/YAML 往返/OOM 测试。

## 本次变更

无。

## 后续考虑

若元数据变热路径，先测规模/查找分布，再单独批准布局变化。

