---
tags:
  - area/architecture
---

# Platform

`symocraft_platform` 拥有 SDL3 窗口、按需 GL context、窗口事件/键鼠事实和进程内存查询，不拥有游戏角色、World 或 Renderer 资源。公开行为唯一正文为 [platform spec](../openspec/specs/platform/spec.md)，本文维护正确性机制与未覆盖限制。M3-T2 已于 2026-10-09 获用户批准，依据只在 [VERIFICATION](../verification/VERIFICATION.md)；旧笔记中的“待 T2 验收”是点时文字，不是当前状态，也不关闭 Fix1、同源 Q06 或首次可操作时间缺口。

## 资源所有权与线程

当前生产静态库私有链接 SDL3 3.4.18 与 Psapi，不再以 GLFW 创建游戏窗口。公开 Window/input/process-memory 头无 SDL、Win32、GL、Vulkan SDK；renderer 与显式探针通过 CMake 允许的私有 bridge 访问图形入口，不形成普通消费者 include 权限。

Window 独占 `unique_ptr<Impl>`，Impl 拥有 SDL_Window、GL 模式的 SDL_GLContext、InputState 及首个输入适配异常；Window 不可复制、不可移动。Create 返回拥有的裸指针，app 立即接管 unique_ptr；函数内局部 owner 覆盖创建中途失败，不发布半初始化对象。窗口用途在创建前确定为 OpenGL/Native/Vulkan，不在运行中热切换。

TU 静态状态记录本模块 video 初始化责任、一个活窗口、计时起点/频率。Init 主线程取得 SDL video，重复 Init 不再增加该责任；计时频率失败时退回 video。Free 拒绝活窗口，仅 QuitSubSystem(video)，不调用全局 SDL_Quit 或替其他持有者释放责任。主线程/单窗口是调用者约束并在主要 checked 入口检查，没有后台窗口线程、锁或可并发调度保证。

销毁顺序是外部 GPU 工作/Renderer/借用 surface 结束后，再 Window 的 context → NonRudeHWND 属性 → native window → 输入/像素/活窗口计数，最后 video。Destroy 在主线程且 video 有效的前提下，即使 context/属性清理报错也继续余下回收，保留首故障、具名记录次级故障，完成回收后重抛首故障；再次 Destroy 是空操作。noexcept 析构捕获并输出故障，是兜底而非 checked 清理成功证据。错误线程或 video 已释放等前提失败会在回收开始前拒绝，不能概括为任意调用都能清干净。

依据：[公开头](../../game/modules/platform/include/symocraft/platform/window.h)、[实现](../../game/modules/platform/src/window.cpp)、[目标](../../game/modules/platform/CMakeLists.txt)、[主程序清理](../../game/app/src/main.cpp)。

## 创建、尺寸与图形交接

非正创建尺寸使用主显示器一半及 800/600 下限；SetSize 的参数则是正逻辑尺寸。`width/height` 目前仍是可写 public int，由 SDL_GetWindowSizeInPixels 在创建、尺寸/最小化/恢复事件及 SetSize 后更新，表示真实像素，不是请求值。消费者不能通过写字段伪造实际 extent；GetAspectRatio 使用 max(width,1)/max(height,1) 只避免除零，不证明可绘制。T4 的受控尺寸/Runtime 实例尚未实施。

创建先隐藏、配置完成后显示。GL 要求并核对实际 OpenGL 4.6 Core、4x MSAA 和 Debug context，不静默降级；GL 全局 attributes/hints 的变化没有事务回滚。Benchmark 是普通无边框窗口，非 exclusive/topmost，以 NonRudeHWND 避免覆盖任务栏，并核对实际像素与请求工作负载完全相等。SetCursorMode 的相对模式、可见性、Reset 是连续操作，后项失败不会自动回滚已经完成的原生状态。

三条私有桥接各保留自己的寿命：

- GraphicsBridge 只用于 OpenGL：MakeCurrent 核对 window/context 一致；GetProcedure 可合法返回空入口；ExtensionSupported 的不支持为 false，查询错误则抛错，不混同两者；SetVsync 读取实际 swap interval 核对，不能据此推定物理显示延迟；Present 要求此窗口/context 当前，交换失败即停止正常提交。
- NativeBridge 返回经校验的借用 HWND，至 Window::Destroy 失效，调用者不得 DestroyWindow。它可从三种活窗口取 handle，但当前实际 D3D12 后端创建 Native 模式窗口并消费它，不是“只为未来预留”；platform 不拥有 swapchain/device。游戏默认仍为旧 GL，D3D12 仍是 opt-in 候选。
- VulkanBridge 限定 Vulkan 模式；SDL 负责对应窗口的 loader 责任，GetInstanceProcedureAddress 是实例入口来源，required extensions 返回自有 string vector。instance/device/surface 属于调用方，须用同 loader/匹配 allocator，在有效窗口/loader 期间销毁并先结束相关在途 GPU 工作；DestroySurface 的空 handle 为无操作，但不会清调用者的非空变量，也不保证重复非空销毁安全。不要额外 UnloadLibrary，不从静态 SDL 推导静态 Vulkan loader。桥接/探针已存在，不等于 Vulkan 游戏后端已实现。

依据：[GL bridge](../../game/modules/platform/src/graphics_bridge/graphics_bridge.h)、[Native bridge](../../game/modules/platform/src/graphics_bridge/native_bridge.h)、[Vulkan bridge](../../game/modules/platform/src/graphics_bridge/vulkan_bridge.h)、[D3D12 消费者](../../game/modules/renderer/src/backend/d3d12/d3d12_backend.cpp)。

## 输入归约与借用失效

