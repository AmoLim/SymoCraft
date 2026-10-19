---
tags:
  - area/meta
---

# T4 交付、T3/T4 归档与 T5 前置核对提交划分

## 本轮边界与基线

2026-10-19 先编写本页划分；随后本轮明确确认“T5的文档也修正了描述，进行提交吧”，授权按下述四组执行本地暂存及提交，不推送。不复用旧批次授权，不再次执行 spec sync/archive、迁移生产代码、勾选验收或修改既有结果。下述检查是执行要求，实际执行与未执行结果在聊天报告。

当前分支 `feature-refactoring`，规划基线 HEAD 为 `f38e515987e17331db1917b8b256fbd3be477829`，起点索引为空。排除个人目录后，盘点到116项已跟踪差异及48项未跟踪文件，共164个路径；加入本页后候选为165个唯一路径。计数按 Git 原路径与归档目标分别计入，不等于 rename 识别后的文件数，也不是按数量拆组。

当前事实：T3 已同步 renderer 主规格并归档；T4 已获本机范围整体验收批准、同步五份主规格并归档，验收后的生产源码及必要构建/测试支持已迁入 F 主工作区。保留 SimulationSession、simulation/physics 内部 CPU target、实例 Application 和现有三后端路径；正式顶层 physics 模块、EnTT 生产替换和 T5 整体验收均未因此完成。

T3/T4 归档中的“当时 F 仍为 T3/仅同步文档”等段落保留原历史时点，不删除或改写旧命令与结果；当前源码状态由后续同步说明、AGENTS/README 及实际源码判断。源码同步不等于 Git 历史合并。

## 顺序与依赖

| 组 | 拟提交消息 | 交付意图与依赖 |
| --- | --- | --- |
| G1 | `build(msvc): 切换VS2026工具链并校验编译缓存与运行库` | 固定正式 CLI 工具链、SDK 和匹配 CRT，保留旧缓存；包含本页 |
| G2 | `fix(benchmark): 修正YAML发布替换策略并覆盖竞争失败` | 独立工具发布语义及真实回归，不恢复 foundation Fix1；排在 G1 后 |
| G3 | `feat(t4): 交付应用模拟生命周期并同步归档T3与T4规格` | 依赖 G1/G2，原子交付 T4 必需生产/测试闭包、机制、现役规格、批准导航及两份完整归档 |
| G4 | `docs(t5): 补齐EnTT前置核对与待审冻结设计` | 依赖 G3 交付身份及归档路径；仅保存已有前置实验结论/DBG01/待审稿，不实施新 ECS |

G3 不再按 app/platform/simulation/telemetry 或源码/测试任意切碎：Runtime/Window、Session/physics、Sealed 导出、应用清理与原生探针存在真实调用与链接依赖。T4 的限定旧 ECS/allocator 安全修补是 T4 装配前提，必须随 G3 交付，不误归为 T5 EnTT 重构。

G3 将 T3/T4 主规格同步、原目录移除、完整归档目标和所有相关链接放在同组，避免出现“主规格声明新行为但实现缺失”或文档指向不存在归档的中间提交。两份归档含相对链接调整、T4 新结果/批准等内容，不宣称全部只是原字节 rename。

## G1 正式工具链与运行库

完整文件：
```text
cmake/ValidateMsvcEnvironment.cmake
scripts/README.md
scripts/build.ps1
scripts/build.py
scripts/package-d3d12-candidate.py
scripts/verification_common.py
test/scripts/test_developer_scripts.py
docs/git/gitspec.md
```

另从 `game/app/CMakeLists.txt` **只取 CRT 识别块**：以 `MSVC_TOOLSET_VERSION` 设置 `SYMOCRAFT_MSVC_REDIST_NAME`，检查匹配 `Microsoft.${SYMOCRAFT_MSVC_REDIST_NAME}.CRT`。保留其余 HEAD 内容；不要提前带入 benchmark_scenario、symocraft_application、链接可见性和后端宏 owner 迁移。缺少这一块会让新 MSVC 被旧 VC143 候选检查拒绝，不能只提交 Python 默认版本。

本组共9个暂存路径：8份完整文件加1份共享文件局部。build/verification helper 固定 VS2026/MSVC tools14.51.36231、SDK10.0.22621.0，核对实际环境；换编译器拒绝旧缓存、不自动删除；候选打包按实际 runtime family 核对。scripts/README 同组交付，AGENTS/根 README 含 T4 状态，留 G3 整份提交，不在 G1 预先宣称 T4 已存在于该中间提交。

