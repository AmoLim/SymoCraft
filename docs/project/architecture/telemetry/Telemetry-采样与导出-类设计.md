---
type: 数据设计
status: 已验证（T0范围）
project: Symocraft
module: telemetry
created: 2026-10-05
updated: 2026-10-06
tags:
  - area/architecture
---

# Telemetry 采样数据与导出摘要

历史文件名保留，资源状态与原 I1-I4 归 [Session](Session-类设计.md#不变量)。
关联：[功能验收](Telemetry-采样与导出.md)、[协议](../../benchmark/Benchmark-文件协议.md)、[覆盖清单](../对象笔记覆盖清单.md)。

## 当前设计

### 工作负载与布局

Frame/Memory append 到 Session 自有 vector，GPU ms 迟到按索引回填；采样结束复制数值/排序/YAML 转换，公开头不泄漏 parser/app 配置。AoS 并排字段，预留/上限见 Session；规模/访问分布/瓶颈未测，不宣称 DOD 优化。

### SessionConfig

自有 string output_directory/focus_policy（strict）/benchmark；unsigned width=1920,height=1080,warmup_seconds=60,sample_seconds=180；vsync=false。仅构造消费，不借用 CLI string_view，Session 不重校所有 CLI 组合。

### Frame

double elapsed 为秒；frame_ms/simulation_delta_ms/event_ms/simulation_ms/mesh_ms/pack_ms/render_cpu_ms/upload_cpu_ms/present_ms/instrumentation_ms 为毫秒；optional<double> gpu_draw_ms 默认缺失。
uint64 rebuilt_chunks/vertices/upload_bytes/draw_calls/edits 为计数/字节；double edit_lateness_ms 毫秒、x/y/z 世界坐标、yaw 角度；bool measured/focused。数值/布尔默认 0/false，app 设置预热/采样标志。
frame_ms 为连续 SwapBuffers 返回间隔，非物理延迟；simulation_delta_ms 为裁剪帧步长。GPU 缺失不同于真实零绘制段 0，范围归 [GpuTimer](../renderer/GpuTimer%20Class.md#采样关联与计时范围)。

### Memory

elapsed 秒；optional<uint64_t> working_set_bytes/private_bytes 进程字节，device_dedicated_kib/device_available_kib 设备级 KiB。失败缺失不补零，不是进程 VRAM；本记录不拥有系统 handle。

### YAML 适配

Data::DumpYaml/LoadYaml 为自由函数，公开仅 Data::Value，私有 YAML 转换保留 map 顺序和 float/double。状态文件同卷关闭临时文件原子发布，不虚构 Adapter 类。
[适配头](../../../../game/modules/telemetry/include/symocraft/telemetry/document_io.h)、[实现](../../../../game/modules/telemetry/src/document_io.cpp)。

### 有效期、历史与风险

值复制独立，数组扩容/替换/销毁使元素借用失效；单线程采集/导出，无并发/重入保证。T0 YAML/app 解耦，strict/allow-unfocused、预热排除/nearest-rank/慢帧保留/异常导出沿 v2 协议；并非所有入口校验所有字段。
[声明](../../../../game/modules/telemetry/include/symocraft/telemetry/performance.h)、[功能验收](Telemetry-采样与导出.md#验收案例)、[T0 报告](../../../milestones/m3-t0/README.md)保留正式导出/YAML/static/edit 短测结论；2026-10-06 未重新跑完整正式协议或作性能提升判断。

### 公开接口预期行为

| 签名 / 入口 | 调用方与可见范围 | 预期行为：输出及状态变化 | 前提 / 边界 | 失败反馈及失败后状态 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| 不适用：SessionConfig / Frame / Memory 无显式函数 | app / Session | 默认值见各字段节；按值复制 optional/数值/string，标准移动语义 | 字段公开，不全面校验所有数值；Session vector 借用会随扩容失效 | string 复制/分配可抛，不拥有 GPU/OS handle | [performance.h](../../../../game/modules/telemetry/include/symocraft/telemetry/performance.h) |
| Milliseconds / Statistics / YAML adapter（索引） | 模块公开自由函数 | [API 权威表](Telemetry-namespace-API.md#公开接口预期行为) | 不是 Frame/Memory 方法 | 同目标表 | performance.h / document_io.h |

### 私有函数预期行为

| 签名 / 入口 | 内部调用方 | 预期行为：处理规则及副作用 | 前提 / 边界 | 失败传播及清理责任 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| 不适用：数据类型无 private 函数 | 数据消费者 | Encode/Decode 为外部 adapter helper，见 [API 内部表](Telemetry-namespace-API.md#私有函数预期行为) | Output/Optional 见 [Session](Session-类设计.md#私有函数预期行为) | 相应 owner 负责清理 | document_io.cpp / performance.cpp |

## 本次变更

无。

## 后续考虑

采样扩展先定义单位/缺失/有效性与版本，不复制 benchmark 权威协议。

