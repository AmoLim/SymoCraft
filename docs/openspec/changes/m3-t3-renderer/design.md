---
tags:
  - area/spec
---

# T3 目标机制与实施边界

## Context

公开行为以 [现役 renderer](../../specs/renderer/spec.md) 与 [接管 delta](specs/renderer/spec.md) 为准；当前内部实现由 [renderer 架构](../../../architecture/renderer.md) 维护，工程边界由 [engineering](../../../architecture/engineering.md) 维护，不在本页复写逐类 API。
R1 已冻结 v1/target **设计**且 R2 有实施入口；R2 顺序1–4交付候选，不表示 R3/默认切换或 R4/Vulkan获批/完成。原批准、快照及局部运行边界统一见 [VERIFICATION](../../../verification/VERIFICATION.md)。

## Goals / Non-Goals

用同一游戏调用方完成 D3D12 和 Vulkan 的当前固定基础方块玩法接管，保护 CPU 场景与公开 v1，使 SDK、设备资源、命令和呈现留在私有后端。选择范围与非目标只由 [proposal](proposal.md) 和 [限定决策](decisions.md) 维护。
默认切换依赖 D3D12 接管批准；GL 最终退役是后续单独授权节点，暂称 R5但未定正式编号。不把 GL 全面迁入新门面或永久功能等价当作前置。

## Architecture

`app -> Renderer(PImpl) -> 私有后端工厂/接口 -> 单一真实后端`。Renderer 不读取 World、ECS、输入或 singleton；app 在 callback 内复制 World 顶点，并以 WorldId/ChunkCoord/发布 revision 记账。先分配缓存记账再 Create，接受成功才推进 revision；失败保留先前已接受记录，不推断缺席为卸载。真实 atlas 的底行优先层号、top-left 单层存储与一次 UV 转换继续保持。
公开门面不可复制/移动；Impl 独占状态，Window 只借用。所有显式入口在创建线程串行且不重入，窗口回调只记录事件，帧边界使用实际 drawable 像素 Resize；GPU资源/提交不交给后台网格工作。

Create/Update 先验证复制自有快照，再由后端接受，最后无新分配地发布 CPU 状态。句柄 owner/slot/generation 类型分离且不回绕；销毁仅逻辑失效，在途 frame 引用独立保留旧原生版本。texture 不新增 update API；不将世界材质层号改为 GPU 句柄。帧完整准备与能力校验先于唯一 Render/Present，不能借用跨返回 span。

Ready/Suspended/Failed/Stopped 生命周期保留；零尺寸合法帧完整校验后 skip且消费 ID，非零重建成功才恢复。参数/可恢复准备失败不消费 ID、不污染旧资源；原生不可回滚工作后故障必须 fatal。Info 读取失败不能使已接受原生操作继续使用旧缓存提交。关闭先停止提交，成功有界清理才 Stopped；失败保留原诊断，noexcept 析构不是正常关闭的替代证据。
上述机制落实 R-I1–R-I6；当前 P-I1–P-I8、G-I1–G-I7、D-I1–D-I15及 legacy I1–I6仍链接架构，不重编号或另建学习目录。

## Backend Mechanisms

D3D12 当前借 Native Window 的 HWND；真实非软件硬件要求 FL12_0/SM6_0，实际 samples/tearing/validation不降级。三组 FrameContext 以 fence 约束 allocator/list、descriptor、upload/readback/query及资源引用复用；mesh版本替换、texture copy/barrier/有界等待后接受。draw Execute与唯一 Present均纳入退休覆盖。缺完成/设备移除证明时的终止 quarantine 保留至进程退出，不是正常回收或成功 Shutdown。double相机/selection准备及float/clip界验证在 Execute前拒绝无法表示输入。

Vulkan 目标使用 T2 的专用 Vulkan Window 与同一 SDL loader；私有桥接返回自有扩展名/入口并创建 caller-owned surface。后端拥有 instance/device/queue、surface、swapchain、buffer/image/memory、descriptor、SPIR-V pipeline、command pool/buffer、fence/semaphore及其销毁责任；surface早于窗口与instance销毁，全部资源/instance早于窗口销毁/loader卸载。扩展名复制不延长入口/窗口寿命。
真实跟踪帧与交换链图像完成，配对 acquire/submit/present；out-of-date/suboptimal/resize与DeviceLost分开处理，不能留下已reset而无人signal的fence。所有等待有限；不引入渲染线程或设备原地恢复承诺。T2 instance/surface探针不是完整后端寿命证明，需 R4单独验证。

