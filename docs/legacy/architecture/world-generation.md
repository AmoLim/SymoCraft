---
tags:
  - area/legacy
  - topic/world
---

# 可复现世界生成

> 历史架构参考：M2-T2。文中的“当前”指该阶段，不代表最新实现；归档不表示相关机制全部废弃。源码链接固定到迁移前 `cadc349` 快照，保留旧路径定位；该快照不等同于本篇原阶段的精确版本，历史结论仍以原报告为准。参见 [历史架构索引](README.md)；后续设计约定见 [项目范围](../../spec/project-scope.md)，不据此推定重构已经完成。

## 范围与契约

M2-T2 新增生成版本 `1`，只恢复固定世界的可重复输入，不增加存档、无限世界、并发生成或通用回放。相同生成版本、源码及依赖、MSVC 构建环境、seed、配置和方块格式，在相同执行路径下应产生相同方块内容。

**不保证不同编译器、平台、浮点选项或生成版本之间位级一致。** 本次 Debug/Release 恰好取得相同摘要是观测结果，不升级为跨构建契约。M2-A 旧日志中的 seed 没有记录全部随机状态，不能用新版本补救或重放旧世界。

实现入口：[生成模块](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/world/generation.cpp)、[接口](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/include/world/generation.h)、[区块生成](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/world/chunk.cpp)、[启动参数](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/core/startup_options.cpp)。实测证据见 [M2-T2 报告](../../milestones/m2-t2/README.md)，测试夹具见 [固定场景](../../project/benchmark/testing/reproducible-scenes.md)。

## 数据流与所有权

```text
argv -> StartupOptions -> Application::PrepareWorld
  -> 解析实际 seed -> Generation::Settings
  -> Generator 拥有配置副本和三个噪声实例
  -> ChunkManager 创建完整区块方阵、建立邻居
  -> 全部地形 -> 全部植被（固定坐标顺序）
  -> terrain_digest
  -> 可选 TestScene 覆盖几何 -> scene_digest
  -> 可选固定编辑序列 -> final_digest
  -> YAML 摘要和初始姿态
      -> 无窗口：输出摘要 -> 清理
      -> 游戏：玩家/相机 -> CPU 网格 -> 既有批次 -> GPU
```

`Generator` 不拥有区块，也不创建 GL 对象；`ChunkManager` 仍拥有区块，区块容器仍拥有方块和 CPU 网格。每个植被生成调用持有自己的 `mt19937` 值，不共享随机游标。生成完成后的渲染路径不变，本项没有实现按需 GPU 上传。

噪声采样、世界写入、摘要和场景安装均在调用线程执行。旧 FastNoiseLite 1.0.1 的采样接口没有 `const`，适配器把噪声数组标为 `mutable`，并不因此承诺线程安全。区块管理器、邻区块写入和网格临时数据也仍是共享可变状态，不能直接把循环提交给线程池。

## 接口与失败边界

| 接口 | 输入与输出 | 约束 |
| --- | --- | --- |
| `ParseStartupOptions(span<string_view>)` | 参数视图 -> `StartupOptions` | 错误组合抛 `invalid_argument`；返回的 checkpoint 视图引用调用方参数存储，`main` 中由进程期 argv 保持有效 |
| `Generator(Settings)` | seed、半径、植被开关 -> 局部生成器 | 半径只接受 2..10；此限制保护当前固定世界，不是流式世界接口 |
| `Height(x,z)` | 世界整数水平坐标 -> 浮点地表高度 | 不依赖调用次数、区块创建顺序或已存在方块 |
| `VegetationRandom(chunk_x,chunk_z)` | 区块坐标 -> 新 RNG 值 | 同一输入返回同一初始序列，调用者消费互不影响 |
| `Build(settings)` | 配置 -> 重建全局固定方阵 | 先验证半径，再清理旧世界；分配失败不承诺事务回滚，由应用退出清理 |
| `Populate(generator)` | 已存在的完整方阵 -> 重写方块 | 校验数量与所有坐标，重建邻居，清除旧方块/CPU 网格后生成；不接受缺块或任意稀疏集合 |
| `BlockDigest()` | 当前区块 -> 带算法前缀的字符串 | 未完整分配的区块抛 `logic_error`；不创建网格或访问 GPU |
| `Describe(settings)` | 配置、当前方块格式 -> YAML 节点 | 仅描述，不代表生成已经成功 |
| `FindSpawnPosition()` | 当前方块 -> 玩家初始位置 | 保留中心附近搜索并避开水/树的规则；找不到时抛异常，不是可达性证明 |

生产命令行固定半径 `10`、植被开启，即 441 个区块，外围一圈的地形初始化为空且不绘制，内圈最多 361 个参与绘制。外围仍可接收邻块树冠的跨界写入，沿用原有边缘语义，不能假设最终所有外围格都为空。较小半径及关闭植被只用于 C++ 测试，不悄悄缩小游戏世界以获得好看的性能数字。邻居关系必须先于地形生成；外围判断使用邻居建立后的 `m_is_fringe_chunk`。

## Seed 与噪声配置

普通无参数开局调用一次 `random_device` 取得 `uint32_t` seed，之后不再读取随机设备；随机开局不保证每次 seed 都唯一。`--seed` 接受十进制 `0..4294967295`，拒绝符号、溢出、尾随字符和重复选项。测试场景未指定 seed 时默认 `424242`。日志的 `seed_source` 区分 `random`、`explicit`、`scene-default`。

