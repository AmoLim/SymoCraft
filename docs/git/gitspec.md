---
tags:
  - area/meta
---

# 新增代码许可切换与规格归档提交范围（已批准）

2026-11-17 用户先要求整理本次许可切换、主规格同步和归档的提交范围，更新本文件供审核且先不要提交；随后明确同意本文件的 16 个文件范围，接受环境恢复提案的跨批引用限制，授权按计划创建一笔提交，保留其他未提交改动和个人日记的暂存状态，暂不推送。**当前授权仅覆盖下表的一笔提交，不授权推送、合并、标签或历史改写。** 此文件替换旧批次说明，不继承旧提交授权；旧内容仍可从 Git 历史恢复。

代码根为 `G:/Projects/Symocraft`，当前分支 `npc-framework-harvest-mvp`，审阅起点/许可分界均为 `a6620be9e3e6fbc4a06244686d971b6e6ce754a9`。本轮不切换分支或创建工作树。

## 提交意图

建议一笔完整提交：`chore(license): 切换新增代码许可并同步归档规格`。

历史 MIT 代码、新增 Apache-2.0 贡献、混合文件声明和发行随附材料构成同一许可交付；一并纳入消除规格格式警告的文档检查适配、现役主规格、已验收归档及批准导航，避免出现根许可先切换而历史授权/范围说明尚未落地的中间状态。用户已验收 change 并授权同步及归档，这些决定不等于本批 Git 提交授权。

## 准确范围

下表为候选提交唯一文件清单，共 **16 个文件：8 个修改、8 个新增**。状态相对当前 HEAD，尚未构造本批暂存集。

| 状态 | 路径（相对代码根） | 本批内容 |
| --- | --- | --- |
| M | `.gitattributes` | 仅保留 `LICENSE-MIT` 原字节的 `-text` 规则及混合许可范围声明；不更改其他原规则 |
| M | `LICENSE` | 完整 Apache 2.0 英文正文，适用范围由 NOTICE 和贡献规则明确 |
| A | `LICENSE-MIT` | 基线根 LICENSE 原字节，保留 2022 ACTOTG 署名和全文 |
| A | `NOTICE` | 基线 MIT、新原创 Apache、混合文件与第三方/素材例外 |
| A | `CONTRIBUTING.md` | 有权贡献、新增/混合代码标记、来源例外及分发规则 |
| M | `README.md` | 仅新增许可段及许可/贡献说明入口 |
| M | `game/app/CMakeLists.txt` | 仅新增三份项目许可的安装块及文件内混合许可说明；原构建和第三方通知安装规则保留 |
| M | `docs/third-party.md` | 仅新增项目许可边界和三份包根材料说明 |
| M | `tools/openspec/check_documents.py` | 无 YAML 前言时识别独立行 area/topic 标签，保留 YAML 优先级，并标明新增 Apache 代码范围 |
| A | `docs/openspec/specs/repository-licensing/spec.md` | 新主规格：原样 Purpose、4 条 requirement、6 个场景 |
| A | `docs/openspec/changes/archive/2026-11-17-adopt-apache-2-for-new-code/.openspec.yaml` | 原生 change 元数据，symocraft-lite |
| A | `docs/openspec/changes/archive/2026-11-17-adopt-apache-2-for-new-code/proposal.md` | 已批准提案、NOTICE 文案、唯一任务清单 6/6 和必要链接重定位 |
| A | `docs/openspec/changes/archive/2026-11-17-adopt-apache-2-for-new-code/specs/repository-licensing/spec.md` | 已实施 delta，顶部格式修复后严格校验通过 |
| A | `docs/openspec/changes/archive/2026-11-17-adopt-apache-2-for-new-code/verification.md` | 唯一验证正文，保留原结果、复核、用户验收、同步归档和 ZIP 删除记录 |
| M | `docs/verification/VERIFICATION.md` | 仅新增本 change 的验收批准及归档导航两行 |
| M | `docs/git/gitspec.md` | 本次已批准提交范围及执行边界 |

