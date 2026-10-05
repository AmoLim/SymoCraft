---
type: 类设计
status: 已验证（T0范围）
project: Symocraft
module: telemetry
class_name: "Session / SessionConfig / Data YAML Adapter"
inheritance: []
created: 2026-10-05
tags:
  - area/architecture
---

# Telemetry-采样与导出-类设计

关联功能：[Telemetry-采样与导出](Telemetry-采样与导出.md)。不存在的管理类不为模板而增造；自由函数与数据结构按实际实现记录。

最终验证：[M3-T0 交付与验收报告](../../../milestones/m3-t0/README.md)。

## 当前设计

职责：telemetry 接收 CPU 帧、内存和异步 GPU 结果，维持 v2 采样统计与文件协议。其公开头只含项目值类型，不再泄漏 YAML::Node，也不依赖 app 的 StartupOptions。GPU 查询由 renderer 负责，系统内存由 platform 负责。

### 数据成员

| 类型 | 成员 | 初值 / 范围 | 含义与所有权 |
| --- | --- | --- | --- |
| SessionConfig | 构造入参 | 明确分辨率 / 时长 / 焦点策略 | 自有字符串，仅构造期消费 |
| vector<Frame> | Session::frames_ | reserve(100000) | 自有帧记录，上限 1000000 |
| vector<Memory> | Session::memory_ | reserve(1300) | 自有进程 / 设备内存样本 |
| vector<string> | invalid_reasons_ | 空 | 去重后的失败原因 |
| Data::Value | metadata / startup | 未定义 | 自有结构化元数据 |
| bool | completed | false | 完成协议时长后由 app 设置 |

### 不变量

| 编号 | 条件 | 成立边界 |
| --- | --- | --- |
| I1 | 输出目录必须新建且父目录存在 | 构造成功时 |
| I2 | GPU 回填索引必须已存在于 frames_ | SetGpu 入口 |
| I3 | 只有正常退出、完成时长、没有无效原因且采样非空才能有效 | Export |
| I4 | 缺失 GPU 值不伪造成 0，预热不混入统计，慢帧不剔除 | 汇总 / 导出 |

### 接口与生命周期

| 接口 | 行为 | 前提 / 边界 |
| --- | --- | --- |
| Session(config, entry) | 建目录、预留样本容量、发布 initializing | 失败抛异常；不覆盖既有结果 |
| Add(Frame/Memory) | 复制中立样本 | 单线程调用；不借用 GPU / Window 对象 |
| SetGpu(index, ms) | 回填延迟读取的 GPU 样本 | 无匹配帧时抛 logic_error |
| Export(normal_exit) | 写原始样本、统计与 finished 状态 | I3；失败保留已写数据供诊断 |
| Data::DumpYaml / LoadYaml | 项目值与 YAML 私有表示转换 | 不向公开头传递解析器类型 |

- SessionConfig 使用自有字符串，避免 CLI string_view 寿命被统计模块隐含依赖。
- 逐帧仅收集数值，原有预留容量、上限和 GPU 回填方式不变；排序与 YAML 转换发生在采样之后。
- strict / allow-unfocused 的有效性规则、预热剔除、nearest-rank 百分位、缺失 GPU 值和异常导出语义保持原协议。
- 元数据使用 foundation::Data::Value；私有 YAML 适配器负责导出，不改变外部 benchmark 的进程边界。
- 状态文件仍采用同卷临时文件原子替换；不以非原子写入简化迁移。

公开头：[include](../../../../game/modules/telemetry/include/symocraft/telemetry)；实现：[src](../../../../game/modules/telemetry/src)。Session 按单线程调用契约工作，本轮未声明并发或重入能力。

## 本次变更

本次将真实实现归入 telemetry，用公开契约替代旧聚合头依赖。成员、所有权与失效约束以上表为准；尚未完成验证的风险不以“拆库完成”代替。

### 验收案例

验收位置：[关联功能的验收案例](Telemetry-采样与导出.md#验收案例)。正式导出测试、YAML 值往返和 static/edit 真实短采样通过，I1-I4 的已列协议路径具备证据。没有重新运行完整正式协议或用单次短测下性能优化结论。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| 新调用者需要改变生命周期 | 先修改契约与测试，再修改接口，不暴露存储布局解决临时需求 |

