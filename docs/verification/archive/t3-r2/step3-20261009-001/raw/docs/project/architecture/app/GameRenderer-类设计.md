---
type: 类设计
status: R2顺序3已实施，最终验证以阶段报告为准
project: Symocraft
module: app
created: 2026-10-09
updated: 2026-10-09
tags:
  - area/architecture
  - topic/rendering
---

# GameRenderer 私有渲染转接

关联功能：[App 运行编排](App-运行编排-功能.md)。关联协作者：[WorldRenderCache](WorldRenderCache-类设计.md)、[Renderer/PImpl](../renderer/Renderer-PImpl-类设计.md)、[D3D12Backend](../renderer/D3D12Backend-私有类设计.md)。实施与最终成绩统一见 [R2 顺序3报告](../../../milestones/m3-t3/r2-step3-implementation.md)，本笔记不新增验收表。

## 当前设计

`SymoCraft::Application::Detail::GameRenderer` 是 `game/app/src` 内部的过渡适配类，不是 Renderer v1 的新增公开 API。`Application` 仍只有一份输入、物理、交互、编辑与采样循环；本类仅隔离现代门面与旧 GL 的渲染差异。它不能复制或移动，持有 `unique_ptr<Impl>`，不持有 World、Camera 或 Session。

默认仍为 OpenGL。启动参数先由 `SelectWindowMode` 检查所选后端是否已实施并编入，再创建匹配 Window；D3D12 使用 Native 模式，不建 GL context。未编入的 D3D12 和未交付的 Vulkan 具名 Unsupported，不回退其他后端。现行游戏 target 仍是保留旧 GL 的过渡组合，不据此宣称已交付无 GL 依赖的单现代发布包。

源码：[声明](../../../../game/app/src/game_renderer.h)、[实现](../../../../game/app/src/game_renderer.cpp)、[应用循环](../../../../game/app/src/application.cpp)、[CPU 转换](../../../../game/app/src/render_conversion.cpp)。生产 cpp 分别归实际 app 可执行目标和 `symocraft_app_render_data`，不是测试 helper 依赖。

### 数据成员

| 类型 | `Impl` 成员 | 初值 / 范围 | 含义与所有权 |
| --- | --- | --- | --- |
| `Window&` | `window` | 构造时借用 | 不拥有窗口；整个图形清理期间必须存活 |
| `unique_ptr<Rendering::Renderer>` | `modern` | null；D3D12 构造后非空 | 独占真实生产门面；门面拥有后端和原生资源 |
| `ModernMeshPort` | `mesh_port` | `renderer=nullptr` | 仅转发 Create/Update/Destroy；原始指针借用 `modern`，不拥有门面 |
| `unique_ptr<WorldRenderCache<ModernMeshPort>>` | `cache` | null；现代构造后非空 | 独占 app 身份/发布版本到句柄的账本；借用 port，不保存 World 顶点 span |
| `TextureArrayHandle` | `atlas` | null handle | 现代图集的逻辑资源标识；原生所有权在门面，正常关闭时显式 Destroy |
| `MeshUpload` | `atlas_upload` | 字节/调用/CPU 时间为0 | 成功 CreateTextureArray 的输入字节、一次调用与创建 CPU 时间；不含图像解码时间 |
| `uint32_t` | `requested_samples` | 构造时复制 options | 保存请求值，区别于实际 `RendererInfo::actual_samples` |
| `vector<MeshDraw>` | `draws` | 空 | 自有绘制值列表；只含句柄、图集和 identity model，无 World 指针；Render 调用期提供 span |
| `Extent2D` | `extent` | 现代实际 framebuffer；其余零 | 最近成功接受的 Resize 尺寸，避免对相同尺寸重复调用 |
| `unique_ptr<GpuTimer>` | `legacy_timer` | null；仅旧 GL 采样创建 | 独占旧 GL 查询对象；须先于 GL context 释放 |
| `bool` | `legacy_started` | false | 旧 Init 获取清理责任的标记，不等同完整初始化成功 |
| `bool` | `stopped` | false | 首次 Shutdown 进入时置 true，避免二次执行清理；不是故障恢复状态机 |