活动 change 目录未曾进入 HEAD，四份归档材料在本批均是新增文件；不能宣称 Git 中已有四份旧文件被重命名或删除。源码验证 ZIP 原本被 `out/` 忽略且未被跟踪，获授权的本地删除没有 Git 删除条目，也不把 ZIP、日志、安装包或检查摘要加入本批。

## 排除并保留

- `.gitignore`、`CMakePresets.json`、`.python-version`、`pyproject.toml`、`uv.lock`：现有环境恢复输入，与本批许可交付无关。
- `docs/openspec/changes/restore-windows-development-environment/` 整个未跟踪 change：本批不包含其元数据、proposal 或 development-environment delta。
- `docs/openspec/changes/integrate-renderdoc-tracy/specs/application-lifecycle/spec.md`、`platform/spec.md`、`renderer/spec.md`：既有工作区差异不纳入，也不为消除状态标记而改写换行。
- `docs/tag-inventory.json`：现有未提交索引保持，不借归档重新生成。
- 已暂存的 `docs/development-journal/project-home.md`：只核对路径及暂存条目，未读取个人正文，不纳入本批，不取消其暂存或改写其 blob。
- 其他主规格、既有归档/冻结材料、vendor/lib/素材、旧缓存及 out 产物、个人配置：不扩展范围，不清理、不重打包、不统一改证。

归档 proposal 对环境恢复 change 保留了关联链接，但其目标仍未跟踪且被本批排除。因此本机链接存在不等于独立取得本批提交后该关联链接也存在；这是跨批文档引用缺口，不为修补它擅自纳入整份环境恢复计划。用户已明确接受该关联引用限制；根许可、安装规则及新主规格的实现不依赖该 change。

## 验收与验证依据

用户于 2026-11-17 明确验收 `adopt-apache-2-for-new-code`，要求主规格同步后归档并删除源码验证包，已完成；权威依据见[验收与同步归档记录](../openspec/changes/archive/2026-11-17-adopt-apache-2-for-new-code/verification.md#2026-11-17-验收与同步归档)及[新主规格](../openspec/specs/repository-licensing/spec.md)。

- 历史 MIT 副本与基线原 blob 一致；NOTICE 与批准文案一致，根 Apache 正文与仓库完整官方参考一致。
- 既有实施检查：Release 配置/318 步构建/安装成功；安装包三份项目许可及 25 份第三方通知逐字节一致。文档标签适配 7 项针对性检查通过。4596 文件源码验证包的 CRC、成员哈希、许可及范围声明检查通过；该 ZIP 后来按用户要求删除，历史结果保留，不声称当前仍可打开它复算。
- 归档前本 change 严格校验无 issues；新主规格严格校验无 issues，Purpose/4 条要求/6 场景与 delta 对应内容一致；全部主规格普通校验 12/12 通过。全库额外严格检查仍有三个其他能力的四条既有长文本提示，不冒称全库严格校验通过。
- 本轮只整理范围并复核当前文件、许可文本、主规格及归档结构；不重复构建、CTest、游戏/GPU、性能或干净机检查，不把已有成绩当作新提交已运行验证。历史素材权属和 yaml-cpp 来源缺口没有因此关闭。

## 审核与提交执行边界

范围整理阶段仅更新本文件，没有暂存或提交。获准执行本批前，原暂存区只有上述个人日记，**并不是本批候选提交**；其正文不读取、原暂存 blob 与状态必须保持。

本批提交授权现已成立。执行时重新核对 HEAD、分支和全部候选文件，检查并发改动；仅按上表构造和审阅候选差异，确认精确文件集合、各处差异、必要新增文件及原字节，禁止全工作区暂存或直接把现有暂存区整体提交。创建提交时限定本批路径并隔离已有个人暂存内容，保留其原 blob 与暂存状态；提交后核对提交恰含 16 个文件、父提交正确、其他工作区文件未变且日记仍独立暂存。新出现的改动不自动加入，暂不推送。
