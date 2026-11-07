---
tags:
  - area/spec
---

# Foundation Publication Retirement Delta

## REMOVED Requirements

### Requirement: Closed same-volume publication
**Reason**: 独立 benchmark 与性能导出链退役后，Files::Publish 已无生产调用方；2026-11-03 用户将该 API、实现和专属测试纳入退役范围，不继续维护无消费者的发布服务。
**Migration**: 按准确清单确认后移除公开头、实现及正式库专属测试和注册，不提供兼容壳或替代发布 API。M3-T2-Fix1-V02/V03 的原契约、历史成功与失败保留在原记录；撤销现役能力不证明旧发布故障修复。文档 Load/Dump、world-summary、PNG 输出及仍有效的非性能文件行为继续保留。

### Requirement: Single attempt and original native error
**Reason**: Files::Publish 整体退役，不再保留 ReplaceFileW/MoveFileExW 发布入口或无业务调用方的生产实现。
**Migration**: 首次创建竞态及 M3-T2-Fix1-V01 的诊断规则保留历史原义，不补填缺失数值，不把原失败改成通过。Fix1 的取消或归档单独处理，不将未完成的诊断/可靠性增量合并为现役保证。

### Requirement: Error-specific late failure state
**Reason**: 发布 API 被撤销，其晚期失败状态不再作为新候选的活动 API 契约。
**Migration**: M3-T2-Fix1-V03、1175/1176/1177、根因未知及真实现场保持历史记录；既有失败和超时结果不重写，不由移除测试声称修复或回滚保证成立。未来若重新引入发布能力，须另有设计、授权与验证。

### Requirement: Terminal export is not file existence
**Reason**: 旧性能 exporting/CSV/summary/finished 与 runner 消费链和其发布基础能力一起退役。
**Migration**: 新运行不生成旧性能终态；M3-T2-Fix1-V04 及原协议结果、失败输出保留。RenderDoc/Tracy 保存各自诊断产物，不承担游戏文件可靠性或历史终态兼容职责。

## ADDED Requirements

### Requirement: Publication retirement and retained evidence

退役后的生产构建、公开头消费者、测试注册与安装图 SHALL 不再包含 Files::Publish 入口、实现或 foundation.files 专属测试；准确删除清单 MUST 先汇报并获其后的明确确认。退役 SHALL 保留其他 foundation 能力、独立 YAML 适配及其有效测试，不以空实现、吞错或仅禁用失败测试替代能力移除。历史失败、冻结证据与 Fix1 诊断现场 MUST 保留；取消修复计划或归档不表示根因定位、修复或 V01–V07 通过。

#### Scenario: FP-R01 removed capability in the final candidate
- **WHEN** Publish 按已确认清单实际移除并建立最终候选
- **THEN** 生产源码、公开头编译消费者、CTest 注册及安装中均无该能力；仍有效的文档往返、world-summary 与图像输出有对应验证，历史 foundation.files 的失败不回写为通过

#### Scenario: FP-R02 historical Fix1 versus current acceptance
- **WHEN** 取消或归档 Fix1、审阅新 N1 或未来重新提出文件发布需求
- **THEN** 历史未完成任务与失败保持原义，未实施的 Fix1 增量不合并现役规格；新 N1 以实际退役后的候选和适用检查为依据，规格修改或归档本身不证明 API 已移除或节点已通过
