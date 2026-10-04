---
type: 修复
status: 已验证
project: SymoCraft
module: benchmark
created: 2026-10-04
---

# Benchmark 1080p 窗口修复

## 问题描述

- 预期：在 1920×1080 桌面上得到真实 1920×1080 的采样 framebuffer；允许切出，不置顶、不抢焦点，不主动隐藏任务栏。
- 实际：朋友回传的首轮在初始化时得到 1920×1061，触发尺寸不匹配异常，退出码 3、采样帧数 0。不是显存不足或网格缓冲容量不足。
- 环境：i7-10750H、GTX 1650、约 16 GB 内存、Windows build 26100、OpenGL 4.6.0 NVIDIA 591.74。不是 ARM 或 RTX 3060 机器。
- 关联功能：[[Benchmark-执行与导出]]、[[Benchmark-文件协议]]、[[Performance-失焦采样]]、[[使用与交付]]。

## 最小复现

1. 使用旧原生包，游戏 SHA256 为 `3717AF7168452CD741F294BBCF9C24C2259F660A3B12E257782A7DFA14C4E744`。
2. 在朋友的 1920×1080 桌面启动默认正式采样。
3. 第一轮 static 在窗口/上下文建立后失败；后续八轮没有启动。

原始证据为 `.temporary/yecat-results`，关键日志和摘要另存于 [原始失败](evidence/window-fix/original-failure/summary.yaml)。摘要明确记录请求 1920×1080、实际 1920×1061，stderr 为 `Actual framebuffer does not match requested benchmark resolution`。

## 定位记录

| 假设 | 检查方法 | 观察证据 | 结论 |
| --- | --- | --- | --- |
| GPU 缓冲区或显存不足 | 对照异常位置与初始化顺序 | 尺寸校验早于 Renderer::Init 和网格缓冲分配 | 不是本轮失败原因 |
| OpenGL 版本不足 | 查看实际 GL 字符串 | 上下文成功，GL 4.6 | 不是本轮失败原因 |
| 实际绘制区域与固定协议不符 | 对照 requested/actual framebuffer | 高度少 19 像素 | 已确认直接原因 |
| 带装饰窗口在满屏尺寸下受系统窗口约束 | 查看普通窗口创建路径 | monitor=nullptr、默认有装饰；桌面也是 1080p | 最可信的适配方向；缺少原机 DPI/外框/工作区记录，不断言 19 像素来自任务栏 |

## 修复设计

只调整 benchmark 窗口路径，不改变世界、网格、MSAA、分辨率、时长或帧时统计，不增加 FBO，不修改执行器架构。

```text
Application：benchmark 标志 → Window::Create(..., benchmark_window=true)
  隐藏创建普通窗口，DECORATED=false、FLOATING=false、monitor=nullptr
  在首次显示前设置 Win32 NonRudeHWND 属性
  设置位置并建立 OpenGL → 显示 → 查询实际 framebuffer
  严格比较实际与请求尺寸 → 不匹配则具名失败，不降级
退出：移除本窗口属性 → 销毁窗口；不改系统任务栏设置
```

- `NonRudeHWND` 阻止 Shell 将这个窗口自动按全屏应用处理，保留任务栏原有层级。任务栏可以覆盖画面底部，自动隐藏设置仍由用户控制。
- 保留既有 allow-unfocused 的 FOCUSED/FOCUS_ON_SHOW=false，不在每轮强制激活窗口；strict 策略仍沿用原本的焦点要求。
- 参数默认 false，普通游戏仍使用原来的有边框窗口；本次窗口创建提示在创建后恢复装饰/可见默认值，不泄漏给之后的普通窗口。
- Win32 句柄和属性仅在 window.cpp 内使用，不暴露到公共头；属性设置失败时清理窗口并报告系统错误。
- 摘要增加 `window_mode` 和 `taskbar_policy` 诊断字段；错误显示请求/实际尺寸。不改变协议版本 2 的既有必需字段，执行器仍严格检查 1080p。
- 改动会改变游戏哈希及窗口呈现环境；新旧包分开留存，不能把旧版结果冒充本次版本。

## 验证与回归

