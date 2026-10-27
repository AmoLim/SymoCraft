---
tags:
  - area/meta
---

# ECS/Memory 交付与 T5 文档收口提交划分

## 授权与基线

2026-10-27 本轮请求为“主工作区 ECS-Memory-apply 分步提交，先写 gitspec”。当前只授权盘点及替换本页，尚未授权暂存、commit、push、merge 或创建 PR；旧批次批准不沿用。后续取得明确提交授权后，才按本页逐组执行。

分支 `feature-refactoring`，规划基线 HEAD 为 `6fea31e4631e267f132aa9b1608dc6538b562e13`，盘点时索引为空。排除个人目录后，当前有58个已跟踪差异路径及135个未跟踪路径，其中111个为 EnTT 原始依赖文件。排除另一个 NPC proposal 的2个路径，再加入本页，候选共192个唯一路径。旧路径删除与归档目标新增分别计数，不等于 Git rename 识别后的文件数。

当前事实以 [scope](../scope/project-scope.md)、[文档治理](../DOCUMENTATION_MAP.md)和 [T5 验证记录](../openspec/changes/archive/2026-10-25-m3-t5-ecs-memory/verification.md)为依据：T5/M3 已于2026-10-25获明确整体批准，ecs 与 module-boundaries 主规格已同步，T5 已归档；随后独立授权的生产传播已将 EnTT typed ECS、DBG01、调用方及构建/测试支持同步到 F 主工作区。此轮是保存这些既有改动，不重新实施、验收、同步规格或归档。

归档中“F 尚未同步”“T5 待批准”等是历史时点，后续批准和生产传播章节已明确覆盖当前状态；不得为统一措辞改写旧命令、失败、结果或批准含义。源码传播不等于合并 Git 历史。

## 分组顺序

| 组 | 拟提交消息 | 完整意图 | 路径数 |
| --- | --- | --- | --- |
| G1 | `build(ecs): 引入固定EnTT依赖并统一Debug ABI` | 固定原始依赖、许可证安装和全体 Debug C++ ABI；包含本页 | 118 |
| G2 | `refactor(ecs): 交付类型化注册表并迁移生产调用方` | ECS 实现、业务调用、必要构建边界与测试一起替换，退役准确五文件 | 37 |
| G3 | `fix(docs): 校验压缩证据成员并修正文档入口` | 独立保存已有文档审计修补、冻结材料读取规则和纯导航修正 | 6 |
| G4 | `docs(t5): 同步ECS主规格归档与M3验收状态` | 已完成 T5 的完整归档、主规格、机制、范围与批准导航收口 | 31 |

G2 依赖 G1。G3 是现有配套文档维护，不是 ECS 行为的一部分，也不以通过审计替代 ECS 验收；单独提交便于审阅或独立回退。G4 在 G2 后提交，使现役规格与当前交付状态有对应实现；同时收进 scope/third-party 中混合的压缩链接修正，与 G3 共同完成这批文档维护。按 G1 → G2 → G3 → G4 执行，不按目录或任意文件数量拆分。

所有路径均按当前差异整份归组，无共享文件局部暂存。以下清单是固定候选，不是目录通配授权；后续新增文件或相同路径的新并发差异不能自动纳入。

## G1 固定依赖与全局 Debug ABI

EnTT 固定为项目已经选定的官方 v4.0.0 提交 `85c6bba014049b5de8fad49d25424df2f1f6a8c1`；保存原始 src 与 LICENSE，不补丁第三方文件、不改用在线下载。独立 INTERFACE target 提供受控模板依赖和 `ENTT_NO_ETO`，不调用上游顶层 CMake。

`MsvcAbi.cmake` 在根构建和独立 benchmark 的依赖配置前统一 MSVC Debug C++ 的 `_ITERATOR_DEBUG_LEVEL=0`，保留 Debug CRT 和调试信息。这是已批准 DBG01 的实际配置，不是提高优化级别或取消 Debug。将来构建须使用全新目录并重建依赖闭包，不能混入旧 level-2 对象或库；本轮 Git 工作不因此启动构建或删除缓存。

许可证源声明与安装规则一起交付。此组只准备依赖，不提前切换 ECS；`game/modules/ecs/CMakeLists.txt` 的源码替换及公开链接必须随 G2，否则会从旧实现中提前移除仍需的 internal.cpp。

完整文件清单共118项，其中 EnTT 原始文件111项：

