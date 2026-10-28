---
tags:
  - area/spec
---

# Renderer Profiling Delta

## MODIFIED Requirements

### Requirement: Owned Results and Observability

现代门面的 Info、帧结果与截图 SHALL 为自有值，Info SHALL 报告实际后端/设备/驱动、extent、MSAA samples、呈现模式与能力，不拿请求值代填。只为旧性能链服务的 `PollGpuSamples`、GPU 样本/缺失队列、timestamp query/校准及公开细分耗时结果 SHALL 退役，不在普通循环暗中创建、提交或轮询；这里的 GPU 时间样本不等于保留的 MSAA samples。CPU profiling SHALL 改由可选埋点表达 prepare/submit/present 等真实阶段和上传计数，而非旧导出协议。资源退休、呈现完成、设备错误判断、截图 readback 必需的 fence/等待/完成事实 MUST 保留，不能因删除计时而删除正确性同步。

#### Scenario: Frame result and screenshot
- **WHEN** Render 返回 Presented 或合法 Skipped，或 Presented 请求截图
- **THEN** 结果 ID 对应输入，Presented 无 skip reason，Skipped 只有 ZeroExtent/Occluded 且不伪造截图；请求截图为实际 extent 的自有 top-left 紧密 RGBA8。返回不等于所有 GPU 工作已完成，截图不要求旧性能 Session

#### Scenario: Query and cost scope
- **WHEN** 普通运行、独立场景或截图路径执行一帧，或发生设备/提交失败
- **THEN** 不再建立旧 GPU timing query 或 Poll 队列来获得耗时；原 DeviceFailure/提交失败仍从实际操作/完成检查传播并停止续交，不以移除采样吞掉故障。CPU 区域不冒称 GPU draw；RenderDoc 捕获与 readback 等待有独立诊断身份，不并入未经定义的正式预算

#### Scenario: Upload and protocol identity
- **WHEN** app 上传持久 mesh/texture，或工具观测当前运行
- **THEN** 成功 Create/Update 与 Render-local 上传计数按真实归属表达，不重复计数，CPU cache/simulation 不伪装为门面 prepare；实际后端/设备/配置及 shader 生成身份仍可在诊断工作流核对，不依赖旧 summary/`gl_*` 字段或 Benchmark 消费者。历史报告/安装包身份不回写，不因新采集器替换而放宽当前 shader/资产要求


### Requirement: Modern Backend Takeover Acceptance
D3D12在R3、Vulkan在R4 SHALL 用真实生产target与统一游戏调用方分别满足全部适用A01–A19。GPU必验仅本机RTX5070Ti；MUST核对实际创建设备与呈现身份，不以另一GPU/API、WARP、mock、旧GL或库存代替。未执行/失败/未交付 SHALL 明示，不因工作负载/能力缩减或旧成绩变为通过。

旧 benchmark 产品、异步计时、文件协议及 strict 前台准备条款 SHALL 仅用于解释退役前候选及原证据；退役后的当前产品不再提供这些入口。截图、实际后端/配置、部署/资产身份和正确性同步继续有效；后续正式性能验证须先确认新工具口径，不自动执行、豁免预算或把旧成绩移用于新候选。

#### Scenario: A01 Public-only independent consumer
- **WHEN** 构建仅公开头消费者及越界包含检查
- **THEN** 相同签名无需GLAD/GLFW/SDL3/Windows/D3D12/Vulkan私有头；私有接口/后端类型越界被边界检查拒绝。

#### Scenario: A02 Production boundaries and independent configurations
- **WHEN** 配置 CPU-only、关闭测试，或显式请求已退役 benchmark-only
- **THEN** 游戏/renderer 生产在 game、测试在 root test，生产不依赖 test/support；CPU-only 无图形 SDK 发现/链接且独立 CPU 入口可构建运行，关闭测试无 fixture。benchmark-only 明确拒绝并提示迁移，不为保留此验收身份重建旧产品；原工具结果保持历史候选身份

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
- **WHEN** 开关 VSync 或请求 MSAA 采样设置
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
- **WHEN** 当前产品请求截图，或核对退役前候选的异步 GPU 样本
- **THEN** 当前截图仍为非空、方向/内容正确的实际帧自有结果，不要求旧 Session/Poll；必要 readback 等待保留且不冒称普通帧性能。样本 frame ID、缺失标志及不为计时每帧阻塞的原判据只解释历史样本，不要求当前产品继续创建计时 query；历史结果不回写

