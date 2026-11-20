---
tags:
  - area/spec
---

# NPC 框架与耗时采集 MVP 验证

## 当前候选与范围

2026-11-16，用户明确授权继续 apply，并允许三个继承相同推理强度的子 agent 分别处理 N1、N2、N3。实施根为 `G:/Projects/Symocraft`，分支 `npc-framework-harvest-mvp`，基点 `a6620be9e3e6fbc4a06244686d971b6e6ce754a9`；候选包含尚未提交的本次源码、资产和文档。原有无关改动保留。本轮未提交、推送、同步主规格或归档，也未登记用户整体验收。

正式 physics 迁移的独立验收见[原节点](../2026-11-16-promote-physics-module/verification.md)。当前候选使用 `game/modules/physics` 的唯一生产实现；下述新检查来自当前候选，不借用该节点历史成绩证明 NPC 正确性。

## 正式 CPU 与替身检查

环境：Windows x64、MSVC 14.51.36231、Windows SDK 10.0.28000.0、CMake 4.4.2、Ninja，Debug CRT 及 `_ITERATOR_DEBUG_LEVEL=0`。配置通过 `windows-debug` preset 并显式覆盖 Python 路径，CPU-only 使用独立目录 `out/npc-mvp/cpu-debug-002`。开发终端环境由现有 `scripts/build.py` 的初始化函数取得。

- CPU-only 完整构建成功，CTest **110/110 通过**。包括独立公开头消费者、模块正负边界、现有玩家/物理/会话回归、World 采集和角色 CPU 导入。命令为 `cmake --build out/npc-mvp/cpu-debug-002 --parallel 8`，以及 `ctest --test-dir out/npc-mvp/cpu-debug-002 --output-on-failure --no-tests=error -j 8`，在上述开发环境中执行。
- `world.harvest` 使用正式 World，覆盖旧配置、采集参数拒绝、同格同种替换、跨世界标识、无变化/其他格编辑、真实表项分配失败和版本耗尽；失败不发布方块或版本的部分结果。
- `simulation.npc` 使用正式 ECS、World 与物理，初轮八组通过。审阅后增加提交前当前地形支撑/身体空间复核，重编后九组通过；实际检查包含装配/移除、隔离、真实移动与受阻、计时/暂停、S1 顺序与玩家优先、重复批次、F1 溢出/真实分配失败/移除后故障。后一次定向 CTest **4/4 通过**，同时覆盖 NPC 两个公开头和 AI 测试。
- `simulation.npc_ai` 分开使用受控动作替身验证树与端口，以及正式三 NPC 会话验证独立目标和同目标竞争、胜者计数、败者失败及后续 Idle。正式树运行来自固定的 BehaviorTree.CPP 4.9.0，不以替身代替 World 或物理结果。`assets/ai/npc_nodes.xml` 已由实际工厂导出接口生成；没有实际操作 Groot2 GUI，不冒称已完成编辑器人工体验。
- `assets.character_import` 正式导入真实 Blender 4.5.16 导出的两份 GLB，检查拥有数据寿命、颜色、硬法线、坐标及坏资产诊断。可编辑 `.blend` 与生成脚本保留，cgltf 固定为 v1.15。
- `renderer.facade_contract` 的替身后端验证角色共享模型、实例清单、句柄隔离及失效；真实 GPU 结果单独见下节。

## 联合构建、真实 GPU 与部署

| 当前配置 | 实际结论 |
| --- | --- |
| `out/npc-mvp/modern-debug`，D3D12 + Vulkan + 保留 GL | 完整构建成功，CTest **120/120**；最终 N1/N2 及角色契约增补定向 **8/8** 通过，最终应用接线重编成功 |
| `out/npc-mvp/modern-release`，同后端组合 | 完整构建成功，CTest **120/120**；最终应用接线重编成功 |
| `out/npc-mvp/cpu-tests-off`，CPU-only + `BUILD_TESTING=OFF` | 全新配置及完整生产构建成功，不以空测试成绩代替构建 |
| `out/npc-mvp/install-release` | 安装成功，包含 AI/场景/GLB、两后端角色 shader、新库及嵌入组件许可；D3D12/Vulkan 搬迁包、缺资产及非法参数检查分别通过 |

真实 GPU 为本机 NVIDIA GeForce RTX 5070 Ti，D3D12 驱动 `32.0.16.2101`，Vulkan 报告 `NVIDIA:621.01`。两后端扩展既有 lifetime probe，各绘制同一角色模型的两个实例，在真实未完成 fence/timeline 下删除逻辑句柄，证明 native 资源仍被 frame slot 持有，完成后随 slot 复用释放。两者均通过，验证层警告/错误均为 **0/0**，关闭后记录的活资源字节均为 0。该探针使用受控静态几何与真实 backend，不将它写成 Blender 模型或正式性能预算证据。