```text
CMakeLists.txt
cmake/MsvcAbi.cmake
cmake/ThirdPartyEnTT.cmake
cmake/licenses/SOURCES.txt
game/app/CMakeLists.txt
tools/benchmark/CMakeLists.txt
docs/git/gitspec.md
vendor/entt/LICENSE
vendor/entt/src/BUILD.bazel
vendor/entt/src/entt/config/config.h
vendor/entt/src/entt/config/macro.h
vendor/entt/src/entt/config/version.h
vendor/entt/src/entt/container/dense_map.hpp
vendor/entt/src/entt/container/dense_set.hpp
vendor/entt/src/entt/container/fwd.hpp
vendor/entt/src/entt/container/table.hpp
vendor/entt/src/entt/core/algorithm.hpp
vendor/entt/src/entt/core/any.hpp
vendor/entt/src/entt/core/bit.hpp
vendor/entt/src/entt/core/compressed_pair.hpp
vendor/entt/src/entt/core/concepts.hpp
vendor/entt/src/entt/core/enum.hpp
vendor/entt/src/entt/core/family.hpp
vendor/entt/src/entt/core/fwd.hpp
vendor/entt/src/entt/core/hashed_string.hpp
vendor/entt/src/entt/core/ident.hpp
vendor/entt/src/entt/core/iterator.hpp
vendor/entt/src/entt/core/memory.hpp
vendor/entt/src/entt/core/monostate.hpp
vendor/entt/src/entt/core/ranges.hpp
vendor/entt/src/entt/core/tuple.hpp
vendor/entt/src/entt/core/type_info.hpp
vendor/entt/src/entt/core/type_traits.hpp
vendor/entt/src/entt/core/utility.hpp
vendor/entt/src/entt/entity/component.hpp
vendor/entt/src/entt/entity/entity.hpp
vendor/entt/src/entt/entity/fwd.hpp
vendor/entt/src/entt/entity/group.hpp
vendor/entt/src/entt/entity/handle.hpp
vendor/entt/src/entt/entity/helper.hpp
vendor/entt/src/entt/entity/mixin.hpp
vendor/entt/src/entt/entity/organizer.hpp
vendor/entt/src/entt/entity/ranges.hpp
vendor/entt/src/entt/entity/registry.hpp
vendor/entt/src/entt/entity/runtime_view.hpp
vendor/entt/src/entt/entity/snapshot.hpp
vendor/entt/src/entt/entity/sparse_set.hpp
vendor/entt/src/entt/entity/storage.hpp
vendor/entt/src/entt/entity/view.hpp
vendor/entt/src/entt/entt.hpp
vendor/entt/src/entt/fwd.hpp
vendor/entt/src/entt/graph/adjacency_matrix.hpp
vendor/entt/src/entt/graph/dot.hpp
vendor/entt/src/entt/graph/flow.hpp
vendor/entt/src/entt/graph/fwd.hpp
vendor/entt/src/entt/locator/locator.hpp
vendor/entt/src/entt/meta/adl_pointer.hpp
vendor/entt/src/entt/meta/container.hpp
vendor/entt/src/entt/meta/context.hpp
vendor/entt/src/entt/meta/factory.hpp
vendor/entt/src/entt/meta/fwd.hpp
vendor/entt/src/entt/meta/meta.hpp
vendor/entt/src/entt/meta/node.hpp
vendor/entt/src/entt/meta/pointer.hpp
vendor/entt/src/entt/meta/policy.hpp
vendor/entt/src/entt/meta/range.hpp
vendor/entt/src/entt/meta/resolve.hpp
vendor/entt/src/entt/meta/template.hpp
vendor/entt/src/entt/meta/type_traits.hpp
vendor/entt/src/entt/meta/utility.hpp
vendor/entt/src/entt/natvis/config.natvis
vendor/entt/src/entt/natvis/container.natvis
vendor/entt/src/entt/natvis/core.natvis
vendor/entt/src/entt/natvis/entity.natvis
vendor/entt/src/entt/natvis/graph.natvis
vendor/entt/src/entt/natvis/locator.natvis
vendor/entt/src/entt/natvis/meta.natvis
vendor/entt/src/entt/natvis/poly.natvis
vendor/entt/src/entt/natvis/process.natvis
vendor/entt/src/entt/natvis/resource.natvis
vendor/entt/src/entt/natvis/signal.natvis
vendor/entt/src/entt/poly/fwd.hpp
vendor/entt/src/entt/poly/poly.hpp
vendor/entt/src/entt/process/fwd.hpp
vendor/entt/src/entt/process/process.hpp
vendor/entt/src/entt/process/scheduler.hpp
vendor/entt/src/entt/resource/cache.hpp
vendor/entt/src/entt/resource/fwd.hpp
vendor/entt/src/entt/resource/loader.hpp
vendor/entt/src/entt/resource/resource.hpp
vendor/entt/src/entt/signal/delegate.hpp
vendor/entt/src/entt/signal/dispatcher.hpp
vendor/entt/src/entt/signal/emitter.hpp
vendor/entt/src/entt/signal/fwd.hpp
vendor/entt/src/entt/signal/sigh.hpp
vendor/entt/src/entt/stl/algorithm.hpp
vendor/entt/src/entt/stl/array.hpp
vendor/entt/src/entt/stl/atomic.hpp
vendor/entt/src/entt/stl/bit.hpp
vendor/entt/src/entt/stl/cmath.hpp
vendor/entt/src/entt/stl/concepts.hpp
vendor/entt/src/entt/stl/cstddef.hpp
vendor/entt/src/entt/stl/cstdint.hpp
vendor/entt/src/entt/stl/functional.hpp
vendor/entt/src/entt/stl/ios.hpp
vendor/entt/src/entt/stl/iterator.hpp
vendor/entt/src/entt/stl/limits.hpp
vendor/entt/src/entt/stl/memory.hpp
vendor/entt/src/entt/stl/ostream.hpp
vendor/entt/src/entt/stl/sstream.hpp
vendor/entt/src/entt/stl/string.hpp
vendor/entt/src/entt/stl/string_view.hpp
vendor/entt/src/entt/stl/tuple.hpp
vendor/entt/src/entt/stl/type_traits.hpp
vendor/entt/src/entt/stl/utility.hpp
vendor/entt/src/entt/stl/vector.hpp
vendor/entt/src/entt/tools.hpp
vendor/entt/src/entt/tools/davey.hpp
```

