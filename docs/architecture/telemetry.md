---
tags:
  - area/architecture
---

# Telemetry

> 已验收候选及保留限制见 [change 验证记录](../openspec/changes/integrate-renderdoc-tracy/verification.md)。构建和运行命令以仓库根目录为准。

telemetry 只保存必要调试数值，不拥有业务对象、采集线程或文件协议。源码为 [DebugObserver](../../game/modules/telemetry/src/debug_snapshot.cpp) 和 [自有快照](../../game/modules/telemetry/include/symocraft/telemetry/debug_snapshot.h)；本轮行为变化见 [change delta](../openspec/changes/integrate-renderdoc-tracy/specs/telemetry-lifecycle/spec.md)，尚未同步为主规格。

## 所有权与接线

Application 持有单线程 DebugObserver，在原有 Advance/Render 之后传入 steady_clock 时刻、实际 Presented、AdvanceStatistics::physical_steps、自有玩家坐标及实际后端。读取快照只复制值，不推进模拟、消费输入、遍历 World/ECS 或查询 GPU/系统内存；旧副本不随下一次更新改变。没有新 HUD 或调试面板。

DebugSnapshot 区分 Unavailable、Active、Paused；FPS、相邻成功呈现返回间隔（毫秒）、实际物理 TPS、position 都为可选值，缺失不填 0/NaN/Inf。非法坐标成为缺值；实际后端单独保存。暂停保留最后位置/后端但使速率失效，读取者必须结合状态解释。

## 常量空间的速率

首个成功 Presented 只建立时间基线。后续成功呈现累计帧数与实际完成物理步数，至少一秒后各除以同一现实时间窗口，发布 FPS/TPS 并开启下一窗口；间隔取相邻成功呈现返回时刻之差，不代表显示延迟。计数不使用固定120Hz或Advance次数替代；零/多子步保持实际值。

Skipped、Pause 或非递增时刻清空速率窗口及上一呈现时刻，恢复首帧重新建立基线，不将暂停时间算进正常输出。计数溢出同样重建窗口。只保存两个时刻、两个计数及最后快照，运行时长不增加存储；没有逐帧历史、分位数、采样状态机或封闭/重试/导出。

## 工具、文档适配与历史

CPU 时间线由独立可选 [profiling 适配](engineering.md#可选-cpu-profiling) 提供，GPU 捕获由外部 RenderDoc 完成，二者不成为快照/普通游戏的运行前提。原 Session、GPU样本回填、进程/显存采样及CSV/YAML性能协议已退役；非性能 YAML 编解码归 [foundation 独立适配](foundation.md#独立-yaml-适配)，world-summary 与故障诊断继续使用。

旧协议/失败与当时批准保持原候选含义，从 [T4 验证](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/verification.md) 及 [验证索引](../verification/VERIFICATION.md) 定位，不重写历史数据或声称修复暂停的 Fix1。当前结果、失败及未测项只记录在 [本 change verification](../openspec/changes/integrate-renderdoc-tracy/verification.md)。