| 案例 | 输入 / 步骤 | 预期 | 修复前结果 | 修复后结果 / 证据 |
| --- | --- | --- | --- | --- |
| 原机 1080p | 朋友用修复包先快速检查 | 三场景真实 1920×1080 | 首轮 1920×1061 失败 | 待朋友复测 |
| Debug/Release 回归 | 当前全部 CTest | 全部通过 | 旧包 20/20 | 各 20/20；[Debug](evidence/window-fix/ctest-debug.log)、[Release](evidence/window-fix/ctest-release.log) |
| 本机原生包短测 | 三场景 allow-unfocused | 有效，尺寸及新增诊断正确 | 旧版无窗口策略字段 | 3/3 有效，均为 1920×1080；[会话](evidence/window-fix/quick-session.yaml) |
| 切出与任务栏 | 本机 1920×1200 满屏尺寸诊断，5 秒预热 + 120 秒采样 | 切出不自动最小化；任务栏行为保留 | 未纳入原生包窗口保证 | 退出 0，有效，19714 帧、10 次焦点变化；用户确认“任务栏和切换都正常”；[摘要](evidence/window-fix/full-monitor/summary.yaml) |
| 普通游戏 | 从非程序工作目录运行固定场景 120 帧 | 沿用普通窗口配置，正常退出 | 已有普通游戏路径 | ready、120 帧、退出 0，stderr 为空；[日志](evidence/window-fix/ordinary-game/stdout.log) |
| 独立交付包 | 解压到新目录、资源预检、核对文件 | 资源/CRT 完整，不含脚本 | 旧包保留 | 解压后 --check-assets 通过，ZIP 无 ps1/bat/cmd；[身份](evidence/window-fix/build-identity.json) |

本机是 1920×1200 桌面，三场景短测只申请 1920×1080；另一次 1920×1200 探针专门检查窗口尺寸等于显示器时的行为，**不是 1080p 正式基线，也不代替朋友原机复测**。满屏探针的采样期失焦时间估计为 96.6666 秒；执行了激活/Alt+Tab 检查，并收到用户对任务栏及切换的人工确认。最初快速检查阶段的一次 UI 操作撞上场景窗口退出，未将该工具错误当成成功证据；使用后续较长探针完成验证。

自动测试首次在受限环境下有两项资源重定位探针失败，获准在正常 Windows 环境复测后两种配置均 20/20。未为此修改资源代码。现有自动测试没有新增窗口断言，本次窗口行为证据来自真实短测和人工确认；朋友原机、高 DPI/多屏差异及属性设置失败注入仍未覆盖。

## 实现记录

- `include/core/window.h`、`src/core/window.cpp`：可选 benchmark 窗口策略、Win32 属性与清理。
- `src/core/application.cpp`：传入模式、增加诊断和尺寸异常信息。
- 原始完整数据和发布包留在 `out` 或原回传目录，模块 evidence 只保留小型摘要、日志和必要截图。
- 参考：[GLFW 窗口行为](https://www.glfw.org/docs/latest/window_guide.html)、[Windows 全屏与任务栏属性](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-itaskbarlist2-markfullscreenwindow)。
- 不承诺后台运行不占用资源，也不承诺被遮挡后的默认 framebuffer 工作量与完全可见时一致；不将日常后台采样等同于无干扰性能基线。

### 本次交付身份

- 基础提交：`daacb7701c85d1eca72d3432c0f540cf76dfdbee` 加本次尚未提交的三个生产源码变更；用户已有 `.gitignore` 改动未修改。
- 游戏 SHA256：`F73285A0AC164073DA7B8DFB912EF2FAE558061DF433A69A2FDBFA7B25729AF1`。
- 执行器没有修改，SHA256 仍为 `2E2210D1A938790A3251228F4D5DCAE02CE786EE865F4C254B86BB2F17225709`。
- 修复包：`out/packages/benchmark-window-fix/SymoCraft-Benchmark-windows-x64.zip`，1,165,655 字节；SHA256 为 `12771D0DB58308D28E015F27BBDF4C076998D56AC0CF14D52DEED712FC314BBF`。
- 安装目录：`out/install/benchmark-window-fix`；完整短测记录在 `out/benchmark-window-fix`。证据目录只有日志和小型 YAML/JSON，约 123 KB，不含程序或逐帧 CSV。
- 本文“已验证”限本机列明范围。未执行 36 分钟采样，也未恢复之前暂停的正式采样。

## 完成检查

- [x] 已核对原始失败记录与触发代码。
- [x] 保留严格尺寸校验，不以降分辨率或删除校验解决。
- [x] 当前版本构建、回归与实际短测完成。
- [ ] 朋友原机复测完成。
- [x] 记录新包身份与尚未覆盖边界，更新 status。
