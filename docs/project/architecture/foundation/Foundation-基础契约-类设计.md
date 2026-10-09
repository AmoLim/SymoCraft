---
type: 功能
status: 已验证（T0范围）
project: Symocraft
module: foundation
created: 2026-10-05
updated: 2026-10-06
tags:
  - area/architecture
---

# Foundation 基础契约摘要

历史文件名保留为入口，函数不虚构为类。关联：[功能与唯一验收](Foundation-基础契约.md)、[Value 对象](Value-类设计.md)、[覆盖清单](../对象笔记覆盖清单.md)。

## 当前设计

基础模块提供数学配置、整数类型、诊断、旧分配器和自有元数据；无 world/Window/Registry 服务定位器。Value 原 I1/I2 移入 [对象不变量](Value-类设计.md#不变量)，构建/deep copy/序列化不进入逐帧网格/绘制。

### AmoBase 与诊断

AmoBase 是 namespace，LogLevel 是诊断枚举。allocations、memory_mtx、track_memory_allocations、buffer_unit/哨兵与 log_mtx/log_level 是私有静态状态；Allocate/ReAlloc/Free/CopyMem、日志与追踪保留旧行为。局部锁不代表整个模块或 ECS 并发安全，旧分配器全面正确性/失败证明留 T5。

### DebugMemoryAllocation

私有全局 struct：const char* file_allocator（宏文件名借用）、int file_allocator_line/references、size_t memory_size、void* memory；相等按 memory 地址。追踪数组拥有记录，记录不负责释放 payload，不是公共分配 handle 或可靠共享引用计数。

### 不变量

原 I3：Files::Publish 仅处理已关闭、同卷的临时文件，调用者前提；失败异常报告，原子替换目标，无窗口依赖。

### 既有实现与依据

T0 归入 foundation 替代旧聚合头依赖；[功能验收](Foundation-基础契约.md#验收案例)保留自有值复制/类型边界、YAML 往返、状态发布与 Debug/Release/CPU-only 结论，整体交付用户验收边界不变。
[公开头](../../../../game/modules/foundation/include/symocraft/foundation)、[AmoBase](../../../../game/modules/foundation/src/legacy/AmoBase.cpp)、[T0 报告](../../../milestones/m3-t0/README.md)。2026-10-06 仅文档/源码核对，无新运行结果。

## 本次变更

无。

## 后续考虑

新生命周期或分配器需求先单独定义契约与测试，不借整理宣称已修复。

