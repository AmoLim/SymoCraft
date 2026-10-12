---
tags:
  - area/meta
---

# 当前 Git 提交批次

## 授权与状态

日期：2026-10-12。分支：`feature-refactoring`。起点：`8a95613435f9bdf433e98ebf136a5e01a13079e5`。

用户要求「给我这轮改动进行git提交划分，先更新spec」，随后要求同步新增改动，并明确「好的进行提交吧」。**本批三组方案及已报告的13项文档检查缺口已获确认，现执行本地暂存与提交。** 旧批次审批和成绩不沿用，不追加旧历史；本页记录当前批次身份和实际结果。

用户明确「纳入压缩迁移，不用核对」。据此纳入现有准确删除项及21个压缩包；不执行解包、成员字节、原清单或恢复核验，也不把这些核验设为本批前置。范围确认之外的stage/commit已由上述本轮消息另行授权；仍不授权继续删除其他材料、重压包或修复工程。实际staged diff与范围核对不等同于已免除的压缩迁移核验。

规则见 [AGENTS](../../AGENTS.md)、[文档治理](../DOCUMENTATION_MAP.md)、[scope](../scope/project-scope.md)。本页是Git分组，不是新的功能proposal；不改功能计划或人工勾选，不执行真实sync/archive、继续M3开发、push、merge、PR、tag或历史改写。

## 输入身份

按NUL分隔Git状态、关闭rename推断盘点，排除个人目录及本页；执行起点HEAD如上，当时暂存区为空。下表固定这次授权输入，不是各提交完成后的dirty计数。

| 当前变化 | 路径数 |
| --- | ---: |
| 归档外已跟踪修改 | 59 |
| 归档外已跟踪删除 | 46 |
| 归档外新增文件 | 8 |
| 旧归档已跟踪删除 | 7217 |
| 新增 .7z 包 | 21 |
| 候选合计，不含本页 | 7351 |
| 加本页后的唯一路径 | 7352 |
| 排除个人笔记/Obsidian | 11 |

7个包承接原历史目录，14个为本轮已有R2配置/故障/消费者/短采样包。7217是当前Git删除数量，不授权再次删除7219份或递归清空archive。`migration-manifest.json` 和 `evidence-consolidation-20261011.json` 仍保留。

归档外删除46项：milestones 34项、spec 11项、testing 1项；相对起点的46项包含此前已经删除的10篇旧规格正文，以及最新处置记录所说的剩余36项，不能把Git差异数量当成这次额外删除46项的实施成绩。本页仅把已有删除及配套改链纳入计划，不继续删除或恢复文件。

当前候选SHA256：`50b55d61980c469efd1649c7d4e271fbe99296e561f7ff583e59636f5a6d2f87`。算法：排除 `docs/.obsidian/`、`docs/development-journal/` 和本页；逐项记录path/status/size/sha256，删除项size/sha256为null；按path排序，JSON以UTF-8、ensure_ascii=False、sort_keys=True、separators=(',', ':')序列化。此摘要只固定Git输入，不证明压缩包内原件或恢复一致性。

现有候选内容共27623241字节，无超过100 MiB的新增单文件；不等同于凭据扫描、远端保存或恢复测试。提交前重新取得准确路径，批准后出现或改字节的并发输入不自动纳入。

## 提交划分

### G01：D3D12配置、故障验证与默认切换

消息：`feat(renderer): 完善 D3D12 配置与故障验证并切换默认后端`。

22项路径。完整纳入可选GL/现代独占构建、资源及许可部署、世界摘要预检、默认选择、候选打包和旧manifest兼容、配置矩阵、资产夹具，以及私有诊断factory/故障probe/运行器/构建注册。同组直接依赖一起提交，不拆缺头或缺probe的中间提交。

