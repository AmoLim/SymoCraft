---
type: 人工核查
status: P01材料与CPU基线已交付，待用户审阅，无T4生产运行包
project: Symocraft
module: M3-T4-App-Platform-Simulation
created: 2026-10-09
updated: 2026-10-09
tags:
  - area/milestones
---

> 2026-10-11 文档浓缩后已退役复核源：现行入口见 [权威位置](../../verification/VERIFICATION.md#原位置保留的历史材料)；原文保留点时反馈、历史身份和复核依据，不再维护为当前机制或活动计划，旧勾选不产生新批准。

# M3-T4 人工核查入口

本页保留 2026-10-09 P01/CPU 材料的原始人工反馈，原勾选不改写，旧授权描述按对应记录时点理解。2026-10-10 起，未决选择只在 [T4 decisions](../../openspec/changes/m3-t4-application-simulation/decisions.md)，核查要求与唯一活动计划在 [tasks](../../openspec/changes/m3-t4-application-simulation/tasks.md)，关键节点批准只在 [VERIFICATION](../../verification/VERIFICATION.md)。不再维护平行用户主表；没有 T4 生产迁移候选包，不因迁移要求新试玩或补测。

材料版本：2026-10-09首轮P01文档。源码身份为 [工作区清单](consumer-source-manifest.json)，不是该HEAD的干净构建；T3并行变化后须核对受影响材料。目标与不变量见 [正式spec](../../openspec/changes/m3-t4-application-simulation/proposal.md)，本轮授权见 [阶段README](README.md)。

## 当前人工主表

| 状态 | 稳定编号/核查时点 | 用户审阅材料或操作 | 预期结果/通过标准 | 实际结果与证据 |
| --- | --- | --- | --- | --- |
| [ ] | T4-M01 P01范围 | 阅读README与接口候选的归属/并行边界 | 确认本轮仅细化/静态核对，D01–D13不重开；未授予物理/应用/平台/观测生产实施 | 待用户审阅 |
| [ ] | T4-M02 P01接口/消费者 | 审阅 [接口清单](interface-contract-candidate.md) 与 [消费者核对](consumer-audit.md) | 窗口事实、CPU输入/自有结果、镜头转接、错误/提交进度、记录/关闭出口满足原契约；H01–H04仍为后续交接，静态核对不冒称编译/运行通过 | 待用户审阅 |
| [ ] | T4-M03 P01未决项 | 在 [决策表](interface-decisions.md) 选择Q01–Q04，并补充约束/理由 | 最终格明确，候选/对比保留；选择同步到正式spec，未决定不标冻结 | 待用户选择，当前4项 |
| [ ] | T4-M04 CPU局部范围 | 阅读 [基线报告](cpu-behavior-baseline.md) 的范围与未覆盖项 | 本批只建立旧CPU基线/夹具，不宣称新physics核心、会话、生产迁移或D12已完成 | 待用户审阅；局部自动交付已完成 |
| [ ] | T4-M05 CPU来源身份 | 审阅 [归档入口](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/README.md) 的final/after身份与原失败 | 实际dirty源码hash为准；生产CPU未改；app不冻结；沙箱C1902与早期/最终结果分别保留 | 自动核对通过，用户结论待填写 |
| [ ] | T4-M06 CPU用例与结果 | 审阅 [结构化结果](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/summary.json) 与两配置原始日志 | 15物理+9玩法各重复三次通过；Debug58启用项通过/1禁用、Release59通过；关闭测试构建与真实依赖成立 | 自动结果已留证，用户结论待填写；无需人工复跑 |

## 后续节点

Q01–Q04确认后先收口接口，再接收T3实际版本及H01–H04交接。T4完整实施仍需前序必需契约/验收、正式计划整体审核与独立授权；届时原地更新主表，已完成的P01审阅移入历史记录，不复用编号或自动批准下一节点。A01–A27的实现/测试矩阵不在本轮增列人工试玩checkbox，A23按D02继续归后续。

## P01 静态检查

实施方负责核对文件存在、相对链接/标题、决策数量、候选状态与来源清单可解析；不把这些操作转交用户。P01本身未运行自动构建/CPU/边界/故障/真实SDL/GPU/性能测试；另行授权的CPU基线自动结果见T4-M04–M06，不改写P01静态核对为运行成绩。
