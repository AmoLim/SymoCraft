---
tags:
  - area/meta
---

# RenderDoc/Tracy 规划提交划分

## 当前授权与基线

2026-10-28 本轮明确要求“给 integrate-renderdoc-tracy 的 spec 改动进行 Git 提交划分，先更新 gitspec.md，不要暂存或提交”。本轮只替换本页，不修改其他文件、不调整索引、不提交或推送；旧 ECS/Memory 批次授权不沿用。

当前分支 `feature-integrate-renderdoc-tracy`，HEAD 为 `f7ac7a89cb318e0e47febbf877e074db16262bf4`。旧 gitspec 属于已经完成的 ECS/Memory 批次，本页取代旧计划，不追加提交历史。

盘点起点并非空索引：已有12个新增路径暂存，其中本 change 9项、独立 NPC proposal 2项、个人文档1项。起点工作区相对索引无差异；本页修改后保持未暂存。本轮不会把“已暂存”解释为当前提交授权，也不会擅自取消其他文件的暂存。

本 change 当前为 `symocraft-lite`，创建日期2026-10-27；短设计、N1–N4 分工及唯一 S1–S10 任务清单均在 proposal.md，七份 delta 定义目标行为。所有实施任务尚未勾选；本批没有生产实现、工具安装、实际捕获、验收、主规格同步或归档结果。

## 提交划分

| 组 | 拟提交消息 | 完整意图 | 路径数 |
| --- | --- | --- | --- |
| G1 | `docs(profiling): 规划RenderDoc与Tracy接入及旧性能链退役` | change 元数据、完整 proposal、七份相互依赖的 delta 与本页一起提交 | 10 |

本轮只需一组原子文档提交，不为满足“分步”而制造不完整规格快照。N1–N4 是后续实施的编辑责任及交接节点，不是四个可独立落地的规格包；S5/S7 跨应用与渲染，退役边界又分布于多个原 capability。仅先提交 proposal 会引用尚不存在的 delta，仅提交工具接入而遗漏退役边界则不能完整表达已选方向。

未来实施提交需要在实际生产差异出现后另定批次，不将当前规划提交冒称 N1/N2/N3/N4 已交付，不在本页复制第二份实施任务清单。Git 消息使用 docs，是因为本次保存规划文档；父 Issue 使用 feature 不意味着本次已经新增运行能力。

### G1 准确文件清单

9份 change 文件相对 HEAD 均为新增，当前已暂存624行；本页为已有文件的替换修改，当前不暂存。以下10项全部按整文件候选纳入，不使用目录通配，后续新增文件和并发修订不自动进入本组。

```text
docs/openspec/changes/integrate-renderdoc-tracy/.openspec.yaml
docs/openspec/changes/integrate-renderdoc-tracy/proposal.md
docs/openspec/changes/integrate-renderdoc-tracy/specs/application-lifecycle/spec.md
docs/openspec/changes/integrate-renderdoc-tracy/specs/development-profiling/spec.md
docs/openspec/changes/integrate-renderdoc-tracy/specs/foundation-files/spec.md
docs/openspec/changes/integrate-renderdoc-tracy/specs/module-boundaries/spec.md
docs/openspec/changes/integrate-renderdoc-tracy/specs/platform/spec.md
docs/openspec/changes/integrate-renderdoc-tracy/specs/renderer/spec.md
docs/openspec/changes/integrate-renderdoc-tracy/specs/telemetry-lifecycle/spec.md
docs/git/gitspec.md
```

### 完整交付意图

- development-profiling：外部 RenderDoc 捕获/回放、有限 Tracy CPU 区域/帧标记/计数、工具与配置身份、历史证据保护。RenderDoc 不进入产品依赖，不新增 Tracy GPU 时间线或全局分配器挂钩。
- module-boundaries：默认关闭的窄 profiling 适配、唯一静态 client、固定来源与许可、Release 优化/符号及 Debug DBG01；CPU-only/基础 foundation/ECS/独立 physics 不承接无关依赖。非性能 document_io 原义迁到 foundation 所属独立 YAML 适配 target。
- application-lifecycle 与 platform：独立 static/walk/edit 场景、新参数及有界时限、旧参数拒绝、输入隔离与退出分类；保留 walk→spawn、static/edit→four-chunk、初始45度FOV、真实时间驱动及0.25秒编辑节奏。普通玩法和 SimulationSession 业务职责不被性能链退役取代。
- telemetry-lifecycle：性能 Recording/Sealed/Export 退役，改为常量级、自有只读的 FPS/帧间隔/实际物理 TPS/坐标/后端快照，缺值及暂停恢复明确；不新增 HUD 或第二模拟/计步权威。
- renderer：CPU 观测和两现代后端原生命名/成对 marker，退役旧计时 query/Poll；保留截图、实际配置/设备身份、部署/资产要求及正确性 fence/readback/资源寿命。旧 A02/A17/A18、预算和 strict 准备的适用范围在原 capability 内分别界定，不靠新增工具能力跨路径覆盖。
- foundation-files：只撤销旧性能终态消费者要求，不修改 Files::Publish 机制、错误码或晚期失败边界。1175/诊断完整性不因消费者退役视为关闭，Fix1 继续暂停。
- proposal：保留单一 change、S1–S10 和 N1–N4 的分工/文件归属/依赖。N4 先统筹 S1，共同接口成立后 N1–N3 可并行；共享构建归 N1，最终同一候选由 N4 汇合。并行开发安排不是本轮派发任务或实施授权。