正式 Application 另实际加载交付的 Blender GLB 和树：D3D12 独立目标、D3D12 争抢、Vulkan 争抢、部署后 Vulkan 独立目标均正常退出。独立场景两人各得 `wood:2`；争抢场景一人成功、另一人 `InvalidTarget` 且无资源，随后两树 Idle，僵尸始终 Idle。D3D12 与 Vulkan 各读取一次真实窗口画面，能辨别人类和僵尸，独立场景的目标均移除；两个交互窗口均以 Escape 正常关闭。窗口观察由 agent 执行，不能登记为用户人工体验或整体验收。

保留 GL 的 `--scene regression --smoke-frames 120` 正常启动/渲染/退出。既有正式 `app_profile_scenario_probe` 的 D3D12 static/input、walk/focus-loss 及 Vulkan edit/focus-loss 通过；初态检查成立，edit 一秒内执行三次既定模拟前编辑，原冷却和时点另由会话/场景 CPU 回归核对。这些是有界功能检查，不是性能采样或长时稳定性结论。

## 尚待人工验收

V2 保持未勾选，等待用户在本机 D3D12、Vulkan 各做一次短体验及给出整体验收结论。每个后端所有子场景/重启累计不超过三分钟，两者合计不超过六分钟。代理窗口观察、测试、CLI 校验及任务勾选均不代替用户决定。其他机器、长期稳定性、正式性能预算与干净机发布仍未测试。

## 已遇失败及处理

受限进程环境的首次 MSVC 编译器探测报 C1902 调试数据库管理器错误；在正常开发进程环境配置新目录后通过，失败目录保留。首轮源码编译暴露测试文件多余括号及并发完成中的 N2 声明/定义不一致，修复后 CPU 构建通过。双后端首轮构建发现 Vulkan 角色着色器将 push constant 属性附在 cbuffer 上，改为结构化全局 ConstantBuffer 后重编通过。

探针首次误传 D3D12 shader 根目录、一次误用 `--validation on` 及搬迁包脚本的初次 shell 路径转交均在有效验证前失败；使用正确目录、`--validation 1` 及结构化路径参数后分别通过。一次窗口观察未赶上有限帧程序的退出，随后用交互窗口完成观察并正常退出。最终 Release 首次 1200 帧仅覆盖一人完成，后续使用足够帧数检查完整终态，不把前次短运行当成两人完成的依据。失败与短覆盖日志留在 `out/npc-mvp`，没有删除或重写为首次通过。

OpenSpec 普通 `validate` 通过，严格模式有八项增量文件开头标签元数据警告，未报契约错误；按项目标签要求保留元数据，未以删标签掩盖警告。文档检查报告 36 项旧缺失链接，分布于 `docs/issue/issues.md`、其他变更的环境恢复 proposal 和 `docs/verification/VERIFICATION.md`；此次 NPC/架构文件没有报告链接错误。它们不在本变更中修复，也不宣称全库文档检查通过。Git 差异空白检查通过，现有行尾转换提示未触发全库归一化。

## 2026-11-16 托管工作树迁移后复验

用户要求在新托管工作树重新构建和测试。本轮代码根为 `G:/Sandbox/Worktrees/npc-framework-harvest-mvp/Symocraft`，分支 `codex/npc-framework-harvest-mvp`，基点仍为 `a6620be9e3e6fbc4a06244686d971b6e6ce754a9`。上述主工作区路径和成绩保留迁移前的历史含义；下列结果来自新工作树的独立完整编译。追加本节前，247 份迁移文件的 SHA256 全部与迁移快照一致。

环境为 Windows x64、MSVC tools `14.51.36231`、SDK `10.0.28000.0`、Python `3.12.16`、CMake `4.4.5`、Ninja `1.13.2`。通过现有 `scripts/build.py` 初始化开发环境，使用 `windows-debug` / `windows-release` preset，显式绑定编译器、Ninja、Python 和 Vulkan SDK `1.4.363.0` 的 DXC/headers。Debug 保留 Debug CRT 与 `_ITERATOR_DEBUG_LEVEL=0`。

四个目录均在本工作树的 `out/npc-migration-recheck/` 下全新配置，执行完整依赖构建；有测试的配置均执行 `ctest --test-dir <目录> --output-on-failure --no-tests=error -j 8`。