InputState 以窗口 ID 过滤事件，接受全局 Quit，只转换平台事实；不访问 simulation/renderer。11 个控制键映射固定物理 scancode，不采用字符布局语义。`down` 记录 held，`pressed` 锁存非 repeat 的短按；Capture 对每个键执行 held OR pressed 后仍无条件清 pressed，避免短路导致旧按键漏消费。repeat 不制造新短按，鼠标按钮保持 held。

相对 motion 保留 double dx、反转 SDL y 为向上正；wheel 保留小数并归一 flipped 方向。窗口激活/恢复/Reset 后抑制首个 motion，再按事件顺序存储，不合并成总量，保持 simulation 的逐事件 pitch/FOV 裁剪。published/pending 两个 vector 各预留 64，Publish clear/swap 复用；预留不是事件无界或全程零分配保证。

PollInt 排空 SDL 队列，经同一 Handle 适配，再按真实窗口 flags 收敛焦点/最小化并 Publish；Capture 在 app 的 simulation_ms 边界内同步 scancode/鼠标物理态后消费键锁存。只在 focused 且非 minimized 时采纳全局物理按键；丢失 regain/restore 事件可由 flags 修复，失焦后同轮恢复仍保留一次 focus_lost。Reset 清已发布/待发布控制、短按、指针及首 motion 基线；活跃窗口仍已知的 held 可保留/在下一 Capture 重同步，Reset 不是永久丢弃物理 held。

Window::Input 返回内部 InputSnapshot 的 const 引用，不是冻结帧：Poll/Wait 的收敛、Capture、Reset、cursor mode 更新可能改变它；pointer 元素在 clear/swap/扩容后失效，Window Destroy 后整个借用失效。延后使用须自行复制，当前生产路径同步消费。`Down` 要求合法枚举，不检查非法索引。KeySampling::Capture 是独立纯 CPU helper，顺序同步调用每个 Control 一次，不保存 callback；异常上抛且外部读键副作用不回滚。当前 SDL Capture 使用私有 InputState，不靠该 helper 采集硬件。

适配中的 vector 等分配异常保存首个嵌套原因并立即停止当前 Poll/Wait；后续 Poll/Wait/Capture/Reset checked 入口重抛，不能返回伪成功空输入或以 Reset 恢复。已取出的事件和先前状态修改不回滚；Input() 本身不检查该 latch，旧内容不可当作失败后的新成功帧。SDL 查询/操作失败具名抛出，但并非每一种 SDL 异常都进入 input_error。

依据：[InputState](../../game/modules/platform/src/input_state.h)、[KeySampling](../../game/modules/platform/include/symocraft/platform/key_snapshot.h)。

## Wait、焦点与有限诊断

WaitEvents 接受有限非负秒数，ceil 转毫秒并封顶 INT_MAX；等待取出的第一个事件经 Handle 恰好一次，再排空队列、收敛 flags，不 Publish，随后 Poll 才发布待输入。超时不是事件错误，真正 SDL 故障抛出；不能只等待而丢弃首个 close/focus/resize 事件。

普通无限游戏失焦、最小化或零实际像素时清输入/水平移动/待跳和共享物理预算，Wait 而不忙绘；恢复首帧 dt=0。有限 frame-limit 是专用诊断，可在失焦但仍可绘制时继续，不能推广成普通游戏后台规则；最小化/零像素始终不绘制。关闭事件在该帧 simulation/render 前退出。Window::Time 是 Init 起点的单调计时，未 Init/Free 后为 0，再 Init 重置起点，不是应用持久时钟。

Benchmark platform reducer 屏蔽玩家控制/指针/鼠标，只保留本窗口 Escape 和关闭；本窗口已排队未消费 Escape 在失焦两种先后顺序及 flags 收敛时保留，不采集后台全局 Escape，最小化清除。app 对尺寸变化/最小化判 invalid，allow-unfocused 仅改变失焦有效性，不允许最小化，也不能偷渡普通游戏有限帧规则。Poll 归 event_ms、Capture 仍归 simulation_ms，迁移不改测量边界。

## 内存查询、不变量与验证边界

SampleProcessMemory 返回自有 optional working-set/private bytes；GetProcessMemoryInfo 失败时两项保持缺失，不填 0，不依赖 Window/GPU。设备级 GPU 内存估计在 renderer，不是平台的进程 VRAM。主线程窗口约束不等于所有纯值/进程查询 API 都必须拥有窗口。

| 原 Window 编号 | 当前成立边界 |
| --- | --- |
| I1 | native 有效期 Impl 存活；Create 局部 owner 不发布半初始化对象。 |
| I2 | 有效清理前提下 Destroy 继续释放、清空 context/native/input/像素并可重复，完成后报告首错。 |
| I3 | 每个映射键一次 Capture 消费短按，指针保序交换复用；借用随后续更新失效。 |
| I4 | 事件只生成平台事实，公开 API 不泄漏 SDK，不含游戏状态 callback。 |
| I5 | 主线程单窗口，video 覆盖 Window；GPU/surface 先于窗口/context 释放。 |

静态核对包括 [key snapshot 测试](../../test/unit/platform/key_snapshot_tests.cpp)、[公开头/三 bridge 声明测试](../../test/experimental/platform-handoff/contracts.cpp)、[生产 InputState 合成测试](../../test/experimental/platform-sdl3/input_state_tests.cpp) 与 [分配失败探针](../../test/experimental/platform-sdl3/allocation_tests.cpp)。合成 reducer 或声明编译不证明真实 SDL、硬件布局/DPI、任务栏或驱动行为；本轮没有执行这些测试或创建窗口。历史证据/批准从 VERIFICATION 进入；[T4 platform delta](../openspec/changes/m3-t4-application-simulation/specs/platform/spec.md) 的 owned snapshot、Runtime 与受控尺寸仍是目标。
