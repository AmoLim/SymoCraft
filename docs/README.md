# Symocraft 技术文档

本文档集同时服务工程维护和源码学习。按下面的用途入口阅读；新建、移动或维护文档前，先看 [文档结构与维护参考](DOCUMENTATION_MAP.md)。

本文是导航，不重复维护阶段测试数量。设计约定、实现说明和验证报告分别阅读；文档整理不改变 M2、人工连续游玩、性能或指定笔记本的验收状态。桌面结果不能替代指定 Y9000P 的实测。

## 项目约定

- [项目范围与验收约定](spec/project-scope.md)：目标、阶段边界、参考硬件和交付标准。
- [M3-T0 模块软硬边界](spec/M3-T0-模块软硬边界.md)：模块边界设计约定，不代表实现已完成。
- [M3-T2 渲染器重构](spec/M3-T2-渲染器重构.md)：后续渲染设计与验收要求，不代表多后端已通过验证。

## 构建与开发

- [构建与 CLion 指南](project/build/build-and-clion.md)：M1 构建与开发资源部署指南，适用范围见文首。
- [CMake 模块边界](project/build/CMake-模块边界.md)：后续游戏与工具构建目标的边界说明。
- [自定义构建目录日志修复](project/build/CMake-自定义构建目录日志修复.md)：问题、修复与验证边界。
- [第三方依赖盘点](third-party-inventory.md)：依赖关系、历史二进制检查与待补充材料。

## Benchmark

- [使用与交付](project/benchmark/使用与交付.md)：原生执行器操作与交付说明；开发脚本不进入便携包。
- [执行与导出](project/benchmark/Benchmark-执行与导出.md)、[文件协议](project/benchmark/Benchmark-文件协议.md)：执行流程、输入输出与有效性约定。
- [App 类](project/benchmark/Benchmark-App类.md)、[Process 类](project/benchmark/Benchmark-Process类.md)：应用与子进程职责。
- [失焦采样](project/benchmark/performance/Performance-失焦采样.md)、[Session 类](project/benchmark/performance/Performance-Session类.md)：采样策略、状态与数据所有权。
- [测试与共享场景索引](project/benchmark/testing/README.md)：区分固定场景、旧脚本协议与原生执行器。
- [桌面交付验证](project/benchmark/Benchmark-桌面交付验证.md)：局部交付结果、失败与复测及二进制身份边界，不替代正式基线或全硬件验收。
- [GTX 1650 低端机基线](project/benchmark/Benchmark-GTX1650低端机基线.md)：指定低端整机的修复版九轮归档、窗口复测、长帧与失焦边界；指定中端 Y9000P 及分档目标以项目约定为准。

## 通用验收

- [M2 玩法冒烟验收](testing/gameplay-smoke.md)：玩法、失败路径、人工连续游玩和后续稳定性用例；用例列表不是通过记录。
- [固定世界与边界场景](project/benchmark/testing/reproducible-scenes.md)：通用回归与 Benchmark 共享的 M2-T2 场景夹具。

## 历史参考与阶段报告

- [历史架构索引](legacy/architecture/README.md)：M0 至 M2-T3 的架构资料；不当作最新实现或未来目标架构。
- [M0 环境与原始构建报告](milestones/m0/README.md)：原始基线、环境和静态风险。
- [M1 可靠构建报告](milestones/m1/README.md)：构建、资源部署、失败与复测。
- [M2-A 桌面阶段报告](milestones/m2-a/README.md)：桌面局部实现与集成证据及整体验收边界。
- [M2-T2 验证报告](milestones/m2-t2/README.md)：可复现世界、固定场景与有限帧运行证据。
- [M2-T3 桌面基线报告](milestones/m2-t3/README.md)：旧脚本九轮采样、成本分析及硬件待验收项。
- [Y9000P 首轮诊断与脚本补丁](milestones/m2-t3/laptop-first-run.md)：历史失焦记录、兼容性修复与重试方法。

## 文档规范

- [文档结构与维护参考](DOCUMENTATION_MAP.md)：面向维护者与 agent 的目录归属和时效判断规则。
- [文档整理 Spec](md-organize/mdspec.md)：本轮迁移清单、保护规则与验收记录。
- [功能笔记模板说明](Obsidian-功能笔记模板/00-使用说明.md)、[类设计模板说明](Obsidian-功能笔记模板/01-类设计模板-使用说明.md)、[示例与演进维护](Obsidian-功能笔记模板/02-示例与演进维护.md)：笔记写法与维护约定。
- [Git 分步版本控制规范](git/gitspec.md)：提交边界、验证要求与执行记录。

## 证据规则

版本控制与提交拆分遵循 [Git 分步版本控制规范](git/gitspec.md)，包括提交边界、验证要求、忽略规则维护和执行记录。

- 构建成功、启动成功、玩法通过、稳定性通过、性能达标分别记录，不互相替代。
- 静态源码事实、实测结果、待验证风险、未来设计明确区分。
- 每个节点保留失败记录、修复后的复测记录和已知限制。
- 文档中的本机绝对工具路径只用于复现本次实验，不是后续工程配置的硬编码要求。
- M0 架构及依赖表保留历史基线；M1 新文档和依赖变更说明反映后续修改，不把历史观测重写成当前结果。
- M2-A 桌面验证、指定笔记本验证、人工连续游玩和性能采样分别留证；实现文档和测试用例不等同于验收通过记录。
- `docs` 保留技术说明、报告、关键日志、小型结果摘要和少量验收截图；exe、DLL、obj、lib、pdb、运行包及完整逐帧 CSV 留在 `out`，不复制进文档目录。

## 后续模块文档标准

新建或实质改造的模块至少说明：职责与非职责、公开接口、输入输出、数据流、资源所有权、生命周期、错误处理、关键算法、方案取舍、测试入口和已知限制。

涉及并发的模块额外说明：任务输入归谁所有、哪些数据不可变、结果怎样交接、如何处理过期结果、取消与退出顺序。涉及性能的模块额外说明：如何测量、测试场景、原始数据、对照结果和结论边界。

学习说明默认读者已掌握现代 C++ 基础；重点解释并发、图形资源和性能分析，而不是重复基础语法教程。