- `CMakeLists.txt`
- `CMakePresets.json`
- `cmake/RequiredAssets.cmake`
- `cmake/ThirdPartyGame.cmake`
- `game/app/CMakeLists.txt`
- `game/app/src/game_renderer.cpp`
- `game/app/src/startup_options.h`
- `game/modules/renderer/CMakeLists.txt`
- `game/modules/renderer/src/backend/d3d12/d3d12_backend.cpp`
- `game/modules/renderer/src/backend/d3d12/d3d12_backend.h`
- `scripts/package-d3d12-candidate.py`
- `test/integration/CMakeLists.txt`
- `test/integration/CheckAssetPackage.cmake`
- `test/integration/renderer/test_candidate_scripts.py`
- `test/integration/renderer/verify-d3d12-candidate.py`
- `test/unit/app/startup_options_tests.cpp`
- `test/unit/assets/CMakeLists.txt`
- `test/unit/assets/asset_paths_tests.cpp`
- `game/modules/renderer/src/backend/d3d12/diagnostic_access.h`
- `test/integration/renderer/d3d12_fault_probe.cpp`
- `test/integration/renderer/verify-d3d12-faults.py`
- `test/integration/renderer/verify-renderer-configurations.py`

新游戏配置默认D3D12；标准Debug/Release游戏preset现显式设置D3D12=ON，再次配置时也启用旧缓存中的后端。根option不使用FORCE，自定义配置仍可命令行显式OFF覆盖，CPU-only规则不变。不能继续把“只改首次默认、旧OFF缓存不受preset影响”写成当前行为。GL可显式保留/关闭，未交付后端具名拒绝、不fallback。普通游戏不创建诊断控制。原生设备API移除、真实在途等待与HRESULT注入分别保留性质，不冒称自发事故、真实OOM、正常退休全部通过或双现代整体验收。

新验证入口仍有旧archive目录/输出约束；压缩包不能直接代替目录参数。不在本轮修脚本、降低能力或扩测矩阵，跳过解包核验不等于工具已适配新归档政策。

### G02：原生Benchmark现代协议消费

消息：`feat(benchmark): 接入 D3D12 采样协议与原生消费者`。

10项路径。完整纳入GUI/CLI选择、会话/恢复设置与实际设备身份、GL25列/现代32列及telemetry v1校验、GPU缺失原因/上传归因、原生夹具和capture probe、构建注册及操作/机制说明；依赖G01的生产配置和后端选择。

- `docs/architecture/benchmark.md`
- `test/benchmark/CMakeLists.txt`
- `test/benchmark/runner_tests.cpp`
- `test/support/benchmark_fixture.cpp`
- `tools/benchmark/README.md`
- `tools/benchmark/include/benchmark/runner.h`
- `tools/benchmark/src/gui.cpp`
- `tools/benchmark/src/main.cpp`
- `tools/benchmark/src/runner.cpp`
- `test/benchmark/capture_probe.cpp`

游戏默认D3D12不等于Benchmark默认切换；执行器当前仍默认OpenGL并显式传renderer。旧GL协议/会话保持身份；短CLI结果不推定GUI人工、正式九轮、视觉容差或预算通过。独立工具不反向依赖游戏模块，不把原生统计逻辑移入Python。

### G03：验证治理、旧文档退役与压缩迁移

消息：`docs(verification): 收口验证治理与旧文档退役及压缩归档`。

7320项路径：下列35项现有正文/消费者（32项修改、3份新增verification）、46项旧文档删除、7217项准确旧归档删除、21个准确新包，以及本页。依赖G01/G02。旧目录退役、当前索引/消费者改链与压缩包入库同组，不先提交缺目标导航，也不只提交删除。

修改及新增的准确范围：
- `AGENTS.md`
- `README.md`
- `docs/DOCUMENTATION_MAP.md`
- `docs/architecture/engineering.md`
- `docs/architecture/renderer.md`
- `docs/issue/issues.md`
- `docs/openspec/changes/m3-t2-publication-fix1/design.md`
- `docs/openspec/changes/m3-t2-publication-fix1/proposal.md`
- `docs/openspec/changes/m3-t3-renderer/decisions.md`
- `docs/openspec/changes/m3-t3-renderer/design.md`
- `docs/openspec/changes/m3-t3-renderer/proposal.md`
- `docs/openspec/changes/m3-t3-renderer/specs/renderer/spec.md`
- `docs/openspec/changes/m3-t3-renderer/tasks.md`
- `docs/openspec/changes/m3-t4-application-simulation/design.md`
- `docs/openspec/changes/m3-t4-application-simulation/proposal.md`
- `docs/openspec/changes/m3-t4-application-simulation/specs/application-lifecycle/spec.md`
- `docs/openspec/changes/m3-t4-application-simulation/tasks.md`
- `docs/openspec/config.yaml`
- `docs/openspec/specs/foundation-files/spec.md`
- `docs/openspec/specs/module-boundaries/spec.md`
- `docs/openspec/specs/platform/spec.md`
- `docs/openspec/specs/renderer/spec.md`
- `docs/openspec/specs/world/spec.md`
- `docs/scope/project-scope.md`
- `docs/tag-inventory.json`
- `docs/verification/VERIFICATION.md`
- `docs/verification/methods.md`
- `scripts/README.md`
- `test/experimental/platform-handoff/README.md`
- `test/experimental/platform-sdl3/README.md`
- `test/experimental/publication-fix1/README.md`
- `test/experimental/sdl3/README.md`
- `docs/openspec/changes/m3-t2-publication-fix1/verification.md`
- `docs/openspec/changes/m3-t3-renderer/verification.md`
- `docs/openspec/changes/m3-t4-application-simulation/verification.md`
- `docs/git/gitspec.md`

