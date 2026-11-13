---
tags:
  - area/architecture
---

# App 组合与运行

> 已验收候选及保留限制见 [change 验证记录](../openspec/changes/integrate-renderdoc-tracy/verification.md)。构建和运行命令以仓库根目录为准。

2026-10-19当前源码状态：用户在T4验收、规格同步及归档后明确授权同步生产代码，F主工作区feature-refactoring现已取得E目录T4实现及必要构建、测试配套。同步以已验收候选为源，保留主目录独立修改及旧构建，不执行Git提交/合并；下文2026-10-14/17等旧状态保留原时点。当前Application/SimulationSession与内部physics均已在主工作区，实际计算服务与T5仍未实施。既有候选批准及风险见[T4记录](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/verification.md#2026-10-19-t4整体完成批准)。

`game/app` 是私有组合根，不是下层模块的服务定位器。公开行为引用 [module-boundaries](../openspec/specs/module-boundaries/spec.md)、[platform](../openspec/specs/platform/spec.md)、[world](../openspec/specs/world/spec.md)、[simulation](../openspec/specs/simulation/spec.md) 和 [renderer](../openspec/specs/renderer/spec.md)。

2026-10-25 T5 更新：用户在 T5/M3 验收及归档后独立授权同步，F 已取得 E 的 EnTT 调用迁移；上段“T5 未实施”保留 T4 同步时点，不再代表当前源码。`application.cpp` 注册 PhysicsStepOrder 等六类组件，Session 创建玩家后用 Each<const PlayerComponent> 唯一查找并保留项目 EntityId，初态从装配后的组件读取，不以空 Advance 取初态。Scenario 的模拟前姿态覆盖仍在稳定结构阶段按 ID 重取 Transform，未新增相机 owner、原生存储逃生口或改变输入/时间/普通编辑时点。

T5 的 Registry 仍先于借用 Session 存活且后于它清理；ECS 结构阶段后即重取借用，不以失败保值恢复旧地址。现有逐实例装配/Shutdown、首错与存储寿命保持；旧性能封闭导出已随工具迁移退役，相关机制分别见 [ecs](ecs.md#t5-实现机制) / [simulation](simulation.md#t5-调用适配)。新候选集成与三路径用户反馈只在 [T5 verification](../openspec/changes/archive/2026-10-25-m3-t5-ecs-memory/verification.md) 记录，不继承 T4 性能或自动形成 T5/M3 批准。
以下T4机制已落实于[Application](../../game/app/src/application.cpp)，该实现已按后续明确授权同步到F主工作区。`symocraft_application`正式静态库唯一拥有Application/GameRenderer/RunGame实现，`symocraft_app_runtime`仅为旧消费者兼容target别名，exe仅编薄main；T4于2026-10-19获用户整体批准并保留风险，节点与实际结果分别从[VERIFICATION](../verification/VERIFICATION.md)和change verification判读，本次源码同步不产生Git历史合并。

## 所有权与寿命

共享RunGame入口拥有不可复制/移动的私有Application实例。Impl复制checkpoint/renderer/profile_scenario三个CLI文本值并重新绑定StartupOptions views，拥有Runtime、Window、GameRenderer、World、专属Registry、SimulationSession、可选Scenario、DebugObserver及复用输入缓冲。没有Performance::Session、输出目录或Camera实体/静态玩法owner。
World/Registry覆盖会话，Window覆盖GameRenderer，Runtime覆盖Window；Session只借存储，不在析构删除玩家。玩家ID仅供app场景边界覆盖姿态，运动长期权威仍是组件；镜头、预算/选材/冷却属于Session。Q01失败整体丢弃专属装配，不保证共享Registry精确回滚。
定义在CPU预检阶段取得、atlas层验证后移动进同一VoxelWorld。PreparedWorld只交接World与自有Pose/报告，Pose名字借静态字符串。夹具、物理、射线、编辑、摘要与网格都消费同一实例。

Initialize一次装配：可选Scenario→CPU定义→Runtime→指定模式Window→Renderer/atlas→World→Registry注册→Session玩家→首网格/上传；每个对象取得后立即进入成员owner。失败后不可Run或重试装配，由共享入口显式Shutdown，部分装配首诊断可保留，不宣称全字段事务。PrintWorldSummary仍不创建窗口、Renderer或CPU网格。
Run仅在完整装配成功后一次执行；装配、模拟、缓存和关闭串行、非重入、创建线程调用。独立CPU Session可交错验证隔离，不推出多窗口、共享Registry或后台GPU调度保证。

## 帧策略与提交顺序

加载/首网格后才开启帧时钟。顺序为Poll/关闭/实际像素→Capture及暂停策略→普通意图或Scenario→Session一次Advance（输入/Character/固定步/恢复/交互结果）→普通World提交→mesh/cache/render→观测。所有输入消费者完成后Clear复用缓冲，不借用Window内部帧。
普通有序指针逐事件应用，键盘Down纯读held OR pressed，不重复填指针汇总。独立BenchmarkScenario由显式profile_scenario配置选择，不依赖工具或DebugObserver；static/walk/edit可在无记录的CPU调用方使用。普通编辑在Advance后、场景edit在前，预条件错误抛出，不悄悄跳过。
普通无限帧玩法失焦、最小化或零 drawable 暂停：清旧输入/运动/待跳、水平速度及物理余数，WaitEvents 有限等待；恢复首帧规则 dt=0。同轮失焦再获焦也处理清理事实。普通有限帧诊断只豁免失焦暂停，不豁免零尺寸/最小化绘制。
Session先全量预检double时间/有序指针再开始写入，规则float时间最多0.1秒，独立预算每轮最多8/120秒；预检Rejected不改状态，推进后故障转Failed并保留此前提交。低于y=-8恢复World出生点、清力/接地/跳跃/预算。持有组件引用期间禁止结构修改；最小ECS修补不替代[T5边界](ecs.md)。
独立场景用实际经过时间调度原 static/walk/edit；walk→spawn、static/edit→four-chunk，初始FOV45度。玩家键鼠不改变场景，本窗口Escape/关闭取消为0并明确诊断；时限完成另记completed。失焦允许继续，实际尺寸偏离或最小化为运行失败3，不等待恢复或生成有效成绩。Tracy分别观测event/input/scenario等实际CPU区域，不恢复旧event_ms/simulation_ms输出字段。

## 两条渲染路径

GameRenderer连接默认D3D12/Native与显式Vulkan/Vulkan Window，保留显式legacy OpenGL；没有热切换或失败fallback。现代分别使用DXIL/SPIR-V及实际shader生成身份。它独占Renderer、port/cache、图集句柄和draws；port只借门面且覆盖cache使用/释放，不持有旧timer或性能Session。截图由Render返回的自有FrameResult直接取得，不需要采样/最终导出。
Vulkan精确surface_extent_pending在旧尺寸资源退休前保留接受状态，app有界等待一次再在后续poll重试；ZeroExtent清本地extent缓存，恢复后重新Resize。仅识别该具名错误，不吞其他InvalidState。
现代每帧只调用一次 Renderer::Render，由其唯一 Present；app 不额外交换 Native 缓冲。旧 GL 继续每帧 VisitMeshes→AppendMesh→Render→独立 Present，不继承现代持久缓存和 Failed/Stopped 强保证。
现代Resize使用Window真实drawable extent，最小化传零，成功后更新缓存。app从同一身体姿态及自有Lens派生中立PlayerView；现代转换度到弧度、near=0.1/far=2000，GL按实际aspect使用RH_NO投影，不复制GL矩阵到现代接口；零像素拒绝派生/绘制。
现代真实图集必须 512x512、64 个 64x64 tile，底行优先层号、层内 top-left RGBA8、RGB 补 alpha=255、Linear。World 顶点已是 world-space，model 恒等；自有化时只作一次 V=1-V，shader 不再翻转，不裁剪 repeat UV 或混层三角形。
资源预检按选中路径检查：legacy 必需项只要求 regular_file，D3D12/Vulkan 必需项还检查非零大小；不初始化 GPU、不证明解码或 shader/PSO 正确。新 RenderError 保类别/后端/操作与原生诊断，具名向上退出，不转普通 World::Rejected。

## 发布缓存与失效

`WorldRenderCache<MeshPort>` 是 app 私有不可复制/移动模板，不是 renderer 后端或 fence 队列。账本只保存 WorldId+ChunkCoord、句柄、成功接受的发布 revision 与顶点数，不保存 World/Chunk 指针、span 或完整世界快照。
callback 内先复制网格并预检统计溢出；首次 Create 先分配 map 节点，接受失败删占位，避免资源先创建而账本分配失败。Update 成功后才推进 accepted revision/顶点数/统计；失败保旧状态。使用发布 revision 而非 mesh-input revision，仍 pending 的旧发布不重复上传，空发布有效清旧几何。
同步是逐记录接受，不是整批事务。前序成功保留；异常退出 World guard 恢复，未到 TakeStatistics 的成功统计仍保留。draws 重组或 AppendDraws 分配失败可能留下部分 vector，app 退出，不提交半轮 FrameInput。
缺席记录不是卸载；只用显式 ReleaseMesh/World/All。Destroy 成功才 erase，失败保对应及未处理 entry；正常 Run 在 World 尚有效时显式释放该身份。析构不调用借用 port；异常终端依门面完整资源所有权清理，不能用已销毁 port 释放。
Find 返回账本元素只读借用，在对应 entry erase/cache 销毁后失效；TakeStatistics 只归零统计。计数为成功接受的 CPU mesh 字节/调用，empty Update 计一次且零字节，不是显存分配/峰值。资源上传与 Render-local staging 分开记一次。

## 关闭与失败后状态

Run结束只报告实际场景完成/取消和已呈现帧数，不自动追加旧性能截图或导出。显式截图诊断由其调用方请求、保存并处理PNG错误，不以旧文件存在当成功。
Shutdown先停Scenario/新模拟→Bindings/texture/Renderer→SimulationSession/World/Registry→Renderer对象→Window→Runtime。固定数组ShutdownReport逐项保存NotAcquired/Released/Failed/Unavailable及原exception_ptr，继续全部适用清理；RunGame保留运行首因并记录关闭次错。
GameRenderer先置stopped，仍允许资源操作时释放cache/图集，再执行正式Renderer Shutdown；失败后不续交普通资源调用。现代未知在途资源仍由完整后端终止保留，不将其写为安全释放。GL只清理其texture/program/buffer，不创建timer/query。
重复Shutdown交接首次缓存事实，不重做清理或把失败改绿；析构noexcept只作有界兜底。main转交同一RunGame，保持解析64、资产2、运行/关闭3、正常/取消0，不使用旧退出4或Seal/Export。未修复1175/Fix1，任意OOM下不保证完整文字。

## 稳定不变量与验证

保留原Application I1-I6局部身份：I1存储覆盖借用会话（旧Camera仅兼容测试）；I2图形先于Window/context；I3 modules不包含app或查静态owner；I4 profiling场景不提交普通意图；I5生产cpp唯一owner；I6同一局部World。与renderer/T4同号身份不混淆。
缓存和 GameRenderer 原无编号约束由上述所有权、成功接受/显式释放、单次 Present、复制与转换章节维护，不新造历史验收身份。
依据：[Application](../../game/app/src/application.cpp)、[薄main](../../game/app/src/main.cpp)、[共享入口](../../game/app/src/game_entry.cpp)、[GameRenderer](../../game/app/src/game_renderer.cpp)、[cache](../../game/app/src/world_render_cache.h)、[统计](../../game/app/src/world_render_cache.cpp)、[转换](../../game/app/src/render_conversion.cpp)、[参数](../../game/app/src/startup_options.h)、[cleanup](../../game/app/src/cleanup_sequence.h)。
聚焦测试保cache/conversion/参数、正式Scenario/关闭报告及真实Application装配探针。T3/R4已批准，T4于2026-10-19独立获整体批准并归档；整合后的实际构建/CPU/SDL/真实后端结果只在[T4 verification](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/verification.md#2026-10-17-t3批准基线整合)，不由架构文字推定人工体验、长期资源或整体验收。

本轮场景、调试快照与旧链退役的实际检查见 [integrate-renderdoc-tracy verification](../openspec/changes/integrate-renderdoc-tracy/verification.md)，不将历史批准继承为新候选通过。
