---
tags:
  - area/spec
---

# Foundation Files

## Purpose

界定 Windows `Files::Publish` 退役后的构建、文档及诊断边界。旧发布契约及故障现场见 [Fix1 历史记录](../../changes/archive/2026-11-03-m3-t2-publication-fix1/verification.md)及 [退役清单](../../changes/integrate-renderdoc-tracy/verification.md#2026-11-03-publish-追加退役范围与准确清单)。

## Requirements

### Requirement: Publication retirement and retained evidence

退役后的生产构建、公开头消费者、测试注册与安装图 SHALL 不再包含 Files::Publish 入口、实现或 foundation.files 专属测试；准确删除清单 MUST 先汇报并获其后的明确确认。退役 SHALL 保留其他 foundation 能力、独立 YAML 适配及其有效测试，不以空实现、吞错或仅禁用失败测试替代能力移除。历史失败、冻结证据与 Fix1 诊断现场 MUST 保留；取消修复计划或归档不表示根因定位、修复或 V01–V07 通过。

#### Scenario: FP-R01 removed capability in the final candidate
- **WHEN** Publish 按已确认清单实际移除并建立最终候选
- **THEN** 生产源码、公开头编译消费者、CTest 注册及安装中均无该能力；仍有效的文档往返、world-summary 与图像输出有对应验证，历史 foundation.files 的失败不回写为通过

#### Scenario: FP-R02 historical Fix1 versus current acceptance
- **WHEN** 取消或归档 Fix1、审阅新 N1 或未来重新提出文件发布需求
- **THEN** 历史未完成任务与失败保持原义，未实施的 Fix1 增量不合并现役规格；新 N1 以实际退役后的候选和适用检查为依据，规格修改或归档本身不证明 API 已移除或节点已通过

## Known Risk And Evidence Boundary

M3-I1 间歇正常替换失败和 M3-I2 诊断完整性仍开放，Fix1 已因发布 API 退役取消并归档。退役前的 ReplaceFileW 修补只解决旧移动 API 不满足兼容读者的确定性缺口；closed-target 和阻塞读者关闭后也出现失败，外部句柄/过滤来源未知，不归因 SDL/SDK/Defender/沙箱。诊断开关会改变时序，诊断批次全绿不能关闭 M3-I1。T2 节点批准不恢复 Fix1、不将失败改为通过；权威状态见[验证索引](../../../verification/VERIFICATION.md#未通过与未覆盖)及 [Issues](../../../issue/issues.md#m3-i1)。V01-V07 在本页只作稳定验收身份引用，尚未完成的目标/检查不会因迁移进入当前实现。
