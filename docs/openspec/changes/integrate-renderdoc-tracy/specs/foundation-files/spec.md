---
tags:
  - area/spec
---

# Foundation Files Consumer Retirement Delta

## MODIFIED Requirements

### Requirement: Single attempt and original native error

当前 Publish SHALL 先调用无备份、flags=0 的 ReplaceFileW；仅当其即时错误是 ERROR_FILE_NOT_FOUND 时尝试一次 flags=0 的 MoveFileExW，不覆盖竞态中新建目标。ERROR_FILE_NOT_FOUND 不独立证明仅目标缺失。两条失败路径 SHALL 在其他记录/格式化/存在性检查前保存即时 GetLastError，抛出具有原数值 code 的 std::system_error 并区别 API 上下文；不得自动 retry/sleep/吞错。仍活动的调用方 SHALL 保留原 code 与实际阶段；旧 telemetry v2 消费者随性能导出退役，不再要求维持该协议，不修改 Publish 机制或晚期允许失败边界。

#### Scenario: First publication or competing destination
- **WHEN** ReplaceFileW 返回 ERROR_FILE_NOT_FOUND，或其后另一发布者已创建目标
- **THEN** 只尝试一次不覆盖移动；失败保留移动的原数值 code 与前序 replace_error，不用预先 exists 检查当成功依据

#### Scenario: M3-T2-Fix1-V01 numeric error versus incomplete diagnostics
- **WHEN** 发布失败向仍活动的调用方传播或启用可选 API 诊断
- **THEN** 原 API/code 和调用阶段保持；错误对象具有 code 不证明采集日志完整，缺数值历史记录不得从本地化 what() 猜测回填，不因旧 telemetry 退役关闭诊断完整性问题

## REMOVED Requirements

### Requirement: Terminal export is not file existence
**Reason**: 本条仅约束随本 change 退役的性能 exporting/CSV/summary/finished 与 runner 消费链；不是 Files::Publish 基础能力。
**Migration**: 新运行不再发布这些性能终态；历史失败、导出文件和原协议依据原样保留。其删除不解除 Closed same-volume publication、Single attempt and original native error、Error-specific late failure state，不修改或恢复暂停 Fix1 的目标，不将 1175/1176/1177 改述为已修复。