#### Scenario: A18 Benchmark protocol and installed release
- **WHEN** 从无关 CWD 运行当前现代后端安装包，或核对退役前性能报告及其消费者结果
- **THEN** 当前包仍须有合法 shader/运行依赖、不需开发 SDK，实际后端/adapter/API/driver/shader 生成/配置身份可核对；该部署条件不再同时触发旧性能导出/读取协议。历史 CPU/上传/present/GPU draw 语义、差异成本、gl 字段及显存/缺失口径保持，不要求新产品继续读取旧报告；旧 CLI 明确拒绝而非兼容伪成功。OpenGL 仍是显式过渡路径，其最终退役条件不提前宣称成立

#### Scenario: A19 Local gameplay regression
- **WHEN** 每个现代后端在本机RTX5070Ti验证适用14项玩法及至少15分钟连续实际操作
- **THEN** 分别记录实际adapter/驱动/工作负载/正常退出与未执行/不适用依据，不继承旧GL/T2成绩；不宣称第二机器、M5的60分钟、中低端整机门槛或M4性能通过。

#### Scenario: Reference and evidence boundaries
- **WHEN** 以截图、性能或候选包提出接管结论
- **THEN** 先确定正式画面参考/容差与同条件基线/明确预算；无对应基线不称性能无回归，不接受丢几何/错材质/颠倒坐标；证据绑定实际版本、失败和复测，查验快照不冒称完整可运行包。

