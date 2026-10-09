# Symocraft

Symocraft 是一个面向 Windows x64 的 C++20 单机体素游戏工程，由课程原型逐步演进为可维护、可测量的图形与游戏引擎项目。项目同时关注游戏玩法、模块契约，以及从世界模拟到 GPU 呈现的数据流。

当前可玩路径使用 **SDL3 + OpenGL 4.6 Core**，支持固定世界中的移动、跳跃、方块放置与破坏。现代渲染路线是先由 **D3D12** 接管现有玩法，再接入 **Vulkan**；OpenGL 仅作为过渡与回归对照，不以永久维护三个等价后端为目标。现代后端实验不等于生产玩法接管已经完成。

完整范围、阶段 spec 与验收约定见 [项目约定](docs/README.md#项目约定)；实现说明和验证证据从 [技术文档](docs/README.md) 进入。

## 当前能力与进度

- **基础玩法**：固定 441 区块世界、噪声地形与植被、角色运动和碰撞、方块选择及编辑。
- **可复现回归**：显式 seed、固定边界场景与 checkpoint、世界摘要和固定启动编辑序列。
- **模块化构建**：九个生产模块与独立 app，文件夹边界与 CMake target 边界并行约束；测试集中在根级 `test/`。
- **世界契约**：显式 World 实例所有权、只读查询、编辑版本，以及供消费者借用的 CPU 网格数据；世界核心不依赖窗口或图形 SDK。
- **SDL3 平台层**：窗口、输入、像素数据和私有图形桥接；SDL3 通过源码构建，图形 SDK 不作为平台公共契约暴露。
- **性能观测**：CPU 模拟、网格和上传成本，异步 GPU 绘制计时、present 等待、内存及绘制统计；CPU 提交时间不冒充 GPU 执行时间。
- **Benchmark**：独立 Windows 原生执行器，支持 static / walk / edit 场景、重复采样、取消、重试与结果导出。

M3-T0 模块迁移和 M3-T1 世界重构已有实现与阶段证据。**M3-T2 已于 2026-10-09 获用户正式节点批准**；T3 现代渲染接管仍在推进。节点批准不代表所有遗留问题、性能预算或整个 M3 已通过，详细状态以各阶段报告为准。

## 项目范围与方向

当前开发保持 Windows x64、单玩家与固定世界，不把区块流式加载、存档、联网、完整生存系统或通用编辑器自动纳入已交付功能。D3D11 及已约定路线以外的图形 API 不在当前范围内。

下一玩法阶段的 P0 是 **NPC 实体 → NPC AI → 村落演化** 的可观察闭环；生态生成、装备栏和攻击动作属于后续玩法范围，不作为完整实现的前置条件阻塞 P0。

Micro voxel 仅作为 NPC / 玩家实体外表的构建方向，基础方块世界保持原样，不扩展为细体素世界、体素破坏或光追系统。

长期逐步形成 **独立计算服务 + 渲染客户端**：计算服务承载权威世界模拟，客户端负责呈现与交互。先建立可无窗口运行的逻辑边界，再按需求验证独立进程和跨机器部署；这不是已经交付的联网功能，也不是 GPU compute shader 或云端视频渲染。

上述方向是范围约定，具体实施、规模与验收须在相应阶段 spec 中确认。

## 工程结构

```text
game/
  app/          应用入口与运行编排
  modules/      九个生产模块
test/           单元、集成、契约测试及独立实验
tools/
  benchmark/    原生 Benchmark 执行器
cmake/          构建与边界检查
scripts/        开发辅助脚本
assets/         运行资源
vendor/         第三方依赖源码与材料
docs/           范围、设计、阶段报告及验证证据
out/            本机构建、运行包与采样产物，不纳入版本控制
```

| 模块 | 主要职责 |
| --- | --- |
| foundation | 基础类型、文件与通用底层契约 |
| scene | 中立场景数据与共享数据契约 |
| assets | 资源读取与图像解码 |
| ecs | 实体与组件存储 |
| world | 世界生成、查询、编辑及 CPU 网格 |
| simulation | 角色运动、碰撞与模拟推进 |
| platform | SDL3 窗口、输入与平台桥接 |
| renderer | 图形资源、绘制与后端实现 |
| telemetry | 性能采样、统计与导出 |

模块职责、依赖方向和公开接口见 [架构索引](docs/project/architecture/Overview.md)。物理核心的进一步独立化、渲染 PImpl 与现代后端属于后续重构路线，不能由目录名称推定已经完成。

## 构建与测试

开发环境：**CLion + MSVC + CMake**。需要 Visual Studio 2022 C++ 工具、Windows SDK、英语编译器语言包、CMake 3.22 或更新版本，以及 Ninja。共享 presets 使用 `VSLANG=1033`；支持 CLion 自带的 CMake 和 Ninja。

当前游戏需要实际支持 OpenGL 4.6 Core 的驱动。CPU-only 和资源单元测试不需要窗口或 GPU；真实图形验证仍需对应设备与驱动。

在已初始化的 x64 Visual Studio 开发终端中执行：

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug
ctest --preset windows-debug --no-tests=error

cmake --preset windows-release
cmake --build --preset windows-release
ctest --preset windows-release --no-tests=error
```

普通 64 位 PowerShell 可使用辅助脚本：

```powershell
./scripts/build.ps1 -Configuration Debug -CLionPath 'C:/path/to/CLion'
./scripts/build.ps1 -Configuration Release -Action Install -CLionPath 'C:/path/to/CLion'
```

将 CLion 路径替换为实际安装位置，或设置 `CLION_HOME`。脚本默认执行配置、构建与测试，不安装工具或修改系统设置；Install 动作在检查后生成开发用安装目录。

CLion 中使用名为 **Symocraft MSVC** 的 Visual Studio toolchain，选择 amd64/x64 并启用两个 presets。详见 [构建与 CLion 指南](docs/project/build/build-and-clion.md)。

### 独立构建边界

默认同时构建游戏和 Benchmark。可通过 CMake 开关选择范围：

| 开关 | 用途 |
| --- | --- |
| `SYMOCRAFT_BUILD_GAME` | 构建游戏 |
| `SYMOCRAFT_BUILD_BENCHMARK` | 构建原生 Benchmark |
| `SYMOCRAFT_BUILD_CPU_MODULES` | 构建 CPU 模块 |
| `SYMOCRAFT_CPU_ONLY` | 仅构建 CPU 模块及适用测试，关闭游戏和 Benchmark |
| `BUILD_TESTING` | 是否构建测试 |

例如，在上述开发终端中建立独立 CPU-only 目录：

```powershell
cmake --preset windows-debug -B out/build/cpu-debug -DSYMOCRAFT_CPU_ONLY=ON
cmake --build out/build/cpu-debug
ctest --test-dir out/build/cpu-debug --output-on-failure --no-tests=error
```

测试覆盖资源定位、世界确定性与编辑、模块契约、失败清理等行为。Shader 加载单元测试使用驱动替身，不能代替真实 GLSL 编译；真实 SDL3、窗口输入和 GPU 实验各自留证，不用 CPU 测试推定图形或人工玩法通过。

## 运行

```powershell
./out/build/windows-debug/bin/SymoCraft.exe
./out/build/windows-debug/bin/SymoCraft.exe --check-assets
./out/build/windows-debug/bin/SymoCraft.exe --smoke-frames 120
./out/build/windows-debug/bin/SymoCraft.exe --seed 424242
./out/build/windows-debug/bin/SymoCraft.exe --scene regression --checkpoint four-chunk
./out/build/windows-debug/bin/SymoCraft.exe --world-summary --seed 424242 --scene regression --test-edits
```

- 资源部署在可执行文件旁，按可执行文件位置解析，不依赖当前工作目录；移动运行包时保留相邻资源。
- `--check-assets` 只检查必需文件，不创建窗口，也不验证图片、shader 内容或玩法。
- `--smoke-frames 1..10000` 运行真实游戏并在指定帧数后退出，需要图形环境，不替代人工玩法、长时稳定性或性能验收。
- 普通启动使用随机 seed 并记录实际值；指定 seed 仅保证在相同生成版本、配置及构建环境下复现初始世界，不承诺任意编译器或平台间一致。
- `--world-summary` 无窗口输出 YAML；`--test-edits` 是固定启动编辑序列，不是玩家输入回放。

回归场景与 checkpoint 见 [固定世界与边界场景](docs/project/benchmark/testing/reproducible-scenes.md)。完整游戏的旧资源加载路径在 Unicode 支持验证完成前，建议使用 ASCII 安装路径。

`cmake --install out/build/windows-release` 默认生成 `out/install/windows-release`。这是开发用 staging，不是已经通过无开发环境部署验收的最终发布包。

### 操作

| 输入 | 动作 |
| --- | --- |
| W / A / S / D | 移动 |
| 左 Shift | 奔跑 |
| 鼠标 | 转动视角 |
| 滚轮 | 调整视野 |
| Space | 接地时跳跃 |
| 鼠标左键 / 右键 | 破坏 / 放置方块 |
| Q / E | 选择方块类型 |
| 按住 Caps Lock | 无重力、无碰撞的调试模式 |
| 按住 Caps Lock 时按左 Ctrl | 下降 |
| Esc | 退出 |

## Benchmark 与硬件边界

构建后可运行独立执行器：

```powershell
./out/build/windows-release/bin/SymoCraftBenchmark.exe
```

执行器与游戏分进程运行，支持三场景采样及原始帧数据、摘要、日志和图表导出。正式冻结协议采用每场景预热 60 秒、采样 180 秒、重复三轮；失焦记录进入分析，最小化等无效条件仍独立判定。操作与交付见 [Benchmark 使用与交付](docs/project/benchmark/使用与交付.md)。

2026-10-05 已归档三档整机的 OpenGL 基线：开发机 Ryzen 7 9700X / RTX 5070 Ti / 64 GB、指定中端 Y9000P i7-12700H / RTX 3070 Ti Laptop / 32 GB，以及指定低端 i7-10750H / GTX 1650 / 约 16 GB。具体身份、条件和结果见 [基准冻结记录](docs/project/benchmark/exp/Benchmark-基准冻结-20261005.md)。

当前 M3 功能与兼容验收限定开发本机 RTX 5070 Ti。**下一玩法功能开始实施前，仍必须验证届时候选版本在指定中低端整机上的性能**。旧 OpenGL 数据不能代替新后端实测；设备分档不是最低配置认证，数据有效也不等于性能达标或 M4 通过。

## 后续里程碑与已知限制

| 节点 | 方向 |
| --- | --- |
| M3-T0 / T1 | 已有模块边界与世界重构实现，阶段结论见报告 |
| M3-T2 | SDL3 平台迁移，用户已正式批准节点 |
| M3-T3 | D3D12 先接管现有玩法，再接入 Vulkan；两者完成后验收 |
| M3-T4 | 完善应用、平台与模拟接口，逐步独立物理核心 |
| M3-T5 | ECS、内存与生命周期契约，以及 M3 整体验收 |
| M4 | 同条件测量与性能优化，不隐降世界规模或画质 |
| M5 | 至少 60 分钟连续运行及异常场景稳定性 |
| M6 | 脱离源码与 IDE，在无开发环境目标机器上验证 Release 交付 |

T2 批准未关闭 Win32 1175 文件发布问题；Fix1 当前暂停，正式同源 GLFW / SDL3 Q06 对照与首次可操作时间预算仍未完成。树叶面剔除、视角跳变等已记录问题继续保留。不要把节点批准或局部成功记录读成所有测试全绿、全部预算达标或最终发布通过。

阶段入口：[T0](docs/milestones/m3-t0/README.md)、[T1](docs/milestones/m3-t1/README.md)、[T2](docs/milestones/m3-t2/README.md)、[T3](docs/milestones/m3-t3/README.md)。待处理问题见 [Issues](docs/issue/issues.md)，依赖及通知材料见 [第三方依赖盘点](docs/legacy/third-party-inventory.md)。

构建成功、启动成功、玩法通过、性能达标、长时稳定性和独立交付分别验收；README 是项目入口，不替代阶段证据。

## 学习参考

- [LearnOpenGL](https://learnopengl.com/) 与 *OpenGL SuperBible*
- [Procedural generation tutorial](https://www.youtube.com/watch?v=wbpMiKiSKm8)
- [Voxel mesh generation](https://0fps.net/2012/06/30/meshing-in-a-minecraft-game/)
