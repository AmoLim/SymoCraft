---
tags:
  - area/spec
---

# Development Profiling Delta

## Purpose

定义开发工具工作流及旧性能链退役后的兼容边界；不是工具安装、实际捕获或性能验收通过声明。

## ADDED Requirements

### Requirement: External RenderDoc capture workflow

项目 SHALL 支持通过外部 RenderDoc 启动同一生产游戏，分别选择已编入的 D3D12、Vulkan，捕获并回放真实世界帧。工作流 SHALL 明确可执行文件、工作目录、资产、参数及实际工具/后端/adapter/驱动/配置身份，不依赖源码内 RenderDoc SDK、链接库、内置捕获按钮或全局自动注入。未安装工具 SHALL 不妨碍游戏构建、启动或退出。

#### Scenario: DP-A01 independent modern captures
- **WHEN** 分别用 RenderDoc 启动 D3D12 与 Vulkan 的 static/walk/edit 场景并选择有效绘制帧
- **THEN** 两后端各自能捕获和回放实际网格、纹理层及适用选框，画面与所选场景相符；记录真实配置和失败/未执行，不以另一后端、空帧或仅捕获文件存在替代

#### Scenario: DP-A02 ordinary run without tools
- **WHEN** 未安装 RenderDoc、未启动 Tracy Viewer 或关闭所有 profiler 构建开关
- **THEN** 普通游戏和独立场景不加载/等待这些外部工具，不因缺少工具改变玩法、渲染能力或正常退出

### Requirement: Optional CPU timeline and bounded instrumentation

开启 Tracy 时 SHALL 提供稳定命名的帧边界、真实主要 CPU 阶段和已有物理步/网格重建/draw/上传字节事实，区分增量、累计及单位；CPU Present 等待不命名为 GPU 执行时间。关闭时 SHALL 不评估埋点参数、不建立采集线程/连接/存储。埋点 SHALL 不持有业务对象指针供异步访问、不推进规则、不改变提交顺序，不新增逐体素/逐实体高频区域或无界动态名称集合。

#### Scenario: DP-A03 connected and disconnected CPU profiling
- **WHEN** 启用 Tracy 运行现有工作负载、连接/断开 Viewer 或始终不连接
- **THEN** Viewer 可见实际经过的 CPU 阶段及同一口径的帧标记/计数；无 Viewer 仍能运行和退出，不为补齐轨迹无限等待或改变游戏错误处理，未经过的工作不伪造时间

#### Scenario: DP-A04 observation preserves computation
- **WHEN** 相同生产 CPU 序列分别启用和关闭埋点，或读取轻量快照
- **THEN** World 提交、输入消费、角色/物理步数、返回值所有权与既有数值阈值保持，采集不会增加 Advance、World 查询遍历或第二业务状态权威；独立 physics 核心不含 Tracy 调用

### Requirement: Reproducible profiling identity and interpretation

工具工作流 SHALL 固定 Tracy client 与匹配 Viewer 的来源身份，提供保留 Release 优化且带匹配调试符号的独立构建配置，并区分普通运行、Tracy、RenderDoc 捕获/回放、validation 与 shader 调试设置。新工具输出 SHALL 不自动视为旧 CSV 同口径或正式性能预算通过，不声称完整替代进程内存/显存分析。

#### Scenario: DP-A05 tool evidence versus budget evidence
- **WHEN** 使用工具轨迹定位慢帧或提出性能结论
- **THEN** 结论绑定源码/构建/实际工具/后端/设备/场景/时长和诊断开关；明确采集扰动、指标缺失及比较条件，CPU 时间不当 GPU draw/显示延迟，短诊断不替代未运行的正式预算或其他机器验证

### Requirement: Retired performance products and immutable history

本轮交付后 SHALL 不再提供活动旧 benchmark GUI/CLI、旧性能采样/导出/报告协议或专属运行脚本；不维持伪兼容壳，也不要求新游戏继续读取历史报告。既有历史证据、命令、失败、冻结包、旧构建缓存和暂停任务现场 MUST 原样保留。仅属于退役链的测试可退役，仍有效的文件、场景、截图、输入和 GPU 寿命契约 SHALL 先迁移覆盖。

退役适用边界 SHALL 由各原能力明确维护：图形样本、截图及部署见 [renderer](../renderer/spec.md)，T4 旧工具/协议验收见 [application-lifecycle](../application-lifecycle/spec.md)，T2 既有测量口径见 [platform](../platform/spec.md)，产品/构建隔离见 [module-boundaries](../module-boundaries/spec.md)；本能力不跨路径覆盖其条款。暂停 Fix1 的目标冲突 SHALL 单列，不能以消费者退役推定文件故障修复或擅改暂停计划。

#### Scenario: DP-A06 exact retirement and retained obligations
- **WHEN** 实施方形成旧链删除清单或核对新的可运行候选
- **THEN** 清单先完整汇报并获其后的明确确认才删除；新候选不产生旧性能查询/文件或安装旧工具，仍有效的测试不靠删除变绿；历史材料保持原字节，未解决发布风险与旧未测范围仍明确

#### Scenario: DP-A07 no automatic historical verification campaign
- **WHEN** 审核本 proposal、工具接入完成或 OpenSpec 显示规划完整
- **THEN** 不自动恢复正式配对采样、Fix1、长时人工/替代自动测试、双机测试、规格同步或 Git 操作；具体新增验证范围与有界人工时长须先确认，未执行如实记录
