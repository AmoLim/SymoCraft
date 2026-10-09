---
tags:
  - area/milestones
---

# Y9000P 首轮采样诊断与脚本修复

日期：2026-10-03。本次检查用户从 Y9000P 返回的正式 static-1 数据。**游戏完成了约定时长，但因失焦无效；同时发现并修复采样脚本的 Windows PowerShell 5.1 兼容性问题。** 不将该轮转为有效基线，不修改用户原始结果，笔记本九轮正式验收仍待执行。

源码/夹具链接固定到迁移前 `cadc349` 快照用于旧路径定位，不把它当作当次脚本补丁的精确版本或新的实测证据。

## 已确认事实

原始上传目录为 `docs/testing/laptop-baseline-001(1)/laptop-baseline-001`，本次未移动或删除。小型日志副本见 [机器条件](evidence/laptop-first-run/machine.json)、[脚本结果](evidence/laptop-first-run/result.json)、[游戏输出](evidence/laptop-first-run/stdout.log) 和 [应用摘要](evidence/laptop-first-run/summary.yaml)。

- 游戏 exe SHA256 与桌面基线一致，为 `381211C0FF055F2420BE0B5AD8CD11564ACA6324D12DA5EF207A5094D81B9C3F`。
- 实际 GL renderer 为 NVIDIA GeForce RTX 3070 Ti Laptop GPU/PCIe/SSE2，驱动 595.59；纹理/着色器正常加载，并非不支持 OpenGL 或缺资源导致本次停止。
- `completed: true`，但 `valid_run: false`，原因为 `window-not-focused`。游戏输出 `[runtime] shutdown complete; exit_code=4`，是受控无效退出，不是崩溃。
- 总计 22016 帧，其中正式样本 16440 帧；6531 帧失焦，连续发生于 elapsed 168.4151004..239.9930018 秒，约为最后 72 秒。日志不能证明用户切窗还是其他应用抢焦点。
- 脚本 `exitCode: null` 与游戏日志的退出码 4 不同，说明外层没有可靠取得系统退出码。即使修复这个问题，已有失焦仍足以使该轮无效。
- 用户在 F 盘包目录运行命令，输出却位于 `C:/Users/ShiRo/laptop-baseline-001`。脚本将相对路径交给 `.NET Path.GetFullPath`，它使用进程目录，不一定等于 PowerShell 的当前位置。

这些帧的 P95/P99 仅用于故障诊断，不纳入有效双机对照，也不通过删除失焦样本来“修正”基线。

## 修复范围

`scripts/benchmark.ps1` 的采集脚本版本为 2，游戏的 schema/workload 版本仍为 1，exe 和实际帧采集逻辑不变。

1. 输出目录通过 PowerShell `GetUnresolvedProviderPathFromPSPath` 解析，支持尚不存在的目录，并限制为 FileSystem provider。仍拒绝覆盖已有结果。
2. `Start-Process -PassThru` 后立即保留进程 Handle，再等待退出和输出结束，最后读取一次实际 ExitCode；不从游戏日志猜测退出码，也不把 null 当作 0。
3. 结果新增 `failureReasons`，终端报错区分失焦帧数、缺失退出码、非零退出码、缺少导出/退出标记、缺少 CSV、无采样帧、GL 诊断和超时。
4. `machine.json` 新增采集脚本版本和 PowerShell 版本，便于追溯。失焦规则、GPU 传感器参数、预热/采样时长、分位数算法均未放宽或改变。

PowerShell 位置与进程目录的差异、解析 API 的契约见 [Microsoft about_Locations](https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.core/about/about_locations?view=powershell-7.5) 和 [路径解析 API](https://learn.microsoft.com/en-us/dotnet/api/system.management.automation.pathintrinsics.getunresolvedproviderpathfrompspath?view=powershellsdk-7.6.0)。退出码问题由本机 Windows PowerShell 5.1 的无窗口夹具实际复现，不仅依赖文档推断。

## 验证

新增 [原生子进程夹具](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/tests/benchmark_process_fixture.cpp) 与 [脚本契约测试](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/tests/benchmark_script_tests.ps1)。它们不是游戏或性能结果，不创建窗口、GL 上下文或操作键鼠。

测试人为分离 PowerShell 与原生进程目录，覆盖相对路径/空格路径、成功及立即退出、退出码 0/3/4、失焦失败、失败后不再启动后续轮次、GPU 缺失样本统计、预热排除和旧目录保护。Windows PowerShell 5.1 测试固定注册；找到 PowerShell 7 时额外注册对应测试，因此本机总计 17 项，只有 Windows PowerShell 的环境为 16 项。

修复前成功夹具也被记录成 `exitCode: null`，且写到原生目录；[复现日志](evidence/script-fix/m2-t3-script-fix-reproduced.log) 保留。测试启动环境中另遇到 CTest 继承的 Path/PATH 重复大小写，已仅在测试子进程内规范化，不修改用户系统环境。

修复后 [Release 17/17](evidence/script-fix/m2-t3-script-fix-release-tests.log)、[Debug 17/17](evidence/script-fix/m2-t3-script-fix-debug-tests.log) 通过；真实桌面游戏未重新采样。构建后 Release exe 哈希仍与原候选一致，原桌面九轮证据保留不动。

## 笔记本重试

补丁为 `out/packages/Symocraft-M2-T3-benchmark-fix.zip`，只含脚本、说明及补丁身份记录。将其中 `scripts/benchmark.ps1` 更新到笔记本原 `m2-t3/scripts/benchmark.ps1`，不替换 exe 或 assets。旧包及其原始清单继续保留历史身份，补丁后的脚本哈希以 [补丁身份](evidence/script-fix/patch-identity.json) 为准。

在笔记本包目录先执行三场景短测：

```powershell
./scripts/benchmark.ps1 -Executable ./SymoCraft.exe -OutputDirectory ./laptop-short-002 -WarmupSeconds 2 -SampleSeconds 10 -Repeats 1 -MachineLabel Y9000P -PowerMode 'plugged-in' -GpuMode 'discrete' -ClockSettings 'stock'
```

成功应显示 `Finished 3 runs`，`results.json` 三项 passed 均为 true，exitCode 均为 0，focusLostFrames 均为 0。保持游戏前台，不切回终端看进度，也不要点击其他窗口或让远程控制窗口占据焦点。若启动本身没有取得焦点，不把点击后剩余部分算作完整有效轮；保留诊断并反馈。

短测通过后，在相同环境运行正式九轮，使用新的 `./laptop-baseline-002` 输出目录，省略短测时长/次数参数。保留原失败轮，不与新结果混合。若仍失败，回传新目录的 result.json、stdout/stderr 和 summary.yaml；新版报错会直接列出已检测到的原因。