执行时最小检查：Python 开发脚本定向回归；CRT 检查及打包参数/实际选中 runtime family 对账；可用环境下用全新目录作当前中间提交的配置/定向构建。未执行的打包/编译明确记未执行，不以 E 目录既有产物证明 G1 中间树通过。

## G2 独立 Benchmark 发布修补

完整文件：
```text
tools/benchmark/src/platform.cpp
tools/benchmark/src/platform.h
test/benchmark/CMakeLists.txt
test/benchmark/publication_tests.cpp
```

WriteYaml 在 close 成功后先 ReplaceFileW(flags=0)，仅 ERROR_FILE_NOT_FOUND 才尝试一次 MoveFileExW(flags=0) 首次发布，不覆盖竞态中新建目标；其他错误保留原生码/操作/路径，不自动 fallback、重试、先删目标或清临时文件。私有 API 绑定服务确定性测试，正常运行仍使用真实 API。

仅涉及独立 benchmark_core 及其测试注册，不改 foundation 文件发布，不宣称历史1175根因关闭。机制文档 `docs/architecture/benchmark.md` 同时改了 T3 归档链接，整份留 G3；本组实现与回归测试先独立闭合。

执行时最小检查：独立工具 Release 定向构建与 `benchmark.publication`，受影响 executor/modern/vulkan 消费者回归；关闭测试检查无根测试实现依赖。当前历史执行记录不当作本次重跑。

## G3 T4 完整交付与已完成规格归档

完整意图：
- 限定旧 ECS 初始化/注册/扩容/代数读取和公共 Allocate/ReAlloc 失败清理修补，保留 POD、旧通用 Remove 等未完成 T5 边界；私有 allocation diagnostics 与真实生产库测试一并交付。
- Runtime/Window 明确 owner、自有 InputFrame 与窗口事实；Session 独立预算、暂停恢复、自有结果、失败/部分提交；内部 physics CPU target、World/ECS 适配与真实公开消费者。
- 实例 Application、一次 Run、有界关闭及首错报告；GameRenderer 消费 PlayerView；场景策略分离，telemetry Recording→Sealed 与 Export 独立结果。
- 生产 target/include 闭包、窄私有测试授权、CPU/公开头/正负边界；D3D12/Vulkan/GL 旧探针及增量检查适配新 owner/输入/镜头，补受影响缓存失败和容量回归。不得独立先提交改签名或只改测试期望。
- T3 renderer 现役规格、T4 platform/simulation 增量及 application-lifecycle/physics-core/telemetry-lifecycle 新主规格；完整 T3/T4 归档、批准/导航、范围、机制和入口同步。暂停 Fix1 proposal 只重定位 T3/T4 链接，不恢复该任务。

准确路径如下，共151项；18项旧 T3/T4 路径为删除，18项对应归档路径为新增，二者必须配对。同组含3项新主规格文件。

