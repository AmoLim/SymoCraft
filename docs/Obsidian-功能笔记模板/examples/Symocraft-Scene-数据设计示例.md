---
type: 数据设计
status: 草稿
project: Symocraft
module: scene
created: 2026-10-05
tags:
  - 示例
  - area/templates
---

# Scene 共享值与网格视图数据设计示例

> 按 [07 数据与存储模板](../templates/07-数据与存储设计.md) 整理的教学摘录，2026-10-05 核对下列源码。不是新的生产设计权威笔记，未独立运行编译或渲染验收；原验证结论只在来源中维护。

关联功能：[Scene 共享场景数据](../../project/architecture/scene/Scene-共享场景数据.md)。抽取来源：[原 Scene 设计](../../project/architecture/scene/Scene-共享场景数据-数据设计.md)、[World 存储](../../project/architecture/world/World-类设计.md)、[Renderer 设计](../../project/architecture/renderer/Renderer%20namespace%20API.md)。

## 当前设计

职责 / 数据边界：scene 提供 world、simulation、renderer 共享的 CPU 值和只读视图，不拥有世界、ECS、窗口或 GPU 资源，也不是场景管理器。

使用场景 / 数据规模 / 更新频率：world 在同步网格访问中提供每个非空、非边缘区块的顶点；app 将视图交给 renderer。相机输入是两份矩阵值。视图本身不规定顶点容量或存储增长策略；那些由 world 与 renderer 的拥有者维护。

### 工作负载与访问模式

| 处理操作 / 消费方 | 规模 / 频率 | 读取与写入的字段 | 访问路径 |
| --- | --- | --- | --- |
| renderer 的 `AppendMesh` / Batch 追加 | 主循环每帧访问所有可提供的网格；每份顶点数由视图长度决定 | 复制完整 BlockVertex3D 记录；具体顶点属性在绘制时使用 | 从 world 连续范围复制到 renderer 自有容器，不按实体 ID 查询 |
| app 准备 CameraView | 每个实际渲染帧两份矩阵 | 从 Camera 计算结果建立矩阵值，renderer 读取 | 小型值交接；没有批量组件存储或热/冷字段拆分 |

本例记录已有连续顶点范围与聚合字段的消费方式，不把它改成 SoA 或不可变世界。目标硬件、缓存命中率、带宽和复制耗时在本摘录中未测量；这是一份支持引擎 DOD 分析的数据契约示例，不是已完成性能优化的证明。

### 数据语义

| 记录 / 类型 | 字段 | 初值 / 范围 / 单位 | 含义与来源 |
| --- | --- | --- | --- |
| `BlockVertex3D` | `glm::ivec3 pos_coord` | 由 world 网格生成填入；整数世界格点坐标 | 方块面顶点位置，不是实体或区块 ID |
| `BlockVertex3D` | `glm::vec3 tex_coord` | 由 world 填入 | 前两分量为 UV，第三分量为纹理层 |
| `BlockVertex3D` | `float normal` | 由 world 填入 | 旧面法线编码，不将字段类型误写成三维法线向量 |
| `LineVertex3D` | `glm::vec3 pos_coord` | 由调用方填入 | 线顶点位置值 |
| `CameraView` | `glm::mat4 projection / view` | 默认单位矩阵 | 投影与观察矩阵的自有副本，来自 Camera 计算 |
| `MeshView` | `std::span<const BlockVertex3D>` 的数据与长度 | 空或提供者传入的范围 | 描述已有顶点，不复制或拥有它们 |

顶点字段没有统一默认初始化器；调用方必须在消费前填写所需字段，不能把聚合类型误写成默认全零保证。

### 表示与布局

| 逻辑数据 | 实际表示 / 存储 | 访问方式与约束 |
| --- | --- | --- |
| 方块网格 | world 的 `vector<BlockVertex3D>`，记录内字段聚合排列 | MeshView 提供连续只读顶点；不推定为字段 SoA 或 ECS archetype |
| 临时网格访问 | span 描述该 vector 的范围 | `VisitMeshes` 同步调用消费者；访问顺序沿用当前 map，不承诺稳定排序 |
| 渲染批次 | renderer 自有顶点容器 | `AppendMesh` 在调用内复制顶点，返回后不保存输入 span |
| 相机描述 | 两个矩阵的自有值 | app 创建 CameraView 交给 renderer，不借用 Camera 或组件 |