| 目录 | 配置与结果 |
| --- | --- |
| `cpu-debug` | CPU-only Debug，完整构建成功，CTest **110/110** |
| `modern-debug` | D3D12 + Vulkan + 保留 GL，Debug 完整构建成功，CTest **120/120** |
| `modern-release` | 相同后端组合，Release 完整构建成功，CTest **120/120** |
| `cpu-tests-off` | CPU-only、`BUILD_TESTING=OFF`，完整生产构建成功 |

N1 的正式 World、ECS、物理和会话测试重新通过，包括采集规则、真实移动、暂停/取消、目标身份、玩家优先、稳定竞争顺序、一次入账及故障边界。N2 的正式 BehaviorTree.CPP 测试重新通过，包括资产/表达式拒绝、隔离、节点描述、扩展和正式三 NPC 闭环。N3 的正式 GLB 导入、拥有数据/属性/坏资产及 renderer 门面契约重新通过；公开消费者和模块边界也包含在上述当前测试结果中。

Release 安装到本轮 `install-release` 成功，7 份 NPC 配置/树/GLB 及两个后端共 12 份编译 shader 与构建产物逐字节摘要一致，新增依赖所需许可齐全，Blender 源未进入运行包。

真实 GPU 仍为 RTX 5070 Ti，D3D12 驱动 `32.0.16.2101`、Vulkan `NVIDIA:621.01`。新 Debug 的两个 lifetime probe 均通过，实际在途角色模型保持至 GPU 完成，删除后随 slot 复用释放；两者验证层警告/错误 **0/0**，关闭后记录的活资源字节均为 **0**。

正式游戏运行以下四组，每组实际完成 **3600 帧**并正常退出，现代后端均启用 validation：Debug D3D12 独立目标、Debug Vulkan 同目标争抢、Release D3D12 同目标争抢、安装包 Vulkan 独立目标。独立场景两人分别取得 `wood:2`；争抢场景仅 NPC 0 取得 `wood:2`，NPC 1 为 `Failed/InvalidTarget` 且无资源。人类随后进入 `idle_after_job`，僵尸始终 `zombie_idle`。Vulkan 应用关闭诊断同为警告/错误 **0/0**。保留 GL 的 regression 120 帧正常退出；既有 D3D12 static/input、walk/focus-loss 和 Vulkan edit/focus-loss 探针全部通过。这些是有界功能结果，不作为性能预算或长期稳定性结论。

本轮首次自动启动误传了仅供 profile scenario 使用的 `--width`、`--height`、`--vsync`，程序按现有契约拒绝并返回 64；修正验证命令后上述场景通过，原失败日志保留。一次有限帧窗口截图未能绑定进程；用户随后要求重新启动交互窗口，D3D12 独立目标和争抢场景均已成功置于前台，可见两个人类及绿色僵尸。本轮未完成 Vulkan 窗口外观观察。争抢是对同一个方块的采集竞争，当前没有资源面板或采集动画，结算依据仍为真实规则测试和动作/资源诊断。长对象路径的 CMake 提示及上游编译警告保留，实际构建与测试均成功。

新日志和结果留在本工作树 `out/npc-migration-recheck/`，旧缓存和历史结果保持原位置。复验未发现迁移造成的缺失或自动回归失败，未修改生产源码。V2 人工短体验及用户整体验收保持待完成，既有 Groot2 人工编辑、其他机器、长时及正式性能未验证的范围不变。

## 2026-11-16 分件动作与可观察演示实施复验

用户在更新方案后明确授权当前代码根 `G:/Sandbox/Worktrees/npc-framework-harvest-mvp/Symocraft` 继续实施、重新构建和测试，并要求三个相同推理强度子 agent。本轮三个子 agent 均继承同一模型与推理强度，分别负责分件资产、动作/屏幕反馈、演示控制，主 agent 完成只读状态交接、集成及统一验证。此前候选和迁移成绩保持原义，本节单独覆盖 C2、N1.5、N3.3–N3.5、V3；不登记用户整体验收。

实际交付新增 `--npc-scene demo`：默认单人完整采集，Space 开始/暂停/继续，Q 从初态新建一轮，E 切换单人与同目标争抢，Escape 退出。身体分件共享静态几何，以实际水平路程和有效采集时间驱动简单摆腿、摆臂；屏幕提示来自提交后的真实动作/资源值。每轮更换 World、Registry、Session、树实例及运行身份，保留窗口、Renderer 和单调帧号。没有历史录制、时间轴、任意时刻回退、历史续算或拆放干预。当前机制见 [assets](../../../../architecture/assets.md)、[simulation](../../../../architecture/simulation.md)、[app](../../../../architecture/app.md)和[renderer](../../../../architecture/renderer.md)。

