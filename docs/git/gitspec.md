---
tags:
  - area/meta
---

# 正式 physics 模块归档提交计划

2026-11-16 用户在验收、主规格同步和实施提交完成后，另行明确要求归档 `promote-physics-module`，随后修改本文件并创建对应提交。本批代码根为 `G:/Sandbox/Worktrees/promote-physics-module/Symocraft`，分支 `codex/promote-physics-module`，起点为实施提交 `74532909a41bfc07e3f3a0baa841a1694d442236`。

## 提交意图

一笔提交：`docs(physics): 归档物理模块迁移与验证记录`。

将已完成 change 的五份材料整体移入 `docs/openspec/changes/archive/2026-11-16-promote-physics-module/`，保留 .openspec.yaml 与唯一 verification.md；重定位归档材料和当前维护文档中的必要链接，更新归档导航和当前上下文。本批不重新提交生产代码，不再次合并已同步的 requirement。

## 准确范围

| 范围 | 路径 |
| --- | --- |
| 五文件归档 | `docs/openspec/changes/promote-physics-module/` → `docs/openspec/changes/archive/2026-11-16-promote-physics-module/`：`.openspec.yaml`、`proposal.md`、`verification.md`、`specs/module-boundaries/spec.md`、`specs/physics-core/spec.md` |
| 当前导航及上下文 | `README.md`、`docs/README.md`、`docs/architecture/engineering.md`、`docs/architecture/simulation.md`、`docs/verification/VERIFICATION.md`、`docs/openspec/config.yaml` |
| 主规格验证引用 | `docs/openspec/specs/physics-core/spec.md`、`docs/openspec/specs/module-boundaries/spec.md`：仅将验证记录链接改指归档位置，Purpose、requirements 和 scenarios 不变 |
| 当前批次 | 本文件替换上一提交批次计划；旧计划可由本批起点提交恢复，不保留第二计划 |

## 前置依据及检查

- symocraft-lite 的 proposal/specs 均完成，项目入口返回任务 5/5；用户明确验收及本轮归档授权已成立。
- 归档前两份 delta 与主规格逐 requirement/场景/链接语义对比均为已同步，采用本轮明确的“直接归档”选择，无需再同步。
- 已有 192/192、八配置与 130 个夹具结果及当前源码输入身份维持原范围，见[归档 verification.md](../openspec/changes/archive/2026-11-16-promote-physics-module/verification.md)。本轮只检查文档、主规格和归档状态，不重跑构建、不启动游戏。
- .openspec.yaml 原字节、材料集合与各段历史命令/结果保留；只重定位必要 Markdown 链接，既有旧冻结归档、out、个人文件及其他 change 不动。
- 两条 delta 标签前言兼容性告警以及本树缺失的 NPC/环境恢复关联引用继续列明，不冒称严格校验通过或恢复缺失材料。
- 核对活动列表已排除本 change、主规格校验和链接检查、准确文件集、完整暂存差异及原生源码/测试零差异后提交；提交消息与本文件意图一致。

## 主工作区

相同归档位置及相关文档同步至 `G:/Projects/Symocraft`，按主工作区仍使用 simulation/physics 的实际源码状态维护说明。主工作区原有其他改动、暂存 blob、分支和 HEAD 保持；不传播生产代码、不创建主工作区提交、不推送或合并。
