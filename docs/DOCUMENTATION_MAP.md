---
tags:
  - area/meta
---

# 文档结构与维护参考

更新日期：2026-10-05。面向后续维护者与 agent，说明文档归属、Obsidian 标签、阅读入口和变更边界。本文是目录维护参考，不替代项目 spec、实现说明或验收报告。

## 先读什么

1. 从 [技术文档索引](README.md) 选择任务相关入口。
2. 查阅 [项目范围与验收约定](spec/project-scope.md) 及对应任务 spec，确认目标和非目标。
3. 修改已有模块前阅读 `project/` 中的相关笔记，并核对源码；遇到旧架构问题时参考 `legacy/architecture/`。
4. 判断“是否验证通过”时阅读对应报告与证据，不能从 spec、测试用例或目录位置推断完成状态。

## 目录参考

```text
docs/
├── README.md                          # 面向读者的分类导航
├── DOCUMENTATION_MAP.md               # 面向维护者与 agent 的归属规则
├── spec/                             # 项目范围、目标设计与规范子模块
│   ├── project-scope.md
│   ├── M3-T0-模块软硬边界.md
│   ├── M3-T2-渲染器重构.md
│   ├── git/
│   │   └── gitspec.md                # 版本控制规范子模块
│   └── md-organize/
│       ├── mdspec.md                 # 文档结构与图谱规范
│       └── tag-inventory.json        # 逐文件标签与例外清单
├── project/                          # 按工程主题维护的实现与操作说明
│   ├── architecture/                 # M3 起实际模块的功能、类设计与边界
│   │   └── Overview.md               # 九个生产模块、应用和构建测试入口
│   ├── build/                        # 构建、CLion、CMake 与构建问题修复
│   │   └── build-and-clion.md         # M1 指南，适用范围见文首
│   └── benchmark/                    # 原生 Benchmark 功能、类、协议与交付
│       ├── performance/              # 采样策略与 Performance 类设计
│       ├── testing/                  # 采样协议、固定场景及适用版本
│       │   ├── README.md
│       │   ├── performance-baseline.md
│       │   └── reproducible-scenes.md
│       └── evidence/                 # Benchmark 局部交付证据，位置不变
├── testing/                          # 通用玩法与回归验收，不从属于 Benchmark
│   └── gameplay-smoke.md
├── legacy/
│   ├── architecture/                 # M0 至 M2-T3 的历史架构参考
│   │   └── README.md                 # 各篇文档对应阶段
│   └── third-party-inventory.md      # 历史依赖盘点
├── milestones/                       # 按阶段记录的结果、失败、复测与交接
│   ├── m0/
│   ├── m1/
│   ├── m2-a/
│   ├── m2-t2/
│   └── m2-t3/                        # 各阶段 evidence 随原报告保留
└── Obsidian-功能笔记模板/              # 写作规范、模板与示例
```

此树展示主要目录职责，不枚举全部类笔记和证据文件。磁盘上的整理 spec 文件名为 `mdspec.md`；引用遵循实际大小写，便于跨平台使用。

## 新文档放哪里

| 内容 | 归属 | 注意事项 |
| --- | --- | --- |
| 项目范围、未实施方案、任务契约 | `spec/` | 目标设计不等于已实现；明确状态与非目标 |
| 模块实现、类设计、操作指南、局部修复 | `project/<主题>/` | 优先复用已有主题目录，标明适用版本与源码依据 |
| M3 起生产模块的功能与类设计 | `project/architecture/<模块>/` | 每模块配套功能文档与类设计；统一入口见 [模块架构索引](project/architecture/Overview.md) |
| 构建、CLion、CMake 问题 | `project/build/` | 阶段构建结果仍由报告承载 |
| Benchmark 功能、进程、文件协议、交付 | `project/benchmark/` | 保持功能、类、协议与实验报告的职责区别 |
| 性能观测与采样类设计 | `project/benchmark/performance/` | 不把旧计时说明直接当成最新契约 |
| 性能采样协议、Benchmark 共享固定场景 | `project/benchmark/testing/` | 共享场景需链接通用玩法验收，旧脚本流程注明版本 |
| 通用玩法、启动失败、连续游玩与回归验收 | `testing/` | 不因与性能场景共享夹具就放入 Benchmark |
| 阶段验证结果、失败和复测、包交接 | `milestones/<阶段>/` | 保留时间、版本、环境、结果与结论边界 |
| 旧架构资料 | `legacy/architecture/` | 保留原阶段事实；新模块说明不继续堆入此处 |
| 历史第三方依赖盘点 | `legacy/third-party-inventory.md` | 属于 legacy，不独立建立依赖板块 |
| 版本控制规范 | `spec/git/` | spec 的规范子模块 |
| 文档结构、图谱约定和迁移记录 | `spec/md-organize/` | spec 的规范子模块，不作为工程模块设计目录 |

不要为尚未实施的模块预建空目录，也不要建立第二份平行的“当前架构”真相。后续模块落地时，其维护说明进入对应 `project/<主题>/`，历史资料只增加必要的后续入口。

## Obsidian 板块标签

以 `docs/` 为 vault 根目录，每篇纳管笔记的 YAML `tags` 列表恰好包含一个 `area/*`。下面的归属与目录一致；目录调整时同时更新标签、导航和 [逐文件清单](spec/md-organize/tag-inventory.json)。不能为了改变图谱颜色给笔记添加第二个板块。