`MeshUpload` 是自有值：`uint64_t bytes/calls` 与 `double cpu_ms`。`GameFrame` 是自有 `Rendering::FrameResult` 加渲染、上传和 present CPU 毫秒值；截图像素归 FrameResult 所有。

### 不变量

| 条件 | 成立边界 |
| --- | --- |
| 后端选择先于 Window 创建；一次运行只走所选渲染分支 | main / Init 的组合顺序，不支持热切换 |
| `window` 晚于本类、门面、查询和 cache 的实际清理 | Application::Free；异常也分段尝试 |
| port 指针指向当前 `modern`，cache 使用期间 port 和门面有效 | 现代构造成功至 cache.reset；不向底层传播 app 类型 |
| World/Camera/Session 仅调用期借用；持久缓存只记身份、句柄、已接受发布 revision | Synchronize / Render / 采样入口 |
| 现代路径只调用一次新 `Renderer::Render`，该调用唯一 Present | Render；app 不再对 Native Window 额外交换缓冲 |
| 旧 GL 仍由旧 API 逐帧 Append、Render 和 Present | 仅旧分支；没有把 legacy 实现伪装成新门面后端 |
| Shader、CPU 图集与网格转换不依赖 R1 实验代码 | 产品源与构建；公开 Renderer v1 不变 |

### 入口预期行为

下表均为 app 私有入口。资源错误、RenderError 与分配失败传播至应用退出处理，不作为普通方块编辑拒绝。

| 入口 | 处理与输出 | 前提 / 借用 | 失败后状态 |
| --- | --- | --- | --- |
| `GameRenderer(Window&, options, collect_samples)` | 现代创建实际 Renderer、port/cache、读取真实 info；旧分支 Init，采样时创建 GpuTimer | Window 已按 SelectWindowMode 创建；普通运行现代 VSync 开启，采样按 options | 现代成员 RAII 清理；旧分支捕获原异常并尝试 Free，保留原失败 |
| `~GameRenderer() noexcept` | 尝试 Shutdown，异常仅诊断，不向析构调用方抛出 | Window 尚未销毁 | 不能据析构无抛出声称实际设备故障已动态验证 |
| `Modern() const noexcept` | 返回 `modern` 是否非空 | 活对象；不是后端健康探测 | Shutdown reset 后返回 false；关闭后不再调用业务入口 |
| `LoadAtlas(path)` | 旧 API 加载；现代 DecodeImage(false) → ToRendererAtlas → CreateTextureArray，返回64层并记 atlas_upload | 现代图集只允许成功加载一次 | 重复现代加载 logic_error；未成功创建不发布 handle/统计 |
| `AtlasUpload() const noexcept` | 返回最近成功现代图集创建统计值 | 值查询；旧分支默认0 | 不重复向帧上传累计器追加 |
| `Synchronize(const VoxelWorld&)` | 现代按发布更新 cache，然后清空/重组 draws，返回接受的网格 bytes/calls 和本次 CPU 时间；旧分支 VisitMeshes→AppendMesh | 当前 World 访问只读，借用仅 callback 内；旧 pack 时间由应用外层测量 | 现代逐记录成功保留，失败记录不改已接受版本；失败后应用退出，不继续绘制半轮新列表 |
| `ReleaseWorld(identity)` | 现代先清 draws，再显式释放该 World 的 cache；旧分支无额外操作 | 正常 Run 末尾；仅身份值 | Destroy 失败传播；后续 Shutdown 仍尝试全部清理 |
| `Resize()` | 现代按 Window 实际像素/最小化状态得到 extent；变化时调用门面 Resize并更新记录 | 串行 Window 状态；零 extent 表示暂停 | 门面拒绝或故障时不更新缓存 extent；旧 viewport 在 Render 内处理 |
| `DescribeSession(Session&) const` | 旧写原 gl 字段；现代 SetRendererInfo 记录实际设备/driver/API/尺寸/格式/呈现/采样及能力，并写请求值与计时范围 | 有采样 Session；旧 timer 已创建 | 保留具名现代协议，不能向 `gl_*` 填 D3D12 数据 |
| `SampleDeviceMemory(Memory&) const` | 旧使用旧设备内存查询；现代没有此处支持的查询，保持 optional 缺失 | 内存值借用 | 不补0、不用进程内存冒充设备显存 |
| `PollGpu(Session&, excluded_frame)` | 旧维持原查询；现代非阻塞 Poll，按原始 frame ID 回填 duration 或具名缺失，忽略本次 excluded ID | Session 保存稳定采样 ID；不为每帧等待 GPU | 未就绪不是0耗时；调用异常传播 |
| `Render(frame_id, camera, selection, Session*, screenshot)` | 现代 Resize→真实 Camera 转换→draws→选择框→FrameInput→唯一 Render；普通无 Session 玩法先非阻塞清空样本环，截图诊断不清空待回填结果；旧 viewport/selection→Render→独立 Present | frame ID 由应用递增；Camera 调用期借用；选框为 floor(min)到min+1 | 现代 skip/统计原样返回；错误上抛，不回退 GL |
| `Capture(frame_id, camera, selection, path, Session&)` | 现代以独立额外 frame 请求 readback，使用 Assets::WritePng，排除该截图 frame 的 GPU 样本；旧继续旧 framebuffer 捕获 | 正式采样结束之外；截图 frame ID 不复用 | 无截图、过大 encoder 尺寸或写入错误传播；不是正式性能样本 |
| `Shutdown()` | 分段 ReleaseAll→DestroyTextureArray→Renderer::Shutdown→清 draws/cache→reset modern；旧释放 timer→Free | Application 先调用本入口，再 reset 适配器，最后销毁 Window | 尝试后续清理并重抛首故障；重复调用不重做清理，不承诺故障恢复 |

