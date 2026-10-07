---
type: 系统设计
status: 已验证（T1适配），已知问题延期
project: Symocraft
module: app
created: 2026-10-05
updated: 2026-10-06
tags:
  - area/architecture
---

# Application 生命周期与值对象

关联功能：[运行编排](App-运行编排-功能.md)。Application 当前为 app 私有 namespace，不是可供底层依赖的 singleton 类。

验证：[T1 统一报告](../../../milestones/m3-t1/README.md)；[T0 历史报告](../../../milestones/m3-t0/README.md)不作为本轮证据。

## 当前设计

### 数据成员

| 类型 | 成员 | 初值 / 范围 | 含义与所有权 |
| --- | --- | --- | --- |
| `unique_ptr<Window>` | `runtime_window` | null | app 唯一持有；晚于 renderer 销毁 |
| `unique_ptr<Registry>` | `runtime_registry` | null | 实体存储；生命周期覆盖 Camera 借用 |
| `unique_ptr<Camera>` | `camera` | null | Camera facade；借用 Registry |
| `optional<World::BlockDefinition>` | `pending_block_definition` | 空 | Init 获取的配置集合；验证纹理层后移进局部 world 并 reset，不作为运行查询源 |
| `EntityId` | `player_id` | 初始化时返回 | 玩家标识值，world 不保存 |
| `PlayerInputState` | `input_state` | 默认选材 0 | simulation 解释，app 保存跨帧状态 |
| `float` | `block_place_debounce` | 0 | 每帧递减，交 simulation 验证编辑节流 |
| `bool` | `platform_initialized/renderer_started` | false | 已获取资源的清理责任；不代表完全初始化成功 |
| `unique_ptr<VoxelWorld> + Pose + Data::Value` | `PreparedWorld` | 准备时填充 | 独占本次运行世界、初始姿态与 CPU 元数据；Run/summary 局部持有，不可复制 |
| 命令行值 | `StartupOptions` | 解析器默认 | 仅 app 参数，不成为 telemetry 的接口类型 |

### 不变量

| 编号 | 条件 | 成立边界 |
| --- | --- | --- |
| I1 | `camera` 存活期间 `runtime_registry` 存活 | 初始化至 camera.reset |
| I2 | renderer/query 对象释放前 Window/context 仍有效 | 正常与异常退出 |
| I3 | modules 不包含 app 头，也不获取其内部静态对象 | CMake 与源码边界检查 |
| I4 | benchmark 分支不提交普通玩家输入 | 每个采样帧 |
| I5 | 一个生产 cpp 只有一个静态库或 exe 所有者 | 构建图 |
| I6 | 物理、交互、夹具、摘要、网格都用同一局部 `VoxelWorld` | T1 app 组合入口至 Run 返回，不依赖当前世界服务 |

<a id="接口与生命周期"></a>
### 公开接口预期行为

以下覆盖 Application 公开入口、参数解析与进程入口；省略外层 `SymoCraft::`。表中 Session/Registry/PlayerIntent 分别指 Performance/ECS/Simulation 类型，BlockDefinition/VoxelWorld 属 World。参数与 Session 指针仅调用期借用，Session 归 main。

