---
tags:
  - area/architecture
---

# App 组合与运行

2026-10-19当前源码状态：用户在T4验收、规格同步及归档后明确授权同步生产代码，F主工作区feature-refactoring现已取得E目录T4实现及必要构建、测试配套。同步以已验收候选为源，保留主目录独立修改及旧构建，不执行Git提交/合并；下文2026-10-14/17等旧状态保留原时点。当前Application/SimulationSession与内部physics均已在主工作区，实际计算服务与T5仍未实施。既有候选批准及风险见[T4记录](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/verification.md#2026-10-19-t4整体完成批准)。

`game/app` 是私有组合根，不是下层模块的服务定位器。公开行为引用 [module-boundaries](../openspec/specs/module-boundaries/spec.md)、[platform](../openspec/specs/platform/spec.md)、[world](../openspec/specs/world/spec.md)、[simulation](../openspec/specs/simulation/spec.md) 和 [renderer](../openspec/specs/renderer/spec.md)。
以下T4机制已落实于[Application](../../game/app/src/application.cpp)，该实现已按后续明确授权同步到F主工作区。`symocraft_application`正式静态库唯一拥有Application/GameRenderer/RunGame实现，`symocraft_app_runtime`仅为旧消费者兼容target别名，exe仅编薄main；T4于2026-10-19获用户整体批准并保留风险，节点与实际结果分别从[VERIFICATION](../verification/VERIFICATION.md)和change verification判读，本次源码同步不产生Git历史合并。

## 所有权与寿命

共享RunGame入口拥有不可复制/移动的私有Application实例。Impl复制五个CLI文本值并重新绑定StartupOptions views，拥有可选Performance::Session、Runtime、Window、GameRenderer、World、专属Registry、SimulationSession、可选Scenario和复用输入缓冲。Performance记录早于装配、保留到显式关闭后Seal/Export；无Camera实体或静态玩法owner。
World/Registry覆盖会话，Window覆盖GameRenderer，Runtime覆盖Window；Session只借存储，不在析构删除玩家。玩家ID仅供app场景边界覆盖姿态，运动长期权威仍是组件；镜头、预算/选材/冷却属于Session。Q01失败整体丢弃专属装配，不保证共享Registry精确回滚。
定义在CPU预检阶段取得、atlas层验证后移动进同一VoxelWorld。PreparedWorld只交接World与自有Pose/报告，Pose名字借静态字符串。夹具、物理、射线、编辑、摘要与网格都消费同一实例。

Initialize一次装配：可选记录/Scenario→CPU定义→Runtime→指定模式Window→Renderer/atlas→World→Registry注册→Session玩家→首网格/上传；每个对象取得后立即进入成员owner。失败后不可Run或重试装配，由共享入口显式Shutdown，部分启动metadata可保留，不宣称全字段事务。PrintWorldSummary仍不创建窗口、Renderer或CPU网格。
Run仅在完整装配成功后一次执行；装配、模拟、缓存和关闭串行、非重入、创建线程调用。独立CPU Session可交错验证隔离，不推出多窗口、共享Registry或后台GPU调度保证。

## 帧策略与提交顺序

加载/首网格后才开启帧时钟。顺序为Poll/关闭/实际像素→Capture及暂停策略→普通意图或Scenario→Session一次Advance（输入/Character/固定步/恢复/交互结果）→普通World提交→mesh/cache/render→观测。所有输入消费者完成后Clear复用缓冲，不借用Window内部帧。
显式strict前台准备沿T3既有协议：连续1秒获焦且实际尺寸可绘制后才开始完整预热；有界等待不Advance/Render，输入自有交接后逐轮Clear，成功Reset输入、Resume本会话并重设时钟，准备时间单列startup。超时/Esc/关闭保具名失败并沿同一Shutdown/Seal/Export；awaiting-foreground仅Recording可发布，不能借状态入口提前宣布exporting/finished。
普通有序指针逐事件应用，键盘Down纯读held OR pressed，不重复填指针汇总。独立BenchmarkScenario由显式benchmark配置选择，不由Performance指针存在性驱动；static/walk/edit可在无记录的CPU调用方使用。普通编辑在Advance后、场景edit在前，预条件错误抛出，不悄悄跳过。
普通无限帧玩法失焦、最小化或零 drawable 暂停：清旧输入/运动/待跳、水平速度及物理余数，WaitEvents 有限等待；恢复首帧规则 dt=0。同轮失焦再获焦也处理清理事实。普通有限帧诊断只豁免失焦暂停，不豁免零尺寸/最小化绘制。
Session先全量预检double时间/有序指针再开始写入，规则float时间最多0.1秒，独立预算每轮最多8/120秒；预检Rejected不改状态，推进后故障转Failed并保留此前提交。低于y=-8恢复World出生点、清力/接地/跳跃/预算。持有组件引用期间禁止结构修改；最小ECS修补不替代[T5边界](ecs.md)。
benchmark 使用实际经过时间调度原 static/walk/edit，不被玩家键鼠修改；本窗口 Escape/关闭仍能取消。实际尺寸改变/最小化使记录无效，allow-unfocused 不豁免。Poll 计入 event_ms，Capture 在 simulation_ms 中，不挪计时点制造收益。

## 两条渲染路径

2026-10-14审核同步：E盘refactor-renderer的[Vulkan候选GameRenderer](E:/Codex/Worktrees/refactor-renderer/Symocraft/game/app/src/game_renderer.cpp)已将显式Vulkan/Vulkan Window连接同一公开v1、持久缓存、atlas/相机/selection与循环。选中现代路径分别预检DXIL/SPIR-V，telemetry使用实际后端名；后端先观察到ZeroExtent时清本地extent缓存，防恢复旧尺寸却跳过Resize。F盘源码不因本文同步复制或变为已交付Vulkan，现役默认仍D3D12、GL保留；机制及实际范围见[renderer候选机制](renderer.md#vulkan工作目录候选机制)和[T3验证](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#2026-10-14-r4生产vulkan实现)，不是T4迁移或R4批准。

当前T4 E候选的GameRenderer已连接默认D3D12/Native与显式Vulkan/Vulkan Window，保留显式legacy OpenGL；没有热切换或失败fallback。现代分别使用DXIL/SPIR-V及实际生成身份，Vulkan精确surface_extent_pending重试与ZeroExtent缓存失效沿上游，受控Window实际尺寸读取替代旧public字段；旧两路径成绩不代替整合后组合验证。
GameRenderer 独占现代 Renderer、port/cache、图集句柄、draws 或旧 timer；port 只借门面且其寿命覆盖 cache 使用/释放，World/PlayerView/观测记录仅同步借用。普通无记录现代帧仍非阻塞清空有界 GPU 结果队列。
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

Run 正常结束先非阻塞交接已就绪 GPU 结果、记录最终摘要；completed 时才在正式采样外作独立额外截图帧，排除其 GPU frame ID。PNG 写入错误传播，不能把旧文件存在当成功。该结束观测不是额外 GPU 等待/Seal 保证。
Shutdown先停Scenario/新模拟→仍合法时交接已就绪GPU→Bindings/texture/Renderer→Session/World/Registry→Renderer对象→Window→Runtime。固定数组ShutdownReport逐项保NotAcquired/Released/Failed/Unavailable及原exception_ptr，继续所有适用清理；正常return与自有ShutdownError是两个出口，RunGame在catch内复制完整报告后统一FinalFacts。错误保持原类别/原生码，不用文字取代。
GameRenderer::Shutdown 先置 stopped，再尝试 ReleaseAll、图集 Destroy 和现代 Shutdown，清 draws/cache并 reset门面；旧路径释放 timer和旧Renderer。首次清理失败后重复调用不重做原步骤，不代表成功/可恢复；终止 quarantine 与安全等待由 [renderer](renderer.md) 保证，app 不强行释放未知在途资源。析构仅诊断兜底，不能替代显式成功关闭。
重复Shutdown返回或抛出首次缓存事实，不重做资源操作，也不把首次失败改绿；析构只作有界兜底，不Seal/Export。GameRenderer对终止状态禁止额外Poll/Destroy普通资源API，交给正式Renderer Shutdown/quarantine；GPU观测缺失不伪装安全释放。
main仅转交同一生产RunGame；入口保解析64/缺资产2/运行关闭导出错误3/已保存无效4/正常0。先完成Window/Runtime关闭，再汇合首错/次错/completed并Seal，显式Export检查自有receipt；Saved重复不写盘，失败仅显式调用重试。std/非std运行及最终化错误皆有退出路径，但不承诺任意OOM时完整诊断文字或目录事务，也未修复1175/Fix1。

## 稳定不变量与验证

保留原Application I1-I6局部身份：I1存储覆盖借用会话（旧Camera仅兼容测试）；I2图形先于Window/context；I3 modules不包含app或查静态owner；I4 benchmark不提交普通意图；I5生产cpp唯一owner；I6同一局部World。与renderer/T4同号身份不混淆。
缓存和 GameRenderer 原无编号约束由上述所有权、成功接受/显式释放、单次 Present、复制与转换章节维护，不新造历史验收身份。
依据：[Application](../../game/app/src/application.cpp)、[薄main](../../game/app/src/main.cpp)、[T4共享入口](E:/Codex/Worktrees/refactor-application-simulation/Symocraft/game/app/src/game_entry.cpp)、[GameRenderer](../../game/app/src/game_renderer.cpp)、[cache](../../game/app/src/world_render_cache.h)、[统计](../../game/app/src/world_render_cache.cpp)、[转换](../../game/app/src/render_conversion.cpp)、[参数](../../game/app/src/startup_options.h)、[cleanup](../../game/app/src/cleanup_sequence.h)。
聚焦测试保cache/conversion/参数、正式Scenario/关闭报告及真实Application装配探针。T3/R4已批准，T4于2026-10-19独立获整体批准并归档；整合后的实际构建/CPU/SDL/真实后端结果只在[T4 verification](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/verification.md#2026-10-17-t3批准基线整合)，不由架构文字推定人工体验、长期资源或整体验收。
