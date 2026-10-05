---
type: 类设计
status: 已验证（T0范围）
project: Symocraft
module: renderer
class_name: Renderer / Batch / Shader / ShaderProgram / TextureArray / GpuTimer
inheritance: [ TextureArray -> Texture ]
created: 2026-10-05
tags:
  - area/architecture
---

# Renderer 内部协作者

关联功能：[显式场景输入](Renderer-显式场景输入-功能.md)。`Renderer` 当前为命名空间门面而非类；此处按真实实现记录协作者，不虚构尚未存在的 PImpl。

最终验证：[M3-T0 交付与验收报告](../../../milestones/m3-t0/README.md)。

## 当前设计

### 数据成员

| 类型 | 成员 | 初值 / 范围 | 含义与所有权 |
| --- | --- | --- | --- |
| 私有 `Batch<BlockVertex3D>` | `chunk_batch` | 空 / 最多 10,000,000 顶点 | CPU vector、VBO、VAO 的唯一所有者 |
| 私有 `Batch<LineVertex3D>` | `line_batch` | 空 / 容量 100 | 选择框 GPU/CPU 数据 |
| `ShaderProgram` | `block_shader/line_shader` | programId=0 | GPU program 与 uniform 缓存 |
| `TextureArray` | `texture_array` | textureId=0 | 图集切片后的 GPU 纹理；原 CPU 图像不常驻 |
| `DeviceInfo` | `device` | 空描述 | 当前设备缓存，供采样启动时读取 |
| `int` | `viewport_width/height` | 0 | 最近提交尺寸，避免重复 viewport 操作 |
| `array<Slot,64>` | `GpuTimer::slots_` | 未 pending | 每槽两个 GPU query，保存帧号和等待状态 |
| `Slot*` | `GpuTimer::current_` | null | 借用本对象槽位，本帧结束即清空 |

### 不变量

| 编号 | 条件 | 成立边界 |
| --- | --- | --- |
| I1 | 所有 GPU 调用在持有当前上下文的主线程上执行 | AttachContext 成功至 Free 完成 |
| I2 | Batch `data.size() <= m_batch_size`，追加先验证余量 | Init 后所有公开批次操作 |
| I3 | 借用的网格 span 不跨 AppendMesh 返回保存 | 每次 AppendMesh 返回 |
| I4 | query 槽 pending 时不重用，未就绪时不读结果 | GpuTimer 生命周期 |
| I5 | 上下文销毁前 texture/program/buffer/query ID 均已释放或归零 | app 清理顺序 |
| I6 | 门面不保存 Camera、Window、World、Registry 指针 | 全运行期；Present 临时借用 Window |

### 接口与生命周期

| 接口 | 行为 | 前提 / 边界 |
| --- | --- | --- |
| `AttachContext(Window&)` | 加载 GLAD、检查 4.6、保存设备值 | 有效窗口；失败不创建批次 |
| `Init/Free` | 建立 / 释放 GPU 渲染状态 | Init 要求 I1；失败仍调用 Free 清理已获取对象 |
| `AppendMesh(span)` | 复制到 CPU 批次 | 批次已初始化；超容量在写入前抛出 |
| `SetSelection(optional)` | 为值位置追加 24 条线顶点 | 无值不追加；调用者每帧最多一次 |
| `Render(CameraView,stats,timer)` | 清屏、绑定纹理、绘制批次并清空 CPU 数据 | 不轮询输入，不访问世界 |
| `Present/SetVsync` | 通过私有桥接交换缓冲 / 设置策略 | 当前上下文存在，不持有窗口 |
| `GpuTimer::Begin/Poll/End` | 关联帧号并异步回收结果 | 主线程；池满丢本帧测量而非阻塞 |
| `SampleDeviceMemory` | 可选 NVX 设备级计数器 | 不支持时空；不冒充进程显存 |

Batch 禁止复制/移动，析构兜底 Free。Texture 禁止复制、支持移动转移 ID，移动源 ID 清零；TextureArray 沿用此所有权。Shader 的编译和 ShaderProgram 链接失败释放临时资源。GpuTimer 不可复制，其析构结束活动查询并删除 query，必须早于上下文销毁。Renderer 仍为单上下文进程内状态，T0 不宣称可并行或可多次完整重建。

实现：[批次](../../../../game/modules/renderer/src/batch.hpp)、[计时器](../../../../game/modules/renderer/include/symocraft/renderer/gpu_timer.h)、[纹理](../../../../game/modules/renderer/src/texture.h)。

## 本次变更

公开头去除 GL 类型与批次暴露；GpuTimer 的 query 存储使用无 SDK 的无符号整数，不对调用者提供操作句柄。Batch/Shader/Texture 为私有实现，白盒测试以单独的 include 白名单进入。world 的块规则与配置加载从 renderer 移至 app 编排调用。

验收见[功能验收表](Renderer-显式场景输入-功能.md#验收案例)。正式库单测、公开头/增量边界和真实短测支持 I1-I6 的已列正常与失败路径，旧新 static 截图字节一致，安装包普通玩法获用户确认。未做长期泄漏、并发/多实例和所有驱动异常组合验证；T2 的 PImpl、句柄及多后端设计保持未实施。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| T2 开始 | 将 namespace 状态收束进实例所有权，定义显式资源句柄与后端契约 |
