# SymoCraft Benchmark

Windows x64 原生采样工具，GUI 与 CLI 使用同一固定执行器。运行已交付包不需要 Python、PowerShell、CLion 或管理员权限；开发构建/测试另有工具链要求。工具无网络上传，不修改电源、驱动或系统设置。

本文同时安装为包内 `BENCHMARK-README.zh-CN.md`。以下运行、操作和结果说明不依赖源码仓库；末尾的开发入口与仓库链接仅用于源码 checkout。随包第三方盘点文件名为 `THIRD-PARTY-INVENTORY.md`。

## 运行条件

- Windows 10 1903+ / Windows 11 x64，实际驱动支持 OpenGL 4.6；不根据显卡品牌推定能力。
- 完整解压游戏、assets 和随包 VC++ Release 运行库，保持包目录结构，不直接在压缩软件内启动。
- 1920x1080 固定 framebuffer，VSync 请求关闭；尺寸不匹配时失败，不自动降分辨率。
- 采样是普通无边框窗口，不是独占全屏或置顶。保留任务栏原有行为，任务栏可能遮挡底部但不降低渲染尺寸。
- ZIP 依赖系统 `System32/tar.exe`；缺失时保留导出目录并报错，不下载安装替代工具。

执行器不提供 renderer selector，仍启动默认 OpenGL 并校验 GL 25 列协议。D3D12 游戏采样扩展不等于本工具已接入现代后端。

## GUI

1. 打开 `SymoCraftBenchmark.exe`，选择实际游戏路径及可写结果父目录。默认结果位置为 `%LOCALAPPDATA%/SymoCraftBenchmark/results`。
2. 填不含姓名的机器代号；电源、显卡模式和频率未知时保留未记录，不把手填声明当自动探测。
3. 先运行快速检查：static/walk/edit 各 10 秒预热和 10 秒采样，协议共 60 秒，启动/校验/截图另计。
4. 按本次授权运行正式基线：每场景三轮，每轮 60 秒预热和 180 秒采样，协议共 36 分钟。
5. 默认允许切换应用，不抢回焦点；不要最小化游戏、锁屏、休眠或改显示配置。可以最小化执行器。
6. 取消或失败后检查保留记录；重试从失败轮次新建 attempt 重新预热，原失败不覆盖。重试组合不标为连续正式基线。
7. 用“打开结果”查看，或“导出 ZIP”生成 `results.zip`；回收采样 ZIP，不必发送游戏或 DLL。

GUI 重试限当前会话，没有会话导入器；重新打开程序后可用 CLI 显式 resume。后台活动、驱动后台限帧及遮挡仍可能影响结果，能运行不等于受控性能达标。

## CLI

从包目录执行，输出必须是新目录；下面的目录名只是示例：

```powershell
./SymoCraftBenchmark.exe --run --quick --game ./SymoCraft.exe --output ./results/quick-001 --machine-label development
./SymoCraftBenchmark.exe --run --game ./SymoCraft.exe --output ./results/baseline-001 --focus-policy allow-unfocused
./SymoCraftBenchmark.exe --run --resume --game ./SymoCraft.exe --output ./results/baseline-001 --focus-policy allow-unfocused
./SymoCraftBenchmark.exe --export --game ./SymoCraft.exe --output ./results/baseline-001
```

`--resume` 必须带 `--run`，并传与原会话完全一致的 quick/focus/机器声明等设置及同游戏/资源。`--export` 与 `--run` 互斥，导出 quick 会话时也带 `--quick`。额外声明参数为 `--power-mode`、`--gpu-mode`、`--clock-settings`；焦点策略只接受 strict/allow-unfocused。无参数打开 GUI，不支持任意自定义计划或 shell 指令。

CLI 返回 0 为成功、4 为已记录采样失败/取消、64 为参数或外层操作错误。游戏瞬退、超时或缺数据不算通过；详情在该轮日志/result 和 session 中。取消只停止执行器创建的游戏进程，不能保证强制终止前游戏已导出 CSV；文件写入或磁盘故障也不保证存在最终报告，保留已有目录诊断。

## 保存与分享

