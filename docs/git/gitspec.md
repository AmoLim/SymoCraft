---
tags:
  - area/meta
---

# Agent 技能目录忽略规则修正批次

2026-10-19 本轮明确授权将忽略规则中的 `/.agent/` 改为 `/.agents/`，取消实际技能目录的跟踪，并创建一次本地提交。不推送、不重写历史、不删除本地文件。

## 完整意图与范围

修正此前单复数目录不一致：忽略根 `.agents/`，从索引移除目前已跟踪的七项：

- `.agents/skills/.openspec-target`
- `.agents/skills/openspec-apply-change/SKILL.md`
- `.agents/skills/openspec-archive-change/SKILL.md`
- `.agents/skills/openspec-explore/SKILL.md`
- `.agents/skills/openspec-propose/SKILL.md`
- `.agents/skills/openspec-sync-specs/SKILL.md`
- `.agents/skills/openspec-update-change/SKILL.md`

准确提交集合为七项索引删除、`.gitignore` 与本页，共九项。只改变版本跟踪，不移动或改写技能正文；本地 OpenSpec 技能及更新入口保留。新克隆不再携带这些文件，所需技能须由本地环境准备。本轮明确目录排除授权不扩展至其他必需输入。

不纳入架构、T3/T4/T5、OpenSpec config、scope、验证索引、个人日志或冻结证据等并发改动。不沿用此前批次授权，不执行 OpenSpec update、实施、sync 或 archive。

## 检查与提交约定

记录七项文件本地 SHA256，取消跟踪及提交后核对文件存在且字节未变；验证忽略匹配、准确暂存路径、完整暂存差异及空白检查。提交后检查索引无剩余与其他工作区改动保留。纯 Git 跟踪调整，不运行构建或测试，云端当前分支需另行推送后才反映变化，历史提交仍保留原文件。

提交消息：`chore(git): 修正agents忽略规则并取消技能目录跟踪`。仅创建一次本地提交，不为回填自身 hash 追加提交。
