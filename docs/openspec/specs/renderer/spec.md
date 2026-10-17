---
tags:
  - area/spec
---

# Renderer

## Purpose

最小现役公开行为基线，覆盖当前游戏选择与现代 `Rendering::Renderer`，不是 T3 整体验收声明。
2026-10-10 迁移时已静态核对公开头、门面/registry/校验、D3D12 关键实现和 app 转接；未运行 C++、游戏或 GPU。
2026-10-12 用户批准 R3 并授权默认切换后，仅更新 Backend Selection 现役行为；定向构建/启动结果及既有未覆盖风险见 [T3 验证](../../changes/m3-t3-renderer/verification.md#默认切换验证)，不将其他目标整组同步为已验证。
内部正确性机制及 P-I/G-I/D-I/旧 GL I 编号由 [renderer 架构](../../../architecture/renderer.md) 维护。
历史结果、版本身份、查验包与可运行包的区别及批准只从 [VERIFICATION](../../../verification/VERIFICATION.md) 判读。
源码/测试存在不证明实际设备故障、长期回收、人工玩法或 R3/R4 通过；未纳入本基线的行为也不等于未实现。

## Requirements

### Requirement: Backend Selection and Availability
游戏 SHALL 在创建窗口前选择一个后端；当前默认 SHALL 为 D3D12，保留的 legacy OpenGL 4.6 SHALL 仅通过显式 `--renderer opengl` 进入同一游戏循环。未实现/未编入或能力不足的选择 SHALL 具名拒绝，不静默回退或使用软件设备冒充能力成立。

#### Scenario: Current game default and explicit transition
- **WHEN** 游戏未给出 `--renderer`，显式选择 `d3d12`，或显式选择已编入的 `opengl`
- **THEN** 前两者选择 Native-mode Window 与真实 D3D12，后者选择隔离的 legacy OpenGL；两默认配置均为 D3D12，不热切后端。

#### Scenario: Unavailable backend or capability
- **WHEN** 现代门面请求 OpenGL/Vulkan、请求未编入的 D3D12，或 D3D12 硬件不支持 FL12_0/SM6_0、请求采样或呈现能力
- **THEN** 请求具名失败，不改选其他后端/WARP，不静默降低能力或采样数。

#### Scenario: Selected asset preflight
- **WHEN** 执行选中后端的 `--check-assets`
- **THEN** 仅检查选中路径的必需文件：legacy OpenGL 检查 regular_file，D3D12 还拒绝零大小或不可访问文件；不初始化 GPU，也不将文件存在视为解码/shader/PSO/驱动验证。

#### Scenario: Uncompiled default and missing default assets
- **WHEN** 游戏省略 `--renderer` 但 D3D12 未编入，或默认 D3D12 所需 DXIL 缺失
- **THEN** 前者以 Unsupported 拒绝，后者资产预检失败并具名 DXIL；即使 GL 已编入且资产可用也不回退。新游戏构建首次默认编入 D3D12，已有 OFF 缓存不自动重写；CPU-only 与独立工具保持无原生后端。

### Requirement: Public Boundary and Call Discipline
现代 Renderer MUST 以中立 C++20 公开头提供资源/帧/结果/错误，不暴露图形 SDK 或私有后端扩展点；实例不可复制或移动。应用 SHALL 拥有且保持 Window 有效至 Renderer 关闭和销毁，所有显式入口 SHALL 在创建线程串行、非重入调用（R-I1、R-I2）。

#### Scenario: Public-only consumer
- **WHEN** 消费者只使用公开 renderer 头与 CPU 场景类型
- **THEN** 它不需要 GLAD/GLFW/SDL3/Windows/D3D12/Vulkan 私有头，不能从公开签名取得原生对象或实现后端。

#### Scenario: Wrong thread or reentry
- **WHEN** 非创建线程调用显式 API，或正在调用的 Renderer 被重入，包括 GetInfo/Shutdown
- **THEN** 在后端工作前以 WrongThread/ReentrantCall 拒绝，不建立并发访问保证。

### Requirement: Resource Acceptance and Identity
现代门面的 Create/Update SHALL 在返回前取得自有 CPU 数据，后端成功接受后才发布新句柄/内容；可恢复失败 SHALL 保留旧接受内容。类型分离的资源句柄 MUST 只指向本实例当前代资源，成功销毁立即逻辑失效；在途 GPU 使用的旧资源 SHALL 保留至使用完成（R-I3、R-I4）。

#### Scenario: Caller input and recoverable update failure
- **WHEN** 调用者在成功 Create/Update 后修改或销毁原容器，或一次更新在可恢复接受阶段失败
- **THEN** 已接受内容不依赖原地址；失败不替换旧内容、不发布半成品，空 mesh 更新合法并清除旧几何。

#### Scenario: Invalid and exhausted identity
- **WHEN** 使用 null、跨实例、过期或重复销毁句柄，或身份/代数耗尽
- **THEN** 无效句柄以 InvalidHandle 拒绝；身份不回绕，耗尽时拒绝分配或退役槽位，不使旧句柄复活。

### Requirement: CPU Input Semantics
现代资源/帧输入 SHALL 保持基础方块 CPU 语义：完整非索引三角形、有限属性、非负 uint32 整数纹理层；纹理为正尺寸/层数、精确长度的紧密 RGBA8、逐层 top-left rows，颜色空间为 Linear/SRGB。相机 SHALL 使用有限位姿和弧度 FOV、有效 near/far 与非退化 forward/up；transform 与有序 selection AABB SHALL 有限。

#### Scenario: Invalid input and mixed layers
- **WHEN** 输入违反尺寸、精确长度/溢出、三角形、有限值、整数层或相机/AABB 约束
- **THEN** 在原生提交前拒绝；同一三角形顶点层号不同本身不非法，Render 仍按本 draw 纹理数组层数校验全部顶点所用层号是否合法。

#### Scenario: Coordinate and sample meaning
- **WHEN** 使用合法基础方块输入在 D3D12 绘制
- **THEN** 保持世界坐标/Y-up/右手视图与 CCW 绕序，后端内部处理深度范围；point/wrap、alpha < 0.01 discard 不写深度，不引入透明排序；Linear/SRGB 分别保留值/原生解码，当前 UNORM 输出不额外 sRGB encode。

### Requirement: Frame Submission and Suspension
现代门面的 Render SHALL 完整校验并自有化帧准备后才提交唯一后端绘制/呈现，不保存输入 span 或世界/ECS 引用；现代路径应用不得再 Present。首个 frame ID 可为 0，此后成功接受或合法跳过的 ID SHALL 严格递增，输入/可恢复准备失败不得消费 ID。零尺寸 SHALL 暂停而非设备故障。

#### Scenario: Validation and frame retry
- **WHEN** 任一 draw/资源配对/相机/selection/能力或 ID 无效
- **THEN** 不提交部分帧；修正可恢复错误后可用同 ID 重试，不能在已接受帧后复用或递减 ID。

#### Scenario: Zero extent and restore
- **WHEN** 成功 Resize 到任一维为 0，随后收到合法帧，再成功恢复真实非零像素 extent
- **THEN** 暂停时完整校验后返回 Skipped/ZeroExtent、消费 ID，不进入后端 Render/acquire/Present；恢复后继续绘制。重复相同 Resize/VSync 请求不重复原生工作。

### Requirement: Failure and Explicit Shutdown
现代门面的 RenderError SHALL 保留类别、后端、操作及可选文字化原生码/诊断。不可恢复设备/提交/超时/初始化失败，或原生接受后无法取得实际 Info，SHALL 停止续交；Failed/Stopped SHALL 只允许 GetInfo 与 Shutdown（R-I5、R-I6）。关闭及必要等待 MUST 有界，失败不得冒称正常回收或恢复。

#### Scenario: Fatal failure and cleanup
- **WHEN** 出现不可恢复故障或关闭失败
- **THEN** Renderer 进入/保持 Failed，拒绝新资源、帧、Resize/VSync/Poll，仍可读取最后 Info 和尝试清理；不得隐式切后端或无限等待，清理错误不得覆盖原始错误。

#### Scenario: Successful and fallback shutdown
- **WHEN** Window 仍有效时显式 Shutdown 成功，或异常路径触发 noexcept 析构兜底
- **THEN** 成功关闭才使句柄失效并进入 Stopped、重复 Shutdown 幂等；析构执行有界兜底且不抛出，但不替代成功显式关闭或验收。无完成/设备移除证明的终止保留不是正常回收。

### Requirement: Owned Results and Observability
现代门面的 Info、样本、截图 SHALL 为自有值，Info SHALL 报告实际后端/设备/驱动、extent、samples、呈现模式与能力，不拿请求值代填。GPU 样本 SHALL 非阻塞返回已就绪结果并保留来源 frame ID，缺失 SHALL 有原因而非填 0；统计 SHALL 区分 CPU prepare/submit/present 等待及上传归属。

#### Scenario: Frame result and screenshot
- **WHEN** Render 返回 Presented 或合法 Skipped，或 Presented 请求截图
- **THEN** 结果 ID 对应输入，Presented 无 skip reason，Skipped 只有 ZeroExtent/Occluded 且不伪造截图；请求截图为实际 extent 的自有 top-left 紧密 RGBA8。返回不等于所有 GPU 工作已完成。

#### Scenario: Query and cost scope
- **WHEN** Poll 返回有效、缺失或 DeviceFailure 样本
- **THEN** 有效时长非负且 reason=None；缺失无时长并标 Unsupported/Discarded/DeviceFailure，后者使 Renderer Failed。D3D12 draw 时间仅含 mesh/selection draw，不含 clear/resolve/截图/Present；未及时 Poll 的有界队列可诊断丢弃，不承诺无损保存。

#### Scenario: Upload and protocol identity
- **WHEN** app 上传持久 mesh/texture 或导出现代采样
- **THEN** 成功 Create/Update 字节单独计一次，Render-local 上传另计，CPU cache/simulation 不伪装为门面 prepare；现代输出具名后端，不把数据装进 `gl_*` 字段。原生 Benchmark 尚只读旧 GL 协议，不由现代 telemetry 输出推定工具已支持。

### Requirement: Game World Conversion and Cache
现代游戏转接 SHALL 在 World mesh callback 借用期内复制顶点，以 WorldId+ChunkCoord 映射持久句柄并仅在成功接受后推进发布 revision；静止世界不重复上传，空发布清旧几何，缺席记录不得当作流式删除。Renderer SHALL 不接收 World/Chunk 私有对象；legacy GL仍每帧打包/上传，不从本 requirement 获得持久缓存或现代状态机保证。

#### Scenario: Per-record acceptance and conversion
- **WHEN** 同步 World 已发布网格、遇到中途失败或转换真实 atlas/相机
- **THEN** 先前成功记录保留、失败记录可重试，不承诺整批事务；世界坐标不重复平移，UV 只翻一次；游戏 atlas 保持 512x512/64x64 tile、底行优先层号与每层 top-left RGBA8，RGB 补 alpha=255；相机度转弧度、near/far 为 0.1/2000。
