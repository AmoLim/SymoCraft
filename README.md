# Symocraft

Symocraft 是面向 Windows x64 的 C++20 单机体素游戏工程，关注基础玩法、世界模拟、模块契约与可测量的图形呈现。

默认渲染后端为 **D3D12**；**Vulkan** 通过构建选项启用并显式选择；**OpenGL 4.6 Core** 保留为过渡与回归对照，不以永久维护三个等价后端为目标。

## 当前能力

- 固定 441 区块世界，包含噪声地形、植被、角色运动与碰撞、方块选择、放置和破坏。
- 显式 seed、回归场景、checkpoint、世界摘要及固定启动编辑序列。
- 九个生产模块与独立应用入口，世界核心支持不依赖窗口和图形 SDK 的 CPU-only 构建。
- CPU 模拟、网格、上传、异步 GPU 绘制计时，以及呈现等待、内存和绘制统计。
- 独立 Windows 原生 Benchmark，支持三种渲染后端及 static / walk / edit 场景。

M3-T2 平台迁移与 M3-T3 双现代后端接管已获阶段批准。T4 应用与模拟重构正在推进，T5 ECS 与内存契约处于规划阶段；候选实现、分支合入与阶段验收分别记录，互不替代。详细结论见[验证索引](docs/verification/VERIFICATION.md)。

## 快速开始

构建环境为 Windows x64、Visual Studio 2022 C++ 工具链及 Windows SDK、CMake 3.22+、Ninja、Python 3.10+。D3D12 另需 DXC 和支持 FL12_0 / SM6_0 的硬件与驱动。

在已初始化的 x64 Visual Studio 开发终端中运行：

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug
ctest --preset windows-debug --no-tests=error
./out/build/windows-debug/bin/SymoCraft.exe
```

Release 使用 `windows-release` 预设。普通终端可通过 `python scripts/build.py --configuration Debug` 构建，并按环境提供 `--clion-path` 或 `CLION_HOME`；前置条件与完整参数见[脚本说明](scripts/README.md)。

标准游戏预设启用 D3D12。验证层默认开启，缺少 D3D12 Debug Layer 时可显式传入 `--validation 0`。指定后端未编入或运行能力不足时会明确报错，不自动回退。

### Vulkan

Vulkan 默认不参与构建，需要 Vulkan SDK 头文件及支持 SPIR-V 编译的 DXC：

```powershell
cmake --preset windows-debug -B out/build/vulkan-debug -DSYMOCRAFT_RENDER_VULKAN=ON
cmake -E env VSLANG=1033 cmake --build out/build/vulkan-debug
ctest --test-dir out/build/vulkan-debug --output-on-failure --no-tests=error
./out/build/vulkan-debug/bin/SymoCraft.exe --renderer vulkan
```

`SYMOCRAFT_RENDER_OPENGL` 控制保留的 OpenGL 路径。CPU-only 使用独立构建目录及 `-DSYMOCRAFT_CPU_ONLY=ON`；构建边界见[工程说明](docs/architecture/engineering.md)。

## 运行与操作

常用参数包括 `--renderer d3d12|vulkan|opengl`、`--seed 424242`、`--check-assets` 和 `--smoke-frames 120`。资源按可执行文件位置解析，移动运行包时须保留相邻资源。

W / A / S / D 移动，左 Shift 奔跑，鼠标转动视角，滚轮调整视野，Space 接地跳跃；鼠标左键破坏、右键放置，Q / E 选择方块，Esc 退出。按住 Caps Lock 启用无重力、无碰撞调试模式，配合左 Ctrl 下降。

## Benchmark 与验证边界

构建后的 `SymoCraftBenchmark.exe` 提供原生 GUI 和命令行入口，支持后端选择、重复采样、取消、重试及结果导出。操作见[Benchmark README](tools/benchmark/README.md)，结果判定见[验证方法](docs/verification/methods.md)。

当前 M3 功能与兼容验收限定 RTX 5070 Ti 开发机。下一玩法阶段实施前仍需完成指定 Y9000P 中端和 GTX 1650 低端整机的候选版本性能验证。历史 OpenGL 基线不替代现代后端实测，阶段批准不代表全部缺陷关闭、长期稳定性或最终发布通过。

## 工程与文档

生产代码位于 `game/modules/` 与 `game/app/`，测试位于 `test/`，独立 Benchmark 位于 `tools/benchmark/`，自动化入口位于 `scripts/`。

[文档导航](docs/README.md) · [项目范围](docs/scope/project-scope.md) · [模块架构](docs/architecture/Overview.md) · [现役规格](docs/openspec/specs/) · [活动计划](docs/openspec/changes/) · [已知问题](docs/issue/issues.md) · [第三方依赖](docs/third-party.md)

各阶段验证正文位于对应 change 的 `verification.md`，全局验证索引仅维护关键节点批准与导航。协作与文档维护规则见 [AGENTS.md](AGENTS.md) 和[文档治理](docs/DOCUMENTATION_MAP.md)。

## 后续方向

近期技术工作集中于应用、平台、模拟以及 ECS 和内存生命周期契约，后续继续开展性能、长时稳定性与独立部署验证。

下一玩法阶段优先形成 **NPC 实体 → NPC AI → 村落演化** 的可观察闭环。Micro voxel 仅作为角色外表的构建方向，基础方块世界保持不变。独立计算服务与渲染客户端属于长期演进方向；存档、联网、区块流式加载及完整生存系统不计入当前已交付能力。