```text
AGENTS.md
README.md
cmake/ModuleBoundaries.cmake
cmake/PublicHeaderTests.cmake
docs/README.md
docs/architecture/Overview.md
docs/architecture/app.md
docs/architecture/benchmark.md
docs/architecture/ecs.md
docs/architecture/engineering.md
docs/architecture/foundation.md
docs/architecture/platform.md
docs/architecture/renderer.md
docs/architecture/simulation.md
docs/architecture/telemetry.md
docs/architecture/world.md
docs/openspec/changes/archive/2026-10-19-m3-t3-renderer/.openspec.yaml
docs/openspec/changes/archive/2026-10-19-m3-t3-renderer/decisions.md
docs/openspec/changes/archive/2026-10-19-m3-t3-renderer/design.md
docs/openspec/changes/archive/2026-10-19-m3-t3-renderer/proposal.md
docs/openspec/changes/archive/2026-10-19-m3-t3-renderer/specs/renderer/spec.md
docs/openspec/changes/archive/2026-10-19-m3-t3-renderer/tasks.md
docs/openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md
docs/openspec/changes/archive/2026-10-19-m3-t4-application-simulation/.openspec.yaml
docs/openspec/changes/archive/2026-10-19-m3-t4-application-simulation/decisions.md
docs/openspec/changes/archive/2026-10-19-m3-t4-application-simulation/design.md
docs/openspec/changes/archive/2026-10-19-m3-t4-application-simulation/proposal.md
docs/openspec/changes/archive/2026-10-19-m3-t4-application-simulation/specs/application-lifecycle/spec.md
docs/openspec/changes/archive/2026-10-19-m3-t4-application-simulation/specs/physics-core/spec.md
docs/openspec/changes/archive/2026-10-19-m3-t4-application-simulation/specs/platform/spec.md
docs/openspec/changes/archive/2026-10-19-m3-t4-application-simulation/specs/simulation/spec.md
docs/openspec/changes/archive/2026-10-19-m3-t4-application-simulation/specs/telemetry-lifecycle/spec.md
docs/openspec/changes/archive/2026-10-19-m3-t4-application-simulation/tasks.md
docs/openspec/changes/archive/2026-10-19-m3-t4-application-simulation/verification.md
docs/openspec/changes/m3-t2-publication-fix1/proposal.md
docs/openspec/changes/m3-t3-renderer/.openspec.yaml
docs/openspec/changes/m3-t3-renderer/decisions.md
docs/openspec/changes/m3-t3-renderer/design.md
docs/openspec/changes/m3-t3-renderer/proposal.md
docs/openspec/changes/m3-t3-renderer/specs/renderer/spec.md
docs/openspec/changes/m3-t3-renderer/tasks.md
docs/openspec/changes/m3-t3-renderer/verification.md
docs/openspec/changes/m3-t4-application-simulation/.openspec.yaml
docs/openspec/changes/m3-t4-application-simulation/decisions.md
docs/openspec/changes/m3-t4-application-simulation/design.md
docs/openspec/changes/m3-t4-application-simulation/proposal.md
docs/openspec/changes/m3-t4-application-simulation/specs/application-lifecycle/spec.md
docs/openspec/changes/m3-t4-application-simulation/specs/physics-core/spec.md
docs/openspec/changes/m3-t4-application-simulation/specs/platform/spec.md
docs/openspec/changes/m3-t4-application-simulation/specs/simulation/spec.md
docs/openspec/changes/m3-t4-application-simulation/specs/telemetry-lifecycle/spec.md
docs/openspec/changes/m3-t4-application-simulation/tasks.md
docs/openspec/changes/m3-t4-application-simulation/verification.md
docs/openspec/changes/m3-t5-ecs-memory/decisions.md
docs/openspec/changes/m3-t5-ecs-memory/design.md
docs/openspec/changes/m3-t5-ecs-memory/proposal.md
docs/openspec/config.yaml
docs/openspec/specs/application-lifecycle/spec.md
docs/openspec/specs/physics-core/spec.md
docs/openspec/specs/platform/spec.md
docs/openspec/specs/renderer/spec.md
docs/openspec/specs/simulation/spec.md
docs/openspec/specs/telemetry-lifecycle/spec.md
docs/scope/future-scope.md
docs/scope/project-scope.md
docs/verification/VERIFICATION.md
game/app/CMakeLists.txt
game/app/src/application.cpp
game/app/src/application.h
game/app/src/application_probe.h
game/app/src/benchmark_scenario.cpp
game/app/src/benchmark_scenario.h
game/app/src/game_entry.cpp
game/app/src/game_renderer.cpp
game/app/src/game_renderer.h
game/app/src/render_conversion.cpp
game/app/src/render_conversion.h
game/app/src/shutdown_report.h
game/modules/ecs/include/symocraft/ecs/registry.h
game/modules/ecs/src/component_container.h
game/modules/ecs/src/internal.cpp
game/modules/ecs/src/registry.cpp
game/modules/foundation/include/symocraft/foundation/memory.h
game/modules/foundation/src/allocation_diagnostics/allocation_probe.h
game/modules/foundation/src/legacy/AmoBase.cpp
game/modules/platform/include/symocraft/platform/window.h
game/modules/platform/src/input_state.h
game/modules/platform/src/window.cpp
game/modules/renderer/src/backend/vulkan/vulkan_backend.cpp
game/modules/renderer/src/legacy_renderer.cpp
game/modules/simulation/CMakeLists.txt
game/modules/simulation/include/symocraft/simulation/physics_system.h
game/modules/simulation/include/symocraft/simulation/player.h
game/modules/simulation/include/symocraft/simulation/player_math.h
game/modules/simulation/include/symocraft/simulation/playercontroller.h
game/modules/simulation/include/symocraft/simulation/session.h
game/modules/simulation/physics/CMakeLists.txt
game/modules/simulation/physics/include/symocraft/physics/physics.h
game/modules/simulation/physics/src/physics.cpp
game/modules/simulation/src/diagnostics/session_diagnostics.h
game/modules/simulation/src/physics_adapter.cpp
game/modules/simulation/src/physics_adapter.h
game/modules/simulation/src/physics_system.cpp
game/modules/simulation/src/player.cpp
game/modules/simulation/src/playercontroller.cpp
game/modules/simulation/src/session.cpp
game/modules/telemetry/include/symocraft/telemetry/performance.h
game/modules/telemetry/src/export_diagnostics/export_probe.h
game/modules/telemetry/src/performance.cpp
test/integration/CMakeLists.txt
test/integration/CheckNegativeBoundaries.cmake
test/integration/renderer/CheckRendererIncrementalBuild.py
test/integration/renderer/d3d12_backend_smoke.cpp
test/integration/renderer/d3d12_fault_probe.cpp
test/integration/renderer/d3d12_game_fault_probe.cpp
test/integration/renderer/d3d12_lifetime_async_probe.cpp
test/integration/renderer/d3d12_srv_cache_gpu_probe.cpp
test/integration/renderer/draw_window_probe.cpp
test/integration/renderer/layer_summary_gpu_probe.cpp
test/integration/renderer/verify-vulkan-lifetime-faults.py
test/integration/renderer/vulkan_backend_smoke.cpp
test/integration/renderer/vulkan_frame_bindings_gpu_probe.cpp
test/integration/renderer/vulkan_lifetime_fault_probe.cpp
test/integration/renderer/vulkan_wsi_probe.cpp
test/scripts/test_migrated_scripts.py
test/scripts/test_renderer_vulkan_lifetime_fault.py
test/support/sealed_export.h
test/unit/app/CMakeLists.txt
test/unit/app/application_probe.cpp
test/unit/app/scenario_tests.cpp
test/unit/ecs/CMakeLists.txt
test/unit/ecs/registry_allocation_tests.cpp
test/unit/ecs/registry_tests.cpp
test/unit/foundation/CMakeLists.txt
test/unit/foundation/memory_tests.cpp
test/unit/platform/CMakeLists.txt
test/unit/platform/input_frame_tests.cpp
test/unit/platform/runtime_probe.cpp
test/unit/renderer/d3d12_srv_cache_basic_tests.cpp
test/unit/renderer/vulkan_frame_bindings_basic_tests.cpp
test/unit/simulation/CMakeLists.txt
test/unit/simulation/cpu_baseline_fixture.h
test/unit/simulation/legacy_gameplay_baseline_tests.cpp
test/unit/simulation/legacy_physics_baseline_tests.cpp
test/unit/simulation/physics_core_consumer.cpp
test/unit/simulation/physics_core_tests.cpp
test/unit/simulation/session_tests.cpp
test/unit/telemetry/CMakeLists.txt
test/unit/telemetry/lifecycle_tests.cpp
test/unit/telemetry/performance_tests.cpp
test/unit/telemetry/renderer_performance_tests.cpp
```

