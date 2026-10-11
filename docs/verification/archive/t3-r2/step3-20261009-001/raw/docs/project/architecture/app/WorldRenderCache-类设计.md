---
type: 类设计
status: R2顺序3已实施，最终验证以阶段报告为准
project: Symocraft
module: app
created: 2026-10-09
updated: 2026-10-09
tags:
  - area/architecture
  - topic/rendering
---

# WorldRenderCache 私有发布缓存

关联调用方：[GameRenderer](GameRenderer-类设计.md)、[App 运行编排](App-运行编排-功能.md)。契约入口：[World 数据](../world/World-数据设计.md)、[T1→T3 CPU 契约](../../../spec/M3/M3-T1-T3-CPU契约清单.md)。实施与最终成绩统一见 [R2 顺序3报告](../../../milestones/m3-t3/r2-step3-implementation.md)，本笔记不另建人工或技术验收 checklist。

## 当前设计

`SymoCraft::AppRendering::WorldRenderCache<MeshPort>` 是 app 私有模板，生产实例借用 GameRenderer 的 `ModernMeshPort`，最终调用真实 Renderer v1 Create/Update/Destroy。模板仅用于窄网格操作协作及无原生依赖的 CPU 测试；没有更改冻结的门面 API，也不使测试 fake 成为生产后端。

它按 `WorldId + ChunkCoord(x,z)` 保存持久资源标识和已成功接受的发布 `record.revision`。新记录只在版本不同或首次接受时复制/上传；`mesh_input_revision` 不参与版本记账。漏访不意味着卸载，世界访问期间提供的旧发布仍可用；释放仅由明确 ReleaseMesh/ReleaseWorld/ReleaseAll 触发。

源码：[模板](../../../../game/app/src/world_render_cache.h)、[统计边界](../../../../game/app/src/world_render_cache.cpp)、[转换声明](../../../../game/app/src/render_conversion.h)、[转换实现](../../../../game/app/src/render_conversion.cpp)。这些 cpp 属于真实 CPU 静态库 `symocraft_app_render_data`，仅依赖 Renderer API、world/assets/simulation 等 CPU 契约，不引入原生 SDK 或实验 helpers。

### 数据成员

| 类型 | 成员 | 初值 / 范围 | 所有权与含义 |
| --- | --- | --- | --- |
| `MeshPort&` | `port_` | 构造时借用 | 不拥有 port；生产 port及门面须覆盖 cache 使用和显式释放 |
| `map<MeshIdentity, Entry, MeshIdentityLess>` | `entries_` | 空 | 自有身份、句柄、accepted revision和顶点数量账本；不保存 World/Chunk 地址或 MeshView |
| `MeshCacheStatistics` | `statistics_` | 三项0 | 上次 TakeStatistics 后已成功接受的 Create/Update 调用和输入字节；失败访问前已成功的记录继续保留 |

`Entry` 是自有值：`Handle handle{}`、`World::Revision accepted_revision=0`、`size_t vertex_count=0`。Handle 必须 trivially-copyable；生产实际为 `Rendering::MeshHandle`，资源实体归 Renderer。存在的 entry 区分已接受 revision0与缺席，不能靠 revision0充当未发布哨兵。

`MeshCacheStatistics` 是 `uint64_t created_meshes/updated_meshes/upload_bytes`；`MeshIdentityLess` 按 world、chunk.x、chunk.y依次排序，不使用对象地址或散列偶然顺序。

cache 不可复制或移动。析构没有自动调用 port销毁资源，不抛出清理故障；正常生命周期由调用方显式释放，异常终端则由 Renderer 的完整资源所有权清理兜底。不能用一个已销毁的 port或门面执行释放。

### 不变量

| 条件 | 成立边界 |
| --- | --- |
| key 包含 World身份和x/z区块值；同坐标不同World不共享资源 | 首次接受至显式释放 |
| accepted revision只对应已经成功 Create/Update的实际顶点 | 每个 Accept成功返回后；不是整个访问的事务提交 |
| record及MeshView只在当前callback借用，自有化在资源调用前完成 | Accept内；span复制不会被当作寿命延长 |
| 只有变化记录复制/上传；静止版本与仅输入变脏不重复上传 | 相同发布revision返回false |
| 已发布空mesh是有效更新，不作为缺席或销毁通知 | empty Create/Update；稳定身份保留、vertex_count=0 |
| 没有以当帧visited集合删除未见entry | Synchronize与AppendDraws；卸载需显式指令 |
| 顶点world-space不额外加chunk平移；绘制model恒等 | 转换和生产AppendDraws |
| 原生在途退役/句柄失效由门面/backend处理 | app仅发Destroy；cache不是fence或GPU回收队列 |

### 方法预期行为

所有方法均在 app 串行生命周期中使用，不提供并发、重入、流式世界或失败后整会话恢复保证。

