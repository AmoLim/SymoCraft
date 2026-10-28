---
tags:
  - area/spec
---

# RenderDoc、Tracy 与轻量调试观测

## Why

M3/T5 已验收，接受的 EnTT、SimulationSession 与 DBG01 配套已同步主工作区。后续性能诊断改用成熟工具，避免继续维护自有逐帧采集、统计、文件协议与 benchmark 产品。当前源码仍由 [Application](../../../../game/app/src/application.cpp)、[GameRenderer](../../../../game/app/src/game_renderer.cpp) 和 [telemetry](../../../../game/modules/telemetry/include/symocraft/telemetry/performance.h) 联合维护这条链，普通游戏路径也仍有采样相关成本。

本 change 落实聊天中已选 B（仅 P1–P3）及 A（旧 benchmark 同步退役）：RenderDoc 负责帧内图形诊断，Tracy 负责 CPU 时间线与计数，telemetry 只保留必要调试数值。以[当前范围](../../../scope/project-scope.md)、[模块边界](../../specs/module-boundaries/spec.md)、[renderer](../../specs/renderer/spec.md) 和 [telemetry 生命周期](../../specs/telemetry-lifecycle/spec.md)为依据；目标不是已实现行为，也不是新的实施或 Git 授权。

## What Changes

| 范围 | 本轮交付 |
| --- | --- |
| P1 RenderDoc | Windows x64 外部 MSI/ZIP 工具启动游戏；D3D12、Vulkan 分别完成真实帧捕获与回放。提供可复现的启动、工作目录、资源、场景和捕获操作说明 |
| P2 Tracy CPU | 固定本地官方源码、唯一静态 client、默认关闭的构建开关；帧标记、主要 CPU 阶段、按帧计数与会话身份；Viewer 与 client 匹配 |
| P3 图形可读性 | 两现代后端原生资源命名与成对事件分组，捕获中可定位 world、selection、upload、resolve/readback 等实际存在的阶段；不虚构 GPU 时间线 |
| 轻量 telemetry | FPS、帧间隔、实际物理 TPS、玩家坐标、当前后端的自有只读快照，常量级聚合存储；不再承担性能分析或文件导出 |
| 旧链退役 | 移除活动 `SymoCraftBenchmark` GUI/CLI、Session/Seal/Export、逐帧历史/分位数、周期进程/GPU 内存采集、CSV/YAML 性能协议及其专属构建/安装/脚本/测试入口 |
| 场景保留 | static/walk/edit 从旧采集入口解耦，直接运行游戏，不需要 Session、Viewer、RenderDoc 或输出目录；动作、固定世界、真实经过时间及编辑时点不漂移 |

**兼容性变化：** 新入口拟为 `--profile-scenario static|walk|edit`，配套有界 `--scenario-seconds 1..1200`，默认 60 秒，沿用 seed 424242、既有 regression/workload 身份、1920×1080、4x MSAA、VSync-off 请求与 allow-unfocused 的场景条件。实际能力仍核验，不静默降级；不是新正式采样协议。保留现有 renderer/seed/尺寸/MSAA/validation/VSync 参数供场景配置；旧 `--benchmark`、`--output`、warmup/sample、focus-policy/foreground-ready 等旧链参数明确拒绝并提示替代，不保留伪成功兼容壳。普通玩法、资源预检、world-summary 与有限帧诊断不改义。

场景初态保留[现有映射](../../../../game/app/src/startup_options.cpp)：`walk → spawn`，`static/edit → four-chunk`；位置、yaw/pitch 使用现有 [TestScene::Checkpoint](../../../../game/modules/world/src/test_scene.cpp) 定义，初始 FOV 为 45 度，不将三个场景统一套用默认 spawn。解析映射及装配后的首次 Advance 前位姿/镜头均纳入回归。

FPS 用实际 Presented 帧数/单调现实秒，帧间隔用相邻成功呈现返回时刻；TPS 用同窗口实际完成物理子步数/现实秒，不用 Advance 次数或设定 120 Hz。暂停/跳过不沿用旧速率，恢复重新建立窗口；尚无有效间隔时明确不可用，不产生 NaN/Inf。只累计计数/时长及最后值，不保存逐帧序列。app 读取已有推进结果与位姿，不为显示另做 World/ECS 遍历。本轮交付数值契约及读取接线，不引入新 HUD、ImGui 或调试面板；显示载体另行确认。

