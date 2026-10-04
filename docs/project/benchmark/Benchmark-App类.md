---
type: 类设计
status: 实现中
project: SymoCraft
module: benchmark
class_name: App
inheritance: []
created: 2026-10-04
---

# Benchmark App 类设计

关联功能：[[Benchmark-执行与导出]]。App 在 `gui.cpp` 匿名命名空间中，仅服务本工具，不复用游戏 Application。

## 当前设计

职责：Win32 控件、用户配置、一个后台工作任务、只读进度与结果表格。`RunGui` 负责 COM 初始化、窗口类和消息循环；`Run/Export` 负责耗时工作。

### 数据成员

| 类型 | 成员 | 初值 / 范围 | 含义与所有权 |
| --- | --- | --- | --- |
| `HINSTANCE / HWND` | `instance / window` | 借用模块 / 空至窗口创建 | window 绑定 App，不拥有模块；WM_NCDESTROY 解绑 |
| `HFONT` | `font / title_font` | 空 → GDI 字体 | App 独占，窗口结束后 DeleteObject |
| `int` | `dpi` | 96 → 窗口 DPI | 用于 DIP 尺寸/字体转换，当前为系统 DPI awareness |
| `std::jthread` | `worker` | 不运行 → 一次 Run 或 Export | 独占；提供 stop_token；结束后 join |
| `mutex / Progress` | `mutex / snapshot` | 空进度 | 工作线程发布，UI 定时器复制；锁内不执行文件或图形工作 |
| `Outcome / string / path` | `outcome / error / exported` | 空结果 | 工作线程最终写，finished 发布后 UI 才读 |
| `atomic_bool` | `finished` | false | 工作完成的 release/acquire 交接，不是性能测量指标 |
| `Config` | `last_config` | 默认配置 | UI 保存值；worker 捕获不可变副本，重试沿用原配置 |
| `bool` | `busy / exporting / close_pending / has_session` | false | 仅 UI 线程读写；按钮可用性和关窗状态 |

### 不变量

| 编号 | 条件 | 成立边界 |
| --- | --- | --- |
| A1 | 一个 App 最多一个 worker；重用 worker 前完成 join | Begin 返回及所有消息处理边界 |
| A2 | 所有 HWND/GDI/控件调用由 UI 线程执行，worker 不 SendMessage 操作 UI | 全生命周期 |
| A3 | snapshot 的并发访问持 mutex；复制后解锁再更新控件 | worker 回调、Tick |
| A4 | outcome/error/exported 在 finished acquire=true 并 join 后读取 | Tick 完成分支；busy=true 时按钮表达式短路，不读 outcome |
| A5 | worker 持有配置副本；运行中控件不能改变本轮参数 | Begin 至工作完成 |
| A6 | App 地址稳定且存活覆盖 HWND 绑定和所有 worker 回调 | 构造至析构；禁止拷贝/移动 |
| A7 | 忙时关窗只请求停止，不立即销毁 HWND；任务结束后再销毁 | WM_CLOSE、Tick |

### 接口与生命周期

| 接口 | 行为 | 前提 / 边界 |
| --- | --- | --- |
| `Init` | 创建控件、字体、250ms 定时器 | WM_CREATE 期间建立界面；异常由 Procedure 捕获 |
| `Begin(resume, export_zip)` | 验证状态、复制配置、启动 worker | UI 线程；busy 时无操作；A1/A5 |
| `Tick()` | 读进度或消费最终结果 | UI 线程；A3/A4；性能统计仍由文件产生 |
| `FillResults()` | 读取会话和每轮结果，重建表格 | 完成后；路径由固定场景/attempt 规则生成 |
| `Procedure / Message` | 消息分发、按钮、关闭、异常边界 | 不让 C++ 异常穿过 Win32 回调；A6/A7 |
| 析构 | 请求停止并 join，再释放字体 | 正常消息循环已关闭窗口；异常循环也先结束工作再销毁 |

为什么不是从工作线程直接改进度条：UI 所属线程明确，避免同步消息阻塞和关闭时回调访问已释放对象。jthread 能发送停止请求，但不能自动让外部进程退出；停止请求由 Run 转为 Process::Stop，见 [[Benchmark-Process类]]。

实现与证据：`tools/benchmark/src/gui.cpp`；中文便携目录中实际运行快速检查、导出、轮次间取消后继续成功，截图见 `evidence/gui-quick-export.png`。当前源码在短测后补充清空旧结果、长 GPU 字符串提示、静态文本背景及 WM_NCDESTROY 解绑；最终构建回归覆盖编译，新增显示细节未单独再次截图。

## 本次变更

功能闭环已实现；多显示器混合 DPI、低分辨率桌面、屏幕阅读器完整体验仍待专门验收。当前固定布局的客户区为 874×650 DIP，桌面测试确认各字段/按钮不重叠。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| 需要 720p 或高缩放小工作区 | 可缩放/滚动布局和 Per-Monitor V2 DPI，保持操作顺序不变 |
| 需要会话重开 | 显式会话选择器与身份复核；当前 GUI 重试限当前会话，CLI 支持 resume |