环境沿用本机 Windows x64、MSVC tools `14.51.36231`、SDK `10.0.28000.0`、Python `3.12.16`、CMake `4.4.5`、Ninja `1.13.2`，Debug 为 DBG01。两份 `.blend` 和 GLB 由现有 Blender `4.5.16` 正式重新导出，源及导出均保留六个具名身体部件和显式肩髋轴；未引入字体文件、通用 UI 库或动画库。新目录及本轮工具输出均在 `out/npc-demo-recheck/`，旧缓存、失败日志和冻结材料保持原位置。

| 新构建目录 | 实际结果 |
| --- | --- |
| `cpu-debug` | CPU-only Debug 全依赖构建成功，CTest **113/113** |
| `modern-debug` | D3D12 + Vulkan + 保留 GL 全依赖构建成功；最终完整重编后 CTest **123/123** |
| `modern-release` | 相同后端组合全依赖构建成功；最终完整重编后 CTest **123/123** |
| `cpu-tests-off` | CPU-only、`BUILD_TESTING=OFF` 全新配置及生产构建成功 |
| `install-release` | 最终 Release 安装成功，7 份 NPC 配置/树/GLB 与当前源逐字节摘要一致，18 份部署 shader 与最终构建产物摘要一致，运行不要求 Blender/Groot2 或字体资产 |

新增 CPU 验证实际覆盖：分件拥有数据与层级重建、显式关节及局部旋转、整体静态兼容、缺失/重复/成环/零轴/非有限/奇异变换及节点上限拒绝；只读显示重复读取不消费运动/结果、暂停与恢复零时间、就绪与提交成功分离、跨会话身份和返回容器自有性；真实路程步态、受阻不踏步、采集有效时间、同次推进内取消/终态立即回中立、不同实例隔离；演示控制状态及有界字形/进度输入。原真实 World、物理、行为树、稳定争抢、失败边界、公开消费者和模块边界继续包含在当前测试中。

新增显式 `npc_demo_probe` 使用正式 Application、真实 World/Session、正式 Blender GLB 与 GPU，在窗口事件入口注入测试按键，并在提交及呈现后读取自有事实。D3D12、Vulkan 各完整通过 **8 轮**：Ready 不推进，移动与采集期间暂停及恢复首帧零规则时间，同轮失焦/获焦不解除手动暂停；从进行中、暂停及完成状态重播；反复切换，World/Session 身份更新、采集成功后目标变为空气 1 且重播复原为 12、旧动作/计数消失、原生帧号不回退；单人一次入账 `wood:2`，争抢仅一人入账、败方 `InvalidTarget` 且无资源，僵尸 Idle。完成后重复显示保留终态和资源。两后端还分别通过重播至新一轮 Meshes 装配点注入失败、报告首因并正常清理的检查。该自动探针使用现有有限帧诊断策略允许后台窗口运行；合成焦点事件结果不冒充真实系统焦点或最小化的完整故障矩阵。

真实 GPU 仍为 RTX 5070 Ti，D3D12 驱动 `32.0.16.2101`、Vulkan `NVIDIA:621.01`。当前两个 lifetime probe 均通过，角色句柄逻辑删除后仍被未完成帧持有，GPU 完成并复用 slot 后退休；验证警告/错误均 **0/0**，关闭后记录的活资源字节均 **0**，未隔离保留未明资源。这是受控 native 生命周期验证，不是实际设备丢失、任意 OOM 或性能预算证明。

最终正式游戏另通过四组 **3600 帧**正常退出：Debug D3D12 独立目标、Debug Vulkan 争抢、Release D3D12 争抢、安装目录 Vulkan 独立目标。独立场景两人各得 `wood:2`，争抢仅 NPC 0 入账，NPC 1 为 `Failed/InvalidTarget` 且无资源，僵尸保持 Idle。保留 GL 的 regression **120 帧**正常退出；既有 D3D12 static/input、walk/focus-loss 和 Vulkan edit/focus-loss 功能探针均通过，未扩展性能采样。

代理窗口观察使用新 Debug D3D12 及最终安装目录 Vulkan，操作与观察分别约 **159 秒**和 **79 秒**，均在每后端三分钟、合计六分钟内。实际看到真实行走摆腿、采集摆臂、进度及结算，切换和重播恢复初态，僵尸位于侧边。D3D12 首次观察发现较长争抢提示挡住角色头部，随后合并目标/资源/原因提示行；最终安装版 Vulkan 窗口确认紧凑面板、字形方向和完整可见角色。两个观察窗口均以 Escape 正常退出；暂停的逐值冻结及恢复依据来自上述正式 native 自动探针，未将未捕获的人工暂停画面写为已观察。代理窗口观察和自动测试不代替 V2 的用户短体验及整体验收。

