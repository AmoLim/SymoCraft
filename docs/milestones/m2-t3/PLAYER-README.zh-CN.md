# Symocraft M2-T3 候选版

本包用于 Windows x64 桌面与 Y9000P 的 M2 验证，包含现有单机玩法、固定场景与性能采集。它不是 M6 最终发行版，也不代表人工玩法、笔记本性能或长时稳定性已经通过。

## 启动和操作

保持 `SymoCraft.exe`、`assets` 与 `scripts` 在同一包目录。建议使用 ASCII 路径，可含空格。正常启动 exe 随机生成固定大小世界，VSync 开启；需要支持 OpenGL 4.6 的显卡与驱动。若报告缺少 MSVC x64 运行库，记录错误后处理运行依赖，不从不明网站下载单个 DLL。

W/A/S/D 移动，Left Shift 跑动，鼠标转向，Space 跳跃；左键移除、右键放置方块，Q/E 切换材料，滚轮改变视野，Esc 退出。按住 Caps Lock 进入无碰撞/重力的调试移动，期间 Left Ctrl 下降。当前没有存档、联网或动态区块加载。

在包目录打开 PowerShell，可进行无窗口资源检查或固定场景启动：

```powershell
./SymoCraft.exe --check-assets
./SymoCraft.exe --seed 424242
./SymoCraft.exe --scene regression --checkpoint four-chunk
```

## 性能采样

先短测，再正式采样。采样模式自动移动或编辑，不响应普通游戏操作；保持窗口前台，不最小化、不调整分辨率，不同时编译或运行其他图形负载。Esc/关闭可以取消，但取消不算有效基线。每次使用新输出目录，脚本不会自动覆盖旧结果。

笔记本接电，下面的电源、显卡和频率参数要填写真实设置，不要原样保留占位文字。实际 OpenGL 渲染设备应为 RTX 3070 Ti Laptop；脚本会记录机器信息及可用的 NVIDIA GPU 传感器数据，不会改变系统设置。

```powershell
./scripts/benchmark.ps1 -Executable ./SymoCraft.exe -OutputDirectory ./laptop-short-001 `
  -WarmupSeconds 2 -SampleSeconds 10 -Repeats 1 -MachineLabel Y9000P `
  -PowerMode 'plugged-in; fill actual performance mode' `
  -GpuMode 'fill hybrid or discrete' -ClockSettings 'fill actual setting'

./scripts/benchmark.ps1 -Executable ./SymoCraft.exe -OutputDirectory ./laptop-baseline-001 `
  -MachineLabel Y9000P -PowerMode 'plugged-in; fill actual performance mode' `
  -GpuMode 'fill hybrid or discrete' -ClockSettings 'fill actual setting'
```

正式协议：1920 x 1080 framebuffer、VSync 请求关闭、seed 424242；静止、移动、连续编辑各预热 60 秒、采样 180 秒、重复 3 次，总计约 36 分钟加启动/导出时间。短测仅检查采集链路，不计入正式结果。驱动强制 VSync、混合显卡设置和未知条件需另行说明。

保留并返回整个结果目录，包括 `machine.json`、`results.json`、各轮 stdout/stderr、`capture/frames.csv`、`memory.csv`、`summary.yaml`、截图和 GPU 传感器文件。只有 FPS 截图无法复核慢帧。CPU 温度当前缺失，设备显存不能冒充游戏进程的精确显存。

## 版本和验收边界

包内 `build-identity.json` 记录构建输入和 exe 身份，`package-manifest.json` 列出打包文件哈希。不要混用不同包的 exe、资源或采样脚本做同一组对照。

性能脚本验证固定工作负载，不验证玩家鼠标点击、碰撞手感、失焦恢复或 15 分钟人工游玩。两台机器的完整玩法检查与人工验收仍独立进行。运行依赖部署、完整 Unicode 路径及第三方发布材料的最终核对保留到 M6。
