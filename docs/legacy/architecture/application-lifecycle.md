# 应用生命周期与运行验证

> 历史架构参考：M2-A，含 M2-T2 / M2-T3 增补。文中的“当前”指该阶段，不代表最新实现；归档不表示相关机制全部废弃。源码链接保留定位用途，不固定历史版本或行号。参见 [历史架构索引](README.md)；后续设计约定见 [项目范围](../../spec/project-scope.md)，不据此推定重构已经完成。

## 范围与状态

本文说明 M2-A 当前 `main`、`Application`、方块配置加载，以及 `verify-runtime*.ps1` 的接口和数据流。它描述代码契约与验证方法，不是测试通过记录；实际构建、真实驱动运行、故障注入和人工玩法结果见 [M2-A 阶段报告](../../milestones/m2-a/README.md)。

M2-T2 更新：保留下述图形生命周期，新增 [可复现生成](world-generation.md) 与 [场景输入](../../project/benchmark/testing/reproducible-scenes.md)。`main` 通过 `ParseStartupOptions` 解析参数，图形模式调用 `Run(const StartupOptions&)`；无窗口模式调用 `PrintWorldSummary`，不调用 `Init`。当前测试及运行证据见 [M2-T2 报告](../../milestones/m2-t2/README.md)。

M2-T3 更新：`Init` / `Run` 接受启动参数及可选 `Performance::Session*`，默认不采样。benchmark 模式固定窗口和工作负载，分别记录 CPU、上传、异步 GPU 查询及内存；完整接口与计时边界见 [性能观测](performance-observation.md)。这不改变普通模式的输入与暂停契约，也不把脚本移动当成人工玩法验收。

本模块负责把已有子系统按明确顺序组织起来，使启动失败能留下诊断，并在正常退出或受控异常后清理已创建的资源。它不实现动态区块加载、线程调度、完整保存系统，也不替代 M2 的人工连续游玩、指定笔记本实测和后续性能基准。

## 职责与入口

| 位置 | 职责 | 不负责的事项 |
| --- | --- | --- |
| `src/main.cpp` | 解析参数、资源存在性预检、调用初始化和运行、捕获运行异常、统一清理与退出码 | 不直接生成世界或持有 GL 对象 |
| `src/core/application.cpp` | 拥有窗口、相机、注册表；安排子系统初始化、固定世界生成、每帧更新和逆序清理 | 不把各模块实现合并成新的通用引擎框架 |
| `src/world/block.cpp` | 校验和发布方块配置，提供名称/ID 查询及纹理层范围检查 | 不创建纹理，不替代碰撞与网格算法 |
| `scripts/verify-runtime.ps1` | 启动真实进程、限制等待时间、保留输出并检查运行标记 | 不自动操作键鼠，不做视觉验收或帧时间统计 |
| `scripts/verify-runtime-faults.ps1` | 在独立副本中制造三种资源损坏，核对受控失败与原资源未变 | 不覆盖全部故障类型，不修改正常运行包中的资源 |

相关子模块详见 [资源定位](asset-paths.md)、[运行期图形资源](runtime-resources.md)、[网格安全](mesh-safety.md) 和 [玩家循环](player-loop.md)。

## 启动数据流

```text
命令行参数
  -> main 校验参数组合与数值
  -> Assets::CheckRequiredAssets 检查 exe 旁六个必需文件
  -> --check-assets 模式到此结束
  -> Application::Init
       -> GLFW 初始化
       -> 创建窗口、上下文并加载 OpenGL
       -> 创建 ECS 注册表并注册组件
       -> 标记 renderer_started，初始化 Renderer
            -> 相机、shader、批次、方块配置
       -> World::Init 创建玩家
  -> Application::Run
       -> 加载纹理图集并验证方块纹理层索引
       -> 安装输入与焦点回调
       -> PrepareWorld 确定实际 seed、初始化生成器、创建全部固定区块
       -> 建立邻居关系
       -> 按区块坐标顺序生成全部地形
       -> 再按固定顺序生成全部植被
       -> 可选测试场景覆盖与编辑、生成配置/摘要日志
       -> 搜索普通出生位置或选择固定测试姿态、同步 Transform 与相机
       -> 生成初始区块网格
       -> 重置物理时钟与帧时钟
       -> 输出 [runtime] ready
```

