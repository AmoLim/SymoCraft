---
tags:
  - area/meta
---

# 当前 Git 提交批次

## 授权与边界

日期：2026-10-11。分支：`feature-refactoring`。起点：`b01f0068b27447456cfb6fa1462fe9a6ea11c856`。

用户本轮要求先编写提交 spec，并明确选择「将现有实现与自动化迁移也纳入，按完整意图拆分提交」。本页仅规划这一批；旧批次审批不沿用，旧 `docs/spec/git/gitspec.md` 不恢复为现行规则，也不累加旧提交历史。

当前状态：**用户已确认并授权本批暂存与本地提交。** 实际分组、已有提交与本页所属收口提交见末节。只执行本地提交及本页规定的检查，不自动 push、merge、创建 PR、打 tag、改写历史、执行功能 change 的 sync/archive，或继续 M3 开发。

稳定规则见 [AGENTS](../../AGENTS.md)，文档归属见 [DOCUMENTATION_MAP](../DOCUMENTATION_MAP.md)，功能边界见 [scope](../scope/project-scope.md)。本页不是新的 OpenSpec 功能 proposal，不重开已有方案或登记游戏节点批准。

## 输入身份

本轮只读盘点从 Git 的 NUL 分隔 porcelain 状态取得准确路径，未读取个人笔记或 Obsidian 配置正文。下列数字描述 **Git 相对起点的路径变化**，不等同于文档迁移过程中累计搬移、清理或恢复的数量。

| 范围 | 路径数 |
| --- | ---: |
| 候选集合，不含本页及私有目录 | 8430 |
| 新增未跟踪路径 | 7417 |
| 已跟踪修改 | 73 |
| 已跟踪删除 | 940 |
| `docs/verification/archive/` 新归档文件 | 7219 |
| 归档外的 docs 变化 | 1007 |
| docs 外的实现、工具及规则变化 | 204 |
| 排除的个人目录与 Obsidian 路径 | 11 |

归档当前合计 **198597613 字节**，候选集合未发现超过 100 MiB 的单文件；这只是本地保存可行性检查，不证明 GitHub 上传或远端恢复。加本页后的候选路径数为 8431；执行时重新核对，不按数字凑提交。

候选内容集合 SHA256：`f38392e7ad5caa2336b03f8af22c6e33412eb971fec06e3a885d6acce40e2bb5`。

复核算法：排除 `docs/.obsidian/`、`docs/development-journal/` 和本页；逐路径记录 `path/status/size/sha256`，删除路径的 size/sha256 为 null；按 path 排序，以 UTF-8、`ensure_ascii=False`、`sort_keys=True`、`separators=(',', ':')` 序列化 JSON，再取 SHA256。它用于发现新增、消失、改字节或改状态的并发输入，不是提交后仍保持不变的状态摘要。出现 rename/copy 状态时须另核对源和目标，不能套用单路径算法。

提交前把确认后的准确路径、所属意图及内容身份记录到本地 `out/` 批次核对材料，不提交第二份长期计划。并发变化必须逐项说明并重新确认范围，不能因位于同一目录自动纳入。

## 拆分方案

按以下依赖顺序提交。路径范围是归属说明，**不是可以直接全目录暂存的授权**；准确集合只取上节已核对输入。共享文件按相应改动块分配，不能让一个提交引用尚未纳入的源码、脚本、安装文档或资源。

### G01：现代渲染生产候选

建议消息：`feat(renderer): 接入 D3D12 生产候选与同循环渲染桥接`

完整意图：将 R1 实验之后已有的 R2 生产候选、调用方与配套验证一起纳入版本控制，不宣称 R3、默认切换或 Vulkan 已通过。

- renderer 的公开类型/错误、PImpl、资源 registry、校验、私有后端与 D3D12 实现；旧 OpenGL 实现及头文件的隔离迁移，保留游戏默认。
- app 后端选择、启动选项、GameRenderer、CPU 输入转换与持久 WorldRenderCache；相应 main/application、assets 图像写出、telemetry 扩展。
- `assets/shaders/hlsl/`、`cmake/RendererD3D12.cmake`、`cmake/licenses/`，以及 root/game/app/renderer/assets 的关联构建、安装和模块边界改动。
- renderer/app/assets/telemetry 单测、公共头消费者、原生后端探针、游戏报告探针及构建矩阵的相关改动块。
- `scripts/package-d3d12-candidate.py` 与 `test/integration/renderer/` 的构建、窗口/游戏/候选验证和打包脚本；依赖的 `scripts_common.py`、`verification_common.py`、`windows_verification.py` 一并纳入，不能只提交入口。
- 同一共享工具若有单份证据归档相关行为，也须连同直接依赖及 `test/scripts/test_evidence_archive.py` 等适用回归纳入；文档消费者改链另归 G04，不因拆分重复提交或遗漏。

