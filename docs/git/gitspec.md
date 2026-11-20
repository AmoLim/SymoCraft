---
tags:
  - area/meta
---

# NPC 框架与耗时采集 MVP 分步提交计划

2026-11-19 本轮用户要求：为本次实现、规格同步和归档重新拟定 gitspec，按 N1/N2/N3 分步提交，先审阅本计划、确认后再提交；撤回原单笔 `feat(npc)`，保留其他未提交内容，暂不推送。本文件替换旧的单笔提交方案，仅适用于当前批次。**用户已于 2026-11-19 明确确认本计划，授权先备份并撤回原提交，再按四笔方案验证及本地提交；保留其他未提交内容，暂不推送。**

## 当前基线与保留内容

- 代码根：`G:/Sandbox/Worktrees/npc-framework-harvest-mvp/Symocraft`；分支：`codex/npc-framework-harvest-mvp`。
- 待替换的分支顶端提交：`d59891aa8bc2eb46a5d08ed33d6fd95df121c318`，`feat(npc): 实现耗时采集演示并同步归档规格`。
- 重组起点：该提交的唯一父提交 `a6620be9e3e6fbc4a06244686d971b6e6ce754a9`。不撤回此前 physics 提交，不修改其他分支或远端。
- 原提交共涉及 273 个文件；候选范围以该提交相对父提交的准确文件集及差异为准，不以当前目录通配符代替范围核对。新 gitspec 是该文件集内唯一允许重写的计划正文。
- 开始检查时暂存区为空，另有 `game/app/CMakeLists.txt` 的 Debug UCRT 安装路径修改：根据选定 DXC SDK 定位并显式检查 `ucrtbased.dll`。这段通用修订不属于原 NPC 提交，完整保留在工作区，不进入四笔新提交。
- 已保存该未提交文件的原字节及补丁。其 SHA256 为 `f193001a744c5c573c5bd892a0ff1108c92d9f355905b3256022b00786bad21d`；执行前后核对。期间出现其他改动、暂存或 HEAD 变化时先重新核对，不能据本快照整体覆盖。

## 提交顺序与完整意图

前三笔分别交付 N1、N2、N3，第四笔集中交付已验收的主规格、归档和说明，避免在基础实现尚未落齐时发布最终验收正文。顺序为 **N1 → N2 → N3 → 规格与归档**。

| 顺序 | 提交信息 | 可独立审阅的交付 |
| --- | --- | --- |
| 1 · N1 | `feat(npc): 建立实体与耗时采集核心` | 实体装配/移除、真实地形移动、耗时与暂停取消、稳定争抢顺序、World 原目标身份、F1 资源准备与一次入账、帧提交及只读显示交接；不引入行为树或角色显示依赖 |
| 2 · N2 | `feat(npc-ai): 接入行为树与离线策划流程` | 固定 BehaviorTree.CPP、私有动作适配、隔离树/黑板、校验与节点模型导出、策划 XML、能力注册/诊断、Idle 类型语义及命名回归；依赖 N1 的正式动作入口 |
| 3 · N3 | `feat(npc-demo): 接入分件角色与采集演示` | cgltf 与 Blender/GLB 分件管线、D3D12/Vulkan 角色共享绘制、真实位移/耗时驱动姿态、进度与资源反馈、完整 Application 场景接线及演示控制；依赖 N1/N2 |
| 4 · 规格与归档 | `docs(npc): 同步主规格并归档已验收变更` | 8 份主规格、14 文件归档、V2/整体批准、架构与操作说明、依赖说明及导航；交付最终可追踪的完整验收记录 |

原任务 N1.5 的 Application 场景装配、开始/暂停/从头重新演示与场景切换依赖 N2.2 和 N3 的显示。它作为 N1/N2/N3 联合接线放在第 3 笔，不在 N1 中留下引用尚未提交 AI/角色接口的半成品。前三笔是按依赖形成的提交边界，不修改原 change 的节点职责或验收范围。

### 第 1 笔：N1

完整纳入以下意图和对应测试：

- `game/modules/simulation/` 的 `npc.h`、`npc.cpp`、`npc_runtime.h`、`session.h`、`session.cpp`、`diagnostics/npc_diagnostics.h`。包含 C1/C2 的自有动作/显示值，沿用正式 physics 与 ECS；不提前加入 `npc_ai.*`。
- `game/modules/world/` 中采集规则、稀疏单格目标版本和条件移除的全部改动；`assets/configs/npc_blocks.yaml` 及其必需资产登记。
- `test/unit/simulation/npc_tests.cpp`、`test/unit/world/harvest_tests.cpp`、World 窄测试访问及对应测试注册；只开放 `npc_tests` 所需的诊断入口。
- simulation CMake 此步仅加入 `npc.cpp` 和 NPC 核心测试，不链接 BehaviorTree.CPP；本 gitspec 随第 1 笔提交，并在用户确认后准确记录本批授权。

