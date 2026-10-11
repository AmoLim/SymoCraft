---
type: 契约索引
status: T1 已交付，T3-R1 已冻结，R2 已获准进入
project: Symocraft
module: M3-T1-T3
created: 2026-10-06
tags:
  - area/spec
---

> 2026-10-11 旧导航引用已失效，已改指现行权威或明确归档入口；历史命令、结果和批准含义保持不变。

# T3 CPU 契约清单

> 2026-10-10 OpenSpec 迁移后已退役：本页仅保留原文供复核，元数据、待办及“当前”等文字均属原记录时点，不是现役规格、规则或活动计划。CPU消费者交接与现役world/renderer基线；旧待办不描述当前R2候选状态。现役入口见 [新权威位置](../../openspec/changes/m3-t3-renderer/design.md)；来源映射见 [迁移 design](../../openspec/changes/docs-openspec-migration/design.md#来源映射)。旧源未删除，清场须另行确认。

用途：T1 → T3 的交接核对入口，不另定义一份数据契约。下表链接当前实现的权威笔记；T3 的 Renderer v1 与 target 设计基线以 [T3 Spec](M3-T3-渲染器重构.md) 和 [R1 冻结快照](../../milestones/m3-t3/evidence/r1/freeze-20261009-001/README.md) 为准，清单存在不表示 T3 生产接管已交付或 T1 节点已批准。

2026-10-09 用户明确批准：“我已经审查了R1了，可以冻结R1，批准进入R2”。R1 / Renderer v1 和当前 target 设计基线现已冻结，批准与来源身份见 [冻结清单](../../milestones/m3-t3/evidence/r1/freeze-20261009-001/freeze-manifest.json)；下一步按 [R2 实施入口](../../milestones/m3-t3/r2-entry.md) 落实真实生产门面与 D3D12 接管。节点批准不是新实测，不将实验 helpers 升格为生产 API 承诺；生产采样/颜色、混层行为、相机 GPU 画面及生产 target 边界仍须在 R2 实施复核，不得静默收窄既定契约。

前置平台入口：[T2 SDL3 迁移正式计划](M3-T2-SDL3迁移.md#已确定的决策)。已选 D03/A，T2 须准备 GL、无 GL 原生窗口和 Vulkan 三种模式，并完成真实 GL 呈现、HWND、Vulkan instance/surface 与 loader 清理验证；T2 已于 2026-10-09 获用户正式节点批准，已验证范围和遗留风险以 [T2 记录](../../milestones/m3-t2/README.md) 为准。用户已确认先迁移再冻结渲染器：T3-R1 核对窗口/像素/私有桥接证据，不将探针视为 T3 现代后端的完整玩法接管验收，也不修改下表已交付 CPU 数据。

## T1 已交付

| 交接项        | T3 消费时必须保留的边界                                                                                                  | 权威入口                                                                                                                                             |
| ---------- | -------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------ |
| CPU / 构建边界 | scene 是 CPU-only INTERFACE；world 不依赖 renderer/GPU。app 转接 World 记录，renderer 不接收 World/Chunk 私有对象                | [依赖图](../../architecture/Overview.md)、[公开头清单](../../milestones/m3-t1/evidence/public-headers.tsv)                                         |
| 顶点与网格      | `MeshData` 自有顶点，`MeshView` 只读借用；28 字节 `BlockVertex3D`、三角形列表、每面六顶点及原展开顺序保留。后端自行核对 GPU 布局，不能将 CPU 布局视作通用 GPU ABI | [Scene 数据](../../architecture/scene.md)、[Mesher](../../architecture/world.md)                        |
| 稳定身份       | `MeshIdentity = WorldId + ChunkCoord`；ChunkCoord 的 x/y 表示世界 x/z。重建不改变身份，不用地址作缓存 key，不将进程内身份当持久化 ID             | [World 数据语义](../../architecture/world.md)                                                                                   |
| 发布版本       | `record.revision` 对应当前顶点；`mesh_input_revision` 可更新而旧网格仍有效。T3 只在资源更新成功后记录对应的已发布版本，不能给旧顶点贴新输入版本                  | [版本与编辑提交](M3-T1-世界模块重构.md#版本与编辑提交)、[Chunk 状态](../../architecture/world.md)                                                        |
| 首次、空结果与待更新 | 首次未发布不出现在访问中；已发布空网格仍提供记录，须清掉旧几何；脏块仍可提供旧发布。fringe 不发布；缺席不是流式卸载或逐帧删除通知                                           | [按块发布与视图寿命](M3-T1-世界模块重构.md#按块发布与视图寿命)、[World 接口](../../architecture/world.md)                                           |
| 借用寿命       | 整个 `WorldMeshRecord` 引用及其顶点仅在当前 callback 内有效；可复制身份/版本值，延后消费顶点须当场取得自有存储。复制 span 不延长寿命                           | [World 所有权](../../architecture/world.md)、[MeshView](../../architecture/scene.md)      |
| 调用与失败      | 夹具编辑后显式首次重建；按 x/z 串行逐块发布、首错停止，先前成功保留，失败块与未处理块可重试；不是整批事务。访问期间修改/重入被拒绝，无并发或流式保证                                  | [World 接口](../../architecture/world.md)、[失败保证](M3-T1-世界模块重构.md#状态raii与失败保证)                                              |
| 材料与纹理层     | 顶点 UV/层语义保留，定义按实际资源层数校验；纹理层索引不等于未来 GPU 句柄。T1 没有交付 `TextureArrayData` 的完整像素契约                                   | [Scene 数据](../../architecture/scene.md)、[BlockDefinition](../../architecture/world.md) |

上述内容的实际测试、失败记录及验证限制统一见 [T1 验收报告](../../milestones/m3-t1/README.md#验收)，不在清单重复维护通过状态。

## T3-R1 冻结基线与 R2 转接

| 核对项 | R2 落实与复核的冻结边界 | 设计入口 |
| --- | --- | --- |
| 网格适配与缓存 | 当前输入是 callback-only `MeshView`，Create/Update 设计接收 `const MeshData&`；落实 app 的自有化/暂存、身份到句柄映射和失败后版本记账。renderer 在接口返回前取得必要数据，不保存调用者地址；异步 GPU 上传不延长 CPU 借用 | [公开数据与接口](M3-T3-渲染器重构.md#公开数据与接口契约)、[所有权与同步](M3-T3-渲染器重构.md#所有权同步与失败处理) |
| 纹理数组输入 | 按冻结设计落实尺寸、层数、像素格式/布局、UV 方向与显式颜色空间，生产采样和混层行为在 R2 复核；现有 `assets::Image` 和路径加载接口不等于 `TextureArrayData`，夹具设置不替代生产画面验收 | [现有 Image](../../architecture/assets.md)、[画面一致性](M3-T3-渲染器重构.md#shader坐标与画面一致性) |
| 相机与帧输入 | 当前 `CameraView` 是 projection/view 矩阵；按冻结设计落实位姿与 FOV/near/far、中立单位、视图/投影约定及调用迁移，不能直接把现有 GL 投影当跨后端契约。CPU 数值对照不替代生产 GPU 画面复核 | [现有 CameraView](../../architecture/scene.md)、[相机对象](../../architecture/simulation.md)、[画面一致性](M3-T3-渲染器重构.md#shader坐标与画面一致性) |

R1 的设计基线冻结与后续 GPU 验收依 [T3 分步实施](M3-T3-渲染器重构.md#分步实施与冻结点) 执行。后续 [R2 顺序1](../../milestones/m3-t3/r2-step1-implementation.md) 已落实生产公开头、CPU 门面/句柄与实际 CPU target 边界并留证；GPU 缓存、原生后端、生产采样和统一 World 调用迁移仍待实施，不将本步 CPU 结果扩为生产 GPU 通过。

2026-10-08 R1 准备补充（当时状态）：[独立 CPU 转接候选](../../milestones/m3-t3/README.md#r1-转接准备与双配置复核) 已消费真实资产、Camera 与 World target，Debug/Release 各 4/4；callback 内复制网格，按 World/Chunk 身份映射，只在更新成功后记 `record.revision`，原 world-space 顶点不再加 chunk 平移。实际 atlas 64 层的底行优先编号保持；候选每层 top-left 数据与旧 `v=1` 顶端需统一采样转换，生产 GPU 方向/颜色/采样尚未验收。相机度→弧度、near/far 0.1/2000 与 RH_NO/RH_ZO 有 CPU 数值对照，但不是 GPU 画面迁移通过。新增声明消费者无 SDK 传播，当时生产公开契约尚未冻结；当前节点批准以上文 2026-10-09 记录为准，历史成绩和验证边界不改写。

2026-10-07 冻结边界补充（当时设计）：优先稳定游戏侧 Renderer v1，私有后端与按真实复用需求提取的内部 RHI 可演进，不冻结完整通用 RHI。已选 D3D12 先行，R1 冻结前先完成其实际纹理绘制、资源更新/删除及窗口呈现资源重建实验；Vulkan 接入前单独复核同组实验，入口见 [[spec/M3/M3-T3-渲染器重构#R1 关键契约实验与冻结门槛|R1 关键契约实验与冻结门槛]]。两个现代后端都完整接管后才申请 T3 验收，OpenGL 仅作过渡回归，最终退役留后续。实验不替代完整功能验收，不改变上文 T1 已交付数据语义；当时 T3 生产接管未实施、v1 未冻结。后续 R1 成绩及 2026-10-09 冻结批准另见上文，当前生产接管仍未交付。

下一玩法阶段的 micro voxel 只作为 NPC/玩家实体外表构建思路。基础方块与上述 CPU 网格契约继续保留，实体外表数据及 renderer 输入另定，不将 T1 的 `BlockVertex3D` 改作通用细体素角色格式。范围见 [NPC 与玩家外表设计边界](../../scope/project-scope.md#npc-与玩家外表的-micro-voxel-设计边界)。
