---
tags:
  - area/legacy
---

# 第三方库依赖报告

## 当前报告范围与结论

- 更新日期：2026-10-08；保留 M3-T2 S5 的活动 GLFW 配置、授权与安装许可撤除，以及 S1/S2 的 SDL3 生产窗口、私有桥接、静态部署记录和 M3-T1 的 world/YAML 结论。本次重新核对活动构建声明、库引用、独立实验与退役决策，新增下方淘汰候选名单；没有重新核验上游来源、版本或二进制。以工作区工程文件为准，不仅依据 Git 提交。
- 核对范围：顶层、`game/`、`tools/benchmark/`、`test/` 的 CMake 配置，`cmake/ThirdParty*.cmake`、`cmake/ModuleBoundaries.cmake`，当前参与构建的源码、`vendor/` 本地材料及 `scripts/benchmark.ps1`。
- 原静态盘点不代表所有库的上游来源或运行验证。SDL3 的官方发行身份及阶段实际构建/运行结果另见 [T2 记录](../milestones/m3-t2/README.md)；不将该增量推广为其它库已重审、完整硬件输入或 T2 已验收。Y9000P 验证延期，既有冻结包不因本次安装规则改变而更新。
- 当前游戏使用的第三方项目为 **SDL3、GLM、glad、stb、FastNoiseLite、robin_hood、yaml-cpp**；S2 撤除 platform 的 GLFW 链接，S5 进一步撤除 `EXCLUDE_FROM_ALL` 配置、platform 外部授权和新安装包 GLFW 许可规则。stb 分别以图片解码和图片写出两个组件接入。Khronos 平台头是 glad 的配套传递依赖。
- **irrKlang、`vendor/glfw`、`lib/glfw`、`lib/yaml-cpp` 下的 Google Test/Mock 二进制以及 `vendor/stb-master` 不参与当前生产构建**。它们在仓库中存在，不代表游戏需要它们。
- Windows SDK、MSVC/UCRT、显卡 OpenGL 驱动属于系统/工具链依赖；系统 `tar.exe` 和可选 `nvidia-smi` 属于外部程序依赖，与随仓库编译的第三方库分开记录。

本文所说“需要”，指当前实现用该库完成什么工作，以及移除后需要补上的能力，并不表示该功能只能由此库实现。性能方面的选型解释是用途分析，不是本次已经测得的性能收益。原 M0、M1、M2-T3 审计材料保留在文末历史附录，不能用其中旧路径或旧构建关系解释当前工程。

## 第三方库依赖淘汰候选名单

### 核对口径与优先级

本名单依据 2026-10-08 当前工作区的源码、CMake、测试/实验和已有阶段决策，属于**候选评估，不是删除或替换授权**。本次只修改本文，没有移除库、改写实现、执行构建或复跑阶段验收。未使用判断针对活动构建；文档、边界负向测试、停用源码及历史实验出现库名，不等于生产仍在使用，也不能反过来把历史材料当作没有保留价值。

淘汰分两种：**活动依赖退出**是停止编译、包含、链接和部署；**仓库材料清理**是删除或归档原始文件。两者的收益不同：清理已经停用的库主要减少材料混淆和仓库维护负担，不会再次减少当前可执行文件的依赖或证明 FPS 提升。

| 优先级 | 含义 | 当前判断 |
| --- | --- | --- |
| A：闲置材料整理 | 生产已不用，先做身份清单和归档/保留决策 | irrKlang、历史 Google Test/Mock 二进制、stb-master；GLFW 有既定保留限制，不可立即删除 |
| B：活动库优先替换 | 用途集中，可在保持行为的前提下退出一个项目依赖 | robin_hood 是首选；仍需单独实施和验证 |
| C：有前置条件的后续退出 | 与后端迁移或格式/公共接口决策绑定，不适合顺手删除 | glad + 根目录 Khronos 配套头；yaml-cpp 仅列为长期评估候选 |
| D：暂保留 | 核心能力仍在使用，当前没有已验证等价替代，删除收益不足以支撑风险 | SDL3、GLM、FastNoiseLite、stb_image、stb_image_write |

### A 类：已退出生产的材料候选

| 编号 / 候选 | 当前依赖事实与模块影响 | 入选理由 / 预期收益 | 处理前提与验收 |
| --- | --- | --- | --- |
| A01 irrKlang：`vendor/irrKlang`、`lib/irrKlang` | 活动 game/tools/test 源码和构建未接入；旧聚合头仅在 `.disabled` 中保留引用；没有现行音频模块 | 不用的音频头、导入库和 DLL 容易被误认为发布必需。可列入历史音频材料归档候选；不需要为当前玩法引入替代音频库 | 先确认后续音频路线及历史审计保留范围，记录原始文件身份；未启用音频不是未来永久不用的证明。清理后干净 Debug/Release、安装及 CPU/工具配置应不变；不改写旧包或旧审计事实 |
| A02 历史 Google Test/Mock `.a`：`lib/yaml-cpp/libgtest.a`、`libgtest_main.a`、`libgmock.a`、`libgmock_main.a` | `test/CMakeLists.txt` 注册自有测试可执行程序；未链接这些库。活动 yaml-cpp 来自 `vendor/yaml-cpp/src`，并非这个 lib 目录 | 来源/工具链不明且目录名容易误导；四份预编译测试材料可优先列入归档候选 | 只针对这四份历史测试二进制，不删除 `vendor/yaml-cpp`。记录身份和归档位置，完整 CTest 及独立 runner 测试保持通过；未来采用测试框架应重新明确来源与同工具链构建 |
| A03 重复/闲置 stb 目录：`vendor/stb-master` | 当前头视图只引用 `vendor/stb/stb_image.h` 和 `stb_image_write.h`；D3D12 R1 实验也使用 `vendor/stb` | 第二份 stb 目录没有活动消费者，可减少重复材料及改错目录的风险 | 先逐文件核对独有内容、工具和许可证并留身份；旧审计只证明一份 `stb_image.h` 相同，不能直接断言整个目录重复。保留 active `vendor/stb` 与许可，验证图片解码、截图和独立实验 |
| A04 GLFW：`vendor/glfw`、`lib/glfw` | 生产配置/链接/许可安装已撤除；但 `test/experimental/glfw-baseline/CMakeLists.txt` 仍显式导入冻结构建的 GLFW 静态库，输入对照脚本与历史证据仍使用基线 | 活动依赖淘汰已落实，不再是待换的生产窗口库；仅可作为未来历史材料审计候选 | **现阶段保留，不物理删除**。[T2 D11](../spec/M3-T2-SDL3迁移.md) 要求保留休眠旧代码、vendor/lib 和历史证据供 T5 审计。归档须另行确认，并保持冻结构建身份、基线重现及旧许可可追溯；最终 exe 不导入 GLFW DLL 不能替代构建图、包含路径、cache 和安装检查 |