### 第 2 笔：N2

- `vendor/behaviortree_cpp/` 的原提交完整文件集及来源身份；`cmake/ThirdPartyBehaviorTree.cmake`、`ThirdPartyGame.cmake` 接入与 simulation 的私有链接、tinyxml2 路径。
- `npc_ai.h`、`npc_ai.cpp`、`test/unit/simulation/npc_ai_tests.cpp` 和其测试注册；`assets/ai/` 三份 XML 及运行资产登记。
- `.gitattributes` 的 BehaviorTree/补充许可原字节规则，`Boost-1.0`、`nlohmann-json` 两份完整许可、来源说明中 BehaviorTree 段，以及游戏安装的 BehaviorTree/内嵌贡献者许可段。
- ModuleBoundaries 中允许 simulation 的私有 BehaviorTree target，并阻止公开头泄漏 BehaviorTree、tinyxml2、cgltf 类型；该头部防泄漏规则可以先于 cgltf 实现进入。
- 保留 CPU-only、离线固定版本及无编辑器连接的运行边界；不提前提交 Application 对 `NpcBrain` 的接线。

### 第 3 笔：N3 与演示联合接线

- `vendor/cgltf/`、其 `.gitattributes` 原字节规则和许可/来源/安装接线；scene 的 `character_mesh.h`、assets 角色导入及测试、必要 assets→scene 依赖。
- 两份 `.blend`、两份实际导出 `.glb`、`scripts/create-character-fixtures.py`、角色 HLSL、两个后端 shader 构建、renderer 门面/共享资源/双后端实现，以及角色输入和在途寿命测试。
- `game/app/` 的 Application/GameRenderer、启动参数、显示交接探针、程序姿态、演示控制和面板的最终改动；`assets/configs/npc_scene.yaml`、GLB 必需资产及安装包检查。
- app 单元测试、`npc_demo_probe`、改名 native 回归入口和生命周期探针必要链接；不把显式 GPU/窗口探针改为无人值守 CTest。
- `game/app/CMakeLists.txt` 使用原提交中的 NPC/许可段落，排除当前另有的 Debug UCRT 修订；不通过整文件暂存把它夹带进来。

### 第 4 笔：规格、归档及说明

- 主规格：`application-lifecycle`、`character-assets`、`module-boundaries`、`npc-ai`、`npc-entities`、`renderer`、`simulation`、`world` 的当前已同步内容，共 20 条新增要求、1 条修改要求。
- `docs/openspec/changes/archive/2026-11-19-npc-framework-harvest-mvp/` 原提交内全部 14 份文件，包括 `.openspec.yaml`、唯一 verification、任务和决策；不重建旧活动目录，也不改写已验收的历史结果。
- `README.md`、`scripts/README.md`、`docs/architecture/` 中 app/assets/engineering/renderer/simulation/world、`docs/third-party.md`、`docs/verification/VERIFICATION.md`、`docs/tag-inventory.json` 的原提交改动。
- 先前 physics 批次的“当前 gitspec”链接清理沿原提交纳入，physics 批准本身不变。其他旧归档、冻结证据与个人材料不改。

## 必须按段落拆分的共用文件

以下 7 个文件不能整文件提前暂存；先从父提交构造该步候选内容，再检查相邻步骤的增量。最终生产内容应与原 NPC 提交逐字节一致。

| 文件 | 归属和拆分 |
| --- | --- |
| `.gitattributes` | N2：BehaviorTree 和两份补充许可；N3：cgltf |
| `cmake/ModuleBoundaries.cmake` | N1：NPC 诊断白名单；N2：树依赖及公开头防泄漏；N3：assets→scene |
| `cmake/RequiredAssets.cmake` | N1：npc_blocks；N2：三份树 XML；N3：npc_scene 和两份 GLB |
| `cmake/licenses/SOURCES.txt` | N2：BehaviorTree 与补充许可来源；N3：cgltf 来源，最终顺序保持原提交 |
| `game/modules/simulation/CMakeLists.txt` | N1：npc.cpp；N2：npc_ai.cpp、树库和 tinyxml2 私有路径 |
| `test/unit/simulation/CMakeLists.txt` | N1：simulation.npc；N2：simulation.npc_ai |
| `game/app/CMakeLists.txt` | N2：BehaviorTree/补充许可部署；N3：角色数据 target、cgltf 许可；通用 UCRT 修改始终不提交 |

## 已确认的执行与保护

