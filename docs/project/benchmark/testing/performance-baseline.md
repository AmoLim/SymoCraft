---
tags:
  - area/benchmark
  - topic/performance
---

# M2-T3 双机性能采样

> 适用范围：M2-T3 旧 PowerShell 采样流程与候选包，保留当时的协议、命令和交接约定。原生 Benchmark 的操作见 [使用与交付](../使用与交付.md)，焦点策略见 [失焦采样](../performance/Performance-失焦采样.md)。下文的禁止切出要求属于旧流程，不覆盖新策略；两者的版本、结果格式和证据不可混用。本轮未重新执行采样。

> 2026-10-05 当前基准已 [冻结](../exp/Benchmark-基准冻结-20261005.md)：指定 GTX 1650 低端、Y9000P 中端及开发机九轮均已归档；采用 allow-unfocused，失焦不失败、不删帧，不要求前台重测才成立。下文“双机”和 strict 禁止切出是旧脚本协议的历史范围，不覆盖当前规则，不要求用旧脚本重跑或改写旧失败记录。

## 执行约定

本指南适用于台式机和指定 Y9000P，二者必须分别实测。使用同一 Release exe、资源、生成与工作负载版本；保存 exe/资源/脚本哈希。正式配置为 1920×1080 framebuffer、VSync 请求关闭、seed=424242、441 区块、原有渲染设置，三个场景各预热 60 秒、采样 180 秒、重复 3 次，约 36 分钟加启动及导出时间。

普通交互模式默认 VSync 开启不变；采样的 VSync 是向驱动请求的 swap interval，控制面板强制覆盖需单独记录。不要更改窗口尺寸、最小化或切出；Esc 可取消，取消不算通过。不要在采样期间编译、录像、跑其他基准或运行另一个游戏。记录无法避免的后台活动，受干扰则保留原轮、另建目录整轮重测。

机器记录包括操作系统、CPU、内存、实际 GL renderer/版本、驱动、电源与性能模式、独显/混合模式、超频/降压和温度。未知字段写未记录，不把桌面限帧或少开核心当作低配实测。笔记本必须接电，并确认实际 GL renderer 是 RTX 3070 Ti Laptop，而不是只根据机器型号推测。

## 本机运行

2026-10-03 兼容性补丁：Windows PowerShell 5.1 用户应先使用采集脚本版本 2，修复退出码为 null 和相对输出路径偏移；原候选 exe 不变。新版在 `machine.json` 记录 `collectorScriptVersion` / `powershellVersion`，失败结果新增 `failureReasons`。见 [Y9000P 首轮诊断与补丁](../../../milestones/m2-t3/laptop-first-run.md)。

先按构建指南构建并测试 Release；当前安装候选版位于 `out/install/m2-t3`。以下命令在仓库根目录运行，结果目录必须是新目录。

```powershell
# 接入检查，不是正式基线
./scripts/benchmark.ps1 -Executable ./out/install/m2-t3/SymoCraft.exe `
  -OutputDirectory ./out/m2-t3/my-short-001 -WarmupSeconds 2 -SampleSeconds 10 -Repeats 1 `
  -MachineLabel desktop -PowerMode normal -GpuMode discrete -ClockSettings stock

# 正式九轮，每个场景独立启动三次
./scripts/benchmark.ps1 -Executable ./out/install/m2-t3/SymoCraft.exe `
  -OutputDirectory ./out/m2-t3/my-baseline-001 `
  -MachineLabel desktop -PowerMode normal -GpuMode discrete -ClockSettings stock
```

单场景或手动检查可以直接运行：

```powershell
./out/install/m2-t3/SymoCraft.exe --benchmark edit --output out/m2-t3/manual-edit-001 --seed 424242 --width 1920 --height 1080 --vsync 0 --warmup-seconds 60 --sample-seconds 180
```

三个模式为 `static`、`walk`、`edit`，会自动选择 regression 场景和检查点。不能组合 `--scene`、`--checkpoint`、`--test-edits`、`--smoke-frames`、`--world-summary` 或 `--check-assets`。`--output` 必填；采样时间 1..600 秒，预热 0..600 秒。所有采样选项只对 benchmark 有效，不改变普通玩法参数。

