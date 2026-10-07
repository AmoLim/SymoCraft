---
type: 验收报告
status: 技术交付完成，已知问题延期，待节点批准
project: Symocraft
module: M3-T1
created: 2026-10-06
tags:
  - area/milestones
---

# M3-T1 世界模块重构

对应 [正式 Spec](../../spec/M3-T1-世界模块重构.md)。世界实例、受控编辑、CPU 网格及调用迁移已实施，自动验证完成。用户发现树叶剔除与视角突变，明确本轮仅记录、不扩展修复，并确认其余人工项目正常。技术交付完成，节点批准待确认。

2026-10-07 范围澄清：用户明确上述两项问题不属于 T1 spec，也不属于 T2 前置范围，两项继续延期，直接授权继续 T2 准备。本澄清不改写历史实测、不宣称两项修复通过；T2 生产迁移仍须准备评审后另获授权。

## 版本与边界

- 起点：`d79d255325b4361f153463d92fbddfc4ce783e1e` 加用户已有文档修改；未替用户提交或回退工作区。
- 旧 T0 Release SHA-256：`93c970a955cde39b9a5fa81ccc494d0d2b8a33d336547c72a226c0be7ac81eb4`。旧包与冻结基准不覆盖。
- 当前验证：Windows x64、MSVC 19.38、CLion CMake 4.3.1 / Ninja、英文诊断环境。[实际机器记录](evidence/environment-verified.json)：Windows 11 Pro 26200、Ryzen 7 9700X、约 64 GB、实际 GL 设备 RTX 5070 Ti / NVIDIA 616.64、电源“平衡”；无超频/降压沿用用户旧声明，未复测频率或温度。
- 实际接口和状态由 [world 功能](../../project/architecture/world/World-功能.md)、[对象覆盖清单](../../project/architecture/对象笔记覆盖清单.md)维护；本报告只维护验收结果。

## 实现

1. `VoxelWorld` 独占定义、固定 x/z 存储、不可移动 Chunk 和一个串行 Mesher；旧 ChunkManager / 全局定义入口退出。simulation 显式消费同一实例，其公开 World 类型依赖由 CMake PUBLIC 传播。
2. 整数坐标和唯一检查式浮点转换；编辑三态、三种版本。所有版本预检完成后才提交；无变化不新增待更新，已有待更新保留。
3. Mesher 复用可见面描述 scratch，返回独立候选。完整校验后无异常交换，按块首错停止；空发布和首次未发布分开，回调视图限本次访问。
4. 保留生成、植被写入、28 字节顶点及面序。元数据按实际 World 配置与生成时保存的噪声种子在报告阶段构建，避免漏计或再造不同 Generator。
5. 真实分配失败发现 YAML Stream 构造期裸缓冲泄漏；最小数组 `unique_ptr` 修补，保留失败与复测证据，不降低断言。

## 验收

以下“通过”只表示具名案例有证据，不代替整个节点批准。待执行项不从旧 T0 结果继承。

| 编号 | 状态 | 结果 / 证据入口 |
| --- | --- | --- |
| A01 两实例 | 自动通过 | 定义、方块、邻接、网格、版本及身份隔离；world.contract |
| A02 定义/工厂 | 自动通过 | 非法/重复、必需 ID、层数、半径及构造失败；block_config/contract/allocations |
| A03 坐标 | 自动通过 | 正负边界、四块交点、y 极值、NaN/Inf/i32 极值；contract |
| A04 编辑三态 | 自动通过 | 全字段候选比较、光字段保留、无变化/拒绝不提交；contract、实际有效编辑对照 |
| A05 发布失败 | 自动通过 | 候选、真实尺寸及分配失败；先前成功保留、重试剩余；contract/allocations |
| A06 空发布/格式 | 自动通过 | 首次/空/旧发布、稳定身份、宽计数；真实 World→旧 Batch GL 替身清除旧批次，不代替空结果 GPU 验收 |
| A07 确定性 | 自动通过 | 七组配置 × 三轮，2,445 条逐块顺序/字节摘要、世界摘要和出生点一致；实际固定场景摘要一致 |
| A08 连续复用 | 自动通过 | A→B→A 实际容量复用，独立输出未被覆盖；contract |
| A09 RAII | 自动通过，配置限定 | Release / 全库关闭迭代器代理的专用 Debug 真实 new 故障扫描；普通 Debug 限制见下节 |
| A10 原玩法 | 人工确认，两项延期 | 用户确认移动/跳跃/碰撞、射线选择、放置破坏、区块边界、材料切换、选择框及退出重启均正常；树叶剔除/视角突变另行登记，不表示已修复 |
| A11 构建边界 | 自动通过 | Debug/Release、CPU-only、独立工具、关闭测试、公开头及负向边界；Debug 扫描显式禁用 |
| A12 测量/协议 | 自动通过 | CPU 分段/分配、scratch 峰值、旧新运行和有效编辑数；不宣称性能目标达标 |
| A13 耗尽 | 自动通过 | 任一受影响版本用尽时内容与全部版本不改；最大值可发布；contract |
| A14 回调 | 自动通过 | 重入先拒绝、异常恢复；不解引用悬空数据作测试；contract |
| A15 特殊成员 | 自动通过 | 四种特殊成员编译期禁止、独立公开头；最终四配置定向复测通过 |
| A16 失败后复用 | 自动通过 | 同 Mesher 在真实分配/候选失败后可再次构建；contract/allocations |

