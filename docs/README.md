---
tags:
  - area/meta
---

# 技术文档入口

> 已验收候选及保留限制见 [change 验证记录](openspec/changes/integrate-renderdoc-tracy/verification.md)。构建和运行命令以仓库根目录为准。

## 项目约定
- [Agent 工作边界](../AGENTS.md)与[文档治理](DOCUMENTATION_MAP.md)：先确认现行规则，不沿用退役模板或历史授权。
- [项目范围](scope/project-scope.md)、[未来方向](scope/future-scope.md)、[自动化职责](scope/C-Scripting-Policy.md)。

## 当前架构
从 [architecture/Overview.md](architecture/Overview.md) 按模块进入：业务模块、app、可选观测适配与工程级机制使用扁平正文；不维护平行逐类/逐函数学习目录。

## 设计与任务
- [现役规格](openspec/specs)：精简、已核对的公开行为基线，未覆盖不等于没有实现。
- [T3渲染接管归档](openspec/changes/archive/2026-10-19-m3-t3-renderer/proposal.md)、[T4应用与模拟归档](openspec/changes/archive/2026-10-19-m3-t4-application-simulation/proposal.md)、[Fix1 取消归档](openspec/changes/archive/2026-11-03-m3-t2-publication-fix1/proposal.md)：T3/T4保留已完成设计、决策与验证历史；Fix1于2026-11-03因 Publish 退役取消计划归档，未合并修复规格，历史失败不改判通过。
- 文档迁移已归档，历史范围与处置从 [VERIFICATION](verification/VERIFICATION.md#后续集中与收口) 进入，不是活动计划或游戏实施授权。
- [RenderDoc / Tracy 当前 change](openspec/changes/integrate-renderdoc-tracy/proposal.md)及其[验证记录](openspec/changes/integrate-renderdoc-tracy/verification.md)：独立场景、轻量观测和旧链退役已通过整体验收。现役行为见 [开发观测主规格](openspec/specs/development-profiling/spec.md)。
- 当前无 Git 提交批次，稳定规则见 AGENTS，不预建空批次计划。

OpenSpec 的唯一目录为 docs/openspec，命令入口与工作边界见 [AGENTS 的 OpenSpec 约定](../AGENTS.md#openspec)。

## 构建与操作
- [项目构建运行](../README.md#快速开始)、[Python 工具](../scripts/README.md)。
- [Tracy / RenderDoc 操作](../scripts/README.md#profiling-build)：独立场景、CPU 时间线与双现代后端捕获；旧 benchmark/性能协议已退役，历史结果仍从验证索引恢复。
- [第三方来源与通知](third-party.md)：核对实际依赖、通知安装与尚未确定的来源，不代替发行合规/干净机验证。

## 验证与归档
[verification/VERIFICATION.md](verification/VERIFICATION.md) 是唯一权威归档入口及关键节点批准记录；当前方法与场景由其导航，不在这里重复维护成绩、批准或硬件结论。

## 过渡保留
旧 docs/spec、testing、milestones 目录已按此前授权退役，恢复身份从下述 VERIFICATION 导航进入；历史阶段、人工反馈及必要证据保持点时含义，不当当前计划/批准。已授权处置和保留边界从 VERIFICATION 进入，不宣称全库无残留。个人 development-journal、Obsidian 配置和冻结字节不调整。