A 类的“低风险”仅指对当前生产实现的直接影响小，不代表可以绕过历史复现、来源和许可保留要求。不要为减少目录数量而修改官方 SDL3 原样源码，或把 SDL3 自带的第三方内部材料按“游戏没有直接 include”批量删掉。

### B 类：robin_hood，优先考虑的活动库退出

**结论：优先级最高的活动库候选，改动面集中，但尚未替换。**

- 当前唯一活动生产调用位于 [shader_program.cpp](../../game/modules/renderer/src/shader_program.cpp)：`robin_hood::unordered_set<ShaderVariable, HashShaderVar>` 缓存 `(shaderProgramId, name)` 对应的 uniform location。world 在 T1 已改为固定槽/排序 vector，其 CMake 和当前 world 测试均不再链接 robin_hood。
- 候选替代路径是保持现有 key、hash、相等比较、`find`、`emplace`、`clear` 语义，使用标准库 `std::unordered_set`，不新增第三方项目。现有代码没有借出该缓存元素地址供长期持有，搜索范围内也没有需要保留 robin_hood 专属接口的其他生产调用。这里是基于当前调用面的可行性判断，不是已实施或性能等价证明。
- 实施涉及 renderer 源文件与私有链接、`cmake/ThirdPartyGame.cmake` 的 `symocraft_robin_hood` 头目标及 `cmake/ModuleBoundaries.cmake` 的 renderer 授权；必须一起消除残留活动接入，不能只换 include 或删 vendor 文件。库文件的物理归档另按历史材料政策处理。
- 最小验收应补上缓存命中不重复查询、相同变量名在不同程序中隔离、`-1` location 仍正确缓存、程序销毁/重建后清缓存的回归测试，并运行 renderer 单测与真实游戏/短采样。当前 `shader_loading_tests` 主要检查编译/链接与失败清理，不能代替这些缓存专用检查。
- 预期收益是退出一项私有第三方容器依赖及其来源/通知维护责任；实际 CPU、内存和帧时间变化待同条件测量，不承诺标准容器更快。若即将整体替换该 GL shader 实现，可与该变更合并，避免为短期旧路径引入重复改动。

### C 类：条件成熟后再评估

| 编号 / 候选 | 当前仍需要的功能 | 为什么列入 / 候选路径 | 淘汰前置条件与影响 |
| --- | --- | --- | --- |
| C01 glad + `vendor/KHR/khrplatform.h` | 生产 renderer 的绘制、shader、纹理、缓冲、GPU timer、截图读回及 NVX 查询仍调用 OpenGL；renderer 白盒测试与若干显式 GL 实验仍使用 glad | 属于 GL 专属接入，未来 GL 活动路径完全退役时可联动退出；不是更换另一个 GL loader 的建议 | [T3 正式计划](../spec/M3-T3-渲染器重构.md) 把最终 GL 退役放在 T3 后另立的 R5 节点。当前 D3D12 R1 仅为独立实验，不是生产接管或双现代后端验收。须先完成既定 D3D12/Vulkan 玩法、计时/截图/发布及基准协议验证，再在另行授权节点退出 GL target、调用、资产和活动测试；历史 GL 对照仍需保留 |
| C02 yaml-cpp | world 的 `BlockDefinition::FromConfig`、telemetry 文档转换、benchmark_core 会话/状态/结果、测试与 SDL 交付检查仍使用；runner 公共头公开 `YAML::Node` | 本地版本/来源/完整许可待补，且 T1 有本地 stream 修补，值得长期减依赖评估。优先补来源与隔离 runner 的解析器类型；若未来批准格式迁移，再评估解析实现替换 | **不列入近期删除**。world / telemetry 虽已隔离公共 YAML 类型，runner 尚未隔离；只把 CMake 改成 `PRIVATE` 不能解决。全局退出须为全部消费者提供等价解析/输出或正式协议迁移，覆盖 YAML 标量、数字样字符串、错误处理、配置规则及旧结果读取；仅重写方块配置解析不能淘汰整个库 |

C01 中的 Khronos 候选只指根目录 glad 所需的 `vendor/KHR` 配套头，不包括 SDL3 内部 `src/video/khronos`，也不包括未来 Vulkan 所需的 SDK/类型。仅停用 OpenGL 的部分配置或替换一份头，不等于这些材料都能一起删除。SDL 的 OpenGL/Vulkan 窗口能力开关也不能仅因本报告而调整。

C02 的来源/许可缺口是补证据事项，不自动等于必须淘汰；没有计划在本次引入另一份 YAML 库、自写通用 YAML parser 或悄悄改成另一种文件格式。替换实现仍可能有新的许可和维护责任，不能把减少一个 vendor 目录当作已降低全部成本。

### D 类：当前不建议淘汰

| 依赖 | 保留原因与不应采用的淘汰理由 |
| --- | --- |
| SDL3 | 正在承担生产窗口/输入与 GL、Native、Vulkan 私有桥接；现有正式决定选择 SDL3。原生 benchmark GUI 不用 SDL，不代表游戏窗口库可删。T2 验收未关闭也不是重新选择窗口库的依据 |
| GLM | foundation/scene/world/simulation/renderer 公共数学类型和顶点布局跨模块使用；现代后端实验也直接使用 GLM。换渲染 API 不会自动消除数学依赖，当前应补齐来源/许可而非发起广泛数学重写 |
| FastNoiseLite | world 仍使用三层种子噪声；改变采样实现会改变固定种子的世界与摘要。没有等价生成方案和基线验证，不应为减少一个头文件项目而改变世界契约 |
| stb_image | assets 仍需解码纹理图集，公共 `Assets::Image` 已隔离实现。当前没有接入等价解码器；图形后端重构不替代 CPU 图片解码 |
| stb_image_write | 生产 `final-frame.png` 和 D3D12 R1 的诊断 PNG 均在使用；截图也是 T3 验收证据。换掉 OpenGL 读回方式不等于不再需要 PNG 编码。只删除该组件也不会淘汰整个 stb 项目，因为读取组件仍使用 |

Psapi、Win32、MSVC/UCRT、显卡驱动及未来现代图形 SDK 不按“闲置第三方库”处理。`tar.exe` 是 runner 归档功能依赖，`nvidia-smi` 已可选；本名单不把取消功能、缺省关闭传感器或删除测试当作成功淘汰库。

### 建议顺序与完成标准