参数解析和资源存在性检查发生在窗口创建之前。`--check-assets` 不初始化 GLFW，不编译 shader，不解析 YAML，也不解码 PNG；所以一个存在但内容损坏的文件可以通过该预检，随后在真实启动阶段失败。

`--world-summary` 在资源预检后加载真实方块 YAML，走同一个 `PrepareWorld` 生成/摘要入口，输出纯 YAML，再走 `Application::Free` 清空区块；不创建窗口、注册表、相机或 GL 对象，也不输出 ready / loop / shutdown 标记。此模式不验证纹理解码或 shader 编译，不能替代真实渲染探针。

`Application::Init(options, performance)` 拒绝重复初始化。`GetWindow()` 和 `GetRegistry()` 在对应对象尚未创建时抛出逻辑错误，不再隐式构造一个未就绪对象。`GetCamera()` 只在已有窗口的前提下按需创建相机。

`renderer_started` 在调用 `Renderer::Init()` **之前**设置，使 shader、批次或配置加载中途失败后，清理路径仍进入 `Renderer::Free()`。因此各图形对象的销毁接口必须接受“尚未创建”或“只创建了一部分”的状态。

创建全部区块、建立邻居、生成全部地形、生成全部植被是不同步骤。区块通过引用遍历，不复制所有权；跨边界植被不会因后续邻居地形初始化而被覆盖。

出生点在中心附近逐层搜索，优先选取允许的地面方块并避开水面和树木。未找到合适位置时抛出异常，而不是继续使用原先可能落在地形内部的固定高度。该搜索不是完整导航或所有复杂地形的可达性证明。

## 每帧数据流

```text
轮询窗口事件
  -> 检查关闭状态与活动状态
  -> 处理失焦/最小化、清理粘滞按键、重置时钟
  -> 计算本帧 delta_time，正常情况下夹到 0..0.1 秒
  -> 更新交互冷却时间
  -> 一次采样本帧按键，更新玩家输入意图
  -> TransformSystem 更新方向
  -> Character::Player 应用玩家移动意图
  -> Physics 更新
  -> 必要时把掉出世界的玩家送回出生点
  -> 相机同步到本帧玩家位置
  -> 射线检测及方块交互
  -> 更新失效区块网格，装入渲染批次
  -> 绑定纹理、渲染、交换缓冲
  -> 递增已渲染帧数，检查冒烟帧数上限
```

正常交互运行中，窗口失焦、最小化或宽高不合法时暂停模拟，通过短时等待窗口事件避免空转。暂停和恢复会清除水平速度、移动输入及待处理跳跃，并重置物理计时。恢复首帧的 `delta_time` 为零，避免把失焦期间累计时间一次性送入模拟。

GLFW 焦点回调之后可能继续发送合成的按键释放事件，因此粘滞按键状态在事件轮询后清理。鼠标重新进入时先更新参考坐标，避免恢复焦点后的第一次移动产生视角跳变。

若玩家高度低于 `-8`，应用重新寻找出生位置、清理力和落地/跳跃状态并重置物理计时。这是现有有限世界的恢复路径，不表示已经支持无限地形或完整世界边界设计。

## 所有权与退出顺序

