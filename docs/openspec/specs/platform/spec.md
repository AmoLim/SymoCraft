---
tags:
  - area/spec
---

# Platform

> 2026-10-19：T4已验收并同步本页要求；Purpose保留原编写时点。交付范围与保留风险见[T4记录](../../changes/archive/2026-10-19-m3-t4-application-simulation/verification.md#2026-10-19-t4整体完成批准)。后续用户明确授权后，T4生产代码及必要构建、测试配套已同步F主工作区；不是Git提交/合并或T5实施授权。

## Purpose

维护 SDL3 迁移后已经存在的单窗口/输入/三种私有图形交接能力，不发布 T4 实例化目标或生产 Vulkan renderer。依据[公共头](../../../../game/modules/platform/include/symocraft/platform/window.h)、[窗口实现](../../../../game/modules/platform/src/window.cpp)、[输入 reducer](../../../../game/modules/platform/src/input_state.h)、[应用循环](../../../../game/app/src/application.cpp)及 [T2 S4 历史交接](spec.md)。旧T2正文可由全局退役材料恢复导航读取；本轮无构建/硬件输入/GPU 成绩。

## Requirements

### Requirement: Platform lifecycle and graphics handoff

platform SHALL 在 SDL主线程串行以一个活动 Runtime及单个活 Window管理资源；Runtime只释放自身取得的 video初始化责任，不终止外部SDL owner，有活窗口时拒绝先结束。Window SHALL 明确唯一拥有 PImpl/系统窗口并借用Runtime，禁止复制/移动，完整创建才发布、部分失败清理；公开尺寸改为受控实际drawable值，不暴露原生资源。创建前 SHALL 选择 OpenGL（默认）/Native/Vulkan，不支持热变更或第二窗口。renderer/app SHALL 在 Window/Runtime关闭前结束全部窗口借用与图形资源。

OpenGL SHALL 要求并核实实际4.6 Core/4x MSAA与Debug所需context，不静默降低条件，GL资源先于context/window释放。Native桥 SHALL 返回实际有效HWND借用、不取得销毁权。Vulkan桥 SHALL 使用SDL窗口关联loader、返回自有扩展字符串并辅助真实surface创建/销毁；调用方拥有instance/device/surface及配对allocator/在途责任，相关调用/对象先于instance/window/loader终结。桥成功不是完整现代renderer能力。

#### Scenario: M3-T2-A07 A08 lifecycle and mode rejection
- **WHEN** 未建立Runtime、非主线程、第二窗口、非法模式或已销毁窗口被用于受检查操作
- **THEN** SHALL 具名拒绝，部分创建回收已取得资源，重复显式关闭按幂等事实处理；noexcept析构日志不当正常清理通过

#### Scenario: M3-T2-A02 A08 borrowed graphics access
- **WHEN** GL MakeCurrent/Present、HWND或Vulkan交接被消费
- **THEN** GL核对模式/context/窗口，HWND不跨销毁保存；Vulkan按原loader来源与有效寿命使用，调用方保证instance/surface/allocator及在途责任

#### Scenario: M3-T4-A04 A05 controlled owners
- **WHEN** 部分装配失败、显式重复关闭或关闭仍有活窗口的Runtime
- **THEN** 已取得对象立即有owner，Renderer先Window、Window先Runtime；失败报告真实阶段，不把未取得/未知状态当已安全回收，不变更后端quarantine保证

### Requirement: Physical input snapshot and ordered pointer events

11个控制 SHALL 按物理scancode位置映射现有键，不按文本替代；非repeat短按锁存至一次交接，同轮按放不丢、held按当前事实持续，锁存消费不被分支短路。鼠标按钮 SHALL 是held而非sticky。PointerEvent SHALL 保序且x右正/y上正、浮点滚轮方向原样，不能相加改变裁剪。

app SHALL 提供清空的自有InputFrame，平台在原Capture边界完成键盘事实采样后交换已发布事件容器并填事实，不保存调用方对象地址；pending事件独立，交换不清待发布事件。所有消费者结束后才清记录内容保容量；记录不借用Window，Reset/Destroy不改已交接对象，历史须另留独立记录，显式复用同对象后不再保持旧内容。输入扩容/采集失败 SHALL 锁存并停止有效发布、具名报告，不用半轮或空输入伪成功、不承诺零分配。Q02方案1不改变上述消费保证。

#### Scenario: M3-T4-A06 keyboard fact queries
- **WHEN** 平台向自有InputFrame交接当前键盘事实，或多个消费者查询同一记录
- **THEN** SHALL 分开表达当前held与本窗口非repeat短按pressed；Down为只读held/pressed合并，重复查询不消费锁存，锁存只在既定交接边界消费一次；app集中选择普通/场景意图，不因字段拆分改变既有玩法Down触发或直接将平台记录交给simulation

#### Scenario: M3-T2-A03 physical short tap
- **WHEN** 受支持物理键在一次交接前down/up或held/repeat持续
- **THEN** 短按下一交接见真且只消费一次，held依事实，repeat不造新短按

#### Scenario: M3-T2-A03 A04 ordered pointer and reset
- **WHEN** 连续运动/滚轮交替或Reset/Normal-Hidden-Lock切换
- **THEN** 保序应用原符号/灵敏度/缩放，清旧锁存和运动建立首运动基线；获焦后采当前held，不承诺Raw Input校准

#### Scenario: M3-T4-A06 owned handoff and pending events
- **WHEN** 消费后清记录、Wait累积后下一正常发布，或平台Reset/窗口销毁且旧记录仍持有
- **THEN** 全消费者完成后才复用，pending/Wait首事件不丢或双消费，旧自有记录不被改写；多物理子步不重复输入，事件与汇总不双用

### Requirement: Focus recovery and event delivery

Poll SHALL 排空事件、收敛真实焦点/最小化并发布指针及一次性失焦事实；Capture/自有交接 SHALL 同步held并消费短按，不重新排队/重复发布指针。Wait SHALL 将首事件恰好交同一适配器一次并排空剩余，仅累积不替代下一正常发布/采样；非法timeout拒绝。全局quit/本窗口close SHALL 成关闭请求，不在事件处理器做GPU/ECS操作。

普通交互玩法失焦暂停/最小化/零实际像素 SHALL 清旧控制/运动、停止绘制，由app显式Pause/Resume并在首次成功恢复Advance以dt=0重新开始，不积压暂停时间。有限帧诊断失焦例外 SHALL 保留但不授权零尺寸绘制。同轮失焦再获焦 SHALL 清理一次，不仅看最终focused。受控尺寸值 SHALL 为实际drawable像素而非逻辑请求，尺寸事件只记事实、renderer在帧边界处理。

#### Scenario: M3-T2-A04 A05 wake and drawable recovery
- **WHEN** Wait被关闭/输入唤醒或普通窗口最小化后恢复
- **THEN** 首事件不丢/不双消费，不可绘制不提交GPU，恢复无旧输入/首成功dt0且用最新实际像素

#### Scenario: M3-T4-A09 same poll focus recovery
- **WHEN** 同轮失焦再获焦或恢复后当前held仍按住
- **THEN** app仍清旧记录/锁存/运动并重置会话约定状态，再采当前held，保首运动保护，不要求先放开再按或重放旧短按

### Requirement: Benchmark window isolation

Benchmark SHALL 保持普通无边框/非独占/非置顶/NonRudeHWND、实际1920×1080及原strict/allow-unfocused协议，不静默降负载。D05 SHALL 由app依据平台原始事实隔离玩家键鼠并选择场景，不能以Performance::Session是否存在决定。只允许本窗口Escape/关闭影响退出；已排队未消费Escape跨失焦保留、不补采后台全局键盘；最小化/尺寸变更沿既有无效/重置路径，allow-unfocused不豁免，不直接套普通暂停。event_ms SHALL 保持Poll成本，Capture在simulation_ms内。

#### Scenario: M3-T2-A06 workload and cancellation
- **WHEN** Benchmark收到玩家控制、失焦、最小化或尺寸不符
- **THEN** 玩家输入不改脚本，焦点按所选策略记账，最小化/尺寸变更无效；缺合法终态不作成绩

#### Scenario: M3-T4-A20 fact completeness and cancellation
- **WHEN** Escape已排队后失焦/恢复、同轮失焦再获焦或无完整观测Session运行场景
- **THEN** app仍可按窗口局部事实保留取消及场景动作，不依靠全局键或只删benchmark_筛选；计时和T2有限帧诊断例外保留，不授权零尺寸绘制
### Requirement: Platform SDK and deployment boundary

SDL SHALL 固定本地 release-3.4.18、静态构建并只由 platform 私有消费，普通 main 与私有 SDL_MAIN_HANDLED/SetMainReady 不传播到 app。公共 Window SHALL 无 SDL/GLAD/Win32/D3D/Vulkan 类型或裸 `void*` 逃生口；renderer 仅消费授权三份私有桥，不取得整个 platform/src。CPU-only 与独立 benchmark SHALL 不配置/链接 SDL 或 GPU SDK；普通 GL/Native 路径不强制外部 Vulkan SDK 或 loader。静态 SDL 不代表静态 CRT/Vulkan loader，不新增 SDL3.dll 部署。

#### Scenario: M3-T2-A01 A09 build and deployment

- **WHEN** 构建公开消费者、CPU-only、独立工具或部署游戏
- **THEN** 头和 target 不传播 SDL/SDK；所需运行条件、CRT/资源和干净机器证明仍分开，不由安装成功推定无开发环境运行通过

## Rationale And Proof Boundary

T2 D02/D03 选择窄 Window 适配及三模式，而不是提前完成 Runtime/Window/Application 对象化或新 renderer；D04/D05 选择固定本地 SDL 和静态部署以保持离线可复现及依赖隔离；D06 保持物理位置；D07 保留既有鼠标缩放/符号，不重选 Raw Input；D08 使用实际像素而不默认强制高 DPI，避免改变负载；D11 移除活动 GLFW但保留退休原件。当前生命周期机制与边界例外见[工程级契约](../../../architecture/engineering.md#图形与第三方边界)，不复制旧学习版 API 表。

T2 节点批准只在[验证索引](../../../verification/VERIFICATION.md#关键节点)记账。15 分钟人工/完整 DPI 条件、正式同源 Q06、首次可操作预算及 Win32 1175/Fix1 未关闭；S4 对恢复 dt=0 仅有源码核对，Benchmark 真 resize 为 partial/unavailable，Vulkan loader 实际 OS 卸载没有实测断言。FB01/MB01 的已选评价条件保留于下节，[项目范围](../../../scope/project-scope.md)维护其范围及设备门槛，不迁成已测性能通过。当前 scope 的 RTX 5070 Ti 功能范围不替代下一玩法前中低端整机门槛。

## Retained Measurement And Acceptance Conditions

以下是仍有效的检查身份/已选评价条件，不是待重新实施 SDL3 的任务，不从批准反推未验证通过。M3-T2-D01 的插入顺序、D09 当前仅本机 RTX 5070 Ti、D10/B 延后正式长时对照、D12/A 采样前定预算保持；已选 P06/Q01-Q08 不因换格式重开。S1/S2/S4 技术交付、S3/S5 局部结果与缺口按原身份保存。

M3-T2-A01 构建/头边界、A02 真 GL、A03 真键鼠、A04 焦点/暂停、A05 尺寸/显示器、A06 Benchmark 隔离、A07 寿命/失败、A08 Native/Vulkan 桥、A09 安装、A10 本机人工玩法、A11 协议/短采样、A12 预算/活动依赖保留原义。A05 的 100%/150%/200% 缩放及可用跨屏、A10 的实际 GL 设备与进入世界后至少 15 分钟、A09 的中文/空格路径和缺运行依赖清楚退出均需具体条件依据，有限帧/合成输入/本机系统补足 CRT 不补造这些结果。旧视角/树叶问题不属本轮必修；遗留不重新阻断已批准节点。

Q06 仍为每版 static/walk/edit 各 3 独立进程、10 秒预热+30 秒采样，真实同源码/MSVC/资源/adapter 条件，seed 424242、441 块、regression v1/workload v2、实际 1920×1080/4x MSAA/VSync-off 请求、allow-unfocused；除窗口适配不混入其他重构。保留原始 CPU/GPU/present/帧数据、实际 GL/驱动、供电模式、焦点/尺寸和缺指标原因。003 只是旧 GLFW 单端探索依据，002 已废弃；不是现 SDL 同源 A/B、正式长时协议、性能等价或 M4 通过。

已选 M3-T2-FB01/方案2：P95 <= 10 ms、P99 <= 12 ms。已选 MB01/方案2：working set <= 704 MiB、private bytes <= 1408 MiB，为用户确认绝对值，不重算为 003 的百分比。两者只评价正式采样帧全程 focused 的完整轮次，三场景各三轮逐轮同时检查两项，不混池/平均/挑最好。失焦轮可以协议有效，但不符合前台预算条件；保留整轮和全部慢帧，不删失焦帧重算、不偷改 strict，额外采样另确认。

MB01 取每轮 `10 <= elapsed_s < 40` 约每秒观察的两指标各自最大值，不相加/取平均/分位数；working set 为驻留工作集，private bytes 为私有提交，均不是 VRAM、瞬时/全过程峰值、加载/截图/导出范围或无泄漏证明。frame_ms 为相邻交换返回间隔，不是显示延迟，CPU/GPU/present 分位数不相加。首次可操作预算仍未选。

超限按 Q06 仅追加一组同参数复测并保留初次超限；复测仍超限未通过，即使复测全部合格也不隐去首次异常。不可恢复故障/协议不符/输入丢失/泄漏/降低画面不由预算放宽；失败轮不作为成绩、不自动无限补跑。正式同源 Q06/A11 仍未完成，这些既定规则不授权当前新采样或恢复 Fix1。
