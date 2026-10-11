---
type: 实施记录
status: R2顺序1已实施验证，原生后端待顺序2
project: Symocraft
module: M3-T3-Renderer
created: 2026-10-09
updated: 2026-10-09
tags:
  - area/milestones
  - topic/rendering
---

> 2026-10-11 旧导航引用已失效，已改指现行权威或明确归档入口；历史命令、结果和批准含义保持不变。

> 2026-10-11 文档浓缩后已退役复核源：现行入口见 [权威位置](../../verification/VERIFICATION.md#原位置保留的历史材料)；原文保留点时反馈、历史身份和复核依据，不再维护为当前机制或活动计划，旧勾选不产生新批准。

# R2 公开边界与门面实施

范围为 [R2 入口](r2-entry.md) 的实施顺序1，基于 [R1 冻结基线](../../verification/archive/t3-r1/freeze-20261009-001/README.md)。公开三头、错误、PImpl、私有状态/句柄/后端契约与工厂已在生产 renderer 模块实现；**现代运行后端尚未交付，D3D12/Vulkan 初始化明确报 Unsupported，不建立空 target 或用测试后端代替生产后端。** 旧 OpenGL 应用仍是默认路径。

## 实现边界

| 项目 | 实际落点与行为 |
| --- | --- |
| 公开三头 | `include/symocraft/renderer/{types,error,renderer}.h`；`SymoCraft::Rendering`、CPU 场景数据与 opaque handle，签名/字段/默认值沿 R1；不导出实验 registry/helpers 或 SDK |
| 真实门面 | `src/renderer.cpp` 定义全部公开方法，PImpl 委托私有 `RendererState`；线程/非重入、Ready/Suspended/Failed/Stopped、完整帧先验校验、明确失败与有界清理责任 |
| 资源事务 | 自有 CPU 数据，owner/slot/generation 校验；先准备完整新版本，再经 backend 原子接受，最后 noexcept 发布；失败不覆盖旧网格，空网格可更新，销毁立即逻辑失效，世代溢出永久停用槽 |
| 帧与诊断 | 首帧 ID 可为 0，之后严格递增；零尺寸暂停、VSync actual extent 同步状态、backend 返回校验，致命故障停止新提交；接受操作后 Info 失败也进入 Failed；样本不伪造 0，精确 GPU query/frame 关联由真实后端负责 |
| 私有后端 | `src/backend/render_backend.h` 使用 CPU 自有快照及私有 ID，不反向依赖门面。原生资源 fence 退休、真正 submit/present、设备身份、query/readback 和有限等待须由顺序2的真实实现落实 |
| 构建与旧 GL | `symocraft_renderer_api` 为 CPU INTERFACE，`symocraft_renderer` 为真实 CPU STATIC，backend contract 私有；旧 GL 单独为 `symocraft_renderer_legacy_opengl`，旧三头在 `legacy/include`，应用只迁旧 include/链接，不更改玩法、Present 或默认后端 |

类笔记：[Renderer 与私有状态](../../architecture/renderer.md)、[ResourceRegistry](../../architecture/renderer.md)。生产源码不包含/链接 `test/experimental` 或 `test/support`；所有测试链接真实 target，不重复编译生产 `.cpp`。

## 验证

最终查验入口：[节点归档摘要](../../verification/archive/t3-r2/step1-20261009-001/README.md)、[验证/源码/副本身份](../../verification/archive/t3-r2/step1-20261009-001/validation-manifest.json)。原字节复制 108 份材料，共 7,115,889 字节，包含原失败、复测、JUnit、实际 target/编译/链接、配置记录及最终边界核查脚本；不含 exe/lib/DLL，不是可运行包。原始 `out/m3-t3` 不移动，Obsidian 链接均使用库内归档。

| 验证                       | 实际结果与范围                                                                                                  | 原字节证据                                                                                                                                                                                                                                                                                                                  |
| ------------------------ | -------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| CPU-only Debug / Release | 全量编译通过；Debug 52/52 enabled，通过之外的既有 `world.allocations` disabled 不计成功；Release 53/53，全量通过                  | [Debug](../../verification/archive/t3-r2/step1-20261009-001/raw/out/m3-t3/r2-step1-cpu-debug-002/ctest-verified-002.log)、[Release](../../verification/archive/t3-r2/step1-20261009-001/raw/out/m3-t3/r2-step1-cpu-release-002/ctest-verified-002.log)                                                                                                            |
| 真实门面核心                   | 各配置 facade 227 checks；自有快照、事务/句柄、全帧先验、线程/重入、错误/清理、尺寸状态与后端结果校验。包括接受 Resize/VSync/零尺寸结果后 Info 失败的9种异常组合停提交 | 对应 JUnit `renderer.facade_contract` 输出；[Debug JUnit](../../verification/archive/t3-r2/step1-20261009-001/raw/out/m3-t3/r2-step1-cpu-debug-002/ctest-verified-002.xml)                                                                                                                                                                       |
| 公开消费者和实际边界               | 所有方法强制真实链接；三公开头自包含；实际 API/core 使用要求、consumer 编译命令和 Ninja 链接无 SDK/私有/test 泄露；源码与负面边界通过                    | [CPU target 属性](../../verification/archive/t3-r2/step1-20261009-001/raw/out/m3-t3/r2-step1-cpu-release-002/renderer-targets.tsv)、[编译命令](../../verification/archive/t3-r2/step1-20261009-001/raw/out/m3-t3/r2-step1-cpu-release-002/compile_commands.json)、[实际链接图](../../verification/archive/t3-r2/step1-20261009-001/raw/out/m3-t3/r2-step1-cpu-release-002/build.ninja)             |
| 旧 GL 双配置                 | Debug / Release 全量构建（含实际 SymoCraft）通过；各11/11 focused tests；旧 shader/batch/timer 用已有 GL 函数替换，不启动上下文或游戏    | [Debug](../../verification/archive/t3-r2/step1-20261009-001/raw/out/m3-t3/r2-step1-game-debug-001/ctest-verified-002.log)、[Release](../../verification/archive/t3-r2/step1-20261009-001/raw/out/m3-t3/r2-step1-game-release-001/ctest-verified-002.log)、[实际 game 图](../../verification/archive/t3-r2/step1-20261009-001/raw/out/m3-t3/r2-step1-game-release-001/renderer-targets.tsv) |
| BUILD_TESTING=OFF        | CPU Release 实际 renderer STATIC 可单独构建，没有 test/native 图形 target 或 SDK cache；不依赖测试的存在                       | [配置/构建检查](../../verification/archive/t3-r2/step1-20261009-001/raw/out/m3-t3/r2-step1-cpu-no-tests-release-002/configuration-check-001.json)                                                                                                                                                                                                 |
| benchmark 独立             | 关闭 game/CPU modules/testing 的 Release 配置构建通过，不配置任何 renderer/platform/SDL/GLAD 或图形 SDK                    | [配置/构建检查](../../verification/archive/t3-r2/step1-20261009-001/raw/out/m3-t3/r2-step1-benchmark-only-release-001/configuration-check-001.json)                                                                                                                                                                                               |

测试中的 RecordingBackend 仅通过私有契约驱动**真实生产 CPU State**，不进入生产工厂、不提供 GPU 成绩；公开构造函数仍不能创建未交付的现代设备。旧 GL 编译/无窗口测试也不等于迁移后玩法、实际呈现或 T2 正式 Q06 验收。

### 增量证据

CPU-only 两配置及包含实际 app/旧 GL 的 game Release 三轮均通过，基线无编译。只临时改变头文件时间戳，完成后恢复，内容 SHA 不变。私有 backend 头重编核心4单元和 facade 测试1单元，公开消费者0；公开 `types.h` 重编核心5、facade 测试1及消费者1；两探针均触发消费者真实重链。已配置的旧 GL、World、Simulation 与 Application 均不重编。CPU-only 中不存在 app/旧 GL，不能用其0计数证明业务隔离；此项由 game Release 增量补证。benchmark 未与 game 同时配置，本步以独立构建证明无 renderer 依赖，不冒称组合构建增量成绩。

[CPU Debug](../../verification/archive/t3-r2/step1-20261009-001/raw/out/m3-t3/evidence/r2-step1-cpu-debug-incremental-001/result.json)、[CPU Release](../../verification/archive/t3-r2/step1-20261009-001/raw/out/m3-t3/evidence/r2-step1-cpu-release-incremental-001/result.json)、[game Release](../../verification/archive/t3-r2/step1-20261009-001/raw/out/m3-t3/evidence/r2-step1-game-release-incremental-001/result.json)。可复用脚本在 `test/integration/renderer/CheckRendererIncrementalBuild.ps1`，要求已构建稳定、使用新输出目录，不修改头内容或覆盖历史记录。

### 来源与原失败

本次使用 MSVC 19.38.33145 / tools 14.38.33130、Windows SDK 10.0.22621.0、实际 CMake/Ninja 版本见身份。555 份 scoped 源码/生成消费者及实际编译单元留 SHA；工作区有未提交变更，不能只以 Git HEAD 代表本次来源。这不是完整 vendor/SDK 头快照或生产发布身份。当前 `foundation/src/files.cpp`（3012字节，SHA `17c28320571e9f144f3e87d7959050b7fd94d43d6383a5fd01ab0735601434fe`）已重新编译，不复用 R1 旧 foundation 库；不据此改判 Win32 1175、暂停 Fix1、正式 Q06 或未定首次可操作时间预算。

保留两轮沙箱编译器 C1902 探测失败；经批准在正常开发环境构建成功，没有改 PDB 选项。首次 Debug 沙箱测试的两项 assets 路径访问拒绝与一项边界脚本路径登记错误也保留；登记路径修正后正常环境全量复测通过。独立无测试配置首次因验证命令的单项参数展开错误尚未配置，改为明确数组后新目录通过，不算源码失败。对应原失败与复测见 [归档摘要](../../verification/archive/t3-r2/step1-20261009-001/README.md#原失败)。最终只读复核未发现新的实质缺陷。

最终边界核查还修正了拒绝规则中旧 `legacy_gl` 拼写为实际 `legacy_opengl`，并补齐 D3D/DXGI/DXC 缓存排除。四配置以此最终脚本重跑，上表引用新 `ctest-verified-002`；生产源码未改，原日志和 [初始验证身份](../../verification/archive/t3-r2/step1-20261009-001/validation-initial-001.json) 留存，最终身份单列核查脚本 SHA 变化，不用后改脚本冒充原执行版本。

R1 冻结身份 SHA、12份冻结来源、114份查验材料及原来源、16份原包 payload 均再次核对未变；本步不覆盖 R1 或 T2。

## 后续

顺序2实现 D3D12 真实 backend 后再接工厂/实际 target；顺序3迁统一游戏调用方，顺序4交付同源生产候选。R1 冻结快照、旧成绩及原便携包保持不变。现阶段不新增生产玩法人工待办，不改 A01–A19 或整个 R2 的完成状态，不切默认或删除 GL。