本轮保留的失败与修正：受限进程中首次编译器探测报 C1902，正常开发进程环境下重配通过；首轮现代 Debug 构建发现新 native 测试缺少 telemetry 包含依赖，补齐后重编通过。首个后台无限帧 native 探针按真实失焦策略停在第零帧并超时，改用已有有限帧诊断入口；一次测试误将空气方块预期为 0，核对 World 正式移除契约后改为 1。交叉审阅发现 Vulkan 屏幕绘制在负高度 viewport 之外再次翻 Y，移除额外翻转并重新构建、native 验证及观察；最终面板遮挡修订也已重新构建并重跑双后端八轮检查。以上失败不改写为首次通过，原输出仍在本轮 out 目录。

交付前另启动最终安装版 D3D12，短暂确认 Ready、资源为空及可见操作说明后交给用户，未开始新一轮模拟；加上该次交接观察，D3D12 累计仍少于三分钟。此前遗留静态候选窗口已正常关闭，当前留下的是 `install-release/SymoCraft.exe --npc-scene demo --renderer d3d12 --validation 1`。

V2 保持未勾选；用户整体验收、Groot2 人工编辑、其他机器、长期稳定性、正式性能预算及干净机发布仍未验证。本轮不修改 Git 暂存，不提交/推送/合并，不同步主规格或归档。

## 2026-11-17 OpenSpec 实现核验

本次按 `symocraft-full` 的实际 contextFiles 对照 proposal、8 份增量规格、design、tasks 和生产实现，另读取已确认决策及既有测试/运行证据。共识别 **21 条 ADDED/MODIFIED requirement、39 个 scenario**，没有 REMOVED/RENAMED。CLI 的规划完整状态不作为实现验收；任务仍为 **16/17**。

| 维度 | 核验结论 |
| --- | --- |
| Completeness | 21 条 requirement 均找到实现入口；V2 未完成，列为 1 项 CRITICAL |
| Correctness | 本次相关测试 **14/14**；39 个 scenario 均完成源码/证据映射，其中 A08、A11 的 Groot2 实际编辑往返部分未验证，Scenario Coverage 为部分验证 |
| Coherence | S1/F1、Q01/Q02、CPU/显示隔离、分件共享与按真实事实驱动、每轮运行上下文替换均有实现依据；发现演示完成判定依赖可编辑节点名称，列为 WARNING |

实现映射覆盖 [NPC 装配/动作/结算](../../../../../game/modules/simulation/src/npc.cpp)、[会话交接](../../../../../game/modules/simulation/src/session.cpp)、[树校验/运行/模型导出](../../../../../game/modules/simulation/src/npc_ai.cpp)、[World 条件移除](../../../../../game/modules/world/src/world.cpp)、[分件导入](../../../../../game/modules/assets/src/character.cpp)、[应用演示编排](../../../../../game/app/src/application.cpp)、[真实事实姿态](../../../../../game/app/src/character_pose.cpp)及[共享绘制](../../../../../game/app/src/game_renderer.cpp)。生产代码及新增测试继续位于规定 owner，第三方树/解析器类型留在私有边界；未发现需单列的代码模式偏离建议。

本次运行当前现代 Debug 构建成功，输出仅同步运行资产，没有 C++ 重编；随后重跑 NPC、World 采集、角色导入、姿态、演示控制/反馈、启动参数、renderer 门面和相关公开头测试 **14/14**。11 月 16 日的 CPU 113/113、现代 Debug/Release 各 123/123、双后端八轮及 lifetime/安装/GL 回归仍是此前候选的运行证据，本次未将其写为新执行或再次开展整套矩阵。普通 OpenSpec validate 通过，仍保留 8 份增量文件标签头部的既有警告。

**CRITICAL · V2 未完成。** [唯一任务清单](tasks.md)第 46 行仍未勾选；聊天中的观看与问题反馈不等于用户明确整体验收。建议按约定短体验范围取得实际结果和用户结论，再依据事实更新 V2；本次不代勾人工项。