已存在的准确旧文档删除（不是本轮继续删除授权）：
- `docs/milestones/m0/Probe-Startup.ps1`
- `docs/milestones/m0/README.md`
- `docs/milestones/m1/README.md`
- `docs/milestones/m1/Verify-Incremental.ps1`
- `docs/milestones/m2-a/PLAYER-README.zh-CN.md`
- `docs/milestones/m2-a/README.md`
- `docs/milestones/m2-t2/README.md`
- `docs/milestones/m2-t3/BENCHMARK-FIX.zh-CN.md`
- `docs/milestones/m2-t3/PLAYER-README.zh-CN.md`
- `docs/milestones/m2-t3/README.md`
- `docs/milestones/m2-t3/laptop-first-run.md`
- `docs/milestones/m3-t0/README.md`
- `docs/milestones/m3-t1/README.md`
- `docs/milestones/m3-t2/README.md`
- `docs/milestones/m3-t2/manual-verification.md`
- `docs/milestones/m3-t2/manul-verification.md`
- `docs/milestones/m3-t2/platform-contract-candidate.md`
- `docs/milestones/m3-t2/s3-manual-verification.md`
- `docs/milestones/m3-t2/s4-platform-handoff.md`
- `docs/milestones/m3-t2/s5-delivery-status.md`
- `docs/milestones/m3-t3/README.md`
- `docs/milestones/m3-t3/manual-verification.md`
- `docs/milestones/m3-t3/r2-entry.md`
- `docs/milestones/m3-t3/r2-step1-implementation.md`
- `docs/milestones/m3-t3/r2-step2-implementation.md`
- `docs/milestones/m3-t3/r2-step3-implementation.md`
- `docs/milestones/m3-t3/r2-step4-implementation.md`
- `docs/milestones/m3-t4/README.md`
- `docs/milestones/m3-t4/consumer-audit.md`
- `docs/milestones/m3-t4/consumer-source-manifest.json`
- `docs/milestones/m3-t4/cpu-behavior-baseline.md`
- `docs/milestones/m3-t4/interface-contract-candidate.md`
- `docs/milestones/m3-t4/interface-decisions.md`
- `docs/milestones/m3-t4/manual-verification.md`
- `docs/spec/M3/M3-T1-T3-CPU契约清单.md`
- `docs/spec/M3/M3-T1-世界模块重构.md`
- `docs/spec/M3/M3-T2-Fix1-spec.md`
- `docs/spec/M3/M3-T2-SDL3迁移.md`
- `docs/spec/M3/M3-T3-渲染器重构.md`
- `docs/spec/M3/M3-T4-应用平台与模拟模块接口.md`
- `docs/spec/md-organize/architecture-template-coverage-spec.md`
- `docs/spec/md-organize/data-system-migration-spec.md`
- `docs/spec/md-organize/mdspec.md`
- `docs/spec/md-organize/tag-inventory.json`
- `docs/spec/project-architecture/面向对象与数据导向架构设计初稿.md`
- `docs/testing/gameplay-smoke.md`

