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

本目录记录 M3-T0 实际实现的模块职责、接口、内部状态和资源所有权，服务工程维护与源码学习。它不是“仅换文件夹”的完成声明，也不意味着后续 T1/T2/T3/T4 内部重设计已经完成。任务契约见 [M3-T0 Spec](../../spec/M3-T0-模块软硬边界.md)，历史对照见 [legacy 架构](../../legacy/architecture/README.md)。

最终验证、安装包身份、用户本轮玩法反馈与保留风险统一见 [M3-T0 交付与验收报告](../../milestones/m3-t0/README.md)。技术验证完成不代替用户整体验收，也不自动开启 T1。

## 模块笔记

| 模块 / 构建目标 | 功能与数据流 | 类设计与状态契约 |
| --- | --- | --- |
| foundation / `symocraft_foundation` | [基础契约](foundation/Foundation-基础契约.md) | [值、诊断与共享实现](foundation/Foundation-基础契约-类设计.md) |
| scene / `symocraft_scene` | [共享场景数据](scene/Scene-共享场景数据.md) | [顶点、网格视图与相机描述](scene/Scene-共享场景数据-类设计.md) |
| assets / `symocraft_assets` | [资源读取与解码](assets/Assets-资源读取与解码.md) | [路径、字节与图片](assets/Assets-资源读取与解码-类设计.md) |
| world / `symocraft_world` | [世界与 CPU 网格](world/World-功能.md) | [Chunk 与存储](world/World-类设计.md)、[Generator](world/Generator-类设计.md) |
| ecs / `symocraft_ecs` | [存储边界](ecs/ECS-存储边界.md) | [Registry 与组件存储](ecs/ECS-存储边界-类设计.md) |
| simulation / `symocraft_simulation` | [玩家与玩法推进](simulation/Simulation-功能.md) | [输入、组件与系统](simulation/Simulation-类设计.md)、[Camera](simulation/Camera-类设计.md) |
| platform / `symocraft_platform` | [窗口与输入](platform/Platform-窗口与输入-功能.md) | [Window](platform/Window-类设计.md) |
| renderer / `symocraft_renderer` | [显式场景输入](renderer/Renderer-显式场景输入-功能.md) | [图形资源和绘制](renderer/Renderer-类设计.md) |
| telemetry / `symocraft_telemetry` | [采样与导出](telemetry/Telemetry-采样与导出.md) | [Session 与自有数据](telemetry/Telemetry-采样与导出-类设计.md) |
| app / `SymoCraft` | [运行编排](app/App-运行编排-功能.md) | [Application](app/Application-类设计.md)、[休眠源码清单](app/App-休眠源码清单.md) |
| 构建 / 测试边界 | [构建与测试边界](build/构建与测试边界.md) | [构建契约设计](build/构建契约设计.md) |

独立基准执行器继续在 [Benchmark 笔记](../benchmark/Benchmark-执行与导出.md) 与 `tools/benchmark` 中维护；它通过文件协议控制游戏进程，不链接游戏模块。日常构建入口见 [CLion 指南](../build/build-and-clion.md)，注意文首适用阶段；现有 [CMake 边界](../build/CMake-模块边界.md) 与本轮构建笔记互相补充。

## 编译依赖

箭头表示编译使用关系，而不是运行先后。公开依赖只传播公开头真正需要的类型；YAML、图片解码、噪声、哈希表、GLFW/GLAD 等只通过受限路径提供给授权实现。

```text
app -> simulation, world, renderer, platform, assets, telemetry, ecs, scene, foundation
simulation -> ecs, world(私有实现查询), scene, foundation
world -> scene, foundation
renderer -> scene, assets, platform 的受限图形桥接, foundation
assets / ecs / platform / telemetry -> foundation
scene -> foundation
```

scene 是有真实消费者的 INTERFACE 数据契约，其他八个模块是拥有实际 cpp 的 STATIC 库。测试链接这些正式库，不重复编译生产实现；world 的 Chunk/map 和 renderer 实现头只向指定白盒测试开放，不作为正常调用者的兜底头路径。准确目标与第三方依赖以各模块 CMake 和生成的依赖清单为准。

## 运行数据流

```text
OS -> platform 快照/有序指针事件 -> app 映射 -> simulation
simulation <-> world 公开查询；交互产生自有编辑请求
app 提交编辑 -> world 脏标记/CPU 网格
world 同步网格视图 + simulation 相机/选块值 -> app -> renderer
renderer GPU 样本 + platform 进程内存 + app CPU 时段 -> telemetry
telemetry 文件 -> 独立 benchmark 读取与校验
```

世界网格视图只在同步访问期间借用，renderer 当场复制进原批次；没有为了模块边界新增一份完整世界快照。Camera 借用 Registry，不借用窗口；原生窗口句柄只在受限图形桥接中使用。图形资源释放先于窗口/上下文销毁，退出编排由 app 负责。

## 验证与限制

[源码迁移清单](source-migration.tsv) 按 S0 原路径和原始 SHA256 对照当前所有者，并区分活动实现与保留的休眠源码；新增实现及实际构建目标以生成的目标源码清单为准。

每篇功能笔记区分实现、实测和未测专项。最终独立头消费者、负向依赖检查、Debug/Release/CPU-only、独立 benchmark、真实驱动探针及安装包人工玩法复查均已建立对应证据，详见最终报告；它们不能互相代替，也不覆盖任意故障或后续内部重设计。完整生成物和日志保存在 `out/m3-t0`，选定小型证据归档于阶段报告，不在索引重复维护测试数量。

本轮不引入多线程生成、随移动区块流式加载或新图形后端。世界全局集合、物理步进预算及旧 ECS 内部风险仍按各模块文档明示，后续阶段应在已经成立的边界内解决，而不是恢复跨模块私有访问。

本目录笔记统一使用 `area/architecture`。当前只更新文档标签和导航，没有修改用户 `.obsidian/graph.json`；第九组图谱颜色仍未配置和验证。
