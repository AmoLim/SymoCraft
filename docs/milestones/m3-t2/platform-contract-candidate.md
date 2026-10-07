---
type: 功能
status: 候选契约，准备实验与评审中
project: Symocraft
module: M3-T2-Platform-SDL3
created: 2026-10-07
tags:
  - area/milestones
---

# M3-T2 平台契约与失败责任候选

本文是 [T2 正式计划](../../spec/M3-T2-SDL3迁移.md) 的 P05 准备产物，不是当前平台 API，也不是生产切换或节点验收通过记录。当前生产实现仍使用 GLFW；下列 SDL3 增量须经 P02–P04 实验、P05/P06 评审后才能冻结。树叶剔除与视角突变继续延期，留基线记录，不作为准备工作的前置阻断。

本次沿用已有 `Window::Impl`、应用主循环和静态 Init/Free 调用路径，只补充换库、三种窗口用途及失败释放所需契约。不建立完整 Runtime/SimulationSession 管理器，不在此实施 Renderer PImpl 或应用对象重构；这些工作分别留 T3/T4。原草稿附录不修改。

权威起点：[Window 公共头](../../../game/modules/platform/include/symocraft/platform/window.h)、[当前窗口实现](../../../game/modules/platform/src/window.cpp)、[私有图形桥接](../../../game/modules/platform/src/graphics_bridge/graphics_bridge.h)、[应用生命周期](../../../game/app/src/application.cpp)、[Window 设计](../../project/architecture/platform/Window-类设计.md)。

## 公共接口候选

公共头仍不包含 SDL3、Windows、GLAD、D3D12 或 Vulkan 头，不发布原生对象、SDK 枚举或公开 `void*`。窗口、输入和平台操作继续由应用主线程同步调用。

| 入口 | 候选增量或保留行为 | 冻结前必须确定 |
| --- | --- | --- |
| Init/Free | Init 成功后记录本模块取得的 SDL video 初始化责任；重复 Init 不增加本模块持有次数。Free 只释放本模块取得的责任，重复调用安全 | P02 验证普通 main 的私有入口准备；Free 的前提是所有窗口及所借用图形资源已释放，不替调用者销毁 GPU 对象 |
| Create | 增加项目中立的 OpenGL/Native/Vulkan 用途。现有默认创建仍为 OpenGL；窗口和需要的 GL context 全部成功后才发布拥有的 Window | 采用创建选项结构还是兼容重载；标题借用仅持续到调用结束；请求尺寸、显示器默认尺寸、Benchmark 目标像素的具体转换规则 |
| Destroy/析构 | Destroy 幂等，销毁本 Window 所有的 GL context 和系统窗口；析构只兜底 Destroy，不结束全局 video 生命周期 | renderer 和借用 HWND/surface 的调用方已结束使用；失败窗口不对外发布；不允许一半窗口仍可继续绘制 |
| PollInt | SDL 事件先转换为中立事实；过滤窗口 ID，同时处理全局退出。发布这一帧有序指针事件和失焦标记，不直接修改 ECS/相机或提交图形命令 | 等待取得的首事件如何暂存；事件缓冲失败如何保留具名诊断并停止本帧，不能伪装成成功空输入 |
| CaptureInput/Input | 11 个项目 Key 映射 scancode，每键按现行规则消费一次；短按/同帧按放锁存到一次采样。Input 仍为同步借用，不跨下一次变更/销毁保存 | 最终按住态与未消费短按态分开；repeat 不制造新短按；Benchmark 仅消费 Escape，不采纳玩家键鼠 |
| ResetInput | 清当前快照、未消费锁存、待发布指针事件和鼠标恢复基准，保持失焦/暂停恢复首帧 dt=0 | 失焦按住及恢复时何时重新采纳仍按住的键；相对模式切换和事件消费顺序；不能把旧积压运动带入恢复帧 |
| WaitEvents | 秒到等待单位的转换有边界；取得的事件经相同适配路径消费恰好一次，不吞关闭/焦点/按键 | 小于一等待单位、零值和上限的规则；等待只累积事实，何时由 PollInt 发布 |
| Time | 单调高精度时间，公开单位仍为秒 | 起点及 SDL 初始化/退出前后的合法调用期；不改 Benchmark 的 CPU/GPU/present 统计定义 |
| SetCursorMode | Lock/Hidden/Normal 可以切换和恢复，保持向右/向上为正、现有灵敏度与焦点行为 | 对照实测决定系统加速度/相对输入策略，不默认把迁移变成手感调整 |
| width/height、SetSize/GetAspectRatio | width/height 继续表示实际绘制像素；`SetSize` 保持现行逻辑窗口尺寸入参，不改成像素；零像素代表暂不可绘制 | resize 后查询实际像素，不把请求值直接赋给 width/height；Benchmark 要求实际 1920×1080，不能把逻辑尺寸相等当作通过 |