RenderDoc 不进入源码依赖、链接或产品部署，不接 `renderdoc_app.h`、自动注入/全局 hook 或游戏内捕获按钮。Tracy GPU context/query/calibration、分配器全局挂钩、逐实体/逐体素埋点、完整内存分析和新 benchmark 自动化平台不在本轮。详细进程内存/显存分析不因工具替换而冒称已有等价覆盖。

不新增 NPC/AI、计算服务、运行时业务线程、玩法或物理算法，不删除 OpenGL；GL 只做受影响回归，不强制新增捕获/marker 特性等价。所有历史数据、失败轮、命令/结果、冻结包和暂停任务现场原样保留，退役不意味着旧包在新环境继续可运行或提供现役历史报告查看器。

## Capabilities

- **新增 `development-profiling`：** 外部捕获工作流、CPU 埋点/计数、工具版本及测量解释边界、退役后的历史证据保护。
- **修改 `telemetry-lifecycle`：** 删除封闭采样/导出要求，增加轻量快照；保留并更新场景与观测独立的既有能力路径。
- **修改 `application-lifecycle`、`platform`：** 新场景入口、初始视点映射、输入隔离/窗口退出策略；移除旧 Session/最终导出职责，在原能力中明确 T4 旧工具/协议及 T2 Q06/预算的历史适用范围，保持装配、清理与普通玩法顺序。
- **修改 `renderer`：** 原生命名/marker，退役旧性能专属结果/query；直接修改 `Modern Backend Takeover Acceptance` 中 A02/A17/A18 及 strict 准备的现役适用边界，保留实际 Info、frame ID、截图、部署/资产身份、错误与在途寿命。
- **修改 `module-boundaries`：** 可选 profiler 依赖隔离、唯一编译 owner、旧独立产品退役及非性能 YAML 适配的独立归属；消除依赖/集成要求中继续构建旧 benchmark 的歧义。
- **修改 `foundation-files`：** 仅撤销旧 telemetry/runner 终态消费者约定，保留 Files::Publish 的实现与全部文件可靠性边界。
- **无业务变化：** [ecs](../../specs/ecs/spec.md)、[world](../../specs/world/spec.md)、[simulation](../../specs/simulation/spec.md)、[physics-core](../../specs/physics-core/spec.md)。TPS 读取现有 `AdvanceStatistics::physical_steps`，不建立第二计步或时间权威。

## Impact

实现影响 app、telemetry、renderer 两现代后端及旧 GL timer 适配、新增的窄 profiling 编译适配、构建/依赖与许可、根测试、活动 benchmark 工具和专属脚本/安装规则。实际删除清单须在依赖复核后完整汇报，并在收到该清单后的明确确认下执行；本 proposal 审核不替代删除授权。

当前 telemetry 的 `document_io` 还服务 world-summary、foundation 文档往返和 GPU 故障探针。拟原义迁到 foundation 下独立 `symocraft_document_io` 适配 target，公开只见既有 `Data::Value`，私有消费 yaml-cpp；不使基础 `symocraft_foundation`、独立 physics 或所有 CPU 消费者传递链接 YAML/Tracy。此举不是新文件服务或序列化格式升级。

[NPC 草案](../npc-framework-harvest-mvp/proposal.md)仍只处于用户确认的 proposal 范围，不替它生成或改写规划。它的 T5 未同步/中低端实施前门槛文字落后于当前 scope，以后者及本轮 AGENTS 为准，不把本次工具接入设为新的玩法实施批准。[Fix1](../m3-t2-publication-fix1/proposal.md)继续暂停，其 foundation 可靠性问题保留开放；旧性能消费者退役后，它的相关 V01/V04–V07 目标需另获授权再协调，不能从本 change 推定已修复、关闭或恢复。历史 T2/T3/T4/T5 验收场景及成绩只保留对应候选身份，不要求复活已退役产品来满足旧采样门槛。

上述退役边界由本 change 的各原 capability 增量分别定义，不由 `development-profiling` 跨能力总括覆盖。renderer A18 的安装/CWD/资产条件与旧采样协议拆开，A17 的截图与旧异步计时拆开；保留原验收身份和全部继续有效的场景。原已归档计划、命令和成绩不回写，未实现前不修改主规格。

工具接入完成后再安排指定 Y9000P/GTX1650 其他机器验证，延后时点按当前 scope，不在这里运行、豁免或追加预算。新工具轨迹与旧 CSV 的口径不自动等价，RenderDoc 注入/回放和 Tracy 开关也会改变成本，不预设性能提升或“无开销”。

