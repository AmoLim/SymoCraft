---
type: namespace API设计
status: 自动验证完成（T1范围）
project: Symocraft
module: world
namespace_name: "SymoCraft::World / TestScene / Benchmark"
created: 2026-10-06
updated: 2026-10-06
tags:
  - area/architecture
---

# World 坐标与固定场景 API

关联：[World功能/统一验收](World-功能.md)、[场景值](World-数据设计.md)、[世界owner](World-类设计.md)、[Generator](Generator-类设计.md)。只登记无owner函数；World/定义/生成方法由对应类笔记唯一维护。

## 当前设计

职责：唯一浮点转格、静态场景/编辑序列、benchmark定时编辑算法。无全局当前世界；固定只读表不是World存储或任务队列。

### 状态与所有权

| 类型 / 状态 | 初值 / 范围 | 拥有者与使用者 | 有效期 / 失效条件 |
| --- | --- | --- | --- |
| checkpoints、edit_targets、edits | 静态7个Pose、8个目标/16次正向编辑 | test_scene.cpp自有只读表 | 静态存活；返回span/ref只读借用 |
| TestScene::Version/DefaultSeed | 1 / 424242 | inline constexpr值 | 场景版本/默认值，不绑定当前World |
| Benchmark::EditInterval | 0.25秒 | inline constexpr值 | 时间算法常量 |
| chunk尺寸/biome/sea/radius、BlockConstants | 固定值/面模板/完整Block | 公开constexpr值 | 不是可替换世界状态 |

### 公开接口预期行为

| 签名 / 入口 | 调用方与可见范围 | 预期行为：输出及状态变化 | 前提 / 边界 | 失败反馈及失败后状态 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| `std::optional<BlockCoord> World::TryToBlockCoord(const glm::vec3&) noexcept` | float旧调用方；world.h | 每轴double floor后检查i32，返回整数值 | 不用截断处理负数 | NaN/Inf/越界空optional；无非法转整数/状态变化 | world_coordinates.cpp |
| `std::span<const Pose> TestScene::Checkpoints()` | app/工具 | 静态表借用 | 不保存为动态owner | 不分配/不改World | test_scene.cpp |
| `const Pose& Checkpoint(std::string_view)` | app启动 | 名称查找静态记录 | name参数仅调用借用 | 未知invalid_argument | test_scene.cpp |
| `std::span<const Edit> Edits()` | benchmark/工具 | 固定16步表借用 | 正序语义不变 | 不修改World | test_scene.cpp |
| `void Install(World::VoxelWorld&)` | app固定场景 | 先校验覆盖，显式按既有顺序写场景 | radius>=3；无其他World隐式目标 | 覆盖失败写入前拒绝；后续编辑异常上抛，不承诺整场景事务 | test_scene.cpp |
| `void ApplyEdits(World::VoxelWorld&)` | CPU回归工具 | 逐步检查before，再TryEdit | 需新生成/安装对应场景 | 首错停止；已完成编辑保留 | test_scene.cpp |
| `Data::Value TestScene::Describe()` | app/报告 | 自有场景/路线/编辑元数据 | 无World访问 | 分配异常，不修改表/World | test_scene.cpp |
| `std::uint64_t Benchmark::DueEdits(double elapsed)` | app基准每帧 | floor(elapsed/0.25)计划总数 | 秒，有限0..1201 | 非法invalid_argument | benchmark_workload.h |
| `TestScene::Edit Benchmark::CycleEdit(std::uint64_t index)` | app基准 | 模32选择正向16步或逆向撤销16步，自有值 | 固定表非空 | 不编辑，不保存World | benchmark_workload.h |

### 私有函数预期行为

| 签名 / 入口 | 内部调用方 | 预期行为：处理规则及副作用 | 前提 / 边界 | 失败传播及清理责任 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| Write(VoxelWorld&, ivec3, ushort) | Install/ApplyEdits | Set并要求Accepted；Changed/Unchanged均接受 | 显式当前World | Rejected转runtime_error，资源异常传播 | test_scene.cpp |
| Vector(const vec3&) | Describe | 三元素自有flow序列 | 冷路径值 | 分配异常由局部值清理 | test_scene.cpp |
| edits初始化lambda | 静态初始化 | 每目标before→air→选定材料的16步值 | 只依静态目标 | 无动态World写入 | test_scene.cpp |

### 不变量与调用顺序

| 编号 | 可检查条件 | 成立边界 |
| --- | --- | --- |
| API-I1 | 所有场景写入使用调用方提供World与TryEdit | Install/ApplyEdits |
| API-I2 | Changed/Unchanged计合法应用；转换失败/Rejected/异常不算成功 | app/工具统计前提 |
| API-I3 | 静态表返回借用，Pose::name不复制字符串或借动态参数 | 表/记录访问 |

app创建World → Install可选 → 首网格 → 固定视角/路线/定时编辑；场景写入是同步且可部分完成，网格发布由World单独负责。生产固定场景不依赖test/support。RandomSeed与生成内部helpers见Generator笔记。

## 本次变更

Install/ApplyEdits显式传World；删除旧全局查询/编辑入口。API-I1-I3及坐标边界对应T1-A03/A04/A07/A12/A15；自动结果见 [统一验收](World-功能.md#验收案例)。

## 后续考虑

动态场景、输入回放或异步编辑需新寿命/时序契约，不推广静态表借用。
