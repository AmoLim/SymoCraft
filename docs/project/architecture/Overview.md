---
type: 功能
status: 已验证（T0范围）
project: Symocraft
module: Architecture
created: 2026-10-05
tags:
  - area/architecture
---

# 当前模块架构索引

本目录记录实际模块职责、接口、状态和资源所有权。T0 模块边界已验证，T1 技术交付完成、已知问题延期且节点待批准；不据此宣布 T2/T3/T4/T5 完成。任务契约见 [T0](../../spec/M3-T0-模块软硬边界.md)、[T1](../../spec/M3-T1-世界模块重构.md)，历史对照见 [legacy 架构](../../legacy/architecture/README.md)。

阶段结果与包身份统一见 [T0 报告](../../milestones/m3-t0/README.md)、[T1 报告](../../milestones/m3-t1/README.md)。技术验证不代替用户节点批准。

跨阶段渲染交接从 [T3 CPU 契约清单](../../spec/M3-T1-T3-CPU契约清单.md) 进入，区分 T1 已交付数据与 T3-R1 尚需冻结的输入；T2 SDL3 迁移不改写 CPU 数据语义。

2026-10-07 规划编号更新：用户确认将 SDL3 迁移前移为新 T2，并已填写方案，见 [T2 正式计划](../../spec/M3-T2-SDL3迁移.md#已确定的决策)。原 T2 渲染任务后移为 T3，原应用/平台/模拟与 ECS/内存计划分别为 T4/T5。先完成 T2 迁移验收，再冻结 T3 渲染器 spec；正式化不表示准备实验通过、SDL3 已接入或获得实施授权，回归预算仍按已选 D12/A 单独确认。这里只同步未来任务导航，不把计划写成当前实现。

## 模块笔记

逐个审核对象请从 [对象笔记覆盖清单](对象笔记覆盖清单.md) 进入；辅助类型链接到具名小节，未实现候选不当作当前类。2026-10-06 完成数据/系统拆分与单对象整理，新笔记仅源码核对，保持草稿；不改变 T0 游戏验收状态。

| 模块 / 构建目标 | 功能 / 系统流程 | 独立对象 / 数据契约 |
| --- | --- | --- |
| foundation / `symocraft_foundation` | [基础契约](foundation/Foundation-基础契约.md)、[诊断摘要](foundation/Foundation-基础契约-类设计.md) | [Value](foundation/Value-类设计.md) |
| scene / `symocraft_scene` | [共享场景数据](scene/Scene-共享场景数据.md) | [顶点 / CameraView / MeshView / MeshData](scene/Scene-共享场景数据-数据设计.md) |
| assets / `symocraft_assets` | [资源读取与解码](assets/Assets-资源读取与解码.md)、[namespace API](assets/Assets-资源读取与解码-类设计.md) | [Image](assets/Image-数据设计.md) |
| world / `symocraft_world` | [世界功能](world/World-功能.md)、[无状态 API](world/World-namespace-API.md) | [VoxelWorld](world/World-类设计.md)、[BlockDefinition](world/BlockDefinition-类设计.md)、[Chunk](world/Chunk-类设计.md)、[Mesher](world/ChunkMesher-类设计.md)、[公共值](world/World-数据设计.md)、[Block](world/Block-数据设计.md)、[Generator](world/Generator-类设计.md) |
| ecs / `symocraft_ecs` | [存储边界](ecs/ECS-存储边界.md) | [Registry](ecs/ECS-存储边界-类设计.md)、[组件存储数据](ecs/ECS-组件存储-数据设计.md)、[ComponentContainer](ecs/ComponentContainer-类设计.md)、[Iterator](ecs/Iterator-类设计.md)、[RegistryViewer](ecs/RegistryViewer-类设计.md) |
| simulation / `symocraft_simulation` | [玩法功能](simulation/Simulation-功能.md)、[系统流程](simulation/Simulation-系统设计.md) | [输入 / 组件 / 交互](simulation/Simulation-数据设计.md)、[Camera](simulation/Camera-类设计.md)、[FixedStepBudget](simulation/FixedStepBudget-类设计.md) |
| platform / `symocraft_platform` | [窗口与输入](platform/Platform-窗口与输入-功能.md)、[采样 / 私有桥接 API](platform/Platform-namespace-API.md) | [Window / 输入值](platform/Window-类设计.md) |
| renderer / `symocraft_renderer` | [显式场景输入](renderer/Renderer-显式场景输入-功能.md)、[绘制 namespace API](renderer/Renderer%20namespace%20API.md) | [GpuTimer](renderer/GpuTimer%20Class.md)、[Batch](renderer/Batch-类设计.md)、[Shader](renderer/Shader-类设计.md)、[ShaderProgram](renderer/ShaderProgram-类设计.md)、[Texture](renderer/Texture-类设计.md)、[TextureArray](renderer/TextureArray-类设计.md) |
| telemetry / `symocraft_telemetry` | [采样与导出](telemetry/Telemetry-采样与导出.md)、[统计 / YAML API](telemetry/Telemetry-namespace-API.md) | [Session](telemetry/Session-类设计.md)、[采样数据](telemetry/Telemetry-采样与导出-类设计.md) |
| app / `SymoCraft` | [运行编排](app/App-运行编排-功能.md) | [Application](app/Application-类设计.md)、[休眠源码清单](app/App-休眠源码清单.md) |
| 构建 / 测试边界 | [构建与测试边界](build/构建与测试边界.md) | [构建契约设计](build/构建契约设计.md) |

独立基准执行器继续在 [Benchmark 笔记](../benchmark/Benchmark-执行与导出.md) 与 `tools/benchmark` 中维护；它通过文件协议控制游戏进程，不链接游戏模块。日常构建入口见 [CLion 指南](../build/build-and-clion.md)，注意文首适用阶段；现有 [CMake 边界](../build/CMake-模块边界.md) 与本轮构建笔记互相补充。

## 编译依赖

箭头表示编译使用关系，而不是运行先后。公开依赖只传播公开头真正需要的类型；YAML、图片解码、噪声、哈希表、GLFW/GLAD 等只通过受限路径提供给授权实现。

```text
app -> simulation, world, renderer, platform, assets, telemetry, ecs, scene, foundation
simulation -> ecs, world(公开类型及显式查询), scene, foundation
world -> scene, foundation
renderer -> scene, assets, platform 的受限图形桥接, foundation
assets / ecs / platform / telemetry -> foundation
scene -> foundation
```

scene 是有真实消费者的 INTERFACE 数据契约，其他八个模块拥有实际 STATIC 实现。测试链接正式库，不重复编译生产源码；world 的 Chunk/Mesher 和 renderer 实现头只向指定白盒测试开放。准确依赖以各模块 CMake 和生成清单为准。

## 运行数据流

```text
OS -> platform 快照/有序指针事件 -> app 映射 -> simulation
simulation <-> world 公开查询；交互产生自有编辑请求
app 提交编辑 -> world 脏标记/CPU 网格
world 同步网格 view + simulation 相机/选块值 -> app -> renderer
renderer GPU 样本 + platform 进程内存 + app CPU 时段 -> telemetry
telemetry 文件 -> 独立 benchmark 读取与校验
```

世界网格 view 只在同步访问期间借用，renderer 当场复制进原批次；没有为了模块边界新增一份完整世界快照。Camera 借用 Registry，不借用窗口；原生窗口 handle 只在受限图形桥接中使用。图形资源释放先于窗口/上下文销毁，退出编排由 app 负责。

## 验证与限制

[T0 源码迁移清单](source-migration.tsv)按原路径与 SHA256 记录当时迁移；T1 新增/删除输入以本轮报告清单为准，不追改历史证据。

每篇功能笔记区分实现、实测和未测专项。独立头消费者、负向依赖检查、构建矩阵、真实驱动探针及人工结果按阶段归档；T1 已知问题延期范围见其报告，不以自动测试替代人工结论，也不覆盖任意故障或后续重设计。完整生成物和日志分别保存在 `out/m3-t0`、`out/m3-t1`，选定证据归对应报告，不在索引重复维护测试数量。

本轮不引入多线程生成、随移动区块流式加载或新图形后端。T1 已移除世界全局集合；物理步进预算和旧 ECS 风险仍按对应笔记保留，不恢复跨模块私有访问。

本目录笔记统一使用 `area/architecture`。本次整理不写入用户 `.obsidian/graph.json`，不新增对象 tag/颜色；图谱设置与 UI 呈现以实际用户配置为准，本次未作 UI 验证。
