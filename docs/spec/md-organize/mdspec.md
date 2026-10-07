---
tags:
  - area/spec
---

# docs 文档结构与 Obsidian 图谱 Spec

## 状态与目标

- 确认日期：2026-10-04。
- 状态：已于 2026-10-04 按本 spec 完成目录迁移、导航与引用整理。校验范围见文末实施记录。
- 阶段说明：第一阶段目录整理已完成。第二阶段“Obsidian 标签与图谱边界”已于 2026-10-05 经用户授权实施，标签、导航与图谱文件配置的静态校验通过；Obsidian 实际 UI 验收待完成，详见文末第二阶段记录。
- 2026-10-05 归属修订：用户已将依赖盘点移入 `legacy/`，将 `git/` 和 `md-organize/` 移入 `spec/` 作为子模块。本文件已按实际路径更新结构和标签约定；第一阶段检查结果仍仅代表当时快照，不自动覆盖本次用户迁移。
- M3-T0 追加归属：用户要求各模块在 `project/architecture/<模块>/` 配套输出功能和类设计文档；现增加唯一板块 `area/architecture`。本次只更新标签与导航，不修改用户图谱配置；第九个颜色组未配置、未作 UI 验证，历史八组记录保持原结论。
- 范围：整理仓库 `docs/` 内 Markdown 文档的归属、导航和引用，不修改程序实现，不改写历史验证结论。
- 目标：明确区分历史架构参考、维护文档、未来设计和验证记录，让读者能够按用途找到文档，并识别其适用阶段。

## 目标目录

以下目录均相对仓库根目录，反映 2026-10-05 用户调整后的结构；树中省略部分具体文件。

```text
docs/
├── README.md                    # 分类导航
├── DOCUMENTATION_MAP.md         # 面向维护者与 agent 的文档归属参考
├── spec/                        # 项目范围、设计约定与规范子模块
│   ├── git/
│   │   └── gitspec.md           # 版本控制规范子模块
│   └── md-organize/
│       ├── mdspec.md            # 文档整理与图谱规范子模块
│       └── tag-inventory.json   # 逐文件标签与例外清单
├── legacy/
│   ├── architecture/            # 旧架构参考
│   └── third-party-inventory.md # 历史依赖盘点
├── project/
│   ├── architecture/            # M3 起实际模块功能、类设计与模块索引
│   ├── build/                   # 构建、CLion、CMake 边界与问题修复
│   └── benchmark/
│       ├── testing/             # 性能采样协议、固定测试场景
│       ├── performance/         # 性能观测相关设计
│       └── evidence/            # 保持现有证据位置
├── testing/                     # 通用玩法与回归验收
├── milestones/                  # 阶段报告及其原始证据
└── Obsidian-功能笔记模板/
```

Benchmark 是测试的一部分，不是所有测试的上级。通用玩法、启动失败和连续游玩验收不归入 Benchmark。

## 迁移清单

下表保留第一阶段已执行的迁移记录；2026-10-05 用户追加调整另列于表后。

| 现有路径 | 目标路径 | 处理约定 |
| --- | --- | --- |
| `docs/architecture/application-lifecycle.md` | `docs/legacy/architecture/application-lifecycle.md` | 保留原文，补充历史阶段与适用范围 |
| `docs/architecture/asset-paths.md` | `docs/legacy/architecture/asset-paths.md` | 同上 |
| `docs/architecture/current-data-flow.md` | `docs/legacy/architecture/current-data-flow.md` | 保留文件名，标题改为“M0 架构与数据流” |
| `docs/architecture/mesh-safety.md` | `docs/legacy/architecture/mesh-safety.md` | 保留原文，补充历史阶段与适用范围 |
| `docs/architecture/performance-observation.md` | `docs/legacy/architecture/performance-observation.md` | 同上；现有性能设计文档可继续引用此历史参考 |
| `docs/architecture/player-loop.md` | `docs/legacy/architecture/player-loop.md` | 保留原文，补充历史阶段与适用范围 |
| `docs/architecture/runtime-resources.md` | `docs/legacy/architecture/runtime-resources.md` | 同上 |
| `docs/architecture/world-generation.md` | `docs/legacy/architecture/world-generation.md` | 同上 |
| `docs/development/build-and-clion.md` | `docs/project/build/build-and-clion.md` | 明确 M1 适用范围，关联较新的构建说明 |
| `docs/testing/performance-baseline.md` | `docs/project/benchmark/testing/performance-baseline.md` | 明确旧脚本流程与适用阶段，关联较新的原生 Benchmark 说明 |
| `docs/testing/reproducible-scenes.md` | `docs/project/benchmark/testing/reproducible-scenes.md` | 注明通用玩法回归也复用这些场景 |
| `docs/testing/gameplay-smoke.md` | 不变 | 保留在通用测试目录，链接共享的固定场景文档 |

