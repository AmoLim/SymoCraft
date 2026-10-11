---
type: 阶段记录
status: P01接口候选待审阅；CPU行为基线与夹具已交付，完整T4生产迁移未授权
project: Symocraft
module: M3-T4-App-Platform-Simulation
created: 2026-10-09
updated: 2026-10-09
tags:
  - area/milestones
---

> 2026-10-11 旧导航引用已失效，已改指现行权威或明确归档入口；历史命令、结果和批准含义保持不变。

> 2026-10-11 文档浓缩后已退役复核源：现行入口见 [权威位置](../../verification/VERIFICATION.md#原位置保留的历史材料)；原文保留点时反馈、历史身份和复核依据，不再维护为当前机制或活动计划，旧勾选不产生新批准。

# M3-T4 接口细化与消费者交接

P01 当前节点：**T4-P01 接口细化与消费者核对 → 用户审阅接口候选和未决项**。2026-10-09 用户授权本项与正在推进的 T3 并行，产物写入本阶段目录。P01 当时的授权只覆盖设计、源码静态核对与文档，不覆盖物理核心实施、生产接口切换、测试代码/CMake 改动或 T4 整体实施。另行批准的 CPU 基线批次见下节，不改变 P01 材料及其待选状态。

依据：[T4 正式计划](../../openspec/changes/m3-t4-application-simulation/proposal.md)、[项目范围](../../scope/project-scope.md)、[T3 R2 入口](../m3-t3/r2-entry.md)。D01–D13 均已确认；新增 Q01–Q04 只处理具体接口落地，不重新讨论时间、失败保证、输入复用、CPU 对照或物理迁移路线。

## 阅读入口

1. [接口候选清单](interface-contract-candidate.md)：声明形状、数据字段、所有权和调用顺序。
2. [消费者核对](consumer-audit.md)：现有调用方、迁移要求、T3 交接依赖及未覆盖的验证。
3. [未决项决策表](interface-decisions.md)：4 项新未决项，每项 3 个候选及成本/风险对比。
4. [固定人工核查入口](manual-verification.md)：本轮只审阅材料和选择方案，无运行包或试玩要求。

源码核对身份见 [工作区来源清单](consumer-source-manifest.json)。清单是有未提交变更的工作区文件身份，不是构建包、源码冻结或运行成绩；T3 后续修改相关文件时须复核。

## 本轮结论

| 项目 | 当前结果 | 限制 |
| --- | --- | --- |
| 接口细化 | 已给出 Application、Runtime/Window、输入、SimulationSession、physics、观测/场景的候选声明与最小字段 | 尚未写入生产头文件，Q01–Q04 未代选 |
| 消费者定位 | 生产 app、模拟内部调用、现有 CPU/平台/观测测试、T3 渲染适配、公开头/构建检查和外部 benchmark 均有迁移记录 | 静态核对不是编译、运行或完整行为等价证明 |
| T3 接口接收 | 现代 renderer 消费 CameraParameters，而不是旧 Camera 实体；旧 GL 仍消费中立矩阵值 | 不修改 Renderer v1，也不从活动工作区推定 R2 接管已验收 |
| 新未决项 | Q01 实体装配入口、Q02 键盘事实结构、Q03 关闭结果交接、Q04 诊断可见范围 | 仅在选定并整体审阅后写成冻结接口，不扩大 T4 范围 |
| 验证 | 本轮完成源码定位、文档链接/编号及来源清单检查 | 未构建、未运行 CPU/SDL/GPU/性能测试，T4 A01–A27 不勾选 |

## 并行修改边界

本轮只维护 `docs/milestones/m3-t4/` 及 T4 spec/scope/导航中的本阶段入口。T3 当前生产接管仍拥有 app 帧循环、GameRenderer、相机转接、现代 GPU 样本关联和退出路径的实现修改。本任务不改这些源码、共享构建规则、既有证据或 T3 人工清单，不发送跨聊天实施指令。

后续先确认 Q01–Q04，再由 T3 提供实际接管版本及相关消费者交接。生产切换前核对 T2/T3 必需契约、整体审核正式 T4 spec 并取得独立实施授权；本轮准备授权不取消这些条件。方案2的物理核心独立实施若需提前进行，另行确认其范围，不由本轮授权扩展。

## CPU 基线与夹具交付

2026-10-09 当前聊天独立批准现有 CPU 行为基线/夹具及独立物理核心/单测，并随后明确要求先进行基线与夹具。本批只交付前者：新增根级真实生产入口测试，没有提取 physics 核心、改变算法或迁移 Application/Runtime/SimulationSession/观测接口。P01 的原准备授权不等于这次局部实施授权；本批也不批准完整 T4。

[行为基线报告](cpu-behavior-baseline.md)、[查验归档](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/README.md) 和 [结构化结果](../../verification/archive/milestone-evidence-20261011/m3-t4/evidence/cpu-baseline-20261009-001/summary.json) 是本批入口。固定人工主表新增 T4-M04–M06，不代勾 P01 或 CPU 审阅项。

| 项目 | 最终实测 | 边界 |
| --- | --- | --- |
| 真实旧 CPU 基线 | 15 组物理 + 9 组玩法，两配置各重复三次通过 | 现有生产函数，不另写求解器或假 Renderer |
| CPU-only 全量 | Debug 58/58 启用项通过、1 项既有禁用；Release 59/59 通过 | 不将 Debug world.allocations 禁用计为通过，不关闭 Win32 1175/Fix1 |
| 关闭测试与依赖 | 两配置 BUILD_TESTING=OFF 生产 simulation 构建通过；实际基线命令无 app/renderer/platform/图形 SDK；物理 cpp 只编译一次 | CPU 配置仍有中立 renderer 门面，不代表基线反向依赖它；标准 Windows 默认系统链接库不当原生后端 |
| 身份 | before→final 的生产 CPU/依赖/资产未变；final→after 的生产/依赖/资产/build/fixtures SHA-256 一致 | 只锁定所列来源；app 是参考，不是完整仓库或图形包冻结 |

最终原始构建在 `out/m3-t4/cpu-b02-debug` / `cpu-b02-release`；初轮身份、成功记录与沙箱 C1902 原失败保留在 `out/m3-t4/cpu-baseline-20261009-001`，未覆盖原记录。独立核心、旧全局预算撤销、双会话、D12 和 A24–A27 均未由本批完成。
