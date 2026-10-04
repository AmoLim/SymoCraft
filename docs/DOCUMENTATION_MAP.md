# 文档结构与维护参考

更新日期：2026-10-04。面向后续维护者与 agent，说明文档归属、阅读入口和变更边界。本文是目录维护参考，不替代项目 spec、实现说明或验收报告。

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
├── spec/                             # 项目范围、目标设计与任务约定
│   ├── project-scope.md
│   ├── M3-T0-模块软硬边界.md
│   └── M3-T2-渲染器重构.md
├── project/                          # 按工程主题维护的实现与操作说明
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
│   └── architecture/                 # M0 至 M2-T3 的历史架构参考
│       └── README.md                 # 各篇文档对应阶段
├── milestones/                       # 按阶段记录的结果、失败、复测与交接
│   ├── m0/
│   ├── m1/
│   ├── m2-a/
│   ├── m2-t2/
│   └── m2-t3/                        # 各阶段 evidence 随原报告保留
├── git/
│   └── gitspec.md                    # 版本控制规范
├── third-party-inventory.md           # 第三方依赖盘点与历史边界
├── Obsidian-功能笔记模板/              # 写作规范、模板与示例
└── md-organize/
    └── mdspec.md                      # 本轮整理 spec 与迁移记录
```

此树展示主要目录职责，不枚举全部类笔记和证据文件。磁盘上的整理 spec 文件名为 `mdspec.md`；引用遵循实际大小写，便于跨平台使用。

## 新文档放哪里

| 内容 | 归属 | 注意事项 |
| --- | --- | --- |
| 项目范围、未实施方案、任务契约 | `spec/` | 目标设计不等于已实现；明确状态与非目标 |
| 模块实现、类设计、操作指南、局部修复 | `project/<主题>/` | 优先复用已有主题目录，标明适用版本与源码依据 |
| 构建、CLion、CMake 问题 | `project/build/` | 阶段构建结果仍由报告承载 |
| Benchmark 功能、进程、文件协议、交付 | `project/benchmark/` | 保持功能、类、协议与实验报告的职责区别 |
| 性能观测与采样类设计 | `project/benchmark/performance/` | 不把旧计时说明直接当成最新契约 |
| 性能采样协议、Benchmark 共享固定场景 | `project/benchmark/testing/` | 共享场景需链接通用玩法验收，旧脚本流程注明版本 |
| 通用玩法、启动失败、连续游玩与回归验收 | `testing/` | 不因与性能场景共享夹具就放入 Benchmark |
| 阶段验证结果、失败和复测、包交接 | `milestones/<阶段>/` | 保留时间、版本、环境、结果与结论边界 |
| 旧架构资料 | `legacy/architecture/` | 保留原阶段事实；新模块说明不继续堆入此处 |
| 本轮文档整理约定和迁移记录 | `md-organize/` | 不作为长期模块设计目录 |

不要为尚未实施的模块预建空目录，也不要建立第二份平行的“当前架构”真相。后续模块落地时，其维护说明进入对应 `project/<主题>/`，历史资料只增加必要的后续入口。

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
- 新建或实质更新笔记使用 [Obsidian 模板说明](Obsidian-功能笔记模板/00-使用说明.md)，版本控制遵循 [Git 规范](git/gitspec.md)。

## 待独立核对的内容

- [M1 构建指南](project/build/build-and-clion.md) 的命令与布局未在本轮重新执行，后续更新应对照当前构建配置与 [模块边界说明](project/build/CMake-模块边界.md)。
- [M2-T3 采样指南](project/benchmark/testing/performance-baseline.md) 保留旧脚本与候选包流程，不能与 [原生 Benchmark 交付说明](project/benchmark/使用与交付.md) 或新的失焦策略混用。
- 历史源码链接只提供定位，不固定旧版本；文件存在不表示其中的函数、行号与历史描述仍一致。
- 模板中的示例占位双链和外部 Obsidian 本机路径不属于项目导航；不为使链接检查全绿而伪造文件。

本轮迁移清单与验收记录见 [文档整理 Spec](md-organize/mdspec.md)。
