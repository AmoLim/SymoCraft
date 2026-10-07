---
type: 契约索引
status: T1 已交付，T3-R1 待冻结
project: Symocraft
module: M3-T1-T3
created: 2026-10-06
tags:
  - area/spec
---

# T3 CPU 契约清单

用途：T1 → T3-R1 的交接核对入口，不另定义一份数据契约。下表链接当前实现的权威笔记；T3 候选接口仍以 [T3 Spec](M3-T3-渲染器重构.md) 为准，清单存在不表示 T3 已实施或 T1 节点已批准。

前置平台入口：[T2 SDL3 迁移正式计划](M3-T2-SDL3迁移.md#已确定的决策)。已选 D03/A，T2 须准备 GL、无 GL 原生窗口和 Vulkan 三种模式，并完成真实 GL 呈现、HWND、Vulkan instance/surface 与 loader 清理验证；实际进度以 [T2 准备记录](../milestones/m3-t2/README.md) 为准，不推定节点已验收。用户已确认先迁移再冻结渲染器：T2 验收后，在 T3-R1 定稿前核对窗口/像素/私有桥接证据，不将探针视为 T3 现代后端的完整玩法接管验收。这不修改下表已交付 CPU 数据，也不提前冻结候选纹理数组和相机输入。

## T1 已交付

| 交接项        | T3 消费时必须保留的边界                                                                                                  | 权威入口                                                                                                                                             |
| ---------- | -------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------ |
| CPU / 构建边界 | scene 是 CPU-only INTERFACE；world 不依赖 renderer/GPU。app 转接 World 记录，renderer 不接收 World/Chunk 私有对象                | [依赖图](../project/architecture/Overview.md#编译依赖)、[公开头清单](../milestones/m3-t1/evidence/public-headers.tsv)                                         |
| 顶点与网格      | `MeshData` 自有顶点，`MeshView` 只读借用；28 字节 `BlockVertex3D`、三角形列表、每面六顶点及原展开顺序保留。后端自行核对 GPU 布局，不能将 CPU 布局视作通用 GPU ABI | [Scene 数据](../project/architecture/scene/Scene-共享场景数据-数据设计.md)、[Mesher](../project/architecture/world/ChunkMesher-类设计.md)                        |
| 稳定身份       | `MeshIdentity = WorldId + ChunkCoord`；ChunkCoord 的 x/y 表示世界 x/z。重建不改变身份，不用地址作缓存 key，不将进程内身份当持久化 ID             | [World 数据语义](../project/architecture/world/World-数据设计.md#数据语义)                                                                                   |
| 发布版本       | `record.revision` 对应当前顶点；`mesh_input_revision` 可更新而旧网格仍有效。T3 只在资源更新成功后记录对应的已发布版本，不能给旧顶点贴新输入版本                  | [版本与编辑提交](M3-T1-世界模块重构.md#版本与编辑提交)、[Chunk 状态](../project/architecture/world/Chunk-类设计.md)                                                        |
| 首次、空结果与待更新 | 首次未发布不出现在访问中；已发布空网格仍提供记录，须清掉旧几何；脏块仍可提供旧发布。fringe 不发布；缺席不是流式卸载或逐帧删除通知                                           | [按块发布与视图寿命](M3-T1-世界模块重构.md#按块发布与视图寿命)、[World 接口](../project/architecture/world/World-类设计.md#公开接口预期行为)                                           |
| 借用寿命       | 整个 `WorldMeshRecord` 引用及其顶点仅在当前 callback 内有效；可复制身份/版本值，延后消费顶点须当场取得自有存储。复制 span 不延长寿命                           | [World 所有权](../project/architecture/world/World-数据设计.md#工作负载表示与所有权)、[MeshView](../project/architecture/scene/Scene-共享场景数据-数据设计.md#meshview)      |
| 调用与失败      | 夹具编辑后显式首次重建；按 x/z 串行逐块发布、首错停止，先前成功保留，失败块与未处理块可重试；不是整批事务。访问期间修改/重入被拒绝，无并发或流式保证                                  | [World 接口](../project/architecture/world/World-类设计.md#公开接口预期行为)、[失败保证](M3-T1-世界模块重构.md#状态raii与失败保证)                                              |
| 材料与纹理层     | 顶点 UV/层语义保留，定义按实际资源层数校验；纹理层索引不等于未来 GPU 句柄。T1 没有交付 `TextureArrayData` 的完整像素契约                                   | [Scene 数据](../project/architecture/scene/Scene-共享场景数据-数据设计.md#工作负载与数据语义)、[BlockDefinition](../project/architecture/world/BlockDefinition-类设计.md) |

上述内容的实际测试、失败记录及验证限制统一见 [T1 验收报告](../milestones/m3-t1/README.md#验收)，不在清单重复维护通过状态。

## T3-R1 待冻结

| 核对项 | 尚需明确的交接 | 设计入口 |
| --- | --- | --- |
| 网格适配与缓存 | 当前输入是 callback-only `MeshView`，候选 Create/Update 接收 `const MeshData&`；冻结 app 的自有化/暂存路径、身份到句柄映射和失败后版本记账。renderer 在接口返回前取得必要数据，不保存调用者地址；异步 GPU 上传不延长 CPU 借用 | [公开数据与接口](M3-T3-渲染器重构.md#公开数据与接口契约)、[所有权与同步](M3-T3-渲染器重构.md#所有权同步与失败处理) |
| 纹理数组输入 | 冻结尺寸、层数、像素格式/布局、UV 方向、颜色空间与采样语义；现有 `assets::Image` 和路径加载接口不等于候选 `TextureArrayData` | [现有 Image](../project/architecture/assets/Image-数据设计.md)、[画面一致性](M3-T3-渲染器重构.md#shader坐标与画面一致性) |
| 相机与帧输入 | 当前 `CameraView` 是 projection/view 矩阵；T3 要位姿与 FOV/near/far，须冻结中立参数、单位、视图/投影约定及调用迁移，不能直接把现有 GL 投影当跨后端契约 | [现有 CameraView](../project/architecture/scene/Scene-共享场景数据-数据设计.md#cameraview)、[相机对象](../project/architecture/simulation/Camera-类设计.md)、[画面一致性](M3-T3-渲染器重构.md#shader坐标与画面一致性) |

R1 的公开头冻结、能力/最小实验和后续 GPU 验收依 [T3 分步实施](M3-T3-渲染器重构.md#分步实施与冻结点) 执行；本次只补入口，不新增 GPU 缓存或后端实现。

2026-10-07 冻结边界补充：优先稳定游戏侧 Renderer v1，私有后端与按真实复用需求提取的内部 RHI 可演进，不冻结完整通用 RHI。已选 D3D12 先行，R1 冻结前先完成其实际纹理绘制、资源更新/删除及窗口呈现资源重建实验；Vulkan 接入前单独复核同组实验，入口见 [[M3-T3-渲染器重构#R1 关键契约实验与冻结门槛|R1 关键契约实验与冻结门槛]]。两个现代后端都完整接管后才申请 T3 验收，OpenGL 仅作过渡回归，最终退役留后续。实验不替代完整功能验收，不改变上文 T1 已交付数据语义；T3 当前仍未执行、未冻结。

下一玩法阶段的 micro voxel 只作为 NPC/玩家实体外表构建思路。基础方块与上述 CPU 网格契约继续保留，实体外表数据及 renderer 输入另定，不将 T1 的 `BlockVertex3D` 改作通用细体素角色格式。范围见 [NPC 与玩家外表设计边界](project-scope.md#npc-与玩家外表的-micro-voxel-设计边界)。