### 构建与故障扫描

| 配置 | 最终结果 | 证据 |
| --- | --- | --- |
| 完整 Debug | 61/61 启用项通过，1 项禁用 | [CTest](evidence/full-debug-final-ctest.log) |
| 完整 Release | 62/62 通过 | [CTest](evidence/full-release-final-ctest.log) |
| CPU-only Debug | 44/44 启用项通过，1 项禁用 | [CTest](evidence/cpu-debug-final-ctest.log) |
| 专用 CPU Debug | contract + allocations 2/2 通过 | [CTest](evidence/cpu-debug-alloc-no-proxy-ctest-retry.log) |
| 独立执行器 | 5/5 通过 | [CTest](evidence/standalone-final-ctest.log) |
| 关闭测试 / 独立工具 / 根级工具-only | configure/build/install 通过，安装无测试产物 | [矩阵](evidence/build-matrix.log) |

最后补齐赋值特殊成员断言后，四配置重编译 contract 并各复测 1/1：[Release](evidence/full-release-special-member-ctest.log)、[Debug](evidence/full-debug-special-member-ctest.log)、[CPU Debug](evidence/cpu-debug-special-member-ctest.log)、[专用 Debug](evidence/cpu-debug-alloc-no-proxy-special-member-ctest.log)。[生产源码](evidence/production-sources.tsv)、[依赖](evidence/production-dependencies.tsv)、[公开头](evidence/public-headers.tsv)可核对正式目标。

真实分配故障覆盖 Definition 1,747、Factory/Chunk/noise 57、Generator/noise 1、scratch/output 28 个序号；各失败后资源回收、同 Mesher 重试，见[扫描](evidence/cpu-debug-alloc-no-proxy-ordinal-result.log)。普通 MSVC Debug 的 STL 迭代器代理在 `noexcept vector()` 内分配，bad_alloc 导致终止，[实际栈](evidence/cpu-allocation-stack-diagnostic.log)保留，故该项禁用；专用配置**全库**一致设置 `/D_ITERATOR_DEBUG_LEVEL=0` 并保留 `/EHsc`，不得仅改单个测试 target 的 ABI，不声称普通 Debug 扫描通过。

完整 Debug 首轮 61/62 发现 YAML Stream 构造期 2,048 字节泄漏；[失败](evidence/full-debug-ctest.log)与[两文件 RAII 修补身份](evidence/yaml-raii-fix-identity.json)保留，修复后上述矩阵通过。沙箱首次 C1902 与[获准重试](evidence/cpu-configure-retry.log)也保留，没有放宽断言。

### 成本与协议

[CPU 对照](evidence/world-cost-comparison.json)链接正式 world 库，旧版链接修改前快照，[输入身份](evidence/world-cost-identity.json)可追溯。seed 424242 × 半径 2/3/10 × 有/无植被，加 seed 424243/半径 3/有植被，各三轮。下表为默认半径 10/植被开启中位数，仅是本机观察：

| 指标 | T0 → T1 |
| --- | --- |
| 生成 CPU | 121.3434 → 126.7668 ms |
| 初次网格 CPU | 198.4646 → 142.0267 ms |
| 初次网格 C++ new 请求 | 6,147 → 383 次；209,853,541 → 57,177,919 字节 |
| 保留方块 / 网格容量 | 方块均 231,211,008；网格 69,947,948 → 56,904,456 字节 |
| T1 scratch / 最后候选 / 单次峰值 | 86,320 / 691,824 / 814,600 字节；旧对应量未测，保留 null |

峰值仅计单次 scratch + 候选 + 当前块旧网格容量，不是整世界/RSS/显存峰值。旧固定几何工作区 676 字节与新可见面 scratch 含义不同。new 字节为累积请求，排除 C malloc/OS（旧 map 节点使用 C malloc）；生成 new 次数 443→889 不表示总分配翻倍。冷启动收益不保证编辑零分配或 FPS 提升。

