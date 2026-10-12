---
tags:
  - area/architecture
---

# Renderer

本模块只接收显式场景与资源输入，不读取 World、ECS、Application singleton 或游戏输入。应用拥有窗口、世界转接和帧编排；Renderer 拥有接受后的渲染资源与后端状态。公开行为唯一正文为 [renderer spec](../openspec/specs/renderer/spec.md)，本文维护支撑正确性的当前内部机制，不是第二份规格、逐类学习笔记或验收报告。

## 实现与边界

- 默认游戏路径为 D3D12 现代门面；OpenGL 4.6 的 `SymoCraft::Renderer` namespace 保留为显式过渡路径，独立于现代门面。
- `SymoCraft::Rendering::Renderer` 是不可复制、不可移动的实例/PImpl。CPU 核心已实现；启用 `SYMOCRAFT_RENDER_D3D12` 后可创建生产私有 D3D12 后端，否则请求 D3D12 具名返回 `Unsupported`。现代工厂的 OpenGL、Vulkan 均未实现，不静默 fallback。
- `RendererConfig` 与应用 `StartupOptions` 均默认选择 D3D12；应用在窗口创建前选择 Native mode。新游戏配置默认编入 D3D12，旧缓存不强改；未编入的默认请求仍具名失败、不自动选 GL。显式 `--renderer opengl` 才进入保留的旧路径。
- 中立公开头只暴露场景值类型、句柄、结果与错误；私有 Registry、RenderBackend、GL/D3D12 类型不得成为业务扩展点。CPU-only 保留现代 CPU 核心，不引入原生设备与窗口依赖。
- Shader 编译、目标依赖与 CPU-only 构建隔离见 [工程契约](engineering.md)。关键节点与历史验证依据统一从 [VERIFICATION](../verification/VERIFICATION.md)进入；人工/R3 整组验收、默认切换及 Vulkan 不得由源码存在推定完成。

依据：[模块目标](../../game/modules/renderer/CMakeLists.txt)、[现代公开头](../../game/modules/renderer/include/symocraft/renderer/renderer.h)、[后端工厂](../../game/modules/renderer/src/backend/backend_factory.cpp)、[应用适配](../../game/app/src/game_renderer.cpp)。

## 现代门面机制

### 输入、所有权与资源身份

应用先创建 Window，再创建 Renderer；显式 `Shutdown` 与析构均须在 Window 有效期间完成。Renderer 借用窗口，不能销毁它。每个显式 API，包括 `GetInfo` 和 `Shutdown`，只允许创建线程串行调用并拒绝重入；原子 identity 分配器不代表 Registry 可以并发访问。

资源输入在 Create/Update 当次调用自有化为不可变 CPU 快照，不保留调用者容器地址。`FrameInput::draws` 是仅至 `Render` 返回的借用 span；门面完整准备自有 draw 列表和资源引用后才进入后端。相机、selection、transform 是值输入。返回 Info、样本与 Screenshot 为自有值，不借用原生对象或内部槽位。

Mesh/TextureArray 句柄类型分离，身份由 owner、slot、generation 共同组成。默认句柄为 null；空、跨实例、过期、重复销毁均为 `InvalidHandle`。销毁成功立即逻辑失效，不等于在途 GPU 资源已释放；后端负责保留旧原生版本至最后使用完成。owner 耗尽拒绝分配，generation 耗尽将 slot 永久退役，不回绕复用身份。

Create/Update 的接受顺序是：验证与复制 → 后端成功接受 → 无新分配的 CPU 发布。可恢复失败不发布新句柄、不替换旧接受内容；空 mesh 合法，空更新清除原有几何。没有 TextureArray update API，也不自动级联销毁引用纹理的 mesh。

输入格式、相机/transform/selection 值域及配对拒绝条件由现役 spec 维护。实现先检查纹理长度乘法溢出，以 double 验证相机基底，并在 Render 配对后再核对 mesh layer 与所绑定纹理层数。World 顶点已经是世界坐标；区块平移与 UV 转换不能在 app 和 backend 重复执行。

