---
type: 功能
status: 已验证（T0范围）
project: Symocraft
module: scene
created: 2026-10-05
tags:
  - area/architecture
---

# Scene-共享场景数据

关联类：[类设计](Scene-共享场景数据-类设计.md)。统一验收：[M3-T0](../../../spec/M3-T0-模块软硬边界.md)。

最终验证：[M3-T0 交付与验收报告](../../../milestones/m3-t0/README.md)。

## 当前设计

scene 是真正由 world、simulation 和 renderer 共享的 CPU 值契约，不是场景管理器。其 INTERFACE target 只携带公开数据与数学使用要求，没有无意义的空实现库。

实现位置：[模块源码](../../../../game/modules/scene/CMakeLists.txt)。当前为迁移实现，验证状态见本次变更；不将已有 M2 结果冒充新实现证据。

## 本次变更

### 目标与流程

T0 仅建立真实模块、公开数据契约及测试边界，维持原正常运行路径，不提前做领域算法重写。

```text
world 私有 Chunk 顶点 → 临时 MeshView → app → renderer 批次
simulation 相机姿态 → CameraView → renderer
```

### 关键约束与取舍

- BlockVertex3D 保留原 pos_coord、tex_coord、normal 布局与 28 字节尺寸，LineVertex3D 为 12 字节，有静态断言。
- MeshView 不拥有内存，只允许在所属 world 访问回调期间同步消费；renderer 不保存该视图。
- CameraView 只复制两份矩阵，不借用 ECS、相机或窗口。
- 不把 GPU 间接命令、原生句柄、chunk 状态或材质业务规则搬入 scene。

### 验收案例

| 状态 | 场景 | 预期行为 |
| --- | --- | --- |
| [x] | 独立公开头消费者 | 不需要聚合 core.h、SDK 或私有 include |
| [x] | 公开头独立消费者 + world.mesh_safety | 正常 / 失败路径通过正式库验证 |
| [x] | Debug / Release 与 CPU-only | 相同源码所有者；CPU 库无需窗口 |
| [x] | 共享公开头增量编译与实际渲染 | 真实消费者重新编译；旧新 static 截图字节一致 |

2026-10-05 正式测试矩阵、公开头消费者、共享头增量验证及真实 OpenGL 对照通过。证据与限制统一见最终报告；scene 仍只是 CPU 数据契约，不表示已建立场景图或新的资源句柄系统。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| 边界成为实际瓶颈 | 先测量分配、复制或调用开销，再调整接口；不恢复私有跨模块访问 |
| 后续阶段修改内部算法 | 同步更新本笔记、类不变量与回归证据 |

