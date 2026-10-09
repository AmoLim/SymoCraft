---
type: 类设计
status: 草稿
project: Symocraft
module: renderer
class_name: "SymoCraft::GpuTimer"
inheritance: []
created: 2026-10-06
updated: 2026-10-06
tags:
  - area/architecture
---

# GpuTimer 绘制计时对象

2026-10-06 从 [Renderer 汇总](Renderer%20namespace%20API.md)抽取并核对源码；旧称 GPUTimer 对应同一对象，不另建同义笔记。新笔记未独立运行测试。
关联：[Batch](Batch-类设计.md)、[Session](../telemetry/Session-类设计.md)、[覆盖清单](../对象笔记覆盖清单.md)。

## 当前设计

### 职责与所有权

记录 GL_TIME_ELAPSED 绘制段，延迟非等待式收集并返回自有结果；不是整帧 GPU 时间、CPU 提交、物理显示延迟或上传耗时。app 的 Run 在性能采样模式创建局部 unique_ptr；Renderer/Batch 仅当前 Render/Draw 借用，不保存它。

对象不可复制；用户声明析构和删除复制，未定义移动，实际也不可移动。仅持有当前 GL 上下文的主线程调用，早于窗口/上下文销毁。无并发、重入或多上下文保证。

### 成员

| 成员 | 初态 / 所有权 |
| --- | --- |
| array<Slot,64> slots_ | 全部初始零/非 pending；本对象拥有最多 128 个 query |
| Slot* current_ | null；借用自身槽位，不指向 app/Session |
| bool supported_ | false；构造查询 counter bits >0 后为 true |
| bool active_draw_ | false；是否需要结束当前 GL query |

### Result

`size_t frame` 为 app 调用 BeginFrame 时传入的 Session::NextFrame 样本索引，不是 GPU 帧号；`double milliseconds` 为该槽已记录段的纳秒总和 / 1,000,000。Poll 返回 `vector<Result>` 拥有结果，不再借用槽位；不承诺按帧号全局排序（按槽数组扫描）。

### Slot

`array<unsigned int,2> queries` 保存 GL ID（公开头无 SDK 类型）；frame/count/pending 保存关联索引、已启用段数和等待读取状态。每帧最多两段，当前 chunk/line 两批次正常路径与此容量对应。

### 生命周期与接口

公开头中 8 个可调用接口与 2 个删除的特殊成员逐项列出。GLAD/当前上下文主线程前提适用于所有 GPU 操作；本对象的 public API 同时是模块公开接口。

### 公开接口预期行为