本组不包含 T4 CPU 基线，也不包含 13 份休眠源码的迁移清理。`test/CMakeLists.txt` 中通用 Python 回归注册归 G03；不得因暂存整个共享 CMake 文件提前引入不存在的测试输入。

### G02：旧 CPU 行为基线

建议消息：`test(simulation): 建立旧物理与玩法 CPU 行为基线`

依赖 G01 已纳入的公共验证辅助。完整意图是固定当前生产行为供后续 T4 对照，不实施新物理核心或 SimulationSession。

- `test/unit/simulation/cpu_baseline_fixture.h`。
- `test/unit/simulation/legacy_physics_baseline_tests.cpp`。
- `test/unit/simulation/legacy_gameplay_baseline_tests.cpp`。
- `test/unit/simulation/CMakeLists.txt` 的对应注册。
- `test/integration/simulation/CaptureCpuBaseline.py`、`VerifyCpuBaseline.py`。

15 物理 + 9 玩法组属于历史基线的用例范围，不是本轮重新运行成绩。冻结快照与交接文档的位置迁移在 G04 完整处理，不能把快照源码冒充新增生产实现。

### G03：Python 自动化迁移

建议消息：`refactor(automation): 统一 Python 自动化与验证入口`

依赖 G01 的共享辅助，完整纳入已发生的通用脚本迁移及其调用者：

- `scripts/` 其余构建、Benchmark、运行/故障验证、输入准备、实验/平台/人工输入辅助的 Python 入口与既有 PowerShell 薄启动器改动。
- `test/experimental/` 对应 Python 替换、已发生的旧 ps1 删除及关联 CMake/README；不增加 PowerShell 业务逻辑，不改历史证据中的旧命令。
- `test/integration/` 通用增量、迁移运行与 SDL 交付验证的 Python 替换及旧入口删除。
- `test/scripts/test_migrated_scripts.py`、`test/benchmark/benchmark_script_tests.py`，以及 root test/benchmark/实验 target 的对应 Python 注册；`test_developer_scripts.py` 与文档消费者整体后移 G04。
- `.gitignore` 的 Python bytecode 排除；不忽略归档或必需输入，不变更全局 Git 配置。
- `scripts/README.md` 同时引用新文档权威位置，整体归 G04，与目标一起纳入，不能留下缺链。

`capture-migration-inputs.py` 的新文档输入、对应消费者测试以及 Benchmark 安装改链归 G04；若新文件中的这些部分无法无损拆分，连同该入口、调用者与回归一起后移到 G04，不能临时改造业务来凑提交。

### G04：OpenSpec 文档治理与证据单份归档

建议消息：`docs(governance): 收口 OpenSpec 文档与证据单份归档`

依赖 G01–G03 的实现及工具输入，使现役说明与已提交代码相符。完整意图包含迁移目的、消费者、证据保存和已授权退役，而不只是 Markdown 文件：

- 根 `AGENTS.md`、`README.md`，`docs/DOCUMENTATION_MAP.md`、`docs/README.md`、`scope/`、`architecture/`、`issue/`、第三方说明、标签索引与必要阶段/人工入口改链。
- `docs/openspec/` 的唯一配置、两套 schema、六份现役规格、T3/T4/暂停 Fix1 目标，以及已完成文档迁移 change 的归档；不执行新的 sync/archive，不修改已归档四份原文。
- 根 `.agents/skills/` 的六份已生成技能及 `.openspec-target`；按当前输入核对，不手工改生成文件，不创建另一规划根或技能根。
- `tools/openspec/` 的入口、文档/工作流/消费者/证据路径检查及对应回归；复核 G01 已纳入的证据辅助/回归，以及 G03 的归档收集器适配，不遗漏非 Markdown 工具。
- `scripts/README.md`、`tools/benchmark/README.md`、Benchmark CMake 安装输入改链，输入捕获及对应隔离消费者回归；安装输出文件名与游戏协议不因此改变。
- `docs/verification/` 的唯一索引、共享方法/场景、7219 份准确归档输入；`.gitattributes` 的 archive `-text` 保护必须与归档入库同时成立。
- 已发生的旧知识/证据源删除及只读规格退役标记/迁移；10 篇只读旧规格与旧索引继续作为复核来源，不清空 `docs/spec/`，也不恢复旧平行政策链。
- `game/app/legacy/` 已获准确清单授权的 13 份非活动 `.disabled` 删除，与其恢复身份和处置映射一起入库；不扩为活动 legacy、ECS/内存清理或 T5 验收。
- 本页 `docs/git/gitspec.md`；本批结果只更新本页，不追加旧批历史，也不篡改冻结处置映射。

