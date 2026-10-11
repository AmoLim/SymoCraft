---
type: 验证记录
status: CPU基线自动验证完成，待用户审阅
project: Symocraft
module: M3-T4-App-Platform-Simulation
created: 2026-10-09
updated: 2026-10-09
tags:
  - area/milestones
---

# CPU 行为基线查验

本批只建立旧生产 CPU 行为基线与根级夹具，不交付新 physics 核心或 SimulationSession。人工结论统一在 [阶段主表](milestones/m3-t4/manual-verification.md)，未代勾。结果见 [summary.json](summary.json)，行为及缺口见 [基线报告](../../cpu-behavior-baseline.md)。

## 最终结果

| 项目 | Debug | Release |
| --- | --- | --- |
| 15 组物理、9 组玩法 | 两个测试各重复三次通过 | 两个测试各重复三次通过 |
| CPU-only 全量 | 58/58 启用项通过，world.allocations 既有禁用 | 59/59 通过 |
| BUILD_TESTING=OFF | 独立配置、真实 simulation 构建通过，无测试目标/编译 | 同左 |
| 实际依赖/编译 | 无 app/renderer/platform/图形 SDK；物理 cpp 仅生产库编译一次 | 同左 |

[Debug 全量日志](raw/debug-final/ctest.log)、[JUnit](raw/debug-final/ctest.xml)、[重复日志](raw/debug-final/baseline-repeat.log)、[执行记录](raw/debug-final/result.json)；[Release 全量日志](raw/release-final/ctest.log)、[JUnit](raw/release-final/ctest.xml)、[重复日志](raw/release-final/baseline-repeat.log)、[执行记录](raw/release-final/result.json)。逐案例输出与两个目标的实际 Ninja 命令在同目录的 `*-output.log` / `*-commands.log`；build/no-tests 各保留缓存、编译命令、Ninja 图及目标清单。

工具链为 MSVC 19.38.33145、tools 14.38.33130、SDK 10.0.22621.0、CMake 4.3.5、Ninja 1.13.2。标准 Windows 默认系统链接库不当窗口或图形后端依赖。全量 CPU 配置包含既有中立 renderer 门面，但两个基线可执行程序的实际命令闭包不包含它。

## 来源身份

[before](identity/before.json) 在新增夹具前采集；[tested](identity/tested.json) 对应初轮；[final](identity/final.json) 对应最终严格检查；[after](identity/after.json) 是最终验证后重新采集。before→final 的 production/dependencies/assets 一致，final→after 的 production/dependencies/assets/build/fixtures 一致。HEAD 只作背景，实际未提交文件 SHA-256 才是本批来源依据。

`sources/` 保存 final 中所列 CPU、构建、测试及参考 app 文件的原字节；vendor 只留 hash、不复制。`earlier-fixtures/` 保存初轮夹具与脚本。该快照不是完整可独立重建仓库或图形发行包，也不冻结参考 app、T3 或 P01。完整原始捕获仍在 `out/m3-t4/cpu-baseline-20261009-001`；最终构建/二进制在 `out/m3-t4/cpu-b02-debug`、`cpu-b02-release`，程序 SHA-256 在 summary 中。

## 原失败与收紧

初次 Debug 沙箱编译器检查失败 C1902：[原日志](raw/debug-sandbox-failure/configure.log)。获准正常开发环境后通过，未修改 PDB/EH/ABI 选项。初轮成功的 [Debug](raw/debug-first/result.json) / [Release](raw/release-first/result.json) 与身份保留，没有覆盖或改判；其长对象路径警告亦保留。

只读复核后补显式 dt=0 跳跃、命中格/法线精确比较，以及脚本对 app 依赖的完整排除，再在更短的新目录重跑最终两配置。没有改变输入序列目标、生产算法或扩大浮点阈值。归档文件身份由 [archive-files.json](archive-files.json) 核对。

未执行新核心查询/提交失败保证、双会话隔离、私有 app 暂停/恢复端到端、benchmark 前置编辑组合、D12 普通 Application 逐次对照、SDL/GPU/性能验证。不据本批关闭 Win32 1175/Fix1 或宣布 T4-S04/A24–A27 通过。
