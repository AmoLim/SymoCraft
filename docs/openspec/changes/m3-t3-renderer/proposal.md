---
tags:
  - area/spec
---

# M3-T3 Renderer 接管

## Why

将已批准的 D3D12 先行、随后 Vulkan 的接管意图与未关闭验收迁入一份活动 change，避免旧流水账把实验、设计冻结、候选交付和整体验收混为一谈。
依据为 [项目范围](../../../scope/project-scope.md)、[现役 renderer](../../specs/renderer/spec.md)、[renderer 机制](../../../architecture/renderer.md) 与 [验证/批准索引](../../../verification/VERIFICATION.md)。
已预读含 scenarios 的现役六能力：[renderer](../../specs/renderer/spec.md)、[module-boundaries](../../specs/module-boundaries/spec.md)、[world](../../specs/world/spec.md)、[platform](../../specs/platform/spec.md)、[simulation](../../specs/simulation/spec.md)、[foundation-files](../../specs/foundation-files/spec.md)。最小覆盖不证明其他行为不存在。
[文档迁移](../archive/2026-10-11-docs-openspec-migration/design.md)、[暂停 Fix1](../m3-t2-publication-fix1/proposal.md)及[T4](../m3-t4-application-simulation/proposal.md)未与 T3 交接冲突；T4 生命周期目标、文件发布缺口及其他任务仍由各自 change 管理。

## What Changes

2026-10-17用户明确批准仅前台采样启用的有界获焦准备阶段、基本测试及短启动验证后执行原27轮。已独立预读现役六能力/scenarios及相关T4/Fix1交接；行为修改限app私有启动参数与采样起点之前的准备策略，复用[platform事件/焦点契约](../../specs/platform/spec.md)与本renderer接管验收capability，不改公开v1、玩法/allow-unfocused默认或渲染算法。准备成功后仍完整预热，计时后strict语义及预算不变；不启动T4生命周期迁移、文件可靠性修复或GL退役，文档同步主目录。

