---
tags:
  - area/spec
---

# T4 Platform Delta

本页维护目标验收契约，E候选落地/验证范围见[verification](../../verification.md#2026-10-15-完整生产迁移)，不以docs同步推定F代码合并或现役spec同步。未改动[现役SDK与部署](../../../../../specs/platform/spec.md)及下列MODIFIED全部继续有效场景。Q02当前方案1，向3演进仅方向，不提前实施事件系统；机制见[design](../../design.md)。

## MODIFIED Requirements

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