## 笔记本交接

使用本项候选包的同一份 exe、`assets` 和 `scripts/benchmark.ps1`，放在 ASCII 路径（可含空格），例如 `C:/Games/Symocraft-M2-T3`。这是开发候选，不是 M6 最终发行；若系统缺少 MSVC x64 运行库，先记录启动错误并按构建/部署说明处理，不能把缺 DLL 算作性能测试通过。

1. 接电，记录 Lenovo/Windows 性能模式、混合或独显模式、驱动与是否改过频率/电压。不要为达标临时降分辨率或世界规模。
2. 在包目录打开 PowerShell，执行 `./SymoCraft.exe --check-assets`，再执行短测。短测确认生成、GPU查询、数据导出与截图正常；若失焦或失败，检查 stderr/summary 并使用新的结果目录重试。
3. 执行正式命令，将参数说明替换为真实条件；三个场景都要完成三轮。
4. 保留并返回整个结果目录，尤其是原始 CSV、机器条件和失效轮次；不能只发平均 FPS 截图。当前未配置 CPU 温度采集，若另有传感器工具，记录型号、采样频率、时间基准及附加开销后一起提供，不能填估算值。

```powershell
./scripts/benchmark.ps1 -Executable ./SymoCraft.exe -OutputDirectory ./laptop-short-001 `
  -WarmupSeconds 2 -SampleSeconds 10 -Repeats 1 -MachineLabel Y9000P `
  -PowerMode 'plugged-in; fill actual performance mode' -GpuMode 'fill hybrid or discrete' -ClockSettings 'fill actual setting'

./scripts/benchmark.ps1 -Executable ./SymoCraft.exe -OutputDirectory ./laptop-baseline-001 `
  -MachineLabel Y9000P -PowerMode 'plugged-in; fill actual performance mode' `
  -GpuMode 'fill hybrid or discrete' -ClockSettings 'fill actual setting'
```

`nvidia-smi` 可用时脚本自动记录 GPU 温度、功耗、频率及设备显存；没有工具时继续保留缺失状态，不安装驱动或改变系统设置。`-SkipGpuSensors` 只用于诊断：正式对比必须说明传感器条件与原采样不同，不能悄悄删除采集负载。

## 结果文件

| 文件 | 用途 |
| --- | --- |
| `machine.json` | 运行条件、工具/程序身份、请求的采样配置；未知值明确保留 |
| `results.json` | 所有已尝试轮次的紧凑结果；无效轮次会停止后续启动 |
| `static-1` 等目录 | 每轮独立目录，包含 stdout/stderr、退出码、命令行及可选传感器 CSV |
| `capture/frames.csv` | 全部预热及测量帧，CPU/GPU 区间、顶点/上传/重建/编辑计数、位置及焦点 |
| `capture/memory.csv` | 约每秒一次进程工作集、私有字节、驱动设备显存估计 |
| `capture/summary.yaml` | 完整世界输入与摘要、初始化分段、有效性原因、分位数及 GPU 缺失数 |
| `capture/final-frame.png` | 测量结束后额外渲染的诊断截图，不是整段视觉验收 |

每轮看 P95/P99、最大帧、编辑帧与 GPU 缺失数量，不删慢帧。分别比较三次运行，不把不同机器、配置或协议的样本直接混成一份分布。GPU时间、上传调用CPU时间和呈现等待的含义见 [模块说明](../../../legacy/architecture/performance-observation.md)，这些列不可简单累加。

静止场景应没有持续网格重建；移动场景应有位置变化与往返；编辑场景应产生约每秒 4 次写入及受影响区块重建。它们是数据合理性检查，不是先验性能目标。若渲染恢复失败、严重卡顿阻断玩法或产生未解释的 GL 错误，先修复再采样。

完整逐帧 CSV 和 exe/DLL/obj/lib/pdb/压缩运行包放在 `out` 或交付包目录，不进入 `docs`。`docs/milestones/m2-t3` 保留说明、报告、关键 log、小型汇总和少量截图。完整原始数据不入 Git 时，应另外备份；只有小型摘要无法重新计算整个帧分布。
