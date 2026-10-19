---
tags:
  - area/architecture
---

# Platform

`symocraft_platform` 拥有 SDL3 窗口、按需 GL context、窗口事件/键鼠事实和进程内存查询，不拥有游戏角色、World 或 Renderer 资源。公开行为唯一正文为 [platform spec](../openspec/specs/platform/spec.md)，本文维护正确性机制与未覆盖限制。M3-T2 已于 2026-10-09 获用户批准，依据只在 [VERIFICATION](../verification/VERIFICATION.md)；旧笔记中的“待 T2 验收”是点时文字，不是当前状态，也不关闭 Fix1、同源 Q06 或首次可操作时间缺口。

## 资源所有权与线程

当前生产静态库私有链接 SDL3 3.4.18 与 Psapi，不再以 GLFW 创建游戏窗口。公开 Window/input/process-memory 头无 SDL、Win32、GL、Vulkan SDK；renderer 与显式探针通过 CMake 允许的私有 bridge 访问图形入口，不形成普通消费者 include 权限。

以下T4机制的[生产实现](../../game/modules/platform/src/window.cpp)已于2026-10-19按后续源码同步授权进入F主工作区，不是Git历史合并。不可复制/移动的Platform::Runtime取得一份SDL video责任，覆盖Window；公开Create必须借活Runtime并返回unique_ptr。Window独占Impl/native/context/InputState/首输入故障；局部owner覆盖创建失败，不发布半对象。OpenGL/Native/Vulkan模式创建前确定，不热切换。

TU静态只保单owner/活窗口计数及SDL计时簿记，不承载玩法。第二Runtime/Window拒绝，旧Init/Free为私有；Runtime Shutdown在主线程拒绝活Window，只QuitSubSystem自己一次video引用，可重复，不全局SDL_Quit。计时频率失败归还已取得引用，无后台窗口线程/并发保证；Runtime必须覆盖Window，析构故障诊断不能替代正确借用顺序。

销毁顺序是外部 GPU 工作/Renderer/借用 surface 结束后，再 Window 的 context → NonRudeHWND 属性 → native window → 输入/像素/活窗口计数，最后 video。Destroy 在主线程且 video 有效的前提下，即使 context/属性清理报错也继续余下回收，保留首故障、具名记录次级故障，完成回收后重抛首故障；再次 Destroy 是空操作。noexcept 析构捕获并输出故障，是兜底而非 checked 清理成功证据。错误线程或 video 已释放等前提失败会在回收开始前拒绝，不能概括为任意调用都能清干净。

依据：[公开头](../../game/modules/platform/include/symocraft/platform/window.h)、[实现](../../game/modules/platform/src/window.cpp)、[目标](../../game/modules/platform/CMakeLists.txt)、[主程序清理](../../game/app/src/main.cpp)。

## 创建、尺寸与图形交接

非正创建尺寸使用主显示器一半及800/600下限，SetSize只接受正逻辑尺寸。Width()/Height()只读返回私有实际像素，SDL_GetWindowSizeInPixels在创建/事件/SetSize更新；请求尺寸不能伪造实际extent。GetAspectRatio对零尺寸拒绝，不以max(1)掩盖不可绘制状态。

创建先隐藏、配置完成后显示。GL 要求并核对实际 OpenGL 4.6 Core、4x MSAA 和 Debug context，不静默降级；GL 全局 attributes/hints 的变化没有事务回滚。Benchmark 是普通无边框窗口，非 exclusive/topmost，以 NonRudeHWND 避免覆盖任务栏，并核对实际像素与请求工作负载完全相等。SetCursorMode 的相对模式、可见性、Reset 是连续操作，后项失败不会自动回滚已经完成的原生状态。

三条私有桥接各保留自己的寿命：

- GraphicsBridge 只用于 OpenGL：MakeCurrent 核对 window/context 一致；GetProcedure 可合法返回空入口；ExtensionSupported 的不支持为 false，查询错误则抛错，不混同两者；SetVsync 读取实际 swap interval 核对，不能据此推定物理显示延迟；Present 要求此窗口/context 当前，交换失败即停止正常提交。
- NativeBridge返回经校验的借用HWND，至Window::Destroy失效，不能DestroyWindow；D3D12默认路径实际创建Native Window并消费它，platform不拥有swapchain/device。GL保留显式过渡路径。
- VulkanBridge限定Vulkan模式，SDL负责窗口loader，required extensions返回自有string vector。instance/device/surface归caller，同loader/匹配allocator、窗口有效期间先结束在途GPU再销毁。空surface销毁无操作，不清caller变量或保证重复非空销毁；不额外UnloadLibrary。T4 E目录已整合上游批准的Vulkan后端并由GameRenderer消费此桥，F仍不含T4生命周期改动；不以桥接测试代替整合/运行依据或用户节点批准。

依据：[GL bridge](../../game/modules/platform/src/graphics_bridge/graphics_bridge.h)、[Native bridge](../../game/modules/platform/src/graphics_bridge/native_bridge.h)、[Vulkan bridge](../../game/modules/platform/src/graphics_bridge/vulkan_bridge.h)、[D3D12 消费者](../../game/modules/renderer/src/backend/d3d12/d3d12_backend.cpp)。

