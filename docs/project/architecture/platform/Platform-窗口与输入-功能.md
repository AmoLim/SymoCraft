---
type: 功能
status: SDL3实现已接入，T2阶段验证待汇总
project: Symocraft
module: platform
created: 2026-10-05
updated: 2026-10-08
tags:
  - area/architecture
---

# Platform 窗口与输入

关联对象：[Window](Window-类设计.md)；无实例桥接与内存值见下文具名小节。依据：[M3-T0](../../../spec/M3-T0-模块软硬边界.md)。

历史验证：[M3-T0 交付与验收报告](../../../milestones/m3-t0/README.md)。SDL3 实施依据与进度分别见 [T2 正式计划](../../../spec/M3-T2-SDL3迁移.md)、[T2 记录](../../../milestones/m3-t2/README.md)；本页不宣布 S2 或 T2 通过。

## 当前设计

平台拥有 SDL3 系统窗口、按需 GL context、事件/键鼠事实和进程内存查询，不拥有角色或 renderer 图形资源。实现为 `symocraft_platform` 静态库，私有链接固定 `SDL3::SDL3-static` 与 Psapi，生产窗口不再链接 GLFW。公开头仍只有 `window.h`、`key_snapshot.h`、`process_memory.h`，SDL、Win32、GL 与 Vulkan SDK 类型均不进入公开 API。当前历史 GLFW 配置/包许可尚待 S5 收尾，不等于仍使用 GLFW 窗口。

内部划分：`Window::Impl` 独占 SDL window、按需 context 和私有 InputState；`Window` 提供中立用途/查询，Create 最后参数为 `WindowMode mode = WindowMode::OpenGL`，另有 Native/Vulkan 用途。`GraphicsBridge` 向 renderer 提供 checked GL 操作，`NativeBridge`/`VulkanBridge` 只交接授权私有资源；`SampleProcessMemory` 独立负责进程计数器。没有把 SDL 事件变成 ECS 修改入口。

源码：[窗口实现](../../../../game/modules/platform/src/window.cpp)、[私有输入状态](../../../../game/modules/platform/src/input_state.h)、[内存采集](../../../../game/modules/platform/src/process_memory.cpp)、[构建边界](../../../../game/modules/platform/CMakeLists.txt)。本页同步实际代码，当前 SDL 构建/运行结果由 M3-T2 单独汇总，不把 T0 GLFW 证据或用户确认的准备手感冒充生产 SDL 完整实测。

### GraphicsBridge