第一阶段迁移后，不保留空的 `docs/architecture/` 和 `docs/development/` 目录。当时 `spec/`、`milestones/`、Git 规范、第三方依赖盘点和 Obsidian 模板未调整归属；其中 Git 规范和依赖盘点的位置已由以下追加调整更新。

### 2026-10-05 用户追加调整

| 调整前路径 | 当前路径 | 当前归属 |
| --- | --- | --- |
| `docs/third-party-inventory.md` | `docs/legacy/third-party-inventory.md` | legacy 历史资料，使用 `area/legacy` |
| `docs/git/gitspec.md` | `docs/spec/git/gitspec.md` | spec 的 Git 规范子模块，使用 `area/spec` |
| `docs/md-organize/mdspec.md` | `docs/spec/md-organize/mdspec.md` | spec 的文档整理子模块，使用 `area/spec` |

上述文件已由用户移动，本轮不重复迁移。归属修订时仅更新本 spec；第二阶段实施已重新核对并修复受影响的文档入链、出链和结构参考，不沿用第一阶段“无断链”的结论。

## 内容与导航规则

### 文稿风格

2026-10-06 用户确认：默认使用精简、精准的 Markdown 文稿；阅读不明确时，由用户显式指出需要扩充的位置。

- 只保留职责、核心行为、必要边界与依据；不预先展开教程、术语解释或推演所有场景。
- 同一信息只维护一处；表格、Mermaid 与文字互补，不重复复述，细节通过源码或权威笔记链接定位。
- 精简不减少函数覆盖，不省略影响正确性的前提、失败后状态、所有权与失效规则；避免反复堆叠通用免责声明。
- 收到明确反馈后，仅扩充指定位置及必要关联内容，不因此全面加长其他文档。

适用于后续新增与整理。本次仅更新写作规则，不批量重写既有正文、原始证据或历史验收记录。

### 历史参考

- Legacy 表示历史架构参考，不表示文档作废，也不表示其中所有机制均已被替换。
- 各篇文档按已有证据注明对应阶段，不把整个目录误标为同一个阶段。
- 保留原文中的设计、问题、限制与历史结论；仅为归档定位补充必要说明、调整容易误导的标题和修正链接。
- 不将历史源码描述包装成当前实现，也不将未来 spec 当作已完成的实现说明。

### 维护文档

- 文件迁入 `project/` 不自动意味着其内容适用于最新版本。
- 构建指南保留 M1 范围，性能基线指南保留旧脚本流程的阶段边界，并提供较新说明的入口。
- 本轮不顺带重写构建命令、采样协议或实现细节；发现内容过时且需要实质更新时，明确记录待更新项。

### 入口导航

- `docs/README.md` 改为按用途组织，不再持续堆叠阶段更新。
- 分类包括：项目约定、构建与开发、Benchmark、通用验收、历史参考、文档规范。
- 详细阶段结果与验收状态由对应里程碑报告承载，导航保留入口，不重复维护多份结果。
- 为 legacy 架构和 Benchmark 测试补充必要的简短目录说明或索引，避免为每层目录机械增加 README。

## 引用与证据保护

- 检查仓库内指向迁移文档的 Markdown 链接、Obsidian 双链和作为导航使用的文字路径引用。
- 同步修正被移动文档内部的相对链接，包括指向源码、其他文档和证据的链接。
- 一并修复已发现的错位链接，例如 `docs/README.md` 中的项目范围、失焦采样入口，以及 Performance Session 文档中的性能观测入口。
- 本 spec 的“现有路径”属于迁移记录，迁移后仍保留，不视为需要替换的导航引用。
- 日志、截图、哈希、结果摘要及其他原始证据保持位置和内容不变。
- 报告中记录的实际执行路径、命令输出和历史快照路径不做全局替换；区分可点击导航和历史事实。
- 不修改测试通过数量、验收勾选状态、硬件验证状态或结论边界。

