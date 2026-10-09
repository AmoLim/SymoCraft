---
type: 类设计
status: 草稿
project: D3D12Lab
module: A2-Window-And-Clear
class_name: App
created: 2026-09-23
tags:
  - 示例
  - area/templates
---

# App 类设计

> 设计示例，非可编译代码；当前设计为整理后的假定基线，须核对源码。

流程与验收只记在 [[D3D12Lab-A2持续清屏-功能示例]]。

## 当前设计

### 数据成员

| 类型 | 成员 | 初值 / 范围 | 含义与所有权 |
| --- | --- | --- | --- |
| `bool` | `bClassRegistered_` | `false` | App 是否承担窗口类注销责任 |
| `bool` | `bMinimized_` | `false` | WM_SIZE 更新的最小化状态 |
| `bool` | `bRunning_` | `true` | 是否继续主循环，不代表窗口是否存活 |
| `HINSTANCE` | `hInstance_` | `nullptr` | 借用模块句柄；用于注册/注销窗口类，不卸载模块 |
| `HWND` | `hWnd_` | `nullptr` | App 负责销毁窗口；对外只借用句柄 |
| `int` | `selected_` | `1`；仅 `{1,2,3}` | 选择的唯一来源；标题由此派生 |

### 不变量

| 编号 | 条件 | 成立边界 |
| --- | --- | --- |
| I1 | 请求退出后 `bRunning_` 不再变 true；false 时 HWND 可以仍有效 | 单次运行内 |
| I2 | 创建成功后保存 HWND；WM_NCDESTROY 后清空并解绑 | 创建 API 返回前回调用参数 hwnd，不能依赖成员已赋值 |
| I3 | 窗口销毁后才注销窗口类；注销成功才清除注册标志 | 本例单窗口、独占类名；清理边界 |
| I4 | `selected_` 只取 1/2/3；默认值与显示一致 | 构造完成、消息处理返回时 |
| I5 | GWLP_USERDATA 借用 App 期间，App 存活且地址不变 | 绑定至 WM_NCDESTROY 解绑 |

### 接口与生命周期

| 接口 | 行为 | 前提 / 边界 |
| --- | --- | --- |
| `InitializeWindow()` | 注册窗口类并创建窗口 | 仅初始化一次；失败抛出，保留已获取资源的真实标志 |
| `CleanUp()` | DestroyWindow → UnregisterClassW | 循环停止后调用；失败抛出；成功清理后重复调用为空操作 |
| `IsRunning / IsMinimized / GetSelected / GetHwnd` | 只读查询 | HWND 只借用，窗口销毁后失效 |
| `WndProc` | 绑定 / 查找 App → HandleMessage | 无绑定时默认处理；异常不得穿过系统回调边界 |
| `HandleMessage` | 按键改选择；WM_SIZE 改最小化；关闭 / Esc 停止循环 | WM_DESTROY 兜底停止并 PostQuitMessage；WM_NCDESTROY 清空句柄并解绑 |

- **持有与销毁：** main 持有 App；正常路径先显式清理窗口，再结束 App 寿命。禁止拷贝 / 移动（I5）；异常策略见功能笔记。
- **线程与回调：** 在窗口创建线程调用；创建、销毁窗口也可能同步触发回调。

## 本次变更

### 成员增量

| 类型 | 成员 | 初值 | 含义与所有权 |
| --- | --- | --- | --- |
| `std::unique_ptr<Dx12Renderer>` | `renderer_` | `nullptr` | App 独占；Renderer 借用 HWND，拥有 GPU 资源及 event |

### 不变量增量

| 编号 | 新增条件 | 成立边界 |
| --- | --- | --- |
| I6 | renderer_ 非空表示构造成功且未释放，不证明 GPU 空闲 | 初始化完成后；不另加 ready 布尔值 |
| I7 | 渲染时 HWND 有效、renderer_ 非空、运行中且未最小化 | 每次调用 Renderer 前 |
| I8 | 正常路径：GPU 等待成功 → 释放 Renderer → 销毁窗口 | 关闭阶段；图形失败走功能笔记的致命退出分支 |

### 修改伪代码

```cpp
// 只组合具体 Dx12Renderer：当前仅需 DX12，真实替换需求出现再考虑接口。
// 下列为新增 public 操作；跨类主循环见功能笔记。
void InitializeGraphics() {
    require(hWnd_ && !renderer_);
    renderer_ = make_unique<Dx12Renderer>(hWnd_, 1280, 720);
} // 构造失败不发布对象。

void RenderFrame() {
    require(hWnd_ && renderer_ && bRunning_ && !bMinimized_); // I7
    renderer_->RenderFrame(colors[selected_ - 1]); // I4；颜色表见功能笔记。
}

void RequestExit() noexcept { bRunning_ = false; } // I1；重复请求无副作用。

void ShutdownGraphics() {
    require(!bRunning_);
    if (!renderer_) return;
    renderer_->WaitForGpu(); // 失败抛出，不执行 reset、不继续正常清理。
    renderer_.reset();       // I8；unique_ptr 不会自动等待 GPU。
}
// CleanUp 新前提：renderer_ 已为空；其余清理逻辑不变。
// 关闭/Esc/WM_DESTROY 改调 RequestExit；WM_PAINT 仅 BeginPaint/EndPaint，
// 移除 GDI 填色及背景刷；回调中不做 GPU 工作。
// 若前置声明 Renderer，App 析构定义放在能看到完整类型的 .cpp。
// 验收：引用功能笔记，不复制结果；通过后合并成员、I6～I8及接口到当前设计。
```

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| 出现多窗口，或窗口职责明显过重 | 提取 Win32Window |
| 需要另一后端或测试替换 | 提炼 Renderer 接口 |

选入本轮后才补成员与接口设计。
