---
type: 类设计
status: 已验证（T0范围）
project: Symocraft
module: scene
class_name: "BlockVertex3D / LineVertex3D / CameraView"
inheritance: []
created: 2026-10-05
tags:
  - area/architecture
---

# Scene-共享场景数据-类设计

关联功能：[Scene-共享场景数据](Scene-共享场景数据.md)。不存在的管理类不为模板而增造；自由函数与数据结构按实际实现记录。

最终验证：[M3-T0 交付与验收报告](../../../milestones/m3-t0/README.md)。

## 当前设计

职责：scene 是真正由 world、simulation 和 renderer 共享的 CPU 值契约，不是场景管理器。其 INTERFACE target 只携带公开数据与数学使用要求，没有无意义的空实现库。

### 数据成员

| 类型 | 成员 | 初值 / 范围 | 含义与所有权 |
| --- | --- | --- | --- |
| glm::ivec3 | BlockVertex3D::pos_coord | 调用方填充 | 现有整型世界顶点位置 |
| glm::vec3 | BlockVertex3D::tex_coord | 调用方填充 | UV 与纹理层，保持旧格式 |
| float | BlockVertex3D::normal | 调用方填充 | 旧法线面编码 |
| glm::mat4 | CameraView::projection / view | 单位矩阵 | 自有矩阵值 |
| span<const BlockVertex3D> | MeshView | 空 | 只读、短期借用，不拥有顶点 |

### 不变量

| 编号 | 条件 | 成立边界 |
| --- | --- | --- |
| I1 | sizeof(BlockVertex3D)==28，sizeof(LineVertex3D)==12 | 每次编译 |
| I2 | 消费方不延长 MeshView 超过提供者允许的作用域 | 每次访问 |
| I3 | 公开头不包含图形 SDK、世界或 ECS 私有类型 | 边界检查 |

### 接口与生命周期

| 接口 | 行为 | 前提 / 边界 |
| --- | --- | --- |
| 聚合构造与复制 | 搬运 CPU 数据 | 值类型无需初始化图形设备 |
| MeshView::data / size | 同步读取连续顶点 | 不缓存指针，不跨 world 编辑 / 清理使用 |
| CameraView | 渲染相机快照 | 不代表相机控制器或实体所有权 |

- BlockVertex3D 保留原 pos_coord、tex_coord、normal 布局与 28 字节尺寸，LineVertex3D 为 12 字节，有静态断言。
- MeshView 不拥有内存，只允许在所属 world 访问回调期间同步消费；renderer 不保存该视图。
- CameraView 只复制两份矩阵，不借用 ECS、相机或窗口。
- 不把 GPU 间接命令、原生句柄、chunk 状态或材质业务规则搬入 scene。

公开头：[include](../../../../game/modules/scene/include/symocraft/scene)。scene 只有数据头，故无 src 实现文件；不为文档链接预建空目录。不由本次迁移推定支持多线程或回调重入。

## 本次变更

本次将真实实现归入 scene，用公开契约替代旧聚合头依赖。成员、所有权与失效约束以上表为准；尚未完成验证的风险不以“拆库完成”代替。

### 验收案例

验收位置：[关联功能的验收案例](Scene-共享场景数据.md#验收案例)。顶点布局、公开头隔离、实际消费者增量编译和旧新 static 图像对照已经验证；I2 仍要求调用者不跨 world 修改/销毁保存借用视图，不因此推定异步传递安全。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| 新调用者需要改变生命周期 | 先修改契约与测试，再修改接口，不暴露存储布局解决临时需求 |

