---
tags:
  - area/spec
---

# World

## Purpose

维护有限世界、受控编辑与 CPU 网格交接的现役契约。核对[公开入口](../../../../game/modules/world/include/symocraft/world/world.h)、[定义](../../../../game/modules/world/src/block.cpp)、[编辑/发布](../../../../game/modules/world/src/world.cpp)、[生成](../../../../game/modules/world/src/generation.cpp)及[私有 Mesher](../../../../game/modules/world/src/chunk_mesher.cpp)，对应 [T1 历史验证](../../../verification/VERIFICATION.md#退役材料恢复)。旧T1正文可由全局退役材料恢复导航读取；本轮没有构建、故障扫描或 GPU 运行。

## Requirements

### Requirement: Independent complete world ownership

`VoxelWorld::Create` SHALL 返回唯一拥有的完整有限方块世界；世界 SHALL 按值拥有定义、独占私有 Chunk 和串行 Mesher。World、Chunk、Mesher 及 Generator SHALL 不复制或移动，定义集合支持独立值语义。创建成功时 content revision 与 mesh-input revision 均为 1，尚无已发布 CPU 网格；创建失败不发布半初始化世界。world SHALL 只公开 CPU 值/窄视图，不承担窗口、GPU 上传或 app 查找。

#### Scenario: M3-T1-A01 A02 A15 independent instances

- **WHEN** 创建两份世界、编辑其中一份，或尝试复制/移动不可移动 owner
- **THEN** 定义、内容、网格、版本与身份互不污染；非法特殊成员在编译期禁止，构造失败回收已取得资源

#### Scenario: Initial generation is not mesh publication

- **WHEN** 工厂返回并完成启动场景编辑
- **THEN** 调用方 SHALL 显式重建网格，不能把已生成方块或空容器当作一次空网格发布

### Requirement: Checked definition and coordinates

定义装载 SHALL 先形成受校验候选，拒绝非法/重复 ID 与名称，保持必需 1-11 名称映射；未知规则返回空 optional，不冒充 air。纹理层数 SHALL 大于零且覆盖全部引用。核心方块坐标 SHALL 为 i32 整数；浮点入口 SHALL 统一 floor 并检查 finite/i32 可表示性；x/z 分块 SHALL 使用向负无穷取整，y 有效域为 0-255。查询 SHALL 返回自有 Block/规则值，并区分 `OutsideWorld`、air(ID 1) 与非法存储 ID 0，不借出内部容器。

#### Scenario: M3-T1-A02 malformed definition

- **WHEN** 缺必需定义、ID/名称重复、层数为零或纹理索引越界
- **THEN** 装载/校验 SHALL 失败，不公开半成品表

#### Scenario: M3-T1-A03 negative and unrepresentable coordinates

- **WHEN** 坐标为 -1、x/z 跨 16 边界、y 越域，或浮点含 NaN/Inf/i32 外值
- **THEN** -1 映射到 chunk -1/local 15；越域查询返回 OutsideWorld，不能转换的浮点返回空 optional

### Requirement: Atomic edit result and revision invalidation

编辑 SHALL 区分 Rejected/Unchanged/Changed。Rejected 或准备失败不改变内容/版本；Unchanged SHALL 比较该操作影响的全部语义字段而不是仅 ID 或对象 padding，不新增 dirty，也不清除原先 pending。Set 保留不受操作影响的光字段，Remove 恢复既有 air 状态。Changed SHALL 使目标 content 与 mesh-input 各加一，触及 x/z 边界时只使存在的直接邻块 mesh-input 加一；角点只影响两个直接邻块，不影响对角/额外竖直块。全部受影响 u64 版本 SHALL 在提交前检查，耗尽抛 `RevisionExhausted`，不得回绕或部分提交。

#### Scenario: M3-T1-A04 semantic no-op

- **WHEN** 同 ID 的候选仍改变相关 flags，或候选全部相关字段相同
- **THEN** 前者为 Changed，后者为 Unchanged；已有 pending 保留，拒绝/无变化不虚增版本

#### Scenario: M3-T1-A03 A13 border and exhaustion

- **WHEN** 四块交点编辑，或任一受影响版本已达最大值
- **THEN** 只更新目标及两直接邻块；耗尽时内容与所有版本完全不提交，最大合法 mesh-input 仍可发布

### Requirement: Ordered per-chunk mesh publication

重建 SHALL 按 x,z 升序同步处理非边缘 drawable 块，以 mesh-input 与 last-published revision 判断 pending。每块完整候选及尺寸校验成功后 SHALL 无异常交换并发布当前输入版本；首错停止并传播，此前成功块保留，失败/未处理块保留 pending 和旧发布结果。该保证不是整世界、编辑或整批事务。默认 radius 10 为 441 块/361 drawable，边缘块不成为 draw 记录。

#### Scenario: M3-T1-A05 partial batch failure

- **WHEN** 一批 pending 块中某块候选、校验或分配失败
- **THEN** 已成功发布的前序块不回滚；当前及剩余块不冒充新结果，后续可重试未完成部分

#### Scenario: M3-T1-A08 A16 private scratch reuse

- **WHEN** 串行 Mesher 完成 A→B→A 或构建失败后再次使用
- **THEN** scratch 可保留容量但清空工作状态，输出自有内存不被后续构建覆盖；不据复用承诺零分配、并行或性能收益

### Requirement: Mesh identity and scoped borrow

网格身份 SHALL 为进程内不复用的 WorldId 加 ChunkCoord，不用对象地址或跨进程持久 ID。`VisitMeshes` SHALL 按 x,z 输出所有已发布 drawable 记录，包括空发布及仍 pending 的旧发布；未发布与空发布 SHALL 可区分，空发布用于清除旧绘制。记录 SHALL 同时给出已发布 revision 与当前 mesh-input revision。顶点 span/指针 SHALL 只在当前回调有效，不跨回调保存；身份/版本可按值复制。回调期间编辑、重建和重入 visit SHALL 在改变状态前拒绝；回调异常 SHALL 恢复访问 guard，调用方保证 owner 不在借用中终结。

#### Scenario: M3-T1-A06 empty and stale publication

- **WHEN** 网格首次未发布、已发布空结果，或编辑后尚未重建
- **THEN** 未发布不输出记录；空结果保留稳定身份和发布版本；pending 仍输出旧完整结果而非候选半成品

#### Scenario: M3-T1-A14 visitor lifetime and recovery

- **WHEN** visitor 尝试重入、编辑、重建或抛异常
- **THEN** 非法操作无状态改变，异常退出后正常操作可继续；消费者不得解引用超期借用来“验证”失效

### Requirement: Existing geometry and deterministic generation

世界 SHALL 保持当前 28 字节 `BlockVertex3D`、每面六顶点顺序、UV/纹理层及既有可见面规则。相同定义/settings/seed、生成/噪声版本及数值/构建环境下 SHALL 保持生成和摘要可复现：先完成全部地形，再按 x,z 植被写入且 last-write-wins，沿用已声明 digest schema。该保证不扩展为跨编译器/平台逐位相同、greedy meshing、新透明/树叶剔除或相机 culling。

#### Scenario: M3-T1-A07 fixed input reproduction

- **WHEN** 在相同有效条件重复生成同配置
- **THEN** 块顺序、方块/网格字节摘要、世界摘要及出生点保持一致；环境变化 SHALL 重新建立验证依据

## Rationale And Proof Boundary

保留 T1 D01-D09 的已选理由：完整工厂避免可见半世界；整数坐标与自有规则值减少隐式转换和借用；无变化不使网格失效；按坐标求邻接避免长期邻块指针；WorldId+ChunkCoord 使身份不依赖地址；按块发布降低整批候选峰值，代价是首错前可部分成功。D08 实际选有状态私有 Mesher 而非原草稿推荐的局部函数，以唯一 owner 承担容量复用和失败恢复，付出 guard/scratch 状态成本，不自动获得测量收益。不可移动 Chunk 使固定有限世界的寿命约束直接。

稳定不变量身份为 M3-T1-BC-I1（定义 ID/名称一致、只读且发布前校验）、W-I1（坐标/索引/ID 合法、受控写入口与两世界隔离）、W-I2（自身/邻接版本与完整发布、失败保留 pending）、GE-I1（相同有效条件与固定顺序一致）、ME-I1（独占 scratch、异常后复用且输出不借 scratch），对应上面各 requirement，不另建活动计划。A09 的历史真实分配扫描仅覆盖 Release 和全库 `_ITERATOR_DEBUG_LEVEL=0` 的专用 Debug；普通 MSVC Debug 的已禁用扫描不算通过。树叶剔除/旧视角突变继续延期，当前引用不关闭问题或补造 T1 节点批准。

M3-T1-A10 仍指原 GL 的移动/碰撞/射线/选块/编辑边界/材料/选择框/退出重启及真实人工身份，不是 mesh CPU 测试；A11 指 D/R 全套、CPU-only、独立工具、关闭测试、公开消费者/负向边界；A12 指生成/夹具/first-mesh 分段、有效编辑数及 before/after CPU、scratch/候选峰值、重建/上传/文件协议对照，不作性能目标通过。三项技术/人工历史结果及其限制在 T1 报告，当前复用规则分别见[工程](../../../architecture/engineering.md#测试及验证边界)与[Benchmark](../../../architecture/benchmark.md)，不把已完成验证列成虚假待实施任务。