其中两类共享文件不能整文件暂存：
1. `game/app/CMakeLists.txt`：仅提交 G1 之后剩余的 T4 target/source/链接边界改造；G1 已交付的 CRT 行不得遗漏或反向覆盖。
2. T5 的 `proposal.md`、`design.md`、`decisions.md`：本组只从既有 HEAD 内容作 T3/T4 相对链接重定位，指向 `../archive/2026-10-19-m3-t3-renderer/` 或 `../archive/2026-10-19-m3-t4-application-simulation/`，其余新规划留 G4。禁止在此带入 DBG01、S01.4 草稿或新增 verification 引用；`tasks.md` 当前只有进度勾选差异，整份留 G4。

归档后的主规格与源码一并落地，但不重跑 sync/archive；保留各 .openspec.yaml、原始证据/包及失败。README/AGENTS 的新工具链和 T4 现状在此完整成立；OpenSpec config 的旧时点说明由最后当前状态覆盖，不能取旧句反推当前未迁移。

执行时最小检查：先核对准确暂存 blob 与本轮源文件；必要 Debug/Release、CPU-only、公开消费者/正负边界、关闭测试、独立工具及受影响原生探针定向构建/回归按实际环境执行。GPU/人工/正式性能不默认追加；原有阶段批准不等于当前拆分提交已重新验证。

