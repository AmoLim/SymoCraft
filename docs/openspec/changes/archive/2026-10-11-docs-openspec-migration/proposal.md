---
tags:
  - area/spec
---

## Why

用 OpenSpec 接替原 docs/spec 的目标与计划链，保留工程知识而减少重复和旧状态误导。治理依据为 [DOCUMENTATION_MAP](../../../DOCUMENTATION_MAP.md) 与 [AGENTS](../../../../AGENTS.md)；本轮已独立盘点现役 specs/scenarios，并读取相关旧规格、范围和活动变更。

## What Changes

2026-10-10 用户批准覆盖全部现有 docs/spec、建立经源码及适用依据核对的最小现役基线、精炼文档并验证迁移。原源文件只加退役标记，保留复核现场；不删除、不提交、不执行真实功能 change 的 sync/archive。
目录唯一为 docs/openspec，根技能仍在 .agents/skills。T3/T4/Fix1 的功能授权、暂停和未完成状态保持；修正已核实的入口状态，不新增批准。
2026-10-10 新会话只读复核指出旧文档消费者和当前状态滞后，用户随后批准修订本 change 并继续：浓缩剩余模块知识、纠正状态、适配消费者并验证，先汇报逐项处置候选，再另行确认删除。不是重选架构或改变游戏契约。
准备阶段范围为 docs 的可维护知识与导航、工具 README、Benchmark 安装文档路径、迁移输入捕获的对应文档路径及必要验证工具；该阶段未删除旧源、证据或临时现场。整个迁移不实施游戏功能、构建/运行游戏、GPU/性能重测或 Git 写操作，不读取个人笔记或调整 Obsidian。ProjectShadow 只读参考。

2026-10-11 用户确认窄权限：修订冻结桌面交付报告中指向旧 Benchmark 执行/文件协议的两处引用，保存改前原文与哈希并登记新身份；其余冻结字节、历史命令、结果和批准不改。保留文档中的旧导航用一句话标明失效并改指权威位置，不建立空壳。将 docs/legacy 的10份文档与此前暂缓材料逐项复核、迁移必要知识后扩充候选；不包括生产 legacy/.disabled，不实际删除。六份现役 specs/scenarios 已再次全文预读，相关 T3/T4/Fix1 的设计及决策边界不变，无游戏行为增量。

2026-10-11 用户在完整清单汇报后明确确认下一步清理：仅删除原65份、原暂缓10份及 docs/legacy 的10份，共85份文档；完整路径、身份及归属见[退役处置映射](../../../verification/archive/project-history-20261011/retirement-manifest.json)。清理前再次检查85份身份未变、权威目标齐全、保留文档及56份冻结 Markdown 无候选入链，两个文档消费者无旧源输入依赖；当时另存独立起点和准确原文恢复副本，不覆盖此前复核记录。该85份已按清单删除，生成索引及当前状态随之同步，清理后的文档、消费者、工作流及保护检查已通过，DOC-07 完成。其他10份旧 spec、历史来源、冻结证据、原 out 包及临时现场不随目录整体删除，不将本批清理当作全库无残留。

