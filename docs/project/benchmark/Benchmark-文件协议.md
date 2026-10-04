---
type: 功能
status: 已验证
project: SymoCraft
module: benchmark
created: 2026-10-04
---

# Benchmark 文件协议

关联协作者：[[Benchmark-执行与导出]]、[[Performance-Session类]]。协议不通过 C++ 共享头文件连接游戏和执行器。

## 当前设计

协议版本 `2`、游戏摘要 schema `2`、workload `2`；执行器索引 `runner_schema: 1`。workload 版本提升记录失焦恢复不再重置 benchmark 物理时序这一语义变化，不表示地形生成版本改变。

### 输入

```text
SymoCraft.exe --benchmark static|walk|edit --output NEW_CAPTURE_DIRECTORY
  --seed 424242 --width 1920 --height 1080 --vsync 0
  --warmup-seconds 60 --sample-seconds 180
  --focus-policy strict|allow-unfocused
```

路径通过 `CreateProcessW` UTF-16 参数传入；游戏进程 UTF-8 manifest 使既有窄字符资源加载兼容 Unicode Windows 路径。最低 Windows 版本见使用说明。命令行不经 shell，空格、引号、末尾反斜杠按 CRT 参数规则转义。

游戏默认仍是 `strict`；执行器默认显式传 `allow-unfocused`。普通游戏不能带 `--focus-policy`。旧游戏不支持这个参数时失败，不降级为“成功”。

1080p 窗口修复后，游戏采样使用普通无边框窗口；metadata 另记 `window_mode: borderless-windowed` 和 `taskbar_policy: preserve-shell-z-order`。它们是诊断扩展，不改变已有协议 2 的必需字段或严格尺寸校验；新游戏哈希及窗口呈现条件与旧包区分。见 [[Benchmark-1080p窗口修复]]。

### 输出

| 文件 | 生产者 / 时机 | 内容与约定 |
| --- | --- | --- |
| `session.yaml` | 执行器，每轮前后 | 固定计划、各轮最新 attempt、配置、环境、身份、整组结果和连续性 |
| `SCENE-N-attempt-M/result.yaml` | 执行器，每轮结束 | 退出码、取消/超时/强杀、错误、有效摘要；失败不执行后续轮次 |
| 同目录 `stdout.log` / `stderr.log` | 子进程标准流 | 原始诊断，不用日志里的成功字符串代替数据校验 |
| `capture/status.yaml` | 游戏，阶段边界 | `protocol_version`、`phase`、`elapsed_seconds`，同目录临时文件原子替换 |
| `capture/summary.yaml` | 游戏，退出导出 | completed、valid_run、invalid_reasons、metadata、帧时统计、焦点统计 |
| `capture/frames.csv` | 游戏，退出导出 | 原 25 列，所有预热和正式帧均保留，focused 为 0/1 |
| `capture/focus.csv` | 游戏，退出导出 | 第一帧和之后观测到的焦点变化：frame、elapsed_s、focused |
| `capture/memory.csv` | 游戏，退出导出 | 进程内存；可选 NVX 设备显存，缺失值留空 |
| `capture/final-frame.png` | 游戏，测量后 | 额外绘制/读回的截图，不计入测量帧 |

状态顺序：initializing → warmup → sampling → exporting → finished。阶段文件不是高频遥测；执行器每 250ms 读取一次，用最近阶段的 elapsed 加本地经过时间显示**近似**进度，不用界面进度计算性能指标。读取句柄允许原子替换，避免读者锁住状态文件。初始化最多受整轮总时限约束：协议时长 + 120 秒；取消/超时优雅关闭等待 3 秒，必要时终止自己的 Job，最多再等 5 秒。

游戏退出码：0 正常有效，2 资源预检失败，3 运行/导出异常，4 无效采样，64 参数错误。执行器 CLI：0 成功，4 已记录的采样失败/取消，64 参数或外层操作错误。

### 有效性与可比性

执行器核对退出码、completed/valid_run、空 invalid_reasons、协议/负载版本、场景、策略、预热/采样参数、分辨率/VSync、seed/区块数/世界摘要、实际 GL 设备及必要文件。流式解析逐帧 CSV，核对列数、帧序号、阶段、时序、焦点、样本量、编辑计数、P50/P95/P99、均值和 FPS；统计重算不剔除慢帧。

快速检查与正式协议分开标识。`continuous_full_protocol` 只说明未重试的九轮协议完成，不证明安静环境、Release 构建、温度稳定或性能达标。所有失败 attempt 永久保留；重试重新预热，不拼接中断帧。每轮前后核对包身份，同组核对实际 GPU/驱动字符串，防止中途替换程序或混用设备。

仅支持当前协议，不承诺前向兼容。游戏/执行器升级必须成套。原 M2-T3 v1 桌面结果及笔记本失败记录保持历史身份，不与 v2 汇总合并。

## 本次变更

无。版本化状态、焦点策略与失败重试记录已合并至当前设计。实现位置为 `tools/benchmark/src/runner.cpp`、`src/core/performance.cpp`、`src/core/startup_options.cpp`。验收统一维护于 [[Benchmark-执行与导出]]、[[Performance-失焦采样]]；最终构建与桌面测试身份见 [[Benchmark-桌面交付验证]]。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| 需要长期兼容多个游戏版本 | 增加明确的协议协商，而非宽松接受未知字段语义 |
| 结果文件来自不可信网络 | 单独的隔离导入与更严格格式资源上限；当前为本地生成包 |
