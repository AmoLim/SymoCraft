---
tags:
  - area/verification
---

# 关键节点批准与验证导航

## 退役材料恢复

2026-10-12 用户确认删除旧 spec、testing、milestones 三目录，本批删除剩余36份文件；此前已删除的旧规格不恢复。36份文件均有Git来源，恢复身份为提交 `8a95613435f9bdf433e98ebf136a5e01a13079e5` 下的原仓库路径，可用 `git show <提交>:<原路径>` 读取；未确认远端保存。删除前现存脏差异仅M0换行及T3人工页表格空白，未发现新增事实；原命令、结果、反馈、人工勾选保留在原提交，不作为新运行或批准。T4 D/Q选择和消费者机制在对应change的decisions/design维护，CPU基线在verification维护；M2/ST共享场景在scenarios维护。冻结包不改写，不新增正文镜像或附件。

本页仅维护关键节点批准和导航，不保存验证正文、逐步结果或原始数据。归档规则见 [DOCUMENTATION_MAP](../DOCUMENTATION_MAP.md#每个-change-一份验证记录)，共享方法见 [methods](methods.md)，场景见 [scenarios](scenarios.md)。旧冻结证据及已归档 change 暂不动。

## 关键节点

| 节点类型 | 节点与既有明确决定 | 权威批准依据 | 不可推导的结论 |
| --- | --- | --- | --- |
| 阶段整体验收通过 | M3-T4，2026-10-19用户明确“批准T4整体完成，保留上述风险”并要求归档change | 本聊天用户明确决定；[完成批准与保留风险](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/verification.md#2026-10-19-t4整体完成批准) | 全套自动测试全绿、1175/Fix1关闭、跨工具链全部故障矩阵重跑、长时/中低端/真正干净机通过、T5/正式physics搬迁/GL退役或Git提交合并授权 |
| 阶段整体验收通过 | T3-R4 / Vulkan及双现代接管、整体T3，2026-10-17用户明确“行，目前看来已达性能要求，批准r4/t3的完成” | 本聊天用户明确决定；[完成对账及保留风险](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#2026-10-17-r4及t3完成确认) | 历史失败/尖峰根因已修复、最后局部改动完整边界/回归已补测、Win32 1175/Fix1关闭、长时/中低端/真正干净机通过、GL退役、T4验收或Git/规格sync/archive授权 |
| 阶段整体验收通过 | M3-T2，2026-10-09 用户正式批准 | [批准身份](archive/t2-approval-20261009.7z) | Win32 1175 已修复、Fix1 恢复、正式同源 Q06 或首次可操作预算通过 |
| 设计冻结 | T3-R1 / Renderer v1 与 target 基线，2026-10-09 用户明确冻结 | [冻结清单](archive/t3-r1.7z) | 私有 RHI 全部冻结、生产接管或新 GPU 验证 |
| 正式实施入口 | T3-R2，2026-10-09 同次用户决定批准进入 | [同一用户决定及边界](archive/t3-r1.7z) | R3 进入、D3D12 接管验收、默认切换、Vulkan 或整个 T3 通过 |
| 阶段整体验收通过 | T3-R3 / D3D12接管，2026-10-12 用户明确“批准R3，切换默认后端” | 本聊天明确用户决定；[既有对账与默认切换验证](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#默认切换验证) | 未测A10/故障整合/增量/部署/视觉容差已验证、正式预算达标、1175修复、Vulkan或整体T3通过、GL最终退役 |
| 正式实施入口 | T4完整生产迁移，2026-10-15用户明确授权按已确认方案继续，组合一致性由实施方核对，出现新设计冲突/范围扩大再提请决定 | 本聊天用户明确授权；[实施及验证](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/verification.md#2026-10-15-完整生产迁移) | T4已实现或验收、T3 Vulkan/R4已通过、T5全面重构、Git提交/合并或OpenSpec规格sync/archive |

不新增日常子步骤批准。其他阶段批准尚未集中核对，不能因本页没有条目推定通过或撤销既有真实决定。

## Change 验证导航

| Change | 验证正文 |
| --- | --- |
| T3 渲染接管（2026-10-19已归档） | [verification.md](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md) |
| T4 应用/平台/模拟（2026-10-19已归档） | [verification.md](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/verification.md) |
| T2 发布 Fix1（暂停） | [verification.md](../openspec/changes/m3-t2-publication-fix1/verification.md) |

## 归档入口

| 归档 | 内容及适用边界 |
| --- | --- |
| [OpenGL 三档基准冻结](archive/benchmark-20261005.7z) | 2026-10-05 开发机/Y9000P/GTX1650，固定 allow-unfocused 协议；数据有效不等于性能达标或现代后端通过 |
| [Benchmark 证据目录](archive/benchmark-20261005.7z) | 既有摘要、身份、焦点及成功/失败记录；详细逐帧数据和运行包仍按原身份留在 out |
| [T3-R1 查验包](archive/t3-r1.7z) | 原隔离实验和交接材料，不是已实现生产门面 |
| [T3-R1 设计冻结](archive/t3-r1.7z) | 批准前12份原字节来源、用户冻结决定与公开边界 |
| [T3-R2 顺序1](archive/t3-r2.7z) | CPU 门面、句柄及 target 验证，保留原失败与复测，不是生产玩法验收 |
| [T3-R2 顺序2](archive/t3-r2.7z) | 私有 D3D12 的有限真实 GPU/原生边界验证，不证明实际设备丢失与在途故障完整安全 |
| [T3-R2 顺序3](archive/t3-r2.7z) | 同一游戏循环与持久缓存的有限短测，不替代人工玩法 |
| [T3-R2 顺序4候选](archive/t3-r2.7z) | Debug/Release 002 候选及有限世界/窗口/故障记录；不是 R3 整组验收 |
| [T4 旧 CPU 行为基线](archive/milestone-evidence-20261011.7z) | 原字节归档的基线/夹具与点时身份；只覆盖已存在旧物理/玩法行为，不证明独立 physics target、双会话或完整 T4 迁移 |
| [文档引用修订及历史摘录身份](archive/docs-navigation-20261011.7z)、[最小历史原文](archive/docs-navigation-20261011.7z) | 2026-10-11 用户限定授权：仅修订桌面交付报告两处技术 wiki；四份旧源的六段历史原文用于解释 M0 风险/二进制、M2-T3 旧口径及 GUI 已测/未测范围，不是新运行或批准 |
| [退役项目材料身份](archive/project-history-20261011.7z) | 9篇退役原文的源/目标/大小/SHA256及旧相对路径基址；13份休眠源的精确Git提交/路径/换行恢复信息；完整源码迁移表仅留退役身份，不保留112行副本。运行包副本和迁移临时材料处置按同一记录登记，不是游戏验收或OpenSpec change归档 |
| [阶段必要证据](archive/milestone-evidence-20261011.7z)、[单份归档处置映射](archive/evidence-consolidation-20261011.json) | M0/M1/M2/T0/T1/T2/T4 共634份原字节材料；T3及T2批准6291份复用原归档。外层映射逐文件登记旧位置、唯一目标、大小/SHA256及点时来源基址，不改旧清单或补造历史结果 |

以上旧材料链接指向现存压缩包；成员身份、来源路径及稀疏/负向范围按原清单解释，不将查验快照当完整可运行包。本次仅调整导航，不解包、重打包、重新验证或改写冻结内容。[首次迁移身份](archive/migration-manifest.json)和 [后续处置映射](archive/evidence-consolidation-20261011.json)保持原字节。

## S5 历史日志导航

[S5 原索引所在旧包](archive/milestone-evidence-20261011.7z)与 [Fix1 历史核对边界](../openspec/changes/m3-t2-publication-fix1/verification.md)维护原身份及限制；下表仅导航既存日志，不代表本次重测。

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

见 [T3 未覆盖项](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#未通过与未覆盖)、[T4 验证范围](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/verification.md)及 [Fix1 暂停范围](../openspec/changes/m3-t2-publication-fix1/verification.md)。

## R2 补充验证

见 [T3 验证正文](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#r2-补充验证)。

## R2 短采样收口与保留风险

见 [T3 验证正文](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#r2-短采样收口与保留风险)。

## R3 证据对账

见 [T3 验证正文](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#r3-证据对账)。

## 原位置保留的历史材料

以下导航退役正文的恢复入口与现行归属；旧正文目录已删除，冻结证据未写入，旧状态和批准表不作为当前授权。

| 来源 | 保留原因与不能推定的结论 |
| --- | --- |
| [M0 盘点](VERIFICATION.md#退役材料恢复)、[原 R1-R8 与二进制检查摘录](archive/docs-navigation-20261011.7z) | 点时二进制、完整风险编号与证据限定；不将静态风险自动改为修复/运行通过。10份 docs/legacy 来源已复核、迁移必要知识并经准确清单授权删除，不再作为现行架构入口 |
| [M1](VERIFICATION.md#退役材料恢复) | 失败、授权复测及路径/资源环境，不用当前机制覆盖当时结果 |
| [M2-A](VERIFICATION.md#退役材料恢复)、[M2-T2](VERIFICATION.md#退役材料恢复)、[M2-T3](VERIFICATION.md#退役材料恢复)、[原人工反馈](VERIFICATION.md#退役材料恢复)、[共享场景](scenarios.md) | 玩家反馈、双机与长时结果、包内玩家说明；共享 M2/ST 场景已归 scenarios，但不复制/补填历史勾选 |
| [T0](VERIFICATION.md#退役材料恢复)、[T1](VERIFICATION.md#退役材料恢复) | 构建/失败/玩法/成本与交付记录，不由技术交付推导未核对的用户整体验收 |
| [T2](../openspec/changes/m3-t2-publication-fix1/verification.md)、[S3 人工反馈恢复](VERIFICATION.md#退役材料恢复)、[S4 交接](../openspec/specs/platform/spec.md)、[S5 状态来源](../openspec/changes/m3-t2-publication-fix1/verification.md) | 原人工、1175/预算和交接来源；当前节点已批准，遗留风险与点时失败仍有效 |
| [T3 阶段来源](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md)、[R2 原交接记录](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md) | 实验/候选、顺序1-4与人工入口；当前活动任务在 T3 change。2026-10-11 核对两组5份正文及28对不同链接的实际目标，目标文件 SHA256 相同；根组正文于2026-10-12获准清理，原提交及冻结 raw仍保存历史，r2/的5份副本此前已按清单删除，不证明原 out 整包完整或所有冻结内嵌来源可删除 |
| [T4-P01 来源](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/verification.md)、[消费者核对](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/design.md)、[旧新 CPU 基线方法](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/verification.md) | 静态交接、未选 Q01-Q04、24 case 夹具与局部授权；不推定 SimulationSession/physics target 或正式整体入口完成 |
| [休眠源码身份](archive/project-history-20261011.7z) | 13个 .disabled 来源的点时 SHA256 及非活动边界；2026-10-11 追加授权后已删除准确13份，恢复身份见退役项目材料入口。专项复核不等于 T5 全面审计，不将此证据当当前 app 机制 |
| [旧依赖审计附录原文](archive/docs-navigation-20261011.7z) | M0-M2 的二进制/通知缺口、原复核命令与工具身份；当前安装与来源只在 third-party.md，不追溯改写旧冻结包 |

## 后续集中与收口

[已归档文档迁移 change](../openspec/changes/archive/2026-10-11-docs-openspec-migration/proposal.md)及 [既有处置/恢复/历史检查记录](archive/evidence-consolidation-20261011.json)保留迁移身份与结论边界，具体记录见 `localCloseoutReview`、`auditSceneRetirement`、`changeArchiveRelocation` 和 `postOperationProtection`；[退役材料记录所在旧包](archive/project-history-20261011.7z)保留此前清理身份及历史检查限制。此处只导航，不补建历史 verification.md 或改写旧冻结材料，不推定游戏验收、out 全部归档、远端保存或同步已验证。
