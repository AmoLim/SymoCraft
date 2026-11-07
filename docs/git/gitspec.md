---
tags:
  - area/meta
---

# RenderDoc / Tracy 交付提交计划

2026-11-07 用户明确通过 `integrate-renderdoc-tracy` 整体验收并授权同步主规格，随后指定将完整交付提交到目标分支，先审阅本计划。实施来源为 `G:/Sandbox/Worktrees/integrate-renderdoc-tracy/Symocraft`。目标为主工作区 `G:/Projects/Symocraft` 当前检出的 `feature-integrate-renderdoc-tracy`，两处盘点基线均为 `fd09c947106c686755c269eebd4e4e504ee870d8`。

## 拟提交

| 顺序 | 拟提交消息 | 完整意图 | 最终路径归属 |
| --- | --- | --- | --- |
| G1 | `chore(deps): 固定 Tracy 0.14.1 源码和许可证` | 固定 Tracy 官方源码和许可证，保留 Tracy 与 EnTT 许可字节。 | 90（A 89 / M 1 / D 0） |
| G2a | `refactor(foundation): 迁移 YAML 文档适配` | 将 YAML 编解码移到 foundation 的独立 target，同步所有仍在用的调用方和文档测试。 | 7（A 2 / M 3 / D 2） |
| G2b | `feat(profiling): 接入可选 Tracy 与 RenderDoc 渲染标记` | 添加可选薄宏、构建开关、独立 CPU 区域及 D3D12/Vulkan 原生命名和事件组。 | 18（A 10 / M 8 / D 0） |
| G2c | `feat(profiling): 切换独立诊断场景并退役旧性能链` | 切换独立 static/walk/edit、轻量快照和应用帧观测，退役旧 benchmark、Session、GPU timing query 与 Publish，同步构建、安装及测试迁移。 | 133（A 5 / M 72 / D 56） |
| G2d | `fix(test): 恢复 Debug 世界分配测试` | 修正 Debug ABI 的过时判定，让 world.allocations 重新注册为启用测试。 | 1（A 0 / M 1 / D 0） |
| G3 | `docs(profiling): 同步主规格和整体验收记录` | 同步七份主规格、验收与操作文档，提交 Fix1 的取消归档、导航和本计划。 | 47（A 8 / M 32 / D 7） |

G1 提供固定依赖。G2a 独立迁移 YAML 适配。G2b 启用可选工具接线，旧采集链继续可用。G2c 一次切换应用、渲染接口、导出链及关联测试，Windows SDK 10.0.28000.0 的默认构建约定也随该笔落地。G2d 独立验证现有 DBG01 ABI 下的分配故障测试。G3 同步文档与主规格，作为完整文档审计点。

最终树仍为 **296 个路径**，A 114、M 117、D 65。表中按从基线到最终树的状态统计，每个路径只计入最后完成它的一笔。前置提交需要修改同一路径时，以后文的跨提交清单补充。59 个产品退役路径分布在 G2a、G2c 和 G3。G3 另将 Fix1 的六份文件移到已授权归档位置，保留 14 项未完成任务。EnTT 原许可证 blob 与 HEAD 相同。