本组完整意图：
- 当前Renderer基线、README和机制与默认/可选后端实现一致；T3的R2风险收口、R3用户批准/默认切换、已选预算及未覆盖项同组保留，不由Git提交新造批准。
- T4自动验证分工及四组人工体验：每后端四组和退出/重启合计最多300秒。不将短体验写成原15分钟通过，不改变D01-D13、生产实施授权、T3/M5或下一玩法前中低端门槛。
- 每change一份verification.md，默认无附件/原始数据，全局索引仅关键节点与导航；暂停Fix1不恢复。新政策不追溯改写旧冻结结果。
- 旧spec/testing/milestones退出当前目录，恢复入口按现有VERIFICATION所记起点提交和原仓库路径导航；现役spec、活动change、Issues及四份实验README改链一起入库。只保存必要当前机制/决定，不复制旧正文、复活平行计划或改冻结历史。
- 当前`docs/tag-inventory.json`已有同步修改同组纳入：managed 51、retiredReviewSources 0、preservedPrivateRegistrations 1。这是当前输入，不是本Agent新刷新结果；不读取个人正文、不改旧私有注册或生成技能，不为压缩建立第二索引/报告。
- 当前scope、OpenSpec配置、现役/目标说明及消费者保持成套；与旧首次默认不同的IDE缓存修复及说明纳入本批，文档中的旧运行记录仍具点时含义。

准确新包全部位于 `docs/verification/archive/`：
- `benchmark-20261005.7z`
- `docs-navigation-20261011.7z`
- `m3-t3-r2-config-001.7z`
- `m3-t3-r2-config-002.7z`
- `m3-t3-r2-config-003.7z`
- `m3-t3-r2-config-004.7z`
- `m3-t3-r2-config-005.7z`
- `m3-t3-r2-config-006.7z`
- `m3-t3-r2-consumers-001.7z`
- `m3-t3-r2-consumers-002.7z`
- `m3-t3-r2-faults-001.7z`
- `m3-t3-r2-faults-002.7z`
- `m3-t3-r2-faults-003.7z`
- `m3-t3-r2-faults-debug-001.7z`
- `m3-t3-r2-faults-release-001.7z`
- `m3-t3-r2-short-001.7z`
- `milestone-evidence-20261011.7z`
- `project-history-20261011.7z`
- `t2-approval-20261009.7z`
- `t3-r1.7z`
- `t3-r2.7z`

只纳入当前删除集合，不继续删除其他out、冻结材料或残留文件；不重新生成/重压上述包，不修改保留的旧JSON映射让历史检查变绿。按本轮明确要求不核验包内成员、原清单、恢复或远端保存，不宣称它们已证明。此次准确现存包是用户特别要求纳入的范围，不扩成以后默认保存原始附件的例外。

## 检查与已知缺口

规划同步只盘点状态/差异及治理/恢复说明；获准提交后执行下面的最小离线检查。未执行原生构建、CTest、游戏/GPU/故障、原生Benchmark夹具或采样、候选打包、完整测试矩阵、人工核查或压缩迁移核验。

| 本批实际检查 | 结果 |
| --- | --- |
| 候选输入/分组身份 | 授权SHA256与7351项范围一致；准确22/10/7320项分组，不混入新并发输入 |
| `test/integration/renderer/test_candidate_scripts.py` | 本批26项通过；首次沙箱Temp目录WinError5使setUp/cleanup报错，未取得有效验证，正常权限复验26/26，不改测试/业务 |
| 项目wrapper `validate --all --strict` | 9项通过、0失败；首次沙箱realpath EPERM，正常权限复验，不关闭路径规则或执行sync/archive/update |
| 当前文档检查 | 55篇正文，13项错误、0警告，本页无链接错误；已知缺口保留，不报告通过 |
| 凭据模式筛查 | 68份非私有候选文本未命中私钥/GitHub token/长OpenAI key模式；未解包或扫描压缩内容，不是全面机密审计 |
| G01/G02实际暂存与提交 | 22/10项路径、文件模式及对象内容已核对，空白检查通过；实际commit对象与已审暂存区一致 |
| G03实际暂存 | 7320项准确路径：7263项删除（旧归档7217、旧正文46）及57项现有输入；其中21个压缩包，不做解包/成员/恢复核验；无额外路径、冲突或特殊文件模式 |
| G03空白检查 | 全组返回0，但对已删除归档的部分超长路径查询.gitattributes有Filename too long警告；归档外再查返回0、无警告，不恢复旧文件或修改配置绕过历史身份 |