| 签名 / 入口 | 行为与输出 | 失败后状态 / 借用边界 |
| --- | --- | --- |
| `WorldRenderCache(MeshPort&)` | 保存借用，空账本和统计 | port须覆盖本对象使用；无原生创建 |
| `bool Accept(const WorldMeshRecord&)` | 相同accepted revision返回false；否则CopyRendererMesh、预检统计，再Create或Update；成功记revision/vertex_count并返回true | 复制/分配/port失败上抛；失败Create删除占位entry，失败Update不改旧accepted状态；不回滚之前其他记录 |
| `MeshCacheStatistics Synchronize(const VoxelWorld&)` | 通过VisitMeshes逐记录Accept，正常结束调用TakeStatistics返回累计成功操作 | callback失败时World访问guard恢复；未到Take的统计仍可读取/取出，先前成功记录不丢失；未见记录不删除 |
| `const Entry* Find(MeshIdentity) const noexcept` | 当前账本元素的只读借用；缺席为nullptr | 不拥有返回值；对应entry被显式删除或cache销毁后失效 |
| `size_t Size() const noexcept` | 返回已接受身份数，含空mesh | 不等同实际draw calls、world区块总数或GPU内存大小 |
| `MeshCacheStatistics PendingStatistics() const noexcept` | 返回当前统计副本，不清零 | 只计已成功接受操作 |
| `MeshCacheStatistics TakeStatistics() noexcept` | 返回统计值并将本对象三项归零 | 只取一次，不删除entry或释放资源 |
| `AppendDraws(TextureArrayHandle, vector<MeshDraw>&) const` | 逐entry追加非空mesh句柄、指定图集与identity model；不清调用方vector | 生产Handle类型使用；vector分配失败可留下已追加部分，调用方不提交失败构造的FrameInput |
| `bool ReleaseMesh(MeshIdentity)` | 缺席返回false；存在则Destroy后erase并返回true | Destroy失败保留对应entry供再次处理；不是callback缺席推断 |
| `ReleaseWorld(WorldId)` | 仅匹配world的entry逐项Destroy后erase | 逐项成功保留；中途失败时未处理与失败entry仍在，别的world不受影响 |
| `ReleaseAll()` | 逐项Destroy后erase直至空；空账本调用无作用 | 中途失败保留尚未成功释放的entry；调用方仍尝试Renderer::Shutdown |

首次Create先分配map节点，再调用port，避免账本插入失败时已经创建的资源失去记录。更新只在port成功后覆盖accepted revision/vertex_count；统计也在同一步成功后发布。Handle复制不抛出，发布之后没有新的账本分配。

`Detail::AfterMeshAcceptance` 在实际port调用前检查顶点数×28字节、调用次数与累计字节的uint64溢出，抛 `overflow_error` 时不改变资源或accepted状态。empty Update调用数加1、字节加0；不补虚构上传。当前统计是接受的CPU网格输入字节，不是DXGI allocation、GPU峰值显存、texture upload或selection staging；这些范围由GameRenderer/后端分别记录。

### CPU 转换协作者

| 函数 | 保留的实际数据语义 | 边界 |
| --- | --- | --- |
| `ToRendererCamera(const Camera&)` | 复制position/front/up getters；FOV度→弧度；near0.1、far2000；右手方向不变 | Camera/Registry调用期有效；不把旧GL矩阵作为跨后端矩阵，不替代门面参数验证 |
| `ToRendererAtlas(const Assets::Image&)` | 固定512×512、64×64 tile、64层；源为top-left，层号底行先行，每层top-left；RGB补alpha255，RGBA保留；Linear/RGBA8 | 要求3或4通道及精确字节长度；非法维度/通道/长度invalid_argument；返回自有像素，不改源 |
| `CopyRendererMesh(MeshView)` | 自有复制vertices；仅 `tex_coord.y=1-y` 一次，保留world位置、顺序、U、浮点层与normal；empty合法 | 必须在callback有效期内调用；不clamp repeat UV、不禁止混层三角形，不替代Renderer网格校验 |

atlas方向依据真实旧 `texture.cpp`：DecodeImage(true)后以GL底端为起点切层。现代保留底行优先层号，但层内存储转为top-left，故World UV在app复制时作单次V转换。生产blocks shader直接使用转换后的tex_coord，不再flip。CPU字节/数学对照不是完整生产GPU画面验收。

## 验证边界

聚焦测试位于 [cache测试](../../../../test/unit/app/render_cache_tests.cpp) 和 [conversion测试](../../../../test/unit/app/render_conversion_tests.cpp)：CPU窄port验证版本接受/失败重试/空更新/omission/世界分离/显式释放/溢出，真实World小夹具核对发布与digest/源寿命；真实资产对照全部64层RGBA字节，实际Camera对照getter/角度/nearfar/RH投影。小夹具与fake不承担原生同步、完整世界性能或GPU接管证明。

本轮CPU与原生Debug/Release已构建，最终分配置测试和真实运行成绩仅以 [阶段报告](../../../milestones/m3-t3/r2-step3-implementation.md) 为准。生产包顺序4、R3的完整玩法与至少15分钟操作、实际设备丢失及真正在途故障仍待独立验收，不切默认、不删除GL，也不复制另一套人工核查清单。