## 实施顺序

1. 核对现有工作区与文档内容，保留用户已有修改。
2. 按迁移清单调整文件位置。
3. 补充历史定位和版本适用说明，调整必要标题。
4. 更新总入口及必要的局部导航。
5. 修正仓库内受影响的文档引用，检查 Markdown 链接和 Obsidian 双链。
6. 审阅最终变更，确认没有误改原始证据、历史事实或验收状态。

## 验收标准

- [x] 清单中的文档已迁入约定位置，通用玩法冒烟仍保留在 `docs/testing/`。
- [x] 每篇迁移文档的用途、历史阶段或版本适用范围明确。
- [x] 旧架构参考不再被总入口误称为当前架构，未来设计不被描述为已实现。
- [x] `docs/README.md` 按用途导航，阶段详情链接到对应报告。
- [x] 迁移未引入断链，已识别的错位文档链接已修复。
- [x] 原始证据的位置与内容、历史执行记录和验收结论均未改变。
- [x] 未修改程序实现，未进行无关重命名、格式重写或目录调整。

文档链接检查不等于构建、玩法或性能验证；本次整理不新增任何程序或硬件验收结论。

## 实施与校验记录

- 完成日期：2026-10-04。11 篇文档按清单迁移，原 architecture 与 development 空目录已移除。
- 已新增 [文档结构与维护参考](../../DOCUMENTATION_MAP.md)，作为后续维护者与 agent 的归属入口；历史架构与 Benchmark 测试各补充一份局部索引。
- 总入口已按用途重组；已修复整理前发现的 5 处错位链接，并同步根 README、里程碑、spec 和相关模块说明中的迁移引用。
- 本地 Markdown 链接及图片目标存在性检查通过，文档锚点检查通过；源码行号仅保留历史定位，不声称已核对其语义。
- 39 处 Obsidian 双链中，38 处目标唯一可解析；剩余 1 处为原模板中的“基类名-类设计”占位示例，保持原样。另有 2 处模板引用的外部 Obsidian 本机路径，不属于仓库内导航，保持原样。
- 整理前后的 240 个证据文件逐一比较 SHA256，路径和内容均未改变；全部 241 个非 Markdown 文档树文件亦未改变。
- 原有文档的命令代码块保持不变；除本 spec 的整理验收项外，原有验收勾选状态保持不变。
- Git 差异空白检查通过。未运行程序构建、玩法、性能或硬件测试；本轮仅进行文档结构与引用校验。

模板示例外部路径、旧指南命令适用性和历史源码定位限制，统一见结构参考中的待独立核对项。本次未提交 Git，保留工作区中已有修改与暂存状态。

## 第二阶段：Obsidian 标签与图谱边界

### 状态与目标

- 提出日期：2026-10-05。状态：用户已授权按本 spec 实施；52 篇纳管笔记已补齐标签，图谱配置和静态校验已完成，UI 验收待完成。
- 保留第一阶段的方案、完成记录和验收结果；第二阶段结果单独记录，不修改游戏或硬件验收状态。
- 目标：通过统一 tag 明确每篇文档的唯一板块归属，在 Obsidian 图谱中以颜色区分板块，并能筛选出无其他板块笔记混入的单板块视图。
- 以仓库 `docs/` 作为本方案的 vault 根目录。已发现其中的 `.obsidian/graph.json` 和模板设置；若实际打开的是更大范围的 vault，实施前必须调整路径查询并核对其他笔记的标签冲突。
- 实施前快照：图谱开启标签节点、隐藏附件和未解析节点、显示孤立节点，尚无颜色分组。部分笔记已有 `type`、`status`、`module` 等属性，两个模板示例已有 `tags: [示例]`。实施后配置见文末记录。

### “硬边界”的含义与能力限制

本方案将硬边界定义为可检查的归属规则，而不是图谱引擎的空间隔离：每篇纳管笔记只有一个板块 tag，单板块筛选不能混入其他板块，标签与目录归属冲突必须报告。

