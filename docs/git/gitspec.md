---
tags:
  - area/meta
---

# 许可恢复提交与推送计划

2026-11-21 用户要求拟定本文件，随后提交并推送；经实时查询及用户明确选择，目标确认为现有远端 `main`，采用“接入最新远端 main 后推送”。当前授权本批范围整理、创建一笔提交和正常快进推送，不授权强推或回退远端。本文件替换旧 review copy，不累计旧提交计划。

## 基线与目标

当前代码根 `G:/Projects/Symocraft`，本地分支 `main`，HEAD 为 `8ee84fa417ce96e4c25cd20873dac48a59a2de4b`。原许可分界仍为 `a6620be9e3e6fbc4a06244686d971b6e6ce754a9`，恢复来源为旧提交 `22d0cac381d329f559f0aae0b5cd692102c6e7d6`；恢复不包含正式 physics 或 NPC 功能改动。

远端 `origin` 为 `git@github.com:AmoLim/SymoCraft.git`。本轮实时查询 `refs/heads/main` 为 `ec5f023abf65875b086d9196f63f38276e038f97`。它是本次提交的父版本；原工作区的 `8ee84fa4` 仅作恢复来源，不推送该旧树覆盖远端。隔离接入目录为 `G:/Sandbox/Worktrees/license-main-recovery/Symocraft`，本地提交分支为 `codex/license-main-recovery`，推送目标明确为 `origin refs/heads/main`。

在远端最新版本上隔离接入下列许可增量，保留远端已经合入的 physics/NPC 等内容；README、Git 属性、第三方记录、安装规则和批准导航按实际目标版本合并，不整文件覆盖。远端已有代码按其已发布的 MIT 授权保留，包括历史分界之后、采用新贡献规则之前已发布的部分；不借换证将既有贡献自动重标为 Apache。采用正常快进推送，不强推、不倒退远端；原工作区的其他改动、恢复文件及个人日记暂存不动，原本地 main 的 HEAD 也保持。

## 一笔提交与准确范围

拟提交消息：`chore(license): 恢复新增代码许可并同步归档规格`。

仅含下表 **16 个文件（当前工作区相对 HEAD 为 8 个修改、8 个新增）**。许可政策、原字节保留、发行安装、内联标签适配、主规格及已验收归档一起交付；没有 Git 删除条目。若在最新远端接入，文件内容以保留目标已有内容后的必要增量为准，路径清单保持不变。

| 状态 | 路径（相对代码根） |
| --- | --- |
| M | `.gitattributes` |
| A | `CONTRIBUTING.md` |
| M | `LICENSE` |
| A | `LICENSE-MIT` |
| A | `NOTICE` |
| M | `README.md` |
| M | `docs/git/gitspec.md` |
| A | `docs/openspec/changes/archive/2026-11-17-adopt-apache-2-for-new-code/.openspec.yaml` |
| A | `docs/openspec/changes/archive/2026-11-17-adopt-apache-2-for-new-code/proposal.md` |
| A | `docs/openspec/changes/archive/2026-11-17-adopt-apache-2-for-new-code/specs/repository-licensing/spec.md` |
| A | `docs/openspec/changes/archive/2026-11-17-adopt-apache-2-for-new-code/verification.md` |
| A | `docs/openspec/specs/repository-licensing/spec.md` |
| M | `docs/third-party.md` |
| M | `docs/verification/VERIFICATION.md` |
| M | `game/app/CMakeLists.txt` |
| M | `tools/openspec/check_documents.py` |

## 排除并保留

- `.gitignore`、`CMakePresets.json`、`.python-version`、`pyproject.toml`、`uv.lock`，以及环境恢复 change：不纳入本批。
- `docs/openspec/changes/npc-resource-perception-memory/`、既有 integrate-renderdoc-tracy 的三份脏 delta、`docs/tag-inventory.json`：保留当前内容，不纳入本批，也不重新生成索引。
- 已暂存的 `docs/development-journal/project-home.md`：不读取个人正文，不纳入提交，保留原暂存 blob 和状态。
- 其他游戏源码、第三方原始材料、素材、旧冻结证据、out 构建及安装产物不纳入；此前源码验证 ZIP 已删除，不重新打包或提交。

归档 proposal 保留对未跟踪环境恢复提案的跨批链接限制，该限制此前已经用户明确接受；不借本批纳入环境恢复计划。在独立提交树中仍应明确区分该已接受的关联引用与许可/构建所需输入。

## 验证依据与提交检查

[恢复验证记录](../openspec/changes/archive/2026-11-17-adopt-apache-2-for-new-code/verification.md#2026-11-21-回档后恢复与重新构建)记载：回档代码重新执行 Release 配置、318 步构建和安装成功；三份项目许可、25 份第三方通知逐字节一致，7 项标签检查通过，主规格严格校验无 issues。旧候选的源码包检查仍是历史结果，ZIP 已按要求删除；未运行游戏、CTest、GPU/性能或干净机验收。

本轮已对最新远端目标完成主规格严格校验、7 项标签检查、原内容保留核对，以及新的 Release 配置/363 步构建/安装；三份项目许可和 37 份第三方通知逐字节一致。45 条本地链接仅保留上述已接受的跨批关联缺口，其余有效。目标结果见同一 verification.md 的“接入最新远端 main”段，不把本地回档版本的成绩移作目标成绩。提交前应再次检查精确候选文件集合、许可原字节、NOTICE 和四条 requirement/六个场景。完整审阅限定范围的实际暂存差异，禁止全工作区暂存或把现有日记暂存顺带提交。

创建一笔限定范围的提交，核对其父提交、16 个路径和提交文本；再次检查远端提交，向 main 正常快进推送。若远端并发前进则停止该次推送并重新评估，不改用强推。推送后核对远端目标提交与本地候选一致，并确认原工作区未提交内容、个人日记暂存和分支状态保持。目标树的构建、安装和通知检查完成后记入本 change 的 verification.md，结果不冒称游戏/GPU/性能或干净机验收。
