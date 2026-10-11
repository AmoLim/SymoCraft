---
type: 接口候选
status: P01细化候选，D01至D13保持已确认，Q01至Q04待选择
project: Symocraft
module: M3-T4-App-Platform-Simulation
created: 2026-10-09
updated: 2026-10-09
tags:
  - area/milestones
---

> 2026-10-11 文档浓缩后已退役复核源：现行入口见 [权威位置](../../verification/VERIFICATION.md#原位置保留的历史材料)；原文保留点时反馈、历史身份和复核依据，不再维护为当前机制或活动计划，旧勾选不产生新批准。

# T4 接口候选清单

本页是 P01 点时接口来源，只供迁移复核；现行目标机制在 [T4 design](../../openspec/changes/m3-t4-application-simulation/design.md)，未决选择在同 change 的 decisions。以下声明不是现役实现、冻结接口或独立活动计划，不再在此维护新选型。D01–D13 保持已选约束，Q01–Q04 仍需用户选择，其余局部命名随整体接口审阅，不逐字段制造选型。

## 接口归属与依赖

| 归属 | 候选公开入口/头 | 消费者与依赖约束 |
| --- | --- | --- |
| game/app 私有 | Application、ApplicationConfig、RunResult、Q03 的关闭报告 | main 使用；StartupOptions 只在 app 转配置，不下传模块 |
| platform | 保留实际 SymoCraft::Window；新增 Platform::Runtime、WindowConfig、WindowFacts、InputFrame | app、renderer 授权私有桥和平台测试；不因候选改成 Platform::Window，不公开 SDL/HWND/Vulkan 类型 |
| simulation | session.h、intent.h、result.h、error.h、view.h | app/真实 CPU 消费者；World/Registry 借用，单向依赖 physics，不链接 platform/renderer/app |
| simulation/physics | symocraft/physics 下的 data.h、query.h、step.h、raycast.h、error.h | simulation 适配与独立核心测试；稳定 target 为 symocraft_physics，仍是内部物理板块，不提前搬成正式模块 |
| telemetry | Session、受控元数据/启动指标、FinalFacts、ExportReceipt | app 与独立观测测试；只接受中立值，不持有被测对象或 renderer SDK 类型 |
| game/app 私有场景 | BenchmarkScenario、ScenarioActions | 与观测分别配置；沿用既有 static/walk/edit 规则，不创建通用场景框架 |

不增加共享“引擎 API”target。平台与模拟的指针事件各为 SDK-free 小型值，候选由 app 逐项转换到模拟事件容器并复用容量，再以只读 span 借用；不靠 reinterpret_cast、布局恰好相同或直接导出 Window 的类型实现零复制。此局部转换不是新增输入缓冲玩法，也不承诺零分配；如实测成本需改变归属，另留证评估，不先把输入放进 foundation/scene。

## 应用与平台

| 候选入口 | 参数/返回 | 状态、所有权与消费要求 |
| --- | --- | --- |
| Application::Create / Assemble | 自有 ApplicationConfig；独占 Application owner | 配置拆成窗口、renderer、world/初始角色、可选场景、可选观测；一个实例一次 Run，先建立可留失败事实的组合根，再装配 |
| Application::Run | 返回自有 RunResult | 至少区分正常结束/取消/运行错误和采样是否完成；捕获计算错误后停止本帧，不把部分提交变成成功编辑请求；关闭结果按 Q03 另行交接 |
| Application::Shutdown | Q03 决定报告的交接形状 | 显式执行完整关闭顺序，首个运行原因不被次要关闭原因覆盖；重复调用不重试已经终结的资源；析构只 noexcept 有界兜底 |
| Platform::Runtime 创建 / Close | 主线程 owner、配对本模块初始化责任 | 不复制/移动，Window 借用稳定 Runtime；有活窗口时不得提前结束，只释放自身取得的责任 |
| Window::Create | Runtime& 与 WindowConfig → unique_ptr<Window> | 保留 T2 一个活动窗口和建窗前选择 OpenGL/Native/Vulkan；返回明确拥有权，不继续公开拥有型裸指针 |
| Window::Facts / FramebufferSize | 返回自有 WindowFacts / PixelSize | 真正 drawable 像素，不是逻辑请求尺寸；关闭/最小化/获焦与本轮失焦脉冲可在 Capture 前读取；隐藏可写 width/height |
| Window::PollInt / WaitEvents | 同步处理事件和事实 | 保留 Poll 发布指针事件、Wait 累积不发布、下一次正常采集交接的时点；事件回调不推进模拟/提交 GPU |
| Window::CaptureInput | InputFrame& cleared_frame | 在原有 held 采样边界交换已发布指针容器、填事实；不保存调用方地址，不清待发布事件；原始键盘字段按 Q02 |
| Window::ResetInput / Destroy | 清平台输入 / 显式结束资源 | 不改写已交接自有记录，不清关闭请求；所有借用者和原生图形调用先结束 |

WindowConfig 只描述平台用途：自有标题、逻辑请求尺寸、窗口模式与样式。基准路线、Performance::Session、玩家输入过滤、strict/allow-unfocused 策略不进入平台配置。默认尺寸、无边框/任务栏策略与实际 Benchmark 像素核实沿用 T2，不以新字段静默改变。

WindowFacts 至少含实际像素尺寸、focused、minimized、close_requested、focus_lost_since_poll。最后一项是事件事实而非“最终 focused 的反值”；同轮失焦再获焦仍可触发 D10 清理。InputFrame 拥有键盘事实、两鼠标按钮的 held 值、有序指针值及关联窗口事实；鼠标短按不升级为键盘式锁存。

## 计算输入和成功结果

### 最小值字段

| 候选值 | 最小字段/语义 | 不包含 |
| --- | --- | --- |
| ControlIntent | run、sensor、descend、forward/backward、right/left、jump、next_block/previous_block、place/remove、allow_edits | Key/SDL 枚举、Window、汇总 mouse_delta、显式设置任意速度或选材索引 |
| PointerInput | double mouse_dx、mouse_dy、scroll_y；right/up 为正，与 T2 坐标转换一致 | 按住状态、平台类型、第二份已累计指针输入；按事件逐次裁剪 |
| InitialPlayerState | 初始 body 位置、yaw/pitch、既有角色参数与镜头默认值 | Camera 实体、窗口宽高、资源加载器；装配入口按 Q01 |
| PlayerPose | body 位置、yaw/pitch及必要方向基；眼位偏移单独表达 | 可写组件地址；观测 x/y/z 仍是 body 位置，不误换成眼位 |
| LensValue | FOV 度、near=0.1、far=2000，FOV 沿用 1–45 度与初始45度 | renderer 默认 far=1000 的隐式替换、第二个权威相机实体 |
| SelectionHit | 命中格坐标、法线方向、距离；optional 无命中 | GPU 选择框；app 由格坐标派生旧中心/现代 min-max，不能改命中取整规则 |
| AdvanceStats | 实际有效规则时间 float 秒、已完成物理子步数 | 实际墙钟耗时、GPU/present 指标、第二份时间归一化 |
| AdvanceResult | 自有 pose/镜头与眼位偏移、optional SelectionHit、optional World::EditRequest、AdvanceStats | World/Registry/组件指针、完整实体快照；当前规则一次最多一项普通编辑，不预建可增长请求队列 |

allow_edits 只控制普通交互请求，不能关闭必要模拟；场景仍可得到选择，但不因观测存在而允许玩家操作。相反方向同时按下保持旧优先级；place/remove 同时成立仍先放置。短按和 held 的消费意义按 T2/D10，不新增自动重放。

AdvanceResult 中的姿态、镜头、眼位偏移组成按需视图派生的自有输入，避免派生旧结果时读到下一次推进后的 FOV/位姿。它们是返回值，不形成第二份可写权威。速度、加速度、冷却与预算余数的额外比较采集按 Q04，不借此扩大每次返回为整 Registry。

### 声明形状

```cpp
namespace SymoCraft::Simulation {
    enum class SessionState { Ready, Paused, Failed };

    class SimulationSession {
    public:
        // Q01 determines creation and actor binding; World/Registry stay borrowed.
        [[nodiscard]] AdvanceResult Advance(
            const ControlIntent& intent,
            std::span<const PointerInput> pointer_events,
            double elapsed_seconds);
        void Pause();
        void Resume();
        [[nodiscard]] SessionState State() const noexcept;
        // Q04 determines the extra read-only diagnostic access.
    };

    [[nodiscard]] ViewDescription DeriveView(const AdvanceResult& result);
}
```

会话不可复制/移动，不跨调用保存输入地址。elapsed_seconds 先以 double 校验/裁剪，再走原 float 规则/预算路径；输入所有事件先预检后才开始修改。恢复后的第一次成功推进使用有效 dt=0；拒绝不能提前消费这个恢复边界。Pause 清 D10 指定项，重复 Pause 幂等；Resume 仅从 Paused 恢复，Failed 不可通过 Pause/Resume 复活。状态拒绝走 D13，不把它当作新推进失败。

单次 Advance 的顺序为输入/指针 → 角色规则 → 唯一预算 → 零到多个物理子步/每实体提交 → 玩家恢复检查 → 射线/普通编辑请求 → 自有结果。放置与选材两个冷却分别沿旧语义更新；零步跳跃立即生效，不补位置积分。新会话不执行 mesh/GPU 更新，不读取画面比例。

## 可选视图与 T3 转接

核对 [真实 T3 类型](../../../game/modules/renderer/include/symocraft/renderer/types.h) 后，现代入口接收的是 position/forward/up、FOV 弧度及 near/far 的 CameraParameters，**不接收 Camera 类或投影矩阵，也不要求模拟提供 aspect**。候选 DeriveView 只从自有结果派生中立相机参数值；app 转单位并适配 Renderer v1，不改变 Renderer v1。

旧 GL 对照需要 RH_NO 的 view/projection，仍由 app 的局部适配从同一中立值与实际像素比例生成；现代 RH_ZO/Vulkan 投影由各 backend 负责。非法比例仅拒绝可选 GL 视图派生，不修改会话或旧结果。零尺寸不绘制而不是钳成1强行通过；CPU 路径不调用视图、投影或呈现。此细化解释 T4 的可选视图边界，不把 aspect 加回 Advance。

## 物理数据与查询

沿用 [正式计划最小数据](../../openspec/changes/m3-t4-application-simulation/design.md)：BodyState、AabbShape、PhysicsSettings、StepResult、RayHit。BodyState 保留位置/速度/加速度、gravity/sensor/ground；设置保留重力、轴向限速与既有容差。单步 dt 必须有限且为合法固定步，不消费累计预算。

```cpp
namespace SymoCraft::Physics {
    struct TerrainCollisionQuery {
        const void* context;
        bool (*is_solid)(const void*, const glm::ivec3&);
    };
    [[nodiscard]] StepResult Step(const BodyState&, const AabbShape&,
                                 const PhysicsSettings&, float step_seconds,
                                 TerrainCollisionQuery);
    [[nodiscard]] RayHit Raycast(const RayInput&, TerrainCollisionQuery);
}
```

context 和函数指针仅同步借用；函数不能为空，context 的有效性由绑定适配保证，无状态查询可约定空 context。false 只代表该格正常非碰撞；World 适配的 OutsideWorld/高度越界保持 false，未定义方块与查询错误具名传播。核心不含 ECS/World 类型、不保存 query。单实体先算自有结果，成功才提交该实体，不改扫描/平局处理或宣称整个 Advance 事务。

## 具名错误与提交进度

| 层 | 候选类别/字段 | 消费方式 |
| --- | --- | --- |
| simulation | SimulationError：Rejected / AdvanceFailed、ErrorCode、Operation、Stage、optional AdvanceProgress、自有诊断 | app/CPU 捕获类别，不解析 what()；常规派生 std::runtime_error，不借用 RenderError |
| physics | PhysicsError：参数/数值/查询类别、Step/Raycast、操作阶段、自有诊断 | 核心不含 EntityId；simulation 包装时补会话进度并先置 Failed，保留首因 |
| AdvanceProgress | 规则是否已开始/完成、预算是否已消费、本次计划步数/完成步数、当前子步已提交实体数及已知标记 | “已完成”不等于“预算已消费”；未知不得写成0。值不带 World/ECS 地址或可误提交的普通编辑载荷 |

阶段候选为 Preflight、Input、Rules、Physics、Recovery、Interaction、ResultBuild；错误码至少覆盖非法时间/输入/状态、缺失实体或组件、非法形状/数值、查询/存储、结果构建。具体枚举局部命名不另开用户选型，整体审阅时冻结。

Rejected 在修改/预算消费前，状态与恢复待处理边界不变；AdvanceFailed 先停止后续工作并进入 Failed，再报告。之前角色规则/实体/子步保留 D04 语义，当前失败实体结果不提交。外部 World::TryEdit 的合法 Rejected/Unchanged/Changed 仍是显式世界结果，不强行改成计算异常。

先用 IsEntityValid/HasComponent 等验证已知绑定前提，再使用组件；检查不是整个存储安全证明。旧 GetComponent 的断言和 AddComponent 对空值解引用、内部 Add 分配失败只打印返回等路径必须按实际调用链最小修复或阻断留证，不能仅靠 catch 转换。OOM 时允许保留原 bad_alloc/未知异常兜底，不承诺所有字符串或新异常都可构造；有界诊断失败也不能继续模拟。

## 观测和场景

| 候选入口/值 | 最小语义与消费者 |
| --- | --- |
| RecordMetadata / RecordStartup / SetRendererInfo / AddStartupUpload | 自有 Data::Value 冷路径及 T3 中立描述；只在 Recording 可写，保留现有键/单位与现代追加字段，不公开 YAML |
| Add(Frame) / Add(Memory) / SetGpuSample | 维持 CSV 行序与 renderer frame_id 的不同身份；旧 GL 行号关联和现代 ID 关联分别受控，不误把 ID 当数组下标 |
| FinalFacts | 采样是否完成、正常/取消/运行错误、各关闭步骤的结果/首因、已有无效原因；与保存结果分开 |
| Seal(FinalFacts) | 全部关闭事实确定后封闭；相同事实幂等、冲突拒绝；封闭后所有写入拒绝，不额外等待补指标 |
| Export() → ExportReceipt | 明确 Saved / Failed、运行有效性、失败阶段/路径/诊断；只保存 Sealed 记录，失败可同目录显式重试，成功返回缓存回执不写文件 |
| BenchmarkScenario::Advance | 消费原始实际时间与必要状态值 → 自有场景意图/模拟前编辑动作；不消费裁剪规则时间，不持有 Session |

FinalFacts 的关闭载荷按 Q03；记录对象覆盖最终关闭和导出寿命，不能放到运行资源内随装配失败提前销毁。Seal/Export 不报成功就不得发布 finished；退出码保持0/4/3，CLI 解析/资产预检的既有64/2不改。成功导出后的第二轮测试创建新 Session，不再写 completed/metadata 修改原封闭记录。

配置中的场景和观测为两个独立可选值；现有 CLI 的 benchmark 可映射为“场景+记录”，本轮不新增命令、Off/Basic/Deep、裁剪包或运行中启停能力。普通控制、基准 Escape 与窗口关闭策略由 app 消费 Q02 的事实，不以 Session 是否存在切换玩法。

## 冻结与实施前检查

Q01–Q04确认后，将相应声明补到本页并在正式 spec 留决策记录/对比链接；冻结前用真实 T3 接管版本重新核对 [H01–H04](consumer-audit.md#交接依赖)，再在实施授权后落实生产头/消费者编译及契约测试。本轮没有创建候选空头/target，不把这份清单标成运行验证。