以上是既有目标的提交范围说明，不在本轮重写 proposal、delta、主规格或设计决定。真实删除清单、代码根实施授权、检查范围/有界人工时长及最终接受仍分别确认。

## 规划输入身份

以下是盘点时九份已暂存文件的 Git blob 身份；用于后续发现并发内容变化，不是构建或工具验证。工作区与这九份索引版本当前一致。本页不记录自身 blob，避免循环身份。

| 路径 | 起点索引 blob |
| --- | --- |
| `docs/openspec/changes/integrate-renderdoc-tracy/.openspec.yaml` | `3b5f933c75f56bcc2ff9c17dbb72ed4c29599bac` |
| `docs/openspec/changes/integrate-renderdoc-tracy/proposal.md` | `0cec846aae3d35504db23c52b29ea944401aa9ed` |
| `docs/openspec/changes/integrate-renderdoc-tracy/specs/application-lifecycle/spec.md` | `eb953436af8a1288ea53622bdc61d60179f14e5c` |
| `docs/openspec/changes/integrate-renderdoc-tracy/specs/development-profiling/spec.md` | `f5853e0963bd776b57f03c5eb6e46a08a2dfd9bf` |
| `docs/openspec/changes/integrate-renderdoc-tracy/specs/foundation-files/spec.md` | `c1631caa3fba6a318204e3eebf2bf39f25d67729` |
| `docs/openspec/changes/integrate-renderdoc-tracy/specs/module-boundaries/spec.md` | `bf01218c3277cf23db3a5da21b15322c185a4710` |
| `docs/openspec/changes/integrate-renderdoc-tracy/specs/platform/spec.md` | `80e0c01cdffbb89c49e7c14a8f2810c7738a7807` |
| `docs/openspec/changes/integrate-renderdoc-tracy/specs/renderer/spec.md` | `a602147746b253b7b4e73fa58bfd0e8bee16b3ec` |
| `docs/openspec/changes/integrate-renderdoc-tracy/specs/telemetry-lifecycle/spec.md` | `2ed55620f46201bbbe54d22042bc72ce0f5c0705` |

## 明确排除与依赖缺口

以下三项虽然已在索引中，但不属于本次 change 的提交清单：

```text
docs/openspec/changes/npc-framework-harvest-mvp/.openspec.yaml
docs/openspec/changes/npc-framework-harvest-mvp/proposal.md
docs/development-journal/project-home.md
```

不读取个人文档正文，不修改其内容、跟踪状态或忽略规则。其余个人 journal、Obsidian、被忽略的 .agents、旧缓存/包、冻结证据、其他 change、生产代码与主规格均不纳入。当前没有其他生产或现役主规格差异可因本批而提交。

proposal 引用 NPC 草案作范围说明，但该草案尚不在 HEAD 中。若本组单独提交，这条链接在不含 NPC 的 Git 快照/云端页面暂时无对应文件；这是明确保留的跨批次文档依赖，不是本 change 的运行依赖。后续如需闭合该链接，应另行确认 NPC 的独立提交范围，不在本轮偷带两份 NPC 文件、重写其正文或删除链接来消错。

## 后续执行边界

1. 本轮不执行 git add、restore --staged、reset、commit、push、merge 或任何索引重组；已有暂存内容原样保留。
2. 获得新的明确提交授权后，先复核分支、HEAD、九份 blob、工作区差异及10项清单，重新审阅发生变化的内容，不把并发变更自动纳入。
3. 当前索引混有三项排除文件，不能直接执行会提交全部索引的裸 git commit。后续须明确采用保留排除项索引状态的限定路径提交方式，或在获得相应索引调整授权后仅取消其暂存；无论哪种方式都不删除或覆盖其工作区文件。
4. 对实际拟提交树核对准确路径、status/mode/blob、完整 diff、冲突标记/意外私密信息及空白；不得把其他已暂存内容作为“顺手提交”。本页随 G1 保存，实际提交结果在聊天报告，不追加旧批次历史。
5. 只做必要 Git 检查，不自动追加构建、CTest、OpenSpec validate、全库文档审计、游戏/Viewer/RenderDoc 启动、GPU/人工/性能检查。未执行项明确记未执行；规划完整和历史成绩都不是本轮验证或实施通过。

本轮编写 Git 批次计划不是 OpenSpec proposal 修订、apply、sync/archive、准确删除清单批准或创建外部 Issue 的授权。新工具轨迹也不自动等价于旧 CSV 预算；其他机器验证仍按 scope 后续另行安排。

## 本轮检查事实

本轮只读取 Git 状态、目标规划与差异身份，替换本页，并检查本页空白、路径覆盖、HEAD及索引保持情况。没有修改或提交任何生产代码、规格正文、个人文档或冻结证据；没有暂存、取消暂存、提交或推送，没有运行工程、工具或集成验证。
