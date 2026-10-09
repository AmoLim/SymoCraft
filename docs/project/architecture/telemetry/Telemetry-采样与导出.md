---
type: 功能
status: 已验证（T0范围）
project: Symocraft
module: telemetry
created: 2026-10-05
updated: 2026-10-06
tags:
  - area/architecture
---

# Telemetry-采样与导出

关联设计：[采样数据](Telemetry-采样与导出-类设计.md)、[Session](Session-类设计.md)。统一验收：[M3-T0](../../../spec/M3-T0-模块软硬边界.md)。

最终验证：[M3-T0 交付与验收报告](../../../milestones/m3-t0/README.md)。

## 当前设计

telemetry 接收 CPU 帧、内存和异步 GPU 结果，维持 v2 采样统计与文件协议。其公开头只含项目值类型，不再泄漏 YAML::Node，也不依赖 app 的 StartupOptions。GPU 查询由 renderer 负责，系统内存由 platform 负责。

实现位置：[模块源码](../../../../game/modules/telemetry/CMakeLists.txt)。当前实现与历史验证见下文；不将已有 M2 结果冒充新实现证据。

### 既有 T0 实现与验收

### 目标与流程

T0 仅建立真实模块、公开数据契约及测试边界，维持原正常运行路径，不提前做领域算法重写。

启动参数由 app 组装 SessionConfig，main 持有 Session，app 用 Add/SetGpu 收集与回填；退出后导出顺序唯一维护在 [Session 流程图](Session-类设计.md#导出顺序与发布)。

### 公开接口预期行为

| 签名 / 入口 | 调用方与可见范围 | 预期行为：输出及状态变化 | 前提 / 边界 | 失败反馈及失败后状态 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| Session 全部公开成员 | main / app | [Session 权威表](Session-类设计.md#公开接口预期行为) | 收集、回填、导出串行；文件部分完成 | 同目标表 | performance.h / I1-I4 |
| Milliseconds / Statistics / DumpYaml / LoadYaml / LoadYamlFile | 模块消费者 | [namespace API 权威表](Telemetry-namespace-API.md#公开接口预期行为) | 五项单独签名与转换前提 | 同目标表 | performance.h / document_io.h |
| SessionConfig / Frame / Memory 值语义 | app / Session | [数据权威表](Telemetry-采样与导出-类设计.md#公开接口预期行为) | 无自定义成员函数 | 同目标表 | performance.h |

### 私有函数预期行为

| 签名 / 入口 | 内部调用方 | 预期行为：处理规则及副作用 | 前提 / 边界 | 失败传播及清理责任 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| Output / Optional | Status / Export | [Session 内部表](Session-类设计.md#私有函数预期行为) | TU helper | 同目标表 | performance.cpp |
| Encode / Decode 及 Encode visit lambda | YAML adapter | [API 内部表](Telemetry-namespace-API.md#私有函数预期行为) | 私有 YAML，不传播 parser 类型 | 同目标表 | document_io.cpp |

### 关键约束与取舍

- SessionConfig 使用自有字符串，避免 CLI string_view 寿命被统计模块隐含依赖。
- 逐帧仅收集数值，原有预留容量、上限和 GPU 回填方式不变；排序与 YAML 转换发生在采样之后。
- strict / allow-unfocused 的有效性规则、预热剔除、nearest-rank 百分位、缺失 GPU 值和异常导出语义保持原协议。
- 元数据使用 foundation::Data::Value；私有 YAML 适配器负责导出，不改变外部 benchmark 的进程边界。
- 状态文件仍采用同卷临时文件原子替换；不以非原子写入简化迁移。

### 验收案例

| 状态 | 场景 | 预期行为 |
| --- | --- | --- |
| [x] | 独立公开头消费者 | 不需要聚合 core.h、SDK 或私有 include |
| [x] | performance.export + foundation.document | 正常 / 失败路径通过正式库验证 |
| [x] | Debug / Release 与 CPU-only | 相同源码所有者；CPU 库无需窗口 |
| [x] | static/edit 真实短采样 | v2 元数据和 CSV 列保持；失焦样本继续采集，缺失 GPU 值不伪造成 0 |

2026-10-05 最终测试矩阵通过，真实采样为每场景预热 5 秒、采样 15 秒；新 static/edit 在失焦条件下有效完成，旧新元数据及帧 CSV schema 对照一致。详细结果见最终报告；这不是新的 36 分钟冻结基准，也不证明长时稳定性或各档硬件性能达标。

## 本次变更

无。2026-10-06 仅整理文档；以上为原 T0 实现与验收记录，不是本次重新运行测试。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| 边界成为实际瓶颈 | 先测量分配、复制或调用开销，再调整接口；不恢复私有跨模块访问 |
| 后续阶段修改内部算法 | 同步更新本笔记、类不变量与回归证据 |

