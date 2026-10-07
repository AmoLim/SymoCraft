---
type: 类设计
status: 自动验证完成（T1范围）
project: Symocraft
module: world
class_name: "SymoCraft::Generation::Generator"
inheritance: []
created: 2026-10-05
updated: 2026-10-06
tags:
  - area/architecture
---

# Generator 确定性采样与生成配置

关联：[World工厂/显式写入](World-类设计.md)、[Settings/Timings](World-数据设计.md)、[统一验收](World-功能.md#验收案例)。源码：[公开头](../../../../game/modules/world/include/symocraft/world/generation.h)、[实现](../../../../game/modules/world/src/generation.cpp)。

## 当前设计

Generator是构建期局部RAII采样器，持有配置/噪声/派生seed；不拥有World/Chunk，不保留全局随机游标。World::Impl::Populate负责编排显式目标写入并复制实际NoiseSeeds；工厂返回后Generator销毁，World保留settings/seeds/definition，Describe冷路径构造报告树。

<a id="noisestate"></a>
### 数据成员

| 类型 | 成员 | 初值 / 范围 | 含义与所有权 |
| --- | --- | --- | --- |
| Settings | settings_ | seed uint32、radius2..10、vegetation | 自有只读配置 |
| unique_ptr<NoiseState> | noise_ | 构造成功非空 | 独占三个FastNoiseLite，只在cpp露出库类型 |
| array<int,3> | noise_seeds_ | 原mix32结果掩码为非负int | 自有派生seed |
| array<FastNoiseLite,3> | NoiseState::noises | 构造配置 | 私有噪声对象；API非const，不证明线程安全 |

### 不变量

| 编号 | 条件 | 成立边界 |
| --- | --- | --- |
| G1 | radius2..10，三路噪声配置与Version=1一致 | 构造成功 |
| G2 | 同设置、坐标、噪声/编译数值环境产生同结果，植被每块独立派生流 | 串行同环境，不承诺跨平台位一致 |
| G3 | noise_仅本对象拥有，Config/NoiseSeeds只借自身数据 | 全生命周期 |

### 公开接口预期行为

| 签名 / 入口 | 调用方与可见范围 | 预期行为：输出及状态变化 | 前提 / 边界 | 失败反馈及失败后状态 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| explicit Generator(Settings) | World::Create/CPU | 先校验radius，再分配/配置三路噪声 | 原版本/seed派生 | 参数invalid_argument；分配异常由成员RAII清理 | generation.cpp / G1 |
| ~Generator() | 构建期owner | 出线析构噪声 | 同步采样结束 | 不抛，不手工FreeNoise | generation.cpp / G3 |
| copy/move ctor/assign = delete | 调用者 | 不复制/迁移噪声对象 | 独立世界各建采样器 | 编译期拒绝 | generation.h |
| float Height(int x,int z) const | Populate/CPU | 原频率/权重/Remap、clamp与1.19指数 | 坐标合法整数；const不代表并发安全 | 不改World | generation.cpp / G2 |
| mt19937 VegetationRandom(int chunk_x,int chunk_z) const | Populate | 返回seed+坐标mix派生的自有随机流 | 不共享可变游标 | 值独立，不改World | generation.cpp / G2 |
| const Settings& Config() const | 元数据/构建 | 当前实际settings_借用 | 仅至Generator销毁 | 不复制/不延寿 | generation.h / G3 |
| const array<int,3>& NoiseSeeds() const | 元数据/CPU | 派生seed只读借用 | 仅至Generator销毁 | 不暴露噪声地址 | generation.h / G3 |
| uint32_t Generation::RandomSeed() | app非固定seed | random_device取值 | 非确定性入口；固定基准不使用 | 平台random_device异常传播 | generation.cpp |

### 私有函数预期行为

namespace无C++ private，以下按cpp/TU实际范围登记。

| 签名 / 入口 | 内部调用方 | 预期行为：处理规则及副作用 | 前提 / 边界 | 失败传播及清理责任 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| Remap(float,float,float,float,float)（TU） | Height | 固定区间线性映射 | 内部high!=low | 不写World | generation.cpp |
| Mix(uint32_t)（TU） | ctor/VegetationRandom | 原mix32无符号溢出混合 | 固定常量，不改版本 | 不分配/不共享游标 | generation.cpp |
| Data::Value Metadata(const Settings&, const std::array<int,3>&, const World::BlockDefinition&)（cpp内部） | World::Describe | 实例settings/seeds/定义 → 自有原schema | 完整必需ID1..11，借用仅调用内；不重建采样器 | 值分配异常上抛，World不变 | generation.cpp |
| Impl::Populate / WriteGenerated | World工厂 | [World私有行为权威表](World-类设计.md#私有函数预期行为) | 显式目标，构建期未发布 | 工厂RAII，不返回半世界 | generation.cpp / W-I1 |

### 顺序与生命周期

Create局部Generator → 分配全部Chunk → 全地形 → 按Chunk x,z顺序植被 → 记录计时并复制seeds → 销毁Generator；Describe按需构造报告。terrain内z,x,y写入，摘要/mesh各保留原顺序；跨块植被后写覆盖保留。边缘为空气且不建可绘制mesh；生成不是逐帧扫描。

Timings可选借用只到Create返回，分别记录allocation/terrain/vegetation毫秒；不会把初始mesh并入生成。半径10为441块/28,901,376格，半径2..10有效；本轮不改噪声库、浮点数值算法、植被候选/随机消耗或digest schema。

## 本次变更

T1删除公开Build/Populate/BlockDigest/FindSpawn/Describe(Settings)隐式世界入口；生成阶段只保存实例事实，World::Describe按需建立自有树，避免把报告分配混入生成分段。T0历史结论见 [T0报告](../../../milestones/m3-t0/README.md)；本轮G1-G3与摘要/顺序的自动对照见 [T1唯一验收](World-功能.md#验收案例)。

## 后续考虑

后台生成需独立噪声与写入范围/植被提交顺序；更换算法需提升生成版本及新对照，不归结构迁移。
