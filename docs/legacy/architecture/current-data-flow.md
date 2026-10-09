---
tags:
  - area/legacy
---

# M0 架构与数据流

> 历史架构参考：M0 静态源码基线。文中的“当前”指该阶段，不代表最新实现；归档不表示相关机制全部废弃。源码链接固定到迁移前 `cadc349` 快照，保留旧路径定位；该快照不等同于本篇原阶段的精确版本，历史结论仍以原报告为准。参见 [历史架构索引](README.md)；后续设计约定见 [项目范围](../../spec/project-scope.md)，不据此推定重构已经完成。

## 范围与证据等级

本文描述 M0 阶段对现有源码的只读核对，不是重构后的目标架构。本文仅汇总静态证据；实际构建、启动结果及尚未测量的项目见 [M0 报告](../../milestones/m0/README.md)。

- **静态事实**：可以直接从当前函数、数据成员或调用顺序确认。
- **待验证影响**：从代码推导出的风险，需要测试或运行证据才能确定触发场景和严重程度。
- **后续建议**：尚未实施的设计方向，不代表项目已经具备该能力。

源码链接用于定位迁移前快照；旧行号已不作为导航锚点，应按函数名查找，不用该快照重新推断 M0 结果。

## 当前模块

| 模块 | 当前职责与关键状态 | 主要入口 |
| --- | --- | --- |
| Application | 初始化、主循环、输入轮询、帧时间、退出；提供全局窗口、相机和 Registry 访问 | [application.cpp](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/core/application.cpp) |
| Window | GLFW 窗口、OpenGL 上下文、交换缓冲、事件轮询 | [window.cpp](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/core/window.cpp) |
| ECS | 自定义实体与组件存储；Transform、Physics、Character 系统 | [registry.h](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/include/core/ECS/registry.h)、[registry.cpp](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/core/ECS/registry.cpp) |
| World / ChunkManager | 创建玩家、世界到区块坐标转换；全局区块映射及方块查询、编辑、批量更新 | [world.cpp](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/world/world.cpp)、[chunk_manager.cpp](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/world/chunk_manager.cpp) |
| Chunk | 方块存储、邻区块指针、地形与植被生成、CPU 网格、脏状态 | [chunk.h](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/include/world/chunk.h)、[chunk.cpp](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/world/chunk.cpp) |
| PlayerController | 3 单位射线选择、左右键编辑、方块类型名称输出 | [playercontroller.cpp](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/playercontroller/playercontroller.cpp) |
| Renderer / Batch | shader、顶点批次、CPU 到 GPU 上传、绘制和选择框 | [renderer.cpp](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/renderer/renderer.cpp)、[batch.hpp](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/include/renderer/batch.hpp) |
| Camera | 以 ECS Transform 保存相机变换，生成视图和投影矩阵 | [Camera.cpp](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/camera/Camera.cpp) |

模块已有目录划分，但通过 `Application::Get*()`、命名空间全局状态和跨模块调用连接，目录边界不等于可独立测试的边界。

## 启动与退出

入口 [main](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/main.cpp) 无条件依次调用 `Application::Init()`、`Run()`、`Free()`。

```text
Application::Init
  -> Window::Init / GetWindow
     -> 请求 OpenGL 4.6 Core，上下文创建，GLAD，启用 VSync
  -> 注册 ECS 组件
  -> Renderer::Init
     -> GetCamera 创建相机实体
     -> GLAD、shader、Batch、方块配置
  -> World::Init / CreatePlayer

Application::Run
  -> 记录 previous_frame_time
  -> 纹理图集转纹理数组、安装鼠标回调
  -> 初始化随机噪声
  -> 创建全部区块
  -> 对每个区块：GenerateTerrain -> GenerateVegetation
  -> RearrangeChunkNeighborPointers
  -> 设置玩家出生位置 (0, 140, 0)
  -> 进入主循环

Application::Free
  -> 销毁窗口 / GLFW 终止
  -> 释放区块 CPU 数据
  -> Renderer::Free
  -> Registry::Clear
```