创建模式可使用一个小型项目枚举和创建选项结构表达，不新增拥有 SDL 运行期的公开类。此处不预先写入真实公共头；签名应由三模式实验的实际调用需要决定，避免同时保留互相矛盾的模式入口。

三模式探针后优先候选为 `enum class WindowMode : std::uint8_t { OpenGL, Native, Vulkan }`，作为现有 `Window::Create` 的最后一个参数，默认 OpenGL，保留当前调用点。窗口私有实现记录不可变用途，只设置其中一个图形标志；无 GL 用途不能调用 GL 当前化/呈现。该窄增量仍须 P06 确认，尚未写入生产头。

私有桥接候选分为现有 GL 头、原生 HWND 头、按需 Vulkan 头。Vulkan 所需扩展以项目拥有的字符串副本返回，过程入口统一来自 SDL loader；surface 由探针或后续 renderer 拥有，销毁在 instance/window 之前。通用 GL 头不包含 Vk 头，不要求 GL-only 使用者安装 Vulkan SDK。

## 所有权与私有桥接

| 对象或事实 | 所有者 | 借用方与期限 | 释放责任 |
| --- | --- | --- | --- |
| SDL video 初始化责任 | platform 的现行静态生命周期路径 | app 主线程；从成功 Init 到 Free | Window/所有图形对象结束后释放本模块取得的责任，失败初始化不记为成功 |
| SDL 系统窗口 | 对应 Window::Impl | app 通过项目 Window；私有桥接借用 | Window::Destroy/析构唯一销毁，不允许 app 或探针直接销毁 |
| SDL GL context | OpenGL 用途的 Window::Impl | renderer 的当前化、加载、绘制及呈现 | renderer 先释放图形资源，再由 Window 释放 context，最后销毁窗口 |
| Native HWND | 系统窗口，不另有拥有者 | 授权私有桥接及原生探针，至窗口销毁前 | 不单独 DestroyWindow，不缓存到窗口销毁后；验证有效 HWND 不等于 D3D12 交换链验收 |
| Vulkan loader 入口及窗口关联加载责任 | 按所选 SDL Vulkan 路径固定一个来源 | 探针/后续后端，仅在对应加载责任有效期间 | SDL 窗口关联加载责任随窗口结束；不得额外加载另一来源来掩盖入口或寿命问题 |
| Vulkan instance/surface | 准备阶段为探针；后续 T3 为后端 | platform 只按私有桥接协助创建 surface，不接管实例或 GPU 资源 | 探针先销毁 surface，再销毁 instance，再销毁窗口；T3 实际设备另需结束在途工作并清理其资源 |
| Vulkan 必需扩展名 | 按候选桥接返回契约管理 | 只在合法查询/创建期使用；需长期保存时复制项目自有字符串 | P03 明确是否立即消费或复制，不能靠未声明期限的指针缓存 |
| InputSnapshot/PointerEvent 缓冲 | Window::Impl | app 当前同步消费期 | 下一次发布/重置/销毁可使引用失效，调用者不跨该边界留指针或引用 |

GL 桥接保留现有六项职责：当前化、当前 context 查询、GLAD 函数地址、扩展查询、VSync、呈现。Native/Vulkan 的 SDK 类型仅存在于授权私有桥接或探针；游戏调用方只选择项目窗口用途，不读取 SDL 原生句柄。需要新增私有头路径时同步边界授权和负向测试，不扩大整个 platform/src 的可见范围。

GL 的当前化、VSync 和呈现若返回失败，候选实现必须具名报告并阻止无效继续提交；不能因为旧桥接返回 void 就继续吞失败。能力查询的合法“不支持”与无当前 context/查询失败应能区分。Vulkan 探针要求真实扩展、instance 和 surface，不创建或声称完成游戏 GPU 后端。

输入候选已增加 `PhysicalInputState` / `SynchronizePhysicalState`：drain 后、Capture 前采新 SDL 状态；active Reset 清短按、公开快照和运动，保留 known held，并在 Capture 前再次采新状态。失焦/最小化清 held，恢复后重新采纳真实按住或释放态；repeat 不制造新短按；不采后台全局键鼠，Benchmark 保留本窗口 Escape 事件。185 项合成检查通过，生产实现仍须链接真实 platform 回归，硬件释放与不同布局仍需人工对照。

SDL 相对运动默认不采用系统加速度，而当前 GLFW 未启用 raw mouse。连续对照工具默认请求 `SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE=1`，也提供显式未缩放候选；两端共用冻结游戏的 `ApplyPointerInput`/`Camera::Scroll`，不修改 0.05 灵敏度。设置 hint 和自动渲染通过均不能当作手感等价。