## 输入归约与自有交换

InputState按窗口ID过滤，接受全局Quit，只转换平台事实，不访问simulation/renderer或按benchmark筛玩家控制。11键固定物理scancode；InputFrame分别自有held/pressed，Down纯读OR无副作用，CaptureKeys一次消费内部pressed。repeat不造新短按，鼠标按钮保持held。未来复杂键盘顺序reducer仅方向，不增加空公开接口。

相对 motion 保留 double dx、反转 SDL y 为向上正；wheel 保留小数并归一 flipped 方向。窗口激活/恢复/Reset 后抑制首个 motion，再按事件顺序存储，不合并成总量，保持 simulation 的逐事件 pitch/FOV 裁剪。published/pending 两个 vector 各预留 64，Publish clear/swap 复用；预留不是事件无界或全程零分配保证。

PollInt 排空 SDL 队列，经同一 Handle 适配，再按真实窗口 flags 收敛焦点/最小化并 Publish；Capture 在 app 的 simulation_ms 边界内同步 scancode/鼠标物理态后消费键锁存。只在 focused 且非 minimized 时采纳全局物理按键；丢失 regain/restore 事件可由 flags 修复，失焦后同轮恢复仍保留一次 focus_lost。Reset 清已发布/待发布控制、短按、指针及首 motion 基线；活跃窗口仍已知的 held 可保留/在下一 Capture 重同步，Reset 不是永久丢弃物理 held。

公开借用Input()/InputSnapshot已退出。CaptureInput要求caller目的帧已Clear，先同步物理态，再复制键/按钮/focus事实、交换published pointer vector；不交换pending，不让容量搬移变成事件。app所有消费者结束后才Clear，旧自有帧不随Poll/Reset/Destroy改变。Down要求合法枚举；KeySampling纯CPU helper仍保留，不参与SDL硬件采集。

适配vector等异常保存首个嵌套原因并停止Poll/Wait，后续Poll/Wait/Capture/Reset重抛，不能伪成功空帧或Reset恢复。已取事件/已改内部状态不回滚，先前自有帧仍可读但不是新采集成功。SDL查询失败具名传播，非每类SDL故障都进入input_error；全局持续OOM不保证完整嵌套文字。

依据：[InputState](../../game/modules/platform/src/input_state.h)、[KeySampling](../../game/modules/platform/include/symocraft/platform/key_snapshot.h)。

## Wait、焦点与有限诊断

WaitEvents 接受有限非负秒数，ceil 转毫秒并封顶 INT_MAX；等待取出的第一个事件经 Handle 恰好一次，再排空队列、收敛 flags，不 Publish，随后 Poll 才发布待输入。超时不是事件错误，真正 SDL 故障抛出；不能只等待而丢弃首个 close/focus/resize 事件。

普通无限游戏失焦/最小化/零像素时由app清输入并Pause Session，清水平/待跳/该会话预算，有限Wait不忙绘；Resume首次成功Advance规则dt=0。有限frame-limit仅豁免失焦暂停，不豁免零尺寸/最小化。关闭在模拟/绘制前退出；Window::Time是当前Runtime起点计时，不是持久应用时钟。

Benchmark输入屏蔽属于app Scenario策略，平台交出本窗口全部事实。本窗口已排队Escape跨失焦保留，但后台全局Escape不采纳，最小化清除；普通玩法仍按focus_lost丢旧操作。尺寸变化/最小化判invalid，allow-unfocused不豁免；Poll归event_ms、Capture仍归simulation_ms，不挪边界制造收益。

## 内存查询、不变量与验证边界

SampleProcessMemory 返回自有 optional working-set/private bytes；GetProcessMemoryInfo 失败时两项保持缺失，不填 0，不依赖 Window/GPU。设备级 GPU 内存估计在 renderer，不是平台的进程 VRAM。主线程窗口约束不等于所有纯值/进程查询 API 都必须拥有窗口。

| 原 Window 编号 | 当前成立边界 |
| --- | --- |
| I1 | native 有效期 Impl 存活；Create 局部 owner 不发布半初始化对象。 |
| I2 | 有效清理前提下 Destroy 继续释放、清空 context/native/input/像素并可重复，完成后报告首错。 |
| I3 | 一次Capture消费短按，指针保序交换复用；caller自有帧独立于后续平台状态。 |
| I4 | 事件只生成平台事实，公开 API 不泄漏 SDK，不含游戏状态 callback。 |
| I5 | 主线程单窗口，video 覆盖 Window；GPU/surface 先于窗口/context 释放。 |

当前测试为key_snapshot、owned_frame和显式真实SDL runtime_probe，另对Window工厂实际C++分配序列扫查。InputFrame/pending/app pointer缓冲用可抛的零count构造，避开MSVC Debug vector默认noexcept构造分配proxy时的terminate；不改vector布局/allocator或推广为整个标准库OOM保证。采集同步失败也锁存。范围/成绩见[T4 verification](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/verification.md#2026-10-15-完整生产迁移)。test/experimental保留旧API历史材料，未冒称可直接重放新API或改历史命令/结果。合成事件和真实Native窗口不证明物理键盘布局、全部DPI/任务栏/驱动、人工体验或长期资源稳定。
