---
type: 契约交接
status: S4平台契约已交接，T2节点已由用户批准，Renderer v1未冻结
project: Symocraft
module: M3-T2-S4
created: 2026-10-08
updated: 2026-10-09
tags:
  - area/milestones
  - topic/platform
---

# M3-T2 S4 平台交接给 T3

本页落实 [T2 正式计划](../../spec/M3-T2-SDL3迁移.md) 的 S4：将已经实现且有对应验证范围的平台能力交给 T3-R1 核对，不实现生产 D3D12/Vulkan 后端，也不冻结候选 `Rendering::Renderer` v1。阶段状态、最新验证与包冻结记录以 [T2 实施记录](README.md) 为准；历史事实分别由 [S2 验证身份](evidence/s2-validation.json) 和 [S3 验证身份](evidence/s3-validation.json) 支持，不能把本页源码核对当成本轮重新运行了其测试。

2026-10-09 用户明确批准 [T2 节点正式通过](README.md#t2-正式节点批准)，并确认 MB01 方案2（704 / 1408 MiB），FB01 方案2保持。T3 的 T2 节点批准前置已解除，R1/v1 冻结及 R2 授权仍独立核对。正式同源 Q06 未完成、首次可操作预算未定及 Win32 1175/Fix1 暂停等保留为 [遗留事实](../../spec/M3-T2-SDL3迁移.md#节点批准与遗留事项)，不将节点批准伪称为这些验证已通过；下方历史门槛不覆盖该明确批准。

2026-10-08 用户授权冻结可运行的 S3 验证包并继续下一阶段；当时暂缓 Y9000P，后续最新确认已将 M3 功能/兼容验收改为仅本机 RTX 5070 Ti，见 [[project-scope#M3 本机 RTX 5070 Ti 验收与下一玩法阶段性能门槛|最新验收范围]]。当前 [S3 人工清单](s3-manual-verification.md) 的开发桌面 H01–H15 已由用户填写通过（H14 原文为 `passs`），包括 H04/H07；这是人工反馈，不另行要求重复确认。包/机器/布局/DPI 条件、分项数值和 Benchmark 协议材料仍须按实际证据绑定，不能仅由一列通过补造完整矩阵。真实硬件工具记录可核对来源与正常退出，但不能代替未取得的完整 DPI/Benchmark 数值证据。Y9000P 格保留历史未执行状态，不作为 S3/T2 的当前前置；整体验收状态仍独立保留。

T3 已另获并行进行独立 D3D12 前置实验的授权，本机 RTX 5070 Ti 的 R1 局部结果见 [T3 实验记录](../m3-t3/README.md)。该事实继续有效；它不是本次 S4 新授予的生产接管许可，也不代表生产玩法、R1 全项或公开 Renderer v1 已冻结。T2 通过且 R1 自身交接/实验条件成立后才能冻结 v1、进入 R2，不以其他 GPU 或第二机器为前置。

## 交接来源

| 权威入口 | 本页消费的事实 |
| --- | --- |
| [Window 公共头](../../../game/modules/platform/include/symocraft/platform/window.h) | 实际类型为 `SymoCraft::Window`；`WindowMode`、尺寸、输入与生命周期签名 |
| [Window 实现](../../../game/modules/platform/src/window.cpp) / [真实输入 reducer](../../../game/modules/platform/src/input_state.h) | SDL 只在 platform 私有实现，模式检查、像素查询、输入同步与资源释放 |
| [GL 私有桥](../../../game/modules/platform/src/graphics_bridge/graphics_bridge.h) / [Native 私有桥](../../../game/modules/platform/src/graphics_bridge/native_bridge.h) / [Vulkan 私有桥](../../../game/modules/platform/src/graphics_bridge/vulkan_bridge.h) | 各 API 的实际签名、借用期与错误表达 |
| [当前 renderer](../../../game/modules/renderer/src/renderer.cpp) / [当前 app](../../../game/app/src/application.cpp) | 当前 GL AttachContext/Render/Present 与帧边界处理，非候选 Renderer v1 |
| [边界授权](../../../cmake/ModuleBoundaries.cmake) / [构建与测试边界](../../project/architecture/build/构建与测试边界.md) | 精确私有头白名单、SDL 私有依赖以及 CPU/工具隔离 |
| [T3 候选 spec](../../spec/M3-T3-渲染器重构.md) / [CPU 交接清单](../../spec/M3-T1-T3-CPU契约清单.md) | 后端先行方向与冻结门槛；纹理数组、中立相机仍属 T3 候选 |

本文的 `Window` 不表示新增 `Platform::Window` 类。`GraphicsBridge`、`NativeBridge`、`VulkanBridge` 位于 `SymoCraft::Platform`，通过授权私有友元访问 `SymoCraft::Window`，不改变窗口类型所有者。

## 创建与尺寸

实际公共创建入口：

```cpp
static Window* Create(const char* title, int width = 0, int height = 0,
                      bool benchmark_window = false, bool allow_unfocused = false,
                      WindowMode mode = WindowMode::OpenGL);
```

| Renderer 路径 | 创建时选择 | 平台提供 | 不提供 / 不允许推定 |
| --- | --- | --- | --- |
| 当前 OpenGL / 必要旧路径对照 | `WindowMode::OpenGL`，也是省略末参的默认值 | OpenGL 窗口及显式 GL context；实际 4.6 Core、4x MSAA，Debug 请求并核实 Debug context | 不允许静默降版本/采样数；不是 Renderer v1 已实施 |
| 未来 D3D12 | `WindowMode::Native` | 无 GL/Vulkan 标记的原生窗口，借用 HWND | 平台不创建 D3D12 device、DXGI swapchain、队列、资源或 fence |
| 未来 Vulkan | `WindowMode::Vulkan` | Vulkan 标记窗口、SDL loader 入口、必要扩展副本及 surface 辅助 | 平台不创建或接管 Vulkan instance/device/swapchain/提交同步 |

后端必须在创建窗口前选定；同一窗口不能运行中改模式。当前实现只支持一个活 platform 窗口，重复创建第二个被具名拒绝，不是多窗口或后端热切换机制。非法用途枚举也被拒绝。未选 Vulkan 时不得用 Vulkan 窗口替代 Native/GL 来预加载另一套运行时。

`Create` 的正尺寸和 `SetSize(w,h)` 入参均为**逻辑窗口尺寸**；非正 `Create` 入参沿用当前主显示器派生默认，`SetSize` 非正入参明确拒绝。`Window::width/height` 是 `SDL_GetWindowSizeInPixels` 查询到的**实际 drawable 像素事实**，不是请求值；创建、SetSize 后及相关 resize/pixel/minimize/restore 事件更新它们。调用方不可为了匹配配置直接覆盖公共尺寸字段。

尺寸事件只更新事实，不调用 GPU 命令。renderer 在事件处理完成后的帧边界读取实际像素并更新 viewport 或重建尺寸相关资源。逻辑尺寸相同不证明 drawable 相同；SetSize 返回也不作为跨 DPI/显示器最终稳定尺寸的保证，后续事件仍需处理。`GetAspectRatio()` 的零值钳制只防除零，不授权在零尺寸绘制。`Minimized()` 或实际尺寸非正时，不提交绘制/呈现；恢复后依最新实际像素恢复，不创建新的 Window 对象。

Benchmark 保持普通无边框、非独占、非置顶、NonRudeHWND 策略与实际 1920×1080 要求。创建时实际像素不匹配请求直接失败，不静默降低负载。strict/allow-unfocused 的协议由 app/telemetry 编排，允许后台采样不豁免最小化或尺寸变化无效条件。S3 固定 Benchmark 夹具未成功制造真实尺寸变化，该分支仍是 partial/unavailable，不能从普通窗口 resize 或 R1 Native resize 继承通过。

## 所有权与线程

| 对象 / 入口 | 所有者与借用 | 寿命结束责任 |
| --- | --- | --- |
| `Window::Init/Free` 的 SDL video 责任 | platform 成功 Init 取得一次本模块责任；重复 Init 不增持有次数 | 活窗口存在时 Free 拒绝；只释放本模块责任，不退出外部 SDL video owner |
| `Window` / PImpl / SDL 窗口 | app 独占拥有；当前 app 用 `unique_ptr<Window>` 接受 Create 的拥有型裸指针；禁止复制且当前不支持移动 | app 保证 renderer/所有借用先结束，再 Destroy/析构，最后 Free |
| GL context | 对应 OpenGL Window::Impl 独占；renderer 借用当前 context | renderer 先释放 GL 资源，Window 再删除 context 和系统窗口 |
| HWND | SDL 系统窗口的借用标识，不是新的拥有对象 | 不调用 `DestroyWindow`，不延长或缓存到 Window::Destroy 之后 |
| Vulkan 入口 | 对应 SDL Vulkan 加载责任有效期内借用的函数入口 | 所有 Vulkan 调用结束后才能销毁窗口、卸载对应 loader 责任 |
| Vulkan instance/surface 与未来 GPU 对象 | 当前探针或未来 renderer 拥有；platform 只协助取得入口和创建/销毁 surface | 调用方负责成对释放和结束在途工作，不能期待 Window 代为清理 |

窗口创建、事件、桥接、销毁及 video 生命周期在 SDL 主线程串行执行；当前实现对相关活窗口操作执行主线程检查。不能把存在部分运行时检查理解为任意线程可用或线程安全，`Time()` 等无主线程检查的辅助入口也未交付并发服务承诺。T3 候选公开调用沿用创建线程串行、不从事件回调重入，不在本次交接引入渲染线程。

Window 是唯一系统窗口销毁者，三个桥都不取得拥有权。实现没有为借用者登记 GPU 在途资源或 surface 计数；主线程/模式检查不会替调用方证明所有借用已经结束，正确清理顺序仍是 app/renderer 的责任。

## 输入快照

`Input()` 返回 `const InputSnapshot&`，对象由 Window 的私有 InputState 拥有。引用本体至输入状态销毁前存在，但**值不是历史冻结副本**：Poll、Wait 的状态收敛、Capture、Reset 与鼠标模式切换会改变相关状态，PointerEvent 容器内容/迭代器不可跨这些操作保存。需要异步或延后消费时显式复制项目值，不保存快照、vector 元素或 SDL 事件指针。

- `PollInt()` 排空事件、按真实 flags 收敛焦点/最小化，再发布按顺序的 PointerEvent 与一次性 focus-loss 标记；尺寸同时更新为查询事实。
- `CaptureInput()` 在模拟消费时同步当前实际 held 并消费键盘短按锁存，不额外排空事件，也不重复发布 PointerEvent。两鼠标键是 held 采样，不是键盘式 sticky click；同一次 Capture 前完成按放时按钮可为 false。
- `WaitEvents(timeout)` 处理首事件并排空剩余事件、同步窗口事实，但不替代下一次 Poll 的指针发布或 Capture 的 held 采样；首事件不能丢弃。OS 消息可提前唤醒，当前证据没有强制空闲 timeout 的完整证明。
- `ResetInput()` 清旧短按、快照、待消费运动及首运动基线；获焦活动窗口下一次 Capture 重新采纳仍实际按住的控制，不能把 Reset 理解为忽略直到松开。失焦/最小化不采玩家后台全局输入，恢复后重新采当前事实。
- 模式切换、恢复首运动保护、`xrel/-yrel` 符号、有序浮点滚轮、既有 0.05 相机灵敏度和已选系统缩放策略保持；renderer 不消费或保存输入快照，相机/FrameInput 由 app 转成中立 CPU 数据。
- Benchmark 隔离玩家控制，只保留本窗口 Escape/关闭；已排队未消费 Escape 跨同轮失焦保留，但最小化按无效/重置路径处理。不能用后台全局键盘补采取消操作。

以上实现和自动范围见 S3 JSON；用户人工通过表与具体机器/条件身份单独记录，不将合成队列或 Win32 消息当作真实按键输入。

## 图形桥接

### 当前 OpenGL

六项签名保持：`MakeCurrent(Window&)`、`HasCurrentContext()`、`GetProcedure(const char*)`、`ExtensionSupported(const char*)`、`SetVsync(bool)`、`Present(Window&)`。MakeCurrent/Present 明确要求 OpenGL 模式；Present 还核对当前 context 和窗口正是该 Window。过程、扩展及 VSync 入口针对当前 context，调用方先明确当前化，不假定它们自行切换窗口。

必要操作失败具名抛出拥有文本的异常，不因返回 void 继续提交。合法可选过程/扩展可分别返回 null/false；无 context、非法参数或查询失败不等同于能力不支持。GLAD 仍由当前 renderer 的 `AttachContext` 经私有 GL 桥加载，Window 不承担 GLAD 加载或 renderer 图形资源拥有权。VSync 请求后核实实际 swap interval。

现有 app 在 `Renderer::Render(...)` 后调用 `Renderer::Present(window)`，由后者转调 GL 私有桥。T2 不把这条分离流程改为 Renderer v1。T3 候选由 `Renderer::Render(FrameInput)` 内部完成提交/呈现，迁移时必须同时撤掉对应 app 的外部 Present，避免双份呈现；Native/Vulkan 后端不能调用 GL Present。现代 VSync/呈现模式及实际配置查询归其后端，不复用 GL swap interval 冒充。

### Native HWND

实际入口为 `NativeBridge::GetHandle(Window&) -> HWND`，不是早期候选 `Win32Handle(const Window&)`。它查询 SDL 窗口属性并核实有效 HWND，不产生 HWND 副本或新寿命。该入口**没有限定只能 Native 模式**，当前 Benchmark 的 GL 窗口也用它设置自有 NonRudeHWND；这不授权 D3D12 对 GL 窗口另建混合呈现。D3D12 按表选择 Native，再由 renderer 私有实现借用 HWND。

有效 HWND 仅证明桥接事实。D3D12 device、硬件能力、DXGI swapchain 格式/尺寸、资源状态、fence 与失败清理须由真实后端验证，不能从 T2 HWND 成绩推定。已完成的台式机 D3D12 R1 夹具是独立证据，不升级为生产玩法或双机验收。

### Vulkan loader 与 surface

唯一已选入口来源为 SDL Vulkan 窗口关联的 loader，不额外 LoadLibrary、静态链接 `vulkan-1.lib` 或引入另一 loader 掩盖来源/寿命。静态 SDL 不等于静态 Vulkan loader。

| 实际私有入口 | 调用约定 |
| --- | --- |
| `GetInstanceProcedureAddress(Window&)` | 要求 Vulkan 模式，借出 `PFN_vkGetInstanceProcAddr`；instance/device 函数继续经同一来源取得，不在卸载后调用 |
| `GetRequiredExtensions(Window&) -> vector<string>` | 返回项目自有字符串副本，不借出 SDL 扩展数组；副本可独立保存，传给 `vkCreateInstance` 的 `c_str()` 指针仍须保证其本地容器未移动/修改且调用期间有效 |
| `CreateSurface(Window&, VkInstance, allocator)` | 要求活 Vulkan 窗口与非空 instance；成功 surface 归调用方，instance 也归调用方；不创建 device 或 swapchain |
| `DestroySurface(Window&, VkInstance, VkSurfaceKHR, allocator)` | 在窗口/loader/instance 仍有效时调用；与创建使用兼容的分配回调，释放后调用方置空 handle；空 surface 在活 Vulkan 模式下可直接返回，不允许由此推定销毁后的 Window 仍可用 |

这些签名不能运行时证明传入 surface 属于指定 instance、allocator 或窗口，也不证明 GPU 已空闲；调用方自行保证配对。DestroySurface 经 SDL 的 void 销毁入口，没有额外可查询的“GPU 已完成”或 driver 成功凭据，不据此伪造确认。

退出顺序：停止新提交并按后端约定有界结束/处置在途工作，释放依赖交换链和窗口的 GPU 对象，清理 device 及 surface，再销毁 instance，最后 Window 和 video 责任。device 与 surface 的彼此先后按 Vulkan 依赖设计处理，二者及全部 API 调用必须先于 instance/window/loader 终结。构造只取得部分资源时，仅释放已取得对象，不继续调用未加载/已卸载入口。

生产 platform 只使用原样 SDL 附带的 Khronos 头私有编译；显式 SDK 探针另以固定 `C:/VulkanSDK/1.4.363.0/Include` 编译核对，SDK/驱动不随游戏包部署。未来 Vulkan renderer 的 SDK、shader 工具、私有 target 条件与运行能力由 T3 单独落实，不能让未选 Vulkan 的 GL/Native/CPU/工具强制依赖 SDK 或 loader。

## 错误与清理

Create 以局部唯一拥有者持有未发布的 Window，完整成功才返回拥有型指针；window/context/属性/input 分配中途失败时执行已取得资源清理。Window/Impl 与输入 reserve 分配失败有具名嵌套原因，扩容失败锁存并立即停止本帧发布，后续 Capture/Poll/Wait 重抛，不把半轮输入当成新有效快照。

`Destroy()` 尝试 context、NonRudeHWND 属性及窗口/输入释放，保留首个错误并报告后续清理失败；重复 Destroy 是安全空操作。析构为 noexcept 兜底并报告，不代表失败可被当作正常完成。`Free()` 不销毁借用 GPU 对象且要求本模块活窗口归零。当前 app 的清理序列继续后续释放，不让单项失败阻止余下步骤；存在主运行错误时仍保留原错误。

未来 renderer 负责自己部分构造、上传、命令提交、同步、surface/device/instance 失败后的停止提交与有界清理。Window 无法替代 device-lost 设计；不能在仍借用窗口时先 Destroy，再靠 renderer 析构调用已失效桥接。受限 fixture 的驱动调用失败和 test-only 分配失败只证明对应注入边界，不证明真实驱动故障恢复、OS 内存耗尽或完整 GPU 压力稳定性。

## 依赖授权

platform 私有链接 `SDL3::SDL3-static`、私有定义 `SDL_MAIN_HANDLED`，内部 `SDL_SetMainReady` 保留普通 main。renderer 仅取得 `platform/src/graphics_bridge/` 的私有头视图，白名单正是三份桥接头，不扩大为整个 `platform/src`。SDL 头/target/宏不得传播到 renderer、app、公开头或 CPU 模块；Native/Vulkan SDK 类型只进入授权私有头/真实后端，公开 Window 与未来 Renderer 只使用项目类型。

当前 renderer 只消费 GL 桥；未来现代后端新增 SDK 链接/私有头/单后端组合 target 时，必须同步边界授权、正负测试和独立公开头消费者，不把未来需要误写成当前 target 已具备。CPU-only 与独立 benchmark 不配置或链接 SDL/GL/现代 GPU SDK。根 test 的真实桥接/输入夹具导入实际生产静态库，不重编 Window 替身，不成为游戏生产依赖。

## 证明范围与待办

本轮 S4 声明/边界核对结果见 [验证身份](evidence/s4-validation.json)：Debug/Release 各 4/4 独立公开/GL/Native/Vulkan 消费者、无 Vulkan SDK 路径的 Release 3/3，以及生产边界脚本 45/45。它们只编译真实声明并检查 CPU 默认值，不链接生产库或执行 SDL/GPU；下表真实桥接成绩是有各自原始身份的既有结果，不是本轮声明测试取得的新硬件成绩。S3 冻结包及 ZIP 校验见 [冻结身份](evidence/s3-freeze.json)。

| 能力 | 已有证据范围 | 保留的限制 |
| --- | --- | --- |
| GL/Native/Vulkan 桥与基础生命周期 | S2 双配置各 17/17；S3 在实际 S3 生产库上双配置再各 17/17，真实 GL 属性/像素交换、HWND、Vulkan instance/surface 与清理 | 非现代 GPU 游戏后端；受限失败注入另标，不能证明所有真实 driver 故障 |
| 生产输入与窗口状态 | S3 双配置 reducer 各 263 项，真实库输入各 21/21，分配故障各 4/4；开发桌面人工表用户填通过 | 合成与硬件结果区分；人工条件/身份/协议逐项绑定，Y9000P 暂缓；默认 Raw Input/布局/DPI 不由合成测试代替 |
| 当前安装 GL 游戏 | S3 Debug/Release 各 120 帧正常；完整游戏窗口协议最终 6/7，已验证 strict/allow-unfocused、最小化无效与关闭落盘 | 未制造实际 Benchmark resize，恢复首帧 dt=0 只源码核对，非双机至少 15 分钟玩法；3 秒诊断不计 Q06/Q07 |
| D3D12 R1 | 既有独立台式机 Release 夹具真实纹理、在途更新/删除、Native resize/最小化恢复与 Debug Layer 结果；详见 T3 自有证据 | 不是 T2 本轮新跑；Debug GPU 配置、Y9000P、生产资产/相机迁移与 v1 冻结仍未完成 |
| Vulkan renderer | T2 提供真实入口/扩展/instance/surface 交接 | 设备能力、队列、swapchain、shader/资源、同步、重建、故障、截图/计时和完整玩法均由 T3 另验 |

特别限制：现有真实 instance/surface 创建/释放与 loader 入口结果，没有断言 OS 中 `vulkan-1.dll` 已实际卸载或重载；指定缺失库的失败测试不等于移除系统 Vulkan runtime。窗口关联加载责任是已选实现契约，不能将它推广为完整 device/GPU 在途生命周期实测。

S4 交接及当前 M3 节点不等待其他 GPU 或第二机器，也不据此把历史未执行格关闭。S3 条件/身份材料、S5 玩法/部署与 Q06/A01–A12 的实际证据仍分别保留；T2 当前节点已由用户正式批准，不再以这些遗留缺口推定节点未批准。FB01/MB01 均已确认，首次可操作预算未定。指定 Y9000P 中端与 GTX 1650 低端整机性能验证仍在下一玩法阶段功能开发前完成。已选八项三方案、Q04/P06 批准与原人工手感不重新待选；树叶剔除与旧视角突变继续延期。

CPU-only `performance.export` 的偶发 `status.yaml` 替换拒绝访问仍按 [S2 风险原记录](README.md#s2-窗口与桥接) 保留，原因未定位，不能归因 SDL/SDK、也不能用后续一次 CTest 通过关闭。S4 文档与 S3 验证包冻结不解决该风险；2026-10-09 用户已接受遗留风险并明确批准 T2，Fix1 继续暂停，不把 A01/A11 改为全绿或自动继续定位/复测。

T3-R1 接收本页后，核对实际 Window 类型/模式、借用期、像素与唯一呈现责任，并将已完成的独立实验和未完成的生产能力分别记录；T2 节点和 Renderer v1 不由本页自动批准。
