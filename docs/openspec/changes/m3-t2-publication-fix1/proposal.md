---
tags:
  - area/spec
---

# M3-T2 Publication Fix1

## Why

为 [M3-I1/M3-I2](../../../issue/issues.md#issue-简述)保留一份可恢复但当前暂停的修复意图：已有窄 Publish 修补未关闭普通替换的间歇失败，诊断也仅部分交付。T2 已由用户批准不等于文件可靠性通过，也不恢复本任务；节点事实只从 [VERIFICATION](../../../verification/VERIFICATION.md#关键节点)判读。

本轮已按 AGENTS 重新读取规则、docmap、[scope](../../../scope/project-scope.md)、config 和本 change 的实际 full proposal instructions；写入前独立盘点并完整读取当时全部五份现役 specs/scenarios：[module-boundaries](../../specs/module-boundaries/spec.md)、[world](../../specs/world/spec.md)、[platform](../../specs/platform/spec.md)、[foundation-files](../../specs/foundation-files/spec.md)、[renderer](../../specs/renderer/spec.md)。并读取 [T3](../m3-t3-renderer/proposal.md) 的 delta/design/decisions 与文档迁移 change；T4 当时只有 metadata，后续交叉内容完成后须再读，基线有限不代表其他行为不存在。

## What Changes

目标仍是补齐所有发布阶段的原始数值/API/阶段诊断，形成真实失败与底层操作的因果依据，再实施不降低已有读者/硬错误/终态约定的最小修复并验证。F1 只有部分诊断、结构化测试及采集工具交付；F2 根因未定、F3 未选机制、F4 未完成验收。唯一后续任务在 [tasks](tasks.md)，本 proposal 不复制任务表。

**用户暂停继续有效。** 本轮仅迁移知识，不运行新独立进程、采样、受控映射、追踪/管理员工具，不恢复实现。此前 Process Monitor 管理员配置请求已作废；恢复须有新的明确授权，额外系统权限另行申请。

M3-I1、M3-I2、T2-M08 继续开放/保留用户意见；M3-I3 不列必修，不因迁移自动关闭。排除 SDL/renderer 重构、runner 自身无关联 WriteYaml 全局替换、核显、延期玩法、性能优化与历史证据清理。原 [Fix1 来源](../../../spec/M3/M3-T2-Fix1-spec.md)及失败材料只读保留，不再维护平行计划。

## Capabilities

- Modified `foundation-files`：复用现役窄 Publish/终态契约；[delta](specs/foundation-files/spec.md)增加诊断完整性、有因果依据的正常条件可靠性关闭及 V01-V07 验收要求，不把未完成强保证合入现役基线。
- 不新增文件服务、公共 Win32 类型、多写者或断电持久性能力，不预选替代 API、备份/恢复或协议变更。

## Impact

后续候选范围限 foundation Publish、telemetry 状态调用、真实生产库测试及必要的独立采集/协议夹具；按定位结果缩小实际改动。依赖 module-boundaries 的 CPU/生产测试隔离与 foundation-files 的关闭流/同卷前提，不依赖 platform/GPU 才能证明 CPU 文件故障。

T3/T4 可继续各自现有范围，不继承 Fix1 修复完成；短协议诊断不等于 Q06 同源对照、预算、15 分钟玩法或新 GPU 批准。公开现役契约变化时先受控确认；真实 sync/archive 与 Git 操作均不在本轮授权中。