## G2 类型化 ECS 与生产调用闭包

整体交付 Registry/EntityId、模板实现、配置与私有诊断，保留唯一正式 ECS target。对象构造/销毁、跨 owner 与代数身份、每实例类型注册、const 读取、同步查询/有序查询、短借用和 Failed 状态属于同一个替换意图，不拆成只能链接旧实现的中间提交。

simulation/app 调用方同组迁移：物理按显式 `PhysicsStepOrder` 业务键执行，缺键或平局不靠 native entity 顺序补齐；保留逐体提交与 Session 错误边界。迁移玩家、相机、transform、character、Session 与 Application 的既有消费者，不新增 NPC、计算服务或正式顶层 physics 模块。

CMake 边界、负向夹具、真实库契约/多 TU、编译正负例、身份/资源/代表分配故障及受影响 Session/物理基线测试源码一并保存。私有诊断仅供已经指定的两份测试，不能开放整个 src 或让生产目标依赖测试实现。有界成本 probe 保留既有 EXCLUDE_FROM_ALL 身份，不把源码存在写成已运行。

以下五个已退役文件作为本组删除提交，不恢复成活动后备，也不扩展其他删除：

- `game/modules/ecs/src/component_container.h`
- `game/modules/ecs/src/internal.cpp`
- `game/modules/ecs/src/internal.h`
- `game/modules/ecs/src/legacy/MemoryHelper.h`
- `game/modules/ecs/src/legacy/RawMemory.h`

完整文件清单共37项，包含上述5项删除：