2026-10-14用户授权在refactor-renderer托管worktree继续R4生产实现；独立重读现役六能力及相关T4的Window/Renderer寿命目标，无公开v1签名或T4/Fix1范围交接变化。按[明确批准的呈现退休能力](decisions.md#2026-10-14-vulkan有限呈现退休能力)提高Vulkan能力下限为1.2＋surface/swapchain＋KHR或EXT swapchain_maintenance1及feature；这是renderer后端可用性行为修改，复用本capability，不发布为现役保证或接管通过。默认D3D12/GL保留，性能缺陷继续暂缓，文档及时同步主目录。

保留 R1–R4 及 R1-E01–E03、R-I1–R-I6、A01–A19、T3-M01–M15 的原义，按现役行为与接管目标分开表达。当前已实现 CPU PImpl、私有 D3D12、统一游戏转接/持久缓存及既有候选；2026-10-12 用户确认人工通过、批准 R3 并授权默认切换。逐项未验证范围仍保留，Vulkan与双现代T3验收未完成。

继续由稳定游戏侧 Renderer v1 接收 CPU 场景输入，落实现代后端的真实设备/资源/同步/呈现/诊断；D3D12 接管获批后才能切默认，再实施与独立验证 Vulkan，双现代均完成才申请 T3 通过。GL 只保留必要过渡回归，最终退役另获授权。

不新增 D3D11、热切换、插件 ABI、渲染线程、通用 RHI/Render Graph、光追、新透明排序、流式世界或 micro voxel 实体外表。2026-10-10 创建本 change 时只迁移文档，不执行实现、sync/archive、GPU、人工验收或 Git 操作。
2026-10-12 用户通过 apply 授权继续收敛 R2 缺口：落实构建组合、原生消费者及已选受控故障路径，视觉参考及已选 D3D12 帧时间/内存预算见 [decisions](decisions.md#正式视觉参考与性能预算)；不恢复暂停 Fix1，不取得 R3/默认切换/Vulkan或 Git/sync/archive授权。复用上述六能力路径，未新增独立 capability；T4 的封闭/会话迁移及 Fix1 的文件根因仍不在本轮范围。
同日用户确认将预算口径同步本 change：细化 `renderer` 接管验收，不改变运行行为或扩展代码范围；本次仅更新活动规格/计划，不启动正式采样，不新增启动时间等其他硬预算，也不将预算选择视为通过。
后续用户明确取消本轮正式九轮，改为 5 分钟内短采样比较并收口 R2 工作、文档保留风险；[追加决定](decisions.md#本轮-r2-短采样收口与风险接受边界)只调整本轮验证执行范围，不修改游戏行为或预算数值，不将未测目标同步为现役保证。

## Capabilities

- Modified `renderer`：复用现役公开行为，delta 完整修改后端可用性 requirement，增加逐后端实验、A01–A19 接管门槛及双现代完成边界；本轮只将实际默认选择更新到现役spec，不发布Vulkan或把未测目标整组同步。
- 现役 `renderer` 基线是经静态核对的最小已实现契约，不是由本 change sync 得到，也不宣称 R3 通过。

2026-10-15用户独立确认[Vulkan预算与18轮正式对照](decisions.md#2026-10-15-vulkan已选预算与正式配对口径)并授权本轮采集。已独立重读现役六能力/scenarios及相关T4/Fix1目标，复用renderer验收capability，无游戏运行行为、公开v1或其他change范围变化；仅扩展既有配对采集器支持显式Vulkan和同版本独立GL包，不改统计阈值或生产二进制。预算选择/采集结果与R4/T3批准分开，文档及时同步主目录。

## Impact

2026-10-16用户仅授权共享顶点层扫描缓存摘要的实现及正常路径基本测试，排除边界/回归和重采。已按规则重新盘点/读取现役六能力及相关T4/Fix1交接，无公开v1/行为或其他change范围变化；复用renderer现有CPU Input/接受事务要求，仅修正逐顶点实现措辞，机制见[renderer](../../../architecture/renderer.md#输入所有权与资源身份)、限定结果见[verification](verification.md#2026-10-16-共享纹理层摘要局部优化)。不启动其他优化、不据此批准预算/节点或规格整组sync/archive。

2026-10-13 用户授权在独立refactor-renderer worktree优先补上述两组缺口，并确认 [18分钟正式配对窗口](decisions.md#2026-10-13-正式配对短窗口)。复用已预读六能力及原A04/A13–A18目标，不新增公开游戏行为；必要地将同一app启动/循环/关闭实现归入一个生产STATIC owner供游戏与隔离probe链接，诊断绑定仅为renderer私有创建线程作用域，不扩展公开v1或游戏CLI。Fix1/T4仍保持各自边界，Vulkan未启动。

用户随后认可已完成的人工画面对照足够，按[人工视觉口径](decisions.md#2026-10-13-人工视觉核查口径)收口本轮视觉核查，不强加数值像素方案。人工认可不豁免正式预算超限，实际结果及保留风险只在verification维护，不推定全部技术任务完成。

2026-10-12 本轮复用现役六能力与相关Fix1/T4边界，在用户明确批准R3后实施D3D12默认选择、新游戏配置默认启用及普通构建/候选身份流程隔离；代码范围为app默认、根CMake、打包器及相关测试。不删除GL、不启动Vulkan/长采样，不进行Git/sync/archive；未覆盖项继续具名保留。

后续代码范围为 `game/modules/renderer`、必要 platform 私有桥接、`game/app` 转接、中立 telemetry、根级 `test`、后端 shader/CMake/安装与 `tools/benchmark` 兼容消费者；CPU world/scene 已交付语义保持。
依赖 T0 模块边界、T1 callback-only 网格交接、T2 SDL3/像素/桥接与节点批准；这些能力不因本 proposal 成立而重验通过。R1 冻结的是公开 v1 与 target 设计，不是完整私有 RHI。
当前缺口及唯一计划见 [tasks](tasks.md)，选择及代价见 [decisions](decisions.md)，现行目标与接口依据为 [T3 目标设计](design.md) 和 [现役 renderer 契约](../../specs/renderer/spec.md)，二者不构成额外活动计划。