Platform::GraphicsBridge 是无实例的私有静态适配，六项签名保留，详细契约见 [内部 API 表](Platform-namespace-API.md#私有函数预期行为)。当前化、VSync、呈现必要失败具名抛错；合法无 context、可选空函数地址和不支持扩展不误判为失败，查询故障另行拒绝。仅 renderer 的受限 include 可见，不是公共图形服务或已实例化资源类。
源码：[graphics_bridge.h](../../../../game/modules/platform/src/graphics_bridge/graphics_bridge.h)。

### NativeBridge

NativeBridge 只返回有效活窗口的借用 HWND，用于 Benchmark 任务栏策略、授权真实探针和后续后端；不接管系统窗口，不让 app 自行读取/销毁原生对象。声明仅在 [native_bridge.h](../../../../game/modules/platform/src/graphics_bridge/native_bridge.h)，签名与寿命见 [私有桥接 API](Platform-namespace-API.md#native-与-vulkan-私有桥接)。

### VulkanBridge

VulkanBridge 取得 SDL loader 的 `vkGetInstanceProcAddr`、复制必需扩展名，并创建/销毁 caller-owned surface。instance、surface 与在途 GPU 资源由探针或后续 renderer 后端先释放；最后销毁 Window，不能在卸载后调用保存的入口。SDK 类型仅在 [vulkan_bridge.h](../../../../game/modules/platform/src/graphics_bridge/vulkan_bridge.h)等授权私有位置出现。真实 surface 交接不等于 Vulkan 游戏后端，详见 [桥接 API](Platform-namespace-API.md#native-与-vulkan-私有桥接)。

### ProcessMemory

Platform::ProcessMemory 值语义与 SampleProcessMemory 契约见 [Platform API](Platform-namespace-API.md#状态与值)。Windows/SDK 实现私有，SampleProcessMemory 本身是公开中立 API；不因纯值支持并发共享可变 Window。
源码：[process_memory.h](../../../../game/modules/platform/include/symocraft/platform/process_memory.h)。

2026-10-07 同步 WindowMode 与私有桥接类型；输入值仍见 [Window 输入值](Window-类设计.md#inputsnapshot)、[覆盖清单](../对象笔记覆盖清单.md)，不新增公开 SDK 包装类。

### 目标与流程

输入处理唯一流程图见 [Window](Window-类设计.md#私有函数预期行为)。Poll/Wait 的事件只转换平台事实，像素尺寸由 SDL 实际查询并交给 app/renderer；Wait 首事件同样消费一次，待下次 Poll 发布。进程内存是独立值采样，不参与事件转换。

### 公开接口预期行为

| 签名 / 入口 | 调用方与可见范围 | 预期行为：输出及状态变化 | 前提 / 边界 | 失败反馈及失败后状态 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| Window / InputSnapshot / KeySampling 全部公开函数 | app / CPU 消费者 | [Window 权威表](Window-类设计.md#公开接口预期行为) | 窗口生命周期与有效枚举 | 同目标表 | window.h / key_snapshot.h |
| `SampleProcessMemory()` | app | [Platform API 权威表](Platform-namespace-API.md#公开接口预期行为) | 不依赖 Window | 同目标表 | process_memory.h |

### 私有函数预期行为

| 签名 / 入口 | 内部调用方 | 预期行为：处理规则及副作用 | 前提 / 边界 | 失败传播及清理责任 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| Impl / TU helper / InputState | Window 的受控 C++ 调用边界 | [Window 内部权威表](Window-类设计.md#私有函数预期行为) | 不在 SDL/Win32 C callback 中抛异常，不访问 ECS/renderer | 同目标表 | window.cpp / input_state.h |
| GraphicsBridge 6 个 static 方法 | renderer 授权目录 | [Platform 内部 API 表](Platform-namespace-API.md#私有函数预期行为) | struct public，但模块私有共享头 | 同目标表 | graphics_bridge.h |
| NativeBridge / VulkanBridge static 方法 | 平台内部 / 授权探针 / 后续后端 | [Native/Vulkan 权威表](Platform-namespace-API.md#native-与-vulkan-私有桥接) | 活窗口、匹配用途与 caller-owned 图形资源 | 同目标表 | 两个新增私有桥接头 |

普通游戏失焦由 app 决定暂停并重置输入，平台不代替应用制定暂停规则。Benchmark 保留无边框普通窗口、非置顶、按 allow-unfocused 不抢焦点及 `NonRudeHWND` 任务栏策略，隐藏创建、准备后显示；允许失焦不等于允许最小化。width/height 是实际绘制像素，SetSize 仍接收逻辑尺寸；Benchmark 创建即核对请求像素，不静默降低目标分辨率。

### 关键约束与取舍

- Window 独占原生窗口和按需 GL context，禁止复制或移动。当前只支持主线程一个 platform 窗口；Init 幂等取得一次 video 责任，Free 只释放该责任且拒绝活窗口，不全局结束其它 SDL 持有者。
- 键状态为固定数组，鼠标/滚轮采用保序双缓冲 vector，每个预留 64 条并跨帧复用容量。超过容量的输入突发允许扩容；不承诺绝对零分配。app 借用快照，不复制整个 vector。保序避免“先撞到俯仰/FOV 上限再反向移动”被合并为零增量而改变玩法。完整事件回放不在 T0 范围。
- SDL scancode 对应物理键；按住态与短按锁存分离，同帧按放也采样一次，repeat 不制造新短按。Poll/Wait 排空后同步真实窗口 flags，修复遗漏恢复消息；Capture/Reset 也收敛窗口事实，Capture 仅在真实焦点有效且未最小化时采纳键鼠 held state。Reset 清旧锁存、事件与首运动基准，不把失焦旧输入带回恢复帧；按钮保持 held 采样，不新增 sticky 点击。
- Benchmark 输入 reducer 不采纳玩家键鼠，只采用本窗口 Escape，未消费锁存跨失焦保留，不采后台全局键；最小化仍使 Benchmark 无效。CaptureInput 仍在应用模拟计时内，不把原采样成本任意挪入 event_ms。输入扩容故障具名保存后立即在受控 C++ 边界抛出，停止本帧，不发布误导空输入。普通游戏非 drawable 时，包括有界 smoke，也暂停/等待而不绘制；普通无帧限制失焦暂停与恢复 dt=0 分支保留。
- 事件只修改平台状态，不访问 Registry、相机、世界，也不提交 OpenGL。GLAD 加载和 viewport 仍在 renderer；窗口只验证所请求的实际 GL 4.6 Core/4x MSAA/Debug 属性，不因此拥有 renderer 图形资源。
- 桥接白名单仅有 `graphics_bridge.h`、`native_bridge.h`、`vulkan_bridge.h`；renderer 仅私有获得 `platform/src/graphics_bridge/`，当前只消费 GL 头，不能搜索 InputState 等其他私有实现。共享桥接头不含 SDL，公开头不提供 `void*` handle 逃生口。
- 正常退出为 Run 局部 GPU/renderer→GL context→Window→本模块 video。必要 checked 清理失败仍执行余下释放，次级故障单独记录；析构 noexcept/best-effort，不替代显式清理或覆盖主运行错误。Vulkan 调用方须先结束 surface/instance 等借用对象，静态 SDL 不等于静态 Vulkan loader。
- Windows 进程内存读取失败返回空可选值，不把“不支持”伪装为零。

### 既有 T0 实现与验收

以下表及原数字为 GLFW 的 T0 历史快照，保留作迁移基线，不是 SDL3 当前验收结果。新窗口/桥接失败与真实首帧结果见 T2 阶段记录；硬件输入、布局、DPI/任务栏等剩余矩阵继续由 S3 补齐。

### 验收案例

| 状态 | 场景 | 预期行为 |
| --- | --- | --- |
| [x] | 公开头独立编译 | 无 GLFW/Win32 传播 |
| [x] | 普通游戏切出切回、最小化恢复 | 用户确认本轮安装包视角、输入和恢复正常 |
| [x] | benchmark 失焦但不最小化 | 新 static/edit 全程失焦仍有效完成，焦点样本为 false |
| [x] | 窗口建立后的资源初始化失败 | 三类实际损坏资源探针正常报告错误并退出 3，执行既有清理路径 |
| [ ] | benchmark 主动最小化或改变 framebuffer 专项 | 本轮未执行新的真实故障探针；不能拿普通游戏恢复反馈代替采样无效判定验证 |
| [ ] | 强制窗口创建失败、回调缓冲分配失败 | 尚未专项注入；RAII/延迟异常为实现契约，不称为已实测 |

2026-10-05 最终公开头与全量测试通过，运行证据见最终报告。用户反馈针对本轮 Release 安装包普通玩法；benchmark 失焦来自短采样记录，两者不混用。上表未测专项明确保留，不影响已经建立的 T0 模块边界证据，也不推定长时或跨硬件验证完成。

## 本次变更

2026-10-07 同步 SDL3 生产实现、三种窗口用途、私有 bridge/loader、像素/等待与输入候选移植；上方 T0 GLFW 验收表保持历史结论，不增加 SDL3 通过勾选。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| T2 生产验证与交付 | 已选方案及 S1–S5 实施见 [正式计划](../../../spec/M3-T2-SDL3迁移.md)；SDL3 实现不代替真实游戏、完整失败、双机或包验收 |
| T3 新图形后端落地 | 在 T2 已验收的 SDL3 契约上按真实后端要求扩展桥接，不公开原生 handle 给 app |
| 有输入录制、更多键或多窗口要求 | 独立事件序列、设备映射和多窗口所有权；当前不预造 |
