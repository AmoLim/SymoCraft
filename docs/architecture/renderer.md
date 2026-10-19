---
tags:
  - area/architecture
---

# Renderer

2026-10-17当前状态：本地feature-refactoring已同步通过本机R4/T3验收的真实Vulkan实现及双现代优化，D3D12默认、Vulkan显式编入/选择、GL保留。下文旧“候选/尚未验收/仅E盘”说明保留原实现时点，不再表示当前本地分支未交付；当前批准及保留风险见[T3完成对账](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#2026-10-17-r4及t3完成确认)，其他worktree不由此自动合并。

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

2026-10-16 refactor-renderer工作目录将共享层号校验改为接受内容摘要：ValidateMesh在既有完整遍历中返回optional最大层号，空网格为nullopt；Create候选及Update局部值保留摘要，后端接受成功后与CPU快照无新分配地提交到registry。销毁/清理同时重置摘要，Render每draw仍查当前句柄/绑定纹理，比较最大层号是否小于层数，不重扫顶点，不改变错误/重试或GPU旧版本退休责任。公开v1与legacy GL不变；这是实现机制说明，回滚/空网格/复用等边界未在本轮执行，基本测试和限制见[T3局部交付](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#2026-10-16-共享纹理层摘要局部优化)。主目录同步说明不等于优化源码已同步主目录。

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

2026-10-16 refactor-renderer工作目录将D3D12 SRV改为帧槽私有持久堆：旧slot fence完成后复用堆和按draw位置缓存的完整纹理BackendResourceId，只重写身份变化/未初始化项。堆容量按需倍增且不超过原Tier1上限；先分配空metadata及新堆，再无分配交换并重建必要SRV，失败前保留原堆/键。键不持有原生texture，GPU寿命仍依赖frame refs；当前v1无texture update，未来同身份view变化须另定失效。Resize尺寸资源释放及正常关闭清缓存，quarantine随完整FrameContext保留；root constants/静态sampler/shader和有界等待不变。[局部实现与未测边界](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#2026-10-16-d3d12帧槽srv缓存局部实现)记录首次CPU基本及Release编译依据；后续[真实GPU正常路径](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#2026-10-16-d3d12-srv缓存正常gpu验证)覆盖三slot复用、纹理切换与draw重排，不代表受控真正在途、扩容/故障边界或性能已验证。私有diagnostic observer按真实frame-slot记录成功原生建堆/写SRV次数及容量，并记录最近frame id；仅绑定观测时更新，不引入资源所有权、故障门或计时，区别于phase访问计数。

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

普通GL与现代后端对照不绑定私有细分计时sink或原生诊断observer，保留正常telemetry及各自验证配置；采集统计函数的显式warmup/sample参数只选择原始数据窗口，默认10＋50秒和nearest-rank/两项内存观测最大值算法不变。用户另行选择的30＋150秒单重复仅作中等长度观察，实际[D3D12](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#2026-10-16-gl与d3d12三分钟中等长度对照)与[Vulkan](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#2026-10-16-gl与vulkan三分钟中等长度对照)分别绑定各自同版本GL基线，不跨轮借用或替代三重复正式验收；采集有效、数值满足与节点批准分开，不由执行器成功状态自动推定预算通过。

### D3D12私有性能诊断

D3D12隔离性能probe沿同一RunGame和私有创建线程sink采集，不新增公开v1/普通CLI/正式CSV或优化路径。Render阶段互斥分解SRV堆销毁/创建/写入、selection原生分配/Map写入、命令与同步；共享层号校验按draw累计为facade_prepare嵌套子项：当前计摘要比较，旧版本计逐顶点扫描。UpdateMesh在Render之前记录校验、自有复制、边界、原生分配、Map复制与整体，累计一次归到下一原frame ID；初始Create单列，不能把启动算编辑或把嵌套项加到父项外。未绑定不读新增时钟/分配诊断记录，原root constants、UPLOAD资源版本、原生操作顺序、fence/有界等待及校验保持。记录预分配有界、结束后落盘，额外原生诊断计数及粗粒度线程CPU的限制由[T3结果](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#2026-10-16-d3d12细分计时与短诊断)维护；[计时入口](E:/Codex/Worktrees/refactor-renderer/Symocraft/game/modules/renderer/src/frame_diagnostics.h)位于refactor-renderer工作目录，主目录同步说明不等于源码同步。

2026-10-16两现代私有probe输出cached_max_v1层校验模型及layer_summary_check墙钟/调用数；保留旧扫描枚举/字段为明确退役的零值，不将其解释为未测或独立收益。新解析器校验摘要调用数对应mesh draw、无顶点扫描并禁止混用模型；旧D3D12仍按逐顶点模型解释，旧Vulkan无该独立项则保持null。摘要计时为父阶段子项，不与facade总耗时相加；每draw时钟本身有扰动，普通未绑定游戏不读取这些新增时钟。[前后短诊断及适用边界](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#2026-10-16-缓存摘要前后短诊断)不替代正式预算/配对或原失败处置。

2026-10-16 D3D12私有计时probe另输出frame_slot_delta_v1原生SRV观测：每原frame ID记录三个slot累计成功建堆/写SRV次数、当前slot及容量，解析器先按连续原始行差分再选采样窗口，不把暖槽建堆算入稳态。旧无观测CSV标unmeasured_v1/null，不由phase visits补原生计数。descriptor_retire/allocate/update仍为互斥host阶段，缓存命中时包含边界/容量判断/身份比较而非原生销毁或创建；前后可比较阶段总成本，但不是单条API时间。普通CSV/v1和生产算法不改，额外记录容量和限制见[SRV短诊断](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#2026-10-16-d3d12-srv缓存前后短诊断)。

### Vulkan工作目录候选机制

Vulkan专用性能probe另启用observe_performance，普通及其他未启用绑定不读该层时钟或做额外GPU查询。native_calls_v1按真实API正常返回/VkResult成功累计次数与嵌套host墙钟，Buffer按mesh/mesh常量/选框常量/选框顶点/staging/readback及实际memoryType分组记录requirements兼容位、请求/分配字节和绑定/释放；观察层仅记录实际选择、不干预分配策略。每帧采录累计值，先对连续原frame ID快照差分，再取采样窗口，首行累计值不冒充一帧操作；pre-Render的Poll/Query也落在两快照之间，不误称全在Render prepare内。逻辑phase visits不补原生次数，旧CSV无模型则unmeasured/null。

启用者在QueuePresent前后额外非阻塞读取当前提交fence状态，只记录SUCCESS/NOT_READY或原始错误，不改变完成判定、退休或添加等待；状态前后变化不是连续完成时间。实际VkPhysicalDeviceMemoryProperties、queue family/是否同queue、图像数和时间戳位宽/周期另记录，软件pending数不当成硬件队列深度，内存类型也不当作WDDM驻留/带宽跟踪。宿主区间包含验证/驱动/调度，调用次数中的void正常返回不代表GPU已完成；这些诊断开销与原固定容量记录器单列，[结果与未定位边界](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#2026-10-16-vulkan实际内存与等待原生诊断)不替代无探针A/B或正式预算。

Vulkan私有诊断绑定另提供每slot独立mesh/selection原生计数：仅在MakeBuffer、CreateDescriptorPool、AllocateDescriptorSets成功后累计，vkUpdateDescriptorSets按实际调用及UBO/image/sampler写分别累计，实际uniform写入/字节与pool释放单列。bundle仅借用同一Native所持诊断对象中的稳定计数位置，不延长资源或GPU寿命；未绑定不建立记录。正常帧的ResourceRetire不销毁保留的bundle，UniformAllocate/DescriptorAllocate的phase访问可能只有容量判断而无原生创建，不能再以visits证明分配；旧性能probe需重建并适配这些计数后才可采样。[正常GPU验证与局限](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#2026-10-16-vulkan帧槽绑定正常gpu验证)包括同步截图的扰动，不等于受控真正在途或性能证据。

2026-10-16 Vulkan工作目录按帧槽保留常量与描述符bundle：Finish证明旧提交完成后先reset命令池，再清旧frame资源引用；正常帧只退休mesh/texture refs及readback，选框顶点按下述独立机制保留，mesh/选框常量各有独立的UBO/pool/sets/CPU打包存储与metadata。mesh容量按需增长、选框单独一项，避免draw数量改变时混用绑定位置；原输入上限保持。候选资源全部分配完成才交换，Buffer/pool替换同时换空metadata。UBO内容仍逐帧按原stride写入并保持flush，绑定0/2只在bundle初始化时写，绑定1按完整纹理ID差量写；缓存不持有texture所有者，不以裸原生句柄作为版本键。Rebuild在原Drain后reset命令再清bundle，正常析构池早于Buffer释放，未证完成仍完整Native保留。常量/描述符复用不改公开v1/shader/能力基线/WSI；[局部结果与未测风险](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#2026-10-16-vulkan帧槽常量与描述符局部复用)不代表GPU寿命或性能通过。

2026-10-17网格Buffer仅在兼容requirements_bits内优先HOST_VISIBLE|DEVICE_LOCAL，无兼容类型时保留原HOST_VISIBLE路径；其余角色不改。成功绑定按实际类型一次性记录policy/type/flags/heap/fallback，原生分配失败不换类型重试，Map/flush/Unmap及网格新版本接受事务保持。选框顶点由各frame slot独立懒建288字节Buffer，Finish及命令池reset后重写；首建写成功才发布，隐藏保留但不写/不画。RetireFrame不释放选框，DropFrame在正常退休/重建后释放；不新增持久映射，未证完成仍完整Native终止保留。SelectionAllocate暖槽后可只有命中判断，原生计数仍以实际调用为准。[基本结果](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#2026-10-17-vulkan网格内存与选框局部优化)与[追加正常GPU检查](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#2026-10-17-vulkan网格与选框正常gpu验证)分列；后者通过实际内存类型、分配次数及画面核对，但同步截图不证明受控未完成slot复用或性能收益，原执行器非阻塞查询计数误判及逐次返回缺失保留。

Vulkan性能细分仅由隔离probe安装创建线程私有计时sink，公开v1与普通游戏CLI/正式CSV不变。门面先创建同frame ID记录，后端在19个具名阶段切换互斥墙钟/线程CPU区间，重复阶段累加；未绑定不读取新增时钟或创建诊断记录。probe预分配有界内存、结束后写独立CSV，缺失CPU读取留空，未完成/截图单列；保留普通2500ms等待及validation语义，不注入故障或额外GPU能力。slot阶段含查询，DropFrame阶段含descriptor pool销毁，原生调用阶段含验证/驱动成本，不能过度解读为单条API或精确阻塞时间。CPU计数粒度及本轮证据上限见[T3短诊断](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#2026-10-16-vulkan细分计时与短诊断)。 源码位于refactor-renderer工作目录的[私有计时入口](E:/Codex/Worktrees/refactor-renderer/Symocraft/game/modules/renderer/src/frame_diagnostics.h)，主目录只同步说明，不声称其源码已同步。

Vulkan能力与WSI诊断仍只由私有单次绑定启用：在真实扩展/feature枚举后cap-off进入同一SelectDevice拒绝路径，不尝试软件或降低能力。Acquire OutOfDate注入在API调用前发生，保持未取得图像/未signal同步对象的语义；Suboptimal及Present异常在原生成功后覆盖有效结果，原始返回与有效值分开。实际调用/提交/呈现与交换链创建计数用于验证平衡，未调用API报告null而非上一帧返回；普通游戏不安装这些控制。[WSI probe](E:/Codex/Worktrees/refactor-renderer/Symocraft/test/integration/renderer/vulkan_wsi_probe.cpp)通过公开Renderer检查重复重建、恢复像素和正常退休；实际结果见[T3补齐记录](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#2026-10-15-vulkan能力与wsi生成身份及r4材料补齐)。

两现代构建均生成所选shader目录的build.json，记录实际DXC exe/module、HLSL/include及四个二进制hash、配置/flags；DXIL为D3D12/d3d12/6_0，SPIR-V为Vulkan/vulkan1.2/6_0。编译器输入变化触发构建/清单更新，安装只带预编译shader与清单，不带DXC。GameRenderer将所选清单写入中立报告；Benchmark新运行和resume绑定安装二进制/清单及报告身份，导出保留该元数据。历史D3D12 telemetry-v1无生成元数据仍可单独读取，不赋予新包运行资格；GL旧格式保持。实现见[编译身份](E:/Codex/Worktrees/refactor-renderer/Symocraft/cmake/WriteVulkanShaderIdentity.cmake)、[消费者](E:/Codex/Worktrees/refactor-renderer/Symocraft/tools/benchmark/src/runner.cpp)，这是可复算构建/包关联，不是签名或性能结论。GUI是否实际核查仅从上述验证记录判断。

异步诊断沿同一私有绑定增加实际Wait/Query计数；真实未完成提交的Poll只查fence，不执行等待或读尚未就绪query。256条ring按来源frame ID交付，不保证返回顺序；溢出具名丢弃而非补0。原生零extent产生Discarded、缺时间戳能力产生Unsupported且无耗时；私有cap-off明确是注入，不将支持时间戳的本机说成不支持。查询故障保留原生操作/码并进入终止保留，不返回伪有效0ms或继续提交；在实际成功query之后的一次返回值注入不等于自发DeviceLost。对应[异步与协议验证](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#2026-10-14-r4第4批异步样本协议与部署验证)分列正常0/0诊断与附加观测层通知。

Vulkan候选的受控验证入口为私有、创建线程作用域的一次性绑定，链接同一生产库；不由环境变量、公开v1或游戏CLI启用。仅门控诊断设备查询/启用timelineSemaphore，在真实提交中加入host可释放wait，保留原有提交fence和资源版本。因未满足的host timeline依赖不能进入WSI Present，门控绘制明确Skipped/Occluded并保留取得图像，仅正常门控时host释放/提交完成后才在创建线程经私有挂起点呈现；故障终止保留owner禁止补Present，取得图像/同步资源仍保留，gate清理不续交；不伪称Presented或把该路径当正常WSI。纹理copy门控沿生产同步接受/有限等待，失败仍完整终止保留。分配/提交/Present结果注入分别具名；实际Present结果与注入分开。诊断弱引用、实际分配/释放计数和loader映射查询不延长资源完成语义，不将终止保留后gate完成改成恢复。普通未绑定设备不启用额外feature/门控分支；[本批结果与失败边界](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#2026-10-14-r4第3批在途资源寿命与故障验证)不扩为长期泄漏或自发故障保证。

工作目录候选的尺寸竞态处理：Vulkan surface与SDL请求尺寸未一致时，私有Rebuild在任何旧代退休之前抛出带surface_extent_pending的InvalidState，保留最后已接受的Info和原生所有者；不以请求尺寸刷新实际Info。GameRenderer只识别此精确Resize诊断，单次WaitEvents(.01)后应用层Skipped/Occluded，后续窗口poll再试；该应用层skip未进入Renderer，不伪造原生GPU样本。Render内两次surface查询之间若再变化，也仅Occluded/Discarded，不续交错误尺寸或吞其他InvalidState。匹配后才正常有限退休/重建。候选D3D12仅在真实HWND尺寸于SDL poll后漂移时提交前Occluded/Discarded，缺swapchain或内部extent不一致仍保留错误。实际覆盖和未测分支见[第2批验证](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#2026-10-14-r4第2批生产绘制与窗口验证)，不据机制宣称故障/人工通过。

本节从E盘托管worktree同步供审核，源码未复制到F盘。2026-10-14实现位于refactor-renderer worktree，当前机制不等于R4接管批准。私有Native owner集中持有实际SDL loader入口所在模块的额外引用、instance/surface/device/graphics及present队列、资源版本、三组frame context及交换链图像资源，不链接vulkan-1导入库。普通关闭先取得提交和呈现fence的有限完成证明，再销毁完整owner，最后释放loader引用；缺完成证明时完整owner终止保留，不再调用借用Window，不将保留称成功Shutdown或恢复。

按[已批准能力](../openspec/changes/archive/2026-10-19-m3-t3-renderer/decisions.md#2026-10-14-vulkan有限呈现退休能力)启用KHR/EXT维护扩展与feature。提交fence保护command pool、描述符、常量/选择框/readback及mesh/texture版本；逐交换链图像的presentation fence保护呈现wait semaphore退休。只在资源复用、同步纹理接受、截图、重建/关闭时必要有限等待；Poll只查询已就绪结果，256条有界ring保留来源frame ID，溢出具名诊断。

mesh Create/Update建立新host-visible原生buffer版本，成功后交换registry，不原地覆盖在途内容；空mesh无原生几何。纹理使用device-local二维数组、staging/copy/layout barrier及提交完成后接受，Linear/SRGB选原生格式；非coherent host memory显式flush/invalidate。帧常量按真实uniform alignment打包，上传统计包含实际padding字节；截图使用transfer-to-host可见性barrier、有限fence等待及自有top-left RGBA8，BGRA只交换通道。

GPU布局仍显式28字节mesh/12字节selection和64字节矩阵，位置为int3；RH_ZO投影、负高度viewport单次Y变换、CCW/back-cull、point/wrap与同源alpha discard保持。1x直接颜色附件、MSAA以对应color/depth附件及单采样resolve呈现；深度复用依赖覆盖early/late fragment tests，不以UNDEFINED layout免除写后写同步。selection沿D3D12的LESS/写深度语义。Resize先核对surface实际尺寸与请求模式/格式/采样，退休并丢弃旧尺寸帧引用后重建，持久mesh/texture保留；旧链退休后构建失败fatal，不续用旧Ready假设。OutOfDate/Suboptimal标记后续重建，原生零extent跳过并更新Info；app清除旧extent缓存使恢复可重新Resize。真实并发尺寸压力/错误分支仍需独立验证。

依据：[实现](E:/Codex/Worktrees/refactor-renderer/Symocraft/game/modules/renderer/src/backend/vulkan/vulkan_backend.cpp)、[生产v1 smoke](E:/Codex/Worktrees/refactor-renderer/Symocraft/test/integration/renderer/vulkan_backend_smoke.cpp)；实际结果和限制唯一见[T3验证](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#2026-10-14-r4生产vulkan实现)。实验代码不被生产include或链接，v1公开头未改，不预建通用RHI。

D3D12私有寿命/异步观测沿同一单次绑定增加弱引用、成功mesh buffer/接受texture的创建释放及GetResourceAllocationInfo分配计数、实际Wait/query Map和InfoQueue检查计数；不由弱引用延长版本寿命，计数排除临时上传/帧附件/query以及进程内存或显存预算。普通游戏未安装控制时不记这些计数；原有256条样本队列满时另向stderr记录被丢弃的精确uint64 frame ID，不改有界丢弃策略。[寿命/异步probe](E:/Codex/Worktrees/refactor-renderer/Symocraft/test/integration/renderer/d3d12_lifetime_async_probe.cpp)使用同一公开Renderer和普通Render/Present，以自有queue Wait/fence门控真实在途提交；harness的限时watchdog只释放自有gate，线程创建失败也先释放gate。它不是公开同步接口、专用Present旁路或性能基准。native_cleared只表示后端成员清空，诊断控制的COM引用随Scene销毁且先于Window释放；实际结果及计数边界见[T3验证](../openspec/changes/archive/2026-10-19-m3-t3-renderer/verification.md#2026-10-15-d3d12在途寿命与异步容量补证)。

- [Facade tests](../../test/unit/renderer/facade_tests.cpp) 使用私有 recording backend 覆盖句柄、自有化、接受事务、帧校验、线程/重入、状态、结果和样本一致性；不是原生 GPU retirement 或设备故障验证。
- [GL shader](../../test/unit/renderer/shader_loading_tests.cpp)、[Batch](../../test/unit/renderer/batch_safety_tests.cpp)、[GpuTimer](../../test/unit/renderer/gpu_timer_tests.cpp) 使用 GL 替身覆盖限定边界，不能证明所有真实驱动失败、错误顺序、OOM 或长期泄漏已处理。
- [D3D12 smoke](../../test/integration/renderer/d3d12_backend_smoke.cpp) 提供像素、采样、MSAA、截图与窗口场景的核查入口；历史成绩及其限制只以 verification 对应版本证据为准，不以本文或测试代码存在认定本轮通过。
- [受控原生故障](../../test/integration/renderer/d3d12_fault_probe.cpp) 链接同一正式 RendererState/D3D12 库，显式执行而不注册无人值守 CTest；[有界入口](../../test/integration/renderer/verify-d3d12-faults.py) 核对完成矩阵、源码、库、shader 和探针身份。受控设备移除、真正在途等待、Signal/分配注入分别留证，不覆盖整个游戏/public wrapper 或自发硬件事故。
- 长期不 Poll 无损、长期资源泄漏、任意驱动/真实内存耗尽及人工玩法/R3 整组验收不能宣布通过；当前具体运行及失败只从 VERIFICATION 定位，不靠源码里的有限等待与 quarantine 推定。

历史迁移的源码审阅不构成新运行；后续局部实现/运行也不产生人工批准。历史 T0/R1/R2 记录保留点时含义；需要改变冻结接口、默认后端或故障语义时先走现行 Spec 与关键节点授权。
