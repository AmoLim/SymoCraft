---
tags:
  - area/spec
---

# Platform

## Purpose

维护 SDL3 迁移后已经存在的单窗口/输入/三种私有图形交接能力，不发布 T4 实例化目标或生产 Vulkan renderer。依据[公共头](../../../../game/modules/platform/include/symocraft/platform/window.h)、[窗口实现](../../../../game/modules/platform/src/window.cpp)、[输入 reducer](../../../../game/modules/platform/src/input_state.h)、[应用循环](../../../../game/app/src/application.cpp)及 [T2 S4 历史交接](../../../milestones/m3-t2/s4-platform-handoff.md)。原 [T2 来源](../../../spec/M3/M3-T2-SDL3迁移.md)只供复核；本轮无构建/硬件输入/GPU 成绩。

## Requirements

### Requirement: Platform lifecycle and graphics handoff

platform SHALL 在 SDL 主线程串行管理当前静态 Init/Free 和单个活 Window；成功 Init 取得本模块一次 video 责任，重复 Init 不增持有次数，Free 在活窗口存在时拒绝并只释放本模块责任。Window SHALL 唯一拥有 PImpl/系统窗口，禁止复制且不提供移动；完整创建才发布，部分取得资源失败时清理。创建前 SHALL 选择 OpenGL（默认）/Native/Vulkan，不支持运行中变更或第二窗口。renderer/app SHALL 在 Destroy/Free 前结束全部窗口借用与图形资源。

#### Scenario: M3-T2-A07 A08 lifecycle and mode rejection

- **WHEN** 未 Init、非主线程、第二窗口、非法模式或已销毁窗口被用于受检查操作
- **THEN** SHALL 具名拒绝；部分创建失败回收已取得资源，重复 Destroy 为空操作，不把 noexcept 析构日志当正常清理通过

#### Scenario: M3-T2-A02 A08 borrowed graphics access

- **WHEN** GL MakeCurrent/Present、HWND 或 Vulkan 交接被消费
- **THEN** GL 核对正确模式/当前 context 与窗口；HWND 不跨 Destroy 保存；Vulkan 按原 loader 来源和有效寿命使用，调用方保证 instance/surface/allocator 配对和在途责任
- **AND** OpenGL SHALL 要求并核实实际 4.6 Core/4x MSAA 与 Debug 所需 context，不静默降低条件；GL 资源先于 context/window 释放。Native 桥 SHALL 返回实际有效 HWND 借用，不取得销毁权。Vulkan 桥 SHALL 使用 SDL 窗口关联 loader、返回自有必要扩展字符串、辅助真实 surface 创建/销毁；instance/device/surface 由调用方拥有且在 instance/window/loader 终结前结束相关调用与对象。桥接成功不代表完整 Vulkan 或 D3D12 渲染能力。

### Requirement: Physical input snapshot and ordered pointer events

11 个当前控制 SHALL 按物理 scancode 位置映射 W/A/S/D、Space、LeftShift/LeftControl/CapsLock、E/Q/Escape，不按文本/keycode 替代。非 repeat 短按 SHALL 锁存至一次 Capture，即同轮按放也不丢；held 下轮继续为真，消费锁存不能被玩法分支短路。鼠标按钮是 held 采样而非 sticky click。PointerEvent SHALL 保留事件次序，x 向右为正、y 向上为正（xrel/-yrel），滚轮保留浮点和翻转方向，不预先相加改变相机截断结果。

`Input()` SHALL 明确是 Window 自有的可变快照引用，不是历史冻结副本；Poll/Wait 收敛/Capture/Reset/模式切换可改变状态，容器元素不得跨这些操作借用，延后消费 SHALL 显式复制项目值。输入扩容失败 SHALL 锁存并停止本帧有效发布，后续受检查入口重抛，不继续消费半轮数据。

#### Scenario: M3-T2-A03 physical short tap

- **WHEN** 一个受支持物理键在一次 Capture 前完成 down/up，或 held/repeat 持续
- **THEN** 短按至少在下一次 Capture 见真并只消费一次；held 依实际状态保持，repeat 不另造短按

#### Scenario: M3-T2-A03 A04 ordered pointer and reset

- **WHEN** 连续运动/滚轮交替，或 Reset/Normal-Hidden-Lock 切换
- **THEN** 保序应用现有符号/灵敏度和缩放策略；清旧锁存与运动、建立首运动基线，获焦窗口后续 Capture 可重新采仍实际按住的控制，不承诺新 Raw Input 校准

### Requirement: Focus recovery and event delivery

Poll SHALL 排空事件、收敛真实焦点/最小化事实并发布指针与一次性 focus-loss。Capture SHALL 同步 held/消费短按，不再排队或重复发布指针。Wait SHALL 将首个取出事件恰好交同一适配器一次并排空剩余，不替代下一 Poll/Capture；非法 timeout 拒绝。全局 quit/本窗口 close SHALL 形成关闭请求，不在事件处理器做 GPU/ECS 操作。

普通交互玩法失焦暂停/最小化/零实际像素时 SHALL 清旧控制与累计运动、不继续绘制，在恢复帧以 dt=0 重新开始，而非积压暂停时间。有限帧诊断的失焦例外不授权零尺寸绘制。公开 width/height SHALL 是查询的实际 drawable 像素，不是请求逻辑尺寸；尺寸事件只更新事实，renderer 在帧边界处理。

#### Scenario: M3-T2-A04 A05 wake and drawable recovery

- **WHEN** Wait 被关闭/输入事件唤醒，或普通窗口最小化后恢复
- **THEN** 首事件不丢失且不双消费；不可绘制期间不提交图形，恢复不重复旧输入、首帧 dt=0，并使用最新实际像素

### Requirement: Benchmark window isolation

Benchmark SHALL 保持普通无边框、非独占、非置顶和 NonRudeHWND 策略；要求实际 1920×1080，不为成绩静默降负载。只允许本窗口 Escape/关闭影响退出，不用玩家键鼠改变脚本 workload；已排队 Escape 跨同轮失焦保留，最小化走无效/重置路径。strict/allow-unfocused SHALL 保持既有协议；允许失焦不豁免最小化/尺寸变化无效条件。event_ms 为 Poll 成本、Capture 在 simulation_ms 内，不通过挪计时点制造性能收益。

#### Scenario: M3-T2-A06 workload and cancellation

- **WHEN** Benchmark 接收玩家控制、失焦、最小化或实际尺寸不符
- **THEN** 玩家输入不改变脚本；焦点按所选策略记账，最小化/尺寸变更无效；缺合法终态的轮次不作为成绩

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