1. 再次核对分支、原提交/父提交及其他未提交内容；先生成并核验原 NPC 提交的本地恢复 bundle，保存在 out。确认可恢复后才改动分支历史。
2. 采用保留工作区的 mixed reset 撤回顶端原提交，回到 `a6620be9`。不使用 hard reset、clean、覆盖检出或逆向应用补丁；恢复 bundle 和 reflog 保留原提交。
3. 按上述顺序逐笔构造候选暂存树。完整文件只取原提交准确清单，共用文件按意图分段；不 `git add .`、不将 out/试玩包/失败日志/缓存/个人文件纳入提交。
4. 在独立候选目录验证当步暂存树，不能用当前完整工作区“碰巧有后续文件”的构建作为中间提交通过的依据。检查通过并审阅准确暂存差异后，才创建对应提交。
5. 最终组合树除本 gitspec 的本轮计划外，与原提交内容一致；当前 UCRT 未提交补丁和后续其他改动完整保留。逐笔核对父子关系、文件集及提交信息，最终报告四个新提交 SHA。
6. 不执行 push、merge、PR、标签、主工作区生产传播或全局 Git 配置修改。用户此次确认只适用于本计划的本地历史重组和四笔提交。

## 每步验证与证据边界

| 阶段 | 确认后需要实际执行的检查 |
| --- | --- |
| N1 | 候选树 CPU-only Debug 构建；NPC/World 采集、Session/物理及相关公开头/边界回归；核对未引入 AI/图形依赖 |
| N2 | 候选树 CPU-only Debug 构建；重跑 N1 相关回归和 simulation.npc_ai，覆盖节点模型、表达式拒绝、扩展、正式动作绑定和 Idle 改名；核对离线依赖与许可字节 |
| N3 | 最终实现候选的现代 Debug 构建和相关 14 项 CTest；核对角色/树/shader/许可部署及新增探针可构建。只因拆提交不自动追加 GPU、人工体验、完整多配置矩阵或性能采样 |
| 规格与归档 | 主规格 validate、8 份增量/主规格内容核对、归档输入和链接、差异空白及暂存集核对；记录现有文档提示及旧错误，不宣称全库全绿 |

本轮分步验证和提交按下表执行，实际完成结果在第 4 笔提交前登记；没有结果的检查不记为通过。

既有完整构建矩阵、CPU/正式 World/物理、双后端 native、Groot2 实际编辑往返、V2 和整体验收，见[归档 verification](../openspec/changes/archive/2026-11-19-npc-framework-harvest-mvp/verification.md)。此前结果保留原候选与日期，不直接充当拆分后各中间提交的验证；未覆盖的长期、性能、其他机器和干净机范围不因重组提交关闭。

## 本批执行结果

原提交已先生成并通过本地 bundle 核验，再以 mixed reset 撤回到约定父提交；工作文件未回退，UCRT 修改仍按原字节保留。恢复包位于 `out/npc-demo-recheck/git-split-execution-20261119/original-feat-npc.bundle`，不加入提交。

| 步骤 | 本地提交 | 独立候选树实际检查 |
| --- | --- | --- |
| N1 | `af659297cea7a1419bc05f893ef9e1fab9f043f2` | CPU-only Debug 全部目标构建成功，相关 CTest 27/27，通过 NPC/World、Session、物理、公开头与负向边界 |
| N2 | `11c1fe71b3e9e38226271d134957306e77248e23` | 同一独立源目录仅增加 N2 候选文件后构建成功，相关 CTest 29/29；不含 N3 文件 |
| N3 | `22a084268ace6f4493f31da65d9774396e59a363` | 独立候选的 D3D12/Vulkan/保留 GL Debug 构建成功，相关 CTest 14/14；安装成功，两现代后端非 GPU 资产预检通过；NPC 配置/树/GLB、角色 shader 和许可逐字节核对通过 |

上述每一步的源目录均逐文件核对为当步准确暂存树，验证与结果留在 out，不使用完整工作区的后续文件补齐中间提交。第 4 笔承载原有规格、归档和本 gitspec 的执行结果，最终提交 SHA 由完成报告给出。

本轮准备恢复包时首次使用的裸 SHA 范围被 Git 拒绝为空 bundle，改为显式 HEAD 后备份成功；副本检查发现导出过程的换行转换，已仅在 out 副本中恢复原 blob 字节并重新核对。两者均发生在相关提交之前，没有修改用户生产文件。第三方和许可原有空白不重写，按原提交 blob 核对；项目自有差异执行空白检查。没有重复执行 GPU 窗口、人工体验、完整多配置或性能采样。

第 4 笔候选已通过全部主规格 validate（14/14），8 份增量中的 21 条要求逐条一致，14 份归档文件、17/17 任务及 105 处归档链接核对通过。保留短 Purpose、既有长要求等校验提示；本轮没有重跑全库文档审计，原记录的 36 项旧问题不宣称关闭。暂存内容、独立副本和最终合成树经核对，除本 gitspec 更新外与原 NPC 提交逐文件 blob 一致；原有 UCRT 修改未纳入提交。
