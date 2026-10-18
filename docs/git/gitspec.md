---
tags:
  - area/meta
---

# M3-T5 变更提案提交批次

2026-10-18 本轮授权提交现有 T5 change proposal。范围为完整规划材料及本页，仅创建本地提交，不推送、合并、实施、删除生产文件或执行 OpenSpec 规格同步与归档。

## 完整意图与范围

一个提交保存 M3-T5 ECS 与内存契约的完整提案：EnTT 加项目 typed 门面方向、身份与生命周期、查询/借用/失败边界、T4 消费者迁移、受控退役及分层验证。规划不等于依赖冻结、生产实施或阶段批准；不改写原提案内容或勾选任务。

准确范围为本页及 `docs/openspec/changes/m3-t5-ecs-memory/` 下以下七项：

- `.openspec.yaml`
- `proposal.md`
- `design.md`
- `decisions.md`
- `tasks.md`
- `specs/ecs/spec.md`
- `specs/module-boundaries/spec.md`

其他架构、T3/T4、OpenSpec config、scope、全局验证索引及个人目录的并发改动均不纳入，冻结证据保持不动。提案引用的 T4 最新交接材料部分仍未提交，本批不替代该依赖交付。原提案中 Vulkan/T3 尚未交接或批准等描述属于起草时点，已与当前 T3 提交和批准状态不一致，实施前须刷新并核对；本批只保存现有规划，不据此回退当前阶段结论。

## 检查与提交约定

通过项目 wrapper 执行 T5 单 change 严格校验，结果 valid、零 issues；检查本地相对链接文件目标存在。提交前检查准确暂存路径、对象与完整差异，并运行暂存差异空白检查。仅校验规划文档，不运行 C++ 构建、CTest、候选适配、GPU、人工或性能测试，不将规划校验当作工程验收。

提交消息：`docs(t5): 提交ECS与内存契约变更提案`。提交后核对八项文件集合、索引及剩余工作区状态，不为回填自身 hash 追加提交。