## Implementation

### Short Approach

按“保住场景与基础契约 → 建工具路径 → 迁移观测 → 准确退役 → 验证”的顺序交付，不长期维护两套性能链。

拟采用 Tracy 官方 `v0.14.1` 为评审候选版本，实施时核对完整 commit、源包校验与许可证并记录在依赖来源中，Viewer 使用同版；若实际 MSVC 兼容性要求换版本，先报告，不悄悄跟随 master。在 `game/modules/profiling/` 放置仅含薄宏头与 CMake 接线的 `symocraft_profiling` 适配 target，独立于 telemetry 和基础 foundation，不建立另一个 Session/采集服务。开关拟定为 `SYMOCRAFT_ENABLE_TRACY`、`SYMOCRAFT_GPU_DEBUG_MARKERS`；前者关闭时完全不求值参数/不需 vendor，不在公开业务头暴露 Tracy。开启时私有授权 app、world 网格阶段、simulation 适配与 renderer CPU 阶段，`TracyClient.cpp` 每个最终程序只编译/链接一次；独立 physics 核心不插桩。仅本机连接、按需采集，不启系统追踪/自动采样或退出等待 Viewer，不改变游戏故障处理；Tracy 自带内部线程不获得业务对象访问权。

新增独立 profiling preset，显式开启两项工具开关，保留 Release 优化并生成匹配 PDB，专用目录构建全依赖，不把 Debug 当性能基准；Debug 继续执行 DBG01，全闭包 `_ITERATOR_DEBUG_LEVEL=0`、Debug CRT/调试信息和原检查不变。shader 调试信息与 validation 单独记录，不为可读性静默关闭优化或验证。普通 Debug/Release 默认仍关闭 Tracy 和额外 GPU marker。

CPU 第一批区域只覆盖真实已有的 event/input、scenario、Advance/character/physics adapter、world edit/mesh、render cache/upload、prepare/submit/present 及装配/关闭；共享同一帧标记边界，Present CPU 等待不标成 GPU draw。计数限已有事实中的物理步、网格重建、draw、上传字节；区分每帧增量与累计，不新建业务遍历或持续增长的动态名称。

