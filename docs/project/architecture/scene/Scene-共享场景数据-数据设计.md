---
type: 数据设计
status: 自动验证完成（T1范围）
project: Symocraft
module: scene
created: 2026-10-05
updated: 2026-10-06
tags:
  - area/architecture
---

# Scene 共享 CPU 值与网格

关联：[共享场景功能](Scene-共享场景数据.md)、[World发布](../world/World-类设计.md)、[覆盖清单](../对象笔记覆盖清单.md)。源码：[mesh.h](../../../../game/modules/scene/include/symocraft/scene/mesh.h)、[camera.h](../../../../game/modules/scene/include/symocraft/scene/camera.h)。

## 当前设计

scene仅描述中立CPU值/视图，target为`symocraft_scene` INTERFACE；没有场景管理器、GPU句柄、World/Chunk状态或材质业务规则。

### 工作负载与数据语义

| 记录 / 类型 | 字段 | 初值 / 范围 / 单位 | 含义与来源 |
| --- | --- | --- | --- |
| BlockVertex3D | glm::ivec3 pos_coord | 生产者显式填充 | 世界整数位置 |
| BlockVertex3D | glm::vec3 tex_coord | 生产者显式填充 | UV与纹理层 |
| BlockVertex3D | float normal | 生产者显式填充，旧mesh为0 | 保留旧法线面编码 |
| LineVertex3D | glm::vec3 pos_coord | 生产者显式填充 | 选择框线 |
| CameraView | glm::mat4 projection/view | 单位矩阵 | 自有相机矩阵，不借ECS/Window |
| MeshData | vector<BlockVertex3D> vertices | 空 | 自有连续三角形顶点，不绑定GPU |
| MeshView | span<const BlockVertex3D>别名 | 默认空 | 只读连续借用，不拥有内存 |

world生成/保存BlockVertex3D AoS，app在callback内转交renderer当场按整条记录复制；CameraView每帧复制两矩阵。MeshData没有固定上限或scene分配器，容量由World/Mesher/消费者控制；scene本身无热循环，布局/缓存收益未独立测量。

### BlockVertex3D

保持字段顺序与28字节standard-layout。GPU属性对应由renderer核对，world展开顺序由Mesher控制；不把静态尺寸断言当作所有顶点数据合法的运行时校验。

### LineVertex3D

12字节普通值，选择框生产/消费归simulation/app/renderer。

### CameraView

复制矩阵自有独立；改变原相机不回写已复制矩阵，没有借用者生命周期问题。

### MeshData

vertices可复制/移动。World最终网格与Mesher候选各有独立vector，完整候选返回后由World交换发布。扩容/释放可使借用悬空；移动/交换改变owner，不保证原借用契约继续有效，但vector交换本身不使元素引用悬空。World视图仍只承诺当前callback，不在此增加版本或资源handle。

### MeshView

span复制仅复制指针/长度。World::VisitMeshes提供的view最长当前callback，消费者长期需要顶点时当场取得自有副本；空span可表示已发布空网格，是否已发布由World记录决定，不靠span为空推断。

### 所有权与不变量

| 编号 | 可检查条件 | 成立边界 |
| --- | --- | --- |
| I1 | BlockVertex3D=28字节standard-layout，LineVertex3D=12字节 | 编译期静态断言 |
| I2 | 不延长MeshView超过提供者规定callback，不销毁借用中的owner | 调用者前提；World动态拒绝修改/重入但不检测悬空指针 |
| I3 | 公开头无图形SDK、World/ECS私有类型 | 构建边界检查 |
| I4 | MeshData自有vertices；候选/最终网格不借Mesher scratch | World/Mesher成功与失败路径 |

### 公开接口预期行为

| 签名 / 入口 | 调用方与可见范围 | 预期行为：输出及状态变化 | 前提 / 边界 | 失败反馈及失败后状态 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| 顶点/CameraView聚合构造、复制、移动 | CPU生产/消费者 | 数值复制；CameraView单位矩阵默认 | 顶点完整填充 | 无scene自有清理/校验函数 | mesh.h/camera.h / I1/I3 |
| MeshData聚合构造、复制、移动/vertices操作 | World/CPU | 标准vector所有权；复制自有顶点、移动转移存储 | 不维持已有span；分配由标准vector | 分配异常按vector保证；owner决定发布 | mesh.h / I4 |
| MeshView构造/复制/标准span访问 | VisitMeshes→AppendMesh | 零顶点复制只读借用 | 有效连续范围，callback内 | 越界/过期借用无本模块恢复 | mesh.h / I2 |

### 私有函数预期行为

不适用：无cpp/私有函数或存储owner；数据生产/发布见World/Mesher，GPU消费见renderer笔记。

## 本次变更

T1新增MeshData并使World记录明确使用MeshView；保留顶点/相机布局与T0依赖边界。历史公开头/增量/图像对照见 [T0报告](../../../milestones/m3-t0/README.md)，不能直接作为T1新增所有权/空网格验收。I1-I4的本轮自动结果见 [T1唯一验收](../world/World-功能.md#验收案例)。

## 后续考虑

T2句柄/增量提交不混入CPU值；异步/长寿命传输另定义owner与版本，不缓存当前span。

