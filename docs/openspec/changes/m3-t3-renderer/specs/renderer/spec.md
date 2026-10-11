---
tags:
  - area/spec
---

# T3 Renderer Delta

本文件是未关闭接管目标，不是现役实现或新验收结论。未修改的 [现役 requirements](../../../../specs/renderer/spec.md) 继续有效；R-I1–R-I6完整分布于 Public Boundary、Resource Acceptance、Frame Submission、Failure requirements。机制见 [design](../../design.md)，选择见 [decisions](../../decisions.md)，唯一任务见 [tasks](../../tasks.md)。

## MODIFIED Requirements

### Requirement: Backend Selection and Availability
游戏 SHALL 在创建窗口前选择一个后端，一实例一后端；D3D12接管获用户批准前默认 SHALL 保持隔离的legacy OpenGL4.6且D3D12为显式候选，批准后 SHALL 切到已确认现代后端。随后交付Vulkan SHALL 复用同一游戏侧v1及游戏循环；未实现/未编入/能力不足的后端 SHALL 具名拒绝，不静默回退、软件代替或降低能力。现代工厂无需为过渡GL建立完整门面实现。

#### Scenario: Current game default and opt-in candidate
- **WHEN** D3D12接管尚未获批准且游戏未给出 `--renderer`，或显式选择已编入的 `d3d12`
- **THEN** 前者选择legacy OpenGL，后者选择Native-mode Window与真实D3D12；`RendererConfig`默认D3D12不改变游戏默认，候选交付不批准切默认。

#### Scenario: Unavailable backend or capability
- **WHEN** 现代门面请求未实施的OpenGL/Vulkan、未编入的现代后端，或硬件不满足D3D12 FL12_0/SM6_0、Vulkan1.2加必需surface/swapchain扩展、请求采样/呈现能力
- **THEN** 请求具名失败，不改选其他后端/WARP，不静默降低能力或采样数；调整能力基线须独立确认。

#### Scenario: Selected asset preflight
- **WHEN** 执行选中后端的 `--check-assets`
- **THEN** 仅检查选中路径的必需文件；过渡 legacy OpenGL 检查 regular_file，当前 D3D12 还拒绝零大小或不可访问文件。不初始化GPU，也不将文件存在视为解码/shader/PSO/驱动验证；现代错误包含后端和资产位置，非法参数/资源预检/运行故障保持分类。

#### Scenario: Approved default and second backend
- **WHEN** 用户批准D3D12接管，或后续已交付/编入的Vulkan被显式选择
- **THEN** 前者才切确认的现代默认；后者创建专用Vulkan Window，私有桥接/后端拥有真实资源与呈现，不把Native窗口、T2探针、清屏或空target当作接管。运行中不热切后端。

## ADDED Requirements

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