D3D12 使用 `SetName` 与原生 command-list `BeginEvent/EndEvent`，无需额外 PIX runtime；Vulkan 独立检查 `VK_EXT_debug_utils` 与函数可用性，不将 marker 开关绑定 validation。关闭为 no-op，启用但缺注释能力时只明确诊断并降为无注释，不降低原有渲染能力/校验要求。名称使用稳定逻辑身份，不含地址或用户目录。事件作用域成对，失败路径不改变已有 fence/呈现退休/Shutdown 语义。依据 [RenderDoc 原生注释文档](https://github.com/baldurk/renderdoc/blob/v1.x/docs/how/how_annotate_capture.rst)；Tracy 配置依据 [固定版本 CMake](https://github.com/wolfpld/tracy/blob/v0.14.1/CMakeLists.txt)与[官方发布](https://github.com/wolfpld/tracy/releases/tag/v0.14.1)，均不构成此机验证结果。

### Work Nodes

保留一个 `integrate-renderdoc-tracy` change，按下列四个节点组织分工，不拆成多个 OpenSpec change。节点用于后续 sub-issue/执行分工，本轮不创建外部 issue、不派发实施任务。下方 S1–S10 仍是唯一任务清单；节点表只引用任务身份，不另建 design/tasks 或各节点的平行计划。任务编号不是强制串行顺序。

| 节点 | 职责与交付边界 | 对应任务 |
| --- | --- | --- |
| N1 公共适配与构建 | 分离非性能 YAML 适配；交付 Tracy 薄宏/唯一 client、开关、preset、来源/许可及依赖隔离；集中维护共享构建接线 | S3、S4；配合 S8 的构建/安装退役 |
| N2 应用与 CPU 观测 | 场景解析/初态/输入策略、轻量 telemetry 和应用接线；app/world/simulation 的 CPU zones、帧标记与计数，不插桩独立 physics 核心 | S2、S5 的应用/业务 CPU 部分、S7 的应用/telemetry 部分 |
| N3 渲染观测 | renderer CPU zones、双现代资源名/原生 marker、GameRenderer 解耦旧采集器；迁移旧 GPU 计时消费者，保留截图及正确性同步；负责 RenderDoc 工作流 | S6、S5 的渲染部分、S7 的渲染部分；配合 S8 的渲染退役 |
| N4 退役与整体验收 | 汇总消费者和准确删除清单，协调旧工具/协议退役；核对最终集成及有效契约覆盖，集中维护交付文档和同一 verification.md | 统筹 S1、S8、S9、S10，汇合 N1–N3 的局部检查 |

**依赖与并行：** S1 由 N4 统筹、N1–N3 提供各自输入，先确认既有范围/未决项、代码根授权及适用验证边界，并约定薄宏接口、YAML 新归属/公开头、调试快照、帧边界/计数口径和 GameRenderer 去除 Session 后的调用面；不由协调人代替用户作批准。接口约定是启动并行的共同前提，不要求先完成所有公共实现。

- S1 前提成立后，N1、N2、N3 可同时启动；N2 的场景/轻量数值及 N3 的原生命名/marker 可先推进，不必等待完整 Tracy 接入。
- N1 交出可消费的薄宏/target 和所需 YAML 适配后，N2/N3 才完成相关接线及开启态构建/运行检查；不能用临时复制 client、业务反向依赖或修改独立 physics 闭包来绕过依赖。
- N2/N3 按共同调用面分别改应用调用方与 GameRenderer/renderer 实现，接口调整先协调，禁止两边自行保留旧 Session 兼容壳。S5/S7 必须在两边对应工作及适用检查汇合后才可勾选。
- N4 可提前只读盘点 S8 的消费者与保留原因；实际退役须等待相关替代能力和有效测试迁移就绪、准确删除清单获用户确认。N1–N3 分别交接后，N4 才对最终同一候选执行 S9 并收口 S10，不用局部成绩代替整体验收。

### File Ownership

下列路径相对代码根，表示编辑责任而不是额外功能范围；既有模块生产所有权不因分工改变。共享构建文件的集中规则优先于目录归属，非性能 YAML 和 app 渲染适配的例外如下。表外受影响文件（如必要的平台适配或共享脚本）须先明确单一 owner 再修改，不默认归 N4，也不借分工扩成无关重构。

| 节点 | 唯一主要编辑范围 |
| --- | --- |
| N1 | 根 `CMakeLists.txt`、`CMakePresets.json`、`cmake/` 中受影响文件，以及 `game/`、根 `test/`、各生产模块和单元测试的共享 CMake/注册接线；拟新增 `game/modules/profiling/` 及固定 Tracy 来源/许可接线；foundation 中 YAML 适配的迁入文件、telemetry 原 `document_io.*`、`test/unit/foundation/document_tests.cpp`。不改 Files::Publish 机制或其他 foundation 业务 |
| N2 | `game/app/src/` 中 application、startup_options、benchmark_scenario、foreground_readiness、game_entry/main、cleanup/shutdown 和应用探针，排除下行 N3 的渲染适配；`game/modules/telemetry/` 排除 N1 的 document_io/构建文件；world/simulation 的既定 CPU 埋点调用点，排除 `simulation/physics/`。负责对应 `test/unit/app/`、`telemetry/`、`world/`、`simulation/` 测试，排除 N3 的渲染缓存/转换测试 |
| N3 | `game/modules/renderer/` 的公开门面、私有后端及旧 GL 受影响适配；`game/app/src/game_renderer.*`、`world_render_cache.*`、`render_conversion.*`；`test/unit/renderer/`、app 的 `render_cache_tests.cpp`/`render_conversion_tests.cpp` 及 `test/integration/renderer/` 中仍有效的图形探针/检查。构建接线仍交 N1 |
| N4 | `tools/benchmark/` 旧产品范围、`test/benchmark/`、专属 benchmark fixture 和 `scripts/benchmark.py`/`.ps1` 等经核对的旧链入口；汇总跨域脚本的保留/迁移/退役意见，不整目录清理 scripts 或图形测试。负责本 proposal、全局操作/导航说明及同一 change 的 verification.md；受影响模块架构由 N1–N3 提供必要内容后集中协调更新 |

- YAML 新公开头由 N1 交付，各调用方 include/链接迁移由其文件 owner 配合；N1 不为迁移批量改写 N2/N3 正在工作的文件。
- 各节点提出所需构建/安装/测试注册改动，统一由 N1 接线；N4 汇总的旧产品退役也遵守此规则。共享头或跨归属文件须先明确交接，禁止并行修改同一文件或依赖“最后覆盖”。
- 删除候选由 N4 汇总，收到用户对准确清单的明确确认后，由对应文件 owner 执行或明确交接；节点目录表不是删除授权，也不允许清理历史包、旧缓存、Fix1 现场或个人文件。
- D3D12/Vulkan 私有后端可在 N3 内按文件分开推进，但公开契约保持单一 owner；同机真实 GPU 捕获/性能检查错开执行，不混用构建目录或继承另一后端成绩。开发分工不授权运行时并发访问 World/ECS 或新增业务线程。
- N1–N3 各自完成局部检查并交接必要结论；N4 统一核对任务勾选与最终候选身份。只维护一份 verification.md，不新建节点验证报告，不自动填人工批准、sync/archive 或执行 Git 操作。

### Tasks

- [ ] S1 固化实施前输入：记录 Tracy/RenderDoc 与工具链身份、确认代码根授权，核对新场景参数和本轮“数值无新面板”边界；向用户提出并确认适用构建/真实捕获检查及有界人工时长，不直接恢复历史长测。
- [ ] S2 从旧采集中解耦 static/walk/edit 与启动解析，加入新场景/时限；覆盖无输出目录/无工具运行、参数互斥/旧参数拒绝、玩家隔离、Esc/关闭、时限、失焦/尺寸和普通编辑顺序回归。逐场景检查 walk→spawn、static/edit→four-chunk 的解析映射、首次 Advance 前位姿及 45 度 FOV，保持现有 checkpoint 定义、原 0.25 秒 edit 节奏和反向恢复循环。
- [ ] S3 分离现有 YAML 适配 owner 与调用方，保持编码/加载/错误行为；验证 world-summary、文档往返、受影响探针及 CPU/physics 实际依赖图，不将非性能诊断输出误删。
- [ ] S4 增加可选 Tracy client/薄宏与 profiling preset、来源/许可/部署规则；验证默认 OFF 且 vendor 缺失仍可配置、ON 缺依赖明确拒绝、参数不求值、多翻译单元/最终符号唯一、DBG01 ABI 一致、CPU-only/关闭测试隔离及安装产物无 Viewer/RenderDoc 强依赖。
- [ ] S5 在上述既有生产阶段接入 CPU zones、帧标记、计数与真实配置身份；以同一生产库验证开关不改变模拟/编辑结果、物理步数与关闭，核对 Viewer 连/断/未运行时游戏均可正常退出，无每体素埋点或额外业务线程。
- [ ] S6 两现代后端接入独立开关的资源名/事件组并完善外部 RenderDoc 启动说明；在用户确认的范围内分别核对真实捕获、回放纹理/网格/选框及事件组、Vulkan 注释不可用与 validation 独立性，保留 GL 受影响回归。
- [ ] S7 用轻量快照替换 app 的 Session 接线；测试 Presented/跳过/暂停/恢复、零/多物理步、缺值、持有旧快照和长期常量级存储；从普通循环、截图和关闭路径移除对性能 Session/query/系统内存采集的需要，保留所有正确性等待/错误事实。
- [ ] S8 逐项复核旧工具/协议/采样 API/查询资源、构建安装、专属脚本与测试消费者，汇报准确删除清单并等待确认；先迁移仍有效的截图、身份、输入隔离、文件和寿命测试，再按授权退役旧链，旧缓存选项不得静默重新构建 benchmark。历史数据、冻结包、Fix1 与共享脚本保持。
- [ ] S9 在确认的适用范围内执行最终构建/边界/自动回归及双现代工具验收，检查新候选不再创建旧采样查询或导出文件，且截图及无关 CWD 安装运行不依赖旧 Session/协议/工具；核对各原 capability 的有效契约仍有对应依据，历史采样条款不重新成为现役产品门槛。分别说明 CPU/真实 GPU、失败/未执行、工具引入成本与旧预算不可直接比较的限制。
- [ ] S10 随实际交付更新受影响架构、工具操作、主入口及有效 Agent 构建说明，移除“现役独立 benchmark”导航而保留历史恢复路径；仅在本 change 的 `verification.md` 记录必要结果，不另造报告/附件链，不填人工批准，不自动 sync/archive/Git。

本轮只创建本 proposal 与必要 delta specs，全部任务未执行。规划完整只表示可供审核；实施、准确删除清单、实际检查范围/人工时长及最终接受分别确认。