依据：[公开值类型](../../game/modules/renderer/include/symocraft/renderer/types.h)、[校验](../../game/modules/renderer/src/validation.cpp)、[资源注册](../../game/modules/renderer/src/resource_registry.cpp)。

### 帧、状态与失败边界

Render 在提交任何 draw 前校验全部 draw、资源配对、相机、selection、能力和 frame ID。首帧 ID 可为 0，之后每个成功接受或合法跳过的帧 ID 严格递增；输入或可恢复准备失败不消费 ID，同 ID 修正后可重试。

| 状态 | 可用行为与转移 |
| --- | --- |
| Ready | 接受资源与正常帧；成功零尺寸 Resize 进入 Suspended |
| Suspended | 仍完整校验帧与能力；合法帧返回 `Skipped/ZeroExtent`，不进入后端 Render，消费 ID；成功非零 Resize 恢复 Ready |
| Failed | 拒绝资源、Render、Resize、SetVSync、Poll；仅允许读取最后 Info 快照和 Shutdown 清理，不隐式恢复或切换后端 |
| Stopped | 正常 Shutdown 完成，全部资源句柄逻辑失效；仅允许 GetInfo 和幂等 Shutdown |

后端在帧边界突然检测真实零尺寸时，可返回 ZeroExtent skip，但实际 Info 必须同样 suspended。Resize 同尺寸、SetVSync 同请求值不重复调用后端；成功操作后刷新实际 Info，不能用请求值冒充实际 samples、extent、呈现模式或设备身份。

`RenderError` 携带分类、后端、操作及可选原生码/诊断。参数、句柄、可恢复分配或能力错误不会自动把门面置 Failed；DeviceLost、SubmissionFailure、Timeout、InitializationFailure 会停止续交。原生操作已成功后若 Info 读取失败，任何异常分类都先进入 Failed，不能拿旧缓存继续提交。后端开始不可回滚提交后也不得再报告为可重试失败。

帧结果必须对应输入 ID，Presented 不得带 skip reason，Skipped 只允许 ZeroExtent/Occluded；时长非负。请求且 Presented 的截图必须存在，实际 extent 与紧密 RGBA8 长度一致；Skipped 不伪造截图。门面 `cpu_prepare` 加上后端准备时间，保留原生 `cpu_submit/present_wait` 与计数；准备时长累计溢出进入 Failed。

Shutdown 开始即停止提交，后端有界清理成功后才 Clear/Stopped；失败保持 Failed，不冒充成功。析构 noexcept 请求有界兜底并释放 CPU 引用，不代替正常显式关闭，也不能把记录的清理故障隐藏成验收通过。

### 保留的不变量编号

以下延续已有 P-I/G-I 编号，便于历史决策与证据定位；编号不是逐函数覆盖要求。

| 编号 | 当前约束 |
| --- | --- |
| P-I1 | PImpl 独占状态，窗口仅借用；Renderer 先于 Window 关闭与销毁 |
| P-I2 | 所有显式入口 owner-thread 串行、非重入，包括查询与关闭 |
| P-I3 | backend 成功才发布资源 CPU 接受状态；不保留调用者地址 |
| P-I4 | 完整校验与自有帧准备先于唯一 backend Render；不跨返回借用 draw span |
| P-I5 | fatal 或原生已接受后无法取得实际 Info 均停止续交；GetInfo/Shutdown 仍可用 |
| P-I6 | 零尺寸合法帧完整校验后 skip，不呈现但消费 ID |
| P-I7 | 正常 Stopped 后句柄逻辑失效，只允许查询与幂等关闭 |
| P-I8 | 后端信息、结果、query 身份、缺失时长与截图不得伪造成成功事实 |
| G-I1/G-I2 | 非零唯一 owner；活句柄须 owner/slot/data/generation 全部匹配 |
| G-I3/G-I4 | 不可变自有快照；Prepare 不发布，后端成功后 Commit/Replace |
| G-I5 | owner/generation 不回绕；generation 0 的 slot 永不重用 |
| G-I6 | 逻辑失效与 GPU retirement 分离，后端保存最后在途使用 |
| G-I7 | Registry 私有；State guard 与句柄验证先于 noexcept mutation |