后端内部处理 RH视图、clip/depth/Y/绕序差异，不改变全局 GLM 宏；显式打包GPU顶点/常量布局与binding，不把28字节CPU顶点当通用GPU ABI。同源 HLSL目标分别产 DXIL/SPIR-V，差异限私有编译配置/binding；纹理方向、颜色、整数层/混层与现代alpha-depth修正独立留像素依据。

## Build and Measurement

当前真实 target 是公开CPU `renderer_api`、CPU门面 `renderer`、私有 `renderer_backend_contract`、默认D3D12与可关闭的游戏专用legacy OpenGL；2026-10-12 补充 `SYMOCRAFT_RENDER_OPENGL` 与现代独占/全关闭拒绝组合，没有Vulkan。第二现代后端按真实交付加入选择与单/双组合，不建空target。启用者才找SDK/编译器及部署shader，私有头变化不重编业务模块；静态PRIVATE仍须核对最终link/install，CPU-only/benchmark-only/关闭测试不引测试脚手架或图形SDK。
Vulkan函数入口惰性加载，未选Vulkan时缺loader不得使其他后端在参数解析前失败。shader依赖在构建规则显式声明、输出在build、安装包不要求DXC/SDK。平台bridge不反向依赖renderer，业务目标不直接链接后端；生产与测试遵守game/root test归属。

GPU查询保存精确frame ID，非阻塞Poll、未就绪/不支持/丢弃/设备失败区别，截图等待置正式测量区间外；draw不含clear/resolve/截图/Present。CPU prepare、submit、present等候与startup/frame内上传分别具名，中立协议带后端/adapter/API/driver/shader生成身份和实际图像/呈现设置，不把D3D12/Vulkan装入gl字段。2026-10-12 原生Benchmark增加显式D3D12/现代v1兼容消费者，旧GL身份/25列保持；完整A18部署与生成身份条件仍按适用证据核对，不从消费者代码存在推定关闭。

## Validation and Migration

2026-10-12 用户在证据对账后明确批准R3、授权默认切换；当前app默认D3D12，新游戏配置首次默认启用原生后端，GL仅显式选择。候选POST_BUILD身份只在out/m3-t3且Benchmark关闭的受控目录触发，普通preset可构建游戏及工具。批准与未验证项仍分开，具体新构建/包与定向验证见 [verification](verification.md#默认切换验证)；不由此完成Vulkan、双现代T3或GL退役。

先按 [tasks](tasks.md) 关闭D3D12生产/人工/构建/工具/故障缺口，再申请R3批准与默认切换；Vulkan先独立R1-E01–E03后完成同组A01–A19，最后申请双现代T3通过。每次真实运行绑定源码/构建/包hash、配置、实际RTX5070Ti身份/驱动/能力/呈现、输入顺序、原始数据/诊断及失败/复测，不用另一API或另一设备补格。
正式视觉参考已选同版本GL对照、透明深度差异单列；D3D12帧时间双门槛与两项进程内存相对预算已选，唯一数值/统计窗口/适用边界见 [decisions](decisions.md#d3d12-已选预算口径)，启动时间等其他指标不新增硬预算。具体参考身份/容差、正式同版本同条件GL配对基线与采样/复测执行安排仍待完成或确认；预算选择不是实测通过或采样授权，不能接受丢几何、错材质、颠倒坐标或缩减工作负载。mock、CPU编译、有限短测、人工画面判断、受控原生故障与自发硬件事故分别记录；无SDK机器部署、14项玩法/15分钟及R3整组不由局部工程证据关闭。
保留证据原字节与旧命令点时身份；查验归档不必包含EXE/DLL，实际包依原out身份定位。保留旧spec为只读复核来源，不sync/archive，不补用户勾选/批准，不删除GL或历史回归材料。

2026-10-12 后续用户取消本轮正式九轮，改为 5 分钟内短对照后收口 R2 工作；实际范围、替代关系及风险见 [decisions](decisions.md#本轮-r2-短采样收口与风险接受边界)。完整验收目标不因该执行收口变为已验证；不恢复长采样或以用户人工通过反馈推定数值视觉容差、R3 节点批准与默认切换。
