---
tags:
  - area/spec
---

> 2026-10-11 旧导航引用已失效，已改指现行权威或明确归档入口；历史命令、结果和批准含义保持不变。

## Context

Symocraft 已有实现、冻结证据和未完成目标，不能照搬参考项目的空现役基线。迁移建立精简、限定覆盖的 current specs；目标与继续有效验收保存在 changes。没有规格覆盖不代表没有代码，本轮静态核对不冒充重新构建或运行。

## Technical Approach

### 来源映射

下表覆盖原 docs/spec 全部 14 篇 Markdown；tag-inventory.json 是生成索引，现役位置转为 docs/tag-inventory.json，不构成规格。2026-10-11 获准清理的85份中包含4篇旧 spec，其余10篇及旧生成索引仍保留，仅为复核，不得继续从旧链维护任务。下面临时路径描述各批当时的复核现场；追加清理后仅保留最小处置映射/身份/结论及限制，不再保证全部旧笔记全文恢复，最新状态见最后一节。

| 原位置（相对 docs/spec） | 新权威归属及处置 |
| --- | --- |
| M3/spec.md | 各 change 的 proposal 维护依赖，tasks 维护任务；docs/README 仅导航，不建第二阶段计划 |
| M3/M3-T0-模块软硬边界.md | [module-boundaries](../../specs/module-boundaries/spec.md) 与 [engineering](../../../architecture/engineering.md)；实现/历史批准仍各按证据 |
| M3/M3-T1-世界模块重构.md | [world](../../specs/world/spec.md) 现役契约；原候选对比及点时意见只供历史复核 |
| M3/M3-T2-SDL3迁移.md | [platform](../../specs/platform/spec.md)、[VERIFICATION](../../../verification/VERIFICATION.md) 及 Issues；不重开已批准迁移 |
| M3/M3-T2-Fix1-spec.md | [Fix1 change](../m3-t2-publication-fix1/proposal.md)、[foundation-files](../../specs/foundation-files/spec.md)；仍暂停，不恢复调查 |
| M3/M3-T3-渲染器重构.md | [T3 change](../m3-t3-renderer/proposal.md)：目标、R阶段及 A/M/E 验收；现役 [renderer](../../specs/renderer/spec.md) 不含 Vulkan 目标 |
| M3/M3-T1-T3-CPU契约清单.md | world/renderer 现役 specs 和 T3 design 的消费者交接，不另维持清单计划 |
| M3/M3-T4-应用平台与模拟模块接口.md | [T4 change](../m3-t4-application-simulation/proposal.md)：D01-D13、A01-A27、Q待选与单一计划；[simulation](../../specs/simulation/spec.md) 仅已有旧生产行为 |
| M3/decisions.md | T3 change decisions；跨任务编号保持任务限定，不重新编号或自动批准 |
| md-organize/mdspec.md | 本 change；前轮三域试点已复核，未等同全库收口 |
| md-organize/data-system-migration-spec.md | 已退役约定不作为待实施任务；必要值/借用/失败机制留现有模块，现行规则只在 DOCUMENTATION_MAP |
| md-organize/architecture-template-coverage-spec.md | 已取消逐函数覆盖/强制模板；旧进度与流水账只供复核，不迁成活动任务 |
| project-architecture/面向对象与数据导向架构设计初稿.md | 已实施契约→current specs/架构，已形成目标→T3/T4；剩余候选→[future-scope F03](../../../scope/future-scope.md#f03-跨模块架构候选)，旧选型快照保留不重开 |
| git/gitspec.md | 稳定 Git 规则只在 AGENTS；本轮无提交批次，不预建 docs/git/gitspec.md 或虚假 Git change |

### 路径与工具

tools/openspec/run.py 从自身定位代码根，以 docs 为 CLI 工作目录；原生根发现不向下寻找，项目根直接调用不能用于建 change。拒绝重复根、缺配置隐式初始化及直接 init。
config/schema 提示路径相对 planning root；Markdown 相对各文件实际位置。原生 allowedEditRoots 保持 docs，正式功能 apply 仍须取得代码根范围授权，不改写 JSON。
首次技能由官方 init 在隔离目录生成，隔离全局配置，核对文件集及字节后发布到根 .agents/skills；不把初始化临时 openspec 复制到工程根。
update 仅在隔离目录刷新官方托管文件，先缓存全部生成字节，再全量复核输入，每次回写前重查文件集、链接及预期字节；检测到集合增删、链接路径或并行修改即拒绝，其他技能和用户全局配置不变。
这不是锁、原子比较交换或多文件事务；update前须暂停其他会话对托管技能的编辑。检查与写入间仍有竞态窗口，中断不自动回滚已发布文件，以免覆盖新变化；须保留现场复核后重试。

### 状态与正确性

当前基线记录调用者前提、已实现保证和已知失败边界，未来强保证不反填。重要所有权/生命周期/失效/失败/线程同步机制继续留模块架构与必要 target design。
T2 批准、R1冻结/R2入口、T4局部CPU授权分别核对；T3 R3/R4、Vulkan、默认切换和 T4 独立 physics/会话不标完成。Fix1 暂停不是风险关闭。
冻结 evidence/archive 默认保持原字节与历史命令，链接只调整可维护导航；2026-10-11 的单文件两处引用例外见下节，不推广为冻结件编辑权限。13769份既有冻结文件已纳入本轮起点哈希，未提交唯一旧正文保存于 .temporary/openspec-migration-20261010/sources，不假定 Git 已记录。

## Risks And Compatibility

schema 命令带上游实验性提示；Windows 沙盒可能使 realpath 误报越界，按环境批准复验，不削弱检查。
隔离样例验证创建/依赖/任务/归档/current specs 生命周期，不归档真实任务。迁移保护检查保留既有 dirty 改动、源项目只读身份和 HEAD。
ProjectShadow参考文件出现用户确认的其他会话更新时，保留起点哈希并单列外部差异，不回退来源或用后来的字节覆盖迁移输入；目标项目仍以本轮实际CLI及本地验证为准。
新会话只读复核已确认 Benchmark 安装及输入捕获真实引用旧操作文档与依赖盘点。用户已批准本 change 内的窄消费者适配，准备阶段未删除原件；安装输出名、捕获协议及依赖/许可证事实不得改变。
运行/GPU/性能、人工批准、Obsidian UI/云同步均不由工具或链接通过推定。此前85份清理阶段保留未列清单的旧源、临时 context、基线及一次性脚本；最新追加权限与尚未执行的前置见“授权残留材料归档与清理”，不将两批范围混用。

## Decisions

### 引用修订与下一批候选

2026-10-11 用户明确确认：只允许修改 verification/archive/benchmark-20261005/reports/Benchmark-桌面交付验证.md 中原 [Benchmark-执行与导出](../../../architecture/benchmark.md) 与 [Benchmark-文件协议](../../../architecture/benchmark.md) 两处导航，其他历史文字不改。改前5518字节原文和原身份保存在 verification/archive/docs-navigation-20261011；revisions.json 记录原/新 SHA256 与准确替换，逆还原必须与原字节相等。索引仅在 VERIFICATION，不把链接修订变成新实测、批准或重新冻结产品。
可维护来源的导航改指已核对的当前/归档正文，并加一句旧入口失效提示；不改历史命令、勾选或结果，也不留下单句空壳。docs/legacy 的10份及此前暂缓10份须全文比对，必要独有历史按 A 类最小归档保存，当前机制归模块；只有满足这些前置才进入新增候选。此为候选形成时的前置约束；后续明确授权仅覆盖准确85份，当前处置见“剩余来源处置”，不推广为整库清场权限。
本阶段独立起点在 .temporary/docs-navigation-review-20261011；此前起点及审计规则不覆盖。旧审计因本次授权引用修订产生的差异须如实保留，独立审计只豁免该冻结报告的准确两处替换，其他冻结/生产/技能文件继续逐字节核对；可维护旧源仅按记录的导航编辑逆还原，不把全文排除使检查变绿。

### 后续浓缩与消费者适配

继续使用本 change 和唯一 tasks，不新建全库整理计划。原 OpenSpec 起点快照不覆盖或弱化；本阶段独立起点在 .temporary/docs-closeout-20261010，保存可维护原文及全量输入哈希，明确允许两项消费者路径变化，其余生产输入、官方技能与冻结字节保持。
原起点审计仍如实将捕获脚本和既有测试夹具报为 production input 差异，不修改其规则或旧哈希使结果变绿。后续授权由本阶段独立审计和窄 diff 证明：CMake 仅两项安装源；capture 仅两项源及清单分隔符规范化；既有夹具仅两路径及六行清单断言，逆还原与起点 SHA256 相等。

| 原知识域 | 新权威位置与保留边界 |
| --- | --- |
| project/architecture 的 foundation/scene/assets/world | architecture 同名扁平模块，保留定义/值/借用/失败和生成机制；公开行为引用 current specs |
| project/architecture 的 ecs/simulation/platform/telemetry | architecture 同名扁平模块；旧 ECS ID/const/OOM 风险、共享预算和可变记录是现状，不写成 T4/T5 强保证 |
| project/architecture 的 app | architecture/app.md：组合、两条渲染路径、编辑顺序、关闭和部分失败；非实例化 Application 目标 |
| project/architecture renderer/build 及 project/build | 已有 architecture/renderer.md、engineering.md；操作只归 README/scripts，不把命令教程塞进架构 |
| project/benchmark 非冻结操作说明及旧第三方盘点 | tools/benchmark/README.md 与 docs/third-party.md；包内操作自足、许可证来源不丢，实际消费者改读新位置 |
| milestones/testing/legacy 与 Benchmark 历史实验 | 逐项区分必要历史/证据、当前方法、已迁移机制与过时过程文字；未覆盖或唯一原文先保留，不为文件预算强删 |

先完成模块与状态核对，再适配两个消费者并测试，随后重定向可维护导航及生成索引，语义交叉复核和保护检查后在聊天列保留/迁移/待删除候选。旧笔记本轮仍为复核源而非第二权威，不批改历史勾选、命令和冻结内嵌来源。冻结查验快照不冒充完整 EXE/数据包；原 out 运行包和详细原始数据未具可靠替代前不列删除项。

路径/标签/锚点全绿不证明知识完备；每模块结合源码确认所有权、寿命、失效、失败后状态、线程/同步和稳定不变量，留下实际缺口。消费者验证检查真实安装规则的隔离执行与捕获清单/复制；既有 test/scripts/test_developer_scripts.py 捕获夹具同步两项路径，新增窄回归属于验证工具。新 README 同时在 rg 清单中，Windows 原路径分隔符导致重复登记；仅增加清单入 set 前的 Path.as_posix 规范化，不改变捕获协议。测试中 Git 写入仅在临时夹具仓库，项目 Git 不写。不冒称原生 Benchmark 编译、游戏测试、GPU 或部署验收。此前已执行删除仅限准确85份授权；后续授权不追溯扩大该批历史结果。

### 剩余来源处置

下表保留引用修订前的来源分类，不是本轮最新保留状态。原来源命令、勾选、结果和点时数字保持；生成索引对仍存在的复核源检查全部链接/标签，仅移除实际获准删除的文件，不靠排除使审计变绿。逐文件对应与起点字节在独立临时基线记录，不另建持久迁移报告。

2026-10-11 完整汇报后用户明确授权清理上一批65份、原暂缓10份及仅限 docs/legacy 的10份；准确85份已删除，不按目录递归清空。必要机制已补入当前模块；四份来源的六段独有历史原文最小归档，从 VERIFICATION 唯一索引进入。保留文档旧导航已改链并加一句失效说明，不保留导航空壳。冻结桌面报告仍仅按先前授权修订两处技术引用，保留原字节及新旧身份；6份报告副本以原报告字节证明，不冒称它们与修订后的正文仍完全相等。清理前独立起点、85份恢复副本和清理后保护检查留在 .temporary/docs-cleanup-20261011，不覆盖旧审计；旧审计记录仍按当时文件集合解释。该批完成时其余历史来源、10篇旧 spec、13份 .disabled、冻结证据、原 out 包及旧临时现场均保留；本批不是全库无残留、功能 sync/archive 或游戏关键节点通过。

| 来源组 | 引用修订前分类与当时保留理由 |
| --- | --- |
| project/architecture 及 project/build 的学习/构建笔记 | 当前机制迁入扁平模块；停止旧逐类/函数文档维护。含 T0 人工、GLFW 表、短采样或构建失败的来源先保留，不能因机制已迁移认定整组没有独有历史 |
| foundation/scene/assets/world 的17篇 | 17篇逐份映射；功能摘要的历史结果另完整对照 T0/T1 报告，未发现独有原始成绩/批准。保留 T0/T1 及证据后可列下一轮待授权删除候选，本轮仍保留原正文 |
| app/App-休眠源码清单.md | 保留独有 13 个休眠源 SHA256，从 VERIFICATION 导航；不列为机制重复删除项，不删除休眠代码 |
| project/benchmark 的机制、操作及 testing | 机制→benchmark/telemetry，操作→工具 README，当前方法/共享场景→verification。原 1080 修复与测试点时环境/结论未逐项替代前保留 |
| Benchmark-1080p窗口修复.md、testing/performance-baseline.md | 前者独有1061首错、19px原因未证、高DPI缺口/人工确认/复现；后者独有旧strict/v1完整交接命令。暂保留，不凭新机制/方法摘要认定已完整替代 |
| project/benchmark/exp 六篇 | 当前归档入口指向原字节 archive/reports；独立核对剥除两轮过渡/退役提示后，六份正文均与归档逐字节相等，可列下一轮待授权删除来源副本。保留 archive/证据及原 out 包/逐帧数据，不以摘要替代完整来源 |
| legacy/architecture 九篇与旧 third-party | 当前机制及来源已提炼，但独有 M0 风险、M1 失败/复测及 M0-M2 二进制附录保留；不是整体可删组 |
| testing/gameplay-smoke.md 与 milestones 非冻结 Markdown | 当前方法/目标已有唯一位置；原人工反馈、环境/失败、阶段原批准与 P01 基线为必要历史来源，从唯一 VERIFICATION 索引进入。根 r2-* 与 r2/ 重复是否可删须逐字核对，不推定相同 |
| 原14篇 spec 与旧生成索引 | 当前契约/目标/治理已分类迁移；仅为复核。未提交唯一正文有可靠起点副本，但完整汇报、新会话交接验证与新的删除授权仍是前置 |
| evidence、verification/archive、原 out 包及逐帧数据 | 保持字节及原失败状态，不删除或补造；稀疏查验快照不是完整运行包 |
| 临时 context、两次基线与一次性检查/迁移脚本 | 保留复核现场，终态审核通过和新的清场确认前不移除 |

“退役”仅表示不再作为当前权威，不等于“可删除”。新会话未读旧源也应能从核心导航识别当前系统、限制、活动任务与验证；确需历史证据时按索引展开。独有历史须先有最小充分可靠归档，否则报告为保留项，不强行完成 DOC-07。
独立交叉复核还纠正了 Selected asset preflight 的迁移过度概括：原 T3 只要求按后端检查文件，并未要求 legacy 拒绝空文件；现役/目标 scenario 明确 legacy regular_file 与当前 D3D12 非空检查，不改变真实目标或游戏实现。现代 prepare 是门面加后端的具名总计时，非仅后端直接计时，telemetry 同步准确口径。

### 授权残留材料归档与清理

用户最新明确确认9篇文档归档、1份迁移清单清理、13份休眠源码及有保留依据的运行包副本/迁移临时现场清理。此节规定准确边界；DOC-12至DOC-16是唯一任务入口。上节历史表不因追加授权改写为最新状态，旧检查的输入集合和结果不追溯调整。

本批9篇原文已逐字节归档，完整TSV及13份休眠源已删除；生产源码边界和休眠源入target拒绝探针通过。7组整包重复副本的277份文件已清理（54,904,598字节），保留包、冻结ZIP及实际当前Debug004/Release002评审包已核实，外层映射登记在VERIFICATION的退役项目材料入口。665份旧迁移临时文件已清理，最小处置映射及历史审计限制已转存；旧快照/结果仅留下点时身份和必要结论，不再声称完整重放旧审计或恢复全部学习笔记。本批61项回归和10项严格校验重新通过；保护核对发现范围外 docs/.obsidian/workspace.json 变化，未写入或回退，不能将其报告为未变化。最终身份核对与任务状态按 DOC-16 及同一机器记录确认，不以历史检查替代。

#### 文档与恢复身份

9篇准确来源如下，路径相对 docs/project；归档目标为 docs/verification/archive/project-history-20261011 下相同相对路径：

| 原相对路径 | 保留的点时知识 |
| --- | --- |
| architecture/app/App-休眠源码清单.md | T0的13份休眠源身份与非活动归属 |
| architecture/build/构建与测试边界.md | 旧构建矩阵、边界拒绝及增量结果 |
| architecture/build/构建契约设计.md | B1-B6及构建失败/回滚限定 |
| benchmark/Benchmark-1080p窗口修复.md | 1061首错、19px原因未证及高DPI未覆盖 |
| benchmark/testing/performance-baseline.md | 旧strict/v1完整交接方法和命令 |
| benchmark/testing/reproducible-scenes.md | 固定seed、路线、检查点及编辑序列 |
| build/build-and-clion.md | M1工具链、IDE与依赖盘点失败/恢复 |
| build/CMake-模块边界.md | 点时target图及隔离验证 |
| build/CMake-自定义构建目录日志修复.md | 自定义目录故障及当时CTest复测 |

先保存源/目标相对路径、字节数、SHA256并证明9份目标逐字节一致，再移除准确9份原位置。原正文、命令、结果、勾选与失败状态不改；内嵌相对路径按原来源父目录解释，外层路径映射负责定位，不批量修订归档正文。可维护入链在实际迁移时改指新历史入口；VERIFICATION维护唯一索引，不新增归档README、学习版或日常总结。

用户另行允许只为已登记的只读旧规格适配历史路径核对：同时校验旧规格身份、归档映射、目标大小/SHA256及锚点；现役文档、未登记来源及未知缺链不能借此绕过错误。该解析不改旧规格正文，不恢复旧路径空壳。

完整 source-migration.tsv 不归档；删除前只提取恢复13份源码所需提交身份、原/current路径、字节数/SHA256及换行还原方法，保存于同一归档的最小机器可核对身份记录。已核对提交为 b01f0068b27447456cfb6fa1462fe9a6ea11c856；Git对象为LF文本，按已验证方法还原CRLF后再核对原SHA256，不能宣称Git对象字节直接等于工作区。执行前重验，不用过去复核代替本批身份检查；发现不一致或其他独有必要信息先保留并报告，不强删。

273份 project/benchmark/evidence 原副本仍保留，既有 archive/migration-manifest.json 和冻结字节不改；其余10篇 docs/spec 及旧生成索引也不随本批删除。

#### 准确源码范围

本轮独立代码根权限仅覆盖 game/app/legacy 下以下13份；不改CLI的docs授权输出。先重查活动编译、安装、调用及相关T4依赖，保留可靠恢复身份，再删除准确清单，并窄修订可维护文档中针对这13份的旧保留约定。

| 相对 game/app/legacy 的目录 | 准确文件名 |
| --- | --- |
| aggregate/ | core.h.disabled、utils.h.disabled |
| event/ | event.cpp.disabled、event.h.disabled |
| input/ | input.cpp.disabled、input.h.disabled、key_bindings.cpp.disabled、key_bindings.h.disabled、key_handler.cpp.disabled、key_handler.h.disabled |
| thread_pool/ | global_thread_pool.cpp.disabled、global_thread_pool.h.disabled、pool.hpp.disabled |

活动AmoBase/RawMemory/ECS、legacy OpenGL、历史源码快照和冻结payload均不在范围内；拒绝休眠源重新进入活动target的边界检查继续保留，不将删除视为T5全审或T4撤旧实现完成。生产输入捕获会反映13份减少，不修改旧清单/哈希或弱化D3D12打包检查。保留包/当前候选仍是删除前的点时身份，不自动成为新源码树候选；新包须另行授权真实重链接并生成新构建身份。本批不构建/运行游戏或生成新包，未执行的动态验证如实报告。

#### 运行包与临时现场

先按实际活动任务、冻结记录和包内清单识别必要身份及当前候选，再逐包核对文件集合、资源/运行依赖、源码/构建/包身份、原始数据与失败材料引用。相同EXE、目录名较旧或稀疏查验快照不能证明整包重复/可替代。只有整包重复经字节核对，或用途确已退役且无独有必要身份/证据的副本才可清理；每项须有保留位置与核对依据。每个必要冻结身份保留至少一个已核实完整包，当前候选继续保留；无法证明完整性/替代关系的先保留，聊天报告局限。历史包清单/命令不改，必要旧路径从外层身份映射定位到保留位置。不按目录时间、通配符或整个out递归清空，暂停Fix1现场、当前构建、原始采样与解释关键结论的失败样本不自动清理。

临时清理范围限文档迁移复核：候选审计目录均相对 .temporary，为 openspec-migration-20261010、docs-closeout-20261010、docs-closeout-review-20261011、docs-navigation-review-20261011、docs-cleanup-20261011；另包括 docs/openspec-migration-context.tmp.md 与逐项确认归属的迁移一次性辅助文件。不自动删除共享 __pycache__、其他任务现场或 .temporary/*-result 真实返回数据。
清理前从旧现场保留准确85份处置映射、本批文档/源码/包身份、必要核对结论及仍有恢复意义的唯一材料；引用中的旧临时清单改指持久最小记录，记录由VERIFICATION唯一索引进入。已有必要历史摘录不重复归档；未提交唯一正文须核对是否已充分迁移，不能仅以“Git有历史”清除。旧基线/结果不覆盖、不把历史计数或失败重算成绿；只保存解释必要结论的材料，不将整套临时流水账永久复制为第二文档树。仍有唯一必要证据无法充分转存时保留对应项并报告。

本批开始前另存独立输入身份，冻结/官方技能/个人设置/其他生产文件与项目HEAD/index继续保护。归档及删除按准确路径逐项检查：最终绝对目标须在项目内，遇重解析路径、身份漂移或并行修改停止对应项，不递归触及未列文件。重跑文档链接/索引、真实安装/捕获消费者、工作流回归与原生严格校验，并按准确允许差异查验保护范围；验证临时现场自身最后处理，尚需检查的输入不得先删。全部检查及保留/受阻项在聊天如实报告，不由CLI规划完成、历史绿结果或任务勾选推定全库收口/游戏验收。

### 必要证据单份归档与清理

2026-10-11 用户确认采用 verification/archive 单份保存、VERIFICATION 唯一索引，不再要求 out 结果复制到平行 evidence。现行规则只在 [DOCUMENTATION_MAP](../../../DOCUMENTATION_MAP.md#证据单份归档)；本节规定单份归档批的迁移机制，DOC-17至DOC-21实际实施状态只见tasks。拟定时仅获准修订三份规划文件；后续apply及准确清单另经本轮聊天授权，不修改其他活动 change，也不执行 Git 写操作、Obsidian 配置修改或真实 OpenSpec sync/archive。

#### 分类与保存范围

实施前独立盘点各 evidence、archive 及相关 out 原始数据、日志、快照和包身份，记录完整文件集、大小/SHA256、来源及所有消费者。逐项区分必要归档、已证重复、过期且无独有必要信息、工作输入/当前候选和无法判定项；273份 project/benchmark/evidence 副本明确进入复核范围，但不是整组删除结论。不要将 out 全树、第三方输入或其他任务现场当作证据垃圾清理。

必要归档按所支持结论的最小充分范围保留原始数据、版本/环境/配置、复现依据、来源清单及关键失败样本；摘要、相同EXE或相同文件哈希不足以证明整组可替代。相同字节可能承担不同点时来源身份，外层映射必须保留必要身份与语义，不为物理去重删掉复现依赖。日常全部尝试不是默认归档对象，也不新增学习版或过程总结。

当前候选、当前构建/工作输入和暂停 Fix1 原始现场先保留原位置；必要唯一完整运行包必须继续可定位且完整，稀疏查验快照不允许替代。已结束用途的完整包若确属必要长期证据，可在身份、容量、消费路径及完整恢复条件成立后单份归档，不因禁止平行 evidence 副本而删除唯一包。包仍是运行产物时留 out 并由索引引用，不强搬当前评审包或重新打包。本批不启动暂停调查、构建、运行或采样。

#### 原字节迁移与历史定位

已在 archive 的必要材料优先复用，不另建同内容证据树。以新的最小外层处置映射登记旧位置、唯一目标、原字节大小/SHA256、点时来源基址和保留/删除依据；从 VERIFICATION 进入，不新增平行归档入口。现有 migration-manifest、retirement-manifest 及冻结内层清单是历史事实，不覆盖或回填本批结果。

迁移允许先临时复制、核对完整目标后再按准确清单移除源；这是安全搬移过程，不是保留两份权威。源/目标必须在允许路径内，遇重解析、同名不同字节、并行修改或校验失败停止对应项且保留源。既有稀疏选录范围、负向夹具缺失/损坏、历史失败和内嵌 package 身份保持原义；不得填充原未选录文件使检查全绿，也不得修改原命令、来源字段或历史批准。

可维护入链与当前消费者指向唯一目标；冻结内嵌路径按原来源基址和严格登记的外层映射解析。验证须核对来源身份、目标大小/SHA256、必要锚点与完整文件集；未知缺链、目标变更、未登记路径必须失败，不开放任意历史路径兜底、不制造旧位置空壳。现有仅覆盖9篇归档/只读旧规格的解析例外不自动适用于全部证据，扩展须窄实现并补正负向测试。

#### 收集器与计划依赖

现有 [D3D12收集器](../../../../test/integration/renderer/archive-d3d12-candidate.py)、[SDL准备收集器](../../../../test/experimental/sdl3/collect-evidence.py)与 [SDL输入收集器](../../../../test/experimental/sdl3/collect-input-evidence.py) 仍向阶段 evidence 复制 out 材料；下一 apply 应适配到唯一 archive 路径并核对全部实际路径消费者及 [操作说明](../../../../scripts/README.md)。保持来源/包身份采集、fresh-directory、范围约束、拒绝覆盖、字节验证及失败保留，不因减少复制取消这些职责。
新长期证据直接进入获授权的唯一归档位置；运行工具可先在 out 形成工作结果，再经验证搬移被选中的必要证据，不能永久复制每轮全部输出。生产源码快照是点时证据，不意味着删除当前生产输入。收集器不能顺带删除 source/output 或改写历史清单；旧源清理由本批独立准确清单控制，不能在现役工具添加泛化自动清场。

[T3 design](../m3-t3-renderer/design.md) 的原 out 包定位及 [Fix1 design](../m3-t2-publication-fix1/design.md) 的必要证据留 out 文字需区分当前工作现场与最终归档。它们不覆盖新存储规则，存储冲突在各自 update-change 中受控同步；本轮不直接修订其他 change，不改变验收标准或恢复暂停任务。正式 apply 独立确认 test/scripts/tools 的准确代码根授权，不改原生 allowedEditRoots；纯存储适配不产生游戏行为 delta，skip_specs 保持 true。

#### 清理与验证门槛

在聊天汇报准确源/目标/删除清单及理由，按获确认的范围执行；目标字节完整、必要消费者和导航通过、每个必要冻结身份仍可查验/复现后才删除重复或过期材料。未判明、仍有必要输入依赖、未提交唯一材料尚无可靠保存或必要包恢复有缺口的项保留并报告，不能为任务全勾强删。既有冻结内层字节保护与外层迁移可变路径分别核对。

重跑文档/工作流/收集器及安装/捕获相关回归和原生严格校验，增加 archive 路径范围、越界/覆盖/并行变化、缺文件/坏哈希/未登记旧路径、失败夹具语义与完整包保留的负向测试。文档检查器通常跳过 evidence/archive 正文，链接全绿不足以证明归档完整；另核对外层清单文件集、目标逐字节身份及内嵌稀疏/负向状态前后一致性。保护当前候选、Fix1、其他任务输入、官方技能、用户私人目录及项目HEAD/index，按本批实际输入统计，不复用旧61项/10项结果为本批背书。

archive 不进入 .gitignore；检查拟保存文件的实际忽略状态、大文件/二进制保存可行性和本地恢复完整性。不能仅有索引而缺原始数据/必要包；存储方案不成立时保留源并报告缺口。用户自行配置 Obsidian Sync 排除，Agent 不读取/修改私人配置或宣称已验证。Git提交/上传及从远端恢复另需本轮明确授权，在未执行时单列未验证，不把本地归档或Git可见当作已在GitHub持久保存。
