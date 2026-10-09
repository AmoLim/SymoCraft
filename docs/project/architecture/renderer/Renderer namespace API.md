---
type: namespace API设计
status: 已验证（T0范围）
project: Symocraft
module: renderer
created: 2026-10-05
updated: 2026-10-06
tags:
  - area/architecture
---

<a id="renderer-门面与协作者"></a>

# Renderer namespace API 

Renderer 是提供统一渲染入口的 namespace API；这里表示一组调用入口，不指已经实现的 Facade pattern 或 PImpl 实例。保留历史文件名。
关联：[功能与验收](Renderer-显式场景输入-功能.md)、[覆盖清单](../对象笔记覆盖清单.md)。

## 当前设计

### 状态所有者与审核入口

| namespace 状态 | 独立对象 |
| --- | --- |
| chunk_batch / line_batch | [Batch](Batch-类设计.md)，10,000,000 / 100 顶点容量，CPU vector/VBO/VAO |
| block_shader / line_shader | [ShaderProgram](ShaderProgram-类设计.md)，临时 [Shader](Shader-类设计.md) |
| texture_array | [TextureArray](TextureArray-类设计.md)，基类 [Texture](Texture-类设计.md) |
| app Run 局部采样对象（由 app 持有） | [GpuTimer](GpuTimer%20Class.md)，Render/Batch 临时借用 |
| device / viewport_width,height | 设备缓存与最近尺寸，初始空/0 |

### 不变量

| 原编号 | 条件与边界                                                     |
| --- | --------------------------------------------------------- |
| I1  | 所有 GPU 操作在当前上下文主线程，AttachContext 至 Free                   |
| I3  | AppendMesh 同步复制，不跨返回保存 span                               |
| I5  | 上下文销毁前 texture/program/buffer/query 释放/归零，app 编排前提        |
| I6  | namespace API 不保存 Camera/Window/World/Registry 指针，Present 临时借用 Window |

