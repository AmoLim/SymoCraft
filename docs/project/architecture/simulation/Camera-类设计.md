---
type: 类设计
status: 已验证（T0范围）
project: Symocraft
module: simulation
class_name: Camera
inheritance: []
created: 2026-10-05
tags:
  - area/architecture
---

# Camera 显式借用的相机门面

关联功能：[Simulation](Simulation-功能.md)。

最终验证：[M3-T0 交付与验收报告](../../../milestones/m3-t0/README.md)。

## 当前设计

Camera 创建一个 Transform 实体并提供视图/投影查询和 FOV 修改；相机组件由 Registry 持有。它不拥有 Window，不接收 GLFW 回调，不通过 Application 查找 Registry。renderer 接收相机矩阵值，不包含 Camera 类。

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

| 接口 | 行为 | 前提 / 边界 |
| --- | --- | --- |
| `Camera(registry,width,height,position)` | 建立相机实体/Transform，yaw=-90、pitch=0 | Transform 已注册；width/height 为保留的构造形参，不持有窗口 |
| `GetCameraViewMat()` | 从当前 Transform 计算 lookAt | 方向已由模拟同步/更新；C1 |
| `GetCameraProjMat(aspect_ratio)` | FOV、近平面 0.1、远平面 2000 的投影 | app 提供非零有效尺寸所得比例；C3 |
| `Scroll(y_offset)` | 修改并裁剪 FOV | 有限输入；C2 |
| 位置/角度/方向 getter 与 setter | 通过 Registry 读写 Transform | 不保留组件地址；C1 |

Camera 销毁不删除实体，由 app 在 Camera 销毁后统一清理 Registry；不能在 Registry 清空后继续使用 Camera。当前拷贝语义仍是同一实体的别名，不创建独立相机，调用者不应靠拷贝建立第二台相机。构造分配失败由上层清理 Registry；本轮不增加 ECS 事务回滚。所有调用在主线程，无后台读写保证。

实现：[公开头](../../../../game/modules/simulation/include/symocraft/simulation/camera.h)、[实现](../../../../game/modules/simulation/src/Camera.cpp)。

## 本次变更

显式注入 Registry、投影比例和滚轮值；删除 Window/GLFW/Application 依赖，以及没有实现或只存在于注释中的相机移动接口。原位置/角度和矩阵算法不改变。

验收引用[模块功能](Simulation-功能.md#验收案例)：相机/有序指针 CPU 契约通过，旧新 static 截图字节一致，用户确认本轮安装包视角、切出切回和最小化恢复正常。相机实例所有权、多视口及 Registry 失效检查仍属于后续设计，未因本轮通过而勾销。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| T3 相机重设计 | 明确相机实体所有权、拷贝/移动策略，移除不使用的宽高形参 |
| 多视口 | 以独立 CPU 相机描述与视口参数生成绘制输入，不再引回 Window 依赖 |