只读评审建议保持 GL 六项签名并在当前化、VSync、呈现失败时具名抛错；`GetProcedure` 可返回空供 GLAD 判断可选函数，扩展查询先要求有效当前 context。Native 候选为 `HWND Win32Handle(const Window&)`；Vulkan 候选为按窗口取得 `PFN_vkGetInstanceProcAddr`、返回自有 `vector<string>` 扩展、借用 instance 创建/销毁 surface，分配回调须成对一致。上述 SDK 只进入授权私有头，不写入生产公开头。

等待候选规则为拒绝负数/非有限值、零值不阻塞、正数向上取整为毫秒并限制 `Sint32` 上限；等待前清本次 SDL error，false 时区分超时与新错误。首事件只累积、下次 Poll 发布一次。Time 限定 Init 至 Free，以本次初始化起点返回单调秒值。正常退出先释放 GPU，再 checked context/window/video 清理；析构仅作 best-effort 兜底，不因清理异常中断后续释放。

## 失败责任与清理矩阵

| 失败点 | 失败时可能取得的资源 | 处理责任与对外保证 | 准备证据 |
| --- | --- | --- | --- |
| video 初始化或入口准备 | 无成功初始化责任，或局部入口状态 | platform 记录操作名与库诊断，保持未初始化；app 正常错误退出，不继续 Create | 实际失败或受限测试注入；重复 Free |
| Window/输入缓冲分配 | 局部 C++ 对象 | 不发布 Window；局部 RAII 回收；不跨 C 回调边界抛异常 | 分配失败探针；无 public 故障开关 |
| 系统窗口创建 | 局部 Window，无有效系统窗口 | 不建立借用句柄；app 仍可调用幂等清理 | 创建失败与诊断记录 |
| GL context 创建/当前化 | 有系统窗口，可能有 GL context | 先释放已创建 context，再释放窗口；不发布“可绘制”状态 | context 创建、当前化失败及重试/退出 |
| GLAD 加载/GL 属性检查 | 已发布 Window/context，renderer 部分状态 | app 先结束 renderer 的部分资源，再释放 Window/context；不降 GL/MSAA 规格继续跑 | 真实驱动诊断、部分初始化清理 |
| HWND 或 Benchmark 任务栏策略 | 有系统窗口，无完整启动 | Win32 失败具名报告；移除已设置的本窗口属性后销毁；不误改其它 HWND | HWND、NonRudeHWND 与隐藏后显示探针 |
| Vulkan 扩展/loader/instance | 有 Vulkan 窗口，可能有 instance | 探针清理已取得对象；缺环境清楚失败，不影响未选 Vulkan 的 GL/Native 运行 | 启用/关闭探针的配置差异、真实环境结果 |
| Vulkan surface 创建 | 有窗口和 instance，可能有 surface | 探针释放已成功 surface，再 instance，最后 Window；所有入口在 loader 有效期内使用 | surface 失败、逆序退出和重复清理 |
| 事件转换或缓冲分配 | 有 Window/context、上一已发布输入 | 停止本帧正常模拟/绘制，交给 app 具名退出与统一清理；不生成误导的成功空快照 | 真 platform 故障探针及退出记录 |
| WM_CLOSE/全局退出/用户取消 | 正常运行期对象 | 转关闭请求，走既有 app 清理及结果落盘；不在事件处理内直接销毁图形对象 | runner 在强制结束前正常完成；关闭事件经过等待也不丢失 |

故障注入只放根级 test 的受限 fixture/测试私有访问中，不向游戏公共 API 增加 SDK 类型或开关。不以 mock 成功替代真实窗口/context/surface 能力；不能可靠制造的真实失败须记录限制与替代验证范围，不能勾成已通过。

## P05 评审检查

- [ ] P02 的固定源码/入口/静态选项与 Init/Free 责任一致。
- [ ] P03 三种用途各有真实成功、失败及逆序清理记录；GL 不要求 Vulkan SDK/运行环境。
- [ ] P04 已明确 scancode、短按锁存、失焦恢复、相对运动与等待首事件规则。
- [ ] 中立创建模式与私有 HWND/Vulkan 接口的签名、借用期、失败保证有实验依据。
- [ ] GLAD/Vulkan 函数入口来源、实际图形属性和 loader 寿命可追溯。
- [ ] 真实 platform 回归覆盖上表；现有纯 KeySampling 单测不作替代。
- [ ] 新私有桥接路径、SDL 头/target 授权及 CPU/工具防泄漏规则有正向和负向用例。
- [ ] P06 明确剩余限制、短采样参数与预算确认安排；候选转正式增量后再授权生产切换。

上述项目尚未验收。准备记录和候选接口不能把 T2、三后端玩法、两项延期问题或后续 T3/T4/T5 标记完成。