```text
cmake/ModuleBoundaries.cmake
game/app/src/application.cpp
game/modules/ecs/CMakeLists.txt
game/modules/ecs/include/symocraft/ecs/detail/entt_config.h
game/modules/ecs/include/symocraft/ecs/detail/registry.inl
game/modules/ecs/include/symocraft/ecs/entity.h
game/modules/ecs/include/symocraft/ecs/registry.h
game/modules/ecs/src/component_container.h
game/modules/ecs/src/diagnostics/registry_test_access.h
game/modules/ecs/src/internal.cpp
game/modules/ecs/src/internal.h
game/modules/ecs/src/legacy/MemoryHelper.h
game/modules/ecs/src/legacy/RawMemory.h
game/modules/ecs/src/registry.cpp
game/modules/simulation/include/symocraft/simulation/component.h
game/modules/simulation/src/Camera.cpp
game/modules/simulation/src/character_system.cpp
game/modules/simulation/src/physics_adapter.cpp
game/modules/simulation/src/player.cpp
game/modules/simulation/src/session.cpp
game/modules/simulation/src/transform_system.cpp
test/integration/CheckNegativeBoundaries.cmake
test/unit/ecs/CMakeLists.txt
test/unit/ecs/check_registry_compilation.py
test/unit/ecs/contract_components.h
test/unit/ecs/contract_other.cpp
test/unit/ecs/entt_configuration_tests.cpp
test/unit/ecs/registry_allocation_tests.cpp
test/unit/ecs/registry_compile_negative.cpp
test/unit/ecs/registry_contract_tests.cpp
test/unit/ecs/registry_cost_probe.cpp
test/unit/ecs/registry_tests.cpp
test/unit/simulation/CMakeLists.txt
test/unit/simulation/cpu_baseline_fixture.h
test/unit/simulation/legacy_physics_baseline_tests.cpp
test/unit/simulation/session_tests.cpp
test/unit/simulation/simulation_contract_tests.cpp
```

foundation allocator/logger、有实际消费者的兼容路径、默认 D3D12、显式 Vulkan 和保留 GL 均维持现有边界；不顺带全局内存审计、退役 GL 或重选预算。生产接口、消费者及测试必须同组，不先提交新签名再等待后续修调用方。

## G3 文档审计与导航维护

保存工作区已有的独立修补，不在本轮重新运行全库审计：

- 通过系统 libarchive tar 只读流式校验已登记压缩证据成员的路径、文件集合、大小和 SHA256，拒绝缺失、重复、越界及身份变化；不解包恢复散文件。
- 被迁移的 Markdown 仍读取准确成员检查锚点，不因目标变成 .7z 而跳过校验。已退役只读 review 来源按固定历史 Git 对象核验，缺失工具、成员或不可验证原文仍报错。
- DOCUMENTATION_MAP 保存相应维护规则；issues 只修正旧证据链接，scripts/README 只修正根 README 锚点，不关闭问题或改动构建脚本。

完整文件清单共6项：

```text
docs/DOCUMENTATION_MAP.md
docs/issue/issues.md
scripts/README.md
tools/openspec/check_documents.py
tools/openspec/evidence_paths.py
tools/openspec/test_evidence_paths.py
```

scope 与 third-party 同时含 T5 当前状态、依赖说明及压缩链接修正，整份归 G4；不为纯导航在共享文件上制造额外局部补丁。现有冻结包、登记清单及原始证据均不在本批差异清单内，不重新压缩或改写。

## G4 主规格、完整归档与当前文档

保存已完成的规格同步与归档结果，不调用 sync/archive 命令。旧活动目录的8项删除、新归档目录的8项新增，以及 ecs 新主规格、module-boundaries 增量必须同组；归档不是仅改名，包含基线之后的完整设计/任务/实施与验证记录，不声称八份内容与旧 HEAD 全字节相同。

当前 ECS 门面、失败/借用机制、受控 EnTT 模板依赖、DBG01 及必要消费者职责更新到原有架构页；根 README/AGENTS、文档入口、OpenSpec context、第三方许可、scope 和关键节点保持同一交付身份。全局 VERIFICATION 只承载批准及导航，验证正文仍在归档 change 的 verification.md，不复制附件或另建报告。

T5/M3 的整体批准及“F 已取得 T5”的后续传播事实分别保留。其他机器 benchmark 延至现代性能分析工具接入后，设备角色、预算、未测状态和保留风险不取消，不恢复旧玩法前硬门槛，也不暗示性能分析工具已选定或接入。

完整文件清单共31项；旧目录8项为删除，归档目录8项为新增：