1. 先形成 A01–A03 的材料身份及归档方案，A04 GLFW 按 D11 继续保留；没有物理删除授权前仅维护名单和隔离关系。
2. 如批准单独做活动依赖精简，优先实施 B 类 robin_hood 替换及缓存回归，不与仍在进行的渲染器重构修改互相覆盖。
3. C01 按既定现代后端接管与另立 R5 退役门槛推进，C02 先补来源/许可和公共接口边界；不从本名单推导新的阶段验收或实施授权。
4. 每项真正退出时记录原/新文件身份、行为差异、干净 Debug/Release 构建、完整适用测试、game-only/CPU-only/runner-only/独立 runner 配置及安装证据；glad、stb 等还须检查显式实验入口。历史包、基线与许可不改写。

本次确定的是候选及退出门槛，尚无新的“淘汰已完成”项目或实测收益。现有 GLFW 退役状态仅依据当前构建声明与既有 T2 记录，不将本次静态核对冒充重新构建验证。

## 模块与依赖对应表

“直接”指模块直接声明第三方目标并使用其接口；“间接”指通过内部模块获得能力或传递构建要求。`PUBLIC` 表示使用要求向下游传播，`PRIVATE` 表示不作为模块公共编译接口公开。静态库的 `PRIVATE` 链接依赖仍可能需要进入最终可执行文件的链接闭包，不等于运行时完全不存在。

| 功能模块 / 工程目标 | 直接第三方库与接入方式 | 间接关系 | 为什么需要 / 功能用途 |
| --- | --- | --- | --- |
| 基础设施 `foundation` / `symocraft_foundation` | GLM：`PUBLIC symocraft_math` | 使用 foundation 的模块继承 GLM 编译使用要求 | 统一向量、矩阵、四元数及数学扩展；`math.h` 是当前公共数学入口。基础文档值类型 `Data::Value` 自身不依赖 YAML |
| 场景数据 `scene` / `symocraft_scene` | 无单独声明的第三方目标 | 经 foundation 使用 GLM | 相机投影/视图矩阵、网格顶点位置和纹理坐标需要统一数学类型；scene 是 `INTERFACE` 目标，不持有 OpenGL 对象 |
| 资源 `assets` / `symocraft_assets` | stb_image：`PRIVATE symocraft_image_decoder` | CMake 经 foundation 继承 GLM，但图片解码接口本身不需要 GLM | 将 PNG 等图片文件解码为 CPU 像素，供纹理上传；读取与定位资产不需要图形上下文 |
| 实体组件 `ecs` / `symocraft_ecs` | 无 | CMake 经 foundation 继承 GLM；Registry 本身没有直接调用 GLM | ECS 存储、实体版本和组件查询由工程自有代码实现，不依赖第三方 ECS 库；数学组件属于 simulation |
| 世界 `world` / `symocraft_world` | yaml-cpp、FastNoiseLite：均为 `PRIVATE` | 经 foundation / scene 使用 GLM | 配置文本解析与地形噪声；T1 的固定二维槽/排序规则 vector 不再使用 robin_hood |
| 模拟与玩家 `simulation` / `symocraft_simulation` | 无 | 经 foundation / scene 使用 GLM；经 world 使用已有配置与世界存储能力 | 玩家移动、相机、变换、速度、碰撞和射线运算使用 GLM；物理逻辑是自有实现，没有接入第三方物理引擎 |
| 平台 `platform` / `symocraft_platform` | SDL3：`PRIVATE SDL3::SDL3-static` | CMake 经 foundation 继承 GLM；Win32 / Psapi 另见系统依赖 | SDL 窗口/context/键鼠与三模式私有桥接；编译使用要求不向 app/renderer/CPU 传播，公共类型不暴露 SDK |
| 渲染 `renderer` / `symocraft_renderer` | glad、stb_image_write、robin_hood：均为 `PRIVATE` | 经 scene / foundation 使用 GLM；经 assets 解码图片；经 platform 获取图形入口和上下文；glad 带入 Khronos 头 | 分别完成 OpenGL 函数加载与调用、诊断 PNG 截图、uniform 位置缓存；GLM 用于变换与着色器参数 |
| 性能观测 `telemetry` / `symocraft_telemetry` | yaml-cpp：`PRIVATE symocraft_yaml` | 经 foundation 使用工程自有 `Data::Value`；构建上也继承 GLM | 将观测值序列化为 YAML，读取协议文档。CPU/GPU 原始采样由 platform / renderer 提供，并非 telemetry 直接依赖 SDL3 或 glad |
| 游戏组装 `app` / `SymoCraft` | 无直接第三方目标 | 链接上述内部模块，汇集游戏所需第三方实现 | 组织启动、世界、玩家、渲染和观测生命周期；调用模块接口，不再通过旧 `core.h` 一次性引入所有库 |
| 启动参数 `symocraft_startup` | 无 | 无第三方库目标 | 参数解析使用标准库，CPU-only 时仍可构建，不需要窗口、YAML 或数学库 |
| 独立基准核心 `tools/benchmark` / `benchmark_core` | yaml-cpp：`PUBLIC symocraft_yaml` | `SymoCraftBenchmark` 和基准测试继承 YAML 接口；不链接游戏模块 | 读写会话、状态、结果、校验清单，并校验游戏输出协议。公共 `runner.h` 直接使用 `YAML::Node`，因此这里与游戏模块不同，YAML 是公共依赖 |
| 基准 GUI `SymoCraftBenchmark` | 无额外第三方源码库 | 经 benchmark_core 使用 yaml-cpp；Windows GUI 系统库另列 | 窗口、文件选择和进度控件使用 Win32，不使用 GLFW；运行被测游戏是子进程交互，不是链接游戏渲染库 |
| 测试 `test/` | 部分默认单测直接使用 yaml-cpp、glad；显式实验另有 SDL3、GLM、stb 等依赖 | 其余依赖被测模块继承；world 测试不再直接链接 robin_hood | 用于生成/检查 YAML 测试数据、替换 OpenGL 函数指针和检查模块契约；GLFW 基线仅在显式历史实验中导入；当前使用自有测试可执行程序 + CTest，没有接入 Google Test/Mock |

证据入口：各模块的 `game/modules/<模块>/CMakeLists.txt`、`game/app/CMakeLists.txt`、`tools/benchmark/CMakeLists.txt` 和 `test/unit/*/CMakeLists.txt`。**传递获得某库的包含路径不等于模块确实调用该库**，因此 ECS、assets、platform、telemetry 的 GLM 传递关系不应被误写为各自核心功能必须使用 GLM。

## 各库用途与必要性

### SDL3：生产窗口与私有桥接