依据：[门面执行与状态](../../game/modules/renderer/src/renderer_impl.cpp)、[错误分类](../../game/modules/renderer/include/symocraft/renderer/error.h)、[私有后端契约](../../game/modules/renderer/src/backend/render_backend.h)。

## D3D12 正确性机制

### 设备、绘制与资源退休

后端借用 Native-mode Window 的 HWND，独占 device/queue/swapchain/fence、三组 FrameContext、管线与资源版本。它不创建或销毁窗口，不回调门面。设备只从 high-performance 非 software adapter 中选择，要求 FL12_0/SM6_0，不退回 WARP 或其他 backend。

请求 samples 必须是 1 至 32 的受支持二次幂，颜色与深度格式均支持才接受；不静默降级。非零使用三缓冲 FLIP_DISCARD、单样本 backbuffer、D32 depth；samples > 1 绘制到独立 MSAA color/depth，再 resolve 到 backbuffer。VSync-on 是 interval 1/Fifo；off 要求真实 tearing 支持才使用 interval 0/Immediate。校验开启时 InfoQueue warning/error/corruption 也触发失败，不只检查 HRESULT。

Mesh 当前使用自有 UPLOAD buffer；Update 先准备新版本再替换当前 map。TextureArray 使用 DEFAULT texture 与自有 UPLOAD footprint，逐层 copy/barrier/Execute/Signal/有限等待后才发布；等待失败仍保留 pending 上传所有者用于故障清理。当前 map 被 Update/Destroy 替换或删除后，在途 FrameContext 的 shared refs 继续保留旧 mesh/texture。

每个 slot 的 allocator/list、descriptor、临时 selection/readback 与 query 在旧 fence 完成前不重用。正常 Render 只有一处 Present1：draw Execute → draw retirement Signal → Present1 → 覆盖 Present 的 retirement Signal。Resize、VSync、资源操作不得额外呈现。所有 GPU/event waits 有配置上限，不以 `INFINITE` 等待掩盖故障。

相机与 selection 使用 double 准备，再检查最终 float 与 shader clip 乘加界。不能形成可能吞掉方向的 `eye + forward`；mesh 先算 `view * model`，再乘 projection，以保留匹配远平移抵消后的 near-plane 项。无法表示时在 Execute 前具名 `Unsupported`，不是把 NaN/Inf 交给 shader。

### 颜色、采样与截图

RH、CCW 场景绕序保持不变；D3D 使用 RH_ZO 深度范围，不在 shader 额外翻 Y。mesh pass 开启背面剔除、LESS depth、depth write、无 blending；selection 是无剔除线列表、24 顶点，AABB 中心膨胀 1.004，仍参与深度测试/写入。

TextureArray 使用 point filter、UV wrap，无 mip 链。顶点 layer 整数并不禁止三角形插值后产生分数，由数组采样规则选择层；像素 alpha < 0.01 discard。Linear SRV 保留值，SRGB SRV 做 native decode；输出 R8G8B8A8_UNORM，不额外 sRGB encode。不能把色彩空间字段或 actual_samples 自报当作像素正确性证据。

Screenshot 是实际 extent 的 top-left 紧密 RGBA8 自有 vector；在 resolve 后 copy，有限等待后逐行去除原生 RowPitch。截图准备与等待不算 GPU draw 时间。旧验证中的 RGB 容差比较、透明 discard 和选择框“像素有变化”不能扩张为逐字节 RGBA、alpha 或精确线色已经验证。

### 原生故障与关闭

不可回滚阶段后的异常报告 fatal，禁止继续提交。显式 Shutdown 为已有 queued work 建覆盖 fence并有界等待，不执行新绘制；只有完成证明或设备移除证明才能正常释放原生资源。没有证明时，将所有 native 拥有者转入预分配终止 quarantine，保留至进程退出并记录诊断；这不是正常回收、恢复或关闭成功，重复显式关闭仍不能伪装成功。部分构造失败与 noexcept 析构同样处理已获得资源。

