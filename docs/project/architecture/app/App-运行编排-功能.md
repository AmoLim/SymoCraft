---
type: 功能
status: 已验证（T0范围）
project: Symocraft
module: app
created: 2026-10-05
tags:
  - area/architecture
---

# App 运行编排

关联设计：[Application 生命周期](Application-类设计.md)。依据：[M3-T0](../../../spec/M3-T0-模块软硬边界.md)。

最终验证：[M3-T0 交付与验收报告](../../../milestones/m3-t0/README.md)。整体交付仍由用户验收。

## 当前设计

`game/app` 是可执行程序组合入口，不是底层模块的服务定位器。`main` 负责参数、资源预检、致命错误与退出码；`Application` 负责连接模块、主循环和关闭次序。`symocraft_startup` 为真实参数解析静态库，供产品和测试共用，不重编同一份 cpp。

源码：[主循环](../../../../game/app/src/application.cpp)、[main](../../../../game/app/src/main.cpp)、[构建](../../../../game/app/CMakeLists.txt)。原 Application 的 GLFW 回调转至 platform，键鼠玩法映射规则转至 simulation；公共的 GetCamera/GetRegistry/GetWindow 服务入口已移除。

## 本次变更

### 目标与流程

```text
StartupOptions -> Assets 预检 -> SessionConfig（可选）
Window + context -> Registry 注册 -> Camera -> renderer GPU 状态 -> block 配置 -> 玩家
固定世界准备 -> 初始网格 -> runtime ready
Poll -> 物理键映射为 PlayerIntent -> simulation -> 编辑请求 -> world
world CPU mesh -> renderer 批次 -> CameraView -> Render -> Present
CPU / GPU / 进程样本 -> telemetry
Run 局部 query 销毁 -> world释放 -> renderer释放 -> camera/Registry -> Window -> GLFW
```

普通玩法暂停和 benchmark 继续运行是显式分支。benchmark 输入只有 Esc 取消，其移动/编辑场景仍按 v2 固定计划执行，不将鼠标或朋友的键盘操作混入采样。

### 关键约束与取舍

- 不新增游戏算法。`MapInput` 转换物理键；保序鼠标/滚轮事件逐条交 `ApplyPointerInput`，维持每个事件的俯仰/FOV 限制。速度、跳跃条件、选块限制与 debounce 属于 simulation。
- app 拥有玩家 ID 和 camera，world 不再创建玩家或依赖 ECS。
- pack 计时包住 `VisitMeshes -> AppendMesh`，render 包住相机值准备、GPU 提交，present 单独计时；GPU query 保持异步。
- world mesh 只在回调期间借用，renderer 立即复制；模块之间没有额外的整世界缓存和第二份网格容器。
- world-summary 不创建窗口；资源错误沿现有退出码报告。BUILD_TESTING=OFF 产品不依赖 fixture。
- 旧未调用线程池、另一套 input/event 保留为 `.disabled`，不编译、不导出，详见[遗留清单](App-休眠源码清单.md)。它们不是当前支持能力。

### 验收案例

| 状态 | 场景 | 预期行为 |
| --- | --- | --- |
| [x] | Debug/Release 干净构建和原测试 | 通过，生产 cpp 唯一 target 所有者 |
| [x] | 固定 seed summary | 旧新摘要字节一致，digest、441 区块和姿态不变 |
| [x] | 普通游戏操作及失焦恢复 | 用户确认本轮安装包移动跳跃、视角、放破、切出切回、最小化恢复与正常退出均正常 |
| [x] | static/edit 真实短采样 | v2 元数据与采样列对照一致；新 static/edit 失焦条件下有效完成 |
| [x] | 安装包从无关目录启动 / 资源缺失 | 资源定位与错误退出保持，不包含测试脚手架；损坏配置/shader/图片真实探针均退出 3 |

2026-10-05 最终 Debug 与 Release 分别 60/60，通过的独立 benchmark 为 5/5；真实探针和包身份见最终报告。用户对 `out/m3-t0/install/full-release/SymoCraft.exe` 的本轮人工复查与迁移前确认分别记录。上述短采样不包含新的 walk 真实运行，不是 36 分钟正式基准或完整输入回放。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| T3 开始 | 把本层 namespace 生命周期状态收束为实例，完善状态机；不反向恢复服务定位 |
| 明确要求 gameplay profiling | 另行定义可选采集，不复用禁用正常输入的 benchmark 冒充玩法回放 |