### 组合辅助函数

| 函数 | 实际责任 |
| --- | --- |
| `SelectWindowMode(options)` | OpenGL→OpenGL；仅编入 D3D12→Native；其余具名 Unsupported，无 fallback |
| `AttachRendererContext(window, options)` | 仅旧 GL 调用旧 AttachContext；Native 不附着 GL 上下文 |
| `CheckRendererAssets(options, errors)` | 先检查选择；旧原预检，现代检查配置/图集及4个非空生产 DXIL；只检查文件，不替代实际后端 PSO/纹理验证 |
| `ModernMeshPort` | 借用门面并逐项转发网格 Create/Update/Destroy；不是生产 recorder、RHI 或公开 Renderer 注入接口 |

### 转换、计时与退出

Camera 使用当前 getters，FOV 度转弧度，右手方向不变，near 0.1 / far 2000。CPU 转换不直接复制 GL projection；各后端根据中立参数生成适当深度区间。现代选择框接收同一交互结果，不复制玩法算法。

实际512×512图集保留64个64×64层，底行优先编号、层内 top-left RGBA8，自有像素，颜色空间为 Linear。World 顶点已经 world-space，model 恒等；app 自有化时只把 V 改为 `1-V` 一次，U/层/normal/原顶点顺序保持。后端 shader 不再额外翻转 V。旧 GL 图集与 UV 路径不改。

资源 Create/Update 的 accepted bytes 在 cache/atlas 侧计一次，帧 Render 统计另计 selection staging；两项分开记录后由应用累计，不能重复相加。cache CPU 时间包含自有化、Create/Update 和 draws 重组；现代 render CPU 取门面/后端 prepare 与 submit，present_wait 单列；GPU draw区间及排除项由实际 info/阶段报告维护。

正常 Run 显式 ReleaseWorld，异常展开也不会留下 World 顶点借用。Free 即便遇到资源销毁或门面 Shutdown 失败，仍先尝试并 reset 图形所有者，再释放 Camera/Registry、Window 和 SDL video；backend 的终端隔离/有限等待责任仍在 renderer 内部，不由 app 超时后强行回收。

## 验证边界

本轮 CPU Debug/Release 与原生 Debug/Release 已构建；准确测试、真实进程、截图、设备身份与失败记录以 [顺序3最终报告](../../../milestones/m3-t3/r2-step3-implementation.md) 为准，不在本类笔记改判为 GPU/完整玩法已通过。

顺序4的真实可运行包、生产候选交付，以及 R3 的适用 A01–A19、14项玩法与至少15分钟连续操作仍须独立完成；实际 DeviceLost、提交失败和真正在途等待超时也不能由源码审计或 CPU fake替代。R2实施不授权切默认或删除 GL，唯一人工核查入口仍为 [阶段人工记录](../../../milestones/m3-t3/manul-verification.md)。