| 签名 / 入口 | 调用方与可见范围 | 预期行为：输出及状态变化 | 前提 / 边界 | 失败反馈及失败后状态 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| `StartupOptions ParseStartupOptions(span<const string_view>)` | main / 启动测试；startup_options.h | 校验重复/未知项、数值及组合，返回配置；benchmark 指定 regression 与相应 checkpoint | 参数文本覆盖返回 view 的使用寿命；无资源初始化 | 非法项抛 invalid_argument；不返回半配置，不改 Application | [解析源码](../../../../game/app/src/startup_options.cpp) |
| `void Init(const StartupOptions&, Session* = nullptr)` | main；application.h | Window/context → Registry 注册 → Camera/renderer → pending 定义 → 玩家；设置清理责任标记 | 串行未初始化；Session 非空时校验实际 framebuffer 并写启动元数据 | 二次初始化 logic_error；其他异常上抛，可能部分获取，由 main 调用 Free 清理 | [实现](../../../../game/app/src/application.cpp) / I1/I2 |
| `void Run(const StartupOptions&, Session* = nullptr)` | main；application.h | 验证纹理层、消费定义创建局部 world、首 mesh、循环模拟/发布/复制/绘制；benchmark 写既有样本和结束诊断 | Init 成功且窗口/Registry/Camera/pending 均存在；不支持失败后续跑；I4/I6 | 缺必要状态 logic_error；其他异常展开局部 query/world，后续 main 调用 Free；不回滚已模拟帧或导出未结束截图 | [实现](../../../../game/app/src/application.cpp) / I2/I4/I6 |
| `void PrintWorldSummary(const StartupOptions&)` | main；application.h | 局部 PrepareWorld → YAML 输出；不建窗口或 CPU mesh | assets 已预检；配置 view 有效 | 读取/生成/输出异常传播；局部世界自动释放，无可继续会话 | [实现](../../../../game/app/src/application.cpp) / I6 |
| `void Free()` | main；application.h | pending → renderer → Camera/Registry → Window → GLFW；清空持有者与责任标记 | Run 局部 query/world 已结束；主线程、context 仍有效 | 处理部分初始化；自身无领域 throw，但未声明 noexcept，不承诺所有下游清理失败可恢复 | [实现](../../../../game/app/src/application.cpp) / I1/I2 |
| `int main(int argc, char* argv[])` | Windows 进程入口；非模块 API | 参数 → assets 预检 → summary 或 Init/Run → Free → 可选 Export；成功0，参数64，缺资源2，运行/导出错误3，无效采样4 | argv 至返回有效；Session 寿命覆盖 Export；Free 在运行 catch 后，Export 仅捕获 std::exception | 运行 std/非std 异常均记致命错误后清理；未给预检/Free 的所有异常统一退出码保证 | [main](../../../../game/app/src/main.cpp) / I2/I4 |

### 私有函数预期行为

以下为 application.cpp 的 TU helper / 局部 callback，无 C++ namespace private。它们不作为底层模块接口。

| 签名 / 入口 | 内部调用方 | 预期行为：处理规则及副作用 | 前提 / 边界 | 失败传播及清理责任 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| `BlockDefinition ReadBlockDefinition()` | Init / summary | assets Resolve/ReadBytes → 自有文本 → FromConfig | 仅读取配置，不获取图形或世界资源 | std 异常包装为带配置诊断的 runtime_error；诊断分配也可失败；候选自动清理 | application.cpp / [定义契约](../world/BlockDefinition-类设计.md) |
| `PreparedWorld PrepareWorld(BlockDefinition, const StartupOptions&, Session* = nullptr)` | Run / summary | 按值交定义给 Create；记录实例元数据/摘要，安装夹具/编辑并选择 Pose；可选填写生成计时 | 不隐含首 mesh；固定场景 seed/checkpoint 原规则；I6 | 任一阶段异常释放局部独占 world/报告；不返回半对象，不回滚 Session 已写字段 | application.cpp / I6 |
| `PlayerIntent MapInput(const InputSnapshot&)` | Run 普通输入 | 键状态映射 bool 意图；指针汇总保持默认0 | 逐事件指针另交 ApplyPointerInput，避免双算 | 值返回，不计算移动/碰撞或改 ECS | application.cpp |
| `CameraView CurrentCamera(const Window&)` | Run 绘制/结束诊断 | 根据窗口 aspect 和 Camera 返回自有投影/视图矩阵 | camera 存活；window 的比例有效，当前串行帧 | 下游异常传播，不保存 window 或矩阵引用 | application.cpp / I1 |
| `void UpdateInteraction(Registry&, VoxelWorld&, const InputSnapshot&, bool)` | Run | DoRayCast → 可选 TryEdit → SetSelection；同实例同帧消费 | 使用当前 player_id/选材/冷却；禁输入仍查询选择框 | 查询/编辑异常上抛，selection 可能尚未更新；不把资源失败转普通拒绝 | application.cpp / I6 |
| Run 两处 `VisitMeshes` callback | 正常 pack / 结束诊断 | 每个 record 的 span 同步交 AppendMesh，由 renderer 复制 | 仅 callback 内借用；含空发布，不保存第二份世界快照 | AppendMesh 异常上抛，World 访问 guard 恢复；不回滚 renderer 已复制部分 | application.cpp / [借用契约](../world/World-类设计.md) |

