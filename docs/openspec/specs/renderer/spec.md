---
tags:
  - area/spec
---

# Renderer

## Purpose

最小现役公开行为基线，覆盖当前游戏选择与现代 `Rendering::Renderer`，不是 T3 整体验收声明。
2026-10-10 迁移时已静态核对公开头、门面/registry/校验、D3D12 关键实现和 app 转接；未运行 C++、游戏或 GPU。
2026-10-12 用户批准 R3 并授权默认切换后，仅更新 Backend Selection 现役行为；定向构建/启动结果及既有未覆盖风险见 [T3 验证](../../changes/archive/2026-10-19-m3-t3-renderer/verification.md#默认切换验证)，不将其他目标整组同步为已验证。
内部正确性机制及 P-I/G-I/D-I/旧 GL I 编号由 [renderer 架构](../../../architecture/renderer.md) 维护。
历史结果、版本身份、查验包与可运行包的区别及批准只从 [VERIFICATION](../../../verification/VERIFICATION.md) 判读。
源码/测试存在不证明实际设备故障、长期回收、人工玩法或 R3/R4 通过；未纳入本基线的行为也不等于未实现。

## Requirements

### Requirement: Backend Selection and Availability
游戏 SHALL 在创建窗口前选择一个后端，一实例一后端；D3D12接管获用户批准前默认 SHALL 保持隔离的legacy OpenGL4.6且D3D12为显式候选，批准后 SHALL 切到已确认现代后端。随后交付Vulkan SHALL 复用同一游戏侧v1及游戏循环；未实现/未编入/能力不足的后端 SHALL 具名拒绝，不静默回退、软件代替或降低能力。现代工厂无需为过渡GL建立完整门面实现。

#### Scenario: Current game default and opt-in candidate
- **WHEN** D3D12接管尚未获批准且游戏未给出 `--renderer`，或显式选择已编入的 `d3d12`
- **THEN** 前者选择legacy OpenGL，后者选择Native-mode Window与真实D3D12；`RendererConfig`默认D3D12不改变游戏默认，候选交付不批准切默认。

#### Scenario: Unavailable backend or capability
- **WHEN** 现代门面请求未实施的OpenGL、未编入的D3D12/Vulkan，或硬件不满足D3D12 FL12_0/SM6_0、Vulkan1.2加必需surface/swapchain及KHR或EXT swapchain_maintenance1扩展与swapchainMaintenance1 feature、请求采样/呈现能力
- **THEN** 请求具名失败，不改选其他后端/WARP，不静默降低能力或采样数；调整能力基线须独立确认。

#### Scenario: Approved Vulkan presentation retirement capability
- **WHEN** 按2026-10-14用户批准的[有限呈现退休能力](../../changes/archive/2026-10-19-m3-t3-renderer/decisions.md#2026-10-14-vulkan有限呈现退休能力)创建生产Vulkan
- **THEN** SHALL 查询并启用KHR或EXT swapchain_maintenance1及其feature，使用真实presentation fence证明呈现资源退休；提交fence不代替呈现完成，所有必要等待有界。缺该能力具名Unsupported；无完成证明终止保留完整原生owner/loader，不声称正常Shutdown，不改变公开v1或继承实验成绩。

#### Scenario: Selected asset preflight
- **WHEN** 执行选中后端的 `--check-assets`
- **THEN** 仅检查选中路径的必需文件；过渡 legacy OpenGL 检查 regular_file，D3D12/Vulkan 分别检查所选 DXIL/SPIR-V 并拒绝零大小或不可访问文件。不初始化GPU，也不将文件存在视为解码/shader/PSO/驱动验证；现代错误包含后端和资产位置，非法参数/资源预检/运行故障保持分类。

#### Scenario: Approved default and second backend
- **WHEN** 用户批准D3D12接管，或后续已交付/编入的Vulkan被显式选择
- **THEN** 前者才切确认的现代默认；后者创建专用Vulkan Window，私有桥接/后端拥有真实资源与呈现，不把Native窗口、T2探针、清屏或空target当作接管。运行中不热切后端。

#### Scenario: D3D12 default after explicit R3 approval
- **WHEN** 用户明确批准 R3 并授权默认切换后，游戏省略 `--renderer` 或显式选择保留的 `opengl`
- **THEN** 前者 SHALL 选择 D3D12/Native window，后者才进入已编入的过渡 GL；默认能力/资产缺失具名失败、不自动回退或关闭validation。新游戏配置首次默认编入D3D12，旧缓存不强改；批准与未验证技术项分开，旧包字节/默认值不回写。

#### Scenario: Current game default and explicit transition
- **WHEN** 游戏未给出 `--renderer`，显式选择 `d3d12`，或显式选择已编入的 `opengl`
- **THEN** 前两者选择 Native-mode Window 与真实 D3D12，后者选择隔离的 legacy OpenGL；两默认配置均为 D3D12，不热切后端。

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
- **THEN** 成功 Create/Update 字节单独计一次，Render-local 上传另计，CPU cache/simulation 不伪装为门面 prepare；现代输出具名后端，不把数据装进 `gl_*` 字段。原生 Benchmark SHALL 分别消费旧 GL 与具名 D3D12/Vulkan telemetry-v1，校验实际后端/配置及新会话的安装 shader 生成身份；历史 D3D12 缺生成元数据的报告仅保留离线读取兼容，不绕过新运行/恢复的包身份要求。

### Requirement: Game World Conversion and Cache
现代游戏转接 SHALL 在 World mesh callback 借用期内复制顶点，以 WorldId+ChunkCoord 映射持久句柄并仅在成功接受后推进发布 revision；静止世界不重复上传，空发布清旧几何，缺席记录不得当作流式删除。Renderer SHALL 不接收 World/Chunk 私有对象；legacy GL仍每帧打包/上传，不从本 requirement 获得持久缓存或现代状态机保证。

#### Scenario: Per-record acceptance and conversion
- **WHEN** 同步 World 已发布网格、遇到中途失败或转换真实 atlas/相机
- **THEN** 先前成功记录保留、失败记录可重试，不承诺整批事务；世界坐标不重复平移，UV 只翻一次；游戏 atlas 保持 512x512/64x64 tile、底行优先层号与每层 top-left RGBA8，RGB 补 alpha=255；相机度转弧度、near/far 为 0.1/2000。

### Requirement: Stable Game Contract and Takeover Gates
T3 MUST 按M3-T3/D01方案1、D02方案2保持R1→R2→R3→R4：稳定游戏侧Renderer v1，以D3D12先接管、Vulkan再独立接管，两现代及整合验收成立后才申请T3通过。选型、冻结、获准实施、候选交付与节点通过 SHALL 分开；公开所有权/寿命/失败/线程语义变更 MUST 先受控确认。

#### Scenario: R1 design freeze and R2 delivery
- **WHEN** 使用2026-10-09 R1冻结/R2入口或R2顺序1–4候选记录
- **THEN** 只继承其有明确身份的设计/实施/局部结果，不推定A01–A19、默认切换、Vulkan、私有完整RHI或整个T3通过，也不重启旧任务/授权。

#### Scenario: R3 and R4 completion
- **WHEN** D3D12完成适用A01–A19并获接管批准，或随后Vulkan独立完成同组与双现代整合
- **THEN** 前者为阶段交付/默认切换条件，后者才可申请T3整体验收；不能继承另一后端成绩，任务勾选/CLI validate不自动产生用户批准。

#### Scenario: OpenGL and excluded work
- **WHEN** 保留GL过渡对照或准备下一渲染能力
- **THEN** GL只做受影响回归、不承担永久新增特性等价；最终活动依赖/资产退役须后续独立授权，不先删历史依据；不由T3启动micro voxel实体外表、细体素世界、RHI空框架、透明排序、热切换或设备恢复。

### Requirement: Per Backend Contract Experiments
每个现代后端 SHALL 在本机RTX5070Ti独立完成R1-E01–E03，记录版本、请求/实际后端与adapter/驱动/能力、窗口模式、输入顺序、诊断和结果。D3D12既有实验 SHALL 保留点时身份；Vulkan SHALL 不继承其冻结前成绩。

#### Scenario: R1-E01 Actual textured drawing
- **WHEN** 验证选中现代后端的候选CPU表达
- **THEN** 实际网格采样至少两个可区分纹理层，核对UV/像素方向、层号、颜色空间、GPU布局并保留截图/诊断；不以清屏/三角形替代，不使A06/A09整项通过。

#### Scenario: R1-E02 Update and destruction
- **WHEN** 创建/更新/清空/删除资源且已提交帧可能仍在执行
- **THEN** 返回后不保存调用者容器地址，下一次绘制使用已接受内容、空网格清旧画面；不覆写在途数据或过早释放原生资源，过期句柄拒绝，逻辑失效与完成/回收分别取证；不替代A08–A10/A15完整失败和容量组。

#### Scenario: R1-E03 Presentation resource rebuild
- **WHEN** 缩放、零像素尺寸及恢复窗口呈现
- **THEN** 按实际像素重建尺寸相关资源，恢复后纹理网格仍正确，无零尺寸提交忙循环、死锁或失效引用；不是Window对象热重建，也不替代A11/A15整组。

### Requirement: Modern Backend Takeover Acceptance
D3D12在R3、Vulkan在R4 SHALL 用真实生产target与统一游戏调用方分别满足全部适用A01–A19。GPU必验仅本机RTX5070Ti；MUST核对实际创建设备与呈现身份，不以另一GPU/API、WARP、mock、旧GL或库存代替。未执行/失败/未交付 SHALL 明示，不因工作负载/能力缩减或旧成绩变为通过。

#### Scenario: A01 Public-only independent consumer
- **WHEN** 构建仅公开头消费者及越界包含检查
- **THEN** 相同签名无需GLAD/GLFW/SDL3/Windows/D3D12/Vulkan私有头；私有接口/后端类型越界被边界检查拒绝。

#### Scenario: A02 Production boundaries and independent configurations
- **WHEN** 配置CPU-only、benchmark-only或关闭测试
- **THEN** 游戏/renderer生产在game、测试在root test，生产不依赖test/support；无测试脚手架、图形SDK发现/链接，独立入口可构建运行。

#### Scenario: A03 Debug Release backend matrix
- **WHEN** 验证当前现代单后端或第二后端交付后的两种现代单/双组合
- **THEN** Debug/Release逐组合有真实结果，GL过渡组合仅在保留时测；关闭/未交付后端不要求其SDK/资产，游戏开启却全关闭明确配置失败；最终退役后的现代包无GL活动依赖，未实现组合不假称通过。

#### Scenario: A04 Dependencies and incremental builds
- **WHEN** 检查target图及改动私有后端头/公开契约
- **THEN** 无环/全局include泄漏，私有头不重编world/模拟/游戏调用者，公开变更重编真实消费者；核对实际link/install而非只看PRIVATE标记。

#### Scenario: A05 Hardware identity and unsupported devices
- **WHEN** 真实运行或请求未编入/缺运行时/能力不足后端
- **THEN** 记录且确认实际RTX5070Ti设备身份/驱动/后端/能力与实际呈现；不软件fallback/降基线。现代组合未选Vulkan时缺loader仍能运行另一后端，不能在参数解析前失败。

#### Scenario: A06 Cube textures and selection
- **WHEN** 用现有方块、纹理层与选择框画面核对现代后端
- **THEN** 面朝向、UV、层号、深度遮挡、alpha裁剪及选框正确，无镜像/倒置/错层；现代discard不写深度是已批准修正而非任意视觉差异理由，第二后端独立核对，不接受仅三角形截图。

#### Scenario: A07 Full fixed world and camera
- **WHEN** 运行相同seed/摘要/出生位姿的固定完整世界
- **THEN** 世界可见且转向/移动/缩放正确，记真实分辨率/samples/present并解释差异，不减少区块/纹理/操作或静默降低分辨率。

#### Scenario: A08 Static edits and empty mesh
- **WHEN** 首次上传后静止、连续编辑、清空网格或跨正负坐标/四块交点
- **THEN** 静止不重复上传区块几何，只更新受影响发布网格，空结果无残留、无裂缝或错块。

#### Scenario: A09 Resource boundaries and CPU lifetime
- **WHEN** 原CPU容器修改/销毁或输入存在越界层、无效长度/索引、过期/跨实例句柄
- **THEN** 已接受资源不受原容器影响；非法输入/句柄在提交前拒绝，非索引三角形与混层语义不被静默收窄。

#### Scenario: A10 In flight resources and capacity
- **WHEN** GPU在途时更新/删除或资源容量增长/失败
- **THEN** 不覆写仍被使用版本、不发布半成品，延迟释放最终完成且峰值/回收数据可解释，真实校验无未处理同步错误；画面正常不证明寿命正确。

#### Scenario: A11 Resize minimize and restore
- **WHEN** 连续缩放、最小化至少10秒后恢复
- **THEN** 零尺寸无acquire/Present忙循环，过期交换链重建无死锁，真实framebuffer和相机比例正确、输入/画面恢复。

#### Scenario: A12 VSync and sampling
- **WHEN** 开关VSync或请求采样设置
- **THEN** 实际模式/样本可查询，不支持明确记录，不将VSync-on帧率冒充off性能或以静默降级通过。

#### Scenario: A13 Asset and allocation failures
- **WHEN** 隔离包缺/坏shader、坏纹理或发生分配失败
- **THEN** 具名选中后端/资源并清理，可恢复更新失败保留旧资源，不修改原资产；文件预检不代替实际驱动shader/PSO故障。

#### Scenario: A14 Device submit and timeout failures
- **WHEN** 发生实际或注入DeviceLost、提交失败、等待超时
- **THEN** 停止续交，保留原始错误且有限退出，不无限等待/假装恢复；真实与注入分别记录，隔离坏文件或mock不关闭真正在途故障要求。

#### Scenario: A15 Exit and partial construction
- **WHEN** Esc/关闭按钮、构造部分失败、重复Shutdown或重复启动
- **THEN** 遵守R-I1–R-I6：Window借用有效、创建线程串行非重入、不跨返回借用、在途资源待完成、Failed停止提交/零尺寸仅暂停、Stopped只查询/幂等关闭；图形先于窗口清理，无未解释资源增长，失败清理不冒称成功。

#### Scenario: A16 Backend diagnostics
- **WHEN** 验证已交付现代后端及实际保留GL受影响路径
- **THEN** 分别启用D3D12 Debug Layer/Vulkan validation/适用GL诊断并留证，无未解决资源/同步错误；诊断不可用明示受阻，不静默关闭或把Release空日志当等效验证。

#### Scenario: A17 Screenshots and asynchronous samples
- **WHEN** 请求截图并采集异步GPU结果
- **THEN** 截图非空、方向/内容正确，样本保留原frame ID，未就绪/不支持/丢弃有缺失标志；不为查询阻塞每帧，截图readback等待在正式性能区间外标记。

#### Scenario: A18 Benchmark protocol and installed release
- **WHEN** 现代后端导出/工具读取采样或从无关CWD运行安装包
- **THEN** 保持CPU/上传/present/GPU draw语义并具名新后端/adapter/API/driver/shader生成/实际配置；差异成本单列，gl字段不装现代数据，协议变更加版本兼容或显式拒绝，旧GL可读且身份不变；设备/进程显存区别，缺指标不补0；shader/合法运行依赖齐全、不需开发SDK，退役后无GL专属依赖。

#### Scenario: A19 Local gameplay regression
- **WHEN** 每个现代后端在本机RTX5070Ti验证适用14项玩法及至少15分钟连续实际操作
- **THEN** 分别记录实际adapter/驱动/工作负载/正常退出与未执行/不适用依据，不继承旧GL/T2成绩；不宣称第二机器、M5的60分钟、中低端整机门槛或M4性能通过。

#### Scenario: Reference and evidence boundaries
- **WHEN** 以截图、性能或候选包提出接管结论
- **THEN** 先确定正式画面参考/容差与同条件基线/明确预算；无对应基线不称性能无回归，不接受丢几何/错材质/颠倒坐标；证据绑定实际版本、失败和复测，查验快照不冒称完整可运行包。

#### Scenario: D3D12 frame and process memory budget
- **WHEN** 用正式同版本、同机、同配置、同工作负载 GL/D3D12 配对采样判定本机 D3D12 性能预算
- **THEN** SHALL 按 [已选预算口径](../../changes/archive/2026-10-19-m3-t3-renderer/decisions.md#d3d12-已选预算口径)逐场景、逐对应轮次同时检查 P95/P99 的绝对及相对双门槛，以及工作集/私有提交各自正式采样段约每秒观测最大值的相对门槛；不混池、平均、挑最好或删慢帧。内存不包含显存/启动阶段、不叠加 T2 绝对上限；缺失指标不补 0，无有效对应 GL 基线不得称预算通过。启动/首次可操作等其他指标仅观测、不新增硬预算；选择不是采集授权或实测通过，也不自动成为 Vulkan 的预算。

#### Scenario: User limited short comparison closeout
- **WHEN** 用户按 [本轮追加决定](../../changes/archive/2026-10-19-m3-t3-renderer/decisions.md#本轮-r2-短采样收口与风险接受边界)取消正式九轮、改为限时短对照并收口本轮 R2 工作
- **THEN** SHALL 保留原始短采样及用户人工反馈，标记正式预算未验收、实际焦点差异及其余未覆盖风险；不把 quick 结果或工作收口当完整 R2/R3 通过，不自动恢复长采样或降低原接管目标。

#### Scenario: Explicitly approved shorter formal pairing
- **WHEN** 按用户2026-10-13确认的 [18分钟正式配对窗口](../../changes/archive/2026-10-19-m3-t3-renderer/decisions.md#2026-10-13-正式配对短窗口)执行本机D3D12预算对照
- **THEN** SHALL 在同版本GL/D3D12三场景各三独立轮次中使用10秒预热和50秒正式采样，逐对应轮次按原已选帧时间/内存双门槛判定；保留全部失败/慢帧及实际焦点，原始数据与原生消费者校验完整才判数据有效。短窗口风险及原72分钟未执行事实不抹除，原quick不升级为正式，不自动复测或移用为Vulkan预算。

#### Scenario: Explicit Vulkan budget and paired collection
- **WHEN** 按用户2026-10-15确认的[Vulkan预算及采样口径](../../changes/archive/2026-10-19-m3-t3-renderer/decisions.md#2026-10-15-vulkan已选预算与正式配对口径)执行正式对照
- **THEN** SHALL 独立采集同版本GL/Vulkan三场景各三进程、每轮10秒预热＋50秒正式采样，实际1920×1080/4x/VSync-off/allow-unfocused及固定完整世界；逐对应轮次同时检查P95≤10ms、P99≤12ms且各自≤GL的110%，两项进程内存正式窗口约每秒观测最大值各自≤GL的110%。保留全部慢帧/焦点/失败与原生校验，不自动复测/放宽、不借旧GL或D3D12成绩；焦点可比性、短窗口风险、预算结论和节点批准分别记账，不新增启动/显存硬预算或重开暂缓D3D12缺陷。


#### Scenario: Opt-in bounded foreground preparation
- **WHEN** 用户显式选择strict前台采样并设置`--foreground-ready-timeout-seconds 1..120`
- **THEN** 游戏SHALL在初始世界/网格就绪后处理本窗口事件，以连续1秒获焦、非最小化且实际尺寸匹配为准备成功条件；准备等待有界，超时/关闭/Esc具名失败并清理，不生成可接受成绩。准备耗时单列startup，只有成功后才从零开始完整预热和采样，不模拟/绘制工作负载或删除已有测量帧。
- **AND** 省略选项及allow-unfocused路径保持原行为；计时开始后失焦仍使strict整轮无效，不暂停/重置时钟、不剔除坏帧、不自动重试或放宽预算。GL及两现代采用同版本同准备协议配对，新结果不覆盖历史失败或自动产生节点批准。

#### Scenario: Explicit manual visual acceptance for this round
- **WHEN** 用户按[本轮人工视觉口径](../../changes/archive/2026-10-19-m3-t3-renderer/decisions.md#2026-10-13-人工视觉核查口径)认可已完成的GL/D3D12人工画面对照
- **THEN** SHALL 记录“未发现明显错误”并按该范围收口视觉核查，不强加未批准的数值容差；同版本GL参考、透明深度差异单列及不接受丢几何/错材质/颠倒坐标仍保留。不得补造截图/视点/包身份或宣称逐像素完全一致，也不得将该反馈用于豁免正式性能超限或继承为Vulkan验收。