```text
AGENTS.md
README.md
docs/README.md
docs/architecture/Overview.md
docs/architecture/app.md
docs/architecture/ecs.md
docs/architecture/engineering.md
docs/architecture/foundation.md
docs/architecture/simulation.md
docs/openspec/changes/archive/2026-10-25-m3-t5-ecs-memory/.openspec.yaml
docs/openspec/changes/archive/2026-10-25-m3-t5-ecs-memory/decisions.md
docs/openspec/changes/archive/2026-10-25-m3-t5-ecs-memory/design.md
docs/openspec/changes/archive/2026-10-25-m3-t5-ecs-memory/proposal.md
docs/openspec/changes/archive/2026-10-25-m3-t5-ecs-memory/specs/ecs/spec.md
docs/openspec/changes/archive/2026-10-25-m3-t5-ecs-memory/specs/module-boundaries/spec.md
docs/openspec/changes/archive/2026-10-25-m3-t5-ecs-memory/tasks.md
docs/openspec/changes/archive/2026-10-25-m3-t5-ecs-memory/verification.md
docs/openspec/changes/m3-t5-ecs-memory/.openspec.yaml
docs/openspec/changes/m3-t5-ecs-memory/decisions.md
docs/openspec/changes/m3-t5-ecs-memory/design.md
docs/openspec/changes/m3-t5-ecs-memory/proposal.md
docs/openspec/changes/m3-t5-ecs-memory/specs/ecs/spec.md
docs/openspec/changes/m3-t5-ecs-memory/specs/module-boundaries/spec.md
docs/openspec/changes/m3-t5-ecs-memory/tasks.md
docs/openspec/changes/m3-t5-ecs-memory/verification.md
docs/openspec/config.yaml
docs/openspec/specs/ecs/spec.md
docs/openspec/specs/module-boundaries/spec.md
docs/scope/project-scope.md
docs/third-party.md
docs/verification/VERIFICATION.md
```

不重填人工 checkbox、不将历史测试改记为本轮通过、不改其他主规格或 T3/T4 冻结归档。新 NPC proposal 与本组无提交依赖，不因 scope 提到下一阶段就提前纳入。

## 明确排除

以下新文件属于另一项 NPC 规划，不属于本次 ECS/Memory apply 提交，保留未跟踪状态，后续另定范围：

```text
docs/openspec/changes/npc-framework-harvest-mvp/.openspec.yaml
docs/openspec/changes/npc-framework-harvest-mvp/proposal.md
```

个人 `docs/development-journal/`、`docs/.obsidian/`、忽略的 `.agents/`、out、旧缓存、运行包及任何未确认残留均不纳入，不读取个人正文或重新跟踪私有配置。不修改忽略规则、全局 Git 配置、凭据、hooks 或全库换行，不清场、不跨工作树复制或改写历史。

## 后续暂存与提交边界

取得当次明确提交授权后，仅进行必要 Git 操作：

1. 重新核对分支、HEAD、索引和本页固定路径，检查新增并发差异；起点不符或文件内容超出已确认范围时先报告，不覆盖用户改动。
2. 每组使用准确文件清单暂存；删除路径明确纳入，归档来源/目标成对，vendor 只取上述111项，不使用 `git add .` 或不限定范围的 `git add -A`。
3. 检查准确 staged name/status/mode 和完整 diff，核对依赖、意外私密信息、冲突标记及 `git diff --cached --check`；确认只有当组内容后，使用约定中文消息提交。不得用 stash/reset 或改写工作区拼装中间版本。
4. 本页完整计划随 G1 提交，不回填循环 hash 或追加旧批次历史；每组实际提交结果在聊天报告。最后核对剩余差异应为明确排除项及未纳入的并发变化，不自动处理它们。

本轮及后续仅 Git 提交不自动附带构建、CTest、文档审计、OpenSpec validate、游戏启动、安装验证、GPU/人工检查或性能采样。若另需这些工作，须单独明确范围；不把它们设为擅自扩展提交任务的前置流程。push、merge、PR、tag 和历史改写同样需要独立当前授权。

## 验证事实与保留风险

本轮只做只读 Git/差异盘点、依赖与路径归组、本页更新，以及本页空白/清单覆盖核对；索引保持不变。构建、测试、文档审计和运行验证均未执行，不能宣称任何中间提交已经通过工程验证。

既有结果来源仅引用 T5 verification：生产传播轮 F 的 CPU-only Debug/Release 各83/83、独立 benchmark 各9/9、其他已列配置/边界/安装检查均有原版本和适用范围，不是本轮重跑，也不是全量107项测试通过。历史文档审计的16项问题与本批工具修补同存，本轮不作新的全库错误数或通过结论。

继续保留 1175/诊断完整性、暂停 Fix1、DBG01 的检查损失与旧 level-2 terminate、有限 OOM/成本观察、非全量原生故障矩阵、长期/真正干净机和其他设备未测等已列限制。批准不关闭这些问题；不将短复查或功能构建提升为性能、M4/M5/M6 或全局内存保证。