I2 容量归 [Batch](Batch-类设计.md#不变量)；I4 复用/就绪归 [GpuTimer](GpuTimer%20Class.md#不变量)，保留原编号来源，不再双份维护。

<a id="门面流程与接口"></a>

### namespace API 流程与接口

本节以下两表是 Renderer namespace 自身函数的权威行为记录：公开头中 15 个入口全部列出，内部函数仅有 Debug callback。Batch、ShaderProgram、TextureArray 等对象的方法由各自笔记维护，不复制到本表。

### 公开接口预期行为

所有入口均属于 `renderer.h`；GPU 操作要求 I1 的当前上下文主线程。表中的调用顺序是调用者前提，不表示已有完整运行时顺序校验。

| 签名 / 入口 | 调用方与可见范围 | 预期行为：输出及状态变化 | 前提 / 边界 | 失败反馈及失败后状态 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| `void AttachContext(Window& window)` | app；模块公开 | 桥接设当前上下文，加载 GLAD，缓存设备，输出设备信息，设置 viewport；不创建批次 | Window 已创建且有效；先于 Init | GLAD/4.6/设备字符串失败抛 runtime_error；上下文可能已切换，缓存可能部分更新，不回滚 | [实现][renderer-src]；I1/I6 |
| `void Init()` | app；模块公开 | 开启深度/背面剔除及 Debug callback，设置 line 容量，重载 shader，初始化两批次 | AttachContext 成功；当前上下文及 GLAD 有效；不承诺重复 Init | 前提检查抛 runtime_error；shader/批次异常上抛，可能已有部分资源，由 app 调用 Free | [实现][renderer-src]；I1/I5 |
| `void Free()` | app 统一退出；模块公开 | 销毁 texture、两批次和两 program；viewport 缓存归零，Debug 时注销 callback | GPU 资源仍需有效上下文；先结束 GpuTimer 借用/使用 | 未提供统一 GL 错误反馈；不是初始化回滚事务，不清空 device 字符串 | [实现][renderer-src]；I1/I5 |
| `const DeviceInfo& Device()` | app/采样；模块公开 | 返回 namespace 设备缓存的借用 const 引用，不获取新设备信息 | AttachContext 成功后信息才有意义；不跨上下文重建保存语义快照 | 无显式错误反馈；未 AttachContext 时为默认/既有缓存，不表示有效设备 | [实现][renderer-src]；I6 |
| `size_t AllocatedBufferBytes()` | app/采样；模块公开 | 汇总两批次已初始化容量字节，未初始化批次贡献 0 | CPU 查询；不是实际驻留显存或纹理/program 占用 | 无显式失败通道 | [实现][renderer-src]；[Batch](Batch-类设计.md#职责与状态) |
| `uint16_t LoadTextureAtlas(const filesystem::path& path)` | app 启动；模块公开 | 创建/移动替换 TextureArray，返回层数；不设置 world 方块规则 | 有效上下文及图集资源；格式/层数要求见对象笔记 | 解码/尺寸/GL 等异常上抛；创建成功才执行替换，不承诺全部驱动异常恢复 | [实现][renderer-src]；[TextureArray](TextureArray-类设计.md#当前设计) |
| `void AppendMesh(span<const BlockVertex3D> vertices)` | app 同步 VisitMeshes；模块公开 | 当场复制进 chunk_batch；空范围无追加，不保存 span | 批次已初始化，连续范围有效且容量足够 | 未初始化抛 logic_error，超容量抛 length_error；检查先于追加，分配异常上抛 | [实现][renderer-src]；I3；[Batch](Batch-类设计.md#接口与失败边界) |
| `void SetSelection(optional<glm::vec3> position)` | app 每帧；模块公开 | 空值无操作；有值按 floor 后中心追加 24 个放大边框线顶点，不先清旧线 | line_batch 已初始化；每帧最多一次是调用者约定 | 批次/分配异常上抛；逐顶点追加，失败可能留下部分线，非原子操作 | [实现][renderer-src]；[Batch](Batch-类设计.md#接口与失败边界) |
| `void SetViewport(int width, int height)` | app resize；模块公开 | 尺寸与缓存不同时 glViewport 并更新缓存，相同时无操作 | GLAD/上下文有效；调用方提供合法 framebuffer 尺寸，零尺寸可暂停绘制 | 不验证负尺寸或读取 GL 错误；非法尺寸不保证真实 viewport 与缓存一致 | [实现][renderer-src]；I1 |
| `void Render(const CameraView& camera, RenderStats* stats=nullptr, GpuTimer* timer=nullptr)` | app 每帧；模块公开 | 绑定纹理、清屏、上传组合矩阵、绘制两批；成功绘制后清对应 CPU 顶点，stats 累加；不获取输入/world | 批次/program 就绪；camera/stats/timer 仅本次借用；timer 顺序见其笔记 | Bind/Batch/timer 异常上抛；之前的绘制、stats 和清批可能已发生，没有整帧回滚 | [实现][renderer-src]；I1/I3/I6 |
| `void Present(Window& window)` | app 每帧；模块公开 | 桥接交换该窗口缓冲；不保存 Window 指针 | 窗口和上下文有效 | 无 Renderer 层成功值或 GL 错误恢复协议 | [实现][renderer-src]；I1/I6 |
| `void SetVsync(bool enabled)` | app 启动；模块公开 | 桥接设置当前上下文 swap interval | 当前上下文有效 | 无 Renderer 层成功反馈，不承诺驱动实际呈现策略 | [实现][renderer-src]；I1 |
| `void ReloadShaders()` | Init 或 app 重载；模块公开 | 先销毁两旧 program，再从 Assets 路径编译/链接 block 和 line shader | 有效上下文及资源路径 | 任一步失败上抛；旧 program 已销毁，可能仅 block 重建成功，不支持旧 shader 回退 | [实现][renderer-src]；[ShaderProgram](ShaderProgram-类设计.md#接口与失败状态) |
| `DeviceMemory SampleDeviceMemory()` | app 采样；模块公开 | 返回自有 optional KiB 值；NVX 未支持时两项为空，负值对应项为空 | AttachContext 的能力缓存已建立；上下文有效 | 未做完整驱动错误检查；缺失不补零，非本进程 VRAM | [实现][memory-src]；[DeviceMemory](#devicememory) |
| `void CaptureFramebuffer(int width, int height, const filesystem::path& path)` | app 非逐帧诊断；模块公开 | 从 GL_BACK 读 RGB，pack alignment 设 1，启用写图翻转并输出 PNG | 合法正尺寸、有效上下文与可写路径；调用方验证，函数不完整校验 | 分配异常上抛，写图失败抛 runtime_error；pack/写图全局设置不恢复，不承诺无部分文件 | [实现][memory-src]；I1 |

### 私有函数预期行为

namespace 没有 C++ private。这里的“私有”指 translation-unit 内部函数，不属于模块公开头；不把其他对象的方法误列为 Renderer 内部函数。

| 签名 / 入口 | 内部调用方 | 预期行为：处理规则及副作用 | 前提 / 边界 | 失败传播及清理责任 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| `void GLAPIENTRY MessageCallback(GLenum, GLenum type, GLuint id, GLenum severity, GLsizei, const GLchar* message, const void*)` | Init 注册后由 GL 调用；匿名 namespace，仅非 NDEBUG 编译 | fprintf 到 stderr，输出 type/severity/id/message；空 message 写占位，不修改渲染状态 | Debug callback 注册有效；GL 同步调用，不在此访问 app/world | 不抛 C++ 领域异常，无写入成功反馈；Free 在上下文有效时注销 | [实现][renderer-src]；I1/I5 |

### 生命周期与清理顺序

app 创建 Window → AttachContext → Init/LoadTextureAtlas → 每帧 AppendMesh/SetSelection → Render → Present。性能采样时 app 单独负责 GpuTimer 的 BeginFrame/EndFrame/Poll。

退出时 app 先结束采样并释放 GpuTimer，再释放 world 与 Renderer 资源，最后销毁 Camera/Registry、Window/上下文并结束本模块持有的 SDL video 责任。Init 失败仍由 app 调用 Free 分段清理部分资源；同进程多次重建、跨上下文使用和并行调用不属于已保证能力。当前仍由 app 经 GL 桥接 Present；未来 T3 收入 Renderer 的统一呈现职责另见 [T2-S4 交接](../../../milestones/m3-t2/s4-platform-handoff.md)，不将设计交接视作生产现代后端已经接管。

[GpuTimer 全部真实 API](GpuTimer%20Class.md#生命周期与接口)单独维护，不再写不存在的统一 Begin/End。

### RenderStats

size_t vertices/upload_bytes/draw_calls=0、double upload_cpu_ms=0；每帧 Batch 累加/app 复制到 Frame，不拥有资源，不代表 GPU 整帧时间。

### DeviceInfo

string vendor/renderer/version、int msaa_samples=0、bool nvx_memory_supported=false；AttachContext 后缓存，Device const 引用不跨重建/销毁保存。不是设备 handle。

### DeviceMemory

optional<uint64_t> dedicated_kib/available_kib；NVX 设备级驱动估计，单位 KiB，不是本进程显存/温度；缺失不造零。

### 既有实现与限制

T0 去除公开 GL/批次，query 用无 SDK uint；内部资源/白盒头窄白名单。Shader/ShaderProgram 无 GPU 释放析构，须显式 Destroy；Texture 支持 ID 移动但非虚析构，细节各对象维护。
单上下文进程内状态，不保证并行/完整多次重建。[实现](../../../../game/modules/renderer/src/renderer.cpp)、[公开头](../../../../game/modules/renderer/include/symocraft/renderer/renderer.h)、[Stats](../../../../game/modules/renderer/include/symocraft/renderer/render_stats.h)、[T0 报告](../../../milestones/m3-t0/README.md)。
原正式库/边界/短测、static 字节一致、用户玩法结论保留；长期泄漏/多实例/并发/所有驱动异常未验证；2026-10-06 未重跑，原 T2（现 T3）未实施。

## 本次变更

无。

## 后续考虑

T3 实例所有权/handle/后端另行批准，在 T2 SDL3 迁移验收后冻结；不以对象整理替代实现。

[renderer-src]: ../../../../game/modules/renderer/src/renderer.cpp
[memory-src]: ../../../../game/modules/renderer/src/performance_memory.cpp