显式诊断工厂另有窄私有 `D3D12DiagnosticAccess`，不进入公开三头、普通工厂或游戏参数。探针与真实后端共同持有同一设备/队列/完成 fence 的自有 COM 引用，须覆盖后端关闭；不是业务扩展点或设备恢复机制。两个一次性 HRESULT 注入点分别在 mesh 资源接受前及实际 Execute 后的 draw Signal 前，普通游戏不创建控制对象。注入与真正原生调用失败分列，不宣称耗尽真实显存或发生硬件事故。
受控原生探针可移除自己创建设备，或在自己队列放置 fence gate，使真实 texture copy/retirement signal 在途等待；500 ms 原生等待和 30 秒外层进程上限分别约束。gate 在正常/异常离开作用域时解除；无完成证明的 Shutdown 仍 quarantine，之后确认自有工作完成也不把先前失败改判正常回收。不使用 busy shader、全局 TDR 或系统设置修改。

| 保留编号 | 当前约束 |
| --- | --- |
| D-I1/D-I2 | HWND 借用；native 状态私有且 Info 来自实际设备/能力/交换链 |
| D-I3 | samples、tearing、validation、query 能力按实际探测，不 fallback 或伪填 |
| D-I4/D-I5 | 成功才发布新资源；在途 frame refs 保留被替换/销毁的原生版本 |
| D-I6 | slot 先退休再复用，所有等待有限 |
| D-I7/D-I8 | 唯一 Present；不可回滚阶段失败停止续交 |
| D-I9/D-I10 | query 保存精确 frame ID，完成后才读；仅计 mesh/selection draw |
| D-I11/D-I12 | 自有 top-left RGBA8 截图；零 extent 不建零尺寸交换链，恢复接受真实像素 extent |
| D-I13 | Linear/SRGB 输入采样语义与 UNORM 输出边界不能混淆 |
| D-I14 | 部分初始化和关闭失败仍清理；无完成/移除证明则终止 quarantine |
| D-I15 | double 准备后验证 GPU float/clip 算术，Execute 前拒绝无法表示输入 |

依据：[D3D12 实现](../../game/modules/renderer/src/backend/d3d12/d3d12_backend.cpp)、[管线与布局](../../game/modules/renderer/src/backend/d3d12/pipeline.cpp)、[block shader](../../assets/shaders/hlsl/blocks.hlsl)、[selection shader](../../assets/shaders/hlsl/selection.hlsl)。

## GPU 样本与统计语义

现代 `PollGpuSamples` 不阻塞，只读取对应 fence 完成的 query；保存实际 frame ID，允许 ID 有间隔，不能按 slot 或提交序号猜。valid 必须有非负可选时长且 reason=None；missing 必须为空并给出 Unsupported/Discarded/DeviceFailure。DeviceFailure 诊断样本可返回，同时门面进入 Failed。真实零时长合法，缺失不能填零。

D3D12 pending slots 为三组，ready ring 为 256，满时诊断并丢最旧样本；消费者必须及时 Poll，普通游戏无 collector 时应用也排空 ring。query 仅包围 mesh/selection draw，不包含 clear、resolve、截图 copy/等待或 Present。draw_calls 不是 fragment 工作量，全剔除或极短 draw 可测得 0。

CPU prepare 是门面校验/自有帧准备加原生准备（含 slot 等待），不含 World cache 上传或 simulation；submit 与 present-wait 分开，后者包含 frame-latency wait 和 Present 调用。`FrameStatistics::upload_bytes` 只计该帧准备工作，例如 selection staging；持久 mesh/texture Create/Update 上传由应用单独计成功字节，不追记到下一帧。

应用的现代 atlas/mesh 转换属于 app，不隐藏在 Renderer 内：真实游戏 atlas 固定 512×512、64×64 tile，每 layer 内 top-left rows，tile 行号底行优先；RGB 补 alpha=255。World UV 的 `v=1` 在顶端，复制到现代 mesh 时只翻一次 `v=1-v`。World/Chunk/revision cache 同步借用回调时立即复制，成功 Create/Update 后才推进接受 revision，不把 frame draw span 当持久 World view。

