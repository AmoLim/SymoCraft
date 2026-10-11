---
tags:
  - area/spec
---

# Publication Fix1 Delta

本 delta 是暂停中的目标，不是已实现强保证。未修改的[现役窄契约及全部场景](../../../../specs/foundation-files/spec.md)继续有效，包括 1175/1176/1177 各自失败状态、首次移动不覆盖、即时错误传播、兼容读者旧快照和终态要求；不通过 MODIFIED 省略旧场景。机制见 [design](../../design.md)，唯一活动计划见 [tasks](../../tasks.md)。

## ADDED Requirements

### Requirement: Complete publication diagnostics

诊断与采集 SHALL 覆盖 foundation 全部实际发布路径及 telemetry 初始化/各状态/导出终态，保存原数值 code/category、API/flags、实际阶段、运行/配置/源码/程序身份和可关联文件现场。即时错误 MUST 在诊断查询前保存；测试/采集 SHALL 区分预期拒绝、意外发布失败、断言/序列化/写流、超时与采集器故障，不用本地化文本猜码。计数 MUST 使用进程、实际 API 调用及预期拒绝各自分母；缺事件/缺码/采集失败不得被 complete 或空 stderr 隐藏。

#### Scenario: M3-T2-Fix1-V01 all stages and collector gaps

- **WHEN** 两配置执行 closed-target、共享循环、阻塞读者关闭后恢复、初次/回退及各 telemetry 状态，或受控移除 API/终事件、触发采集器异常
- **THEN** 身份/阶段/原数值/API 可准确关联；缺码或漏采被判诊断不合格，不能补填历史缺码；受控故障只证明诊断覆盖，不计真实 API 稳定性

### Requirement: Causally justified normal-condition publication

修复关闭 MUST 有至少一份真实失败与底层操作时间线/受控对照支持的机制，区分证实来源、排除项和仍未知项。针对机制 SHALL 采用最小所属修复，保持现役窄契约和硬拒绝/错误边界；不得用重试、休眠、吞错、先删目标、改 ACL/只读、降协议或换输出位置掩盖失败。不以一次/追踪批次全绿、统计变化或等待环境变化关闭 M3-I1；正常条件仍有未解释失败时 SHALL 保留开放状态。

#### Scenario: M3-T2-Fix1-V02 normal and shared-reader publication

- **WHEN** 验证首次发布、closed-target 和原 100 次兼容共享删除读者循环
- **THEN** 每次实际成功后新读者取得完整新内容、既有读者保持完整旧快照且无未提交 tmp，不仅检查 API 返回值

#### Scenario: M3-T2-Fix1-V03 rejection recovery and race

- **WHEN** 验证阻塞目标/源、只读、目录、缺源、读者关闭后恢复或首次 Move 前目标竞争创建
- **THEN** 硬错误及原数值保持，受控早期安全失败内容不受损，关闭阻塞读者后正常发布成立，首次 Move 不覆盖竞争目标；用受控同步而非休眠碰概率。晚期错误分类不冒充真实 API 触发的文件状态验证

### Requirement: Valid terminal delivery and bounded acceptance

Fix1 验收 SHALL 保持 Benchmark v2 协议/工作负载、现有终态有效性与原文件失败；新候选 SHALL 绑定真实源码/库/exe/包及本机条件。M3-I1 的关闭 MUST 同时满足 F2/F3 因果依据与 V01-V07，不由 400 个绿进程单独证明 Windows 普遍稳定。用户暂停时 MUST 不执行新采样/追踪/实现，T2 批准不恢复本任务，也不自动关闭 M3-I2/T2-M08。

#### Scenario: M3-T2-Fix1-V04 status and terminal protocol

- **WHEN** 真实 telemetry 初始化/导出、strict/allow-unfocused，以及 runner CPU 协议夹具被验证
- **THEN** 成功时 finished、summary/CSV 一致且旧读者保持旧快照；runner 拒绝 valid_run/CSV 存在但缺 finished、仍 exporting 或非零退出的轮次，保留失败材料和正常清理

#### Scenario: M3-T2-Fix1-V05 independent final repeats

- **WHEN** 获恢复授权后最终同一身份的 Debug/Release 各运行 foundation.files 与 performance.export 各 100 个独立进程
- **THEN** 共 400 计划进程全部完成且意外发布失败为零，无进程内重试/追踪验收，分母及实际次数如实区分；不足额或未解释失败不申请关闭

#### Scenario: M3-T2-Fix1-V06 complete regression

- **WHEN** 执行全新两配置主 CTest、CPU-only、独立工具及根 runner-only/独立 tools/benchmark/game-only/关闭测试的适用构建安装和边界
- **THEN** 应执行项有独立真实结果，禁用/不可用单列，不固定旧数量、不启用无关禁用测试、不用定向绿代替完整结果

#### Scenario: M3-T2-Fix1-V07 actual candidate delivery

- **WHEN** 新候选游戏/runner 运行三场景短协议、终态、正常取消/落盘和受影响部署路径
- **THEN** 保留实际本机 RTX 5070 Ti/程序/包身份及完整工作负载；短诊断不宣称 Q06 同源对照、性能预算或 15 分钟人工通过，也不覆盖旧包/失败