- 固定官方 `release-3.4.18`、commit `829a65d769d935c4852f8159e964312c0957260a`，`vendor/sdl3` 原样源码；ZIP 和逐文件核对见 [来源证据](../milestones/m3-t2/evidence/sdl-provenance.json)。没有修改上游文件。
- 只在游戏图形配置中创建 `SDL3::SDL3-static`，platform 私有链接、私有 `SDL_MAIN_HANDLED`；保留 MSVC `/MDd` / `/MD` CRT 策略。窗口/输入、桌面 OpenGL 和动态 Vulkan 窗口桥接启用；无关子系统、共享库、SDL 自测/示例/独立安装关闭。
- SDL 自带 Khronos 内部头和动态 loader 桥接不引入完整 Vulkan SDK 或 `vulkan-1.lib`；外部 SDK 仅用于显式 Vulkan 探针。CPU-only、根 runner-only 与独立 benchmark 不配置或编译 SDL。
- platform 私有实现调用 `SDL_SetMainReady`、持有一次 video 初始化责任；显式 GL context 和窗口分别释放。GL 六项入口检查必要失败，私有 NativeBridge 借出 HWND、VulkanBridge 借出 loader 并复制扩展名；真实三模式探针不代表现代 GPU 游戏后端。
- 使用 `vendor/sdl3/LICENSE.txt` 的 zlib 许可，游戏安装复制为 `licenses/SDL3-LICENSE.txt`，不增加 `SDL3.dll`。S2 双配置最终游戏链接不含 GLFW；S5 已撤除旧活动配置和新安装包 GLFW 许可规则，历史包保持原样。生产使用原样 vendor 的官方 Khronos 头，外部 SDK 仅用于显式探针。

### GLFW：已退役的历史窗口基线

- S2 前的 platform 用 GLFW 创建窗口、查询键鼠、等待与管理 GL 上下文；现由 SDL3 私有实现替换，renderer 的 GL 桥接签名保留，未把 SDL 类型转为公开依赖。
- GLFW 不负责绘制，也不能替代 glad；旧输入/窗口手感及包保留为历史对照，不能用旧包结果宣称当前 SDL 游戏通过。
- `cmake/ThirdPartyGame.cmake` 不再配置 `vendor/glfw`，platform 不再授权或链接 GLFW；新游戏安装包不再复制 `GLFW-LICENSE.md`。`vendor/glfw`、`lib/glfw`、休眠源以及历史/冻结包中的原许可按 D11 继续保留，不删除或改写。
- 构建边界拒绝活动 GLFW 目标（包括未链接的 `EXCLUDE_FROM_ALL` 目标及 vendor 内改名目标）、生产 GLFW 头/相对 vendor 包含、原始或表达式链接、vendor/lib 包含路径。实际构建矩阵核对所有模式的 target/cache/build/install，不能仅用“最终 exe 没有 GLFW DLL”证明撤除完整。

### GLM：公共数学类型与运算

- `game/modules/foundation/include/symocraft/foundation/math.h` 引入向量、矩阵、哈希、旋转、四元数等头，并统一当前 GLM 宏设置。CMake 通过 `symocraft_math` 暴露头文件，`glm::glm` 是其别名，不是另一份数学库。
- scene 的 `camera.h`、`mesh.h` 使用 `glm::mat4`、`glm::ivec3`、`glm::vec3`；world 使用整数坐标及网格数学类型，T1 已不再使用旧区块哈希表；simulation 使用向量运算、相机矩阵、移动/碰撞计算；renderer 使用矩阵和着色器参数。
- 需要它是为了保持世界、模拟和渲染之间一致的数学表达，避免分别实现并维护线性代数。当前公共接口直接出现 `glm::*`，替换会影响多个模块及数据布局，不只是修改一个包含目录。
- scene 对顶点尺寸存在 `static_assert`（例如 `BlockVertex3D` 为 28 字节）。升级 GLM、改变对齐或宏设置时应复核 CPU/GPU 顶点布局；头文件库没有独立 DLL，不代表没有布局兼容风险。

### glad 与 Khronos 平台头：OpenGL API 接入

- renderer 的 `renderer.cpp` 使用 `gladLoadGLLoader`，函数地址来自 platform 的图形桥接。`shader.cpp`、`shader_program.cpp`、`texture.cpp`、`batch.hpp` 调用着色器、纹理、缓冲和绘制 API。
- `gpu_timer.cpp` 使用 `GL_TIME_ELAPSED` 查询 GPU 绘制时间；`performance_memory.cpp` 读回帧缓冲，并按设备能力查询 NVX 显存信息。NVX 是驱动扩展能力，不是新增随包部署的库，不支持时不能当作已有读数。
- 需要 glad 是因为当前渲染器依赖运行时可调用的现代 OpenGL 函数入口；仅包含头文件或建立窗口还不够。需要 Khronos 头是因为 glad 使用其跨平台基础类型定义；它不是独立功能模块或单独运行时库。
- `symocraft_glad` 从 `vendor/glad/glad.c` 编译为静态库，公共头视图包含 `glad/glad.h` 和 `KHR/khrplatform.h`。生成配置是 OpenGL 4.6 core；窗口也请求 4.6 core。生成文件存在并不保证目标显卡驱动满足要求。

### stb_image：资源图片解码

- 唯一当前生产实现宏位于 `game/modules/assets/src/image.cpp`；`stbi_load_from_memory` 把文件字节转换为像素，管理翻转、错误说明及解码内存释放。公共接口返回工程自有 `Assets::Image`，不暴露 stb 类型。
- renderer 的 `texture.cpp` 调用 `Assets::DecodeImage`，然后检查尺寸/通道并上传纹理。因此“渲染依赖图片解码”成立，但 stb_image 的直接所有者是 assets，不再是 application 或 renderer。
- 需要它是为了读取当前 PNG 纹理图集等图片资源。移除时必须替换解码器；单纯读取文件无法得到供 GPU 使用的像素。实现被编译进 assets，不需要另带 stb DLL。

### stb_image_write：诊断截图写出

- 实现宏位于 `game/modules/renderer/src/performance_memory.cpp`。`CaptureFramebuffer` 先调用 OpenGL 读回像素，再用 `stbi_write_png` 保存 `final-frame.png`。
- 需要它是为了为基准采样留下可检查的最终画面证据，不负责纹理读取。`application.cpp` 在测量结束后调用截图接口；当前截图功能随 renderer 一起编译，尚未做独立可选构建开关。
- `symocraft_image_writer` 只暴露该头，`symocraft_stb_headers` 是此目标的兼容别名，不能理解为整个 stb 目录均被公开或使用。删除写图能力主要影响截图和交付证据，不等于地形生成需要该库。

### FastNoiseLite：确定性地形噪声

- 直接归属 world，证据为 `game/modules/world/src/generation.cpp`。`Generator::NoiseState` 持有三个噪声采样器，设置种子、OpenSimplex2、FBm、octaves、频率等参数，再将混合噪声映射为地形高度。
- 需要它是为了获得连续、分层、可由种子复现的地形高度场，而不是无关联的随机高度。植被随机分布还使用标准库 `std::mt19937`，不能把整个随机系统都归因于 FastNoiseLite。
- 这是头文件库，由 `symocraft_noise` 私有接入；公共 generation 接口用不完整类型隔离采样器。更换算法或版本会影响固定种子世界及摘要，必须同步验证生成测试和基准场景，而不能视为无行为变化的替换。

