# 历史架构参考

本目录保存 M0 至 M2-T3 的架构资料。Legacy 表示历史定位，不表示内容作废或相关机制全部被替换；文中的“当前”以各篇对应阶段为准。源码链接只用于定位，不固定历史版本与行号。

| 文档 | 对应阶段 | 内容 |
| --- | --- | --- |
| [M0 架构与数据流](current-data-flow.md) | M0 | 静态数据流、模块职责和风险基线 |
| [资源路径模块](asset-paths.md) | M1，含 M2-A 更新 | 资源定位与接口；保留 M1 失败和复测记录 |
| [应用生命周期](application-lifecycle.md) | M2-A，含 M2-T2 / M2-T3 增补 | 初始化、运行、退出及验证接口 |
| [网格与批次内存安全](mesh-safety.md) | M2-A | 所有权、容量与边界处理 |
| [玩家更新与交互循环](player-loop.md) | M2-A | 输入、物理、相机与编辑顺序 |
| [运行期图形资源](runtime-resources.md) | M2-A | OpenGL 资源生命周期和失败处理 |
| [可复现世界生成](world-generation.md) | M2-T2 | seed、写入顺序、摘要与确定性边界 |
| [性能观测与采样数据流](performance-observation.md) | M2-T3 旧脚本链路 | CPU/GPU 计时、上传、内存与指标限制 |

后续设计约定见 [项目范围](../../spec/project-scope.md)；原生采样设计见 [Performance Session](../../project/benchmark/performance/Performance-Session类.md) 与 [失焦采样](../../project/benchmark/performance/Performance-失焦采样.md)。这些入口不代表 M3 已实施或任何硬件验收已通过。

新模块说明按 [文档结构参考](../../DOCUMENTATION_MAP.md) 放入对应工程主题目录。返回 [技术文档索引](../../README.md)。
