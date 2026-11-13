---
tags:
  - area/spec
---

# telemetry-lifecycle Specification

> 2026-10-19：T4已验收并同步本页要求；Purpose保留原编写时点。交付范围与保留风险见[T4记录](../../changes/archive/2026-10-19-m3-t4-application-simulation/verification.md#2026-10-19-t4整体完成批准)。后续用户明确授权后，T4生产代码及必要构建、测试配套已同步F主工作区；不是Git提交/合并或T5实施授权。

## Purpose

定义游戏调试快照、速率和独立场景的观测边界。旧 Session 封闭、导出和文件发布协议的适用范围见 [开发观测规格](../development-profiling/spec.md)及 [T4 历史记录](../../changes/archive/2026-10-19-m3-t4-application-simulation/verification.md)。

## Requirements

### Requirement: Scenario and observation are independent

app 私有场景对象 SHALL 自有 static/walk/edit 路线/编辑游标，由实际经过时间及必要状态值产生意图/模拟前编辑，不借 Session、Tracy、RenderDoc 或轻量 telemetry 是否存在启停，不拥有 World/Registry 或采样统计。保留原路线、动作预条件、固定世界与实际时间，不把脚本时间改为 Advance 裁剪时间。独立场景不需要输出目录，不新增 Off/Basic/Deep 采样模式、热注入或无窗口性能文件协议。

#### Scenario: M3-T4-A17 A20 existing workload and scope
- **WHEN** 普通运行读取数值，或 static/walk/edit 在无完整性能记录/无外部工具时运行并接收场景窗口事实
- **THEN** 场景动作/顺序/预条件/实际时间保持，edit 仍在模拟前；玩家隔离/允许失焦/Escape 局部锁存/最小化或尺寸不符的明确失败由 app 策略成立，观测不成为玩法前提；旧采样协议不继续生成

### Requirement: Minimal owned debug snapshot

telemetry SHALL 只维护必要调试值：FPS、相邻成功呈现返回的帧间隔、实际物理 TPS、玩家坐标和实际后端，并区分不可用、暂停与有效值。快照 MUST 为自有值，不含 Window/Renderer/World/Registry 或组件地址，下一更新不修改旧副本。采集 SHALL 仅消费 app 传入的时刻、既有推进结果和位姿，不采系统/GPU 内存、不控制玩法、不遍历业务存储；本轮不要求新调试面板。

#### Scenario: DT-A01 read-only snapshots and absent values
- **WHEN** 保留旧快照后继续推进，或还没有两个成功呈现/已暂停/来源不可安全读取
- **THEN** 旧快照保持不变，缺失不填伪 0/NaN/Inf，最后坐标与当前运行状态可区分；重复读取不推进模拟、消费输入或触发 GPU/系统查询

### Requirement: Bounded rates based on completed work

FPS SHALL 使用短窗口内实际 Presented 帧数除以同窗口单调现实秒，TPS SHALL 使用实际完成的物理子步数除以同窗口现实秒，不使用 Advance 次数、实体数或设定 120 Hz 代替。窗口更新 SHALL 仅保留计数、时间和最后值，存储不随运行帧数增长，不保留逐帧历史、分位数、CSV/YAML 性能数据或封闭/导出状态。暂停/不可绘制时 SHALL 使旧速率失效，恢复重建窗口，不将停顿算成持续正常输出或重放暂停时间。

#### Scenario: DT-A02 zero multiple steps and recovery
- **WHEN** 一个窗口出现零/多物理步、Skipped 帧、普通暂停后恢复或很长运行
- **THEN** 按实际完成步数计算 TPS，Skipped 不记 Presented；暂停区间的旧速率不被当作当前值，恢复首帧不带暂停时长生成伪帧间隔，存储保持常量级，既有 120 Hz/最多八步预算不改