### robin_hood：着色器缓存

- T0 world 的规则/区块哈希容器已在 T1 退出，旧实现见 [T0 报告](../milestones/m3-t0/README.md)；当前 world 无该依赖。
- renderer 的 `shader_program.cpp` 使用 `unordered_set` 缓存程序和变量名对应的 uniform 位置，避免重复查询 `glGetUniformLocation`。
- 需要的是哈希索引/缓存能力；当前选择 robin_hood 是其面向时间和内存效率的容器实现。没有本次与标准容器的对比数据，不能声称已证明比 `std::unordered_map` 更快。
- 当前 active platform 输入路径不使用 robin_hood；旧输入映射只存在于停用遗留文件，不计入当前依赖。

### yaml-cpp：配置、性能协议与独立基准会话

- world 的 `block.cpp` 用 `YAML::Load` 解析 app 传入的配置文本，生成自有 BlockDefinition；路径/读取归 assets/app。必需名称、ID 与纹理校验归项目，YAML 类型不进入 world 公开头。
- telemetry 的 `document_io.cpp` 在工程自有 `Data::Value` 与 `YAML::Node` 间转换，提供 `DumpYaml`、`LoadYaml`、`LoadYamlFile`。`performance.cpp` 使用该接口生成 `status.yaml`、`summary.yaml`。foundation 的值容器、world 生成报告值及 app 的业务组装不直接公开 YAML 类型。
- benchmark_core 的 `runner.cpp`、`platform.cpp` 则直接使用 `YAML::Node`、`YAML::Load`、`YAML::Dump`，处理会话、轮次、状态校验、结果及 SHA256 清单；其公共头 `tools/benchmark/include/benchmark/runner.h` 也暴露 YAML 类型。
- 需要它是因为当前资源和进程间文件协议使用 YAML。去除会同时影响方块配置读取、游戏观测输出和独立 runner 的读取/校验；必须保留等价解析与序列化行为或迁移格式，不能只改一个模块。
- `cmake/ThirdPartyYaml.cmake` 明确列出源码，将本地源码子集编译为 `symocraft_yaml` 静态库，头通过 `symocraft_yaml_headers` 暴露。`lib/yaml-cpp` 里的 `.a` 文件不是当前 yaml-cpp 实现。
- T1 本地修补只涉及 `vendor/yaml-cpp/src/stream.h`、`stream.cpp`：prefetch 数组由 `unique_ptr<unsigned char[]>` 接管。失败/复测与哈希统一见 [T1 报告](../milestones/m3-t1/README.md)，未升级库版本或补称已核验上游来源。

## 构建模式与依赖边界

| 构建入口 / 模式 | 需要的第三方范围 | 不应混入的依赖 |
| --- | --- | --- |
| 默认游戏 + runner | 游戏使用上述全部项目；runner 单独使用 yaml-cpp | irrKlang、历史预编译 `.a`、stb-master |
| 顶层 game-only：`SYMOCRAFT_BUILD_GAME=ON`、`SYMOCRAFT_BUILD_BENCHMARK=OFF` | 游戏模块依赖保留 | benchmark_core 及 runner GUI 不进入目标图 |
| 顶层 runner-only：`SYMOCRAFT_BUILD_GAME=OFF`、`SYMOCRAFT_BUILD_BENCHMARK=ON`、`SYMOCRAFT_BUILD_CPU_MODULES=OFF` | yaml-cpp + runner 系统库 | 游戏模块、GLFW、SDL3、glad、GLM、stb、FastNoiseLite、robin_hood |
| `tools/benchmark` 独立配置 | yaml-cpp + runner 系统库 | 不经过 `game/CMakeLists.txt`，不需要游戏图形或数学依赖 |
| `SYMOCRAFT_CPU_ONLY=ON` | 构建 foundation、scene、assets、ecs、world、simulation、telemetry、startup；实际使用 GLM、stb_image、yaml-cpp、FastNoiseLite，不再使用 robin_hood 实现 | 强制关闭游戏可执行程序和 runner，不创建 platform / renderer；不构建 GLFW、SDL3、glad 或 stb_image_write 实现 |

注意：`SYMOCRAFT_BUILD_CPU_MODULES` 默认值取配置时的 `SYMOCRAFT_BUILD_GAME`，已有 CMake cache 的值不会自动随另一开关改变，因此严格 runner-only 应显式关闭 CPU modules。CPU-only 中 `ThirdPartyGame.cmake` 仍声明若干头文件 `INTERFACE` 目标，但头目标存在不等于已编译/链接图形实现。顶层始终包含 `ThirdPartyYaml.cmake`，所以“游戏之外不需要 YAML”的判断也不成立。

`cmake/ThirdPartyHeaders.cmake` 将每项依赖的选定头复制到构建目录 `header-views/<target>`，以目标级 `SYSTEM INTERFACE` 包含路径暴露，不公开整个 vendor 根目录或第三方 `.cpp`。`cmake/ModuleBoundaries.cmake` 检查模块引用、目标依赖及公共头边界；GLM 是允许公开的数学例外，glad、YAML、stb、噪声和容器头不得进入游戏模块公共头，已退役 GLFW 不得进入任何生产源码或目标。独立 runner 不套用这一公共 YAML 隔离方式。

CPU-only 仅隔离图形/窗口依赖，**不表示已支持非 Windows 平台**：顶层仍要求 Windows x64 + MSVC，foundation / assets 的实现仍包含 Windows API。

## 系统、工具链与外部程序依赖