| 板块标签 | 路径 | 图谱颜色 |
| --- | --- | --- |
| `area/meta` | 根 `README.md`、`DOCUMENTATION_MAP.md` | 灰色 |
| `area/spec` | `spec/`，含 `git/`、`md-organize/` | 紫色 |
| `area/build` | `project/build/` | 橙色 |
| `area/architecture` | `project/architecture/`，含模块索引与构建测试边界 | 青色建议；尚未写入或验证图谱设置 |
| `area/benchmark` | `project/benchmark/`，含 `performance/`、`testing/` | 蓝色 |
| `area/testing` | `testing/` | 绿色 |
| `area/legacy` | `legacy/`，含依赖盘点 | 褐色 |
| `area/milestones` | `milestones/`，不含证据 | 红色 |
| `area/templates` | 模板说明与 `examples/`，不含模板源文件 | 粉色 |

可选 `topic/*` 只表达关联主题，默认不添加、最多两个；不改变板块归属。词表为 `build`、`benchmark`、`performance`、`world`、`rendering`、`gameplay`、`resources`。已有 `type`、`status`、`module` 属性照常维护，不再复制成标签。两个模板示例原有的 `示例` 标签保留为兼容标签。

任何 `evidence/` 文件和 `Obsidian-功能笔记模板/templates/` 下的模板源文件均不加标签。前者是不可改写的证据；后者不能在插入新笔记时带入错误板块。正式笔记生成后按最终目录补齐归属，不直接复制示例的 `area/templates`。功能、类、数据与系统模板的选择见 [模板使用说明](Obsidian-功能笔记模板/00-使用说明.md#选择模板)；模板类型不增加板块归属。

默认图谱按板块着色，关闭标签节点，保留真实跨板块链接，隐藏附件与未解析节点，显示孤立笔记。硬边界指唯一归属与筛选集合，不保证集群固定位置或完全分离。

常用图谱搜索条件如下。它们是切换视图时使用的查询，不是自动生成的独立命名预设。

| 视图 | 查询 |
| --- | --- |
| 默认工程视图 | `tag:#area -tag:#area/meta -tag:#area/templates -path:"/evidence/" -path:"Obsidian-功能笔记模板/templates/"` |
| 全量文档视图 | `tag:#area -path:"/evidence/" -path:"Obsidian-功能笔记模板/templates/"` |
| 单板块，例如 Benchmark | `tag:#area/benchmark -path:"/evidence/"` |
| Git 规范子模块 | `tag:#area/spec path:"spec/git/" -path:"/evidence/"` |
| 文档整理子模块 | `tag:#area/spec path:"spec/md-organize/" -path:"/evidence/"` |
| 跨板块性能主题 | `tag:#topic/performance -path:"/evidence/"` |

当前归属词表扩展为 9 个板块，新增 `area/architecture` 对应用户要求的实际模块文档目录。图谱既有 8 组设置和历史验证记录不改写，本次没有修改 `.obsidian/graph.json`；第九组颜色仍待用户在 Obsidian 中配置与验证。颜色分组只按板块 tag，不增加重叠的主题颜色组。全量笔记清单需要与实际文件数对照，避免无标签的新笔记被正向筛选隐藏。完整规范和验收边界见 [mdspec](spec/md-organize/mdspec.md)。

## 如何判断时效

- `legacy/` 是历史参考，不是作废标记，也不是所有机制均被替换的证明。文中的“当前”以其阶段为准。
- `project/` 表示按工程主题归类，不保证每篇文档适用于最新版本。尤其是 M1 构建指南与 M2-T3 旧脚本采样指南，先读文首范围提示。
- `spec/` 记录约定；是否已实施、已验证，仍须核对实现与对应证据。
- 测试用例、自动测试、真实驱动运行、人工游玩、稳定性、性能与指定硬件验收分别记录，不能相互替代。

## 引用与维护规则

- 总入口负责导航，本文负责归属规则，任务 spec 负责设计约定，报告负责验证结果；避免在多个入口重复维护测试数量与进度。
- 共享入口与跨目录导航优先使用相对 Markdown 链接，兼容普通 Markdown 阅读器；已有 Obsidian 双链可保留，目标名必须唯一可解析。
- 移动文档时同时检查入链、出链、源码链接、图片链接及锚点；只调整导航，不批量替换历史命令中的路径。
- `evidence/` 内日志、截图、哈希与结果文件保持原位置和原内容，不为目录美观重排证据。
- 可执行文件、DLL、对象文件、调试符号、完整运行包与逐帧 CSV 留在 `out/`，不放入文档目录。
- 保留用户已有修改；不要借文档整理改动程序实现、验收状态、历史测试数量或模板示例。
- 新建或实质更新笔记使用 [Obsidian 模板说明](Obsidian-功能笔记模板/00-使用说明.md)，版本控制遵循 [Git 规范](spec/git/gitspec.md)。

## 待独立核对的内容

- [M1 构建指南](project/build/build-and-clion.md) 的命令与布局未在本轮重新执行，后续更新应对照当前构建配置与 [模块边界说明](project/build/CMake-模块边界.md)。
- [M2-T3 采样指南](project/benchmark/testing/performance-baseline.md) 保留旧脚本与候选包流程，不能与 [原生 Benchmark 交付说明](project/benchmark/使用与交付.md) 或新的失焦策略混用。
- 历史源码链接只提供定位，不固定旧版本；文件存在不表示其中的函数、行号与历史描述仍一致。
- 模板中的示例占位双链和外部 Obsidian 本机路径不属于项目导航；不为使链接检查全绿而伪造文件。

本轮迁移清单与验收记录见 [文档整理 Spec](spec/md-organize/mdspec.md)。