证据身份及准确处置依据只引用 [VERIFICATION](../verification/VERIFICATION.md)、[单份归档映射](../verification/archive/evidence-consolidation-20261011.json)、[退役身份](../verification/archive/project-history-20261011/retirement-manifest.json)。940 个 Git 删除路径不能自动等同于历史获授权的 6925 个旧源删除；未提交新文件的清理和 Git 已跟踪删除须分别解释。

若共享 CMake、脚本或安装输入有不可拆的依赖环，先汇报准确关联，再将相邻组按完整意图合并，不能硬拆出失效中间提交。未经实际验证，不声称四个中间提交各自构建或测试通过。

## 排除与保护

- `docs/development-journal/`、`docs/.obsidian/` 全部排除；不读取个人正文、不暂存配置/插件、不回退用户改动。现有标签索引的私有注册不新增个人内容。
- `out/`、`.temporary`、IDE 状态、机器专用 presets、缓存、原始返回现场、唯一完整运行包和暂停 Fix1 现场不提交、不自动删除或迁移。
- 归档 raw 中必要截图、日志、JSON/YAML、选录配置/资源属于冻结证据，不能按扩展名一刀切排除；稀疏快照不能代替完整运行包。
- 旧清单中的包缺失、负向数据、T4 README 历史声明差异、旧 source 路径及旧命令保持原义；不补造文件或改字节使校验全绿。
- 已登记冻结桌面报告的两处改链例外，以保存的改前原文和 revisions 身份核对；本批不再新增冻结修订例外。
- 不使用 `git add .`、`git add -A` 或整目录无差别暂存；不归一化全库换行，不替换已生成官方技能，不混入批准后出现的并发新文件。

## 执行前检查

1. 用户确认本页后，重读最新状态和输入身份；确认 HEAD/index 没有其他提交者或 Agent 的并发修改。保存当前分组的准确路径和内容身份，已有暂存变化不擅自撤销。
2. 检查每组必要源码、Python import、CMake source/install/test 输入与文档消费者闭包；共享改动块无损分配，不更改工作区来制造中间状态。
3. 对准确候选检查凭据/私钥、意外运行包和大文件；历史绝对机器路径只按证据身份解释，不把明文凭据作为证据上传。发现唯一必要大文件的保存缺口时报告，不静默省略。
4. 执行文档链接/锚点检查，以及适用的 Python 离线回归、OpenSpec 严格只读校验；使用 Python 3.10+ 和明确 UTF-8，不运行 `update`、`--write-index` 或真实 sync/archive 来顺带改材料。新增本页若需索引同步，仅窄更新当前索引并核对，不改旧索引。
5. 逐项核对外层归档清单、完整文件集、大小/SHA256及必要消费者；分别解释内嵌稀疏/负向状态，未知缺失拒绝。按 archive `-text` 确认实际暂存 blob 与工作区冻结字节相同，不能只看工作区哈希。
6. 每次提交前查看准确 staged diff、名称/模式/删除及必要二进制身份，检查忽略状态与私有路径没有混入；不对冻结 raw 的历史行尾空白做自动修复。
7. 提交后核对该提交文件集、工作区保留内容与下一组输入；将 commit hash、实际检查、未执行项和残余风险回填本页。本页尚未产生的结果不预填。

## 本轮检查与限制

已完成：读取现行规则、范围与迁移/验证入口；完整候选路径/内容盘点、凭据模式和大文件筛查；准确暂存集合、删除/模式、内容与实际提交 blob 核对。本批所用本地核对材料位于 `out/git/20261011-b01f006-approved/`，不提交第二份长期计划。

实际解释器为 `C:/Users/ldh20/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe`，使用 `-B -X utf8`。`python` 不在 PATH，`py -3` 未发现已安装运行时；没有修改 PATH 或全局配置。OpenSpec 使用项目 wrapper 和已安装的 `F:/nodejs/node_global/node_modules/@fission-ai/openspec/bin/openspec.js`，不执行项目 update、sync 或 archive。

