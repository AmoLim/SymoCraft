---
type: 类设计
status: 已验证（T0范围）
project: Symocraft
module: app
class_name: Application / StartupOptions / PreparedWorld
inheritance: []
created: 2026-10-05
tags:
  - area/architecture
---

# Application 生命周期与值对象

关联功能：[运行编排](App-运行编排-功能.md)。Application 当前为 app 私有命名空间，不是可供底层依赖的单例类。

最终验证：[M3-T0 交付与验收报告](../../../milestones/m3-t0/README.md)。

## 当前设计

### 数据成员

| 类型 | 成员 | 初值 / 范围 | 含义与所有权 |
| --- | --- | --- | --- |
| `unique_ptr<Window>` | `runtime_window` | null | app 唯一持有；晚于 renderer 销毁 |
| `unique_ptr<Registry>` | `runtime_registry` | null | 实体存储；生命周期覆盖 Camera 借用 |
| `unique_ptr<Camera>` | `camera` | null | 相机业务外观；借用 Registry |
| `EntityId` | `player_id` | 初始化时返回 | 玩家标识值，world 不保存 |
| `PlayerInputState` | `input_state` | 默认选材 0 | simulation 解释，app 保存跨帧状态 |
| `float` | `block_place_debounce` | 0 | 每帧递减，交 simulation 验证编辑节流 |
| `bool` | `platform_initialized/renderer_started` | false | 已获取资源的清理责任；不代表完全初始化成功 |
| `Pose + Data::Value` | `PreparedWorld` | 准备时填充 | 初始姿态、独占 CPU 元数据；只在启动期使用 |
| 命令行值 | `StartupOptions` | 解析器默认 | 仅 app 参数，不成为 telemetry 的接口类型 |

### 不变量

| 编号 | 条件 | 成立边界 |
| --- | --- | --- |
| I1 | `camera` 存活期间 `runtime_registry` 存活 | 初始化至 camera.reset |
| I2 | renderer/query 对象释放前 Window/context 仍有效 | 正常与异常退出 |
| I3 | modules 不包含 app 头，也不获取其内部静态对象 | CMake 与源码边界检查 |
| I4 | benchmark 分支不提交普通玩家输入 | 每个采样帧 |
| I5 | 一个生产 cpp 只有一个静态库或 exe 所有者 | 构建图 |

### 接口与生命周期

| 接口 | 行为 | 前提 / 边界 |
| --- | --- | --- |
| `ParseStartupOptions` | 验证组合并返回配置值 | 错误抛 invalid_argument，main 返回 64 |
| `Init` | 按依赖次序创建模块对象 | 禁止二次初始化；失败由 main 调用 Free |
| `Run` | 普通玩法或固定场景循环 | Init 成功；GPU timer 局部对象自动在退出前释放 |
| `PrintWorldSummary` | 无窗口生成确定性摘要 | assets 已预检；最后同样释放世界 |
| `Free` | 世界 -> renderer -> camera/Registry -> 窗口 -> GLFW | 支持部分初始化后的清理，不抛出新的领域错误 |
| 私有 `MapInput` | 物理键 -> PlayerIntent | 不计算运动/碰撞规则 |
| 私有 `UpdateInteraction` | 查询模拟结果，提交世界编辑和渲染选择 | 返回值立即消费，不跨帧借用内部数据 |

main 持有 Session，其寿命覆盖 Application 运行及退出后结果导出。SessionConfig 是显式值映射，telemetry 不依赖 StartupOptions。所有运行编排在主线程，不在 T0 引入后台工作。未承诺同进程反复 Init/Free 可重建旧 ECS 全局类型编号，相关正确性属 T4。

实现：[application.h](../../../../game/app/src/application.h)、[startup_options.h](../../../../game/app/src/startup_options.h)。

## 本次变更

移除面向底层的 `GetRegistry/GetCamera/GetWindow`；把输入规则与 GPU 操作交给正确模块。保持启动、采样和退出的顺序，不将所有模块内部对象打包成新的全局上下文传递。

验收见[功能验收表](App-运行编排-功能.md#验收案例)。构建/边界、静态与编辑短采样、Debug 120 帧、三类资源错误和正常退出均已验证；用户确认本轮安装包手动玩法与恢复正常，证据分别支持 I1-I5 的已列运行路径。未验证同进程反复 Init/Free、多会话或所有分配失败组合，仍保留 T3/T4 生命周期及正确性工作。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| T3 设计冻结 | 用实例生命周期与显式运行状态替代命名空间静态值，保持依赖方向 |