验收范围见 [整体验收记录](../openspec/changes/integrate-renderdoc-tracy/verification.md#2026-11-07-整体验收通过与主规格同步)。此前八种配置构建、累计 758 项测试、两后端的 Tracy 与 RenderDoc 检查绑定原验收候选。本轮更新的是提交划分。

## 主工作区保留项

- 主工作区索引里已有 NPC change 的两份文件和一份个人日志。三项排除在本次提交外，复核其暂存 blob 与状态保持。
- `restore-windows-development-environment`、`.python-version`、`pyproject.toml`、`uv.lock` 及主工作区现有 `.gitignore` 改动属于其他工作。保留在原位置和原状态。
- 两个共享文件需按差异合并。主工作区 `CMakePresets.json` 的 `.venv/Scripts/python.exe` 路径，以及 `game/app/CMakeLists.txt` 从选定 SDK 显式收集 Debug UCRT 的改动，保留在主工作区的未提交层。G2a–G2d 的最终实现以已验收的工作树版本为准。
- 已复制到主工作区的旧审阅稿需逐项与新版本核对，保留仅存在于主工作区的独有事实。生成的 `docs/tag-inventory.json` 需要区分分支候选与主工作区本地视图，主工作区的个人登记以及其他 change 的索引入口继续保留。
- `out/`、本机工具、安装包、捕获轨迹、缓存、个人日志、Obsidian 和冻结归档均在批次之外。当前 change 目录保留在活动区。

## 跨提交文件与中间态

下表是最终路径清单之外的前置改动。按改动内容组装提交，不能用整文件复制提前带入后续改动。

| 路径 | 先提交的内容 | 后续归属 |
| --- | --- | --- |
| `cmake/ModuleBoundaries.cmake` | G2a 授权独立 YAML target 及私有依赖，G2b 加 profiling 和唯一 Tracy client 的边界 | G2c 移除旧渲染源的边界登记 |
| `game/modules/foundation/CMakeLists.txt`、`game/modules/telemetry/CMakeLists.txt` | G2a 建立独立 YAML target，旧 telemetry 私有链接该适配，保留 Publish 和旧性能实现 | G2c 移除 Publish 源和旧 telemetry，接入快照 |
| `game/app/CMakeLists.txt` | G2a 增加 YAML target 链接 | G2b 添加 profiling 安装符号 |
| `game/app/src/application.cpp`、`game/app/src/game_renderer.cpp`、`test/unit/app/application_probe.cpp` | G2a 只迁移 YAML include | G2c 切换运行流程、计数和保留测试 |
| `test/integration/renderer/d3d12_fault_probe.cpp`、`test/integration/renderer/d3d12_lifetime_async_probe.cpp`、`test/integration/renderer/vulkan_wsi_probe.cpp`、`test/integration/renderer/vulkan_lifetime_fault_probe.cpp` | G2a 只迁移 YAML include | G2c 迁移旧采样、故障及寿命验证路径 |
| `game/modules/telemetry/src/performance.cpp`、`test/integration/renderer/game_report_probe.cpp` | G2a 将仍存活的旧导出和旧报表探针接到新 YAML 适配 | G2c 退役两个旧消费者 |
| `test/integration/CMakeLists.txt`、`test/unit/app/CMakeLists.txt`、`test/unit/foundation/CMakeLists.txt` | G2a 接线文档消费者和往返测试，旧报表探针先链接新适配 | G2c 移除旧消费者及其注册、接入迁移后的检查 |
| `CMakeLists.txt`、`test/CMakeLists.txt` | G2b 加可选 Tracy、原生标记、符号配置、CPU-only 隔离和 profiling 测试，旧 benchmark 和旧测试注册继续生效 | G2c 退役旧入口、配置和注册 |
| `game/modules/renderer/src/backend/d3d12/d3d12_backend.cpp`、`game/modules/renderer/src/backend/vulkan/vulkan_backend.cpp` | G2b 在既有提交、呈现和资源路径加入 CPU 区域与原生注释，保留旧计时、查询和同步 | G2c 移除旧计时与消费者，保留正确性同步 |

## 提交前核对

1. 复核分支 HEAD、两处工作区状态及准确清单。核对主工作区共享路径、59 个退役路径和 Fix1 的 6 个旧位置，保留主工作区独有修改。
2. 从目标 HEAD 用隔离索引依次组装六笔，逐笔审阅相对父提交的 diff、模式和关键 blob。G1 核对 vendor 与许可字节。G2a 检查单一 YAML owner、公开头、文档往返和仍在的旧导出。G2b 检查工具开关、缺依赖拒绝、默认构建、双后端和原生注释，同时确认旧链仍可构建。
3. G2c 验证受影响的构建、测试、安装和旧入口退役。G2d 在四种 Debug 配置验证 world.allocations 已启用并实际通过。每笔使用适用工具链验证自己的中间树，早期候选使用明确的 SDK 选择。
4. G3 校验主规格、历史记录和文档链接。当前操作入口使用仓库相对链接，历史路径保留原候选身份。最终组合与已验收工作树逐路径对账，记录文档或本地叠加差异。历史 GPU 回放与人工结论沿用其原候选身份。
5. 审阅通过后依次提交至现有目标分支。逐笔对照提交清单，并核对主工作区本地差异与原三项暂存。当前状态为提交计划待审。

## G1 准确路径 · 固定 Tracy 0.14.1 源码和许可证

```text
M  .gitattributes
A  vendor/tracy/LICENSE
A  vendor/tracy/public/TracyClient.F90
A  vendor/tracy/public/TracyClient.cpp
A  vendor/tracy/public/client/TracyAlloc.cpp
A  vendor/tracy/public/client/TracyArmCpuTable.hpp
A  vendor/tracy/public/client/TracyCallstack.cpp
A  vendor/tracy/public/client/TracyCallstack.h
A  vendor/tracy/public/client/TracyCallstack.hpp
A  vendor/tracy/public/client/TracyCpuid.hpp
A  vendor/tracy/public/client/TracyDebug.hpp
A  vendor/tracy/public/client/TracyDxt1.cpp
A  vendor/tracy/public/client/TracyDxt1.hpp
A  vendor/tracy/public/client/TracyElf.hpp
A  vendor/tracy/public/client/TracyFastVector.hpp
A  vendor/tracy/public/client/TracyKCore.cpp
A  vendor/tracy/public/client/TracyKCore.hpp
A  vendor/tracy/public/client/TracyLock.hpp
A  vendor/tracy/public/client/TracyMangle.hpp
A  vendor/tracy/public/client/TracyOverride.cpp
A  vendor/tracy/public/client/TracyProfiler.cpp
A  vendor/tracy/public/client/TracyProfiler.hpp
A  vendor/tracy/public/client/TracyRingBuffer.hpp
A  vendor/tracy/public/client/TracyRocprof.cpp
A  vendor/tracy/public/client/TracyScoped.hpp
A  vendor/tracy/public/client/TracyStringHelpers.hpp
A  vendor/tracy/public/client/TracySysPower.cpp
A  vendor/tracy/public/client/TracySysPower.hpp
A  vendor/tracy/public/client/TracySysTime.cpp
A  vendor/tracy/public/client/TracySysTime.hpp
A  vendor/tracy/public/client/TracySysTrace.cpp
A  vendor/tracy/public/client/TracySysTrace.hpp
A  vendor/tracy/public/client/TracyThread.hpp
A  vendor/tracy/public/client/apple/TracyMach.cpp
A  vendor/tracy/public/client/tracy_SPSCQueue.h
A  vendor/tracy/public/client/tracy_concurrentqueue.h
A  vendor/tracy/public/client/tracy_rpmalloc.cpp
A  vendor/tracy/public/client/tracy_rpmalloc.hpp
A  vendor/tracy/public/client/windows/TracyETW.cpp
A  vendor/tracy/public/client/windows/TracyETW_compat.h
A  vendor/tracy/public/common/TracyAlign.hpp
A  vendor/tracy/public/common/TracyAlloc.hpp
A  vendor/tracy/public/common/TracyApi.h
A  vendor/tracy/public/common/TracyAssert.hpp
A  vendor/tracy/public/common/TracyColor.hpp
A  vendor/tracy/public/common/TracyForceInline.hpp
A  vendor/tracy/public/common/TracyFormat.h
A  vendor/tracy/public/common/TracyMutex.hpp
A  vendor/tracy/public/common/TracyProtocol.hpp
A  vendor/tracy/public/common/TracyQueue.hpp
A  vendor/tracy/public/common/TracySocket.cpp
A  vendor/tracy/public/common/TracySocket.hpp
A  vendor/tracy/public/common/TracyStackFrames.cpp
A  vendor/tracy/public/common/TracyStackFrames.hpp
A  vendor/tracy/public/common/TracyString.hpp
A  vendor/tracy/public/common/TracySystem.cpp
A  vendor/tracy/public/common/TracySystem.hpp
A  vendor/tracy/public/common/TracyTaggedUserlandAddress.hpp
A  vendor/tracy/public/common/TracyVersion.hpp
A  vendor/tracy/public/common/TracyWinFamily.hpp
A  vendor/tracy/public/common/TracyYield.hpp
A  vendor/tracy/public/common/tracy_lz4.cpp
A  vendor/tracy/public/common/tracy_lz4.hpp
A  vendor/tracy/public/common/tracy_lz4hc.cpp
A  vendor/tracy/public/common/tracy_lz4hc.hpp
A  vendor/tracy/public/libbacktrace/LICENSE
A  vendor/tracy/public/libbacktrace/alloc.cpp
A  vendor/tracy/public/libbacktrace/backtrace.hpp
A  vendor/tracy/public/libbacktrace/config.h
A  vendor/tracy/public/libbacktrace/dwarf.cpp
A  vendor/tracy/public/libbacktrace/elf.cpp
A  vendor/tracy/public/libbacktrace/fileline.cpp
A  vendor/tracy/public/libbacktrace/filenames.hpp
A  vendor/tracy/public/libbacktrace/internal.hpp
A  vendor/tracy/public/libbacktrace/macho.cpp
A  vendor/tracy/public/libbacktrace/mmapio.cpp
A  vendor/tracy/public/libbacktrace/posix.cpp
A  vendor/tracy/public/libbacktrace/sort.cpp
A  vendor/tracy/public/libbacktrace/state.cpp
A  vendor/tracy/public/tracy/Tracy.hpp
A  vendor/tracy/public/tracy/TracyC.h
A  vendor/tracy/public/tracy/TracyCUDA.hpp
A  vendor/tracy/public/tracy/TracyD3D11.hpp
A  vendor/tracy/public/tracy/TracyD3D12.hpp
A  vendor/tracy/public/tracy/TracyLua.hpp
A  vendor/tracy/public/tracy/TracyMetal.hmm
A  vendor/tracy/public/tracy/TracyOpenCL.hpp
A  vendor/tracy/public/tracy/TracyOpenGL.hpp
A  vendor/tracy/public/tracy/TracyVulkan.hpp
A  vendor/tracy/public/tracy/TracyWebGPU.hpp
```

## G2a 最终路径归属 · 迁移 YAML 文档适配

```text
M  cmake/PublicHeaderTests.cmake
A  game/modules/foundation/include/symocraft/foundation/document_io.h
A  game/modules/foundation/src/document_io.cpp
D  game/modules/telemetry/include/symocraft/telemetry/document_io.h
D  game/modules/telemetry/src/document_io.cpp
M  test/integration/renderer/d3d12_game_fault_probe.cpp
M  test/unit/foundation/document_tests.cpp
```

## G2b 最终路径归属 · 接入可选 Tracy 与 RenderDoc 渲染标记

```text
M  CMakePresets.json
M  cmake/licenses/SOURCES.txt
M  game/CMakeLists.txt
M  game/app/CMakeLists.txt
A  game/modules/profiling/CMakeLists.txt
A  game/modules/profiling/include/symocraft/profiling/profiling.h
A  game/modules/renderer/src/backend/d3d12/debug_markers.h
A  game/modules/renderer/src/backend/vulkan/debug_markers.h
M  game/modules/simulation/src/character_system.cpp
M  game/modules/simulation/src/physics_adapter.cpp
M  game/modules/simulation/src/session.cpp
M  game/modules/world/src/world.cpp
A  test/unit/profiling/CMakeLists.txt
A  test/unit/profiling/check_configuration.py
A  test/unit/profiling/gpu_markers_off_tests.cpp
A  test/unit/profiling/profiling_other.cpp
A  test/unit/profiling/profiling_tests.cpp
A  test/unit/profiling/vulkan_debug_markers_tests.cpp
```

## G2c 最终路径归属 · 切换独立诊断场景并退役旧性能链

```text
M  AGENTS.md
M  CMakeLists.txt
M  cmake/ModuleBoundaries.cmake
M  game/app/src/application.cpp
M  game/app/src/application.h
M  game/app/src/application_probe.h
M  game/app/src/benchmark_scenario.cpp
D  game/app/src/foreground_readiness.h
M  game/app/src/game_entry.cpp
M  game/app/src/game_renderer.cpp
M  game/app/src/game_renderer.h
M  game/app/src/shutdown_report.h
M  game/app/src/startup_options.cpp
M  game/app/src/startup_options.h
M  game/modules/foundation/CMakeLists.txt
D  game/modules/foundation/include/symocraft/foundation/files.h
D  game/modules/foundation/src/files.cpp
M  game/modules/platform/CMakeLists.txt
D  game/modules/platform/include/symocraft/platform/process_memory.h
D  game/modules/platform/src/process_memory.cpp
M  game/modules/renderer/CMakeLists.txt
M  game/modules/renderer/include/symocraft/renderer/renderer.h
M  game/modules/renderer/include/symocraft/renderer/types.h
D  game/modules/renderer/legacy/include/symocraft/renderer/gpu_timer.h
M  game/modules/renderer/legacy/include/symocraft/renderer/legacy_renderer.h
M  game/modules/renderer/legacy/include/symocraft/renderer/render_stats.h
M  game/modules/renderer/src/backend/d3d12/d3d12_backend.cpp
M  game/modules/renderer/src/backend/d3d12/diagnostic_access.h
M  game/modules/renderer/src/backend/render_backend.h
M  game/modules/renderer/src/backend/vulkan/diagnostic_access.h
M  game/modules/renderer/src/backend/vulkan/vulkan_backend.cpp
M  game/modules/renderer/src/batch.hpp
D  game/modules/renderer/src/frame_diagnostics.h
D  game/modules/renderer/src/gpu_timer.cpp
M  game/modules/renderer/src/legacy_renderer.cpp
D  game/modules/renderer/src/performance_memory.cpp
M  game/modules/renderer/src/renderer.cpp
M  game/modules/renderer/src/renderer_impl.cpp
M  game/modules/renderer/src/renderer_impl.h
M  game/modules/telemetry/CMakeLists.txt
A  game/modules/telemetry/include/symocraft/telemetry/debug_snapshot.h
D  game/modules/telemetry/include/symocraft/telemetry/performance.h
A  game/modules/telemetry/src/debug_snapshot.cpp
D  game/modules/telemetry/src/export_diagnostics/export_probe.h
D  game/modules/telemetry/src/performance.cpp
D  scripts/benchmark.ps1
D  scripts/benchmark.py
M  scripts/build.py
M  scripts/capture-migration-inputs.py
M  scripts/verify-runtime-faults.py
M  scripts/verify-runtime.ps1
M  scripts/verify-runtime.py
M  scripts/windows_verification.py
M  test/CMakeLists.txt
D  test/benchmark/CMakeLists.txt
D  test/benchmark/benchmark_script_tests.ps1
D  test/benchmark/benchmark_script_tests.py
D  test/benchmark/capture_probe.cpp
D  test/benchmark/publication_tests.cpp
D  test/benchmark/runner_tests.cpp
M  test/integration/CMakeLists.txt
M  test/integration/VerifyBuildMatrix.cmake
M  test/integration/VerifyMigrationRuntime.py
D  test/integration/inspect-sdl-delivery.cpp
M  test/integration/renderer/d3d12_backend_smoke.cpp
M  test/integration/renderer/d3d12_fault_probe.cpp
D  test/integration/renderer/d3d12_frame_timing_probe.cpp
M  test/integration/renderer/d3d12_lifetime_async_probe.cpp
M  test/integration/renderer/draw_window_probe.cpp
D  test/integration/renderer/frame_timing_schema.py
D  test/integration/renderer/game_report_probe.cpp
A  test/integration/renderer/gl_capture_probe.cpp
D  test/integration/renderer/test_budget_scripts.py
D  test/integration/renderer/test_d3d12_frame_timings.py
D  test/integration/renderer/test_foreground_comparison.py
D  test/integration/renderer/test_vulkan_frame_timings.py
D  test/integration/renderer/verify-d3d12-budgets.py
M  test/integration/renderer/verify-d3d12-candidate.py
D  test/integration/renderer/verify-d3d12-frame-timings.py
M  test/integration/renderer/verify-d3d12-game.py
M  test/integration/renderer/verify-d3d12-lifetime-async.py
D  test/integration/renderer/verify-renderer-comparison.py
M  test/integration/renderer/verify-renderer-configurations.py
D  test/integration/renderer/verify-vulkan-async.py
D  test/integration/renderer/verify-vulkan-frame-timings.py
M  test/integration/renderer/verify-vulkan-lifetime-faults.py
M  test/integration/renderer/verify-vulkan-wsi.py
M  test/integration/renderer/vulkan_backend_smoke.cpp
M  test/integration/renderer/vulkan_frame_bindings_gpu_probe.cpp
D  test/integration/renderer/vulkan_frame_timing_probe.cpp
M  test/integration/renderer/vulkan_lifetime_fault_probe.cpp
D  test/integration/renderer/vulkan_native_observation.py
M  test/integration/renderer/vulkan_wsi_probe.cpp
M  test/integration/verify-sdl-delivery.py
M  test/scripts/test_developer_scripts.py
M  test/scripts/test_migrated_scripts.py
M  test/scripts/test_renderer_d3d12_lifetime_async.py
D  test/scripts/test_renderer_vulkan_async.py
M  test/scripts/test_renderer_vulkan_lifetime_fault.py
M  test/scripts/test_renderer_vulkan_wsi.py
D  test/support/benchmark_fixture.cpp
D  test/support/benchmark_process_fixture.cpp
D  test/support/sealed_export.h
M  test/unit/app/CMakeLists.txt
M  test/unit/app/application_probe.cpp
D  test/unit/app/foreground_readiness_tests.cpp
A  test/unit/app/profile_scenario_probe.cpp
M  test/unit/app/scenario_tests.cpp
M  test/unit/app/startup_options_tests.cpp
M  test/unit/foundation/CMakeLists.txt
D  test/unit/foundation/files_tests.cpp
M  test/unit/renderer/CMakeLists.txt
M  test/unit/renderer/facade_tests.cpp
D  test/unit/renderer/frame_diagnostics_tests.cpp
D  test/unit/renderer/gpu_timer_tests.cpp
M  test/unit/renderer/layer_summary_basic_tests.cpp
M  test/unit/renderer/public_api_consumer.cpp
M  test/unit/telemetry/CMakeLists.txt
A  test/unit/telemetry/debug_snapshot_tests.cpp
D  test/unit/telemetry/lifecycle_tests.cpp
D  test/unit/telemetry/performance_tests.cpp
D  test/unit/telemetry/renderer_performance_tests.cpp
D  tools/benchmark/CMakeLists.txt
D  tools/benchmark/README.md
D  tools/benchmark/include/benchmark/runner.h
D  tools/benchmark/runner.manifest
D  tools/benchmark/src/gui.cpp
D  tools/benchmark/src/gui.h
D  tools/benchmark/src/main.cpp
D  tools/benchmark/src/platform.cpp
D  tools/benchmark/src/platform.h
D  tools/benchmark/src/runner.cpp
M  tools/openspec/check_documents.py
```

## G2d 最终路径归属 · 恢复 Debug 世界分配测试

```text
M  test/unit/world/CMakeLists.txt
```

## G3 准确路径 · 同步主规格和整体验收记录

```text
M  README.md
M  docs/README.md
M  docs/architecture/Overview.md
M  docs/architecture/app.md
D  docs/architecture/benchmark.md
M  docs/architecture/engineering.md
M  docs/architecture/foundation.md
M  docs/architecture/platform.md
M  docs/architecture/renderer.md
M  docs/architecture/telemetry.md
M  docs/architecture/world.md
M  docs/git/gitspec.md
M  docs/issue/issues.md
A  docs/openspec/changes/archive/2026-11-03-m3-t2-publication-fix1/.openspec.yaml
A  docs/openspec/changes/archive/2026-11-03-m3-t2-publication-fix1/design.md
A  docs/openspec/changes/archive/2026-11-03-m3-t2-publication-fix1/proposal.md
A  docs/openspec/changes/archive/2026-11-03-m3-t2-publication-fix1/specs/foundation-files/spec.md
A  docs/openspec/changes/archive/2026-11-03-m3-t2-publication-fix1/tasks.md
A  docs/openspec/changes/archive/2026-11-03-m3-t2-publication-fix1/verification.md
M  docs/openspec/changes/integrate-renderdoc-tracy/proposal.md
M  docs/openspec/changes/integrate-renderdoc-tracy/specs/development-profiling/spec.md
M  docs/openspec/changes/integrate-renderdoc-tracy/specs/foundation-files/spec.md
M  docs/openspec/changes/integrate-renderdoc-tracy/specs/module-boundaries/spec.md
M  docs/openspec/changes/integrate-renderdoc-tracy/specs/telemetry-lifecycle/spec.md
A  docs/openspec/changes/integrate-renderdoc-tracy/verification.md
D  docs/openspec/changes/m3-t2-publication-fix1/.openspec.yaml
D  docs/openspec/changes/m3-t2-publication-fix1/design.md
D  docs/openspec/changes/m3-t2-publication-fix1/proposal.md
D  docs/openspec/changes/m3-t2-publication-fix1/specs/foundation-files/spec.md
D  docs/openspec/changes/m3-t2-publication-fix1/tasks.md
D  docs/openspec/changes/m3-t2-publication-fix1/verification.md
M  docs/openspec/config.yaml
M  docs/openspec/specs/application-lifecycle/spec.md
A  docs/openspec/specs/development-profiling/spec.md
M  docs/openspec/specs/foundation-files/spec.md
M  docs/openspec/specs/module-boundaries/spec.md
M  docs/openspec/specs/platform/spec.md
M  docs/openspec/specs/renderer/spec.md
M  docs/openspec/specs/telemetry-lifecycle/spec.md
M  docs/openspec/specs/world/spec.md
M  docs/scope/project-scope.md
M  docs/tag-inventory.json
M  docs/third-party.md
M  docs/verification/VERIFICATION.md
M  docs/verification/methods.md
M  docs/verification/scenarios.md
M  scripts/README.md
```