| 依赖类别 | 所属功能与证据 | 为什么需要 / 限制 |
| --- | --- | --- |
| Windows SDK / Win32 | foundation 的 `files.cpp` 使用 `MoveFileExW` 发布文件，`legacy/AmoBase.cpp` 保留 Windows 调用；assets 的 `asset_paths.cpp` 使用 `GetModuleFileNameW`；platform 窗口也有 Win32 处理 | 为文件发布、可执行文件定位和 Windows 窗口行为提供系统接口，不是另一个 vendor 库 |
| Psapi | `game/modules/platform/CMakeLists.txt` 私有链接 `Psapi`；`process_memory.cpp` 使用 `GetProcessMemoryInfo` | 采集当前进程 working set / private bytes，供 app 交给 telemetry；不等于 GPU 显存或温度采集 |
| OpenGL 驱动 / 系统图形组件 | SDL platform 创建并核实实际 4.6 Core/4x MSAA 上下文，renderer 加载 API；Windows 构建使用系统窗口/图形组件 | 真实绘制依赖设备驱动支持；glad 是加载器，不提供 OpenGL 实现。GPU 时间查询和 NVX 内存读数须按能力检查 |
| Windows 基准核心系统库 | `benchmark_core` 私有链接 `bcrypt`、`advapi32`、`powrprof`、`user32`；`tools/benchmark/src/platform.cpp` 使用 BCrypt SHA256、注册表、电源方案、窗口关闭消息以及 Win32 进程/Job API | 计算包和结果校验值、记录机器信息、管理被测进程；无需 OpenSSL 或游戏平台模块 |
| Windows 基准 GUI 系统库 | `SymoCraftBenchmark` 私有链接 `comctl32`、`comdlg32`、`shell32`、`ole32`；`gui.cpp` 调用公共控件、文件/目录选择、Shell 打开与 COM | 实现原生基准工具窗口，不需要另引 GUI 第三方框架 |
| Windows 系统 `tar.exe` | `tools/benchmark/src/runner.cpp` 的导出逻辑定位系统目录下 `tar.exe`，启动子进程生成 ZIP | 仅结果归档功能需要；不随仓库存放，不是当前接入的压缩库。缺失/失败时导出报错并保留已生成目录，不能将 runner 的导出能力描述为完全无外部程序依赖 |
| 可选 `nvidia-smi` | `scripts/benchmark.ps1` 通过 `Get-Command` 查找并采集 GPU 传感器，可用 `SkipGpuSensors` 跳过 | 仅脚本额外观测使用，不是游戏或原生 runner 的链接依赖，也不随包部署；设备级读数不是进程 VRAM，缺失时记录 missing |
| PowerShell / CMake / CTest / 构建工具 | 脚本观测与脚本测试使用 PowerShell；工程由 CMake 配置、CTest 注册测试；顶层要求 MSVC x64，C++ 目标使用 C++20 | 属于开发/脚本工具依赖，不应归为游戏第三方运行库；`SYMOCRAFT_CPU_ONLY` 不解除工具链约束 |
| MSVC / UCRT 运行库 | 顶层与独立 runner 的 Release 配置调用 `InstallRequiredSystemLibraries` | 发布产物仍要满足 C/C++ 运行库需求。当前配置没有重新做二进制导入检查；不得将“第三方库静态集成”等同于“程序没有 DLL 依赖”，Debug 运行库不作为发布材料 |

## 版本、来源与许可材料

| 当前依赖 | 本地版本 / 来源证据 | 本地许可与发布观察 |
| --- | --- | --- |
| GLFW（历史材料） | `vendor/glfw/CMakeLists.txt`：3.3.7；本地 README 记载项目来源 | `vendor/glfw/LICENSE.md` 完整且保留；S5 新安装规则不再复制，历史/冻结包原许可不改 |
| SDL3 | 官方 3.4.18 / commit `829a65d769d935c4852f8159e964312c0957260a`；原样来源与 ZIP 摘要见 T2 证据 | `vendor/sdl3/LICENSE.txt` 完整；游戏安装为 `licenses/SDL3-LICENSE.txt`，独立 runner 不安装 |
| GLM | `vendor/glm/detail/setup.hpp`：0.9.9.8；`glm.hpp` 记载项目网站与 GitHub 来源 | 未在本地 GLM 目录发现完整独立许可材料；当前安装规则未单列通知 |
| glad | `vendor/glad/glad.h`：glad 0.1.35 于 2022-06-05 生成，OpenGL 4.6 core，`Reproducible: False` | 未发现 glad 自身完整许可文件；生成参数不构成可重复生成保证 |
| Khronos 平台头 | `vendor/KHR/khrplatform.h`：Khronos 来源说明及 2008–2018 版权 | 头部完整许可文本；当前安装规则未单列通知 |
| stb_image / stb_image_write | `vendor/stb/stb_image.h`：2.27；`vendor/stb/stb_image_write.h`：1.16；头部有 stb 项目地址 | `vendor/stb/LICENSE` 及头文件许可，MIT / Unlicense 双许可；安装为 `licenses/stb-LICENSE.txt`，并非仅覆盖读取组件 |
| FastNoiseLite | `vendor/fast_noise_lite/FastNoiseLite.h`：1.0.1、上游地址 | 头部完整 MIT 文本；当前安装规则未单列通知 |
| robin_hood | `vendor/robin_hood.h`：3.11.5、上游地址 | 头部完整 MIT 文本和 SPDX 标识；当前安装规则未单列通知 |
| yaml-cpp | 本地头文件和源码子集；未找到明确库版本/提交标识 | 未找到完整本地许可文件；当前安装规则未单列通知 |

以上记录是本地材料事实，不提供法律结论。“未发现”表示需补齐证据，不表示禁止使用；“头部有完整文本”也不意味着发行包已经附带该通知。当前游戏安装规则明确复制 SDL3、stb 的许可，独立 benchmark 安装规则复制本报告但没有单独安装 yaml-cpp 许可。GLFW 许可撤除只针对不再使用该库的新安装包，不改写历史包。报告不能替代完整的发行许可材料。

## 未使用材料与风险事项

| 仓库材料 | 当前状态 | 处理含义 |
| --- | --- | --- |
| `vendor/irrKlang`、`lib/irrKlang` | 当前生产源码和 CMake 无 active 引用；旧聚合头中的引用位于 `.disabled` 遗留文件 | 没有现行音频模块需要它；不能仅凭库文件存在宣布支持音频。恢复音频需重新决定接入、验证头/DLL 版本和许可 |
| `vendor/glfw`、`lib/glfw` | S5 后不再配置源码目标，也未链接历史 DLL / `.a` | 按 D11 保留历史材料，不是当前 MSVC 游戏的活动依赖 |
| `lib/yaml-cpp/libgtest*.a`、`libgmock*.a` | 未被当前测试链接；属于历史 Google Test/Mock 材料，不是 yaml-cpp 库 | 当前测试无需它们；未来采用测试框架应明确来源并按同一工具链构建 |
| `vendor/stb-master` | 当前工程只接入 `vendor/stb` 选定头；此目录未使用 | 历史审计仅确认 `stb_image.h` 内容相同，未确认整个目录一致；本次不删除材料 |
| `game/app/legacy/**/*.disabled` | 不参与生产源码编译；包含旧输入、事件、线程池与聚合头 | 搜索出的旧依赖不计入当前模块报告；尤其不能继续将 robin_hood 标为当前输入映射依赖 |

后续维护重点：