该顺序来自 [初始化和启动](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/core/application.cpp)、[世界生成](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/core/application.cpp)、[退出](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/core/application.cpp)。OpenGL 版本是代码请求值，不是已测得的驱动能力。

## 每帧与编辑数据流

```text
计算 delta_time，更新输入防抖
  -> processInput
  -> PlayerController::DoRayCast
     -> ChunkManager::SetBlock / RemoveBLock
     -> 修改方块，将本块及必要邻块标记 ToBeUpdated
  -> TransformSystem::Update
  -> Physics::Update：累计时间，以 1/120 秒步长更新位置和碰撞
  -> Character::Player::Update：设置运动速度、跳跃状态、同步相机
  -> ChunkManager::UpdateAllChunks
     -> 只对需要更新的非外围区块 GenerateRenderData
  -> ChunkManager::LoadAllChunks
     -> 每个有效区块的顶点 -> Batch::AddVertex -> glNamedBufferSubData
  -> Renderer::Render
     -> 方块批次绘制，选择框上传和绘制
     -> Batch::Draw 后清零批次计数
  -> SwapBuffers -> PollInt
```

参见 [主循环](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/core/application.cpp)、[物理固定步进](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/core/ECS/Systems/physics_system.cpp)、[角色更新](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/core/ECS/Systems/character_system.cpp)、[脏状态传播](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/world/chunk.cpp)。

要区分两个事实：**不是每帧重建所有区块网格，但当前每帧重新上传所有有效区块网格**。网格重建在主线程执行；当前路径没有后台生成任务、网格结果队列或按帧上传预算。

## 世界规模与确定性

- 单区块为 `16 x 256 x 16`，见 [区块常量](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/include/world/chunk_manager.h)。
- `chunk_radius = 10`，创建坐标为 `[-10, 10] x [-10, 10]`，因此是固定的 441 个区块，不是无限地图。
- 外围缺邻居的区块被标记 `m_is_fringe_chunk`，跳过网格生成和上传；内圈为 19 x 19，即 361 个区块。见 [邻居和外围标记](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/world/chunk_manager.cpp)、[更新与上传](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/world/chunk_manager.cpp)。
- 当前主循环没有随玩家移动创建或卸载区块的调用。走到世界外围时的预期行为需要明确，不能以“会继续生成世界”验收现版本。
- 随机数引擎来自 `random_device`；日志中的 `seed` 不是三个噪声源及植被生成的完整重放输入。见 [随机状态](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/include/world/chunk.h)、[InitializeNoise](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/world/chunk.cpp)。当前不可把“记录 seed”当作已经具备确定性回放。

## 所有权与线程边界

| 对象 | 当前所有者 / 使用者 | 当前释放方式与注意点 |
| --- | --- | --- |
| Window、Camera、Registry | `Application::Get*()` 中静态保存的 `new` 对象，调用者获取指针或引用 | `Free` 清理部分内部状态，但对象不是由作用域自动管理 |
| 区块映射 | `chunk_manager.cpp` 文件作用域静态容器 | `FreeAllChunks` 手工调用各 Chunk 的 `Free`，不清空容器 |
| 方块与 CPU 网格 | Chunk 的原始指针 `m_local_blocks`、`m_vertex_data` | 手工分配、重分配和释放；Chunk 可被按值复制，指针随之浅拷贝 |
| 邻区块 | Chunk 的四个非拥有原始指针 | 指针可为空；未来加入卸载时必须先定义失效规则 |
| VAO / VBO、shader | 全局 Batch 和 Renderer 内的 shader 对象 | Batch 的 `Free` 目前只释放 CPU 数组；Renderer 仅销毁 block shader |
| 纹理数组 | `Run` 的局部 TextureArray，保存 GL 句柄 | 类型没有析构释放逻辑，见 [texture.h](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/include/renderer/texture.h) |

