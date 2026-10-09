---
type: 类设计
status: 已验证
project: SymoCraft
module: performance
class_name: SymoCraft::Performance::Session
inheritance: []
created: 2026-10-04
tags:
  - area/benchmark
  - topic/performance
---

# Performance Session 类设计

关联功能：[[Performance-失焦采样]]、[[Benchmark-文件协议]]。既有测量范围详见 [性能观测数据流](../../../legacy/architecture/performance-observation.md)。

## 当前设计

职责：本次游戏运行的 CPU/内存观测与已回收 GPU 时间的内存集合、无效原因、阶段状态和退出导出。不拥有 GLFW/GL 资源，不启动进程，不分析多机结果。

### 数据成员

| 类型 | 成员 | 初值 / 范围 | 含义与所有权 |
| --- | --- | --- | --- |
| `Clock::time_point` | `entry` | main 入口 | 启动计时基点 |
| `YAML::Node` | `metadata / startup` | 构造基础项后由 Application 补充 | 自有结构化观测；actual GL 来自当前上下文 |
| `bool` | `completed` | false | 仅达到协议时长的循环置 true，不等于有效 |
| `filesystem::path` | `directory_` | 新目录 | 自有路径值；UTF-8 参数转换为 Windows 原生路径 |
| `vector<Frame>` | `frames_` | reserve 100000，最多 1000000 | 自有全部帧；保留 warmup、慢帧和缺失 GPU 值 |
| `vector<Memory>` | `memory_` | reserve 1300 | 自有约每秒内存采样 |
| `vector<string>` | `invalid_reasons_` | 空 | 去重，不删除已经出现的无效原因 |
| `bool` | `allow_unfocused_` | 构造参数固化 | 控制失焦是否构成无效原因，不改变 frame 内容 |

### 不变量

| 编号 | 条件 | 成立边界 |
| --- | --- | --- |
| S1 | 新 Session 不复用旧目录 | 构造成功后，避免覆盖历史 CSV |
| S2 | Add 每个帧只追加；SetGpu 的下标必须已存在 | 帧收集期间；GPU 完成结果由 Application 按帧 ID 交回 |
| S3 | 失焦策略构造后不变；失焦帧不被过滤 | Add / Export |
| S4 | valid_run 要求正常退出、completed、无无效原因且非空样本 | Export 返回 |
| S5 | 所有方法由游戏主线程调用 | 当前实现全生命周期；此类不是线程安全容器 |
| S6 | finished 状态只在 CSV 和摘要写完后发布 | 正常导出结束；写失败不能报告成功 |

### 接口与生命周期

| 接口 | 行为 | 前提 / 边界 |
| --- | --- | --- |
| 构造 | 新目录、容量预留、版本/策略元数据，initializing 状态 | 父目录存在，S1 |
| `Add(Frame) / Add(Memory)` | 保存观测；strict 失焦记录原因 | S2/S3/S5 |
| `SetGpu(frame, ms)` | 填充已存在 CPU 帧的可选 GPU 时间 | 未回收结果留空，不补 0 |
| `Invalidate(reason)` | 去重追加无效原因 | 可重复；不会改 completed 伪装结束 |
| `Status(phase, elapsed)` | 新临时 YAML → 同目录原子替换 | 只在阶段边界调用；失败抛异常 |
| `Export(normal_exit)` | 输出逐帧/内存/焦点、重算统计、写摘要、finished | S4/S6；导出失败 main 返回 3 |

持有与销毁：main 的 unique_ptr 持有 Session；Application 只借用。Run 内 GL 资源先销毁，Application::Free 再释放上下文；随后 Session 导出纯 CPU 数据。这里没有引入采集线程，也没有跨线程 GL 调用。

复制/移动：禁止复制和移动；main 仅持有一个 Session，避免两个对象向相同目录写出不同结果。

统计：nearest-rank，`ceil(p*N)-1`，不剔除异常慢帧。失焦时长是逐帧 interval 分类估算。状态 sampling 的单次写入发生在采样边界，可能计入首个采样帧成本，未从原始数据中扣除。完整 CSV/排序/YAML 大量导出均在循环退出后。

验证：`performance.export` 的策略、变化计数、缺失 GPU、预热隔离、慢帧保留、异常导出测试；真实窗口结果见 [[Performance-失焦采样]]。

## 本次变更

无。策略、状态和焦点统计已合并到当前设计。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| 帧上限或长时间采样容量不足 | 设计有界异步落盘并独立评估扰动，不简单扩大到无限内存 |
| 需要精确焦点事件时间 | 增加事件时钟与区间归属规则，保持估算字段与精确字段分开 |