1. 补齐 yaml-cpp 的版本/提交及 GLM、glad、yaml-cpp 的完整许可材料，记录每项原始来源、本地修改与校验值；仅 vendoring 文件不能完整说明来源。
2. 根据实际发行目标补齐许可/通知安装规则，区分游戏包、runner-only 包、系统运行库和外部工具，不把闲置二进制一并当作必要依赖分发。
3. 保持 GLM 公共数学接口与其他第三方私有实现边界；如隔离 runner 的 `YAML::Node`，应作为明确的接口改造，而非简单改成 `PRIVATE`。
4. 变更依赖后重点核验 world 固定种子摘要、scene 顶点布局、纹理解码、renderer 函数加载/截图/计时、telemetry YAML 标量往返及 runner 文件协议/导出。
5. S5 本次只撤除 GLFW 的活动配置、授权和新包许可安装规则，没有升级、删除、重新生成 vendor/lib 库文件；旧二进制、历史附录和冻结包保持原样，历史二进制检查结论不冒充当前构建验证。

## 历史附录：M0 至 M2-T3 审计记录

以下保留此前盘点内容。这里的“当前”“顶层”及 `include/`、`src/` 路径指对应历史阶段，不代表 2026-10-05 模块化工程；现在的实际关系以前文为准。

### M0 范围与证据等级

- 盘点日期：2026-09-17，阶段：M0。
- 本文依据仓库内文件、源码引用、当前顶层 CMake 配置，以及本机已有 `dumpbin` 的只读输出。
- 依赖盘点没有下载、安装或升级依赖；实际配置、构建和启动由独立实验验证，结果见 [M0 主报告](../milestones/m0/README.md)。
- 下述版本与来源均为本地文件记载，未与上游发行包或提交进行逐文件比对。“文件记载来源”不等同于已经验证供应链来源。
- “未找到完整许可材料”表示仓库材料待补充，不表示禁止使用或分发。本文不作法律结论。

### M1 构建变更

以下 M0 表格保留历史审计事实，不代表修改后的构建图。M1 未升级或删除第三方源码，但调整了集成方式：GLFW 的示例、测试、文档和安装关闭；游戏显式链接 `glm::glm`；glad 与 yaml-cpp 分别进入静态库目标；取消未使用的 irrKlang 链接、DLL 复制和公共头引入。原二进制目录继续保留，不再纳入游戏构建及开发暂存安装。

这不是新增音频系统，也未补齐上游版本和许可材料。当前目标依赖关系见 [构建指南](../project/build/build-and-clion.md)，实际验证状态见 [M1 报告](../milestones/m1/README.md)。

### M0 使用的依赖

M2-T3 增量说明：复用已有 `vendor/stb/stb_image_write.h` 保存采样结束后的诊断截图，没有下载或升级 stb；CPU 内存读数使用 Windows SDK 的 Psapi，GPU 设备传感器可选调用本机已有 `nvidia-smi`。后者不是随包部署的依赖，缺失时明确记录。M0 表格及尚未补齐的许可/来源事项仍保留，不因本次增加观测而视为关闭。

表中路径均相对于仓库根目录。

| 依赖            | 本地版本与来源证据                                                                              | 当前实际使用                                                                                               | 本地许可材料                                             | ABI / 兼容性状态                                                                |
| ------------- | -------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------- | -------------------------------------------------- | -------------------------------------------------------------------------- |
| GLFW          | `vendor/glfw/CMakeLists.txt:3` 记载 3.3.7；`vendor/glfw/README.md` 记载项目说明与来源              | `CMakeLists.txt` 添加源码子目录并链接 `glfw`；负责窗口、输入与 OpenGL 上下文                                               | `vendor/glfw/LICENSE.md` 有完整许可文本                   | 当前使用源码构建，不依赖 `lib/glfw` 中的二进制；实际 MSVC 编译结果另行记录                             |
| GLM           | `vendor/glm/detail/setup.hpp:6` 起记载 0.9.9.8；`vendor/glm/glm.hpp:99` 起记载项目网站与 GitHub 地址 | `include/core.h` 引入数学相关头文件；CMake 另建立 `glm` 接口目标，但游戏目标未显式链接该接口目标，当前依靠全局包含目录                           | 未在 `vendor/glm` 内找到完整许可文件                          | 主要为头文件，无独立预编译库 ABI；C++20 / MSVC 源码兼容性需由构建确认                                |
| glad          | `vendor/glad/glad.h:3` 起记载由 glad 0.1.35 于 2022-06-05 生成，目标为 OpenGL 4.6 core，并保留生成参数    | `vendor/glad/glad.c` 直接编入游戏，加载 OpenGL 函数                                                             | 未在 `vendor/glad` 内找到 glad 自身的完整许可文本                | 无预编译 ABI；生成记录标注 `Reproducible: False`，当前不能仅凭参数保证重新生成内容相同                   |
| Khronos 平台头   | `vendor/KHR/khrplatform.h:30` 指向 Khronos EGL Registry；头部有版权年份                          | glad 所需的平台类型定义                                                                                       | 头部有完整版权与许可文本                                       | 头文件，无独立预编译 ABI                                                             |
| stb_image     | `vendor/stb/stb_image.h:1` 记载 2.27，并记载项目地址                                             | `include/renderer/texture.h:3` 引入；`src/core/application.cpp:5` 定义实现宏；纹理加载使用 `stbi_*`                 | `vendor/stb/LICENSE` 与头文件末尾含 MIT / Unlicense 双许可文本 | 随应用源码编译；无外部预编译 ABI                                                         |
| FastNoiseLite | `vendor/fast_noise_lite/FastNoiseLite.h:47` 记载 1.0.1，下一行记载上游地址                         | 区块地形噪声，实际使用 OpenSimplex2 与 FBm                                                                       | 头部有完整 MIT 文本                                       | 头文件，无独立预编译 ABI                                                             |
| robin_hood    | `vendor/robin_hood.h:37` 起记载 3.11.5；`:9` 记载上游地址                                        | 区块、方块配置、输入映射与着色器变量等容器                                                                                | 头部有完整 MIT 文本及 SPDX 标识                              | 头文件，无独立预编译 ABI                                                             |
| yaml-cpp      | 本地目录为头文件与源码子集；未找到明确库版本或上游提交标识                                                          | `CMakeLists.txt:22` 起直接收集源码编入游戏；`src/world/block.cpp:35` 使用 `YAML::LoadFile` 读取方块配置                  | 未在 `vendor/yaml-cpp` 内找到完整许可文件                     | 当前随应用源码编译，并未链接 `lib/yaml-cpp` 中的文件；`vendor/yaml-cpp/dll.h` 默认不启用 DLL 导入导出宏 |
| irrKlang      | `vendor/irrKlang/irrKlang.h:37` 记载头文件版本 1.6.0，并包含厂商项目地址；DLL 文件资源未提供可读取的版本号             | CMake 固定链接并复制 `lib/irrKlang`；`include/core.h:60` 引入头；唯一发现的引擎创建语句在 `src/core/application.cpp:29`，已被注释 | 头部有版权及免责声明；未找到完整发布许可材料                             | 导入库和三个 DLL 已确认 x64；导入库为 MSVC 风格符号。头文件与 DLL 版本是否完全匹配、实际加载调用是否正常仍待验证         |

