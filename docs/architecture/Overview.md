---
tags:
  - area/architecture
---

# 当前架构入口

本入口按模块机制导航，不要求逐类/逐函数笔记。现行维护规则见 [文档治理](../DOCUMENTATION_MAP.md)，公开行为见 [现役规格](../openspec/specs/)，T5 交付范围见 [T5 归档](../openspec/changes/archive/2026-10-25-m3-t5-ecs-memory/proposal.md)，历史设计见 [T3](../openspec/changes/archive/2026-10-19-m3-t3-renderer/proposal.md) / [T4 归档](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/proposal.md)，批准与证据见 [验证索引](../verification/VERIFICATION.md)。

## 模块导航

| 内容 | 当前维护位置 |
| --- | --- |
| 基础值、文件发布与旧分配/日志边界 | [foundation.md](foundation.md) |
| CPU 场景值、布局与短借用 | [scene.md](scene.md) |
| 资源路径、图片与非事务 PNG 输出 | [assets.md](assets.md) |
| 有限世界、生成、受控编辑与网格发布 | [world.md](world.md) |
| 已同步主工作区的 T5 EnTT 门面、身份与查询 | [ecs.md](ecs.md) |
| 独立Session预算、自有结果、World适配与物理核心 | [simulation.md](simulation.md) |
| SDL 单窗口、输入事实与三种私有图形交接 | [platform.md](platform.md) |
| 受控观测、GPU关联、Seal与显式幂等导出 | [telemetry.md](telemetry.md) |
| 私有组合根、帧策略与现代发布缓存 | [app.md](app.md) |
| Renderer：现代门面、私有后端与旧 GL 过渡 | [renderer.md](renderer.md) |
| Benchmark：进程控制、文件协议与采样边界 | [benchmark.md](benchmark.md) |
| 工程级构建、测试隔离与资源部署 | [engineering.md](engineering.md) |

本页为机制导航，不建逐类学习目录。公共行为引用各 current specs，目标留各 change；新稿只作静态源码核对，不是新运行成绩。旧笔记暂保留复核，按迁移 design 的归属处置，不继续维护第二套当前架构。

## 跨模块不变量

- CPU 世界/模拟与中立场景数据不依赖窗口或图形 SDK。依赖边界由真实 CMake target 表达，不靠目录名称保证。
- World 网格 callback 数据仅在同步访问期借用；app 在有效期内复制，以 World/Chunk/发布 revision 管理现代持久资源，成功接受更新后才推进版本。
- 图形资源与 Renderer 先于其借用的 Window/上下文清理。GPU 未完成不能把逻辑销毁当作物理资源可回收。
- 原生窗口/图形桥接和后端类型留在受限实现侧，不穿透现代公开三头。
- telemetry 的异步 GPU 样本携带来源帧身份；CPU 提交成本不能冒充 GPU 执行时间，现代输出不能假定由旧 Benchmark 协议正确消费。

这些事实不代替运行成绩。T3/R4 已于 2026-10-17 获用户批准，T4 后续也获批准并将源码同步 F，无 Git 历史合并；实际计算服务未实施。2026-10-25 T5 与 M3 获整体批准、规格同步并归档后，用户独立授权将 E 的 EnTT 门面、调用迁移和必要配套同步 F，现已完成源码传播；本轮只重跑受影响非游戏检查，不继承 E 的真实后端成绩为 F 新运行。当前交付、批准和传播结果见 [T5 verification](../openspec/changes/archive/2026-10-25-m3-t5-ecs-memory/verification.md)，上游与节点状态从 [T4 verification](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/verification.md) / [批准索引](../verification/VERIFICATION.md#关键节点)判读。
