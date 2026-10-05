---
type: 功能
status: 已验证（T0范围）
project: Symocraft
module: platform
created: 2026-10-05
tags:
  - area/architecture
---

# Platform 窗口与输入

关联类：[Window 与 GraphicsBridge](Window-类设计.md)。依据：[M3-T0](../../../spec/M3-T0-模块软硬边界.md)。

最终验证：[M3-T0 交付与验收报告](../../../milestones/m3-t0/README.md)。

## 当前设计

平台拥有窗口、事件回调、键鼠事实和进程内存查询，不拥有角色或图形资源。实现为 `symocraft_platform` 静态库。公开头只有 `window.h`、`key_snapshot.h`、`process_memory.h`；GLFW、Win32、Psapi 均为私有依赖。

内部划分：`Window::Impl` 保存原生窗口和事件累积状态；`Window` 提供中立查询；`GraphicsBridge` 仅向 renderer 提供上下文操作与呈现；`SampleProcessMemory` 独立负责进程计数器。没有把输入回调改名后继续留作 ECS 修改入口。

源码：[窗口实现](../../../../game/modules/platform/src/window.cpp)、[内存采集](../../../../game/modules/platform/src/process_memory.cpp)、[构建边界](../../../../game/modules/platform/CMakeLists.txt)。本轮实现已落地，具体构建和运行验收以 M3-T0 汇总证据为准；不把用户确认的迁移前玩法回归冒充迁移后实测。

## 本次变更

### 目标与流程

```text
OS 事件 -> 私有 GLFW 回调 -> 按顺序记录鼠标、滚轮和失焦事实
PollInt -> 发布保序事件；CaptureInput -> 每个物理键消费一次 -> app 映射玩家意图
尺寸回调 -> width/height -> app -> renderer 设置 viewport
进程计数器 -> ProcessMemory 值 -> app -> telemetry
```

普通游戏失焦由 app 决定暂停并重置输入；平台不代替应用制定暂停规则。benchmark 保留无边框普通窗口、非置顶、不抢焦点及 `NonRudeHWND` 任务栏策略；允许失焦不等于允许最小化。

### 关键约束与取舍

- Window 独占原生窗口，禁止复制或移动；回调中的用户指针只借用本对象，生命周期见 I1/I2。
- 键状态为固定数组，鼠标/滚轮采用保序双缓冲 vector，每个预留 64 条并跨帧复用容量。超过容量的输入突发允许扩容；不承诺绝对零分配。app 借用快照，不复制整个 vector。保序避免“先撞到俯仰/FOV 上限再反向移动”被合并为零增量而改变玩法。完整事件回放不在 T0 范围。
- benchmark 不注册鼠标/滚轮回调，只读取 Esc，保持原先不采纳玩家输入的路径。CaptureInput 位于应用模拟计时内，避免把旧键查询成本挪进 event_ms。输入缓冲扩容失败先保存异常，再从 PollInt 抛出，不让 C++ 异常穿过 GLFW 的 C 回调边界。
- 回调只修改平台状态，不访问 Registry、相机、世界，也不调用 OpenGL。GLAD 加载和 viewport 已迁出。
- 桥接头只在 `platform/src/graphics_bridge/graphics_bridge.h`，renderer 只得到该单头目录的白名单访问，不能搜索其他平台私有头；公开头不提供 `void*` 句柄逃生口。
- Windows 进程内存读取失败返回空可选值，不把“不支持”伪装为零。

### 验收案例

| 状态 | 场景 | 预期行为 |
| --- | --- | --- |
| [x] | 公开头独立编译 | 无 GLFW/Win32 传播 |
| [x] | 普通游戏切出切回、最小化恢复 | 用户确认本轮安装包视角、输入和恢复正常 |
| [x] | benchmark 失焦但不最小化 | 新 static/edit 全程失焦仍有效完成，焦点样本为 false |
| [x] | 窗口建立后的资源初始化失败 | 三类实际损坏资源探针正常报告错误并退出 3，执行既有清理路径 |
| [ ] | benchmark 主动最小化或改变 framebuffer 专项 | 本轮未执行新的真实故障探针；不能拿普通游戏恢复反馈代替采样无效判定验证 |
| [ ] | 强制窗口创建失败、回调缓冲分配失败 | 尚未专项注入；RAII/延迟异常为实现契约，不称为已实测 |

2026-10-05 最终公开头与全量测试通过，运行证据见最终报告。用户反馈针对本轮 Release 安装包普通玩法；benchmark 失焦来自短采样记录，两者不混用。上表未测专项明确保留，不影响已经建立的 T0 模块边界证据，也不推定长时或跨硬件验证完成。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| T2 新图形后端落地 | 平台桥接按真实后端要求扩展，不公开原生句柄给 app |
| 有输入录制、更多键或多窗口要求 | 独立事件序列、设备映射和多窗口所有权；当前不预造 |
