---
type: 查验归档
status: R2顺序1CPU实施与构建验证已归档，非运行包
project: Symocraft
module: M3-T3-Renderer
created: 2026-10-09
updated: 2026-10-09
tags:
  - area/milestone
  - topic/rendering
---

# R2 顺序1查验归档

范围和实现见 [实施报告](../../../r2-step1-implementation.md)。[验证清单](validation-manifest.json) 记录555份 scoped 输入身份、四组 CTest、六个实际构建配置、三轮增量和108份原字节副本，合计7,115,889字节；107份来自 `out/m3-t3`，另1份为最终边界核查脚本，副本与来源的长度/SHA一致，复制前后来源未变。本目录仅用于 Obsidian 库内查验，不含 exe/lib/DLL，不是可运行包，不是新的冻结或用户验收。

## 结果

| 查验内容 | 实际结果 | 材料入口 |
| --- | --- | --- |
| CPU-only Debug | 52/52 enabled；53 registered中的既有 world.allocations disabled不计通过；facade 227 checks | [CTest](raw/out/m3-t3/r2-step1-cpu-debug-002/ctest-verified-002.log)、[JUnit](raw/out/m3-t3/r2-step1-cpu-debug-002/ctest-verified-002.xml)、[build](raw/out/m3-t3/r2-step1-cpu-debug-002/build-final-001.log) |
| CPU-only Release | 53/53；包含world.allocations；facade 227 checks | [CTest](raw/out/m3-t3/r2-step1-cpu-release-002/ctest-verified-002.log)、[JUnit](raw/out/m3-t3/r2-step1-cpu-release-002/ctest-verified-002.xml)、[build](raw/out/m3-t3/r2-step1-cpu-release-002/build-final-001.log) |
| 旧GL Debug / Release | 两配置全量构建通过，各11/11无窗口focused；未运行游戏或GPU | [Debug build](raw/out/m3-t3/r2-step1-game-debug-001/build-001.log)、[Debug tests](raw/out/m3-t3/r2-step1-game-debug-001/ctest-verified-002.log)、[Release build](raw/out/m3-t3/r2-step1-game-release-001/build-001.log)、[Release tests](raw/out/m3-t3/r2-step1-game-release-001/ctest-verified-002.log) |
| 实际CPU边界 | API/core、consumer编译及真实链接无私有/SDK/test使用要求传播；不是假设性target图 | [target属性](raw/out/m3-t3/r2-step1-cpu-release-002/renderer-targets.tsv)、[真实编译](raw/out/m3-t3/r2-step1-cpu-release-002/compile_commands.json)、[真实链接](raw/out/m3-t3/r2-step1-cpu-release-002/build.ninja)、[cache](raw/out/m3-t3/r2-step1-cpu-release-002/CMakeCache.txt) |
| game图下CPU消费者隔离 | 游戏链接legacy GL，现代consumer仍仅链接CPU core；两配置边界测试通过 | [game属性](raw/out/m3-t3/r2-step1-game-release-001/renderer-targets.tsv)、[编译](raw/out/m3-t3/r2-step1-game-release-001/compile_commands.json)、[链接](raw/out/m3-t3/r2-step1-game-release-001/build.ninja) |
| 三轮增量 | 私有头核心4+测试1/消费者0，公开头核心5+测试1+消费者1，均真实重链且还原原时间戳/SHA；game配置补证实际app/GL不重编 | [CPU Debug](raw/out/m3-t3/evidence/r2-step1-cpu-debug-incremental-001/result.json)、[CPU Release](raw/out/m3-t3/evidence/r2-step1-cpu-release-incremental-001/result.json)、[game Release](raw/out/m3-t3/evidence/r2-step1-game-release-incremental-001/result.json) |
| 不依赖testing | CPU Release真实core可构建，无testing/native图形target/SDK | [检查](raw/out/m3-t3/r2-step1-cpu-no-tests-release-002/configuration-check-001.json)、[构建](raw/out/m3-t3/r2-step1-cpu-no-tests-release-002/build-001.log) |
| benchmark独立 | 无game/CPU modules/testing的Release构建通过；不含renderer或native图形依赖 | [检查](raw/out/m3-t3/r2-step1-benchmark-only-release-001/configuration-check-001.json)、[构建](raw/out/m3-t3/r2-step1-benchmark-only-release-001/build-001.log) |

## 原失败

原失败均保留，不以复测覆写。

- 沙箱编译器探测 C1902：[Debug原日志](raw/out/m3-t3/r2-step1-cpu-debug-001/configure-001.log)、[Release原日志](raw/out/m3-t3/r2-step1-cpu-release-001/configure-001.log)；正常开发环境同工具链重试成功，PDB选项未改。
- 首次Debug沙箱测试：两项assets路径访问拒绝、一项renderer边界脚本路径登记错误；[原CTest](raw/out/m3-t3/r2-step1-cpu-debug-002/ctest-001.log)、[原JUnit](raw/out/m3-t3/r2-step1-cpu-debug-002/ctest-001.xml)。登记路径修正，外部同源最终enabled项52/52通过；不是GPU失败或Win32 1175修复证明。
- 独立无测试配置首次命令单项参数展开错误，未完成配置：[原日志](raw/out/m3-t3/r2-step1-cpu-no-tests-release-001/configure-001.log)；明确数组参数后新 `-002` 目录通过。该次是验证命令错误，不是源码失败。

最终复核将拒绝规则中的旧 `legacy_gl` 拼写改为实际 `legacy_opengl`，补齐 D3D/DXGI/DXC cache排除；[最终脚本](raw/test/integration/CheckRendererBoundary.cmake)与四配置 `ctest-verified-002` 对应。生产输入未改，[初始验证身份](validation-initial-001.json) 和原成功日志也保留；最终清单明确这一次测试核查脚本SHA修正及重跑，不追改旧成绩的来源。

## 保留边界

新公开API、PImpl与资源事务是生产CPU代码；测试recorder只经私有契约驱动此核心，不参与生产工厂。所有尚未实施原生backend均明确Unsupported，未建空D3D12/Vulkan target。旧GL测试使用已有函数指针替换，没有真实context/GPU运行。顺序2的原生设备/呈现、shader、fence/延迟退休、精确query关联、真实提交/设备丢失/在途等待失败及有界清理仍需实现留证；后续统一World玩法迁移和生产候选未交付，不切默认。

`validation-manifest.json` 的来源是脏工作区中的scoped文件SHA与实际编译记录，并非完整SDK/vendor依赖快照、便携发布身份或T2正式Q06。当前foundation已重编，但暂停Fix1、Win32 1175及首次可操作时间预算不改判。CPU配置不含app/GL；相关增量隔离由game Release补证，benchmark独立配置不是组合增量测量。

R1冻结清单SHA `ce27adb37619672634521bd5faca54ef605a3dcf73019826476746b897c49985`、12份来源、114份查验副本与原来源、16份便携包payload均重新核对未变。R1历史批准与运行成绩不改写，本步不新增人工主表或代勾用户验收。
