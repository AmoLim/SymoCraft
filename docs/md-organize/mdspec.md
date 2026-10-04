# docs Markdown 文档整理 Spec

## 状态与目标

- 确认日期：2026-10-04。
- 状态：已于 2026-10-04 按本 spec 完成目录迁移、导航与引用整理。校验范围见文末实施记录。
- 范围：整理仓库 `docs/` 内 Markdown 文档的归属、导航和引用，不修改程序实现，不改写历史验证结论。
- 目标：明确区分历史架构参考、维护文档、未来设计和验证记录，让读者能够按用途找到文档，并识别其适用阶段。

## 目标目录

以下目录均相对仓库根目录；树中省略未发生归属变化的具体文件。

```text
docs/
├── README.md                    # 分类导航
├── DOCUMENTATION_MAP.md         # 面向维护者与 agent 的文档归属参考
├── md-organize/
│   └── mdspec.md                 # 本次整理约定，沿用磁盘实际文件名
├── spec/                        # 项目范围、设计约定、待实施方案
├── legacy/
│   └── architecture/            # 旧架构参考
├── project/
│   ├── build/                   # 构建、CLion、CMake 边界与问题修复
│   └── benchmark/
│       ├── testing/             # 性能采样协议、固定测试场景
│       ├── performance/         # 性能观测相关设计
│       └── evidence/            # 保持现有证据位置
├── testing/                     # 通用玩法与回归验收
├── milestones/                  # 阶段报告及其原始证据
├── git/
├── third-party-inventory.md
└── Obsidian-功能笔记模板/
```

Benchmark 是测试的一部分，不是所有测试的上级。通用玩法、启动失败和连续游玩验收不归入 Benchmark。

## 迁移清单

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

迁移完成后，不保留空的 `docs/architecture/` 和 `docs/development/` 目录。`spec/`、`milestones/`、Git 规范、第三方依赖盘点和 Obsidian 模板保持现有归属，本轮仅按需修正导航与引用。

## 内容与导航规则

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
- 已新增 [文档结构与维护参考](../DOCUMENTATION_MAP.md)，作为后续维护者与 agent 的归属入口；历史架构与 Benchmark 测试各补充一份局部索引。
- 总入口已按用途重组；已修复整理前发现的 5 处错位链接，并同步根 README、里程碑、spec 和相关模块说明中的迁移引用。
- 本地 Markdown 链接及图片目标存在性检查通过，文档锚点检查通过；源码行号仅保留历史定位，不声称已核对其语义。
- 39 处 Obsidian 双链中，38 处目标唯一可解析；剩余 1 处为原模板中的“基类名-类设计”占位示例，保持原样。另有 2 处模板引用的外部 Obsidian 本机路径，不属于仓库内导航，保持原样。
- 整理前后的 240 个证据文件逐一比较 SHA256，路径和内容均未改变；全部 241 个非 Markdown 文档树文件亦未改变。
- 原有文档的命令代码块保持不变；除本 spec 的整理验收项外，原有验收勾选状态保持不变。
- Git 差异空白检查通过。未运行程序构建、玩法、性能或硬件测试；本轮仅进行文档结构与引用校验。

模板示例外部路径、旧指南命令适用性和历史源码定位限制，统一见结构参考中的待独立核对项。本次未提交 Git，保留工作区中已有修改与暂存状态。
