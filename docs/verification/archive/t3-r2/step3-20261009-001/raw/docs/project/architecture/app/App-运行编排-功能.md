---
type: 功能
status: R2顺序3已实施，当前验证以T3阶段报告为准
project: Symocraft
module: app
created: 2026-10-05
updated: 2026-10-09
tags:
  - area/architecture
---

# App 运行编排

关联设计：[Application 生命周期](Application-类设计.md)、[GameRenderer 私有转接](GameRenderer-类设计.md)、[WorldRenderCache](WorldRenderCache-类设计.md)。依据：[M3-T0](M3-T0-模块软硬边界.md)、[M3-T1](M3-T1-世界模块重构.md)、[T3 spec](../../../spec/M3/M3-T3-渲染器重构.md)。

当前实施与最终成绩：[T3 R2顺序3报告](../../../milestones/m3-t3/r2-step3-implementation.md)。[T1 统一报告](../../../milestones/m3-t1/README.md) 与 [T0 历史报告](../../../milestones/m3-t0/README.md) 保留各自版本和批准范围，不将旧玩法确认继承为现代后端通过。

## 当前设计

`game/app` 是可执行程序组合入口，不是底层模块的服务定位器。`main` 负责参数、资源预检、致命错误与退出码；`Application` 负责连接模块、主循环和关闭次序。`symocraft_startup` 为真实参数解析静态库，供产品和测试共用，不重编同一份 cpp。

2026-10-09 R2顺序3已将调用方统一为一份玩法循环，由app私有 `GameRenderer` 隔离现代门面与旧GL的渲染差异。后端选择和可编入检查先于Window创建；D3D12使用Native Window、新Render唯一Present，OpenGL仍默认并保留旧context/逐帧Append/旧Present。没有复制另一套物理、输入、编辑循环，也没有把旧GL实现迁为新门面后端。公开Renderer v1保持冻结边界。

源码：[主循环](../../../../game/app/src/application.cpp)、[main](../../../../game/app/src/main.cpp)、[构建](../../../../game/app/CMakeLists.txt)。原 Application 的 GLFW callback 转至 platform，键鼠玩法映射规则转至 simulation；公共的 GetCamera/GetRegistry/GetWindow 服务入口已移除。

### 目标与流程

```mermaid
flowchart TD
    args["main：参数 / 按后端 assets 预检 / 可选 Session"] --> select["所选后端已实施并编入？具名拒绝或确定 WindowMode"]
    select --> init["Init：匹配 Window → Registry → Camera/GameRenderer → 配置 → 玩家"]
    init --> prepare["Run：局部 PreparedWorld 独占 World → 夹具/编辑 → 首 mesh"]
    prepare --> input["Poll / 输入映射 → simulation 查询与推进"]
    input --> edit["app：同实例 TryEdit → RebuildDirtyMeshes"]
    edit --> pack["GameRenderer：旧GL逐帧Append；现代仅接受变化publication并组FrameInput"]
    pack --> render["旧Render/Present或新Render唯一Present；单次相机/UV转接"]
    render --> sample["稳定frame ID / 中立CPU GPU进程样本 → telemetry"]
    sample -->|下一帧| input
    sample -->|结束| close["显式释放World缓存；Free：图形Shutdown并reset → Camera/Registry → Window → SDL video"]
    close --> export["main：可选 Export → 退出码"]
```

普通玩法暂停和 benchmark 继续运行是显式分支。benchmark 输入只有 Esc 取消，其移动/编辑场景仍按 v2 固定计划执行，不将鼠标或朋友的键盘操作混入采样。

### 公开接口预期行为