13份 game/app/legacy/*.disabled 已另行完成专项只读复核：未发现活动编译、安装或游戏入口调用，也未发现 T4 目标需要恢复旧实现；全部哈希符合[休眠源码身份](../../../verification/archive/project-history-20261011/architecture/app/App-休眠源码清单.md)，从已提交 Git 版本在内存还原换行后可逐字节恢复。此前85份文档清理不包括源码；下述追加授权才覆盖这13份。准确清单、代码根范围、恢复身份和验证前置见 [design](design.md#授权残留材料归档与清理)，不将专项复核记作 T5 全面审计或整阶段完成，不扩大到活动 ECS/内存、GL 过渡路径或历史快照副本。

2026-10-11 用户在9篇退役文档及1份源码迁移清单的信息汇报后追加授权，并明确运行包保留边界。本批文档、源码、7组完整重复包及665份旧迁移临时材料已处置，本批61项回归和10项严格校验已重新通过，最终保护核对及任务状态只按 tasks 和退役身份记录确认，范围为：
- 将 docs/project 中剩余9篇 Markdown 按原字节归档至 verification/archive/project-history-20261011，唯一入口维护在 VERIFICATION；历史相对路径按原来源位置解释，不恢复学习版或第二权威。
- 清理 docs/project/architecture/source-migration.tsv 和上述13份休眠源码；先保存必要提交/路径/SHA256/换行恢复信息，再核对无活动消费者并执行准确清单。本次明确源码权限仅限代码根 game/app/legacy 下这13份，不改原生 allowedEditRoots，不授权其他游戏实现。
- 运行包只清理已证实重复、过期且有可靠保留依据的副本；保留必要冻结身份的唯一完整包和当前候选，以及解释关键结论的失败样本、原始采样和暂停 Fix1 现场。包是否可删须逐项核对，不按目录年龄或文件名推定。
- 清理文档迁移临时复核现场前，先转存必要身份/核对记录、更新活动引用并验证；不整目录清空 .temporary 或 out，不删真实返回数据、其他任务现场或尚有独有必要知识的材料。

273份 docs/project/benchmark/evidence 来源副本、其余10篇旧 spec 和旧生成索引不自动纳入本批；9篇归档不等于删除所有 docs/project 内容。归档文档不是归档真实 OpenSpec change，继续禁止真实 sync/archive、游戏构建/运行、GPU/性能重测及项目 Git 写操作。实际进度只在唯一 [tasks](tasks.md) 中维护，上一批历史检查不替代本批验证。

2026-10-11 用户进一步确认必要证据单份归档方案，先授权只更新本 change 的 proposal/design/tasks，随后授权apply及聊天汇报的准确清单；此前已单独批准将新规则写入权威治理。单份归档批按 [证据单份归档](../../../DOCUMENTATION_MAP.md#证据单份归档) 复核旧 evidence、既有 archive 及 out 中的相关证据：必要材料集中于 docs/verification/archive，由 VERIFICATION 唯一索引；移除经证明重复或过期且无独有必要信息的证据，不再维护 out→evidence 的平行持久副本。
上述273份来源副本纳入下一批逐项复核，不预判273份全部可删；其余阶段证据同样按结论、身份及复现需要取舍。当前候选、必要唯一完整运行包、工作输入、关键失败样本和暂停 Fix1 现场不得由稀疏快照替代；未判明项保留，不整目录清空。此前各批范围与结果仍是点时记录，不追溯改写。
用户将 verification/archive 排除于 Obsidian Sync，长期由 Git/GitHub 保留；不修改 Obsidian 设置，不忽略 archive，也不宣称未提交材料已上传。Git 写入及远端恢复验证另获授权；存储不足或必要大文件无法保留时先报告，不静默舍弃。具体映射、消费者适配及删除前置见 [design](design.md#必要证据单份归档与清理)，DOC-17至DOC-21实际进度只在tasks维护。拟定时未实施搬移、删除或脚本适配，后续apply不追溯更改历史批次结果。

## Capabilities

无游戏行为变更，metadata 使用 skip_specs: true。本轮直接迁移经核对的现役规格基线，不调用真实目标 change 的合并或归档。源→目标归属在 [design](design.md#来源映射) 维护；任务仅在 [tasks](tasks.md)。

## Impact

影响规则、文档、OpenSpec 配置/自定义 schema、Python 入口及验证工具。现有六份 current specs/scenarios 和相关 T3/T4/Fix1 delta/design/decisions 已独立重读，公开行为不变；[scope](../../../scope/project-scope.md) 的硬件、后续性能与实施边界不缩减。
上一阶段消费者仅调整 tools/benchmark/CMakeLists.txt 和 scripts/capture-migration-inputs.py 的两项文档输入，安装输出名及捕获协议不变。本批追加历史材料位置、准确13份非活动源与已核对包副本/迁移临时现场处置；按实际范围同步可维护导航及保留说明，不改生产算法或包协议。依赖既有 Node/OpenSpec 全局安装，不给 C++ 游戏新增 npm 依赖。
[T3](../m3-t3-renderer/proposal.md)、[T4](../m3-t4-application-simulation/proposal.md) 与 [暂停 Fix1](../m3-t2-publication-fix1/proposal.md) 分别拥有自己的目标与计划，不因迁移获得实施入口。
获准删除13份休眠源码后，[D3D12 打包工具](../../../../scripts/package-d3d12-candidate.py)及生产输入捕获会观察到源文件集合变化：若要给新源码树生成候选，必须另行取得构建/打包授权、实际重新链接并记录新构建身份，不能靠修改旧清单或放宽校验为旧二进制重新背书。既有候选和历史冻结包仍按各自点时身份保留，不宣称与删除后的生产输入一致；本批不创建新运行包。[module-boundaries](../../specs/module-boundaries/spec.md) 的休眠源重新进入活动目标拒绝契约继续有效。
下一批影响证据归档位置、唯一索引、可维护导航、证据收集器及对应验证工具/操作说明；不改变 [foundation-files](../../specs/foundation-files/spec.md) 的发布失败与终态语义、[renderer](../../specs/renderer/spec.md) 的候选/测量边界或其他现役契约。T3/Fix1 design 中仍按原 out 定位的证据措辞列为跨 change 存储冲突，须在各自规划中受控同步，不据新规则恢复暂停调查、改变功能设计或修改冻结命令。正式 apply 前须重新确认本轮允许的工具代码根范围，不把 docs-only 原生输出当作脚本修改权限。