| 本批实际检查 | 结果 |
| --- | --- |
| `-m unittest discover -s test/scripts` | 62项通过；首次受限会话61项通过、Windows EnumWindows 1项报错，正常 Windows 会话重跑全部通过，不改测试或业务 |
| `-m unittest discover -s tools/openspec -p 'test_*.py'` | 33项通过 |
| `test/integration/renderer/test_candidate_scripts.py` | 24项通过，离线候选协议夹具，不启动窗口或 GPU |
| `tools/openspec/check_workflows.py --cli-entry ...` | 23项通过；CLI 生命周期仅在隔离夹具运行，未操作真实 change 或托管技能 |
| `tools/openspec/run.py --cli-entry ... validate --all --strict` | 9项通过、0失败，规格语法不是工程验收 |
| `tools/openspec/check_documents.py` | 94篇正文、1940处普通链接、147处 wiki 引用、30处已登记历史入链，errors/warnings 为空 |
| 当前标签索引同步 | 只新增 `git/gitspec.md`；managed 47→48、meta 3→4，无删除或其他字段变化，私有注册不变 |
| 凭据模式筛查 | 非私有候选未命中私钥、GitHub token 或长 OpenAI key 模式；不是全面机密审计 |
| 迁移与退役身份 | 7198项目标大小/SHA256、10篇只读旧规格、4份已归档 change 原文及9篇退役原文均吻合；13份休眠源码的历史 Git blob 与声明恢复字节吻合，未恢复到工作区 |
| 7份外层清单 | 声明 payload 文件集与大小/SHA256吻合；根级 README、初始清单及历史 check 文件另按准确已确认输入身份核对，不冒充清单 payload |
| G01 / G02-G03 暂存与提交 | 88 / 80项准确路径及实际 commit blob 已核对，无范围外输入；空白检查通过 |
| G04 暂存 | 8264项准确路径，其中912项删除；7219份归档 staged blob 与工作区原字节完全一致，archive `-text` 同组入库 |
| G04 空白检查 | 归档外通过；完整 `--no-renames --no-textconv --no-ext-diff --check` 返回2，1467952处提示全部在冻结归档，保留原有 CRLF/空白，不自动修复 |

以上离线回归共142项通过，均针对本批完整工作区，不宣称任一中间提交分别构建或通过同样测试。迁移报告中的旧118项等仍为历史事实，不替换成这次成绩。

本轮仅为本页明确生成当前标签索引，并比对确认唯一新增项；未顺带改旧索引或个人内容。Windows 长路径首次暂存失败，暂存区未改变；用单次 `git -c core.longpaths=true` 重试成功，不写仓库/全局配置。保存原始 staged diff 时，本机 `.dot` textconv 缺少 `file` 工具而失败，改用单次 `--no-textconv --no-ext-diff` 保存实际原始差异，不修改转换器配置。Git 的重命名显示仅是表示方式，准确核对统一按源删除/目标新增处理。

未执行：push、远端恢复、代码构建、CTest、游戏/GPU/性能或人工验收、真实项目 sync/archive；专用 Benchmark 原生 fixture 回归也未执行。直接调用该回归入口未提供必需参数，入口拒绝，不能计为测试通过。本批未重新核验34份内嵌运行包或13项原位置完整运行包；保留既有稀疏/负向状态与 T4 README 声明差异，不用外层字节一致性冒称内部清单全绿。

仍保留：OpenGL 游戏默认、D3D12 opt-in 候选、Vulkan 未实现、R3 整组及现代双后端未验收、T4 仅旧行为基线交付、Win32 1175 与暂停 Fix1 等风险；文档迁移或提交不改变这些边界。

## 提交结果

| 实际组 | 消息 | 提交身份 |
| --- | --- | --- |
| G01 | `feat(renderer): 接入 D3D12 生产候选与同循环渲染桥接` | `ac0fd9181954c57b7e77a138c4445f00c77008d0` |
| G02-G03 | `refactor(automation): 统一 Python 验证入口并建立旧 CPU 行为基线` | `ca458d9ddad08647854d2669fa1737cc3161c1d7` |
| G04 | `docs(governance): 收口 OpenSpec 文档与证据单份归档` | 本页所在的同名提交；不在页内预填自身 hash，以 Git 历史定位 |

分组调整均在已确认输入集合内：两份 SDL3 归档收集器随其直接回归前移 G01；复核发现 CPU 捕获入口记录 `scripts/build.py`，共享 `verification_common.toolchain` 也延迟导入它，故按已约定依赖合并规则将 G02、G03 合为一组，执行时已在会话说明。G01 的共享 toolchain 完整运行依赖后继合并组；没有声称 G01 的新脚本可独立全部运行，也没有为补齐顺序改写已提交历史。

`test/CMakeLists.txt` 只在索引中无损拆分：合并组纳入 migrated_helpers；G04 纳入新文档消费者的 python_helpers，工作区字节始终不改。实际3组覆盖8431个唯一候选路径；共享 CMake 在后两组各有一处不同注册，不能把组路径数相加误当唯一数量。

11项个人目录/Obsidian变化全部排除，本轮只核对路径、大小和修改时间，不读取正文、不回退或暂存。准确暂存材料保留在本地 out；本页与当前标签登记是执行期间仅有的候选内容更新，不改冻结处置映射。本页所属 G04 的提交后身份和剩余工作区状态由实际 Git 历史及本轮会话报告确定，不自造循环 hash 或增加另一个批次。
