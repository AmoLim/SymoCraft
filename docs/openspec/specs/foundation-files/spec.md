---
tags:
  - area/spec
---

# Foundation Files

## Purpose

维护 Windows `Files::Publish` 已有窄契约及允许失败边界，禁止把尚未关闭的可靠性目标当作现役保证。源码依据：[公开头](../../../../game/modules/foundation/include/symocraft/foundation/files.h)、[当前实现](../../../../game/modules/foundation/src/files.cpp)、[telemetry 调用](../../../../game/modules/telemetry/src/performance.cpp)、[正式库测试](../../../../test/unit/foundation/files_tests.cpp)与 [T2 历史失败](../../../milestones/m3-t2/s5-delivery-status.md#未关闭项)。未来目标只在[暂停 Fix1](../../changes/m3-t2-publication-fix1/proposal.md)；本轮不运行发布或扩大采样。

## Requirements

### Requirement: Closed same-volume publication

调用方 MUST 在 Publish 前完成写入并关闭临时流，临时文件与目标位于同一 Windows 卷。成功发布 SHALL 让新开读者看见完整新文件，兼容 `FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE` 的旧句柄保持完整旧快照，不通过截断旧文件原位重写来实现。API SHALL 只公开 filesystem 路径，无 Win32 SDK 类型。该契约不提供阻塞读者支持、多发布者所有权、断电持久性或对所有 Windows 环境无失败的承诺。

#### Scenario: M3-T2-Fix1-V02 successful snapshot replacement

- **WHEN** 一个关闭的临时文件在满足兼容共享条件下成功替换目标
- **THEN** 新开读者得到完整新内容，既有兼容读者保持旧快照，成功后临时路径不再保留

#### Scenario: M3-T2-Fix1-V03 hard rejection

- **WHEN** 目标/临时文件有不共享删除读者、目标只读、目标是目录或源不存在等不合法条件
- **THEN** SHALL 显式失败，不删除旧目标再改名、不改变 ACL 或输出位置来制造成功；已验证早期硬错误夹具保留其对应路径/内容，不推广成所有晚期失败的回滚保证

### Requirement: Single attempt and original native error

当前 Publish SHALL 先调用无备份、flags=0 的 ReplaceFileW；仅当其即时错误是 ERROR_FILE_NOT_FOUND 时尝试一次 flags=0 的 MoveFileExW，不覆盖竞态中新建目标。ERROR_FILE_NOT_FOUND 不独立证明仅目标缺失。两条失败路径 SHALL 在其他记录/格式化/存在性检查前保存即时 GetLastError，抛出具有原数值 code 的 std::system_error 并区别 API 上下文；不得自动 retry/sleep/吞错。telemetry SHALL 增加实际阶段上下文并保留原 code，不改 v2 协议。

#### Scenario: First publication or competing destination

- **WHEN** ReplaceFileW 返回 ERROR_FILE_NOT_FOUND，或其后另一发布者已创建目标
- **THEN** 只尝试一次不覆盖移动；失败保留移动的原数值 code 与前序 replace_error，不用预先 exists 检查当成功依据

#### Scenario: M3-T2-Fix1-V01 numeric error versus incomplete diagnostics

- **WHEN** 发布失败向 telemetry 传播或启用可选 API 诊断
- **THEN** 原 API/code 和 telemetry 阶段保持；错误对象具有 code 不证明采集日志完整，缺数值历史记录不得从本地化 what() 猜测回填

### Requirement: Error-specific late failure state

失败后 SHALL 按原错误类别记录现场，不承诺所有 I/O 失败都保留两条原路径或全部元数据不变。按 [Microsoft ReplaceFileW 说明](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-replacefilew)，1175 保留原路径名；当前无备份调用下 1176 可能使旧目标不存在，1177 可能使旧目标改名且替代文件继承部分属性/流。路径名保留不是内容/属性全部回滚证明；未知机制 SHALL 不补造责任进程或根因。

#### Scenario: M3-T2-Fix1-V03 late error recording

- **WHEN** 收到 1175、1176、1177 或其他晚期失败
- **THEN** 保留原 code 和真实文件现场，按该错误的已知允许状态报告，不统一断言两个原路径完整不变

### Requirement: Terminal export is not file existence

telemetry SHALL 先发布 exporting，再写 CSV/summary，最后发布 finished；发布失败 SHALL 传播而非声称导出成功。runner SHALL 同时要求正常退出、完整有效摘要/原始数据和 status.yaml 的 finished；即使 summary 声称 valid_run 也不得接受 exporting/终态缺失/非零退出。失败后可能只留下 status.tmp=finished、status.yaml=exporting；不是跨所有输出的事务，也不要求发布失败后内存 completed 自动回滚。

#### Scenario: M3-T2-Fix1-V04 final status publication failure

- **WHEN** CSV/summary 已写出，但最后 Publish 失败
- **THEN** 游戏报告导出失败、runner 拒绝该轮为成绩，保留现场；summary/内存 completed 的存在或真值不替代 finished

## Known Risk And Evidence Boundary

M3-I1 间歇正常替换失败和 M3-I2 诊断完整性仍开放，用户暂停 Fix1。现有 ReplaceFileW 修补只解决旧移动 API 不满足兼容读者的确定性缺口；closed-target 和阻塞读者关闭后也出现失败，外部句柄/过滤来源未知，不归因 SDL/SDK/Defender/沙箱。诊断开关会改变时序，诊断批次全绿不能关闭 M3-I1。T2 节点批准不恢复 Fix1、不将失败改为通过；权威状态见[验证索引](../../../verification/VERIFICATION.md#未通过与未覆盖)及 [Issues](../../../issue/issues.md#m3-i1)。V01-V07 在本页只作稳定验收身份引用，尚未完成的目标/检查不会因迁移进入当前实现。