| 签名 / 入口 | 调用方与可见范围 | 预期行为：输出及状态变化 | 前提 / 边界 | 失败反馈及失败后状态 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| Application::Init / Run / PrintWorldSummary / Free | main；app 私有头 | [逐函数权威表](Application-类设计.md#公开接口预期行为) | 串行生命周期及局部 world | 同目标表；不保证完整会话恢复 | application.h / I1-I6 |
| ParseStartupOptions / main | 进程启动与参数测试 | [参数与退出码权威表](Application-类设计.md#公开接口预期行为) | argv/view 与 Session 寿命由调用方维持 | 同目标表；错误码不混作世界编辑结果 | startup_options.h / main.cpp |

### 私有函数预期行为

| 签名 / 入口 | 内部调用方 | 预期行为：处理规则及副作用 | 前提 / 边界 | 失败传播及清理责任 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| ReadBlockDefinition / PrepareWorld | Init / Run / summary | [内部权威表](Application-类设计.md#私有函数预期行为) | 定义按值交接；实例失败局部释放 | 同目标表；Session 写入非事务 | application.cpp / I6 |
| MapInput / UpdateInteraction | Run | 同步输入值映射、同实例raycast/edit，返回选框值给GameRenderer | 同帧玩法；无渲染器反向获取app相机 | 异常传播，不回滚已模拟帧 | application.cpp / I1/I6 |
| GameRenderer / WorldRenderCache | Run绘制与结束诊断 | [转接入口](GameRenderer-类设计.md#入口预期行为)、[发布缓存](WorldRenderCache-类设计.md#方法预期行为) | callback内复制；只按成功接受的发布revision缓存现代句柄 | 逐记录成功保留、失败退出；仅显式释放，不以漏访删除 | game_renderer.cpp / world_render_cache.h |

### 关键约束与取舍

- 不新增游戏算法。`MapInput` 转换物理键；保序鼠标/滚轮事件逐条交 `ApplyPointerInput`，维持每个事件的俯仰/FOV 限制。速度、跳跃条件、选块限制与 debounce 属于 simulation。
- app 拥有玩家 ID 和 camera，world 不再创建玩家或依赖 ECS。
- `PrepareWorld` 把定义按值交给 `VoxelWorld::Create`，启动元数据、夹具、碰撞、交互和 benchmark 均使用这同一实例；world 不是 namespace 静态值，也不提供当前世界服务入口。
- 旧pack继续包住 `VisitMeshes -> AppendMesh`；现代cache计时包含变化记录自有化/Create/Update及draws组装，render/submit与present_wait分开记录。GPU query保持异步，现代结果按原始frame ID回填，不写入 `gl_*` 字段，也不把缺样本补0。
- world mesh仅在callback期间借用；现代当场自有化并作一次V转换，持久账本只保存World/Chunk身份、句柄和成功接受的publication revision，不保存World地址/span或第二份完整世界快照。静止不重复上传，empty更新有效，漏访不当卸载。
- 图集保持512×512、64个64×64底行优先层，层内top-left RGBA8/Linear；Camera由真实getters度转弧度、RH、near0.1/far2000。world-space顶点不再加区块平移；现代截图在采样外经Assets::WritePng写入。
- 正常Run显式ReleaseWorld；Free在窗口销毁前尝试Shutdown并reset所有图形所有者。清理保留首故障并继续后续步骤，不将设备故障或有限等待策略视为已经动态验收。
- world-summary 不创建窗口；资源错误沿现有退出码报告。BUILD_TESTING=OFF 产品不依赖 fixture。
- 旧未调用线程池、另一套 input/event 保留为 `.disabled`，不编译、不导出，详见[遗留清单](App-休眠源码清单.md)。它们不是当前支持能力。

### 验收案例

以下是 T0 历史快照；不将旧勾选或用户玩法确认继承为当前 T1 安装包的验收。

当前T3人工核查仍只在 [唯一阶段入口](../../../milestones/m3-t3/manul-verification.md) 维护。顺序4生产候选/真实发布包以及R3的适用A01–A19、14项玩法和至少15分钟连续操作仍待独立完成；此处不新增或自动勾选现代验收项目，不切默认、不删除GL。

| 状态 | 场景 | 预期行为 |
| --- | --- | --- |
| [x] | Debug/Release 干净构建和原测试 | 通过，生产 cpp 唯一 target 所有者 |
| [x] | 固定 seed summary | 旧新摘要字节一致，digest、441 区块和姿态不变 |
| [x] | 普通游戏操作及失焦恢复 | 用户确认本轮安装包移动跳跃、视角、放破、切出切回、最小化恢复与正常退出均正常 |
| [x] | static/edit 真实短采样 | v2 元数据与采样列对照一致；新 static/edit 失焦条件下有效完成 |
| [x] | 安装包从无关目录启动 / 资源缺失 | 资源定位与错误退出保持，不包含测试脚手架；损坏配置/shader/图片真实探针均退出 3 |

2026-10-05 最终 Debug 与 Release 分别 60/60，通过的独立 benchmark 为 5/5；真实探针和包身份见最终报告。用户对 `out/m3-t0/install/full-release/SymoCraft.exe` 的本轮人工复查与迁移前确认分别记录。上述短采样不包含新的 walk 真实运行，不是 36 分钟正式基准或完整输入回放。

## 本次变更

2026-10-09，T3 R2顺序3实现统一游戏调用方、私有现代发布缓存和中立采样/截图/退出转接。CPU Debug/Release及原生Debug/Release已构建；最终测试与实际运行成绩以 [顺序3报告](../../../milestones/m3-t3/r2-step3-implementation.md) 为准，不据本笔记宣称GPU完整玩法已通过。实际DeviceLost/在途超时、生产包顺序4与R3均保留后续边界。

以下2026-10-06的T1适配保留为历史实现依据；原逐帧Append说明仅对应旧GL，对当前现代cache行为以上方当前设计和私有类笔记为准。上面的旧勾选不构成T3通过。

### 世界实例与文件组合

`Init` 使用 assets `ReadBytes` 获取配置文本，再交 `BlockDefinition::FromConfig`；不让 world 接触资源路径。定义临时存于 app 的 `pending_block_definition`，`Run` 在首次绘制前验证实际纹理层数，随后移动进局部 `PreparedWorld::instance`。移动完成即清空 pending 值；运行成功或抛出异常都通过局部 `unique_ptr` 销毁世界。`PrintWorldSummary` 用独立局部对象完成同样组合，不创建窗口或先生成 CPU 网格。

`Run` 只接受尚未消费定义的成功 Init，误用抛 `logic_error`，不解引用缺失 optional。仍不提供完整 T4 Application 实例状态机。

### 编辑、发布和计时

- 普通交互收到整数 `World::EditRequest` 后调用同一实例的 `TryEdit`。拒绝无副作用；正常交互不新增日志或采样计数。
- benchmark 先用整数坐标查询并核对 before ID，再检查 `EditResult::Accepted()`；Rejected 才导致失败，Unchanged 属于已接受计划动作，继续推进原 `applied_edits`、`sample.edits` 与 lateness。
- 世界创建不隐含 mesh。夹具和启动测试编辑完成后调用 `RebuildDirtyMeshes`，保留 `first_mesh` 分段；每帧 mesh、pack、render、present 原分段和 v2 字段不变。
- `VisitMeshes` 的每个 record（含空发布）都同步交 `AppendMesh(record.vertices)`；身份和版本暂不缓存，renderer 不依赖 world，也没有第二份完整世界网格快照。
- 出生点、重生、摘要和最终 benchmark digest 均来自运行实例；旧生成字段由 `VoxelWorld::Describe` 提供，不从另一套 app 设置重建。

本轮摘要、玩法、benchmark 与失败/退出验证统一见 [M3-T1 报告](../../../milestones/m3-t1/README.md)，不另维护平行验收表。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| T4 开始 | 把本层 namespace 生命周期状态收束为实例，完善 state machine；不反向恢复服务定位 |
| 明确要求 gameplay profiling | 另行定义可选采集，不复用禁用正常输入的 benchmark 冒充玩法回放 |