派生函数 `Mix` 的所有乘法均按无符号 32 位回绕：

```text
v ^= v >> 16
v *= 0x7feb352d
v ^= v >> 15
v *= 0x846ca68b
return v ^ (v >> 16)

noise[i] = Mix(seed ^ (0x9e3779b9 * (i+1))) & 0x7fffffff
vegetation(x,z) = Mix(Mix(seed ^ 0xa511e9b3)
                     ^ Mix(uint32(x)) ^ Mix(uint32(z) ^ 0x63d83595))
```

噪声 seed 先掩码再转有符号 `int`；负坐标转无符号遵循模 2^32 规则。不同区块采用不同输入派生，不承诺哈希无碰撞或统计独立。选择简单的显式整数混合和标准 `mt19937`，避免 `std::hash` 实现差异及共享随机游标；这不是密码学随机源。

| 项目 | 版本 1 值 |
| --- | --- |
| 库/算法 | 本仓库 FastNoiseLite 1.0.1，OpenSimplex2 + FBm |
| 三层频率 | 0.00573、0.02、0.1 |
| 三层权重 | 1、0.2、0.03 |
| Octaves / Lacunarity / Gain | 8 / 1.6 / 0.5 |
| 输入与混合 | x、z 除以 1.5；噪声映射到 0..1，加权后除总权重并限制到 0..1 |
| 高度 | 混合值的 1.19 次方映射到 55..145；区块使用整数截断 |
| 水面 | 85 |
| 植被候选 | 每块局部 x、z 各 0..9，按 x 再 z 遍历；`mt()%100 > 98` 后沿用现有树型逻辑 |

10×10 候选范围保持原先默认半径 10 的玩法密度，但不再错误地随世界测试半径变化。保留原树型和取模选择方式，包括轻微取模偏差；本项不重做生态分布。

## 顺序为何必须固定

先把区块坐标排序为 x 升序、同 x 内 z 升序。所有区块完成地形之后，才按同样顺序生成植被。一个区块内固定候选、树干和树叶写入循环顺序。树冠可以写入邻块，重叠位置采用**按该顺序最后一次写入生效**，不依赖 `unordered_map` 的迭代次序。

只有固定 seed 而没有固定写入顺序仍会失败：树 A 和树 B 写同一格时，最终方块由先后顺序决定；如果它们共享 RNG，遍历顺序还会改变各自拿到的随机数。当前分别消除这两个因素。即使以后有后台任务，也要独立设计结果归并规则，不能假定每块 RNG 独立就能无锁并发写邻块。

## 摘要与版本规则

摘要采用 FNV-1a 64 位，初值 `14695981039346656037`，每字节先异或，再乘 `1099511628211` 并按 64 位回绕。输出 `fnv1a64:` 加 16 位小写十六进制。

1. 区块按 x、z 升序。
2. 每个区块先写 x、z 坐标，各 4 字节 little-endian；负数按无符号 32 位表示。
3. 逐块按 y、x、z 顺序写所有方块，每格依次写 `block_id`、`lightLevel`、`lightColor`、`bitwise_compressed_data`，各 2 字节 little-endian。
4. 不写结构体 padding、指针、容量、容器桶、CPU 网格或 GPU 状态。外围区块也参与摘要，包括可能写入的树叶。

配置本身不混入方块摘要，因此必须同时保存配置/版本和摘要。摘要用于快速比较，存在理论碰撞，不能代替安全哈希、存档或逐块检查。自动测试还比较半径 3 世界的完整四字段快照，避免仅靠摘要判断一致。发布文件身份另用 SHA256。

报告分三层：`terrain_digest` 是地形和植被；`scene_digest` 是夹具覆盖后；`final_digest` 是可选编辑后。选择不同观察点不改变方块，因此应保持摘要相同，但 `initial_pose` 不同。

更改噪声/派生、树型、遍历/覆盖规则、外围语义或方块解释时，应审查并更新 `Generation::Version`；更改夹具几何、姿态、路线或编辑计划时更新 `TestScene::Version`；更改摘要序列化时更换 `digest_schema`。版本不是源码身份的替代品，仍应记录构建输入和资源哈希。

## 无窗口验证与学习边界

`--world-summary` 通过真实 exe 的资源定位、YAML 方块格式、生成及场景代码输出纯 YAML，不调用 `Application::Init`，不创建 GLFW 窗口、ECS 玩家或 GL 上下文。仍要求六个资源文件存在，但不解码纹理或编译 shader。错误使用现有退出码：参数 64、缺资源 2、生成/配置异常 3。

`world.generation` 以真实坐标转换和方块配置测试重复生成、清理重建、反向创建区块、不同 seed、植被开关、夹具及编辑。`world.summary_cli` 启动两个独立进程，在无关工作目录比较完整 441 区块报告。`core.startup_options` 覆盖参数及冲突组合。运行全部测试见构建指南。

为此只把真实 `World::ToChunkCoords` 移到独立源文件，去掉网格测试的坐标替身；没有提前完成 M3 的 CPU/渲染模块拆分。测试目标仍链接既有 GL 相关依赖但不调用驱动，未使用的批次端点仍由测试定义。

完整摘要会扫描全部方块，普通开局也会计算一次，测试覆盖层和编辑会追加扫描。当前 `loading_ms` 包含该成本，但不是首次可操作时间，也不是帧性能数据。M2-T3 必须拆分计时，不能把此阶段的日志当作优化基线。
