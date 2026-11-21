---
tags:
  - area/spec
---

# 新增代码采用 Apache 2.0，历史代码保留 MIT

## Why

2026-11-17 用户确认以 `a6620be9e3e6fbc4a06244686d971b6e6ce754a9`（简称 `a6620be9`）为许可分界：历史项目代码保留 MIT，后续新增的原创代码采用 Apache-2.0。提案起点的根许可为 MIT、`Copyright (c) 2022 ACTOTG`，原字节在实施后由 [LICENSE-MIT](../../../../../LICENSE-MIT) 保存；本次保留既有授权，明确新贡献与混合文件的适用范围。

依据[项目范围](../../../../scope/project-scope.md)、[文档治理](../../../../DOCUMENTATION_MAP.md)、[第三方盘点](../../../../third-party.md)及全部十一份现役规格；关联的[模块边界](../../../specs/module-boundaries/spec.md)、[平台部署](../../../specs/platform/spec.md)和[渲染部署](../../../specs/renderer/spec.md)继续成立。条款依据为 [Apache 2.0 第 2–6 条](https://www.apache.org/licenses/LICENSE-2.0)、[官方应用说明](https://www.apache.org/foundation/license-faq.html#Apply)及 [MIT 正式文本](https://opensource.org/license/mit)。

## What Changes

- 将根 `LICENSE` 设为完整、未改写的 Apache 2.0 英文正文；先把基线根 `LICENSE` 原字节保存在根 `LICENSE-MIT`，保留其原年份、署名及免责声明。根 `NOTICE`、README 许可段和贡献说明共同明确两者的适用边界，不能仅替换根许可而宣称全仓库已重许可。
- 基线及其历史版本中原受 MIT 授权的项目代码继续保留 MIT；后续由有权贡献者提交并按新规则接受的新增原创代码、原创修改采用 Apache-2.0。旧代码的移动、复制、格式化、拆分、重命名及保留旧表达的重构不改变其许可。
- 全新自有源文件标记 `SPDX-License-Identifier: Apache-2.0`；旧文件实际加入原创修改时保留 MIT 来源，并明确新增修改的 Apache 范围。混合文件使用 `MIT AND Apache-2.0` 配合范围说明，不能写成任选许可的 `MIT OR Apache-2.0`，也不能只用 Apache 标记覆盖旧代码。
- 随源码及游戏发行包提供 `LICENSE`、`LICENSE-MIT`、`NOTICE`，延续既有第三方通知。新增贡献规则说明提交者应具有授权权利、不发生版权转让；不能代表旧作者追加专利授权。
- 第三方、图片/纹理/PSD、外部材料及冻结证据不由本政策统一改证。现有素材来源和 yaml-cpp 精确上游版本未明的缺口继续明示；本 change 不把它们判定为已获 Apache 或 MIT 授权。

## Capabilities

- **新增 `repository-licensing`：** 基线分界、新贡献和混合文件的许可范围、NOTICE 与发行随附材料，见[增量规格](specs/repository-licensing/spec.md)。分发材料和贡献契约发生变化，因此保留该 delta，不设置 `skip_specs: true`。
- **无游戏行为变化：** 不修改十一份现役游戏/工具能力，不改变模块、算法、渲染器、工具链、运行参数及验收预算。

## Impact

实施预计影响根 `LICENSE`、新增 `LICENSE-MIT`、`NOTICE`、`CONTRIBUTING.md`，README 许可段、`docs/third-party.md` 的项目许可边界及[游戏安装规则](../../../../../game/app/CMakeLists.txt)。只对本次真正加入原创修改的代码/构建文件补充适用声明，不批量重写历史源文件或第三方头。

[Tracy change](../../integrate-renderdoc-tracy/proposal.md)已维护依赖通知，本 change 仅追加项目许可随附材料；[环境恢复 change](../../restore-windows-development-environment/proposal.md)与本 change 共享 README 等入口，实施前重读最新改动，保留其现有未提交内容。CLI 列出的 `m3-t2-publication-fix1` 残留空目录没有可读活动规划；正式[归档记录](../2026-11-03-m3-t2-publication-fix1/verification.md)不因本 change 恢复或修改。

用户此次授权创建 lite 提案及 NOTICE 文案。本轮仅写 change 规划产物，根文件及安装规则留待独立实施；不进行 Git 提交/推送、规格同步、归档或实际发行。当前工作区的未提交改动不属于基线树，必须核对来源和贡献者权限后再确定许可，不能按提交日期或本次许可政策一键归类。

2026-11-17 后续授权：用户同意提案，明确授权在 `G:/Projects/Symocraft` 实施许可、贡献说明及安装规则，保留现有未提交改动，暂不提交 Git。上述仅规划边界保留其前一时点含义；本轮实际交付与检查见 [verification.md](verification.md)，不代表用户已完成独立验收，也不扩展为主规格同步、归档或正式发行。

## Implementation

### Short Approach

以完整基线提交标识内容来源，文件路径及提交时间只作辅助。对旧文件的原创增量，在文件中说明受修改的函数/区域和许可；Git diff 辅助追溯，独立源码包本身也必须读得懂许可范围。来源无法判明时先报告并保留原声明；不默认归 Apache。自有构建/自动化代码与游戏源码使用同一规则；文档、美术及外部输入不因扩展名或存放路径被自动归类。

实施时先保存并逐字节核对基线 MIT 正文，再放置 Apache 正文、根 NOTICE、README 与贡献说明。新贡献默认按 Apache-2.0 提交；显式其他许可或来源不明时先审查，不自动接受、不强制旧贡献者补签，也不把本政策当成 CLA 或版权转让。Apache 后续修改文件须保留适用归属并标明修改事实。

### NOTICE text

以下为拟写入根 `NOTICE` 的完整文案，本提案中保留供审核；实施时按原文落地。它是归属与范围说明，不添加许可条件，也不替代两份完整许可正文。

```text
Symocraft

Historical project code through and including baseline commit
a6620be9e3e6fbc4a06244686d971b6e6ce754a9 that was licensed under the MIT
License remains available under that license. The original project notice is:
Copyright (c) 2022 ACTOTG
The complete historical MIT license is included in LICENSE-MIT.
Other applicable copyright and attribution notices remain in effect.

New original project code and original modifications contributed after this
baseline and accepted under the project's contribution policy are licensed
under the Apache License, Version 2.0, as provided in LICENSE.
Copyright in new contributions belongs to the respective copyright holders.

Files containing both historical MIT code and new Apache-2.0 contributions
retain the applicable notices for both portions. Moving, copying, renaming,
splitting, or reformatting historical code does not change its license.
File-level notices identify the applicable portions; the baseline identifies
the historical project code. This notice does not relicense historical code
or grant patent rights on behalf of its original contributors.

Third-party components retain their respective licenses and notices.
See their source notices and the accompanying third-party license materials;
game distributions include third-party notices in licenses/.
Artwork, textures, reference images, and other non-code assets are outside
this code licensing policy; consult their applicable permission records.
This notice does not grant rights in assets without an established license.

This NOTICE is informational and does not modify either license or replace
the full license texts and applicable third-party notices.
```

### Tasks

- [x] LIC-S1 在实施入口核对基线和最新工作区，逐项判明相关未提交改动的来源/权利；保持用户、第三方及冻结材料，无法判明的项目单独报告，不能误标许可。
- [x] LIC-S2 从基线原字节保存 `LICENSE-MIT`，提供官方完整 Apache `LICENSE` 和上述根 `NOTICE`；核对 MIT 署名与正文未变、Apache 正文完整且 NOTICE 不增加许可条件。
- [x] LIC-S3 更新 README 和新增 `CONTRIBUTING.md`，说明基线、新增/混合代码、拷贝重构和来源例外；对本次实际修改的自有代码/构建文件补充声明，避免批量头部重写和任选双许可误导。
- [x] LIC-S4 在游戏安装规则中追加项目三份根许可材料，随源码包保留相同材料；更新第三方盘点中的项目边界，保持所有现有第三方许可内容、部署条件及已知缺口。
- [x] LIC-S5 执行 OpenSpec 严格校验、链接及声明一致性检查；用适用现有构建配置执行受影响配置/安装到新的临时目录，逐字节核对三份项目材料及原第三方通知仍在，确认不依赖 Git 或联网阅读。检查代表性 MIT 原文件、新 Apache 文件和混合文件的适用范围；仅许可/安装变动不追加完整游戏、GPU、性能或干净机验收。
- [x] LIC-S6 在本 change 唯一 `verification.md` 汇总实际结果和未覆盖项，交用户审核；本次提案完成不勾选实施任务，不宣称已换证、已发行或已解决历史素材权属，不自动 sync/archive/Git。