### M0 预编译文件与重复材料

| 路径 | 已验证事实 | 当前构建关系 | 注意事项 |
| --- | --- | --- | --- |
| `lib/irrKlang/irrKlang.lib` | `dumpbin /headers` 显示 x64；导出 MSVC 风格 `?createIrrKlangDevice@irrklang@@...` 等导入符号，指向 `irrKlang.dll` | 顶层显式链接 | 没有发现 x86 / x64 架构混用，不能将其列为已证实的位数冲突 |
| `lib/irrKlang/irrKlang.dll` | x64；linker version 14.12；静态依赖表列出 `KERNEL32.dll`、`USER32.dll`、`ole32.dll`、`WINMM.dll` | 顶层构建后复制 | 静态依赖表没有显式 VC runtime DLL，不代表已经完成运行时验证或动态依赖验证 |
| `lib/irrKlang/ikpMP3.dll`、`ikpFlac.dll` | 均为 x64；linker version 14.12；静态依赖表仅列 `KERNEL32.dll` | 随整个目录复制 | 是否需要分发插件应由实际音频范围决定 |
| `lib/glfw/glfw3.dll`、`libglfw3.a`、`libglfw3dll.a` | 均显示 x64；DLL linker version 为 2.30，依赖表包含 `msvcrt.dll`；检查 DLL 时工具报告 LNK4078 节属性警告 | 当前顶层不引用，使用 `vendor/glfw` 源码目标 | 不应仅依据扩展名或位数认定 MSVC 兼容；这些文件也不是当前已证实的构建阻塞项 |
| `lib/yaml-cpp/libgtest.a`、`libgtest_main.a`、`libgmock.a`、`libgmock_main.a` | 实际是 Google Test / Mock 命名的四份库，均显示 x64；`libgtest.a` 符号表包含 `_ZN...` 风格 C++ 符号 | 当前顶层不引用；它们不是正在链接的 yaml-cpp 库 | 不应直接作为 MSVC 测试依赖接入；如后续引入测试框架，应使用与主工程一致的工具链从明确来源构建 |
| `vendor/stb-master` | 项目源码未发现引用；其中 `stb_image.h` 与 `vendor/stb/stb_image.h` 的 SHA256 相同 | 当前实际使用 `vendor/stb` | 仅确认这一个头文件相同，不代表整个目录逐文件一致；M0 不删除文件 |

### M0 构建边界观察

1. 顶层 CMake 没有统一依赖清单、版本锁定记录或校验值清单。源码随仓库存放能固定当前文件内容，但尚不能明确追溯每项的原始发行包和后续修改。
2. GLFW 自带构建默认开启示例、测试、文档选项，顶层没有主动关闭。它们扩大默认构建范围，但不能未经构建就认定为失败原因。
3. 当前主要依赖从源码或头文件参与构建。irrKlang 是游戏目标显式链接的外部预编译依赖，应与未使用的 `.a` 文件分开评估。
4. 音频初始化目前被注释。第一阶段是恢复现有玩法，不应把更换音频库或新增音频功能自动纳入范围。

M0 后续实际构建补充：Debug / Release 的游戏目标已使用 MSVC 19.38 x64 编译链接成功；两个最终可执行文件的静态导入表未列出 irrKlang.dll。Release 依赖 MSVC/UCRT 运行库，Debug 依赖调试版运行库，不能把 Debug 产物当作独立发布版本。见 [最终二进制依赖记录](../milestones/m0/evidence/08-executable-dependencies.log)。这个结果不替代启用音频后的实际加载与调用测试。

### M0 本机复核方法

本次使用 VS2022 已安装工具，只读检查，不执行编译或链接。以下命令在仓库根目录的 PowerShell 中运行；工具路径是本次机器记录，不是项目必须硬编码的路径。

```powershell
$dumpbin = 'E:\Applications\Microsoft VS\2022\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\dumpbin.exe'

# 查看二进制架构与链接器版本。
& $dumpbin /headers lib/irrKlang/irrKlang.lib lib/irrKlang/irrKlang.dll lib/irrKlang/ikpMP3.dll lib/irrKlang/ikpFlac.dll

# 查看静态导入依赖，不等同于实际加载验证。
& $dumpbin /dependents lib/irrKlang/irrKlang.dll lib/irrKlang/ikpMP3.dll lib/irrKlang/ikpFlac.dll

# 查看导入库符号。
& $dumpbin /linkermember:1 lib/irrKlang/irrKlang.lib

# 检查当前未使用的预编译库。
& $dumpbin /headers lib/glfw/glfw3.dll lib/glfw/libglfw3.a lib/glfw/libglfw3dll.a
& $dumpbin /headers lib/yaml-cpp/libgtest.a lib/yaml-cpp/libgtest_main.a lib/yaml-cpp/libgmock.a lib/yaml-cpp/libgmock_main.a
& $dumpbin /linkermember:1 lib/yaml-cpp/libgtest.a

# 复核两份实际相关头文件的内容是否相同。
Get-FileHash vendor/stb/stb_image.h,vendor/stb-master/stb_image.h -Algorithm SHA256 | Format-List Path,Hash
```

本次 `dumpbin` 报告版本为 14.44.35222.0。这里记录的是审计工具版本，不是主工程最终选定的编译器版本；不能用它推断 CMake 实际使用了 MSVC 14.44。

### M0 后续建议与验收

| 阶段 | 建议动作 | 验收证据 |
| --- | --- | --- |
| M1 构建恢复 | 明确 MSVC x64 工具链；保留当前依赖基线，先处理实际编译与链接错误，不同时全量升级依赖 | 干净构建目录中的配置、Debug / Release 构建记录及实际编译器版本 |
| M1 运行恢复 | 明确现有玩法是否需要 irrKlang；若继续启用，则验证头文件、DLL 版本与运行调用；若不需要，评估独立可选化 | 依赖决策记录、加载结果和现有玩法冒烟检查 |
| 工程整理 | 为每项依赖记录来源、版本或提交、文件校验值、本地修改说明；区分运行依赖与第三方开发材料 | 依赖清单可追溯，主工程依赖通过目标级配置表达 |
| 发布准备 | 补齐缺少的许可与通知材料，并依据确定的发布范围核对分发要求 | 发布目录中的第三方通知与依赖清单；不以本次本地材料盘点替代授权核对 |
| 后续测试建设 | 使用同一 MSVC 工具链构建选定测试框架，不直接复用来源不明的 `.a` | 测试依赖来源、构建配置与测试执行记录 |

动态区块加载属于后续功能与性能阶段，不影响本次依赖证据的范围，也不是 M0 的实现内容。
