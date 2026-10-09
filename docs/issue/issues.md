---
type: 未关闭问题清单
status: open
project: Symocraft
updated: 2026-10-08
tags:
  - area/meta
---

# Agent 尚未关闭的 Issues

维护 Agent 已发现但尚未关闭的问题；本次同步范围为 2026-10-08 的 T2 文件发布审计，不代表项目全部未关闭项。阶段验收和人工批准仍以对应 milestone 的固定人工核查入口为准。

编号为 `Mx-Ix`，前一个 `x` 是 milestone 编号，后一个是该 milestone 内递增且不复用的 issue 序号。每行三个单元格均链接到同名附录。`review` 记录人工意见；未取得意见时明确写待审核，Agent 建议不冒充人工决定。关闭后从未关闭主表移除，保留编号、附录、关闭依据和人工审核记录，不删除原始失败证据。

## Issue 简述

| issue code      | issue overview                                                           | review  |
| --------------- | ------------------------------------------------------------------------ | ------- |
| [M3-I1](#m3-i1) | T2 文件发布仍间歇失败：ReplaceFileW 出现 Win32 1175，普通无测试自身持有读者的替换也失败；影响状态协议、采样与稳定交付 | Open不阻塞 |
| [M3-I2](#m3-i2) | 发布失败的数值错误码及阶段诊断不完整：主工程 40 次独立重复中的四次失败均未记录 numeric_win32；妨碍根因定位与准确归类。     | Open不阻塞 |
| [M3-I3](#m3-i3) | S5 完整测试证据索引仍引用日志旧路径；日志已归档且主工程三份摘要匹配，但按索引不能直接复核。                          | 无所谓     |

## 附录

### M3-I1

**详细描述与可能影响**

状态：未关闭。影响节点：M3-T2 的 A01/A11、S5 和最终交付批准。

2026-10-08 Fix1 已开始部分诊断实施，随后按用户明确要求暂停，不继续修复或管理员追踪。新增真实 1175、完整测试仍失败，详见 [Fix1 暂停事实](../spec/M3-T2-Fix1-spec.md#当前暂停)。转向有效完整轮次的 P95/P99 和内存预算预采样，不将暂缓修复等同于风险关闭、协议放宽或最终交付批准。

旧 `MoveFileExW` 在目标存在兼容共享读者时失败，确定性红/绿验证支持改用 `ReplaceFileW`。现行实现先调用 `ReplaceFileW`，仅在 `ERROR_FILE_NOT_FOUND` 时尝试一次不覆盖目标的 `MoveFileExW`；无重试、休眠或吞错。该条件是 API 错误码，不是独立的“目标缺失”判定。窄修复解决旧 API 的兼容读者缺口，不等于稳定性已达标。

Debug 完整测试首轮与诊断复测均为 62/63 启用测试通过，`foundation.files` 失败；既有 `world.allocations` 禁用不算通过。复测明确记录共享读者下 `win32=1175`、循环 iteration=0、reader_share=7。Release 完整测试一次 64/64 不能抵消后续重复失败。

实际主工程测试各 10 次独立运行，结果如下；不是单次发布调用的重试，也不是按文件替换次数统计的失败率。

| 配置 | foundation.files | performance.export |
| --- | --- | --- |
| Debug | 9/10 | 9/10 |
| Release | 8/10 | 10/10 |

Debug files 第 5 次在普通 closed-target 替换处失败，尚未进入共享读者循环；Release files 第 6 次在阻塞读者关闭后的恢复处失败。这证明失败不限于测试自身持有兼容读者的阶段，但不排除外部句柄或过滤干预。Debug export 第 4 次保留的 strict 目录中，`status.tmp` 已为 finished，`status.yaml` 仍为 exporting：导出文件存在不能替代终态发布成功，可能使协议消费者无法确认完成。

独立直接诊断另为 16/20，四次共享读者失败明确记录 1175，不混入主工程统计。具体外部句柄、进程或过滤来源尚未定位；不能归因 SDL、SDK、Defender 或沙箱，也不能认定历史 S2 失败都与旧 API 缺口同因。

替代 `FileRenameInfo` 与 `FileRenameInfoEx` 的非 POSIX 实验均在兼容读者下返回 5；每种 API 的 10 个独立循环组都在首次调用停止，成功替换为零，不能宣称完成 1000 次替换。未替换生产策略，未放宽不共享删除读者和只读目标的硬错误约定。

晚期失败状态须按错误类别区分：官方说明 1175 保留两文件的原路径名，但无备份的 1176 可使旧目标不再存在，1177 可使旧目标改名。当前无备份调用不能承诺所有晚期 I/O 失败都保留两个路径；路径名保留也不等于所有元数据和流均未改变。


**归档证据**

- [S5 交付状态与未关闭项](../milestones/m3-t2/s5-delivery-status.md#未关闭项)、[T2 人工核查入口](../milestones/m3-t2/manul-verification.md)：现行交付门槛，重点 T2-M08；人工意见仍待确认。
- [窄修复红/绿摘要](../../out/m3-t2/s5/publish-audit-001/summary.json)、[旧实现失败日志](../../out/m3-t2/s5/publish-audit-001/files-before-runtime.log)、[新实现初次通过日志](../../out/m3-t2/s5/publish-audit-001/files-after-runtime.log)：只证明兼容读者缺口的修复。摘要内旧 `files-after.exe` 身份是历史记录；同名程序后来重新编译，当前身份见下一项。
- [直接诊断摘要及逐次日志索引](../../out/m3-t2/s5/publish-audit-001/diagnostic-002/summary.json)：16/20，记录当前直接诊断程序身份及四次 1175。
- [主工程独立重复摘要](../../out/m3-t2/s5/publication-repeat-003/summary.json)：40 次运行及原始 stdout/stderr、失败现场文件和摘要；本次审计核对其中 108 项文件身份均匹配。`publication-repeat-001/-002` 是采集器中断记录，不计作完整 40 次结果。
- [Debug 首轮日志](../../out/m3-t2/log/s5/build/main-debug-001-test.log)、[Debug 诊断复测日志](../../out/m3-t2/log/s5/build/main-debug-001-followup-test.log)、[Release 完整测试日志](../../out/m3-t2/log/s5/build/main-release-001-test.log)：实际归档位置；证据索引旧路径问题见 [M3-I3](#m3-i3)。
- [替代 API 实验摘要](../../out/m3-t2/s5/rename-api-audit-001/summary.json)、[原始逐案例记录](../../out/m3-t2/s5/rename-api-audit-001/result.jsonl)：两种非 POSIX API 不满足兼容读者约定；本次审计核对摘要关联的 7 项文件身份均匹配。
- [生产发布实现](../../game/modules/foundation/src/files.cpp)、[状态发布调用](../../game/modules/telemetry/src/performance.cpp)：源码依据，不作为新的运行通过证据；写流在发布前显式关闭。
- [微软 ReplaceFileW 文档](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-replacefilew)：1175/1176/1177 的错误与失败后状态边界，不提供责任进程归因。

`out/` 链接为本工作区归档产物，不代表这些日志或程序已随 Git 仓库发布；不移动或改写原始证据。

### M3-I2

**详细描述与可能影响**

状态：未关闭；关联 [M3-I1](#m3-i1)。

历史主工程重复摘要的四次失败均为非零退出，但 `numeric_win32` 为空。当时 `files_tests.cpp` 仅在共享读者循环的 catch 中输出数值错误码、循环下标、share 和路径属性；普通 closed-target 替换、读者关闭后恢复等路径没有同等失败记录。历史 Release 主工程测试程序也不含增强共享读者诊断，摘要已明确其诊断字段不存在，不能拿当前源码快照冒充当时程序修订。

Fix1 已增加结构化全操作记录、API 可选日志和 telemetry 原错误码/阶段上下文，新失败能直接记录 1175；原始缺码不回填。诊断完整性校验仍有缺口，V01 未完成，M3-I2 不关闭。用户已暂停该工作，详见 [当前暂停](../spec/M3-T2-Fix1-spec.md#当前暂停)。

错误对象保留 Win32 错误码，不等于每份原始日志都输出了该码。完整 Debug 诊断复测与直接诊断证明发生过 1175，但不能直接把主工程四次重复失败逐一断言为数值码 1175。缺少阶段、即时错误码及现场关联信息会妨碍来源定位，也可能把不同失败机制合并。


**归档证据**

- [主工程独立重复摘要](../../out/m3-t2/s5/publication-repeat-003/summary.json)：四次失败的 `numeric_win32` 为空；记录两配置程序身份与诊断是否存在。
- [Debug files 第 5 次 stdout](../../out/m3-t2/s5/publication-repeat-003/debug/foundation.files-iteration-005/stdout.log)、[stderr](../../out/m3-t2/s5/publication-repeat-003/debug/foundation.files-iteration-005/stderr.log)：仅完成初次发布，随后普通替换失败，无数值码。
- [Release files 第 6 次 stdout](../../out/m3-t2/s5/publication-repeat-003/release/foundation.files-iteration-006/stdout.log)、[stderr](../../out/m3-t2/s5/publication-repeat-003/release/foundation.files-iteration-006/stderr.log)：已完成共享读者循环及预期拒绝，恢复发布失败，无数值码。
- [Debug 完整诊断复测日志](../../out/m3-t2/log/s5/build/main-debug-001-followup-test.log)、[直接诊断摘要](../../out/m3-t2/s5/publish-audit-001/diagnostic-002/summary.json)：明确 1175 的独立依据，不混同上述缺码记录。
- [foundation 测试源码](../../test/unit/foundation/files_tests.cpp)、[telemetry 测试源码](../../test/unit/telemetry/performance_tests.cpp)：当前部分补齐后的源码，不冒充历史运行身份；完整性审核仍未通过。

### M3-I3

**详细描述与可能影响**

状态：未关闭；关联 [M3-I1](#m3-i1) 的证据可追溯性。

`s5-validation.json` 的 `main_and_matrix_tests` 中，三份主工程测试日志仍引用 `out/m3-t2/s5/` 下的旧路径；当前实际位于 `out/m3-t2/log/s5/build/`。三份归档日志的 SHA-256 与索引记录全部匹配，所以不是这三份证据丢失或结果改写，而是索引未提供有效导航。这里仅确认这三份主工程日志，不把其他矩阵/探针路径未经逐项核对就判为丢失或全部修复。

旧路径会使审核者按摘要无法直接复核 62/63、64/64 和明确 1175 的现场。应区分历史执行路径与当前导航：历史快照和执行事实保留原值，通过可维护的路径对照或旁置导航提供当前归档入口，不重写原始证据。

**归档证据**

| 索引记录的旧路径 | 当前归档入口 | SHA-256 核对 |
| --- | --- | --- |
| `out/m3-t2/s5/main-debug-001-test.log` | [Debug 首轮](../../out/m3-t2/log/s5/build/main-debug-001-test.log) | 匹配 `c437c2e84a0b87e500fdb9d115279878e4688acc281bebe33fc2295f489c2c1d` |
| `out/m3-t2/s5/main-debug-001-followup-test.log` | [Debug 诊断复测](../../out/m3-t2/log/s5/build/main-debug-001-followup-test.log) | 匹配 `82f483de213628c6c74a24c1bda7295d4339febdfc00c40a65a802b32f441779` |
| `out/m3-t2/s5/main-release-001-test.log` | [Release 完整测试](../../out/m3-t2/log/s5/build/main-release-001-test.log) | 匹配 `29c430617566adf4d42238ce0775703d77721980d68f1b95779213d922a48d58` |

索引来源：[S5 验证身份](../milestones/m3-t2/evidence/s5-validation.json) 的 `main_and_matrix_tests`。本次只登记问题与三份日志当前入口，不改动该历史证据文件，也不将当前入口补充视为所有历史入链已修复。
