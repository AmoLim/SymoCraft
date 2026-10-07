---
type: 功能
status: 正式计划，准备已授权，实施待评审授权
project: Symocraft
module: M3-T2-Platform-SDL3
created: 2026-10-07
updated: 2026-10-07
tags:
  - area/spec
---

# M3-T2 GLFW 到 SDL3 迁移正式计划

本计划按用户已填写的决策表确定迁移范围：D01 顺序已确认，D02–D09 采用 A，D10 采用 B，D11–D12 采用 A。将窗口与键鼠平台实现从 GLFW 更换为 SDL3，保留现有 OpenGL 玩法、模块边界和 Benchmark 协议，完成 GL、无 GL 原生窗口及 Vulkan 窗口桥接验证，再交接后续 T3 渲染器重构。

方案选择已确定；用户于 2026-10-07 授权按计划继续 T2 准备，生产迁移仍需 P06 评审后授权，节点验收尚未执行。短采样参数与性能回归预算仍须按本计划确认，不能用正式计划的存在代替实测。原 draft 及已填写选择完整保存在[附录归档](#附录归档)，不再独立维护。

关联入口：[项目范围](project-scope.md)、[T0 模块边界](M3-T0-模块软硬边界.md)、[T1 世界模块](M3-T1-世界模块重构.md)、[T3 渲染器草稿](M3-T3-渲染器重构.md)、[T1 到 T3 CPU 契约清单](M3-T1-T3-CPU契约清单.md)、[当前平台功能](../project/architecture/platform/Platform-窗口与输入-功能.md)、[Window 类](../project/architecture/platform/Window-类设计.md)。

## 当前设计

当前生产实现仍使用 GLFW。窗口公开头已采用 PImpl，不暴露 GLFW 类型；替换主体集中在 platform 私有实现，不需要重写世界、模拟或 GPU 绘制算法。

| 已有能力 | 当前实现与迁移起点 | 权威入口 |
| --- | --- | --- |
| 窗口与上下文 | 静态 Init/Free、GL 4.6 Core、4x MSAA、Debug context、居中、标题/尺寸和创建失败清理 | [window.h](../../game/modules/platform/include/symocraft/platform/window.h)、[window.cpp](../../game/modules/platform/src/window.cpp) |
| 键鼠与焦点 | 11 个项目按键、sticky keys、有序 `PointerEvent`、快照借用、失焦清理和恢复首个鼠标位置 | [Window 契约](../project/architecture/platform/Window-类设计.md)、[key_snapshot](../../game/modules/platform/include/symocraft/platform/key_snapshot.h) |
| 图形交接 | 私有 GraphicsBridge 仅提供 GL 当前化、过程地址、扩展、VSync 和交换缓冲；没有真实 D3D12/Vulkan 路径 | [桥接头](../../game/modules/platform/src/graphics_bridge/graphics_bridge.h)、[renderer](../../game/modules/renderer/src/renderer.cpp) |
| Benchmark 窗口 | 实际像素尺寸、普通无边框、非独占/非置顶、NonRudeHWND、strict/allow-unfocused 和输入隔离 | [应用编排](../../game/app/src/application.cpp)、[冻结协议](../project/benchmark/exp/Benchmark-基准冻结-20261005.md) |
| 依赖与构建 | platform 私有链接 GLFW；CPU-only 和独立 benchmark 不引入窗口库；边界规则尚未覆盖 SDL3 | [第三方集成](../../cmake/ThirdPartyGame.cmake)、[模块边界](../../cmake/ModuleBoundaries.cmake)、[构建矩阵](../../test/integration/VerifyBuildMatrix.cmake) |
| 工具与测试 | runner 按 PID 查找 HWND 并发送 WM_CLOSE；平台单测只测纯 KeySampling，不创建真实窗口 | [runner 平台入口](../../tools/benchmark/src/platform.cpp)、[工具边界](../../test/integration/verify-tool-boundaries.cmake)、[平台测试](../../test/unit/platform/CMakeLists.txt) |

T1 技术交付记录见 [T1 报告](../milestones/m3-t1/README.md)。用户于 2026-10-07 明确：树叶剔除和视角突变不属于 T1 spec，也不属于 T2 前置范围，两项继续延期，直接继续 T2 准备。P01/P04 保留旧问题记录，用于区分已有现象与迁移新增回归；不得以两项延期问题阻断准备，也不把本次准备授权解释为修复或验收这两项问题。

### 公开接口预期行为

下表是迁移入口与变更索引。现行签名、借用期和不变量以 Window/Platform 笔记为准；准备评审通过后补入必要的候选增量，实施验证后才转为当前设计。

| 入口 | 调用方 | 迁移后的要求 | 前提与失败处理 | 权威契约 |
| --- | --- | --- | --- | --- |
| Init/Free、Create/Destroy | app 主线程 | 维持单窗口调用路径；为三种用途增加项目中立创建模式，不向调用方泄漏 `SDL_Window`、`SDL_GLContext`、HWND 或 Vk 类型 | Create 失败不发布半初始化窗口；重复清理安全；新增模式签名在 P05/P06 评审冻结 | [Window 公开接口](../project/architecture/platform/Window-类设计.md#公开接口预期行为) |
| `PollInt`/`CaptureInput`/Input/`ResetInput` | app 同步消费 | scancode 映射、每键采样、短按锁存、有序指针事件和快照借用期成立；`ResetInput` 清掉旧输入和恢复基准 | 输入分配失败具名报告并走退出清理，不当作成功空输入 | [Window](../project/architecture/platform/Window-类设计.md)、[Platform API](../project/architecture/platform/Platform-namespace-API.md) |
| Focused/Minimized/ShouldClose/Close | app 帧循环 | 保留焦点、暂停和关闭语义；同时处理全局退出与对应窗口的关闭请求 | 事件处理不直接销毁 GPU 或修改 ECS；恢复首帧 dt=0 | [平台功能](../project/architecture/platform/Platform-窗口与输入-功能.md) |
| WaitEvents/Time/SetCursorMode | app 主线程 | 等待不吞事件；单调高精度时间转为秒；Lock/Hidden/Normal 可正确切换和恢复 | 秒到等待单位的转换有明确边界；无积压运动、卡键或重复消费 | [Window](../project/architecture/platform/Window-类设计.md) |
| SetSize/SetTitle、width/height、GetAspectRatio | app/采样 | 标题与尺寸行为保留；逻辑窗口尺寸和实际像素分别处理，公开像素语义不静默改变 | 零尺寸表示不可绘制；resize 只记录事实，图形处理在帧边界 | [Window](../project/architecture/platform/Window-类设计.md) |

### 私有函数预期行为

| 入口 | 内部调用方 | 迁移要求与所有权 | 失败处理与源码落点 |
| --- | --- | --- | --- |
| `Window::Impl` 与事件适配器 | platform | 独占系统窗口、GL context、输入锁存及事件缓冲；按窗口 ID 转换为项目中立数据，不直接操作相机/世界 | 中途失败释放已取得资源；[window.cpp](../../game/modules/platform/src/window.cpp) |
| GraphicsBridge 的 GL 入口 | 现有 renderer | 正确当前化、GLAD 函数地址适配、扩展/VSync/交换缓冲；不拥有调用方 GPU 资源 | SDL 返回失败时具名诊断，不沿用无成功值入口掩盖必要失败；[桥接头](../../game/modules/platform/src/graphics_bridge/graphics_bridge.h) |
| 原生窗口与 Vulkan 桥接 | 私有探针、后续 T3 后端 | HWND 仅借用；所需扩展、instance/surface/loader 交接与销毁责任明确，SDK 类型只在授权私有接口/测试中出现 | 真正创建及销毁探针对象；缺驱动或创建失败清楚退出，不能 mock 为通过；[platform](../../game/modules/platform/CMakeLists.txt) |
| SDL 依赖装配、入口与部署 | CMake/platform 私有实现 | 固定源码静态链接，不向 CPU 消费者传播 SDL 头或 target；普通 main 保留 | 单独配置与安装检查均覆盖；[第三方集成](../../cmake/ThirdPartyGame.cmake)、[应用安装](../../game/app/CMakeLists.txt) |

## 本次变更

### 目标与流程

执行顺序为 T1 技术交付 → T2 准备授权与实验 → 准备评审及实施授权 → T2 迁移与节点验收 → T3-R1 冻结 → T3 渲染实施 → T4 应用/平台/模拟 → T5 ECS/内存及整体验收。当前已获用户直接继续 T2 准备的授权，不再将上述两项延期问题或重新确认它们的 T1 验收作为准备门槛。原 T2 渲染任务现为 T3；M2 历史编号和 M4/M5/M6 不变，不重复顺延已经调整的 T4/T5。

正常路径为：明确窗口用途 → 创建 SDL 窗口及所需上下文 → 私有桥接交接 → 事件转换与快照消费 → 既有 OpenGL 模拟/绘制/呈现 → 双机回归与短采样 → 逆序关闭。任何不可恢复失败都停止继续提交，诊断后有界清理，不隐式改库、换后端或降低 GL/MSAA 配置。

### 已确定的决策

决策依据是[归档中的已填写表](#归档---需要用户决策的计划表)。已选方向不再列为待选；实验推翻方向时，先提交变化和影响供用户确认。

| 编号 | 用户选择 | 正式采用的方案 |
| --- | --- | --- |
| D01 | 已确认 | SDL3 为新 T2，先迁移验收再冻结 T3 渲染器；后续为 T4/T5 |
| D02 | A | 只替换平台实现与必要资源清理；不合并完整 Application/SimulationSession 或 Runtime 对象化 |
| D03 | A | 必做 GL、无 GL 原生 HWND、SDL Vulkan 三种窗口创建模式及真实桥接探针，不实现 D3D12/Vulkan 游戏后端 |
| D04 | A | 官方 SDL release-3.4.18 源码纳入 vendor；记录完整 commit、源码摘要与许可证，支持离线源码构建 |
| D05 | A | 静态链接 SDL3，显式配置 `SDL_STATIC=ON`、`SDL_SHARED=OFF`；不增加 `SDL3.dll`，保持既有 MSVC runtime 策略 |
| D06 | A | 项目枚举映射 SDL scancode，保留物理位置操作；核对不同布局、短按锁存、长按及重置 |
| D07 | A | 以 GLFW 实测手感为基线，保留运动符号、灵敏度、焦点与恢复行为，不默认切换为重新校准的原始输入 |
| D08 | A | 区分逻辑尺寸和实际像素；Benchmark 保持实际 1920×1080，不引入增加画面负载的 DPI 策略 |
| D09 | A | 必验为开发桌面与 Y9000P；GTX 1650 不新增为 T2 必验机器 |
| D10 | B | 只做短采样诊断及协议回归，正式九轮 GLFW/SDL3 对照延后；不据此宣称性能等价或 M4 达标 |
| D11 | A | 验收后移除活动构建/部署的 GLFW 依赖；保留休眠旧代码、vendor/lib 材料与历史证据供 T5 审计 |
| D12 | A | 先评估同条件预采样波动，再提出 P95/P99、首次可操作时间及内存预算，提交用户确认；预算未确认不能伪造通过，正确性回归始终阻断 |

### 迁移范围与工程边界

| 区域 | 本次工作 | 不纳入本次 |
| --- | --- | --- |
| platform 私有实现 | SDL video 生命周期、窗口/context、键鼠、焦点、等待/时间、DPI/像素、Benchmark 窗口策略 | 多窗口、音频、手柄、IME/UI、跨平台和线程池 |
| 私有图形桥接 | 保留真实 OpenGL 呈现；实现无 GL HWND 交接和 SDL Vulkan 扩展/instance/surface/loader 探针 | D3D12/Vulkan 设备绘制、交换链、shader、资源上传与同步体系 |
| app/renderer | 仅作三种窗口用途和失败释放所需的窄适配；现有 GL 呈现编排保持 | Renderer PImpl、统一 Render 呈现门面、完整应用/模拟对象重构 |
| 构建、测试、安装 | 固定 SDL 静态源码、头边界、可选探针配置、真实平台回归与许可证 | `SDL_Renderer`/`SDL_GPU`、公开 SDK 类型、长期双平台后端框架 |
| CPU 数据与 Benchmark | 保留网格、世界摘要、固定模拟、统计边界与文件协议 | 纹理数组/中立相机定稿、性能协议升级、新性能指标或基准重采 |

公共头不得包含 SDL3/GLAD/Windows/D3D12/Vulkan 类型，不能用公开 `void*` 作为原生句柄逃生口。SDL 仅进入授权实现；ModuleBoundaries、VerifyBuildMatrix、verify-tool-boundaries 同步加入 SDL3 正向授权和负向用例。CPU-only、根构建 runner-only、`tools/benchmark` 单独配置均不配置或链接 SDL3。

普通 main 和应用主循环保留。入口处理放 platform 私有实现，准备阶段验证 `SDL_MAIN_HANDLED`/`SDL_SetMainReady` 路径，不让 app 因入口处理包含 SDL 头，不使用 `SDL2main` 或改成回调式主入口。[SDL3 入口说明](https://wiki.libsdl.org/SDL3/README-main-functions)

### 窗口桥接与生命周期

| 模式 | 窗口与资源责任 | 本次完成证据 |
| --- | --- | --- |
| OpenGL | OpenGL 窗口与显式 GL context 分别拥有；创建前设置 4.6 Core、4x MSAA 和 Debug 属性；现有 renderer 经私有桥接加载 GLAD/绘制/呈现 | 真实游戏首帧、实际 GL 属性、VSync 请求/诊断、截图/GPU 计时、失败与退出 |
| Native | 普通无 GL/Vulkan 标记的窗口；私有读取 Win32 HWND，句柄借用不延长窗口寿命 | 真实窗口/有效 HWND、事件/像素与失败清理；不是 D3D12 交换链或玩法验收 |
| Vulkan | `SDL_WINDOW_VULKAN` 窗口；私有取得必要扩展，真实创建 instance/surface，明确 loader 入口来源与清理责任 | 扩展/instance/surface 成功和失败路径、销毁及 loader 生命周期；不是 Vulkan 游戏后端验收 |

GL 清理顺序为 renderer 图形资源 → GL context → Window → SDL。Vulkan 使用同一 loader 来源的入口；先结束在途工作并清理相关对象、surface/device/instance，再销毁窗口和退出 SDL。SDL Vulkan 窗口会加载库、销毁窗口时对应卸载，所有 Vulkan 调用必须在可用 loader 寿命内；不能把静态 SDL 推导为静态 Vulkan loader。[GL context](https://wiki.libsdl.org/SDL3/SDL_GL_CreateContext)、[窗口与 loader](https://wiki.libsdl.org/SDL3/SDL_CreateWindow)

Vulkan 探针在明确启用的测试配置中要求 SDK 与可用运行环境；OpenGL-only、Native 运行、CPU-only 和独立工具不强制要求 Vulkan SDK/驱动。未选 Vulkan 时不得创建带 Vulkan 标记的窗口，也不能让缺失 `vulkan-1.dll` 在参数解析前阻断普通启动。SDL3 3.4.18 为固定候选版本，实际源码身份由 P02 留证，不凭版本号填写未取得的 commit/hash。[官方发行](https://github.com/libsdl-org/SDL/releases/tag/release-3.4.18)

### 输入与窗口行为

- 项目 Key 映射 SDL scancode；W/A/S/D、Space、Shift/Ctrl/CapsLock、E/Q/Escape 每个键按现行契约消费，不用 SDL 键码直接驱动玩法。短按与同帧按放要锁存到一次 `CaptureInput`，不能只查最终 `SDL_GetKeyboardState` 而丢失边沿。[键盘状态](https://wiki.libsdl.org/SDL3/SDL_GetKeyboardState)
- `PointerEvent` 保留原顺序，运动向右/向上为正；SDL 运动按 xrel 与反向 yrel 适配，滚轮保留浮点值并核对 direction，不量化为整数或合并成总 delta。鼠标手感及系统缩放/加速度策略必须和 GLFW 基线对照。
- 失焦、最小化、`ResetInput` 与锁鼠标切换清掉旧键和运动，恢复首帧 dt=0，不重复消费或引入新跳视角。相对模式切换会清 pending motion，必须冻结事件适配与切换顺序。[相对鼠标模式](https://wiki.libsdl.org/SDL3/SDL_SetWindowRelativeMouseMode)
- 等待若取得首个事件，必须送入同一适配器或暂存后消费一次；不能丢弃暂停时的关闭/焦点/按键。全局 `SDL_EVENT_QUIT` 与匹配 windowID 的 `SDL_EVENT_WINDOW_CLOSE_REQUESTED` 均转为关闭请求。[等待语义](https://wiki.libsdl.org/SDL3/SDL_WaitEventTimeout)
- 尺寸/焦点事件只更新事实，不执行 GPU 命令。窗口逻辑尺寸与 drawable 像素分别查询，100%/150%/200% 缩放与可用跨屏场景留证；零尺寸不绘制，Benchmark 实际像素维持 1920×1080。
- Benchmark 保持普通无边框、非独占/非置顶、NonRudeHWND 任务栏策略或经验证确认的等价实现；strict/allow-unfocused 口径不变。仅 Escape/关闭影响进程，玩家键鼠不接入 static/walk/edit 脚本负载。
- `PollInt` 仍计入 `event_ms`，`CaptureInput` 仍计入 `simulation_ms`；事件适配导致两段间成本迁移时单独说明，不当作性能改善。计时单位仍为秒，不变更 CPU/GPU/present 统计定义或文件协议。

### 准备步骤与进入条件

本轮已完成方案选定与计划编写；下表是准备工作，不再要求重新填写决策表。准备实验已获用户授权；生产切换需在 P06 审核后另获实施授权。实际进度、三模式探针及限制见 [T2 准备记录](../milestones/m3-t2/README.md) 与 [平台候选契约](../milestones/m3-t2/platform-contract-candidate.md)，下表只在相应完整条件成立后勾选。

| 状态 | 步骤 | 工作与产物 | 进入下一步的条件 |
| --- | --- | --- | --- |
| [ ] | P01 GLFW 基线 | 保存实际源码输入（含未提交修改）、Debug/Release 与安装包身份、GL 属性、键鼠/焦点/最小化/退出记录和旧问题复现 | 可构建同源窗口库对照；HEAD 不代替脏工作区身份，不覆盖 M2 冻结数据 |
| [ ] | P02 依赖与构建实验 | 取得 release-3.4.18 官方源码，登记完整 commit/hash/license；验证 MSVC x64、项目 CMake 3.22、SDL 静态选项、普通 main、离线及安装 | CPU-only/工具独立不引入 SDL；GL-only 不要求 Vulkan；依赖和部署策略可复现 |
| [ ] | P03 三模式窗口探针 | GL 4.6 Core/GLAD/4x MSAA/VSync；Native HWND；真实 Vulkan 扩展/instance/surface/loader；居中、焦点、隐藏后显示、任务栏和像素 | 三模式能力与失败清理有真实结果，不以 mock 代替；准备探针只证明窗口/库能力，不能代替 S2/A02 真实游戏 GL 回归或 T3 后端验收 |
| [ ] | P04 输入对照 | 11 键及不同布局、短按锁存/重复/长按、有序鼠标/浮点滚轮、等待/关闭、锁鼠标/失焦/最小化恢复 | GLFW→SDL 映射表、锁存重置与模式切换规则成立；保留旧视角问题边界 |
| [ ] | P05 契约与失败评审 | 窗口中立创建模式及签名、主线程、事件借用、像素语义、GL/Native/Vulkan 所有权、逐点失败和重复清理 | Window/Platform/构建候选增量与真实探针一致；不建立第二套完整 Runtime 或公开 SDK |
| [ ] | P06 实施进入评审 | 汇总 P01–P05；确定短采样参数和 D12 预算确认安排，审核候选接口、验收矩阵、未完成项及证据清单 | 准备限制已解决或由用户明确确认；批准实施后才能开始 S1–S5，不把准备通过算作 T2 通过 |

准备实验位于根级 test 的相应目录，独立 SDL API 探针可直接链接候选依赖；生产回归必须链接真实 platform 实现。证据进入 `docs/milestones/m3-t2/`，包和大体量采样留 out/。P01 生成的 GLFW 基线在活动依赖清理前保存，回退只使用已知同源输入，不自动删除或重置工作区。

### 短采样与预算确认

D10 已选 B，正式三场景各三轮的 GLFW/SDL3 性能对照不属于 T2 必验，也不要求本轮重新采集 M2 冻结基准。T2 仍在开发桌面与 Y9000P 上做同条件短采样和协议回归，覆盖 static/walk/edit；短时预热、采样时长、次数和异常复测规则在 P06 记录，不能把原正式九轮参数自动移作短采样要求。

两版必须来自同一实际源码输入及相同 MSVC/构建、资源、seed 424242、441 区块、regression v1/workload v2、实际 1920×1080、4x MSAA、VSync 关闭请求及 allow-unfocused 策略，除窗口实现与必需适配外不混入其它重构。记录实际 GPU、驱动、供电/性能模式与焦点/最小化/尺寸状态，保留原始帧数据、摘要、CPU/GPU/present 统计和缺失原因。短采样的样本数、波动和 P99 解释范围必须同时报告。

D12 已选 A，但数值尚未选定：先用同条件基线预采样评估自然波动，提出 P95/P99、首次可操作时间、`working set/private bytes` 的候选预算和复测规则；用户确认后才按预算作迁移判定，不根据最终 SDL 成绩反推通过线。预算尚未确认或超出预算时暂停节点验收，先定位/复测或请求明确调整；不自动扩大为正式九轮，也不自动回滚用户改动。

短采样通过只支持本次诊断与约定回归检查，不支持性能等价、性能提升、完整基准成立或 M4 达标结论。不可恢复故障、输入丢失、资源泄漏、协议不符和画面规格下降不由性能预算放宽。

### 实施步骤与完成条件

每一步保留可构建中间状态，不长期保留 GLFW/SDL 双生产实现；新性能或输入需求需另行确认。

| 状态 | 步骤 | 工作内容 | 完成门槛 |
| --- | --- | --- | --- |
| [ ] | S1 依赖隔离 | 接入固定 SDL 静态源码/target、入口、许可证及安装；更新 ModuleBoundaries、VerifyBuildMatrix、verify-tool-boundaries 的防泄漏规则 | Debug/Release、CPU-only、根 runner-only、工具独立、game-only、`BUILD_TESTING=OFF` 及负向头检查通过 |
| [ ] | S2 窗口与桥接 | 替换 `Window::Impl`、初始化/时间/等待、GL context 及私有桥接；落实 GL/Native/Vulkan 三种模式和逆序失败清理 | 真实 GL 首帧/交换/退出，HWND 及 Vulkan instance/surface 探针、失败和重复清理均有证据 |
| [ ] | S3 输入与窗口行为 | SDL 事件转中立快照/有序指针事件，落实锁存、恢复、关闭、DPI/像素和 Benchmark 策略 | 普通玩家输入与脚本负载隔离分别通过，真实 platform 回归不是纯 KeySampling 替身 |
| [ ] | S4 T3 契约交接 | 同步已验证的三模式/借用期/像素/loader 约定及候选接口；T3 不直接消费 SDL 公开类型 | T3-R1 有可核对的平台输入；GPU 后端及 `TextureArrayData`/中立相机仍归 T3 |
| [ ] | S5 双机与交付 | 双机人工/安装/故障/runner 取消、短采样及预算判定；停止活动 GLFW 构建/部署，更新实际平台、构建和依赖说明 | 下表全部必验有结果，预算与限制已确认，交付候选包/证据后申请 T2 节点批准 |

D11 的清理是撤除活动链接、配置、安装与包许可中的 GLFW 项，不删除 `vendor/glfw`、`lib/glfw`、休眠旧代码或历史证据；依赖清单必须区分“当前使用”和“历史保留”。文件删除及旧系统审计归 T5 或单独授权。本轮不创建空验收报告、不伪填 commit/hash、测试结果或勾选。

### 验收案例

所有项目均未执行。环境为 Windows x64、MSVC、项目 CMake 配置及固定 SDL 静态源码；人工必验为开发桌面和 Y9000P，每项结果绑定实际源码/候选包身份。

| 状态 | 场景 | 预期行为与证据 |
| --- | --- | --- |
| [ ] | A01 构建与头边界 | 全新 Debug/Release、CPU-only、根 runner-only、`tools/benchmark` 单独配置、game-only、`BUILD_TESTING=OFF` 通过；公开 API/CPU 模块越界包含 SDL 或私有 SDK 被拒绝 |
| [ ] | A02 真实 OpenGL | 实际 GL 4.6 Core、4x MSAA、GLAD、VSync 请求/诊断、世界纹理/选择框、截图及 GPU 计时正常，不静默降规格 |
| [ ] | A03 真实键鼠 | 链接真实 platform 的短按/同帧按放、重复/长按、scancode 布局映射、两鼠标键、运动符号/灵敏度及有序浮点滚轮通过；纯 KeySampling 单测不替代 |
| [ ] | A04 焦点与暂停 | 按住键切出/切回、锁鼠标切换、最小化/等待/恢复及关闭不丢失或重复事件，无卡键/积压/迁移新增跳视角；恢复 dt=0 |
| [ ] | A05 尺寸与显示器 | 100%/150%/200% 缩放、resize 和可用跨屏场景，逻辑/像素正确区分；零尺寸不绘制，事件处理不提交 GPU 命令 |
| [ ] | A06 Benchmark 窗口与输入隔离 | 普通无边框、非独占/非置顶、任务栏、实际 1920×1080/VSync 关闭请求成立；strict/allow-unfocused 正确，最小化/尺寸变化仍无效；仅 Escape/关闭生效，玩家键鼠不污染脚本负载 |
| [ ] | A07 寿命与失败 | SDL 初始化、窗口、GL context/加载、事件缓冲分配、三模式桥接逐点失败具名诊断并逆序释放；重复清理安全，无 loader 卸载后的 SDK 调用 |
| [ ] | A08 Native/Vulkan 实桥接 | 真实 HWND、Vulkan 必需扩展/instance/surface 创建销毁与 loader 入口来源/寿命通过；缺运行环境清楚失败，GL-only 与工具不被未选 Vulkan 阻断；不冒称 T3 现代后端完整玩法接管验收 |
| [ ] | A09 安装与独立启动 | 无 `SDL3.dll`；脱离源码/IDE、不同工作目录、中文/空格路径、损坏资源/运行依赖缺失可用或清楚退出；runner WM_CLOSE 在强制结束前正常落盘与清理 |
| [ ] | A10 双机人工玩法 | 两台机器分别完成移动/跳跃/碰撞、选择/放置/破坏、窗口恢复/正常退出及进入可操作世界后的至少 15 分钟连续游玩；旧延期问题单列，不由有限帧运行替代 |
| [ ] | A11 协议与短采样 | 同源 GLFW/SDL3 双机三场景短采样，摘要/seed/区块/workload/结果字段及 CPU/GPU/present 口径保留；原始数据与采样条件齐全，不要求正式九轮，不覆盖冻结数据 |
| [ ] | A12 预算与活动依赖 | D12 候选预算/复测规则由用户确认后判定，有异常说明；活动构建/安装/候选包不依赖 GLFW，休眠材料仍留存，不宣称性能等价或 M4 通过 |

A01–A12 全部有结果、未执行/失败项未被隐藏、性能预算和限制明确、源码与包身份可追溯后，才申请用户批准 T2 节点。批准后进入 T3-R1；三模式桥接验证不能自动批准 T3/T4/T5，正式九轮对照的延期项继续保留。

## 后续考虑

| 触发条件 | 后续工作 |
| --- | --- |
| T2 节点验收通过 | T3-R1 先通过 D3D12 真实实验冻结 Renderer CPU 输入及 SDL3 桥接使用约定，再以 PImpl 让 D3D12、Vulkan 依次接管；两个现代后端都需独立完整玩法/部署/失败验收后才批准 T3。OpenGL 仅作过渡回归，最终退役留后续，详见 [T3 阶段决策](M3-T3-渲染器重构.md#已确定的阶段决策) |
| 完整应用与模拟设计开始 | T4 实施 Runtime/Window、Application/SimulationSession 等完整对象生命周期；不因 SDL C API 建立无必要管理器 |
| ECS/内存及休眠材料审计开始 | T5 审计或清理确认无调用的旧系统/GLFW 材料；历史证据不改写 |
| 正式性能研究或优化开始 | 单独安排延后的正式九轮同条件 A/B 对照或 M4 基准，不把本次短采样结论升级为性能等价 |
| 有明确音频/手柄/中文文本 UI/多窗口需求 | 单独确认 SDL 子系统、公开契约、部署与验收，不从已接入窗口库推定功能已支持 |

## 附录归档

归档日期：2026-10-07。来源为原 docs/spec/M3-T2-SDL3迁移-T2-draft.md 在正式化前的完整内容，包含 D01–D12 的已填写选择与全部 A/B 候选。独立 draft 归入本文后删除旧入口，后续修改正式正文，不继续维护第二份活动计划。

为与仓库 T1 归档格式一致，原 YAML 属性放入代码块，原标题统一加“归档 - ”并下移两级，其余原文保留。原草案的“待选择”、旧编号、D03/B、D10/A 九轮建议、示例填写和状态描述都是历史快照，不覆盖上方已选方案，也不构成新的授权或验收通过。

返回[已确定的决策](#已确定的决策)、[准备步骤与进入条件](#准备步骤与进入条件)、[验收案例](#验收案例)。

### 归档元数据

```yaml
---
type: 功能
status: 草稿，待用户决策
project: Symocraft
module: M3-T2-Platform-SDL3
created: 2026-10-07
tags:
  - area/spec
---
```

### 归档 - T2-draft GLFW 到 SDL3 迁移计划

本计划将窗口与键鼠平台实现从 GLFW 更换为 SDL3，保留现有 OpenGL 玩法、模块边界与 Benchmark 协议，并为后续 T3 的 D3D12/Vulkan 私有窗口桥接准备契约。用户已确认把现有 SDL3 草案前移为新 T2，原 T2 渲染器重构后移为 T3，先完成迁移再冻结渲染器正式 spec。迁移方案仍待决策；本轮只调整文档，不下载依赖、不改游戏代码、不执行迁移实验。

关联入口：[项目范围](project-scope.md)、[T3 渲染器草稿](M3-T3-渲染器重构.md)、[T3 CPU 契约清单](M3-T1-T3-CPU契约清单.md)、[当前平台功能](../project/architecture/platform/Platform-窗口与输入-功能.md)、[Window 类](../project/architecture/platform/Window-类设计.md)。

#### 归档 - 当前设计

GLFW 调用集中在 [window.cpp](../../game/modules/platform/src/window.cpp)，窗口公开头已经使用 PImpl，不暴露 GLFW 类型。替换主要发生在 platform 内部，但需要同步构建、renderer 私有桥接、真实运行测试和部署，不是只改几个函数名。

| 现有能力 | 迁移必须核对的内容 | 权威入口 |
| --- | --- | --- |
| 初始化与窗口 | 静态 Init/Free、OpenGL 4.6 Core、4x MSAA、Debug context、显示器居中、标题/尺寸、创建失败清理 | [Window](../../game/modules/platform/include/symocraft/platform/window.h)、[实现](../../game/modules/platform/src/window.cpp) |
| 事件与输入 | 有序 PointerEvent、键盘快照、GLFW sticky keys、鼠标按钮、滚轮、失焦清理、恢复首个鼠标位置 | [Platform API](../project/architecture/platform/Platform-namespace-API.md)、[key_snapshot](../../game/modules/platform/include/symocraft/platform/key_snapshot.h) |
| 图形交接 | 当前私有桥接只实现 GL 当前化、过程地址、扩展、VSync 和交换缓冲；没有真实 D3D12/Vulkan 路径 | [GraphicsBridge](../../game/modules/platform/src/graphics_bridge/graphics_bridge.h)、[renderer](../../game/modules/renderer/src/renderer.cpp) |
| Benchmark 窗口 | 实际 framebuffer 像素、普通无边框而非独占全屏、不置顶、NonRudeHWND 任务栏策略、allow-unfocused | [应用编排](../../game/app/src/application.cpp)、[冻结协议](../project/benchmark/exp/Benchmark-基准冻结-20261005.md) |
| 构建与保护 | platform 私有链接 glfw；CPU-only 和独立 benchmark 不引入窗口库；扫描器目前识别 GLFW，不识别 SDL3 | [第三方集成](../../cmake/ThirdPartyGame.cmake)、[边界检查](../../cmake/ModuleBoundaries.cmake)、[平台测试](../../test/unit/platform/CMakeLists.txt) |
| 独立工具与测试 | runner 按进程查找 HWND 后发送 WM_CLOSE；当前平台单元测试只测纯 KeySampling，不创建真实窗口 | [取消入口](../../tools/benchmark/src/platform.cpp)、[构建矩阵](../../test/integration/VerifyBuildMatrix.cmake)、[工具边界](../../test/integration/verify-tool-boundaries.cmake) |

T1 技术交付完成但节点仍待批准；已知树叶剔除和视角突变按用户要求延期，见 [T1 报告](../milestones/m3-t1/README.md)。迁移前应复现并记录视角问题，避免把旧问题算成 SDL3 回归，或把更换鼠标输入误称为修复。树叶问题不纳入窗口库迁移。

##### 归档 - 公开接口预期行为

本表只索引现行契约；具体签名、借用期和失败保证仍由平台笔记维护，不在本草稿另造一份 API。

| 入口 | 迁移关注点 | 权威契约 |
| --- | --- | --- |
| Window 创建/销毁、尺寸/焦点/退出 | 原调用方不接触 SDL_Window、SDL_Event 或 HWND；是否新增创建模式由 D03 决定 | [Window 公开接口](../project/architecture/platform/Window-类设计.md#公开接口预期行为) |
| PollInt/CaptureInput/Input/ResetInput | 快照借用期、有序指针事件、按键采样和暂停恢复边界不静默改变 | [Platform API](../project/architecture/platform/Platform-namespace-API.md)、[Window](../project/architecture/platform/Window-类设计.md) |
| WaitEvents/Time/SetCursorMode | 等待不会吞事件，计时单调且单位为秒，Lock/Hidden/Normal 的状态可恢复 | [Window](../project/architecture/platform/Window-类设计.md) |

##### 归档 - 私有函数预期行为

| 入口 | 迁移关注点 | 源码落点 |
| --- | --- | --- |
| GraphicsBridge | GL 操作需要正确当前上下文；SDK/原生类型仅在授权的私有桥接中交接 | [私有桥接头](../../game/modules/platform/src/graphics_bridge/graphics_bridge.h) |
| Window::Impl 与事件适配 | SDL 事件先转换为项目中立数据，不直接修改 ECS/相机/世界；按窗口 ID 过滤 | [window.cpp](../../game/modules/platform/src/window.cpp) |
| 依赖装配与边界扫描 | SDL3 的头路径/target 不传播给 CPU 消费者；禁用的图形后端不要求对应 SDK | [第三方集成](../../cmake/ThirdPartyGame.cmake)、[边界检查](../../cmake/ModuleBoundaries.cmake) |

#### 归档 - 本次变更

##### 归档 - 编号与执行顺序

| 原计划 | 调整后编号 | 任务范围 |
| --- | --- | --- |
| 现有 SDL3 T3-draft | 新 M3-T2 / T2-draft | GLFW 到 SDL3 的平台迁移，前置于渲染器正式 spec |
| 原 M3-T2 | M3-T3 | Renderer PImpl 与 OpenGL/D3D12/Vulkan |
| 原 M3-T3 | M3-T4 | 应用、平台与模拟接口及完整生命周期 |
| 原 M3-T4 | M3-T5 | ECS/内存契约与 M3 整体验收 |

表中的原 T3/T4 指插入 SDL3 任务前的应用/ECS 计划；它们已在前次草案中写为 T4/T5，本次保持 T4/T5，不再次顺延。M2 历史编号以及 M4/M5/M6 大阶段和其子任务不变。旧 T0/T1 的契约快照、附录和报告保留原编号，按本表解释；不改写历史证据。

**D01 已由用户确认，不再作为待选项。** 执行顺序为：T1 节点批准 → T2 准备实验与决策 → T2 正式 spec 审核及实施授权 → T2 迁移与节点验收 → T3-R1 冻结 → T3 渲染器实施 → T4 → T5。原渲染器草稿更名为 T3，设计内容继续使用；顺序确认不等于授权实验、下载依赖或开始实施。

##### 归档 - 需要用户决策的计划表

D01 已确认；D02–D12 均待选择。A 为建议默认值，不代表已经选定。先决定 D02–D05 的范围与依赖，再确定 D06–D08 的行为，最后确定 D09–D12 的验证与交付条件。

| 编号  | 你需要决定什么             | A 建议方案                                                                              | B 替代方案及代价                                                           | 选择  |
| --- | ------------------- | ----------------------------------------------------------------------------------- | ------------------------------------------------------------------- | --- |
| D01 | 编号与执行顺序             | 新 T2 SDL3 先完成，原 T2 渲染改为 T3，原应用/ECS 计划为 T4/T5                                        | 不再保留先渲染、后迁移的分支                                                      | 已确认 |
| D02 | 迁移是否合并对象重构          | 只替换平台实现及必要资源清理；完整 Application/SimulationSession 重构留 T4                              | 同时推进 Runtime/Window 生命周期对象化；需追加类契约与失败测试，扩大本次范围                      | A   |
| D03 | 为后续 T3 做到哪一层桥接      | 准备 GL、无 GL 的原生窗口、Vulkan 三种创建模式；验证 GL 呈现、HWND、真实 Vulkan instance/surface 清理，不实现新渲染后端 | 本次只做 OpenGL 等价替换，其他模式与 loader/surface 约定留 T3；范围较小，但 T3 定稿前仍须补真实桥接实验 | A   |
| D04 | SDL3 来源与固定版本        | 官方稳定源码纳入 vendor，固定 release-3.4.18、完整 commit/源码摘要及许可证；延续离线源码构建                       | 固定 URL/hash 的 FetchContent；仓库较小，但首次构建需要下载、缓存和离线准备                   | A   |
| D05 | SDL3 链接与随包部署        | 源码静态链接 SDL3；不增加 SDL3.dll，保持既有 MSVC runtime 策略                                       | 动态链接 SDL3；必须复制/安装正确 DLL，增加缺失/错版本与脱离开发环境测试                           | A   |
| D06 | 游戏按键映射              | 项目枚举映射 SDL scancode，保持物理位置操作；按当前 GLFW 实测映射核对不同布局，适配短按锁存、长按与重置                       | 映射 SDL keycode，按键随布局变化；属于明确行为选择，需确认非英文布局下操作并补测试                     | A   |
| D07 | 锁鼠标时的运动语义           | 先测 GLFW 手感并保留灵敏度/符号/焦点行为，必要时显式配置 SDL 相对输入策略                                         | 明确改为原始相对输入并重新校准；作为行为变更单独验收，不默认归类为等价迁移                               | A   |
| D08 | DPI 与 Benchmark 分辨率 | 区分窗口坐标和实际像素；正式 Benchmark 保持实际 1920×1080，验证缩放/跨屏，不顺带增加画面负载                           | 引入高像素密度策略；如实际分辨率/负载变化，需单独建立新对照条件，不能混入旧基准                            | A   |
| D09 | 必验机器范围              | 开发桌面 + 指定中端 Y9000P，沿用已有双机范围                                                         | 同时把 GTX 1650 加为 SDL3 迁移必验机器；需额外回收运行和人工结果，不自动扩展所有 M3 后端范围            | A   |
| D10 | 性能证据的门槛             | 先短采样诊断，再以同源码的 OpenGL GLFW/SDL3 对照包，按固定协议在 D09 所选机器做三场景各三轮；不覆盖冻结基准                   | 只做短采样及协议回归，正式九轮对照延后；不能据此宣布性能等价或 M4 达标                               | B   |
| D11 | 旧 GLFW 材料怎么处置       | 迁移验收后移除活动构建/部署对 GLFW 的依赖，保留休眠旧代码与历史材料供 T5 审计                                        | 同时删除无调用的 vendor/glfw、lib/glfw 等旧文件；先列删除清单、查全部引用，再单独确认，不提前删除回退依据     | A   |
| D12 | 性能回归预算与暂停条件         | 先用同条件预采样估计波动，再提交 P95/P99、首次可操作时间及内存的候选预算供你确认；确认前不宣称性能等价，正确性回归始终阻断                   | 现在指定允许增幅、绝对上限和异常复测规则；标准更早确定，但不能借用 M4 目标代替迁移预算                       | A   |

D04 的候选版本是 2026-10-02 发布的官方稳定修订版，正式引入前仍要做本项目编译/运行实验；不保证“最新即最合适”。见 [SDL 3.4.18 官方发行](https://github.com/libsdl-org/SDL/releases/tag/release-3.4.18)。D05 的静态 SDL 不等于静态 MSVC CRT，也不自动消除 Windows 系统或图形驱动运行依赖。D06 的 scancode 指物理键位，不是输入字符；映射以项目枚举为边界，不把 SDL 编号写入玩法层。[SDL scancode](https://wiki.libsdl.org/SDL3/SDL_Scancode)

**选择依赖：** D03/A 增加真实 Vulkan 探针所需的 SDK/运行环境，只在该实验配置启用；GL-only、CPU-only 和独立工具仍不得强制依赖 Vulkan。D03/B 的缺项进入 T3 的准备清单，不假称三后端窗口已经验证。D10/A 是每台必验机器每个窗口实现各九轮：D09/A 双机共 36 轮，D09/B 三机共 54 轮；D10/B 只能交付短采样结论。D12 的预算在正式采样前确认，超出预算先暂停节点验收、定位和复测，不自动删除工作区改动。

##### 归档 - 前期准备与产物

先填决策方向并授权准备实验，再完成 P01–P06，形成 T2 正式 spec；实施和节点验收通过后才冻结 T3 渲染器正式 spec。准备阶段允许在隔离实验配置验证候选库，不提前切换生产实现。

| 步骤 | 要准备什么 | 产物与进入下一步的条件 |
| --- | --- | --- |
| P01 当前 GLFW 基线 | 记录实际源码输入，包含未提交修改；保存 Debug/Release、真实 OpenGL、输入/焦点/最小化/退出、安装包身份与已知问题 | 可回到同一源码的窗口库对照；HEAD 不能代替脏工作区身份；不得覆盖 M2 冻结数据 |
| P02 依赖与构建实验 | 固定 SDL tag/完整 commit/hash/license；验证 MSVC x64、项目 CMake 3.22 最低声明与实际工具链、Debug/Release、main 入口与安装产物；按 D05 显式配置 SDL_STATIC/SDL_SHARED，不仅依赖 BUILD_SHARED_LIBS | 离线复现说明、依赖选项表；普通 main 的 SDL_SetMainReady 等入口处理留 platform 私有实现；CPU-only 与独立 benchmark 不配置/链接 SDL3，OpenGL-only 不强制 Vulkan SDK |
| P03 最小真实窗口实验 | 创建前设置 GL 属性，分别创建 SDL_Window 与 SDL_GLContext；验证 GL 4.6 Core + GLAD 函数地址适配 + 4x MSAA、VSync、像素尺寸；验证居中、隐藏创建后显示、焦点和任务栏；按 D03 验证原生/Vulkan 桥接 | 记录请求与实际 GL 属性、真实驱动、窗口像素及失败清理；GL context 不随窗口隐式创建；HWND/surface 探针不是 D3D12/Vulkan 游戏验收 |
| P04 输入行为对照 | 核对 W/A/S/D、Space、Shift/Ctrl/CapsLock、E/Q/Escape；短按/同帧按放、重复、运动 xrel 与反向 yrel、浮点滚轮及 direction、失焦按住及恢复；验证全局 QUIT 与本窗口 CLOSE_REQUESTED | GLFW→SDL 映射表、快照锁存/重置规则；不能只用 SDL_GetKeyboardState 丢失两次采样间的短按；等待取得的事件必须进入同一适配流程，恢复首帧 dt=0；相对模式切换清 pending motion，须定义消费顺序 |
| P05 所有权与失败冻结 | 明确主线程、SDL video 生命周期；GL 路径按 renderer 图形资源→GL context→window→SDL 释放，Vulkan 按 surface/device/instance/loader 契约清理；检查创建中途失败、事件缓冲分配失败和重复清理 | Window/Platform/构建笔记的候选增量，以及 T3 可借用的私有桥接契约；不引入公开 void* 兜底；静态 SDL 不推导为静态 Vulkan loader |
| P06 决策与正式化 | 填 D02–D12；汇总实验失败/复测/限制，冻结迁移边界、验收矩阵、回归预算与未完成能力 | 审核 T2 正式 spec，再授权实施；尚未冻结的纹理数组/相机 CPU 参数仍归 T3-R1，不在换库阶段提前定稿 |

准备实验应放在根级 `test/` 的相应目录。独立 SDL API 探针可直接链接候选依赖，但只证明库能力；生产回归链接真实 platform 实现，不用探针代替生产通过。证据进入后续 `docs/milestones/m3-t2/`，包与大体量数据留 `out/`。准备产物至少包含源码/包身份清单、依赖选项表、输入映射表、窗口/桥接实验记录、验收矩阵和已选决策。本轮不预建空报告目录，不填写通过结果，也不替用户提交当前工作区。

##### 归档 - 不由选项放宽的工程边界

- SDL3 只用于窗口、键鼠事件、计时/等待及必要图形窗口桥接。不引入 SDL_Renderer/SDL_GPU，不用 SDL 的渲染抽象替代 T3 的 OpenGL/D3D12/Vulkan，不增加音频、手柄、IME/UI、多窗口、跨平台或线程池。
- 公共头不包含 SDL3/GLAD/Windows/D3D12/Vulkan 类型；SDL 依赖仅授予授权实现。边界扫描增加 `SDL3/` 的正向授权和负向用例，不能因遗漏新头前缀而绕过 T0 边界。
- 窗口与事件在主线程处理；保留普通 main 和应用主循环，不无故改成 SDL 回调式主入口。`SDL_main.h`/`SDL_MAIN_HANDLED` 的选择在 P02 明确，不能照搬 SDL2 的 SDL2main 链接方法。[SDL3 初始化与入口说明](https://wiki.libsdl.org/SDL3/README-main-functions)
- SDL_Window 与 SDL_GLContext 分别拥有和清理；候选入口方案为 platform 私有实现使用 SDL_MAIN_HANDLED/SDL_SetMainReady，不要求 app 公开或直接包含 SDL 头。[GL context 创建](https://wiki.libsdl.org/SDL3/SDL_GL_CreateContext)、[普通 main 入口](https://wiki.libsdl.org/SDL3/SDL_MAIN_HANDLED)
- 逻辑窗口尺寸与像素尺寸分别处理，resize/像素尺寸事件只更新事实，图形命令在帧边界提交。SDL 的窗口坐标不保证等于 drawable 像素。[SDL 窗口创建说明](https://wiki.libsdl.org/SDL3/SDL_CreateWindow)
- GLFW sticky keys 不是 SDL 键盘状态查询的同义替换；有序指针事件不合并成一个总 delta，失焦清除和鼠标恢复不造成积压输入或跳视角。SDL 相对鼠标模式与绝对坐标差分需要实际对照。[键盘状态](https://wiki.libsdl.org/SDL3/SDL_GetKeyboardState)、[相对鼠标模式](https://wiki.libsdl.org/SDL3/SDL_SetWindowRelativeMouseMode)
- SDL_WaitEventTimeout 传空 event 指针时不取走事件；若传入 event 取走首个事件，必须送入相同适配器或暂存后消费一次，不能丢失暂停时的关闭/焦点/键盘事件。[等待语义](https://wiki.libsdl.org/SDL3/SDL_WaitEventTimeout)
- 当前 PollInt 计入 event_ms，CaptureInput 计入 simulation_ms。维持相同采样边界；若 SDL 事件适配使成本在两段间迁移，要单独记录，不能把统计搬家当成性能改善。
- Benchmark 模式只采纳 Escape 和窗口关闭；不把玩家移动/跳跃、选块、放置破坏、鼠标运动/滚轮接入玩法，static/walk/edit 继续由脚本工作负载控制。普通游戏的输入回归与 Benchmark 输入隔离分别测试。
- 若使用 `SDL_WINDOW_VULKAN`，窗口创建会加载 Vulkan library，窗口销毁会对应卸载。采用该路径时先等待设备/资源结束，销毁 surface/device/instance，再销毁窗口，最后退出 SDL；不得在 library 卸载后再调用 Vulkan 入口。D3D12 的 HWND 只借用至窗口销毁前。[窗口与 library 寿命](https://wiki.libsdl.org/SDL3/SDL_CreateWindow)、[Win32 窗口属性](https://wiki.libsdl.org/SDL3/SDL_GetWindowProperties)
- SDL3 并不自动提供 T3 三后端资源、shader、同步、交换链或能力检测。T2 保留现有 OpenGL 呈现编排，后续 T3 再统一收束到 Renderer；CPU 顶点、世界摘要、固定物理步和 Benchmark 文件协议不随换库修改。

##### 归档 - 正式实施拆分

以下是待批准的拆分，不表示已经开始；每一步保留可构建的中间状态，最终不长期保留 GLFW/SDL 双平台后端框架。

| 步骤 | 工作内容 | 完成门槛 |
| --- | --- | --- |
| S1 依赖隔离 | 接入固定 SDL3 源码/target、main 入口、许可证与安装规则；同步 ModuleBoundaries、VerifyBuildMatrix 与 verify-tool-boundaries 的 SDL 防泄漏规则；按启用模式分开 SDL、GLAD 与其他 SDK 的需求 | Debug/Release、CPU-only、根构建 runner-only、工具单独配置、game-only 和关闭测试配置通过，公开头/负向包含检查有效 |
| S2 窗口与桥接 | 替换 Window::Impl、初始化/等待/时间、GL context 和 GraphicsBridge；落实 D03 的模式与失败释放 | 真实 GL 首帧/交换/退出、窗口/上下文创建失败及重复清理有证据 |
| S3 输入与窗口行为 | SDL 事件转中立快照与 PointerEvent；实现失焦/最小化恢复、关闭、鼠标模式、DPI 与 Benchmark 窗口策略 | 下表输入/焦点/像素/任务栏用例通过，普通玩法与脚本化 workload 分别验证 |
| S4 T3 交接 | 同步私有桥接、模式、借用期与像素契约；更新原渲染器草稿对 GLFW/no-client-API 的前提 | T3 不直接消费 SDL 类型；必要真实 SDK 探针有结果，尚未实现的后端能力明确标出 |
| S5 回归与交付 | D09 所选机器的游戏/安装包、故障路径、Benchmark 对照；依 D11 清理活动依赖并更新平台/构建/依赖说明 | 全部所选必验项目有结果，再申请 T2 节点批准；不自动宣布 T3/T4/T5 或 M4 完成 |

##### 归档 - 验收案例

| 状态 | 场景 | 预期行为 |
| --- | --- | --- |
| [ ] | A01 构建与头边界 | 全新 Debug/Release、CPU-only、根构建 runner-only、tools/benchmark 单独配置、game-only、BUILD_TESTING=OFF 通过；SDL 头不能从公开 API 或 CPU 模块越界包含 |
| [ ] | A02 真实 OpenGL | 实际 4.6 Core、4x MSAA、GLAD、VSync 请求/诊断、世界纹理/选择框、截图及 GPU 计时正常；不静默降规格 |
| [ ] | A03 键鼠 | 新增链接真实 platform 的输入/事件回归，不能用现有纯 KeySampling 单测替代；短按/同帧按放、长按、重复、按键映射、两鼠标键、浮点滚轮与有序运动，符号/灵敏度/锁存符合已选契约 |
| [ ] | A04 焦点与暂停 | 按住键切出/切回、锁鼠标切换、最小化恢复、关闭事件；无卡键、积压事件、无因迁移新增的跳视角 |
| [ ] | A05 尺寸与显示器 | 100%/150%/200% 缩放、resize、可用的跨屏场景；像素与逻辑尺寸区分，零尺寸不绘制，不在事件处理内执行图形命令 |
| [ ] | A06 Benchmark 窗口与输入隔离 | 普通无边框、非独占/非置顶、任务栏、实际 1920×1080、VSync 关闭请求；strict/allow-unfocused 分别正确，最小化/尺寸变化仍无效；仅 Escape/关闭生效，玩家键鼠不污染脚本负载；不靠永久不可获焦窗口伪造失焦策略 |
| [ ] | A07 寿命与失败 | SDL 初始化、窗口、GL context/加载、事件缓冲分配和所选桥接失败可诊断，部分资源逆序释放；重复清理不泄漏/崩溃，退出无悬空 SDK 调用 |
| [ ] | A08 所选 T3 桥接探针 | 按 D03 验证无 GL 原生 HWND 与真实 Vulkan 扩展/instance/surface/loader 生命周期；关闭对应后端时无需其 SDK，不能用 mock 代替真实探针 |
| [ ] | A09 安装与独立启动 | 脱离源码/IDE、不同工作目录、中文/空格路径、损坏资源与运行依赖缺失清楚退出；runner 按 PID 发送 WM_CLOSE 后在强制结束前走正常退出、落盘与清理；D05 动态方案另验 SDL3.dll 部署 |
| [ ] | A10 所选硬件与玩法 | D09 的每台必验机器完成移动/跳跃/碰撞、选择、放置/破坏、窗口恢复和正常退出；建议各 15 分钟，不由有限帧运行代替 |
| [ ] | A11 协议与性能对照 | 摘要/seed、441 区块、workload、CPU/GPU/present 计时与结果字段保持；按 D10 留原始对照，不把旧冻结包直接当同源码窗口库 A/B |

全部未执行。D03/B、D09/A、D10/B 等缩小范围的选择，应在正式 spec 对应行标出条件和未验证能力，不以勾选整行掩盖缺项。性能差异需说明来源；硬性回归预算在采样前另行确认，不能拿实测成绩反推通过线。

##### 归档 - 决策填写

可用 `D02=A，D03=B，...，D12=A` 确认方向，并写明机器、预算或其他约束；D01 无需再次选择。确认方向后单独批准 P01–P06 准备实验；尚未实测的内容不能由“全部选 A”直接宣布通过。实验推翻选项时先回到本表确认变化，再把已选方案写入正式 T2 spec，保留本草稿的选择记录；正式 spec 审核与实施授权仍是下一道节点。

#### 归档 - 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| T4 的完整对象设计开始 | Runtime/Window 与 Application、SimulationSession 的实例生命周期；不从 SDL C API 形式推导必须建立管理器 |
| 有明确音频、手柄、中文文本 UI 或其他平台需求 | 单独确认 SDL 子系统范围、输入/UI 契约、资源部署与对应验收 |
| 同源码 A/B 发现显著窗口/输入/present 性能差异 | 建立专项实验定位等待、输入模式、DPI 与驱动因素；不顺带重写渲染器或降低世界规模 |