### 身份与索引

- 顶点和 CameraView 不包含实体 ID、版本或资源句柄；实体注册与代数规则不适用。
- span 下标仅定位当前范围中的顶点，不是可跨帧复用的身份。
- world 区块身份和 GPU 资源身份由其各自模块维护，不搬入 scene。

### 所有权与有效期

| 数据 / 访问结果 | 拥有者与访问方 | 有效范围 / 失效条件 |
| --- | --- | --- |
| world 顶点 | Chunk 拥有 vector，app / renderer 只在回调中读取 | 本契约只允许当前 `VisitMeshes` 回调期间访问；重建、释放或改变存储可能使旧地址失效 |
| MeshView | 不拥有顶点；复制 span 只复制描述 | 复制不会延长底层数据寿命，不得缓存到下一帧 |
| renderer 批次顶点 | renderer 拥有从输入复制的内容 | 按 Batch 生命周期管理，不延长 world 借用 |
| CameraView | 值持有者拥有两份矩阵 | 不受源 Camera / ECS 后续修改影响，仍需调用方更新下一帧值 |

访问边界：当前 app 串行主循环同步交接；world 回调期间不允许编辑、生成、清空或重入修改世界。不从只读 span 或值类型推定所有者线程安全。

### 结构变化与不变量

scene 没有增删实体或扩容组件池的接口。结构变化属于数据拥有者：world 编辑与网格重建先完成，再提供视图；renderer 追加复制受其容量和初始化契约约束，失败不能被称为 scene 的事务回滚。

沿用原 Scene 编号作为示例展示，不重编号成 D1-D3；正式维护位置仍是原生产笔记。

| 编号 | 可检查条件 | 成立边界 |
| --- | --- | --- |
| I1 | `sizeof(BlockVertex3D)==28`、`sizeof(LineVertex3D)==12` | 公开头有静态断言；本次只读核对，未重新编译 |
| I2 | 消费方不将 MeshView 延长到 world 允许的同步访问范围之外 | 调用者前提；类型本身不能阻止保存 span |
| I3 | scene 公开头不依赖图形 SDK、world 或 ECS 私有类型 | 公开头边界；完整自动检查结论引用原验收 |

### 按需补充：布局契约

`BlockVertex3D` 有 standard-layout 静态断言；renderer 用 `offsetof` 设置三个顶点属性。保留尺寸和字段格式，不据此声明任意平台/编译配置 ABI 相同。scene 无独立容量、分配器或序列化接口，不增加这些章节。

实现与核对位置：[顶点与视图](../../../game/modules/scene/include/symocraft/scene/mesh.h)、[相机值](../../../game/modules/scene/include/symocraft/scene/camera.h)、[网格生成](../../../game/modules/world/src/chunk.cpp)、[同步访问的 T0 历史记录](../../milestones/m3-t0/README.md)、[渲染复制入口](../../../game/modules/renderer/src/renderer.cpp)、[批次追加](../../../game/modules/renderer/src/batch.hpp)。原同步访问源码 `chunk_manager.cpp` 已在 T1 重构中移除，此处保留历史教学语义，不改为新 World 接口的验证结论。

原验收依据：[Scene 验收案例](../../project/architecture/scene/Scene-共享场景数据.md#验收案例)、[T0 交付报告](../../milestones/m3-t0/README.md)。没有测量此摘录对应的新性能收益，也没有异步访问验收。

## 本次变更

无。本文只展示现有数据契约的写法，不修改产品数据布局，也不表示正式笔记迁移已执行。验收在上述来源中唯一维护，本示例不复制勾选表。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| 需要异步消费 world 网格 | 先明确自有输入或快照、版本与发布规则；当前 span 不能直接跨任务传递 |
| 修改顶点格式 | 同步检查生产数据契约、所有消费者和布局验证；不只修改示例 |
| 正式笔记完成迁移 | 同步来源链接；示例事实需另行核对后才更新摘录日期 |