`session.yaml` 是整组记录，各 `SCENE-N-attempt-M/result.yaml` 是执行器判定，`capture/summary.yaml` 是游戏观测；逐帧 `frames.csv`、焦点 `focus.csv`、内存 `memory.csv` 和 stdout/stderr 诊断同轮保存。完整 CSV 才能复算分布，单张 FPS 截图不足。失败结果保留，输出目录不重复使用；需要自行备份，不自动清理历史结果。

导出固定白名单，不包含 exe/DLL。已知游戏、结果与用户主目录路径仅在导出文本副本替换，本地原件不改；这不是通用匿名化，手填字段和第三方错误仍可能泄露信息，分享前检查。PNG 是游戏帧缓冲，不是桌面截屏。

FPS 是吞吐量，不是显示器实际呈现帧率。比较前核对游戏及六项资源的 SHA-256、协议/负载版本、实际 `gl_vendor/gl_renderer/gl_version`、策略与环境；手填独显/混合模式不能覆盖实际 GL 设备。原生工具未采集温度，NVX 是可缺失的设备级显存估计，不是跨厂商进程 VRAM，也不把未知值补成 0。失焦时长由帧间隔估算，不是逐事件精确计时。

通过意味着本次文件协议和数据校验完成，不意味着受控前台、性能达标或玩法验收。旧冻结包、不同焦点策略、不同后端或不同设备结果不能替代本次候选的验证；不删慢帧、不把重试帧拼成连续基线。被其他窗口遮挡时的呈现行为依赖系统与驱动，allow-unfocused 不保证消除后台竞争或等同完全可见的前台负载。

## 交付边界

当前包用于 Windows x64 工程测试，不能仅凭安装成功宣称无开发环境机器、AMD/Intel 真实驱动、完整人工玩法或公开发行已经通过。Release 随包 CRT 不代替干净机验证；Debug 运行库不是可公开分发的 Release 依赖。既有冻结结果保持原版本与原批准范围，不因本文更新重生成。

游戏包的 `licenses/` 与第三方盘点分别记录通知材料和来源。独立 runner 安装只含原生工具、适用 CRT 和两份说明，当前没有单独安装 yaml-cpp 许可；`THIRD-PARTY-INVENTORY.md` 不能代替完整许可文本，也不证明精确来源已全部核验。本次文档路径调整没有扩大安装或许可范围。

## 开发入口

游戏构建入口见 [根 README](../../README.md)。独立工具开发需已初始化的 VS2022 x64 环境和英语编译器语言包；配置及编译全程设置 `VSLANG=1033`，直接 build 不继承 configure preset 环境。从仓库根目录执行，使用一个新目录：

```powershell
cmake -E env VSLANG=1033 cmake -S tools/benchmark -B out/build/benchmark-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake -E env VSLANG=1033 cmake --build out/build/benchmark-release
ctest --test-dir out/build/benchmark-release --output-on-failure --no-tests=error
```

测试配置见 [test/benchmark](../../test/benchmark/CMakeLists.txt)，启用测试时需要 Python 3.10+；这不是已交付原生工具的运行依赖。本轮未重新执行这些构建命令。

可配置开发采集另有 [scripts/benchmark.py](../../scripts/benchmark.py)，仍是 strict/JSON collector 3，不代替原生 allow-unfocused/YAML 协议。旧 `.ps1` 只转发 Python，详见 [脚本说明](../../scripts/README.md)。

独立工具安装可在构建及适用测试通过后执行 `cmake --install out/build/benchmark-release --prefix out/install/benchmark-release`，使用新的暂存目录；该入口不安装游戏或 assets，完整组合包另按根工程部署。CMake 从本 README 和 [第三方盘点](../../docs/third-party.md) 安装两份说明，输出名保持上述包内名称。

仓库中的机制、方法、预算及关键节点分别从 [Benchmark 契约](../../docs/architecture/benchmark.md)、[验证方法](../../docs/verification/methods.md)、[项目范围](../../docs/scope/project-scope.md) 和 [VERIFICATION](../../docs/verification/VERIFICATION.md) 查阅；这些仓库相对链接不要求随运行包分发。本文核对与文档消费者测试不代表本轮已运行 GUI、GPU、正式采样或上述原生构建命令。