依据：[应用转换](../../game/app/src/render_conversion.cpp)、[世界缓存](../../game/app/src/world_render_cache.h)、[采样和唯一帧适配](../../game/app/src/game_renderer.cpp)。

## Legacy OpenGL 机制

旧 namespace 状态只支持当前上下文主线程使用；与现代 owner-thread runtime guard 不同，旧入口没有完整并发、重入、多实例或跨上下文保护，不将调用者前提写成已实现状态机。

正常顺序：Window/context → AttachContext/GLAD/设备检查 → Init/LoadTextureAtlas → 每帧 AppendMesh/SetSelection/Render → Present。Render 不保存 Camera、Window、World 或 Registry 指针，Present 只临时借用 Window。退出先结束 timer 使用并销毁 query，再 Free texture/program/buffer，最后销毁上下文；初始化部分失败也由 app 分段清理。

| 原编号 | 当前 GL 约束 |
| --- | --- |
| I1 | AttachContext 至 Free 的 GPU 操作都要求当前上下文主线程 |
| I2 | Batch 长度不超过固定容量，追加先检查余量，避免加法溢出 |
| I3 | AppendMesh 当场复制，不跨返回保存 span |
| I4 | 正常 timer 顺序下 pending 槽不重用，未就绪不读结果 |
| I5 | texture/program/buffer/query 先于上下文销毁释放/归零 |
| I6 | namespace 不保存业务对象或窗口指针，输入只临时借用 |

### Batch 与整帧失败

chunk batch 容量 10,000,000 顶点，line batch 100；固定 VBO 容量不等于 CPU vector 没有分配成本，也不代表实际驻留显存。Append 验证初始化、余量与非空指针后复制；SetSelection 逐顶点追加固定 24 顶点，不自动清旧线，失败可能留下部分线。每帧 World 网格重新打包并整批上传；dirty 非空批次上传后 Draw，成功才 Clear。

仅 VAO/VBO 均未创建时可改容量；容量必须为正，顶点数与字节数分别可由 GLsizei/GLsizeiptr 表示。stride、分配及上传使用实际 `sizeof(T)`，不假定类型无填充。

Stats/timer 只借用至 Draw 返回。Draw 或 timer 异常可能发生在已有上传/绘制/统计之后，没有整帧回滚或绑定恢复保证。Clear 保留 CPU capacity，Free 释放 CPU/GPU 状态；Batch 不可复制/移动，所有释放仍需有效 context。保留的 indirect command 声明不是活动间接绘制路径。

### Shader 与纹理所有权

Shader/ShaderProgram 不可复制或移动且没有自动 GPU 释放析构，必须显式 Destroy。Shader 读文件/类型校验失败保留旧 ID，替换后编译失败删除新 ID；Program 候选编译/链接失败保留旧 program。但 namespace ReloadShaders 先销毁两旧 program，后续失败可只重建一部分，不能承诺旧 shader fallback。

Uniform cache 是文件静态状态，按 name+program ID，缺失 location=-1 也缓存；删除非零 program 清全局 cache，不是逐实例/逐上下文隔离。上传不隐式 Bind，名称、类型、长度与指针范围由调用方保证；不统一转换所有 GL 错误。

Texture/TextureArray 具体值拥有 GL ID，禁止复制，移动转移 ID；Texture 析构自动 Destroy，但非 virtual，不能通过 Texture* 多态 delete 派生对象。Destroy 只清 ID，不清路径/尺寸或派生 layer 数；moved-from TextureArray 的 layer 数也不保证归零。创建方法返回新值而非修改 this，局部失败由对象析构释放新 ID，不保证恢复所有 GL 全局状态。

普通纹理解码翻转，仅接受 RGB/RGBA、正尺寸和设备上限；上传期间设置 unpack alignment=1 后恢复，pixelated 决定 nearest/linear。Atlas 固定 64×64 tile，源宽高是 64 倍数，层数在 GL/uint16 上限内；通过临时二维 GPU atlas 逐层 copy，不是额外 CPU 重排。CubeMap 只有保留声明，未实现。选择图层的方块配置校验属于 app/world，不属于纹理资源对象。

### Legacy GpuTimer 与诊断

