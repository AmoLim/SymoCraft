---
tags:
  - area/meta
---

# 正式 physics 模块迁移提交计划

2026-11-16 用户在本聊天明确验收通过 `promote-physics-module`，授权提交本次修改，要求先拟定本文件并同步对应主规格。本批实施/提交根为 `G:/Sandbox/Worktrees/promote-physics-module/Symocraft`，起点 `8ee84fa417ce96e4c25cd20873dac48a59a2de4b`。当前工作树此前为分离 HEAD，提交保存至 `codex/promote-physics-module`；主工作区分支不移动。

## 提交意图

一笔提交：`refactor(physics): 正式迁移物理模块并同步验收规格`。

将唯一生产 owner 原字节迁到正式 physics 模块，同时提交 CMake 接线、边界/消费者保护、完整变更材料、验收决定及 `physics-core` / `module-boundaries` 主规格。源码迁移和新路径检查相互依赖，组成一个完整可验证意图，不拆出缺少对应检查的中间提交。变更仍留活动目录，不归档。

## 准确范围

| 范围 | 路径 |
| --- | --- |
| 唯一生产 owner 原字节迁移 | `game/modules/simulation/physics/{CMakeLists.txt,src/physics.cpp,include/symocraft/physics/physics.h}` → `game/modules/physics/` 下相同三文件 |
| 构建接线 | `game/CMakeLists.txt`、`game/modules/simulation/CMakeLists.txt` |
| 边界与消费者 | `cmake/ModuleBoundaries.cmake`、`cmake/PublicHeaderTests.cmake`、`test/integration/CheckNegativeBoundaries.cmake`、`test/unit/profiling/check_configuration.py` |
| 本 change 完整材料 | `docs/openspec/changes/promote-physics-module/.openspec.yaml`、`proposal.md`、`verification.md`、`specs/physics-core/spec.md`、`specs/module-boundaries/spec.md` |
| 主规格同步 | `docs/openspec/specs/physics-core/spec.md`、`docs/openspec/specs/module-boundaries/spec.md`：合并三个 MODIFIED requirement，保留其他全部 requirement/scenario，重定位相对链接，不把 delta 标记写入主规格 |
| 当前状态、机制及导航 | `AGENTS.md`、`README.md`、`docs/README.md`、`docs/architecture/Overview.md`、`docs/architecture/engineering.md`、`docs/architecture/simulation.md`、`docs/verification/VERIFICATION.md`、`docs/openspec/config.yaml` |
| 当前批次治理 | 本文件、`docs/DOCUMENTATION_MAP.md` 中仅将过时“当前无批次”改为通用条件规则 |

旧 gitspec 无未提交差异，其已提交内容可从本批起点恢复；本文件替换旧批正文，不追加旧提交计划。原字节临时复核副本保留于本工作树 `out/physics-move/commit-20261116/`，不纳入提交。

## 验证与提交前检查

- 既有验收依据为 2026-11-13 的八配置构建、192/192 定向检查、130 个边界夹具及前后核心字节对账，详见 [verification.md](../openspec/changes/promote-physics-module/verification.md)。本轮先核对当前源码/测试/资产/构建输入与该已验收快照一致，历史结果不冒称为本轮重跑。
- 本轮只更新规格/文档，不重跑无变化的生产构建，不启动游戏；当前执行主规格校验、场景保留/链接检查、`git diff --check` 及准确暂存差异审核。实际结果写同一 verification.md。
- 同步主规格仅采用 CLI 当前返回的两份 delta；以当前基线语义合并，保留 profiling/YAML/旧产品退役要求及 T4 历史 A23 的当时含义。
- 两份 delta 顶部标签引起的既有严格校验警告分别报告，不删标签或冒称严格校验通过；GPU、人工玩法、性能、长时及其他机器未测事实继续保留。
- 仅显式暂存上述范围，核对 staged 文件集、重命名的字节一致性及完整 staged diff 后才提交。提交前后核对工作树与主工作区保留状态。

## 主工作区与后续边界

持续同步本批相关文档供主工作区审阅，按两处源码状态分别措辞；主工作区 `G:/Projects/Symocraft` 的生产代码仍在 `simulation/physics`，本批不传播生产代码、不移动其分支、不提交其索引。主工作区已有 NPC/环境恢复/profiling 增量、个人日志、Obsidian、`.gitignore`、预设、UCRT 接线、Python 环境及索引等其他改动均保留。

本轮验收关闭 physics 独立节点门槛，不授权 NPC 功能实施、合并/推送、归档或清理旧缓存/冻结证据。主规格更新与用户验收不扩张为主工作区生产传播或新的运行成绩。