| 资源 | 持有者 | 释放方式与先后关系 |
| --- | --- | --- |
| GLFW 全局初始化状态 | `Application` 的状态标记 | 窗口销毁后才调用 `Window::Free()` / GLFW 终止 |
| 窗口包装对象 | `Application::runtime_window`，`unique_ptr` | 显式 `Destroy()` 原生窗口后重置包装对象 |
| 相机 | `Application::camera`，`unique_ptr` | Renderer 停止持有非拥有指针后释放 |
| ECS 注册表 | `Application::runtime_registry`，`unique_ptr` | 显式 `Clear()` 后重置；ECS 内部容器正确性仍属于该模块责任 |
| 区块、CPU 方块与网格 | 区块管理器与各区块的容器 | `FreeAllChunks()` 清空管理器，不保留已释放的区块条目 |
| shader、VAO/VBO | Renderer 与批次对象 | `Renderer::Free()` 在上下文仍有效时释放 |
| 纹理图集 | `Run()` 局部 `TextureArray` | 正常离开 `Run()` 或异常展开时先析构，早于应用销毁窗口 |
| GPU 计时查询 | benchmark 下 `Run()` 局部 `unique_ptr<GpuTimer>` | 同样在上下文销毁前释放，不等待尚未就绪的采样结果 |
| CPU 性能记录 | `main` 局部 `unique_ptr<Performance::Session>` | `Application::Free()` 后导出，不在主循环写 CSV 或排序 |
| 方块配置映射 | 方块模块的进程级容器 | 校验成功后替换；不是由 `Application::Free()` 清空的局部运行资源 |

正常结束与 `Init()` / `Run()` 抛出异常后，`main` 都会进入同一清理入口：

```text
离开 Run / 异常展开
  -> Run 局部 GPU 查询和纹理释放
  -> Application::Free
       -> 清空区块
       -> 若 Renderer 曾开始初始化，释放批次和 shader
       -> 释放相机
       -> 清空并释放 ECS 注册表
       -> 销毁并释放窗口
       -> 终止 GLFW
  -> 若启用 benchmark，导出 CPU 数据并核对采样有效性
  -> 旧分配器的泄漏检查入口
  -> 输出 [runtime] shutdown complete; exit_code=...
  -> 返回进程退出码
```

最重要的不变量是：**删除 GPU 对象时，对应 OpenGL 上下文仍然存在。** 不能先关闭窗口，再依赖静态对象析构去删除 GPU 资源。

状态标记与空指针判断用于覆盖正常退出和部分初始化失败，不代表可以承诺任意异常后的无限次重新初始化。进程级配置、输入冷却和旧 ECS 的全部状态尚未建立统一的“重开世界”契约。

## 退出码与可观察标记

| 退出码 | 当前含义 | 典型路径 |
| --- | --- | --- |
| `0` | 所请求模式正常结束 | 资源预检成功；正常关闭游戏；达到冒烟帧数上限 |
| `2` | 资源预检失败 | 必需文件缺失、不可访问，或资源路径无法安全解析；尚未创建游戏窗口 |
| `3` | 初始化或运行阶段捕获到异常 | 窗口/GL 初始化、shader、纹理、方块配置、网格/批次等受控异常；随后调用统一清理 |
| `4` | benchmark 已导出但采样无效 | 提前退出、失焦、最小化、分辨率改变或没有完成约定采样时长 |
| `64` | 参数使用错误 | 未知参数、冲突组合、缺少帧数、非整数或帧数超出允许范围 |

`[runtime] ready;` 表示纹理、世界与初始 CPU 网格已准备好，不表示已经绘制首帧或完成玩法验收。`[runtime] loop finished; rendered_frames=...` 记录实际结束循环时累计的渲染帧数。`[runtime] shutdown complete;` 位于应用清理之后，用来区分受控结束与直接中断。

参数错误和仅资源预检路径提前返回，不输出完整运行阶段的 ready / loop / shutdown 标记。脚本不应要求这类路径伪造运行标记。

退出码 `3` 的处理覆盖 `Init()` 和 `Run()` 的 C++ 异常，不是操作系统级崩溃、驱动挂起、强制终止或清理代码本身失败的通用恢复机制。超时后由验证脚本强制结束的进程不能作为“干净退出”通过。