**WARNING · 可编辑节点名影响演示完成。** [Application 完成判定](../../../../../game/app/src/application.cpp#L522)要求 `current_node` 包含 `idle_after_job`，[生产树测试](../../../../../test/unit/simulation/npc_ai_tests.cpp#L208)也固定断言该名字。这与设计中“人类终结并进入 Idle 后显示 Completed”的语义存在耦合：只重命名节点不应改变完成状态。已在 `out/npc-demo-recheck/verify-idle-name-20261117-193526/` 的独立副本中，仅把人类 XML 的 Idle 名称改为 `settled_idle`，使用同一正式 Application 的 D3D12 native probe 运行。合法树成功加载，日志确认 `settled_idle` 为 RUNNING、真实采集成功且 `wood:2`，但 10000 帧内没有 Completed，探针返回 1，复现成立。源码、原始资产及正式试玩包没有修改。建议后续以明确的行为语义/终态事实识别 Idle，并增加“仅重命名 Idle 节点”的回归，避免把策划显示名称作为控制协议。

**WARNING · Groot2 编辑往返缺少实际依据。** A08、A11 已有工厂节点模型导出、XML 校验、注册扩展及加载/运行测试，但此前明确未操作 Groot2 GUI，本次也未执行“导入模型→编辑/保存→重启加载”全过程。建议补充该端到端检查并记录实际结果；当前部分未验证不等于已证实编辑能力不可用。

核验结论为 **1 项 CRITICAL、2 项 WARNING、0 项 SUGGESTION**；目前不能给出全部完成或可归档结论。未验证检查为 Scenario Coverage 中上述 Groot2 两个场景的实际编辑环节。Human 1 同帧稳定获胜符合 S1，独立 Perception、自动换目标、Groot2 实时监控及时间轴历史续算均不属于本 change 当前范围，不将其计为缺失实现。本次仅追加本节及生成 out 下测试产物，未修复实现、修改规格或任务勾选，也未同步、归档或执行 Git 写入。

## 2026-11-17 Idle 命名耦合修复与再次核验

用户随后明确要求修复命名耦合、补充重命名回归、重新构建和运行相关测试，再核验 change。本次在同一托管工作树完成局部修复，保持原目标规格、任务勾选、源 XML 和此前失败证据不变。

[NpcAiSnapshot](../../../../../game/modules/simulation/include/symocraft/simulation/npc_ai.h) 新增自有只读 `idling`，由私有树中实际项目 Idle 动作的 RUNNING 状态派生，并排除其他 RUNNING 动作；未 tick 或已 halt 时为假。节点显示名和诊断路径不参与判定。[Application](../../../../../game/app/src/application.cpp) 以真实动作终态和该语义值判断 Completed，原行为树测试也改为断言语义状态。读取不 tick 树、不改变动作或资源，公开接口不泄漏 BT 类型。当前机制已同步到 app/simulation 架构说明。

[CPU 回归](../../../../../test/unit/simulation/npc_ai_tests.cpp) 覆盖默认 Idle 名、`settled_idle`、省略 name、未 tick/halt，以及非 Idle 动作取名 `idle_after_job` 时仍不被误认；正式 World/Session 的独立采集和争抢分别验证原名及改名树，保持单次入账、败方无资源和真实目标移除。

新增可重复运行的[显式 native 回归入口](../../../../../test/unit/app/verify-npc-demo-renaming.py)：复制已构建探针、运行库和资产到全新 out 目录，只修改副本中 Idle 的 name，再运行正式 Application。脚本拒绝覆盖旧输出，不更改源码资产、常规构建资产或已有试玩包，也不加入无人值守 GUI CTest；使用方法补充在 scripts/README。

使用同一 MSVC tools `14.51.36231`、SDK `10.0.28000.0` 重新构建 CPU-only Debug、现代 Debug、现代 Release 的完整受影响依赖，三者相关 CTest 均 **14/14**。新增测试的局部变量遮蔽警告已通过改名消除，随后三配置分别重编并复跑 `simulation.npc_ai`，均通过；没有压制上游已有警告或更改编译 ABI。未把此前全量 113/123/123 成绩登记为本次全量重跑。

重命名副本在 D3D12、Vulkan 下各通过正式 native 探针 **8 轮**：单人和争抢均能进入 Completed，暂停/恢复、进行中/暂停/完成后的重播、场景切换、World/Session 身份隔离及原生帧号保持检查均通过。每次成功者只获 `wood:2`，败方无资源。两组日志均确认 `settled_idle` 实际运行；Vulkan 关闭报告验证警告/错误为 **0/0**，两后端输出未发现验证错误或警告。该改名自动回归仍不等于实际操作 Groot2 GUI。

修复后的 Release 安装至新的 `out/npc-demo-recheck/install-release-idle-fix/`，安装 exe 摘要与当前 Release 构建一致，并提供 `Play-NPC-Demo.cmd`；旧试玩目录和当时正在运行的窗口保留。新测试和构建输出留在既有 `out/npc-demo-recheck/`，此前失败副本未删除。

再次核验仍为 **21 条 requirement、39 个 scenario、16/17 任务**：原命名耦合 WARNING 已由语义判定、CPU 及双后端实际改名回归关闭；其余需求实现映射和模块/所有权模式未发现新偏差。当前为 **1 项 CRITICAL（V2 用户短体验/整体验收未完成）、1 项 WARNING（A08/A11 的 Groot2 实际编辑往返部分未验证）、0 项 SUGGESTION**。Scenario Coverage 在上述编辑环节仍为部分验证，因此不宣布全部完成或可归档。普通 validate 通过，8 条既有标签头部警告保留。没有因本次修复扩大 Perception、自动换目标、实时编辑器连接或历史回放范围，也未执行主规格同步、归档或 Git 写入。

## 2026-11-17 Groot2 实际编辑往返与再次核验

本轮按用户要求补充实际工具操作，不修改生产代码、正式树资产或任务勾选。使用本机已安装的 `F:/Toolchains/Groot2/bin/groot2.exe`，启动日志明确为 FREE，窗口标识 `Groot 2 - BT.CPP 4.10`。用户原实例带有未保存项目，保持原样；另启独立实例，使用 `out/npc-demo-recheck/groot-roundtrip-20261117-200036/` 下的节点描述及完整 Release 安装包副本。游戏仍为前节已构建的 BehaviorTree.CPP 4.9.0 候选，未升级依赖。

通过 Groot2 GUI 导入 `npc_nodes.xml` 后，Models 中实际出现 Harvest、Idle；打开副本 `human_harvest.xml`，在节点编辑器将 Idle 实例名改为 `groot_saved_idle`，将 Harvest 的 `target_x` 从黑板映射改为常量 0，再改为 4，并实际保存。端口输入需先按 Enter 提交，再 Ctrl+S 保存；仅输入并离开字段时，首次保存仍保留旧端口值，核对落盘 XML 后已纠正。Groot2 保存文件带 `BTCPP_format="4"`、`BTCPP_version="4.10"` 和项目节点模型；当前 4.9.0 运行时实际加载和执行通过，此结论只覆盖本次节点与端口，不推定支持全部 4.10 功能。

使用同一副本 exe 和正式 World/Session/物理、D3D12、`--validation 1`，实际执行以下往返；游戏均经 Escape 正常退出，日志均为 `shutdown complete; exit_code=0`。

| 保存及启动顺序 | 实际结果 |
| --- | --- |
| 保存 X=0，启动 `independent` | 两人实际请求 `0,200,-1`；Human 1 成功并取得一次 `wood:2`，Human 2 为 `Failed/InvalidTarget` 且无资源；两人均进入 `groot_saved_idle` |
| 上述进程仍运行时在 Groot2 保存 X=4，退出并重启同一 `independent` 包 | 新进程两人实际请求 `4,200,-1`；Human 2 先完成并取得一次 `wood:2`，Human 1 失败且无资源；改名后的 Idle 正常运行 |
| 以 X=4 启动 `demo`，在 Ready 时通过 Groot2 将磁盘改回 X=0 并保存，随后开始及 Q 重新演示 | 两轮仍实际采集 X=4，各取得一次 `wood:2`，均进入改名后的 Idle 并显示 Completed；磁盘新版本没有影响已加载树，Q 也没有重载资产 |
| 退出上述进程，以磁盘 X=0 重启 `demo` 并开始 | 实际目标改为 `0,200,-1`，成功取得 `wood:2` 并显示 Completed，确认运行中保存的修改只在下次进程启动生效 |

这补齐 A08 的编辑、保存、真实动作绑定、启动加载和运行中版本保持证据；A11 的项目自定义 Harvest 节点描述、端口编辑、正式动作参数与结果、节点/动作诊断也完成实际往返。通用能力注册的 Count 扩展、参数拒绝和诊断继续由 [AI 回归](../../../../../test/unit/simulation/npc_ai_tests.cpp)验证，不把该替身能力冒称本次 GUI 中新增的生产能力。游戏没有连接编辑器，编辑和运行消费本地 XML；本轮未另外测试首次许可激活或断网冷启动。

四个游戏进程的目标、终态、资源、Idle 名称、三轮 demo 完成及正常关闭均由独立汇总检查再次核对；输出未发现 D3D12 验证错误或警告。源树摘要与启动前一致，三份正式 XML 与原安装包逐字节一致，副本 exe 与原安装包一致。独立编辑器保留验证副本供查看。首次验证启动误传超出 CLI 上限的 `--smoke-frames 100000`，被明确拒绝；失败输出保留，后续采用交互运行并正常退出，不把该失败计为有效运行。过程产物留在上述 out 目录，不新增文档附件。

再次读取 `symocraft-full` 当前 proposal、8 份增量规格、design、tasks 及决策，复核生产入口和此前证据。现代 Debug 相关 CTest 本轮重跑 **14/14**，覆盖 AI/能力扩展/重命名、NPC、World 采集、角色导入、程序姿态、演示控制/显示、启动和相关公开消费者。没有生产代码修改，未重新编译或重跑完整 Debug/Release/CPU-only/GPU 矩阵；前节成绩保持其历史范围。普通 OpenSpec validate 再次通过，仍有 8 条既有增量标签头部提示，未改写元数据来消除提示。

| 维度 | 本次核验结论 |
| --- | --- |
| Completeness | **16/17** 任务；21 条 requirement 均有实现映射，唯一未完成项仍为 V2 |
| Correctness | 39 个 scenario 完成实现与证据复核，A08/A11 的实际 Groot2 往返缺口已补齐；A22/V2 的完整双后端用户短体验及整体验收仍未完成，不能将其记为全体验收通过 |
| Coherence | 节点名不参与完成协议、树仅启动加载、演示重建复用启动资产、CPU/显示与第三方依赖边界符合设计；未发现新增偏差 |

当前结论为 **1 项 CRITICAL、0 项实现 WARNING、0 项 SUGGESTION**。Groot2 编辑往返 WARNING 关闭；保留的 CRITICAL 是 [V2](tasks.md#联合验证与交付) 用户短体验与整体验收，须由真实体验结果及用户明确结论关闭，本轮不代勾。Scenario Coverage 中 A22 的完整用户体验部分仍未验证；上述定向编辑/加载功能检查不作为新增 V2 双后端短体验通过，也不证明长期稳定性、正式性能或其他机器兼容性。本次未同步主规格、归档或执行 Git 写入。

## 2026-11-19 V2 与整体验收通过

用户在本聊天明确确认：“V2短体验和整体验收通过了”，并要求归档 `npc-framework-harvest-mvp`。据此登记 V2 及本 change 整体验收通过，唯一任务清单更新为 **17/17**；此前因 V2 未完成而保留的 CRITICAL 已关闭，A22 用户体验部分按本次用户结论关闭。11 月 17 日的核验是当时状态，原文及所有历史失败保持不变。

本次是用户验收结论登记，没有新增构建、测试或游戏运行；不补造体验时长、逐项操作或新的性能测量。既有正式 CPU/World/物理、替身、GPU 与 Groot2 实际往返的证据及适用范围沿前节记录，长期稳定性、正式性能、其他机器及干净机等未覆盖范围继续保留。归档授权不扩展为 Git 提交、推送、合并或主工作区生产传播授权；主规格同步选择与归档实际结果另按本轮流程登记。

同日用户选择“先同步主规格，再归档”。8 份增量已合入当前主规格：新建 character-assets、npc-ai、npc-entities，更新 application-lifecycle、module-boundaries、renderer、simulation、world，共新增 20 条要求、修改 1 条要求，21 条要求逐条复核一致。现有标题、Purpose、未涉及的要求和仍有效场景保持；新能力 Purpose 取自增量，没有 TBD。npc-entities 的 ECS 链接改用引用式，正文逐字保留且两处均指向同一 ECS 规格。全部 **14/14 主规格** validate 通过，保留 npc-ai Purpose 少于 50 字符及既有长要求提示，不借同步重写契约。旧归档 Markdown 摘要和 Git 暂存差异保持，具体旧测试成绩没有重记为本轮执行。

归档完成于 `docs/openspec/changes/archive/2026-11-19-npc-framework-harvest-mvp/`，包含全部 14 份文件及原 `.openspec.yaml`。移动后逐条复核 8 份主规格已无待应用增量；新归档中的相对链接按新位置调整，现役架构与批准导航改指主规格及归档记录。已有旧归档 Markdown 原字节和 Git 暂存差异核对保持，未提交、推送、合并或传播生产代码。

归档后全部主规格再次校验 **14/14**；本归档 105 处相对链接的目标及 Markdown 锚点检查通过，八份 Purpose 保持同步来源，17 项任务全部完成，活动列表已无本 change。标签导航通过现有检查器刷新。全库文档审计归档前后均有 36 项旧链接问题，没有新增错误；未将全库文档检查宣称为全绿。最终相关差异空白检查通过，本轮没有重新执行构建、CTest 或游戏体验。
