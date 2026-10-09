---
tags:
  - area/benchmark
---

# Benchmark 测试与共享场景

本目录按测试用途收纳协议和场景，并不保证其中每篇文档适用于最新版本。

当前设备分级以 [项目范围与验收约定](../../../spec/project-scope.md) 为准：i7-10750H / GTX 1650 是指定低端性能基准机，Y9000P / RTX 3070 Ti Laptop 是指定中端性能基准机，开发台式机用于开发回归对照。低/中端不能相互替代；旧“双机”指南保留其历史范围，不代表当前只有一档笔记本性能参考。

| 文档 | 适用阶段 | 用途与边界 |
| --- | --- | --- |
| [固定世界与边界场景](reproducible-scenes.md) | M2-T2 | 可定位、可重建的夹具，Benchmark 和通用玩法回归共同复用 |
| [双机性能采样](performance-baseline.md) | M2-T3 旧脚本流程 | 正式九轮协议、历史候选包、笔记本交接和数据保存要求 |

原生 Benchmark 的操作入口为 [使用与交付](../使用与交付.md)，执行与结果契约见 [执行与导出](../Benchmark-执行与导出.md)、[文件协议](../Benchmark-文件协议.md) 和 [失焦采样](../performance/Performance-失焦采样.md)。旧指南的禁止切出要求不覆盖原生执行器的新策略，旧候选包与新协议的结果不能混用。

通用 [玩法冒烟与连续游玩验收](../../../testing/gameplay-smoke.md) 保留在 `docs/testing/`，不归 Benchmark 所有。协议和场景存在不代表验收通过；实际结果分别见 [M2-T2 报告](../../../milestones/m2-t2/README.md)、[M2-T3 报告](../../../milestones/m2-t3/README.md) 和 [原生执行器桌面验证](../exp/Benchmark-桌面交付验证.md)。

指定低端、Y9000P 中端与当前开发机均已有有效九轮，现按 [基准冻结 20261005](../exp/Benchmark-基准冻结-20261005.md) 确定当前基准。失焦仅作分析条件，全部样本保留，不以前台重测为成立前提；不冒称受控前台成绩或玩法验收。报告分别见 [GTX 1650](../exp/Benchmark-GTX1650低端机基线.md)、[Y9000P](../exp/Benchmark-Y9000P中端机基线.md)、[本机](../exp/Benchmark-本机基线-20261005.md)。

返回 [技术文档索引](../../../README.md)；维护规则见 [文档结构参考](../../../DOCUMENTATION_MAP.md)。
