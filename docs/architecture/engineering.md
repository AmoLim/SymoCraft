---
tags:
  - area/architecture
---

# 工程级契约

2026-10-17本地同步状态：feature-refactoring已取得T3代码，游戏生产owner为symocraft_app_runtime，支持按开关启用的真实D3D12/Vulkan与隔离GL；CPU-only关闭三图形后端。下述独立T4候选的symocraft_application/physics机制不包含在本次同步，不能以文档描述推定源码已组合。

本页维护当前构建、依赖隔离、测试及部署的正确性约束，不记录逐函数行为或历史测试成绩。操作入口见[根 README](../../README.md#构建与测试)与[开发脚本](../../scripts/README.md#build-setup-and-actions)；阶段结论、冻结材料与未验证项从[验证索引](../verification/VERIFICATION.md)进入。

## 工具链与配置生命周期

- 支持 Windows x64、MSVC、C++20，CMake 最低 3.22。共享 Debug/Release presets 使用单配置 Ninja；架构环境由 x64 VS2022 开发环境或 IDE 提供，Ninja 不负责初始化 SDK、头文件和库环境。
- Ninja 的 MSVC 头依赖解析要求编译器安装有 `1033/clui.dll`，配置与编译均使用 `VSLANG=1033`，检测前缀为 `Note: including file:`。三者由[环境检查](../../cmake/ValidateMsvcEnvironment.cmake)在配置期校验，不靠过滤日志或修改生成文件规避。
- 编译器、生成器、语言依赖检测或配置模式改变时使用独立构建目录；旧缓存及对象不能直接充当新条件的验证基线。尤其补装语言包后，普通增量成功不能证明旧依赖数据库有效。
- 仓库不固化本机工具绝对安装路径，不自动安装或下载工具。开发自动化使用 Python 3.10+ 标准库；Windows 构建助手需要 64 位 Python，以隔离的 VS 环境启动 CMake/Ninja/CTest，不修改调用者环境、系统 PATH 或 IDE。
- 配置成功只说明目标模型已建立。生成、编译、资产复制、链接和 CTest 是后续独立阶段；任何阶段失败都不保证已生成文件或已复制资源事务回滚，不把局部输出当作完整交付。

依据：[根构建](../../CMakeLists.txt)、[预设](../../CMakePresets.json)、[构建助手](../../scripts/build.py)。

## 生产所有权与依赖

目录是所有权边界，CMake target 是可验证的编译与链接边界。`game/modules/` 的九模块拥有各自实现；`game/app/` 拥有启动、转接及应用编排；`tools/benchmark/` 是独立产品，不是游戏模块；统一测试位于 `test/`。

除 scene 值类型目标及 renderer 的两个明确接口目标外，生产模块使用拥有真实实现的 STATIC target。公开头只通过所属目标的窄 `include/` 根提供，不能依赖全仓库、整个 vendor 或其他模块的 `src/` 搜索路径。

以下为允许的模块依赖方向，不意味着每种配置都创建全部目标；具体 PUBLIC/PRIVATE 关系以各模块 CMake 声明为准。

| 模块 | 允许依赖的其他模块 |
| --- | --- |
| foundation | 无 |
| scene / assets / ecs / platform / telemetry | foundation |
| world | foundation、scene |
| simulation | foundation、scene、ecs、world |
| renderer | foundation、scene、assets、platform |
| app | 上述生产模块 |

### 稳定不变量

以下保留原构建契约的 B1-B6 编号；本页是其当前权威落点。这组编号与现役模块规格的 M3-T0-B1–B6 属于不同命名空间，不可仅凭数字相同互换。

- **B1：唯一实现所有者。** 每个启用的生产 `.cpp` 只编译进一个真实 owner；测试链接正式库，不重编译生产实现；生产不编译测试源码。未启用的历史 `.disabled` 文件不得进入活动目标。
- **B2：单向依赖。** 实际链接和源码包含遵守允许方向；模块不得反向依赖 app，生产不得依赖 test。第三方依赖也需显式授权，不能绕过模块边界。
- **B3：窄公开契约。** 公共头不暴露图形 SDK、SDL、YAML、噪声或私有存储实现；只允许审核过的私有桥接。需要头消费者编译核对自包含性，单靠文本扫描不是完整证明。
- **B4：产品与测试隔离。** game/tool 不包含 test/support；关闭 `BUILD_TESTING` 不注册测试或 fixture，安装不混入测试程序和开发中间产物。
- **B5：无依赖环与产品互隔离。** 生产 target 图无环；game 与独立 benchmark 不相互链接，也不包含对方实现。两者通过进程参数与版本化文件协议协作。
- **B6：CPU-only 图形隔离。** CPU-only 不创建 SDL、GLAD、platform、旧 GL 或原生图形后端。现代 renderer 的 CPU 门面和 app CPU 转接仍存在，不将“无图形库”误写为“无 renderer target”。

根配置末尾的[边界检查器](../../cmake/ModuleBoundaries.cmake)读取实际 source/link/include 属性、检查所有者及环并扫描源码，违规时具名配置失败。成功配置的 DOT、source/dependency TSV 和 renderer target 属性输出归属于本次构建目录；失败过程的全局属性或部分文件不自动回滚。

该检查器不是安全沙箱或完整 C++ 解析器，宏生成 include 被禁止；复杂语义依赖还需代码审阅与真实消费者构建。STATIC 私有依赖在链接接口中可能表现为 `LINK_ONLY`，这只满足最终链接，不等于传播依赖的公开头或编译选项。

## 图形与第三方边界

第三方头使用构建目录内的授权 `header-views/` 子集；数学、YAML、噪声、图像解码、PNG 写出、GLAD 和私有存储分别授权，不能发布整个 vendor 根。头视图复制失败使配置失败，已有副本不自动撤销。[受限头视图](../../cmake/ThirdPartyHeaders.cmake)与[游戏第三方配置](../../cmake/ThirdPartyGame.cmake)是实际来源。

- SDL3 只由 `symocraft_platform` 私有消费，SDL 头仅存在于 platform 实现中；app、renderer 和 CPU 模块不得直接消费 SDL target、include path 或相对 vendor 头。SDL 静态构建，关闭共享库、内部测试、例子与独立安装，保留现有 MSVC CRT 策略。
- platform 使用私有 `SDL_MAIN_HANDLED`；app 不需要引入 SDL 主入口头。SDL 窗口的 GL/Native/Vulkan 交接不代表生产 Vulkan renderer 已实现。
- renderer 到 platform 的私有通道只允许 `src/graphics_bridge/` 及 `graphics_bridge.h`、`native_bridge.h`、`vulkan_bridge.h`，不是整个 platform `src/`。桥接可以在授权私有层携带原生 SDK 类型，不能携带 SDL 头或把 SDK 类型泄漏到公共 API。
- platform 的 Vulkan 私有交接使用 SDL 附带的 Khronos 头和动态 loader，不因普通游戏构建强制外部 Vulkan SDK 或链接 `vulkan-1.lib`；显式 Vulkan 实验探针的 SDK 要求另行声明。
- GLFW 已退出活动目标。目标即使未链接，若重新配置 GLFW、包含其头或引入其库/搜索路径，也由检查器拒绝。仓库中的退休 vendor、休眠源和冻结包不是允许重新接入的依据。

依据：[platform 构建](../../game/modules/platform/CMakeLists.txt)、[renderer 构建](../../game/modules/renderer/CMakeLists.txt)。

### Renderer 的工程分层

`symocraft_renderer_api` 是只发布 CPU 公共头的 INTERFACE target；`symocraft_renderer` 是真实 CPU STATIC 门面，通过 PImpl 隐藏实现。`symocraft_renderer_backend_contract` 只发布 renderer 内部的窄接口，供门面与审核的后端私有消费，不对 app 或公开消费者开放。

D3D12 STATIC 后端只由现代门面私有链接；其原生库、SDK 头与平台定义不传播到 CPU API。`symocraft_renderer_legacy_opengl` 仅在 GAME 与 `SYMOCRAFT_RENDER_OPENGL` 同时启用时存在，是独立对照路径，不混入现代门面。关闭 OPENGL 同时去掉 app 的 GL 调用、GLAD target、SDL OpenGL 实现及 GLSL 部署，不改变运行参数的默认值。

app 的 `symocraft_startup` 与 `symocraft_app_render_data` 是 app 私有 CPU STATIC 库；后者承载 World 网格缓存、相机与图集转换，在 CPU-only 可测试，不把 Application 或窗口依赖引入 CPU 模块。资源寿命及运行错误契约见[Renderer](renderer.md)，不在此重复。

游戏启用时另有 `symocraft_app_runtime`，唯一拥有game_entry/application/game_renderer生产实现，SymoCraft薄main与审核的隔离probe链接同一库；CPU-only不创建它。测试不得再次编译这些生产源，生产库不依赖测试源码或私有SDK测试控制；后端诊断绑定由probe在创建线程显式安装。

## 配置组合

| 组合 | 当前工程契约 |
| --- | --- |
| 默认 | game、CPU 模块与独立 benchmark；现代 renderer CPU 门面、D3D12 与保留的旧 GL；游戏运行默认 D3D12 |
| `SYMOCRAFT_CPU_ONLY=ON` | 局部强制 GAME/BENCHMARK/OPENGL/D3D12/VULKAN OFF、CPU_MODULES ON；图形 SDK 不参与发现、配置或链接 |
| GAME OFF、CPU_MODULES ON | 仍组织 CPU 模块和 app CPU 库，不创建 platform 或游戏；benchmark 由自己的开关决定 |
| 根入口仅 benchmark | GAME 与 CPU_MODULES 显式 OFF、BENCHMARK ON；旧缓存中的 CPU_MODULES 不会因改 GAME 自动消失 |
| `cmake -S tools/benchmark` | 工具独立配置，复用仓库内 YAML 与工具链检查；不创建游戏或检查游戏资源，不表示子目录可以脱离仓库构建 |
| D3D12 ON | 要求 GAME ON；新配置默认取 GAME 状态，私有发现 SDK/DXC；候选身份记录仅在下述候选组合触发 |
| OPENGL OFF、D3D12 ON | 现代独占游戏，省略选择默认 D3D12；显式 OpenGL 具名 Unsupported 退出、不回退 |
| GAME ON、OPENGL/D3D12/VULKAN 均 OFF | 配置期具名失败；不建无实际后端的游戏 |
| `BUILD_TESTING=OFF` | 不注册测试和 fixture；符合候选组合时仍保留 Python 构建身份记录，普通游戏构建不由该步骤强制要求 Python |

`SYMOCRAFT_BUILD_CPU_MODULES` 与 `SYMOCRAFT_RENDER_D3D12` 的默认值在首次定义时取 GAME 状态，之后是独立缓存选项。标准Debug/Release游戏preset显式设置D3D12=ON，重新配置会更新旧OFF缓存；自定义GL-only或GAME OFF组合使用命令行显式OFF覆盖preset，不经preset的旧缓存仍需显式启用，根option不使用FORCE。CPU-only强制值仍是当前配置作用域的覆盖，不将用户缓存全部改写。未编入默认后端时省略运行选择会具名拒绝而不是偷偷回退。模式矩阵应以实际target/Ninja图和产物检查为依据，而非仅看开关文字。

CPU-only 仍进入 game 的 CPU 组织并加载资产清单及共享 staging，仅需要 YAML/PNG 两项公共源资产；GLSL 只随 OPENGL 启用加入。独立 benchmark 入口不加载游戏资产规则。无图形的 `--world-summary` 只预检公共资产，不选择后端或要求 shader。

### Vulkan候选构建

Vulkan工作目录候选由默认OFF的 `SYMOCRAFT_RENDER_VULKAN` 单独启用且要求GAME；CPU-only局部强制VULKAN OFF，GAME全关闭拒绝也覆盖三后端。启用才私有发现SDK头和独立 `SYMOCRAFT_VULKAN_DXC`，构建时检查SPIR-V能力，不复用无该能力的Windows SDK DXC；同一blocks/selection HLSL及常量include生成四个VS/PS SPIR-V并单独安装。固定Vulkan1.2 target、DX布局/列主矩阵和b/t/s绑定位移，Debug保留direction location、Release优化移除，私有顶点layout分别匹配。运行只动态解析SDL所用loader，安装包不需要DXC/开发头；干净机器证明与组合矩阵仍独立验收。真实target及flags见[Vulkan构建](../../cmake/RendererVulkan.cmake)，工作目录来源/结果见[T3验证](../openspec/changes/m3-t3-renderer/verification.md#2026-10-14-r4生产vulkan实现)，不改变D3D12默认、旧GL或其既有预算。

### D3D12 候选身份限制

- 启用后私有发现 `d3d12.h`、`dxgi1_6.h`、x64 D3D12/DXGI 库和实际 DXC 可执行文件，不接受 FXC；缺失使配置失败。
- 当前仅支持分离的 Debug/Release 单配置构建；DXC 对 blocks/selection 分别生成 VS/PS Shader Model 6.0 DXIL，公共 HLSL 常量头参与增量依赖。
- app POST_BUILD 仅在仓库 `out/m3-t3/` 子目录且 BENCHMARK OFF 时记录来源、工具、exe、DXIL 与构建文件身份；普通 preset 构建不触发候选专用记录。候选仍须当前源码根、Ninja、Debug/Release、GAME ON、CPU_ONLY OFF、BENCHMARK OFF及非重解析点子目录；触发后校验失败不能因 exe 已链接当成功候选。
- 打包要求同根下独立且不存在的输出目录；生产输入、工具或产物变化即拒绝使用旧身份。准备检查只容许既有 glob 检查与资产同步，不以暗中重建更新待验证候选。失败安装可能留下部分包，必须保留其失败身份，不推定完整或覆盖重试。

实际条件见[后端构建](../../cmake/RendererD3D12.cmake)、[app 构建](../../game/app/CMakeLists.txt)与[候选打包器](../../scripts/package-d3d12-candidate.py)。身份范围不是整个 SDK、驱动或操作系统快照。

Python 与 DXC 是构建/查验依赖，不是生产游戏或原生 benchmark 的运行依赖；携带 Python runner 的独立实验包另按其包合同处理。

## 测试及验证边界

根 `BUILD_TESTING` 注册包含 Python 3.10+ helper suites，因此 CPU-only 测试配置也需要 Python；这不是把 Python 加入生产 C++ 产品运行时。

公开头消费者逐个包含并链接所属公开目标，不添加私有路径、聚合头或预编译头补依赖；renderer 公共头消费者只链接 API target。生成消费者成功不等于编译、执行通过。

白盒路径只授予明确测试：world 私有状态适配器、renderer 门面/旧 GL 测试、app 启动与转换、benchmark 内部进程测试等。它们链接正式实现，不向生产传播私有路径。现有 GLAD 函数指针替身、recording backend 和故障 fixture 只证明相应 CPU 调用/清理契约，不替代真实驱动或人为输入。

生产 SDL 桥接探针导入同源码、同配置的正式静态库并核对身份，不重编译窗口实现。真实原生 D3D12 探针由显式入口执行，未注册为无人值守 CTest。独立头构建、负向边界、构建模式矩阵、增量探测、硬件运行与人工批准分别有结论，不互相推定。

构建助手指定自定义目录时，CTest 只使用明确的 `--test-dir` 和配置，不混入 preset；默认目录才使用 test preset。测试执行位置和 `Testing/` 日志位置必须一致，不能仅凭日志时间戳或测试退出码认定证据对应本轮。

T3的[后端配置矩阵](../../test/integration/renderer/verify-renderer-configurations.py)检查实际SDK缓存、构建/链接图、各后端资产及无GPU预检；[渲染增量检查](../../test/integration/renderer/CheckRendererIncrementalBuild.py)依据真实生产owner核对私有/公开头与同源shader重编范围，临时改mtime而保持内容并恢复时间戳。Vulkan配置期除DXC帮助外，还按实际SPIR-V目标/布局参数试编译小shader并核对模块头；帮助宣告不能替代可用CodeGen，未启用后端不执行此检查。配置成功、无窗口检查及构建隔离不证明GPU能力、缺loader运行或干净机器部署。

依据：[根测试注册](../../test/CMakeLists.txt)、[公开头消费者](../../cmake/PublicHeaderTests.cmake)、[集成测试](../../test/integration/CMakeLists.txt)、[模式矩阵](../../test/integration/VerifyBuildMatrix.cmake)、[增量检查](../../test/integration/CheckIncrementalBuild.py)。本轮文档迁移没有执行这些构建或运行入口。

## 资源与安装

- 共享 `symocraft_runtime_assets` 在每次构建检查并以 `copy_if_different` 同步两项公共资产，以及 OPENGL 启用时的四项 GLSL，多个消费者共用一份部署任务，避免并发复制。资产内容变化无需重链接 exe 才能部署；缺少源资产在加载 StageAssets 时即配置失败，复制失败由 build 反馈，不撤销已同步内容。更换后端组合使用新目录，不把旧目录残留误认作当前部署清单。
- 游戏及资源消费者输出到本配置的 `bin/`；运行资源根是 exe 旁的 `assets/`，不依赖当前工作目录或源码树。资产解析拒绝空路径、绝对/根路径、父目录穿越、流名及越根链接；移动运行包必须保留相邻资源。存在性检查不证明解码、配置语义、shader 或玩法正确。
- 游戏 target 编入 [windows-utf8.manifest](../../cmake/windows-utf8.manifest)，申请 UTF-8 activeCodePage，衔接执行器 UTF-16 进程参数与游戏中的窄字符资源路径；这是限定机制，不是任意 Unicode 路径、全部第三方加载链或目标机器均已验证的保证。源码编译的 `/utf-8` 与运行时 manifest 是不同约束。
- 安装仅选择产品、必要资源和许可通知，不安装测试/fixture、库/对象、PDB 或无用资源。D3D12 安装另含四份 DXIL；不会携带 DXC、开发 SDK、显卡驱动、Debug Layer 或 SDL DLL，目标机器仍需满足实际系统与驱动条件。
- Release 使用工具链对应的 app-local 可再分发 CRT。普通 Debug 不作为分发包；D3D12 Debug 候选明确安装匹配工具集的 Debug CRT 与 SDK debug UCRT，仅限本机开发诊断，不能对外再分发，缺少要求的运行库会配置失败。
- 安装或静态 DLL/import 检查不等于干净机器运行验收。环境裁剪仍在同一开发主机，不能宣称卸载 SDK 后或新机器通过；开发 staging 也不是最终发布认证。

资源来源：[资产清单](../../cmake/RequiredAssets.cmake)、[部署规则](../../cmake/StageAssets.cmake)、[运行定位](../../game/modules/assets/src/asset_paths.cpp)。独立 benchmark 的安装说明从 [工具 README](../../tools/benchmark/README.md) 与 [第三方盘点](../third-party.md) 取输入，包内输出名不变；文档输入的隔离安装回归不证明原生产品编译、完整通知或真实部署通过。
