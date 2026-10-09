---
type: 功能
status: 草稿
project: D3D12Lab
module: A2-Window-And-Clear
created: 2026-09-23
tags:
  - 示例
  - area/templates
---

# A2 持续清屏

> 设计示例，非可编译代码；窗口基线是假设，DX12 接入与验收均未完成。

类约定：[[D3D12Lab-App-类示例]] · 维护：[[02-示例与演进维护]]

## 当前设计

App 管窗口、选择和运行状态；main 管消息循环与退出。

```text
打开窗口 → 1/2/3 改选择与标题 → 请求退出 → 清理窗口
```

基线还没有 GPU 清屏；默认选择 1、句柄终止状态须与真实源码核对。

## 本次变更

### 目标与流程

**目标：** 持续清屏、1/2/3 切色、最小化暂停、正常关闭。

**范围：** 1280×720、单线程、RTX 5070 Ti、Debug Layer；双缓冲、单 allocator、逐帧等待。禁用缩放、最大化和 Alt+Enter。

```cpp
constexpr Color colors[] = {
    {0.08f, 0.20f, 0.36f, 1}, // 1：蓝
    {0.10f, 0.55f, 0.22f, 1}, // 2：绿
    {0.65f, 0.12f, 0.18f, 1}  // 3：红
}; // 常量表；App 按 selected_ 查表，不保存第二份可变颜色。
```

```text
启动：InitializeWindow → InitializeGraphics → ShowWindow → 主循环
每轮：移除并处理当前消息 → 重查运行标志
      ├─ 已请求退出 → 正常关闭
      ├─ 最小化 → WaitMessage → 下一轮
      └─ 其他 → RenderFrame → 下一轮
正常关闭：ShutdownGraphics（GPU 完成 → 释放 Renderer）→ CleanUp（销毁窗口 → 注销类）→ 返回退出码
发生错误：记录操作名、错误码及设备诊断 → 刷新日志 → ExitProcess(1)
```

WM_QUIT 保存退出码并请求停止，不分发给窗口；回调请求停止后立即结束取消息。WaitMessage 失败也走错误路径。main 在 try 外持有 App；本轮致命退出不承诺恢复或正常析构清理。

### 清屏与同步

Renderer 借用 HWND，拥有 GPU 资源 / event；构造不提交，失败由资源包装清理。

```text
RenderFrame：确认上次 GPU 工作完成 → 获取当前后缓冲 → Reset allocator/list
             → PRESENT 转 RENDER_TARGET → Clear → 转回 PRESENT
             → Close/Execute → Present(1, 0) → WaitForGpu
WaitForGpu：Signal → 检查 fence → 必要时等待 event → 复查完成值
```

- 首帧无此前 GPU 工作；以后等待成功才可再次 Reset 或正常释放，双缓冲不改变此约束。
- fence 判断先排除设备移除的 UINT64_MAX；等待上限暂取 5s，超时或 API 失败走错误退出。析构不代替 GPU 同步。

### 验收案例

环境：Windows / MSVC x64 Debug。全部未执行；通过后将 `[ ]` 改为 `[x]`，失败写 `[ ] 失败`。同项多个场景全部通过才标记通过；原因与证据按场景记在表下。

| 状态 | 场景 | 预期行为 |
| --- | --- | --- |
| [ ] | 无输入运行 10s | 蓝色且持续提交，适配器正确 |
| [ ] | 按 2、3、1、1、4 | 绿、红、蓝；重复或无关键不改变当前颜色、不异常退出 |
| [ ] | 最小化 5s → 恢复，重复三次 | 暂停提交并等待消息，恢复继续画 |
| [ ] | 分别用关闭按钮 / Esc / Alt+F4 | 停止提交，同步释放图形后清理窗口，返回 0 |
| [ ] | 投递 WM_QUIT(7) | 不分发该消息，正常清理后返回 7 |
| [ ] | 分别注入初始化失败 / 提交后 Present 失败 / 等待超时 | 诊断并返回 1，不再提交 |
| [ ] | 遮挡恢复、移动、正常退出 | 无 GDI 覆盖，无未解决 Debug Layer 警告 / 错误 |
| [ ] | 尝试缩放、最大化、Alt+Enter | 本轮禁用的操作无效 |

验证版本：无；证据：无。实现落点：main.cpp、App.h/.cpp；候选新增 Dx12Renderer.h/.cpp。

## 后续考虑

| 触发条件         | 再考虑的变化                    |
| ------------ | ------------------------- |
| 需要自由缩放       | ResizeBuffers 与帧边界重建      |
| 测量发现逐帧等待影响目标 | 多帧资源与 fence 管理            |
| 需要错误后继续服务    | 异常清理与恢复，替换 ExitProcess 策略 |