## 有限帧冒烟模式

```powershell
.\out\build\windows-debug\bin\SymoCraft.exe --smoke-frames 120
```

`--smoke-frames` 仅接受 `1..10000` 的整数，可以与 seed、测试场景和检查点组合，不能与 `--check-assets` 或 `--world-summary` 组合。`0` 不是合法的该命令行参数；不指定该选项的图形运行不设帧数上限。

该模式仍创建真实 GLFW 窗口和 OpenGL 上下文，加载全部正常资源、生成世界并运行正常更新与渲染流程，不是无窗口单元测试。达到指定的渲染帧数后请求关闭窗口，再走正常清理。

为了让自动探针在非前台环境推进，有限帧模式不按正常模式的失焦/最小化分支暂停。这是测试运行边界，而非对交互模式失焦体验的替代验证。窗口事件或用户主动关闭也可能让实际帧数少于请求值；退出码可能仍为 `0`，但验证脚本会把帧数不匹配判为失败。

上限约束的是渲染循环迭代次数，不是墙钟时间。初始化卡住、驱动调用阻塞或单帧异常缓慢时，仍需要外部超时。

## 方块配置的分阶段校验

`LoadBlocks()` 不边读取边修改当前生效的全局映射。它先在临时 `formats` 和 `names` 容器里完成解析与结构校验，通过后才移动到正式映射：

1. 根节点必须为非空映射。
2. 每个条目必须有非空名称，值必须为映射。
3. ID 必须位于 `1..65535`，名称与 ID 不允许重复。
4. `side`、`top`、`bottom`、`isTransparent`、`isSolid` 必须可转换成目标类型；可选的混合、光源和光照字段有显式默认值。
5. 现有地形与快捷栏所依赖的 11 个名称必须存在，且对应固定 ID `1..11`：空气、草方块、沙、泥土、石头、橡木原木、橡树叶、橡木木板、水、桦木木板、圆石。
6. 校验完成后发布新映射，避免解析失败时留下只加载了一半的新配置。

纹理层范围属于另一个阶段：`Run()` 成功创建纹理数组后调用 `ValidateBlockTextures(layer_count)`，要求层数大于零，每种方块的三种纹理索引均小于层数。该校验不包含在前述配置提交事务内；失败时终止启动，而不是宣称恢复到前一组“配置加纹理”的完整组合。

`get_block_id()` 按 `string_view` 的实际长度构造查找键，不假定其末尾有零字符。未知名称返回 `0`，未知或超范围 ID 返回透明、非实体的空格式，避免为未知 ID 隐式插入未初始化数据。保留固定 ID 是本阶段与旧地形/快捷栏的兼容约束，不是最终数据驱动方块注册系统。

## 运行探针与证据

### 普通运行探针

```powershell
.\scripts\verify-runtime.ps1 `
    -Executable .\out\build\windows-debug\bin\SymoCraft.exe `
    -OutputDirectory .\out\evidence\runtime-debug-001 `
    -Frames 120 -TimeoutSeconds 120 -ExpectedExitCode 0
