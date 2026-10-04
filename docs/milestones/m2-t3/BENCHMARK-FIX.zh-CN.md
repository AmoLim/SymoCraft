# M2-T3 采样脚本兼容性补丁

本补丁只更新 `scripts/benchmark.ps1`，不更换游戏 exe、资源或采样工作负载。

在笔记本将补丁内容解压到原 `m2-t3` 目录，确认覆盖的是 `m2-t3/scripts/benchmark.ps1`，而不是多套了一层 scripts 目录。旧包清单记录旧脚本哈希，补丁身份另见本包 `patch-identity.json`。

修复 Windows PowerShell 5.1 可能取得空退出码、相对输出目录偏离终端当前位置的问题；增加明确的失败原因。失焦仍使整轮无效，不会为了通过而删除慢帧或失焦帧。

在原包目录先运行短测：

```powershell
./scripts/benchmark.ps1 -Executable ./SymoCraft.exe -OutputDirectory ./laptop-short-002 -WarmupSeconds 2 -SampleSeconds 10 -Repeats 1 -MachineLabel Y9000P -PowerMode 'plugged-in' -GpuMode 'discrete' -ClockSettings 'stock'
```

保持游戏窗口前台，不切到终端查看进度。成功时显示 `Finished 3 runs`，三项结果的 passed=true、exitCode=0、focusLostFrames=0。输出目录必须不存在；保留原失败记录。

短测通过后执行正式九轮：

```powershell
./scripts/benchmark.ps1 -Executable ./SymoCraft.exe -OutputDirectory ./laptop-baseline-002 -MachineLabel Y9000P -PowerMode 'plugged-in' -GpuMode 'discrete' -ClockSettings 'stock'
```

正式采样约 36 分钟。电源、显卡模式和频率参数应保持真实设置；有更具体的性能模式可补充记录，不需要为了修复脚本改变硬件设置。
