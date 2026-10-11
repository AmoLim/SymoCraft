---
tags:
  - area/verification
---

# 验证与冻结归档索引

本页是唯一权威归档入口及关键节点批准记录。现行治理见 [DOCUMENTATION_MAP](../DOCUMENTATION_MAP.md#验证与批准)，方法见 [methods](methods.md)，场景见 [scenarios](scenarios.md)。2026-10-10 文档试迁移只改变知识位置，没有新增程序、GPU、性能或人工验收成绩。

## 关键节点

| 节点类型 | 节点与既有明确决定 | 权威批准依据 | 不可推导的结论 |
| --- | --- | --- | --- |
| 阶段整体验收通过 | M3-T2，2026-10-09 用户正式批准 | [批准身份](archive/t2-approval-20261009/approval.json) | Win32 1175 已修复、Fix1 恢复、正式同源 Q06 或首次可操作预算通过 |
| 设计冻结 | T3-R1 / Renderer v1 与 target 基线，2026-10-09 用户明确冻结 | [冻结清单](archive/t3-r1/freeze-20261009-001/freeze-manifest.json) | 私有 RHI 全部冻结、生产接管或新 GPU 验证 |
| 正式实施入口 | T3-R2，2026-10-09 同次用户决定批准进入 | [同一用户决定及边界](archive/t3-r1/freeze-20261009-001/freeze-manifest.json) | R3 进入、D3D12 接管验收、默认切换、Vulkan 或整个 T3 通过 |

本轮不新增日常子步骤批准。其他阶段批准尚未集中核对，不能因本页没有条目推定通过或撤销既有真实决定。

## 归档入口

| 归档 | 内容及适用边界 |
| --- | --- |
| [OpenGL 三档基准冻结](archive/benchmark-20261005/reports/Benchmark-基准冻结-20261005.md) | 2026-10-05 开发机/Y9000P/GTX1650，固定 allow-unfocused 协议；数据有效不等于性能达标或现代后端通过 |
| [Benchmark 证据目录](archive/benchmark-20261005/evidence/) | 既有摘要、身份、焦点及成功/失败记录；详细逐帧数据和运行包仍按原身份留在 out |
| [T3-R1 查验包](archive/t3-r1/review-20261009-001/archive-manifest.json) | 原隔离实验和交接材料，不是已实现生产门面 |
| [T3-R1 设计冻结](archive/t3-r1/freeze-20261009-001/freeze-manifest.json) | 批准前12份原字节来源、用户冻结决定与公开边界 |
| [T3-R2 顺序1](archive/t3-r2/step1-20261009-001/validation-manifest.json) | CPU 门面、句柄及 target 验证，保留原失败与复测，不是生产玩法验收 |
| [T3-R2 顺序2](archive/t3-r2/step2-20261009-001/validation-manifest.json) | 私有 D3D12 的有限真实 GPU/原生边界验证，不证明实际设备丢失与在途故障完整安全 |
| [T3-R2 顺序3](archive/t3-r2/step3-20261009-001/validation-manifest.json) | 同一游戏循环与持久缓存的有限短测，不替代人工玩法 |
| [T3-R2 顺序4候选](archive/t3-r2/step4-20261009-001/README.md) | Debug/Release 002 候选及有限世界/窗口/故障记录；不是 R3 整组验收 |
| [T4 旧 CPU 行为基线](archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/README.md) | 原字节归档的基线/夹具与点时身份；只覆盖已存在旧物理/玩法行为，不证明独立 physics target、双会话或完整 T4 迁移 |
| [文档引用修订及历史摘录身份](archive/docs-navigation-20261011/revisions.json)、[最小历史原文](archive/docs-navigation-20261011/historical-extracts.json) | 2026-10-11 用户限定授权：仅修订桌面交付报告两处技术 wiki；四份旧源的六段历史原文用于解释 M0 风险/二进制、M2-T3 旧口径及 GUI 已测/未测范围，不是新运行或批准 |
| [退役项目材料身份](archive/project-history-20261011/retirement-manifest.json) | 9篇退役原文的源/目标/大小/SHA256及旧相对路径基址；13份休眠源的精确Git提交/路径/换行恢复信息；完整源码迁移表仅留退役身份，不保留112行副本。运行包副本和迁移临时材料处置按同一记录登记，不是游戏验收或OpenSpec change归档 |
| [阶段必要证据](archive/milestone-evidence-20261011/)、[单份归档处置映射](archive/evidence-consolidation-20261011.json) | M0/M1/M2/T0/T1/T2/T4 共634份原字节材料；T3及T2批准6291份复用原归档。外层映射逐文件登记旧位置、唯一目标、大小/SHA256及点时来源基址，不改旧清单或补造历史结果 |

archive/ 中这些材料按原字节归档，仅由本索引统一导航。嵌入原材料的“当前”、旧路径、待审核文字及失败状态均保持其记录时含义，不作为现行导航或批准状态。
[原迁移清单](archive/migration-manifest.json)保留首次迁移的点时身份；本批处置只记录在上表单份归档映射中，不回填旧清单。2026-10-11 按用户确认的准确6925份清单完成旧 evidence 源文件清理；清理前核对目标字节、完整文件集、消费者及本地恢复，实际范围与局限见本页末节。

R1 freeze 的 `sources/.../review-20261009-001/archive-manifest.json` 只是该清单自身的批准前输入快照；其 payload 按同一 review 身份定位到上表 R1 查验包，不要求在 freeze/sources 再复制一套。原清单的历史 `source` 字段不拿来校验当前已演进源码。
首次迁移基线曾漏枚举 evidence 内的 raw/out 等构建名称目录；已补齐原文件与副本的逐字节一致性，并用既有内部清单验证点时 payload。迁移清单区分 `original-baseline` 与 `source-at-copy-time`，不把后补枚举冒称本轮开始前的基线保护。

归档是查验快照，不是完整可运行包。外层 freeze/archive/validation 清单定义其实际选录文件，必须完整核对；内嵌 `package-manifest.json` 记录当时运行包身份，不承诺该快照含全部 EXE/DLL/资源。原来未选录的包文件和隔离 `corrupt-*` / `missing-dxil` 失败样本保持原状态，不能为了清单全绿补造或修复。迁移验证分别检查外层完整性与内嵌记录的新旧状态一致性，不将历史缺失冒称新迁移成功的运行验证；实际运行包仍按原身份定位到原 `out`。

本次冻结引用修订是明确授权的单文件例外，不改变一般冻结不可变规则。[桌面报告原字节](archive/docs-navigation-20261011/benchmark-desktop.original.txt)保留原 SHA256 `091d310a06fa29a343fff4c88af82044b0a664941e1c8c34a41afb84fafe6d7b`；两处技术链接现指向 benchmark 模块，修订身份及逐项替换记在上表 revisions.json，历史结果、命令、批准及其他字节不变。原报告中“使用与交付”的旧相对路径未改；当前操作从 [工具 README](../../tools/benchmark/README.md) 进入。历史摘录保存来源路径、原件 SHA256、原行范围和摘录 SHA256，原文中的相对路径只按记录的旧来源位置解释，不是当前可执行导航。

9篇退役项目材料按原字节迁入上表归档，不修改内嵌历史链接。只读docs/spec的已登记旧入链由文档检查器按来源身份及归档大小/SHA256解析，并继续核对锚点；当前文档和未登记缺链不得使用此例外。归档文字按原来源父目录解释，不将旧学习说明重新认定为当前架构。
同一严格边界现覆盖本批登记的旧 evidence 文件；只读规格正文及冻结内嵌路径不改写，未知旧路径仍拒绝。34份内嵌包清单的466项原稀疏缺失及15项已存在但不同的负向数据保持原状，不补文件使其变成完整包。必要完整包及当前候选独立核对原 out，既有归档不替代它们。

## S5 历史日志导航

[S5 原索引](archive/milestone-evidence-20261011/m3-t2/evidence/s5-validation.json)记录的是点时身份和旧执行路径，不重写其中的路径、结果或批准状态。下表是当前导航的唯一维护位置；原目录均为 `out/m3-t2/s5/`，完整大小和 SHA256 仍以原索引对应字段为准。
2026-10-11 离线逐项核对以下18份日志，当前文件大小及 SHA256 均与原索引相符；没有重新构建或执行测试。Debug 失败、禁用用例和 Release 点时通过仍保留原义，不关闭发布风险。其余内嵌源码、运行包、探针结果及其他历史路径未由这18项核对证明完整；关联问题见 [M3-I3](../issue/issues.md#m3-i3)。

| 原索引字段 | 旧目录内文件名及当前入口 |
| --- | --- |
| main_and_matrix_tests | [main-debug-001-followup-test.log](../../out/m3-t2/log/s5/build/main-debug-001-followup-test.log) |
| main_and_matrix_tests | [main-debug-001-test.log](../../out/m3-t2/log/s5/build/main-debug-001-test.log) |
| main_and_matrix_tests | [main-release-001-test.log](../../out/m3-t2/log/s5/build/main-release-001-test.log) |
| main_and_matrix_tests | [matrix-debug-001-cpu-only-test-final-002.log](../../out/m3-t2/log/s5/build/matrix-debug-001-cpu-only-test-final-002.log) |
| main_and_matrix_tests | [matrix-debug-001-cpu-only-test.log](../../out/m3-t2/log/s5/build/matrix-debug-001-cpu-only-test.log) |
| main_and_matrix_tests | [matrix-debug-001-game-only-test.log](../../out/m3-t2/log/s5/runtime/matrix-debug-001-game-only-test.log) |
| main_and_matrix_tests | [matrix-debug-001-standalone-runner-test.log](../../out/m3-t2/log/s5/benchmark/matrix-debug-001-standalone-runner-test.log) |
| main_and_matrix_tests | [matrix-release-001-cpu-only-test.log](../../out/m3-t2/log/s5/build/matrix-release-001-cpu-only-test.log) |
| main_and_matrix_tests | [matrix-release-001-game-only-test.log](../../out/m3-t2/log/s5/runtime/matrix-release-001-game-only-test.log) |
| main_and_matrix_tests | [matrix-release-001-standalone-runner-test.log](../../out/m3-t2/log/s5/benchmark/matrix-release-001-standalone-runner-test.log) |
| main_and_matrix_tests | [probe-debug-001-test.log](../../out/m3-t2/log/s5/platform/probe-debug-001-test.log) |
| main_and_matrix_tests | [probe-release-001-test.log](../../out/m3-t2/log/s5/platform/probe-release-001-test.log) |
| build_and_install_matrix | [matrix-debug-001.log](../../out/m3-t2/log/s5/build/matrix-debug-001.log) |
| build_and_install_matrix | [matrix-release-001.log](../../out/m3-t2/log/s5/build/matrix-release-001.log) |
| executable_imports.dependents | [SymoCraft-debug-dependents.log](../../out/m3-t2/log/s5/build/SymoCraft-debug-dependents.log) |
| executable_imports.dependents | [SymoCraftBenchmark-debug-dependents.log](../../out/m3-t2/log/s5/benchmark/SymoCraftBenchmark-debug-dependents.log) |
| executable_imports.dependents | [SymoCraft-release-dependents.log](../../out/m3-t2/log/s5/build/SymoCraft-release-dependents.log) |
| executable_imports.dependents | [SymoCraftBenchmark-release-dependents.log](../../out/m3-t2/log/s5/benchmark/SymoCraftBenchmark-release-dependents.log) |

## 未通过与未覆盖

默认仍为旧 OpenGL；D3D12 是显式选择的生产候选，Vulkan 未实现。R3 A01–A19 整组、人工玩法、实际设备/提交/真正在途故障、默认切换及双现代后端整体验收没有被本轮批准。
原生 Benchmark 仍按旧 GL 文件协议读取，现代 telemetry 已有扩展输出不等于 GUI/CLI 已接通。Win32 1175、暂停 Fix1、正式同源 Q06 和首次可操作时间预算继续保留原状态。
本机 RTX 5070 Ti 功能范围不代替下一玩法前的 Y9000P/GTX1650 整机性能门槛，旧 GL 冻结数字不是新后端通过依据。

## 原位置保留的历史材料

以下从同一索引进入，未复制成新的阶段总结。保留理由是独有点时身份、人工反馈、失败/复测或未完整替代的证据，不是文件数量预算例外；后续可按 A 类最小充分归档处理。现役机制/目标分别从 architecture/openspec 进入，旧状态和批准表不作为当前授权。

| 来源 | 保留原因与不能推定的结论 |
| --- | --- |
| [M0 盘点](../milestones/m0/README.md)、[原 R1-R8 与二进制检查摘录](archive/docs-navigation-20261011/historical-extracts.json) | 点时二进制、完整风险编号与证据限定；不将静态风险自动改为修复/运行通过。10份 docs/legacy 来源已复核、迁移必要知识并经准确清单授权删除，不再作为现行架构入口 |
| [M1](../milestones/m1/README.md) | 失败、授权复测及路径/资源环境，不用当前机制覆盖当时结果 |
| [M2-A](../milestones/m2-a/README.md)、[M2-T2](../milestones/m2-t2/README.md)、[M2-T3](../milestones/m2-t3/README.md)、[原人工场景与反馈](../testing/gameplay-smoke.md) | 玩家反馈、双机与长时结果、包内玩家说明；共享 M2/ST 场景已归 scenarios，但不复制/补填历史勾选 |
| [T0](../milestones/m3-t0/README.md)、[T1](../milestones/m3-t1/README.md) | 构建/失败/玩法/成本与交付记录，不由技术交付推导未核对的用户整体验收 |
| [T2](../milestones/m3-t2/README.md)、[S3 人工反馈](../milestones/m3-t2/s3-manual-verification.md)、[S4 交接](../milestones/m3-t2/s4-platform-handoff.md)、[S5 状态来源](../milestones/m3-t2/s5-delivery-status.md) | 原人工、1175/预算和交接来源；当前节点已批准，遗留风险与点时失败仍有效 |
| [T3 阶段来源](../milestones/m3-t3/README.md)、[R2 原交接记录](../milestones/m3-t3/r2-entry.md) | 实验/候选、顺序1-4与人工入口；当前活动任务在 T3 change。2026-10-11 核对两组5份正文及28对不同链接的实际目标，目标文件 SHA256 相同；保留根组及冻结 raw，r2/ 的5份可维护副本已按清单获准删除，不证明原 out 整包完整或所有冻结内嵌来源可删除 |
| [T4-P01 来源](../milestones/m3-t4/README.md)、[消费者核对](../milestones/m3-t4/consumer-audit.md)、[旧新 CPU 基线方法](../milestones/m3-t4/cpu-behavior-baseline.md) | 静态交接、未选 Q01-Q04、24 case 夹具与局部授权；不推定 SimulationSession/physics target 或正式整体入口完成 |
| [休眠源码身份](archive/project-history-20261011/architecture/app/App-休眠源码清单.md) | 13个 .disabled 来源的点时 SHA256 及非活动边界；2026-10-11 追加授权后已删除准确13份，恢复身份见退役项目材料入口。专项复核不等于 T5 全面审计，不将此证据当当前 app 机制 |
| [旧依赖审计附录原文](archive/docs-navigation-20261011/historical-extracts.json) | M0-M2 的二进制/通知缺口、原复核命令与工具身份；当前安装与来源只在 third-party.md，不追溯改写旧冻结包 |

## 后续集中与收口

本轮浓缩模块机制、工具操作及验证方法，未执行游戏、GPU/性能采样或新增人工批准。上一批65份、原暂缓10份和 docs/legacy 的10份共85份已完成复核，并在完整清单汇报及用户明确确认后按准确清单删除；必要独有历史已最小归档，保留材料的旧入链已改指现行正文或明确历史入口，并加一句失效说明。不保留学习版或导航空壳；其余独有报告、人工反馈及证据未自动纳入本批。
Benchmark 安装和输入捕获已改读 tools/benchmark/README.md 与 docs/third-party.md，输出说明文件名不变；隔离消费者回归仅证明两项文档输入适配，不是原生工具构建、GUI/GPU、许可证完备或部署验收。
本批追加授权已执行：9篇退役原文逐字节归档，完整源码迁移表及准确13份非活动 .disabled 清理；7组完整重复包共277份文件清理（54,904,598字节），必要完整包及实际当前Debug004/Release002评审现场保留；665份旧迁移临时材料清理，85份处置映射和历史审计限制转存同一退役身份记录，不永久保留整套流水账或保证全部旧笔记全文恢复。
上一批完成时其余10篇旧 spec、旧生成索引及273份旧 evidence 副本继续保留，重新执行的61项回归及10项严格校验通过；这些不是本批结果。范围外 docs/.obsidian/workspace.json 变化单列，未回退或写入。未构建/运行游戏、执行GPU/性能采样、生成新包或项目Git写入；保留候选不自动对应删除后的源码树，也不是游戏节点批准或真实OpenSpec change归档。

2026-10-11 单份归档批：634份必要材料原字节归档、6291份复用已有归档，可维护正文29篇的361处入链改指唯一目标。按已批准准确清单移除6925份旧源，共184184950字节，不递归清空旧目录。273份 Benchmark 旧源在本批起点已不存在，对应归档全部逐字节吻合，不计为本批删除。7198项登记身份的本地恢复文件集与字节均吻合；恢复夹具仅为本轮临时核验，不保留为第二套证据。163024份 out 文件及13项完整包/当前候选检查保留原位置；原始采样、关键失败夹具、暂停 Fix1 和未判明材料没有列删除项，不据此宣布 out 全部完成最终归档或全库无残留。
T3设计与暂停Fix1设计的存储措辞仍需各自update-change澄清工作现场与最终归档，属于未关闭计划依赖，不由本批越权修改或恢复暂停任务。archive 文件忽略检查通过，但没有执行 Git提交/上传、GitHub远端恢复或 Obsidian 同步验证，不声明已远端持久保存。
旧源清理后5份T3冻结Markdown曾在原路径重新出现，仅链接被改写；用户随后确认由Obsidian同步造成，暂停同步并明确批准清理重新核对的5份副本。2026-10-11按新身份逐项删除这5份，共102656字节，对应归档原件保持不变；这是恢复副本的追加清理，不增加原6925份的唯一来源计数。原观察保留，后续处置与当前核对结果追加至上表单份归档映射，不覆盖旧冻结身份。
本轮重新核对7198项登记目标、7219份完整归档文件集、6584份既有冻结字节、34份内嵌清单及13项完整包/候选检查；163024份out的元数据/文件集保持。7198项新本地恢复的文件集及字节均吻合，核验夹具清理后只保留恢复结果。T4内清单的235项中234项符合声明，README的1项声明与迁移起点已不一致；归档仍与起点原件完全一致，保留该历史差异，不改清单或补造新身份。
本轮明确UTF-8模式下重新执行114项离线回归及10项严格校验通过，文档检查无错误或警告。此前系统默认GBK模式有1项夹具读取中文JSON失败，未修改测试或隐去该限制。
用户授权只读核查索引并记录后继续收口；5138项索引的路径、模式和对象身份与未变化的HEAD完整一致，全部stage=0，无暂存差异、冲突或特殊标志，核查期间索引/HEAD字节保持。起点与当前index哈希不同仍原样登记，不回退或重写旧哈希，不把具体来源或“仅缓存刷新”冒称已证明；该准确新身份已完成语义核对，任何进一步变化仍须重新审阅。170011项受保护输入核对未发现其他未登记差异，完整归档集及out集合/身份检查完成，最终核对从上表单份归档映射的localCloseoutReview进入，原失败观察保留。
2026-10-11 用户仅追加批准当前迁移核验现场清理、历史不一致最小事实保留，以及本迁移 change 归档。准确24份临时文件（61859169字节）登记身份后已清理，必要处置、恢复结果及验证结论仍见上表单份归档映射的 `auditSceneRetirement` 和原 `localCloseoutReview`；不长期保留整套临时脚本、基线和恢复清单。冻结原件、内嵌稀疏/负向状态、T4 README 的原声明差异及已复核的 index 字节差异仍如实保留，不删不一致材料或修改旧哈希使历史变绿。
已归档 [docs-openspec-migration](../openspec/changes/archive/2026-10-11-docs-openspec-migration/proposal.md)（symocraft-full，21/21任务完成，无规格增量、未执行spec sync）；`.openspec.yaml`、proposal、design、tasks 四份保持原字节。准确迁移路径/大小/SHA256及原来源基址见同一映射的 `changeArchiveRelocation`；归档正文中的历史相对链接按原来源位置解释，不重写点时授权及命令。旧规格中的准确已登记历史入链仍逐项核对源身份、归档目标及锚点，未知旧路径或当前文档缺链仍报错。
10篇只读旧spec及旧索引继续保留，其他活动change不归档；T3/Fix1的存储措辞依赖仍由各自change处理，不恢复暂停任务。本次只完成迁移change归档，不代表游戏关键节点验收。Obsidian配置/同步、Git提交/上传、GitHub远端恢复、功能change的sync/archive及动态游戏验证未执行；不将完成解释为全库无残留、out全部最终归档或远端持久保存。
归档后明确UTF-8模式下重新执行118项离线回归（脚本62、文档/消费者33、工作流23）及9项严格校验通过；93篇文档的链接/锚点核对无错误或警告，30处已登记历史入链通过。工作流首次有1项因归档后相对基址误解失败，窄适配准确归档身份及原来源基址后23项重跑通过，原失败原因仍保留，不改归档正文。170700项保护输入核对通过，7198项证据身份、10篇旧规格、163024份out集合/元数据、冻结字节、现役spec及官方技能保持，HEAD/index保持已复核新身份；其他活动change仅T3的一处迁移导航改链。实际结果从同一映射的 `changeArchiveRelocation.finalChecks` 和 `postOperationProtection` 进入，不以此前114项/10项成绩代替；本轮自建一次性助手和保护快照亦在核对后清理，不新增长期现场。