最终 [runtime-002 对照](evidence/runtime-comparison.json)：旧新各 static/walk/edit，一轮预热 5 秒 + 采样 15 秒，1920×1080、VSync 关闭、allow-unfocused、v2 schema/protocol/workload 保持。六轮有效；编辑采样均接受 60 次、重建 180 块，总计划应用 79 次；摘要/实际 metadata/首上传/CSV 字段一致。p95 帧间隔旧→新：static 7.2702→6.9238、walk 8.3469→7.2360、edit 7.6394→7.4782 ms。期间无并行构建/成本探针，但旧新失焦分布不同、用户/OS 背景负载未监测，单轮差值不认定性能改善/退化。合法 Unchanged 仍计已接受但不新增重建，协议含义未改。

实际 GPU 静态截图字节一致，SHA-256 `3d4765d8bafd5388c2d60cd5ec4cc41e335d1b6416bebb1c692c299fc3b01851`，[选定截图](evidence/static-frame.png)。Debug 120 帧正常，隔离坏 shader/texture/block-config 均按预期返回 3；[命令与退出码](evidence/runtime-probes.json)。pilot runtime-001 首轮部分与构建重叠，仅作功能证据。没有覆盖冻结的 36 分钟基准或替代人工玩法。

## 交付与剩余项

候选入口：[Release 游戏](../../../out/m3-t1/install/full-release/SymoCraft.exe)，人工固定场景参数 `--scene regression --seed 424242 --checkpoint four-chunk`；[ZIP](../../../out/m3-t1/Symocraft-M3-T1-windows-x64.zip)共 17,283,095 字节，20 个安装文件逐项验证、六个必需资源与源码一致，不依赖 PowerShell。不包含用户后来放入安装文件夹的录像/截图，不是已获节点验收的版本。

- Release SHA-256：`76a695decaf99405483d25d722535bef6d489914002743372d0b1e95a17b5db2`。
- ZIP SHA-256：`0fc16d39c32b6c209ec9f29174178603899a7591ae112cd339b8eec7abaf42f7`；其余身份见[摘要](evidence/package-identity.json)。完整 [1,912 文件输入清单](../../../out/m3-t1/build-inputs.json)是保守仓库范围，不是编译器读取轨迹。
- EXE/DLL/ZIP、构建、原始 CSV 和逐块成本数据只放 `out`；docs 留选定日志、小型摘要和两张截图。用户 journal/Obsidian 设置未覆盖。
- 最终文档归属/YAML/本轮导航静态校验通过，[报告](evidence/docs-audit-validation.json)明确函数覆盖为人工源码审阅，不等同于程序/UI 验证；用户空白 `docs/source-migration.tsv.md` 保持原样。

原 T2 渲染交接入口现为 [T3 CPU 契约清单](../../spec/M3-T1-T3-CPU契约清单.md)，引用 [World 数据契约](../../project/architecture/world/World-数据设计.md)及相关权威笔记，并单列 T3-R1 待冻结项。新 T2 为前置 SDL3 迁移；编号调整不改写本报告的历史证据与 CPU 契约。当前 renderer 仍逐帧全量复制/上传，不在 T1 建 GPU 缓存。

剩余项：T1 节点状态单独确认，不要求用户把两项范围外问题作为 T1 验收内容。两项修复按用户要求后续独立处理，不阻塞已授权的 T2 准备。旧 ECS/分配器、物理预算及多机/长时专项不由本轮替代。

### 人工复查问题

| 问题 | 现象与证据 | 处理边界 |
| --- | --- | --- |
| T1-M01 视角突变 | 用户原地、不按移动键，仅水平缓慢转动鼠标可稳定复现偶发跳动，并非总在同一朝向；[录像](../../../out/m3-t1/install/full-release/promblem-视角突变.mp4)11.71 秒，当前包身份见上文 | 只读对照确认相机/角度/同步/鼠标五文件与 T0 快照相同，不能据此排除外围影响或认定根因。按用户后续指示仅记录，归后续输入/相机任务；未构建的诊断开关已撤回，未改变灵敏度或算法 |
| T1-M02 树叶剔除 | [用户截图](evidence/leaves-user-report.png)；缺少原运行 seed/位置/观察方向，尚未严格复现或确认是透明混合、面提取还是剔除问题 | 按用户要求本轮仅登记，不更改算法、shader 或透明规则；后续 renderer 专项处理前需补复现条件 |

两项素材及五文件旧新哈希见[身份记录](evidence/known-issues-identity.json)。录像展示普通地形，原 seed/位置未知，不能仅凭它推定固定夹具用例已经执行；原录像保留在 out，未放入 docs 或 ZIP。

2026-10-06 用户随后明确“其他项目均正常”，对应 A10 列明的剩余用例；实际持续时长、是否逐项使用固定夹具未记录，不补写为完整输入回放或长时稳定性验证。