main 持有 Session，其寿命覆盖 Application 运行及退出后结果导出。SessionConfig 是显式值映射，telemetry 不依赖 StartupOptions。所有运行编排在主线程，不在 T0 引入后台工作。未承诺同进程反复 Init/Free 可重建旧 ECS 全局类型编号，相关正确性属 T5。

实现：[application.h](../../../../game/app/src/application.h)、[startup_options.h](../../../../game/app/src/startup_options.h)。

### 既有迁移与验证

移除面向底层的 `GetRegistry/GetCamera/GetWindow`；把输入规则与 GPU 操作交给正确模块。保持启动、采样和退出的顺序，不将所有模块内部对象打包成新的全局上下文传递。

历史 T0 验收见[功能验收表](App-运行编排-功能.md#验收案例)。其中构建、短采样与用户玩法确认只支持当时 I1-I5 的路径，不继承为 T1 人工验收。未验证同进程反复 Init/Free、多会话或所有分配失败组合，仍保留 T4/T5 生命周期工作。


### StartupOptions

自有 optional<uint32> seed、frame_limit=0、check_assets/world_summary/regression_scene/test_edits=false；width=1920,height=1080,warmup_seconds=60,sample_seconds=180,vsync=false。
checkpoint（spawn）/benchmark/output_directory/focus_policy（strict）是 string_view，解析后借用调用方参数存储或字面量，**不是自有字符串**；不得跨参数寿命。ParseStartupOptions(span<string_view>) 校验组合/数值并抛 invalid_argument；转换到自有 SessionConfig 后不延长原 view。

### PreparedWorld

Application 私有 PreparedWorld：`unique_ptr<World::VoxelWorld> instance`、`TestScene::Pose pose`、`Data::Value report`。它按值接管工厂结果，同步安装场景后返回；因 unique_ptr 不可复制但可移动。Pose::name 仍借用静态名字，不能整体称为完全自有字符串。局部对象持有世界直到 Run/summary 返回或异常展开，不向 simulation 提供所有权。工厂失败无世界实例返回，夹具/报告失败则销毁局部实例；并不承诺把已退出循环恢复为可继续运行的 Application 会话。

### 真实形态与审核入口

Application 是 namespace 运行系统，不是已批准的新实例类；本文件保留历史名称，改用系统类型。字段访问、初始化/退出串行编排见[功能](App-运行编排-功能.md)。[对象覆盖清单](../对象笔记覆盖清单.md)区分实际值与候选初稿，不把后者迁作实现。
2026-10-06 T1 自动构建/契约与旧新真实运行短测已完成；人工复查两项问题按用户要求延期，其余项目确认正常，节点待批准。证据统一见 [T1 报告](../../../milestones/m3-t1/README.md)。

## 本次变更

最小 T1 变更是局部 world 所有权、pending 定义的初始化交接、显式物理/交互查询和整数编辑。没有引入 Application 类、会话上下文或服务定位器。验证入口归[功能笔记](App-运行编排-功能.md#本次变更)，不复制结果表；完整 T4 生命周期工作仍保留。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| T4 设计冻结 | 用实例生命周期与显式运行状态替代 namespace 静态值，保持依赖方向 |
