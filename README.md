# Symocraft

Symocraft 是面向 Windows x64 的 C++20 单机体素游戏工程，关注基础玩法、世界模拟、模块契约与可测量的图形呈现。

默认渲染后端为 **D3D12**；**Vulkan** 通过构建选项启用并显式选择；**OpenGL 4.6 Core** 保留为过渡与回归对照，不以永久维护三个等价后端为目标。

## 当前能力

- 固定 441 区块世界，包含噪声地形、植被、角色运动与碰撞、方块选择、放置和破坏。
- 显式 seed、回归场景、checkpoint、世界摘要及固定启动编辑序列。
- 九个生产模块与私有应用组合根；World、SimulationSession 及 simulation 内部独立物理核心支持不依赖窗口和图形 SDK 的 CPU-only 构建。
- CPU 模拟、网格、上传、异步 GPU 绘制计时，以及呈现等待、内存和绘制统计。
- 独立 Windows 原生 Benchmark，支持三种渲染后端及 static / walk / edit 场景。

M3-T2 平台迁移、M3-T3 双现代后端接管、T4 应用和模拟重构及 T5 ECS/内存均已获批准，M3 整体验收通过并保留已列风险。T5 已同步主规格并归档；按后续明确授权，EnTT typed 门面、调用迁移及必要构建/测试配套已同步主工作区。源码同步不是 Git 历史合并，也不将未测风险改为通过。详细结论见[验证索引](docs/verification/VERIFICATION.md)。

## 快速开始

构建环境为 Windows x64、Visual Studio 2026 C++ 工具链（MSVC tools 14.51.36231、Windows SDK 10.0.22621.0）、CMake 3.22+、Ninja、Python 3.10+。D3D12 另需 DXC 和支持 FL12_0 / SM6_0 的硬件与驱动。

在已初始化的 x64 Visual Studio 开发终端中运行：

```powershell
cmake --preset windows-debug -B out/build/msvc51-debug
cmake --build out/build/msvc51-debug
ctest --test-dir out/build/msvc51-debug --output-on-failure --no-tests=error
./out/build/msvc51-debug/bin/SymoCraft.exe
```

Release 使用 `windows-release` 预设并选择独立新目录。保留旧编译器缓存，不在原目录切换编译器。普通终端可通过 `python scripts/build.py --configuration Debug --build-directory out/build/msvc51-debug` 构建，并按环境提供 `--clion-path` 或 `CLION_HOME`；前置条件与完整参数见[脚本说明](scripts/README.md)。

T5 的 DBG01 为全部 Debug C++ 库及消费者统一设置 `_ITERATOR_DEBUG_LEVEL=0`，保留 Debug CRT、调试信息和运行时检查。即使编译器版本相同，也须为本次 ABI 变化选择全新目录、完整重建，不混用旧 level 2 对象或库；旧构建及历史成绩保留。

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

当前 M3 功能与兼容验收限定 RTX 5070 Ti 开发机。指定 Y9000P 中端和 GTX 1650 低端整机 benchmark 已改在现代性能分析工具接入后执行，不再作为下一玩法实施前的硬门槛，仍未完成且不免验。历史 OpenGL 基线不替代现代后端实测，阶段批准不代表全部缺陷关闭、长期稳定性或最终发布通过。

## 工程与文档

生产代码位于 `game/modules/` 与 `game/app/`，测试位于 `test/`，独立 Benchmark 位于 `tools/benchmark/`，自动化入口位于 `scripts/`。

[文档导航](docs/README.md) · [项目范围](docs/scope/project-scope.md) · [模块架构](docs/architecture/Overview.md) · [现役规格](docs/openspec/specs/) · [活动计划](docs/openspec/changes/) · [已知问题](docs/issue/issues.md) · [第三方依赖](docs/third-party.md)

各阶段验证正文位于对应 change 的 `verification.md`，全局验证索引仅维护关键节点批准与导航。协作与文档维护规则见 [AGENTS.md](AGENTS.md) 和[文档治理](docs/DOCUMENTATION_MAP.md)。

## 后续方向

T4/T5 迁移已经交付并同步主工作区；后续按独立授权开展性能、长时稳定性与独立部署验证，正式 physics 模块搬迁仍是 NPC 复用前的独立节点。

下一玩法阶段优先形成 **NPC 实体 → NPC AI → 村落演化** 的可观察闭环。Micro voxel 仅作为角色外表的构建方向，基础方块世界保持不变。独立计算服务与渲染客户端属于长期演进方向；存档、联网、区块流式加载及完整生存系统不计入当前已交付能力。