文档检查覆盖现役规格、两份完整归档、保留 requirement/scenario、导航及 T5 被重定位的链接。不得只暂存归档目录而漏旧路径删除，或只暂存主规格而漏生产实现/结果来源。

## G4 T5 前置核对与待审设计

在 G3 仅重定位链接的版本之上提交以下文件的其余差异：
```text
docs/openspec/changes/m3-t5-ecs-memory/decisions.md
docs/openspec/changes/m3-t5-ecs-memory/design.md
docs/openspec/changes/m3-t5-ecs-memory/proposal.md
docs/openspec/changes/m3-t5-ecs-memory/tasks.md
docs/openspec/changes/m3-t5-ecs-memory/verification.md
```

交付 S01.2/S01.3 的有限前置核对、EnTT 候选身份、DBG01 统一 Debug ABI 方向，以及身份批次/高水位和 const/名称/查询公开重载定向结果与 S01.4 待审稿。当前 tasks 只完成2/26，S01.1/S01.4 和生产任务仍未完成，不把候选实验当新 Registry 已交付或正式冻结。

`_ITERATOR_DEBUG_LEVEL=0` 只是已批准的 T5 配置方向及隔离实验条件，本批没有相应生产配置替换；普通 Debug level2 的历史 terminate 不改判。独立实验源码、下载依赖与产物仍在 E 的 refactor-ecs-memory 工作树，不因 F 新增 verification.md 就视为已纳入此提交或可独立复现；本组不自动跨工作树复制、提交 vendor/实验或修改生产代码。如需完整实验源交付，另确认准确范围。

执行时核对现有 T5 change 严格校验、待审稿/任务/验证结果一致性与归档链接，明确未重跑实验。提交授权前 T5 proposal/design 已补充 T3/T4 批准归档及 F 已取得 T4 源码的当前状态，保留 S01.1 自身输入/ABI/权限和 S01.4 冻结门槛；本轮核对并保存这些修订，不重复改写其方案或把旧实验树当最新生产输入。

## 暂存方法与并发保护

- 执行前重新核对 HEAD、索引、路径及文件身份；新的并发改动不自动进入本批。当前165路径只定义本轮候选，不允许 `git add .` 或盲目 `git add -A`。
- G1/G3/G4 涉及共享文件时，从所需父提交构造审核过的局部补丁/blob 进入索引；不改写、stash、reset 或覆盖工作区来制造中间版本，不使用交互式全选。
- 检查每组准确 staged name/status/mode/blob 及完整 diff、依赖闭包、冲突标记/意外凭据和 `git diff --cached --check`，通过后才在当次明确授权范围内 commit。校验失败、输入变化或超范围时停止该组并报告，不擅自吸收。
- 本页完整计划随 G1 提交，以父/当前 Git 对象定位，不回填循环 hash。后续实际结果在聊天报告；必要更新本页须明确区别执行后事实与原计划，不追加旧批次历史。
- 保留个人 `docs/development-journal/`、`docs/.obsidian/` 和被忽略的 `.agents/`，不读取个人正文或重新纳入跟踪；旧冻结证据、out、原缓存、运行包及未确认残留不动。四组之外没有另行清场。

## 验证事实与保留风险

规划轮只做只读盘点、关键源码/构建差异与依赖核对、本页编写及空白/路径覆盖检查，未作 Git 写入或工程测试。后续本轮已获得四组本地提交授权，需针对实际暂存中间树检查；历史验证不自动变成本批成绩。不追加 GPU/人工/正式性能或重新同步归档，不宣称所有计划检查已经通过。

历史批准及来源：T3 2026-10-17 R4/T3 批准与 T4 2026-10-19 整体批准均限定已记录的本机候选；T4 的既有 VS2026 游戏 Debug95/95、Release96/96、CPU Release85/85与CPU Debug83/84（foundation.files/1175失败，world.allocations另禁用1）各保留原身份，不能宣称本次所有中间提交测试全绿。既有文档审计 F17/E53、platform 长条款 strict 警告、旧失败/失焦/尖峰、未完整跨工具链复验、CLion实际选择及长时/中低端/真正干净机等不新增通过结论。

保留 M3-I1/I2、Win32 1175、暂停 Fix1、正式 physics 搬迁及 T5/M3 未完成范围；限定 Benchmark 发布修补不关闭 foundation 发布风险。全批不删除 GL、不改默认 D3D12、不暗加 EnTT 或其他依赖、不规范化全库换行、不修改凭据/hooks/全局 Git 设置，也不创建/合并 PR。