Obsidian 原生图谱支持按搜索条件筛选笔记、按组着色和显示标签节点，但不提供按 tag 强制分区、固定集群位置或禁止跨区连线的保证。颜色区分不等于几何隔离；真实的跨板块引用仍须保留。嵌套标签用于层级检索，不承诺在图谱中自动变成目录树。依据：[Graph view](https://help.obsidian.md/plugins/graph)、[Tags](https://help.obsidian.md/tags)。

若后续要求“固定盒状分区、各区绝不相连”，应另立 Canvas 或其他可视化方案，不把安装第三方插件、修改链接或生成大量虚假枢纽笔记作为本轮默认措施。

### 标签模型

| 维度 | 命名 | 数量 | 职责 |
| --- | --- | --- | --- |
| 板块归属 | `area/<板块>` | 每篇纳管笔记恰好 1 个 | 决定图谱颜色、板块筛选和维护归属 |
| 关联主题 | `topic/<主题>` | 可选，默认 0，最多 2 个 | 表示真实跨板块主题，不改变归属 |

- 标签采用小写 ASCII 和短横线，不使用空格、大小写变体或中文同义标签另建一套分类。
- 板块 tag 使用下表封闭词表。不写裸 `area`，不同时写父子板块，不额外加第二个 `area/*`；新增板块须先更新本 spec 和结构参考。
- 主题初始词表为 `topic/build`、`topic/benchmark`、`topic/performance`、`topic/world`、`topic/rendering`、`topic/gameplay`、`topic/resources`。仅在有检索价值时添加，不根据正文关键词机械批量生成。
- `type`、`status`、`module` 沿用原有属性和语义，不再复制成类型、状态、阶段或类名 tag，避免出现大量连接所有板块的公共标签节点。
- 既有非归属标签，例如模板示例的 `示例`，保留其值并列入兼容清单；本轮不擅自删除或改名。
- 新增的分类标签仅写入 YAML frontmatter 的 `tags` 列表，值不带 `#`。示例中的标签使用代码格式展示，不在说明正文额外制造真实标签。

Obsidian 支持 `tags` 列表和斜杠嵌套标签，搜索父标签会包含子标签；属性键在单篇笔记内必须唯一。依据：[Tags](https://help.obsidian.md/tags)、[Properties](https://help.obsidian.md/properties)。

### 板块归属表

以下路径均相对 `docs/`。先应用证据和可复用模板例外，再按最具体路径规则匹配；新增且无法匹配的路径必须报告，不自动归入通用板块。

| 文档或目录 | 唯一板块 tag | 建议颜色 | 归属边界 |
| --- | --- | --- | --- |
| `README.md`、`DOCUMENTATION_MAP.md` | `area/meta` | 灰色 `#7F8C8D` | 全局文档导航与结构参考，不包含 spec 子模块 |
| `spec/`，含 `git/` 和 `md-organize/` | `area/spec` | 紫色 `#9467BD` | 范围、设计、任务约定及规范子模块，不因讨论某模块而归入实现板块 |
| `project/build/` | `area/build` | 橙色 `#E68632` | 构建、CLion、CMake 与相关修复 |
| `project/architecture/` | `area/architecture` | 青色 `#2CA6A4`，待配置 | 实际模块功能、类设计及模块内构建测试边界；不替代 spec 或阶段结果 |
| `project/benchmark/`，含 `performance/` 和 `testing/` | `area/benchmark` | 蓝色 `#377EB8` | Benchmark 实现、采样协议、共享场景与局部交付说明 |
| `testing/` | `area/testing` | 绿色 `#4DAF4A` | 通用玩法、回归和连续游玩验收 |
| `legacy/`，含 `architecture/` 和 `third-party-inventory.md` | `area/legacy` | 褐色 `#8C6D46` | 历史架构与依赖盘点，不能被主题标签改成当前实现 |
| `milestones/`，排除其中证据 | `area/milestones` | 红色 `#D95F5F` | 阶段报告、交接、失败和复测 |
| `Obsidian-功能笔记模板/` 的说明与 `examples/` | `area/templates` | 粉色 `#CC79A7` | 写作说明和示例，不是本项目的功能实现 |

颜色作为可复现的初始建议，最终以用户主题下的可辨识度为准；颜色不是唯一识别手段，仍须能查看 tag 和使用单板块筛选。

当前共 9 个板块，不再设独立的 `area/git` 或 `area/dependencies`。新增 `area/architecture` 仅归属已经落地的模块文档目录，不将旧 `legacy/architecture/` 移入该板块。`spec/git/` 与 `spec/md-organize/` 是目录层级上的 spec 子模块，继承 `area/spec`，不另设 `area/spec/git`、`area/spec/md-organize` 或第二个板块标签；子模块可用板块 tag 加路径条件筛选。本 spec 自身也归 `area/spec`，不再归 `area/meta`。

示例归属：

- `spec/M3-T3-渲染器重构.md`：`area/spec`，可选 `topic/rendering`，不能同时带 `area/legacy` 或实现板块 tag。
- `spec/git/gitspec.md` 与 `spec/md-organize/mdspec.md`：均为 `area/spec`，分别属于 Git 规范和文档整理子模块。
- `legacy/third-party-inventory.md`：`area/legacy`，不再设置独立依赖板块。
- `project/benchmark/performance/Performance-Session类.md`：`area/benchmark`，可选 `topic/performance`。
- `project/benchmark/testing/reproducible-scenes.md`：`area/benchmark`，可选 `topic/world`、`topic/gameplay`；共享用途不产生第二个 `area/testing`。
- `testing/gameplay-smoke.md`：`area/testing`，可选 `topic/gameplay`。
- `legacy/architecture/performance-observation.md`：`area/legacy`，可选 `topic/performance`。
- `milestones/m2-t3/README.md`：`area/milestones`，可选 `topic/benchmark`；而保存在 `project/benchmark/` 的硬件基线笔记仍归 `area/benchmark`。
- 同名 `README.md` 按所在目录归属，不统一标成 `area/meta`；避免依赖模糊的文件名规则。

### 纳管例外与数据保护

- 任意 `evidence/` 下的文件，包括 `probe-note.md`，均不加 frontmatter、不重写、不移动。它们按路径从默认图谱排除；证据仍可从报告链接打开。
- 可直接插入笔记的 `Obsidian-功能笔记模板/templates/` 文件不写固定 `area/templates`。插入模板会合并属性，固定归属可能污染新笔记。因此这些模板源文件保持原样，按路径排除；新笔记生成后再按最终存放目录补齐唯一板块 tag。
- 模板说明与示例属于纳管笔记，应带 `area/templates`，并在维护说明中强调不可将示例归属原样复制到生产笔记。
- 保留现有 frontmatter 属性、正文、链接、状态和验收数据；已有 `tags` 采用合并、去重，不新增第二个 `tags` 键。
- 无 frontmatter 的普通 Markdown 可新增只含 `tags` 的最小属性块。不得把正文中的分隔线误识别为属性块。
- 任何实际生效的正文标签也须纳入归属冲突检查，不能只检查 YAML。示例代码中的 tag 不作为真实归属。

属性格式示例，适用于新增笔记；实际逐文件结果见文末清单：

```yaml
---
tags:
  - area/benchmark
  - topic/performance
---
```

有现存属性时仅合并 `tags`，不得用上述示例覆盖原有 `type`、`status`、`module`、日期或其他字段。

### 图谱呈现约定

默认采用“笔记按板块着色”，不依赖标签节点撑开空间。即使关闭标签节点，笔记仍可按 tag 查询分组。附件关闭、未解析节点隐藏、孤立笔记显示，避免无链接文档被漏看；不为凑图谱结构增加无意义双链。

颜色分组应按当前 9 个 `area/*` 条件与归属表一一对应，例如 `tag:#area/benchmark`。已有配置仅含历史 8 组，本次 M3-T0 追加文档未修改图谱；`area/architecture` 颜色仍待配置和 UI 验证。不为 `topic/*` 或 spec 子模块另设覆盖板块颜色的分组，也不添加会与所有板块重叠的 `tag:#area` 颜色组。

| 视图 | 搜索条件 | 标签节点 | 用途 |
| --- | --- | --- | --- |
| 默认工程视图 | `tag:#area -tag:#area/meta -tag:#area/templates -path:"/evidence/" -path:"Obsidian-功能笔记模板/templates/"` | 关闭 | 展示工程板块，降低总索引和模板对图谱的干扰 |
| 全量文档视图 | `tag:#area -path:"/evidence/" -path:"Obsidian-功能笔记模板/templates/"` | 关闭 | 同时检查治理、模板说明和工程文档 |
| Benchmark 单板块视图 | `tag:#area/benchmark -path:"/evidence/"` | 默认关闭，可临时开启 | 单独查看 Benchmark 笔记及其联系 |
| Spec 全板块视图 | `tag:#area/spec -path:"/evidence/"` | 关闭 | 同时查看设计约定、Git 规范与文档整理规范 |
| Spec 的 Git 子模块 | `tag:#area/spec path:"spec/git/" -path:"/evidence/"` | 关闭 | 仅查看 Git 规范，仍使用 spec 板块颜色 |
| Spec 的文档整理子模块 | `tag:#area/spec path:"spec/md-organize/" -path:"/evidence/"` | 关闭 | 仅查看文档整理规范，仍使用 spec 板块颜色 |
| 跨板块性能视图 | `tag:#topic/performance -path:"/evidence/"` | 默认关闭，可临时开启 | 查看性能主题涉及的历史与实现资料，保持各自板块颜色 |

其他单板块视图替换 `area/benchmark` 为目标板块 tag。路径查询以 `docs/` 为 vault 根；搜索操作符依据 [Search](https://help.obsidian.md/plugins/search)。

标签节点开启只作为辅助探索模式：原生开关不能在此方案中被当作“只显示 area 标签、不显示 topic 标签”的选择器。共享主题节点可能重新连接多个板块，不能用其位置判断归属冲突。上述视图是查询配方，不声称原生图谱会自动保存为多个独立命名预设。

单板块视图的硬边界针对可见笔记集合，而非删除原链接。默认全局视图仍允许真实跨板块连线，布局形状和节点位置不作为固定验收结果。

### 实施范围与维护步骤

本节已获用户授权执行。当前完成范围以文末验收记录为准；后续维护按相同步骤核对，不将静态校验等同于 UI 验收。

1. 重新盘点实际文件和已有标签，按归属表生成逐文件清单与例外清单，覆盖本阶段新增的 Benchmark 基线笔记及用户追加的三处归属调整，不能沿用上一阶段的固定文件数量。若发现旧的 `area/git`、`area/dependencies`，或文档整理 spec 仍带 `area/meta`，应按新规则纠正，不保留为兼容归属标签。
2. 以 YAML 解析器检查属性并执行最小改动，补齐唯一 `area/*`；可选主题标签逐篇按需要添加，不机械填满配额。工具不可用时报告，不用粗糙替换破坏 frontmatter。
3. 更新 `DOCUMENTATION_MAP.md` 和模板使用说明中的标签维护规则，并核对本轮用户移动涉及的目录树与导航引用；正文事实、通用模板源文件、原始证据均不改。
4. 配置 `docs/.obsidian/graph.json` 的筛选、标签显示和颜色分组，保留无关布局参数。Obsidian 可能回写配置，需在应用不竞争写入时更新并重新加载核对；不覆盖 `workspace.json`、插件清单或其他用户设置。
5. 执行静态归属检查、YAML 校验、链接检查和证据哈希比较，再在实际 Obsidian 中检查全量、默认、单板块和跨主题视图。
6. 回写实施记录。没有实际 UI 验证时，只能记录“静态检查通过、图谱视觉验收待完成”，不能把配置文件写入成功当作图谱已清晰呈现。

### 第二阶段验收标准

- [x] 每篇纳管 Markdown 恰有一个合法 `area/*`，正文中没有额外冲突的归属标签。
- [x] 路径归属与标签一致；例外路径显式列明，新目录不会被默认兜底误分类。
- [x] 通用玩法测试与 Benchmark 测试边界清晰，历史架构、目标 spec 和实现笔记不会混成同一板块。
- [x] YAML 可解析，无重复 `tags` 键；原有属性、既有兼容标签和验收状态保持不变。
- [ ] 8 个颜色分组与板块词表一致；默认视图不受全局导航、模板或证据节点干扰，spec 及其两个规范子模块仍在默认视图内。
- [x] 依赖盘点归 `area/legacy`；Git 规范和文档整理归 `area/spec`，子模块路径规则与文件清单静态核对准确，不产生额外板块或多重归属。
- [ ] 在 Obsidian 中抽查每个板块的筛选集合，单板块视图没有其他板块笔记，且应纳入的孤立笔记仍可见。
- [ ] 全量视图覆盖所有纳管笔记；与文件清单对照，无未打标签笔记被正向标签筛选悄悄遗漏。
- [ ] 跨主题视图可以连接不同板块，但不改变笔记颜色归属；未为隔离图谱删除真实链接。
- [ ] 通用模板插入新笔记后不会自动携带错误板块；发布为正式笔记前须补齐其最终归属。
- [x] 证据哈希和路径、正文事实保持不变；仅修正错位导航，没有删除真实引用，未改动游戏实现或无关 Obsidian 设置。
- [x] 已记录实际 UI 检查结果或明确标注未完成；没有承诺固定几何分区或标签隔离引擎能力。

### 第二阶段实施与校验记录

- 日期：2026-10-05。用户授权实施，并在写入图谱设置前确认已退出 Obsidian；进程检查确认退出后才修改配置。
- 纳管 52 篇 Markdown，逐文件标签和例外清单见 [tag-inventory.json](tag-inventory.json)。8 个板块分别为：meta 2、spec 5、build 3、benchmark 17、testing 1、legacy 10、milestones 9、templates 5。
- 以 YAML 解析器校验全部 Markdown 的 frontmatter；每篇纳管笔记恰有一个正确板块，主题最多两个，无重复属性键或正文归属冲突。原有非 tags 属性语义不变，两个示例笔记的 `示例` 标签保留。
- 已修复用户移动后发现的 14 处错位链接，同步结构参考、总入口、Git 规范、依赖盘点和相关里程碑的导航。Markdown 链接与图片目标存在性检查通过；真实 Obsidian 双链目标唯一可解析，代码示例中的占位双链不作为真实导航。
- 已生成 8 个互斥板块颜色分组，设置默认工程查询、关闭标签节点、隐藏附件和未解析节点、显示孤立笔记。其余图谱布局参数保持原值，未修改 workspace、插件清单和其他用户设置。
- 静态集合核对：全量 52 篇，默认工程视图 45 篇，跨板块性能主题 10 篇；Git 和文档整理两个 spec 子模块各 1 篇。这是根据标签清单推导的预期集合，不是 Obsidian UI 实测数量。
- 对实施前快照的 490 个证据文件与六个可复用模板逐一比较 SHA256，路径和内容保持不变。证据目录内的 1 篇 Markdown 和六个模板源文件均未加标签。
- 已补充模板使用说明：插入模板后按最终路径赋予唯一板块；不直接复制示例的 `area/templates`。未在实际 Obsidian 中执行模板插入测试，因此对应 UI 验收仍未勾选。
- 未运行游戏构建、玩法、性能或硬件测试；保留原有验收状态与工作区修改，未提交 Git。临时校验依赖仅安装在 `out/docs-audit/`，未修改项目依赖清单。
- UI 限制：桌面工具两次报告 `Computer Use native pipe is unavailable`。未取得 Obsidian 实际图谱截图、搜索结果或模板插入结果，不把静态检查通过标成视觉验收通过。

### 待完成的图谱 UI 验收

重新打开本项目的 docs vault 后，核对既有八个颜色分组，并配置/验证新 `area/architecture` 对应的第九组；再按上表查询切换全量、每个单板块、两个 spec 子模块和性能主题视图，与最新清单中的预期集合比对。历史第二阶段的 52 篇计数只代表当时快照，不用于本次追加后的 UI 全量验收。检查孤立笔记、跨板块连线及颜色归属，并在临时笔记中验证模板插入不会继承错误板块。

本节完成后才勾选余下 UI 验收项；若 Obsidian 回写了配置，应先比较变化并确认原因，不覆盖用户新设置。标签节点是可选探索模式，不以强制几何隔离作为验收标准。

### M3-T0 模块文档追加记录

- 2026-10-05 按用户要求建立 `project/architecture/<模块>/` 的功能和类设计笔记，入口为 [模块架构索引](../../project/architecture/Overview.md)。此目录归属新的 `area/architecture`，旧架构和阶段报告不改归属。
- 本次使用已有 YAML 解析器更新新增架构笔记的 `tags`，比较更新前后其余属性语义并保留正文；没有对模板源文件、原始证据或 Obsidian 配置作批量写入。
- 本次快照为 79 篇纳管文档、默认工程视图预期 72 篇，其中 architecture 26 篇。最新逐文件结果见 [tag-inventory.json](tag-inventory.json)；历史第二阶段的 52 篇、8 组颜色、490 个证据比较等记录保持原义，不改写成此次新增结果。
- 全量纳管 frontmatter 可解析、唯一归属检查通过，新增架构文档的 Markdown 链接目标存在性检查通过。没有宣称重新验证所有历史源码链接或文档锚点。静态报告保存在 `out/m3-t0/docs-tag-validation.json`。
- `.obsidian/graph.json` 本次读取前后 SHA256 一致，没有写入第九个颜色组，也没有打开 Obsidian 做实际界面验收；待验收项仍保留。

### 数据与系统模板适配记录

2026-10-05 用户要求编写现有笔记迁移 spec，并抽取两个新模板的代表性示例。详细目标、文件对应关系、不变量归属、保护规则和待执行验收见 [数据与系统笔记迁移 Spec](data-system-migration-spec.md)。本轮仅交付 spec 与配套示例，正式 Scene / Simulation / ECS 笔记尚未改写、拆分或重命名；执行迁移需用户另行确认，不以计划文件存在推定迁移完成。

2026-10-06 同一 spec 先追加“逐对象审核覆盖”：已确定对象须独立笔记或有理由的辅助类型小节，既有汇总不能替代单对象契约，未实现候选须有批准依据并保持草稿。当时仅更新 spec；随后用户明确授权实施。

2026-10-06 已按 spec 迁移 Scene/Simulation，拆出 Simulation 系统和 ECS 存储数据，新增 15 个单主体笔记；复用 Camera/Window/Generator/Registry。模块汇总改为协作/数据入口，当前源码中需要显式释放、调用顺序与 T4 未解决问题均可审核。新增 [对象覆盖清单](../../project/architecture/对象笔记覆盖清单.md)，模块入口直接链接资源对象。

正式笔记唯一 `area/architecture`，没有新增每类标签或图谱颜色；标签清单仅同步本次迁移/新增架构项，不重分类用户笔记。新增笔记草稿/源码核对，不冒充运行测试通过。具体静态结果与待 UI 验收见 [实施记录](data-system-migration-spec.md#2026-10-06-正式迁移与覆盖实施)。历史 26 篇和先前检查数字保留为旧快照。

### Architecture 模板与函数行为覆盖规格

2026-10-06 用户要求归档聊天中的简单 spec，见 [Architecture 模板与函数行为覆盖 Spec](architecture-template-coverage-spec.md)。计划新增 namespace API、构建与依赖契约两个模板，并在各 architecture 模板中分别明确公开接口与私有函数的预期行为表；先整理 Renderer、GpuTimer、Simulation、构建契约四篇样板，确认后铺开。本轮只归档规格并登记入口，尚未修改模板或架构正文，不将归档视为实施授权或验收通过。

同日用户随后授权实施，已新增两份模板、补齐六份既有模板及选择说明，四篇样板分别建立公开/内部函数行为表，旧链接、约束编号和验收范围保留。新增模板源只登记为可复用模板例外，不预填板块或改图谱设置；当时其余笔记待确认后铺开。历史范围见 [样板实施记录](architecture-template-coverage-spec.md#2026-10-06-模板与四篇样板实施)，不改写前序迁移的历史验收。

用户再授权“继续进行spec的工作吧”，现已开始铺开，累计 28/43 篇适用设计/功能笔记具备两类行为表。新增 Platform / Telemetry namespace API 笔记分离自由函数与内部桥接，纳管 105、默认工程视图 95、architecture 46；用户 Renderer/GpuTimer 改名登记同步，图谱配置及旧验证快照不改。剩余 foundation 两篇、world 五篇、ecs 六篇、app 两篇与全量验收尚未完成；下一步队列与静态范围见 [铺开记录](architecture-template-coverage-spec.md#2026-10-06-行为表铺开记录)，无需再次确认样板，不把静态检查当成运行/UI 验收。