| 签名 / 入口 | 调用方与可见范围 | 预期行为：输出及状态变化 | 前提 / 边界 | 失败反馈及失败后状态 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| `GpuTimer()` | app Run；模块公开 | 探测 counter bits，supported_ 置结果；支持时为每个槽生成 2 个 query | GLAD/当前上下文已有效；成员默认零/false | 不支持是正常状态，不生成 query；未完整检查 GL 错误或部分生成失败，无构造事务保证 | [实现][timer-src]；[Renderer I1](Renderer%20namespace%20API.md#不变量) |
| `~GpuTimer()` | app Run 局部对象退出；模块公开 | active 时结束当前 query；supported 时删除 64×2 个 ID；不回填 Session | 早于上下文销毁；无仍在使用对象的调用方 | 无结果/领域错误反馈，不等待 pending 就绪；不证明驱动异常全部恢复 | [实现][timer-src]；[Renderer I5](Renderer%20namespace%20API.md#不变量) |
| `GpuTimer(const GpuTimer&)=delete` | 类调用方；模块公开声明 | 禁止复制构造，避免复制 query 所有权 | 编译期限制 | 调用无法编译，不存在运行后状态 | [头][timer-header] |
| `GpuTimer& operator=(const GpuTimer&)=delete` | 类调用方；模块公开声明 | 禁止复制赋值 | 编译期限制；也没有显式移动操作 | 调用无法编译，不存在运行后状态 | [头][timer-header] |
| `bool Supported() const` | app 能力分支；模块公开 | 返回 supported_，不改变状态 | 对象存活；仅报告构造时探测值 | 无错误通道；不是所有 query 成功分配/驱动无故障证明 | [头][timer-header] |
| `void BeginFrame(size_t frame)` | app Render 前；模块公开 | 清 current_；支持时选首个非 pending 槽，设置 frame/count=0；不分配结果容器 | 前一绘制段已结束；不覆盖 pending 槽 | 不支持/槽全满直接返回，丢本帧测量；不返回错误码，不检查已有 active_draw_ | [实现][timer-src]；I4 |
| `void BeginDraw()` | Batch Draw 前；模块公开 | 无 current_ 时无操作；否则启动 queries[count++]，active_draw_=true | 当前绘制段未活动；每槽至多 2 段 | count>=2 抛 logic_error，检查前不追加 query；嵌套/驱动错误没有完整防护 | [实现][timer-src]；I4 的正常调用前提 |
| `void EndDraw()` | Batch Draw 后；模块公开 | active 时 glEndQuery，再设 false；非 active 无操作，不读结果 | 当前上下文有效 | 无 GL 错误反馈，不验证与 BeginDraw 的完整匹配 | [实现][timer-src] |
| `void EndFrame()` | app Render 后；模块公开 | current_ 存在就设 pending=true，然后清 current_ | 已完成所有 EndDraw；不自动结束 active query | 无反馈；错误顺序可能留下 active 状态，不按安全顺序自动修正 | [实现][timer-src]；I4 |
| `vector<Result> Poll()` | app 后续帧/退出前；模块公开 | pending 槽全部段就绪才读纳秒并求和，追加自有 Result 后回收；未就绪跳过，count=0 得 0 ms；按槽扫描 | 不跨调用借用槽位；frame 由 BeginFrame 提供；不保证按帧号排序 | vector 分配异常上抛；此前槽可能已回收，当前 push 失败槽仍 pending，不提供整次 Poll 事务 | [实现][timer-src]；I4 |

### 私有函数预期行为

| 签名 / 入口 | 内部调用方 | 预期行为：处理规则及副作用 | 前提 / 边界 | 失败传播及清理责任 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| 不适用：没有私有函数或内部 helper | 无 | private 区只有 Slot 与成员，槽选择/就绪检查在公开方法内实现 | 不将私有数据伪装为函数，不虚构 Begin/End helper | 清理由公开析构完成，边界见上表 | [头][timer-header]、[实现][timer-src] |

构造未完整检查 GL 错误或部分 query 获取失败，不提供失败事务/完整资源恢复保证。Poll 返回容器可能分配；分配异常、驱动异常组合未专项验证。

### 状态变化与调用者前提

正常路径：空闲槽 → BeginFrame(count=0) → 一至两次 BeginDraw/EndDraw → EndFrame(pending) → Poll 全就绪读取 → 回到空闲。完整顺序、不能嵌套 BeginDraw、不能带活动 query 开下一帧或提前 EndFrame 都是调用者前提，**实现没有完整 state machine 顺序保护**。

### 不变量

| 原编号 / 来源 | 条件与边界 |
| --- | --- |
| I4（原 Renderer I4） | 正常调用顺序下，pending 槽不重用，未就绪不读结果；BeginFrame/Poll 的实际筛选保证 |

[Renderer I1/I5](Renderer%20namespace%20API.md#不变量)约束上下文和释放顺序；不将该全局条件重复作为本对象新编号。

### 异常与边界路径

- 不支持：BeginFrame 无槽、无查询结果，Session 保持缺失，不造 0。
- 64 槽全 pending：丢新帧测量，不阻塞等待、不覆盖旧 frame。
- 未就绪：Poll 跳过该槽，保留 pending，可能后续帧才收集。
- 第三绘制段：当前槽 count>=2 抛 logic_error，不追加第三 query；调用方统一异常清理，不保证已录帧必定回填。
- 零绘制段：EndFrame 仍标 pending；Poll 的 count=0 循环直接产生 **0 ms** 并回收，与池满/不支持的缺失不同。
- 析构仍 active：结束当前 query 再删除；正常 app 不靠此代替 EndDraw。
- 调用顺序错误、GL query 分配失败、读取后 vector 分配失败没有完整恢复协议，不包装成已解决保证。

### 采样关联与计时范围

app 在 Render 前 BeginFrame(Session::NextFrame())；Batch::Draw 先 ReloadData，再在 glDrawArrays 前后 BeginDraw/EndDraw；Render 后 EndFrame，Present 返回后 Session::Add(Frame)。后续帧 Poll 才 SetGpu(result.frame,ms)，索引必须已存在，否则 Session 抛 logic_error。

因此显式上传、clear、swap、CPU 打包不在 query 段内；不能泛称整帧 GPU 耗时。退出前只有一次不等待的 Poll，未就绪仍缺失；析构不补回 Session，也不保证每个采样都有 GPU 值。

### 依据与审核缺口

源码：[头](../../../../game/modules/renderer/include/symocraft/renderer/gpu_timer.h)、[实现](../../../../game/modules/renderer/src/gpu_timer.cpp)、[Batch 调用](../../../../game/modules/renderer/src/batch.hpp)、[app 调用](../../../../game/app/src/application.cpp)、[Session 回填](../../../../game/modules/telemetry/src/performance.cpp)。
[现有单测](../../../../test/unit/renderer/gpu_timer_tests.cpp)用 GLAD 替身覆盖未就绪不读、饱和丢帧、帧号、纳秒转换/求和、复用和 128 ID 删除；**未覆盖**不支持、零绘制段、第三段、错误顺序和真实驱动失败。原 [功能验收](Renderer-显式场景输入-功能.md#验收案例)、[T0 报告](../../../milestones/m3-t0/README.md)保留，不等于本次重新运行。计时开销、长期泄漏与所有驱动组合未测。

## 本次变更

无。只新增审核入口，不修改计时实现。

## 后续考虑

独立提出顺序防护、驱动失败处理或容量变更 spec 后再实现/测试；不能为填文档虚构已有恢复机制。

[timer-src]: ../../../../game/modules/renderer/src/gpu_timer.cpp
[timer-header]: ../../../../game/modules/renderer/include/symocraft/renderer/gpu_timer.h