```

输出目录必须尚不存在，脚本不会覆盖旧证据。子进程以该目录作为工作目录，验证运行不依赖源码目录；资源仍从 exe 旁加载。脚本使用隐藏启动设置，但仍执行真实图形程序，不提供无显示设备替代方案。

每次保留 `stdout.log`、`stderr.log` 和 `result.json`。JSON 记录程序路径、工作目录、请求/实际帧数、GL 诊断计数、进程 ID、超时、实际/预期退出码、ready / shutdown 标记、经过时间及判定结果。

M2-T2 为普通运行探针增加 `-Seed`、`-Scene terrain|regression`、`-Checkpoint`、`-TestEdits`，JSON 额外记录实际 `arguments`。只传显式给出的 seed，不把默认数值 0 当作用户输入；检查点/编辑要求 regression。该脚本仍只检查图形运行，`--world-summary` 由独立无窗口 CTest 验证。

正常成功条件是：未超时、退出码为 `0`，存在 ready、loop finished 和 shutdown 标记，没有 fatal 文本，且标准错误中的 `OpenGL diagnostic` 数量为零。`Frames > 0` 时实际帧数还必须精确等于请求值。诊断计数采用保守策略，不按严重程度自动放行。

`Frames=0` 表示脚本不添加 `--smoke-frames` 参数，不是向游戏传入非法的零帧值；进程仍受脚本超时约束，需要正常关闭才能通过。预期退出码 `3` 的探针要求在 ready 之前失败且存在 shutdown 标记，不因预期的 shader 编译 GL 诊断而直接失败。它是启动失败探针，不用于验证已进入游戏后的异常恢复。预期退出码 `2` 的检查主要核对退出码与未超时，不要求运行标记。该脚本的 `ExpectedExitCode` 不接受 `64`。

### 资源故障探针

```powershell
.\scripts\verify-runtime-faults.ps1 `
    -Executable .\out\build\windows-debug\bin\SymoCraft.exe `
    -OutputDirectory .\out\evidence\runtime-faults-debug-001
```

脚本为每个用例创建独立的 `package` 和 `evidence` 目录，复制 exe 与其相邻资源，只改副本中已确认位于夹具目录内的文件。三个用例为：非法顶点 shader、不可解码的 PNG 内容、空方块 YAML。所有文件仍存在，因此目标是穿过资源存在性预检，进入真实运行的内容校验路径。

每个用例调用一帧运行探针并预期退出码 `3`，随后额外检查 OpenGL vendor / renderer / version 日志、对应故障诊断、未到达 ready，以及带退出码的完整 shutdown 标记。相关原始资源在探针前后计算 SHA256；原文件被改变会使验收失败。

夹具和证据保留供检查，每例生成 `fault-result.json`，汇总为 `summary.json`。输出目录不得覆盖已有目录或位于源资源目录内部。脚本目前只复制 exe 与 assets；若未来增加外部运行库，需同步扩展夹具构造，否则不能把 DLL 缺失误判为预期资源故障。

**本节仅说明已实现的探针逻辑，不宣称三项真实故障注入已经执行或通过。**

## 测试覆盖与结论边界

`world.block_config` 对应 `tests/block_config_tests.cpp`，当前覆盖正常配置、实体/空气标志、树叶混合标志、非零结尾 `string_view` 查找、纹理层越界，以及非映射根节点、重复 ID、缺少必需名称、负 ID 等失败输入。失败加载后还检查原先有效的草方块 ID 未被替换。它不等于对所有 YAML 输入、重复名称、数值溢出与所有可选字段组合进行了穷尽验证。

其他 CTest 分别覆盖资源路径、按键快照、玩家数学逻辑、网格和批次边界。真实窗口、驱动、着色器编译和解码失败路径通过上述外部探针补充；人工键鼠交互、视口变化、长时间运行与游戏视觉正确性仍需独立检查。

日志中的 `loading_ms` 从进入 `Run()`、加载纹理前开始，到纹理、固定世界和初始 CPU 网格准备好后结束。它不包含先前 `Init()` 中的窗口、shader、批次和 YAML 加载，也不包含首帧 GPU 上传与呈现。脚本的 `elapsedSeconds` 则是整个子进程启动到结束的墙钟时间，两者不可混用。

这些有限帧探针时间只是运行观测，不是性能基准；摘要扫描也计入现有 `loading_ms`。M2-T3 另外提供 [正式采样入口](../../project/benchmark/testing/performance-baseline.md)，使用原始逐帧数据、CPU/GPU 分解及多轮统计。不能由“120 帧正常结束”或一条加载时间推导出稳定 60 FPS、15 分钟稳定性、无泄漏，或 Y9000P 笔记本验收通过。
