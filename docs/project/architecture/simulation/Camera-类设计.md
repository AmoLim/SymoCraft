---
type: 类设计
status: 已验证（T0范围）
project: Symocraft
module: simulation
class_name: Camera
inheritance: []
created: 2026-10-05
updated: 2026-10-06
tags:
  - area/architecture
---

<a id="camera-显式借用的相机门面"></a>

# Camera 借用 Registry 的 facade

关联功能：[Simulation](Simulation-功能.md)。

最终验证：[M3-T0 交付与验收报告](../../../milestones/m3-t0/README.md)。

## 当前设计

Camera 是借用 Registry 内相机状态的 facade：对调用者提供矩阵查询与 FOV/姿态修改入口，不拥有底层实体。构造时创建 Transform 实体，相机组件由 Registry 持有。它不拥有 Window，不接收 GLFW callback，不通过 Application 查找 Registry。renderer 接收相机矩阵值，不包含 Camera 类；facade 不表示已经具备独立相机实体所有权。

### 数据成员

| 类型 | 成员 | 初值 / 范围 | 含义与所有权 |
| --- | --- | --- | --- |
| `ECS::EntityId` | `entity_id` | 构造时创建 | 借用 Registry 中相机 Transform 的标识 |
| `ECS::Registry&` | `registry_` | 构造注入 | 非拥有引用，Registry 必须先创建后销毁 |
| `float` | `fov_` | 45，滚轮后限制 1..45 | 相机独占视角参数 |

### 不变量

| 编号 | 条件 | 成立边界 |
| --- | --- | --- |
| C1 | `registry_` 存活且 `entity_id` 对应 Transform 未被移除 | Camera 使用期间由 app 保证 |
| C2 | `fov_` 在 1..45 度 | 构造及有限滚轮输入后 |
| C3 | 投影矩阵使用调用方提供的当前有效 aspect ratio | 每次调用，不缓存窗口尺寸 |

### 接口与生命周期

### 公开接口预期行为

全部为模块公开类成员；Registry 获取失败不是可恢复错误值，其断言/失效风险见 [Registry](../ecs/ECS-存储边界-类设计.md)。getter 返回数学值而非组件引用；setter 不自动重算方向。源码：[camera.h](../../../../game/modules/simulation/include/symocraft/simulation/camera.h)、[Camera.cpp](../../../../game/modules/simulation/src/Camera.cpp)。

| 签名 / 入口 | 调用方与可见范围 | 预期行为：输出及状态变化 | 前提 / 边界 | 失败反馈及失败后状态 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| `Camera(Registry&, float width, float height, vec3 position = vec3(0))` | app / CPU 消费者 | 创建实体与 Transform，scale=1、yaw=-90、position 为输入 | Transform 已注册；宽高未使用；Registry 更长寿 | 分配异常可能留下实体/组件部分更新，上层清 Registry | Camera.cpp / C1 |
| `Scroll(double)` | ApplyPointerInput | fov 减去滚轮值并 clamp 1..45 | 有限输入 | 不拒绝 NaN；不声称非法输入保持 C2 | Camera.cpp / C2 |
| `GetCameraViewMat() const` | app 渲染输入 | 按当前 position/front/up 返回 lookAt | C1；方向有效 | Registry 契约，不返回错误矩阵状态 | Camera.cpp / C1 |
| `GetCameraProjMat(float aspect_ratio) const` | app 渲染输入 | 当前 FOV、0.1..2000 平面的 perspective | 调用方保证有限正比例 | 不校验比例，无可恢复错误值 | Camera.cpp / C3 |
| `GetCameraPos() const` | app / gameplay | 返回 position 值 | C1 | Registry 契约 | Camera.cpp |
| `GetCameraPos_vec2() const` | app 世界查询 | 返回 position.x/z 值 | C1 | Registry 契约 | Camera.cpp |
| `SetCameraPos(const vec3&)` | app / gameplay | 写 Transform.position | C1；不自动更新方向 | Registry 契约 | Camera.cpp |
| `GetYaw() const` | simulation / app | 返回 yaw 角度值 | C1 | Registry 契约 | Camera.cpp |
| `SetYaw(float)` | simulation / app | 直接写 yaw，不 wrap/clamp | C1；调用方保证语义 | Registry 契约 | Camera.cpp |
| `GetPitch() const` | simulation / app | 返回 pitch 角度值 | C1 | Registry 契约 | Camera.cpp |
| `SetPitch(float)` | simulation / app | 直接写 pitch，不 clamp | C1；输入裁剪归 ApplyPointerInput | Registry 契约 | Camera.cpp |
| `GetFov() const` | app / 测试 | 返回自有 fov，无 Registry 查询 | 已构造 | 无输入校验 | Camera.cpp / C2 |
| `GetCameraFront() const` | app / gameplay | 返回 front 值 | C1，方向已更新 | Registry 契约 | Camera.cpp |
| `GetCameraUp() const` | app / gameplay | 返回 up 值 | C1，方向已更新 | Registry 契约 | Camera.cpp |

### 私有函数预期行为

| 签名 / 入口 | 内部调用方 | 预期行为：处理规则及副作用 | 前提 / 边界 | 失败传播及清理责任 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| 不适用 | Camera | 只有 private 数据成员，无 private 函数 | 隐式特殊成员见下文 | 不拥有实体清理责任 | camera.h / C1 |

Camera 销毁不删除实体，由 app 在 Camera 销毁后统一清理 Registry；不能在 Registry 清空后继续使用 Camera。当前拷贝语义仍是同一实体的别名，不创建独立相机，调用者不应靠拷贝建立第二台相机。构造分配失败由上层清理 Registry；本轮不增加 ECS 事务回滚。所有调用在主线程，无后台读写保证。

实现：[公开头](../../../../game/modules/simulation/include/symocraft/simulation/camera.h)、[实现](../../../../game/modules/simulation/src/Camera.cpp)。

### 既有迁移与验证

显式注入 Registry、投影比例和滚轮值；删除 Window/GLFW/Application 依赖，以及没有实现或只存在于注释中的相机移动接口。原位置/角度和矩阵算法不改变。

验收引用[模块功能](Simulation-功能.md#验收案例)：相机/有序指针 CPU 契约通过，旧新 static 截图字节一致，用户确认本轮安装包视角、切出切回和最小化恢复正常。相机实例所有权、多视口及 Registry 失效检查仍属于后续设计，未因本轮通过而勾销。


### 拷贝与移动边界

隐式复制/移动构造仍指向同一 Registry 与实体，不创建独立相机；引用成员使赋值不可用。Camera 析构不销毁实体；主线程无重入/异步保证。沿用原 C1-C3 调用者前提，不声称新增实体代数防护。
2026-10-06 仅源码与文档核对，见 [覆盖清单](../对象笔记覆盖清单.md)。

## 本次变更

无。已完成的 T0 变更归入当前设计；本轮只整理文档。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| T4 相机重设计 | 明确相机实体所有权、拷贝/移动策略，移除不使用的宽高形参 |
| 多视口 | 以独立 CPU 相机描述与视口参数生成绘制输入，不再引回 Window 依赖 |
