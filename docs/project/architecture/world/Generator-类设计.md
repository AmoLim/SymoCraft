---
type: 类设计
status: 已验证（T0范围）
project: Symocraft
module: world
class_name: Generation::Generator
inheritance: []
created: 2026-10-05
tags:
  - area/architecture
---

# Generator 确定性生成设计

关联功能：[世界功能](World-功能.md)。

最终验证：[M3-T0 交付与验收报告](../../../milestones/m3-t0/README.md)。

## 当前设计

`Generator` 根据 Settings 提供高度与区块植被随机流；世界分配、排序遍历和写入由同命名空间的 `Build/Populate` 编排。它不拥有区块，也不处理输入和渲染。

### 数据成员

| 类型 | 成员 | 初值 / 范围 | 含义与所有权 |
| --- | --- | --- | --- |
| `Settings` | `settings_` | seed uint32、radius 2..10、vegetation | 自有配置值，构造后只读 |
| `unique_ptr<NoiseState>` | `noise_` | 成功构造后非空 | 独占三个噪声实例；具体 FastNoiseLite 类型在 cpp 内 |
| `array<int,3>` | `noise_seeds_` | 原 mix32 值掩码为非负 int | 确定性派生 seed，报告时只读借用 |

### 不变量

| 编号 | 条件 | 成立边界 |
| --- | --- | --- |
| G1 | `settings_.radius` 在 2..10，三路噪声配置与版本 1 一致 | 构造成功后 |
| G2 | 同 settings、坐标和旧库版本产生相同结果 | 单线程、相同编译数值环境 |
| G3 | `noise_` 只被本对象持有，不暴露库类型或地址 | 全生命周期 |

### 接口与生命周期

| 接口 | 行为 | 前提 / 边界 |
| --- | --- | --- |
| `Generator(Settings)` | 校验 radius，创建/配置三路噪声 | 参数错误抛异常；unique_ptr 负责失败清理 |
| `Height(x,z) const` | 按原频率、权重、Remap/指数计算高度 | const 不代表底层采样器线程安全 |
| `VegetationRandom(x,z)` | 返回按世界 seed 和区块坐标派生的 mt19937 值 | 不共享可变随机流 |
| `Config/NoiseSeeds` | 返回对象内只读引用 | 对象销毁后失效，不跨任务持有 |
| `Describe(Settings)` | 返回自有 Data::Value 元数据 | 无 YAML 节点借用，序列化由 app/telemetry 执行 |

拷贝和赋值禁用，当前不提供移动契约；使用栈局部 Generator 驱动同步生成。析构在 cpp 定义，从公开头隐藏 NoiseState 的完成类型。此处单次小型堆分配发生在生成/描述时，不在每帧采样循环中。

实现：[声明](../../../../game/modules/world/include/symocraft/world/generation.h)、[实现](../../../../game/modules/world/src/generation.cpp)。

## 本次变更

把 FastNoiseLite 成员移入私有 NoiseState，将 YAML 元数据改为项目值类型。半径校验、noise seed、植被写入顺序、摘要字节序不改。验收引用[世界功能](World-功能.md#验收案例)：重复生成、插入顺序、CPU-only 和旧新固定 seed 的实际摘要对照均通过。此结论限定当前版本、平台及参数，不推定多线程生成或更换噪声算法后仍确定一致。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| 多线程生成 | 每任务独立采样器和写入归属，先验证跨区块植被覆盖顺序 |
| 更换噪声库或数值算法 | 提升生成版本并重建对照组，不作为无行为变化的结构迁移 |