检查使用应用提供的 `C:/Users/ldh20/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe`，`-B -X utf8`；OpenSpec通过项目wrapper使用已安装CLI。没有修改PATH、全局配置或初始化开发工具链。本批最小离线结果针对完整工作区，不宣称三个中间提交各自完整构建/运行通过；本地准确路径和对象审阅材料在 `out/git/20261012-8a95613-approved/`，不提交第二份长期计划。

当前 `tools/openspec/check_documents.py` 实际返回1：55篇正文、13项错误、0警告，本页无链接错误。先前97篇/414项描述的是更新前工作区，不继续当作当前结果。旧正文移除和现役引用改链使检查对象及错误减少，不证明压缩包成员完整、历史材料已恢复，或剩余归档消费者已经适配。仍有旧证据位置映射找不到独立成员、third-party/Issues/scope等仍指向压缩前目录的错误；本轮不解包、不恢复、不改其他文档/工具/旧映射。

**用户已确认本批保留已报告的13项缺口，不记录成检查通过。** 若要求修复，先另行授权准确范围，不在提交阶段临时混入修复。压缩迁移免核验要求继续有效，但不能隐去已发现的缺链。

T3 verification.md内配置矩阵、受控故障、短采样、默认切换定向结果及旧Python成绩仍为既有记录；本批新26项离线结果只以上表为准。新增“IDE旧缓存修复”中的Release重新配置/目标构建、startup_options 1/1、预检和120帧等是已有实施记录，包含被中止的中间配置，本批未重新执行，不当作全部Debug/Release、全部测试或中间提交的新成绩。Debug foundation.files/1175失败、world.allocations禁用、视觉/正式预算、正常在途回收/完整异常关闭/适用部署等局限不改为通过；R3用户批准不回填A01-A19全绿，默认D3D12不推定Vulkan或完整T3通过。T4规划调整不表示physics或SimulationSession已有实现。

本批按影响范围选择上述最小检查，不默认新增/执行全后端矩阵、长采样或人工清单；已免除的解包/恢复核验不重新加入。历史记录不作为本批或中间提交成绩，未跑、失败和禁用分别保留。

## 执行保护

1. 当前仅凭本轮明确确认执行三组本地stage/commit，不沿用上一批批准，不push。
2. 提交前核对HEAD/index、准确候选与并发变化；逐组按路径清单暂存，查看实际staged diff、模式/删除，禁止git add .、git add -A或整目录无差别暂存；凭据模式筛查范围和限制见上表。
3. 压缩迁移遵照本轮免核验要求，不解包/恢复/核对成员，也不重写冻结原件。Git集合核对不伪装成压缩完整性证明。
4. Windows长路径或本机textconv问题仅用单次 `git -c core.longpaths=true`、`--no-textconv --no-ext-diff` 处理，不改全局/仓库配置、hooks、凭据或全库换行。
5. 个人 `docs/.obsidian/`、`docs/development-journal/` 共11项排除；不读正文、不暂存、不恢复。out、.temporary、IDE/机器设置、当前唯一包和暂停现场不提交、不删除。
6. 共享文件本批按完整文件归组，integration CMake在G01、benchmark CMake在G02；没有索引分块需要。并发变化改变依赖时先说明，不临时改业务凑提交。必要相邻组按完整意图合并，不混入新并发输入。
7. 提交后核对实际提交集合/剩余输入并回填本页；push/PR、真实功能sync/archive、继续实施及历史改写均不在本轮授权内。

## 提交结果

| 组 | 消息 | 提交身份 |
| --- | --- | --- |
| G01 | `feat(renderer): 完善 D3D12 配置与故障验证并切换默认后端` | `ae26ca1e2edb77971f547fb7f1987adb6e25bf1f` |
| G02 | `feat(benchmark): 接入 D3D12 采样协议与原生消费者` | `62682f5f7d37e226b577bdbff98964bc729b668b` |
| G03 | `docs(verification): 收口验证治理与旧文档退役及压缩归档` | 本页所属同名提交；准确hash以Git历史定位，不在正文预填自身循环hash |

G01/G02已逐项核对实际提交对象；G03身份由包含本页的提交确定。执行期间只回填本页，其他7351项授权输入字节/删除状态及11项个人文件元数据持续核对；不改其他正文、代码、冻结映射或压缩包。Git集合/对象核对不证明包内恢复或原始证据一致性，13项缺链仍保留。准确最终提交及剩余工作区状态在本轮会话汇报，不为回填自身hash追加第四个提交。