#### Scenario: D3D12 frame and process memory budget
- **WHEN** 核对退役前候选中用正式同版本、同机、同配置、同工作负载 GL/D3D12 配对采样判定本机 D3D12 性能预算
- **THEN** SHALL 按 [已选预算口径](../../../archive/2026-10-19-m3-t3-renderer/decisions.md#d3d12-已选预算口径)逐场景、逐对应轮次同时检查 P95/P99 的绝对及相对双门槛，以及工作集/私有提交各自正式采样段约每秒观测最大值的相对门槛；不混池、平均、挑最好或删慢帧。内存不包含显存/启动阶段、不叠加 T2 绝对上限；缺失指标不补 0，无有效对应 GL 基线不得称预算通过。启动/首次可操作等其他指标仅观测、不新增硬预算；选择不是采集授权或实测通过，也不自动成为 Vulkan 的预算。

#### Scenario: User limited short comparison closeout
- **WHEN** 用户按 [本轮追加决定](../../../archive/2026-10-19-m3-t3-renderer/decisions.md#本轮-r2-短采样收口与风险接受边界)取消正式九轮、改为限时短对照并收口本轮 R2 工作
- **THEN** SHALL 保留原始短采样及用户人工反馈，标记正式预算未验收、实际焦点差异及其余未覆盖风险；不把 quick 结果或工作收口当完整 R2/R3 通过，不自动恢复长采样或降低原接管目标。

#### Scenario: Explicitly approved shorter formal pairing
- **WHEN** 核对退役前候选中按用户2026-10-13确认的 [18分钟正式配对窗口](../../../archive/2026-10-19-m3-t3-renderer/decisions.md#2026-10-13-正式配对短窗口)执行本机D3D12预算对照
- **THEN** SHALL 在同版本GL/D3D12三场景各三独立轮次中使用10秒预热和50秒正式采样，逐对应轮次按原已选帧时间/内存双门槛判定；保留全部失败/慢帧及实际焦点，原始数据与原生消费者校验完整才判数据有效。短窗口风险及原72分钟未执行事实不抹除，原quick不升级为正式，不自动复测或移用为Vulkan预算。

#### Scenario: Explicit Vulkan budget and paired collection
- **WHEN** 核对退役前候选中按用户2026-10-15确认的[Vulkan预算及采样口径](../../../archive/2026-10-19-m3-t3-renderer/decisions.md#2026-10-15-vulkan已选预算与正式配对口径)执行正式对照
- **THEN** SHALL 独立采集同版本GL/Vulkan三场景各三进程、每轮10秒预热＋50秒正式采样，实际1920×1080/4x/VSync-off/allow-unfocused及固定完整世界；逐对应轮次同时检查P95≤10ms、P99≤12ms且各自≤GL的110%，两项进程内存正式窗口约每秒观测最大值各自≤GL的110%。保留全部慢帧/焦点/失败与原生校验，不自动复测/放宽、不借旧GL或D3D12成绩；焦点可比性、短窗口风险、预算结论和节点批准分别记账，不新增启动/显存硬预算或重开暂缓D3D12缺陷。


#### Scenario: Opt-in bounded foreground preparation
- **WHEN** 核对退役前候选中用户显式选择strict前台采样并设置`--foreground-ready-timeout-seconds 1..120`
- **THEN** 游戏SHALL在初始世界/网格就绪后处理本窗口事件，以连续1秒获焦、非最小化且实际尺寸匹配为准备成功条件；准备等待有界，超时/关闭/Esc具名失败并清理，不生成可接受成绩。准备耗时单列startup，只有成功后才从零开始完整预热和采样，不模拟/绘制工作负载或删除已有测量帧。
- **AND** 省略选项及allow-unfocused路径保持原行为；计时开始后失焦仍使strict整轮无效，不暂停/重置时钟、不剔除坏帧、不自动重试或放宽预算。GL及两现代采用同版本同准备协议配对，新结果不覆盖历史失败或自动产生节点批准。

#### Scenario: Explicit manual visual acceptance for this round
- **WHEN** 用户按[本轮人工视觉口径](../../../archive/2026-10-19-m3-t3-renderer/decisions.md#2026-10-13-人工视觉核查口径)认可已完成的GL/D3D12人工画面对照
- **THEN** SHALL 记录“未发现明显错误”并按该范围收口视觉核查，不强加未批准的数值容差；同版本GL参考、透明深度差异单列及不接受丢几何/错材质/颠倒坐标仍保留。不得补造截图/视点/包身份或宣称逐像素完全一致，也不得将该反馈用于豁免正式性能超限或继承为Vulkan验收。

## ADDED Requirements

### Requirement: Native resource names and balanced marker regions

D3D12 和 Vulkan SHALL 在独立于 Tracy/validation 的注释构建开关下提供有意义的原生资源名及成对命令区域。名称 SHALL 标明逻辑用途和必要稳定身份，覆盖实际使用的持久 world mesh/纹理、呈现/深度/MSAA 资源、upload 与 command 对象；事件组 SHALL 对应实际存在的 world、selection、upload、resolve/readback 等工作，不虚构无工作区域或 GPU 时间。原生 SDK 类型与命名机制 SHALL 留在各私有后端，不泄漏到公开消费者或 World/ECS。

#### Scenario: DR-A01 readable captures in both backends
- **WHEN** 分别捕获启用注释的 D3D12 与 Vulkan 有效帧
- **THEN** 工具可按资源用途及事件层次定位实际 draw/upload/resolve，空 selection/readback 不伪造工作；帧中使用的已命名资源可辨，名称不包含宿主地址/用户目录，不把每帧改名当稳定身份

#### Scenario: DR-A02 marker and validation independence
- **WHEN** Tracy/注释/validation 独立开关组合变化，或 Vulkan 缺 `VK_EXT_debug_utils`/注释函数
- **THEN** Tracy 关闭仍能使用注释，validation 关闭不阻止受支持注释；无注释能力时明确诊断后禁用注释而非整个渲染，原本要求的 validation/能力缺失仍按原规则具名失败，不借此豁免

#### Scenario: DR-A03 disabled and exceptional paths
- **WHEN** 注释关闭、嵌套区域提前退出、记录失败或资源重建/关闭
- **THEN** 关闭不求值动态注释参数，启用时成功打开的区域完整配对且字符串寿命有效；不改变在途资源所有权、呈现退休、有界等待、原始错误及 Failed/Stopped 约束，注释不新增线程或恢复设备