现有 `GlobalThreadPool` 的成员和使用入口在 Application 中被注释，没有接入上述运行路径。不能把该线程池的源码问题写成已经发生的游戏线程故障，也不能未经复核就启用它。噪声/随机状态、mesh 临时数组和邻区块写入均未形成独立任务所有权，直接把生成循环放进线程池不是安全的优化步骤。

## M1 / M2 重点风险

| 编号 | 源码确认的事实 | 尚待验证的影响 / 建议证据 |
| --- | --- | --- |
| R1 生命周期 | 初始化失败没有传回 main 阻止 Run；退出先销毁上下文，随后 shader 释放调用 GL。见 [main](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/main.cpp)、[退出](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/core/application.cpp)、[Renderer::Free](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/renderer/renderer.cpp) | 不支持 GL、资源失败和正常退出时是否崩溃。需要故障注入、退出日志与 GL 调试信息 |
| R2 资源定位 | shader、YAML、纹理路径依赖 `../assets`；Renderer 没有检查 shader 编译返回值。见 [Renderer::Init](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/renderer/renderer.cpp)、[纹理加载](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/core/application.cpp) | 不同工作目录和独立发布目录能否启动；缺失资源是否清楚报错并安全退出 |
| R3 生成顺序 | 每块地形后立即生成可跨区块写入的植被，最后才补齐邻居；后生成地形会清零本块。缺邻居的 SetLocalBlock 可落入本地索引。见 [生成循环](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/core/application.cpp)、[SetLocalBlock](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/world/chunk.cpp)、[清零](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/world/chunk.cpp) | 跨区块树木丢失、错误方块写入等是否触发。需要边界生成与边界编辑用例 |
| R4 网格内存 | 顶点计数为 uint16，容量为 UINT16_MAX，先写后检查；末尾 realloc 返回值未保存。见 [计数](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/include/world/chunk.h)、[写入](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/world/chunk.cpp)、[重分配](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/world/chunk.cpp) | 大网格回绕、越界和重分配搬迁后的失效指针。需要高暴露面测试及可用的内存检查工具 |
| R5 Batch 契约 | 模板步长硬编码为 BlockVertex3D，但也实例化 LineVertex3D；容量检查不计新增数量并允许当前数量等于容量。见 [步长](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/include/renderer/batch.hpp)、[批量上传](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/include/renderer/batch.hpp)、[HasRoom](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/include/renderer/batch.hpp) | 选择框布局异常、满批次错误写入。需画面证据、容量测试与 GL 错误检查 |
| R6 上传成本 | 每帧 LoadAllChunks 上传全部有效网格。见 [上传循环](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/world/chunk_manager.cpp)、[GPU 上传](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/include/renderer/batch.hpp) | 是否为主要瓶颈尚未测量。需 CPU/GPU 帧时间和每帧上传字节数，不只平均 FPS |
| R7 时间步进 | 首帧 delta_time 包含纹理及世界生成耗时；物理补步没有上限；角色速度设置在物理之后。见 [计时起点](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/core/application.cpp)、[系统顺序](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/core/application.cpp)、[补步](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/core/ECS/Systems/physics_system.cpp) | 首帧或恢复窗口后长时间追赶、出生异常及输入延迟。需启动、长帧和恢复用例 |
| R8 可重复性 | 当前记录的 seed 不能重放全部随机状态。见 [InitializeNoise](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/world/chunk.cpp)、[植被随机](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/world/chunk.cpp) | 不同运行的性能和画面差异不能直接归因于重构。需固定测试场景或完整可重放输入 |

## 后续方向，尚未实施

M1 优先解决可重复构建、资源定位和启动失败处理；M2 优先让既有玩法及内存/生命周期稳定，再开始架构迁移。后续可逐步明确“世界数据 -> CPU 网格结果 -> 渲染线程 GPU 资源”的所有权，增加不依赖窗口的测试，并基于测量决定持久网格、剔除、并发或其他优化。

不预先承诺重写 ECS、替换容器或启用线程池。架构验收看依赖、所有权、失败处理和可测试性；性能验收看相同场景与配置下的实测对照，而不是目录数量或抽象层数。
