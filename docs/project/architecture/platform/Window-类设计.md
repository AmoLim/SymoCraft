---
type: 类设计
status: 已验证（T0范围）
project: Symocraft
module: platform
class_name: Window / Window::Impl / GraphicsBridge / ProcessMemory
inheritance: []
created: 2026-10-05
tags:
  - area/architecture
---

# Window 与平台协作者

关联功能：[窗口与输入](Platform-窗口与输入-功能.md)。

最终验证：[M3-T0 交付与验收报告](../../../milestones/m3-t0/README.md)。

## 当前设计

`Window` 管理系统窗口，`Impl` 隔离 SDK 和回调状态；`GraphicsBridge` 是私有静态适配，不拥有窗口；`ProcessMemory` 是独立采样值，不依赖 telemetry。

### 数据成员

| 类型 | 成员 | 初值 / 范围 | 含义与所有权 |
| --- | --- | --- | --- |
| `unique_ptr<Impl>` | `impl_` | 构造时分配 | Window 独占，公开头无需 SDK |
| `GLFWwindow*`，私有 | `Impl::native` | null / 有效窗口 | 唯一销毁责任；不对外暴露 |
| `int` | `width/height` | 0 / framebuffer 像素 | 回调更新；零尺寸可表示暂不可绘制 |
| `InputSnapshot` | `Impl::input` | 键全 false，事件空 | 最近 PollInt 结果；公开借用，不全量复制 |
| `vector<PointerEvent>` | `pending_pointer_events` | reserve 64 | 保序记录，和快照事件 vector 交换复用容量 |
| `exception_ptr` | `input_error` | 空 | 回调分配失败的延迟错误，从 PollInt 安全抛出 |
| `bool` | `first_cursor/pending_focus_loss` | true / false | 抑制首个鼠标跳跃，发布失焦事实 |
| `optional<uint64_t>` | `ProcessMemory` 两计数器 | 空 | 查询失败保留空值；字节单位 |

### 不变量

| 编号 | 条件 | 成立边界 |
| --- | --- | --- |
| I1 | `native` 非空期间 Window 地址稳定且 `impl_` 存活 | 注册回调至 Destroy 完成 |
| I2 | Destroy 后 `native == nullptr`；重复 Destroy 不再调用系统销毁 | 清理返回后 |
| I3 | CaptureInput 每个所需物理键只读取一次；PollInt 事件保序且已消费缓冲清空复用 | 对应操作返回 |
| I4 | 回调不调用 simulation/renderer；公开接口不泄漏 SDK | 编译及边界检查 |
| I5 | GLFW 生命周期覆盖所有 Window，图形对象先于窗口销毁 | app 控制的主线程运行期 |

### 接口与生命周期

| 接口 | 行为 | 前提 / 边界 |
| --- | --- | --- |
| `Init/Create` | 初始化 GLFW、创建窗口/上下文并绑定回调 | 主线程；创建中异常由局部 unique_ptr 清理 |
| `PollInt/CaptureInput/Input` | 发布保序事件、读取按键、借用快照 | 窗口有效；借用内容在下次轮询/采键/重置时更新 |
| `ResetInput` | 清 sticky keys、双事件缓冲和首鼠标状态 | 事件轮询后；不修改游戏组件，不回放失焦前事件 |
| `Focused/Minimized/ShouldClose` | 查询平台事实 | ShouldClose 可在销毁后调用，其余要求窗口有效 |
| `Close/Destroy` | 请求关闭 / 真正销毁 | Close 不立即结束窗口寿命；Destroy 幂等 |
| 私有 `GraphicsBridge` | 当前化、函数地址、VSync、交换缓冲 | 仅 renderer；借用期满足 I5 |

持有者为 app 的 `unique_ptr<Window>`。Window 不可拷贝，因用户指针绑定固定地址也不移动；析构兜底 Destroy。GLFW 全局初始化不在 Window 析构中结束，应用显式在全部对象销毁后调用 Free。无后台线程，无销毁后回调或异步任务。

实现：[window.h](../../../../game/modules/platform/include/symocraft/platform/window.h)、[window.cpp](../../../../game/modules/platform/src/window.cpp)。

## 本次变更

T0 移除公开 `void* window_ptr`；把散落在 app 的 GLFW 回调迁到 Impl。删除平台 glViewport、GLAD 和设备查询职责，改为私有桥接供 renderer 调用。不新增多窗口抽象或输入回放机制。

验收位置：[功能验收表](Platform-窗口与输入-功能.md#验收案例)。公开头与边界、正常/资源错误退出、benchmark 失焦采样，以及用户确认的普通切出切回和最小化恢复均有证据。I1-I5 作为当前调用契约保留；强制创建失败、回调分配失败、benchmark 主动最小化/改尺寸未作本轮专项注入，不能以正常恢复替代。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| 需要独立自动测试事件累积细节 | 抽出纯值累积器，但不把 SDK 回调公开 |