Debug 构建在 callback 可用时启用同步 GL 诊断，屏蔽 notification；回调只写 stderr，不让 C++ 异常穿过 C 回调边界，Free 在上下文仍有效时注销。零诊断文本只是探针的观测结果，不证明 Release 启用了同一回调或所有驱动错误均可发现。

app 持有 GpuTimer，Render/Batch 仅借用；对象不可复制/移动。64 slots，每槽最多两段 GL_TIME_ELAPSED（chunk/line）；顺序由调用者保证：BeginFrame → BeginDraw/EndDraw → EndFrame → 后续 Poll。没有完整顺序保护，第三段抛 logic_error；不支持或槽全满丢新测量、不等待、不覆盖旧 frame。

Poll 等本槽所有段 ready 才求和，返回自有结果并回收槽，按槽扫描而非全局 frame 排序；frame 是 Session 样本索引，不是现代 frame ID。零段产生真实 0ms，与缺失不同。计时只包围 DrawArrays，不含显式上传、clear、swap 或 CPU 打包；退出前一次非等待 Poll 不保证每个样本有 GPU 值，析构不回填 Session。Poll 分配失败可已回收早先槽，不提供整次事务；析构仍 active 时结束 query 并删除 ID，必须早于 context 销毁。

Device 返回缓存的借用 const 引用，不跨上下文重建保存；AllocatedBufferBytes 仅已初始化 VBO 容量。NVX optional KiB 是设备级驱动估计，不是本进程 VRAM，缺失或负值不补零。GL framebuffer capture 读 GL_BACK RGB，设置 pack alignment=1 而不恢复；交给 Assets::WritePng 临时逐行翻转输出，不是写图全局翻转开关，不承诺无部分文件或现代 RGBA8 契约。

依据：[旧 namespace](../../game/modules/renderer/src/legacy_renderer.cpp)、[Batch](../../game/modules/renderer/src/batch.hpp)、[Shader/Program](../../game/modules/renderer/src/shader_program.cpp)、[Texture](../../game/modules/renderer/src/texture.cpp)、[GpuTimer](../../game/modules/renderer/src/gpu_timer.cpp)、[诊断采样](../../game/modules/renderer/src/performance_memory.cpp)。

## 验证边界

- [Facade tests](../../test/unit/renderer/facade_tests.cpp) 使用私有 recording backend 覆盖句柄、自有化、接受事务、帧校验、线程/重入、状态、结果和样本一致性；不是原生 GPU retirement 或设备故障验证。
- [GL shader](../../test/unit/renderer/shader_loading_tests.cpp)、[Batch](../../test/unit/renderer/batch_safety_tests.cpp)、[GpuTimer](../../test/unit/renderer/gpu_timer_tests.cpp) 使用 GL 替身覆盖限定边界，不能证明所有真实驱动失败、错误顺序、OOM 或长期泄漏已处理。
- [D3D12 smoke](../../test/integration/renderer/d3d12_backend_smoke.cpp) 提供像素、采样、MSAA、截图与窗口场景的核查入口；历史成绩及其限制只以 verification 对应版本证据为准，不以本文或测试代码存在认定本轮通过。
- [受控原生故障](../../test/integration/renderer/d3d12_fault_probe.cpp) 链接同一正式 RendererState/D3D12 库，显式执行而不注册无人值守 CTest；[有界入口](../../test/integration/renderer/verify-d3d12-faults.py) 核对完成矩阵、源码、库、shader 和探针身份。受控设备移除、真正在途等待、Signal/分配注入分别留证，不覆盖整个游戏/public wrapper 或自发硬件事故。
- 长期不 Poll 无损、长期资源泄漏、任意驱动/真实内存耗尽及人工玩法/R3 整组验收不能宣布通过；当前具体运行及失败只从 VERIFICATION 定位，不靠源码里的有限等待与 quarantine 推定。

历史迁移的源码审阅不构成新运行；后续局部实现/运行也不产生人工批准。历史 T0/R1/R2 记录保留点时含义；需要改变冻结接口、默认后端或故障语义时先走现行 Spec 与关键节点授权。
