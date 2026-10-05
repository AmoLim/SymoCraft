---
type: 功能
status: 已验证（T0范围）
project: Symocraft
module: world
created: 2026-10-05
tags:
  - area/architecture
---

# World 世界数据与 CPU 网格

关联类：[Chunk 与存储](World-类设计.md)、[Generator](Generator-类设计.md)。关联规格：[M3-T0](../../../spec/M3-T0-模块软硬边界.md)。

最终验证：[M3-T0 交付与验收报告](../../../milestones/m3-t0/README.md)。

## 当前设计

`symocraft_world` 拥有方块格式、有限区块集合、确定性地形与固定回归场景、CPU 网格。它不创建玩家，不读取窗口，不持有 GPU 资源。T0 沿用已有算法，并未完成 T1 的世界类架构重设计。

生产代码位于 [world 模块](../../../../game/modules/world/CMakeLists.txt)。公开头为 `world.h`、`block.h`、`constants.h`、`chunk_manager.h`、`generation.h`、`test_scene.h`、`benchmark_workload.h`；`Chunk` 和 robin_hood 存储接口仅位于私有 `src/`。

依赖：公开依赖 foundation、scene；YAML、噪声和哈希表仅为实现依赖。方块配置接收由应用解析好的资源路径，配置语义由 world 校验。世界报告返回自有 `Data::Value`，不暴露 YAML 节点。

## 本次变更

### 目标与流程

把世界的输出从“直接写入全局 GPU 批次”改为“同步提供 CPU 网格视图”。原网格三角形顺序、哈希容器遍历顺序、帧内打包时点不变。

```text
Settings/seed -> Generator -> 区块方块数据
玩家/基准编辑请求 -> GetBlock/TrySetBlock -> 脏标记
UpdateAllChunks -> 每区块自有顶点 vector
VisitMeshes -> app 立即转交 renderer -> 原批次复制/上传
Describe -> Data::Value -> app/telemetry 导出
```

`TrySetBlock` 返回实际写入是否成功，供原先直接操作 Chunk 的基准场景使用；它不额外复制世界或重新生成网格。`SetBlock`/`RemoveBLock` 保留原来的日志和边界行为。缺失区块查询返回 `NULL_BLOCK`。

### 关键约束与取舍

- 区块及网格归 world 持有。网格视图仅在回调期间有效，回调不得修改世界或保存视图；应用当场复制到现有 renderer 批次。
- 没有新增逐帧网格快照、顶点二次中转容器或排序。`VisitMeshes` 只遍历原容器、跳过空网格和边缘区块。
- 所有更新、访问和销毁仍在主线程执行；原网格临时数组、全局区块集合不是并发接口，不声称支持后台生成或流式加载。
- `Chunk` 中已删除 GPU 间接命令和命令索引，顶点数量来自实际 vector 大小，避免重复计数状态。
- `Generator` 仅隐藏噪声库实现；seed 混合、采样顺序、植被随机数及摘要格式保持原样。
- 生成半径参数在清除旧世界前验证。生成途中分配失败不保证回滚到旧世界，应用应进入退出清理；这不是 T0 新增事务机制。

### 验收案例

| 状态 | 场景 | 预期行为 |
| --- | --- | --- |
| [x] | 固定 seed 重复生成、不同插入顺序 | 重复摘要与内容一致，植被随机流独立；旧新实际 world-summary 字节一致 |
| [x] | 相邻区块边界编辑与极端网格 | 脏标记和顶点数量正确，不出现 16 位溢出 |
| [x] | 真实方块配置缺项、重复 ID、纹理越界 | 保留既有明确错误；非法配置不替换已有完整格式表 |
| [x] | CPU-only 配置及独立头消费者 | 无 GLFW/GLAD/窗口依赖 |
| [x] | VisitMeshes 与运行采样 | 无额外顶点中转，旧新首次上传字节相同，static/edit 短采样有效 |

环境 / 运行入口：Windows x64、MSVC，统一 `test/unit/world` 和集成测试。2026-10-05 最终 Debug、Release、CPU-only 与独立头检查通过；旧新 world-summary 字节一致，固定世界元数据和首次上传字节一致。局部 CPU 日志见 [world-simulation-debug.log](../../../milestones/m3-t0/evidence/world-simulation-debug.log)，真实驱动、安装包与完整矩阵见最终报告。T1 的世界实例、版本和流式失效规则设计仍未实施。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| M3-T1 | 实例化世界/区块拥有者，冻结查询、编辑、网格版本与失效契约 |
| 后续流式区块或后台网格任务 | 显式任务依赖和只读快照，不能直接并发使用当前视图/临时数组 |
| 测得打包/上传开销是瓶颈 | 按版本增量提交；不能在 T0 偷改已冻结的全量上传基准 |
