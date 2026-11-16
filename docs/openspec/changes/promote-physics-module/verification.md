---
tags:
  - area/spec
---

# 正式 physics 迁移验证

2026-11-13，代码根为 `G:/Sandbox/Worktrees/promote-physics-module/Symocraft`，起点提交 `8ee84fa417ce96e4c25cd20873dac48a59a2de4b`，叠加从主工作区原字节复制的本 change 四份规划文件及本次实现。正式实施入口及授权边界见 [关键节点](../../../verification/VERIFICATION.md#关键节点)；三个同推理强度子 agent 分别负责边界、消费者夹具及文档/兼容性审查，由主 agent 集成。2026-11-13 交付时等待用户独立验收；后续节点决定以关键节点记录为准。

## 输入与迁移前基线

采用 VS2026 `F:/Toolchains/Cpp/Microsoft VS/18/Community`，MSVC tools `14.51.36231`（编译器 `19.51.36260.0`）、Windows SDK `10.0.28000.0`（根 `F:/SDK/Windows Kits/10/WindowsSDK`）、CLion CMake `4.4.2`、VS 配套 Ninja `1.13.2`、项目已有 Python `3.12.16`。所有构建使用隔离 x64 开发环境及 `VSLANG=1033`，Debug 全依赖闭包使用 DBG01；没有修改 IDE 或系统环境。

迁移前在全新 `out/physics-move/before/cpu-{debug,release}-tests-1` 分别编译真实核心、独立消费者、玩家及会话。两配置各 **7/7 通过，0 失败、0 禁用、0 未执行**：`physics.core`、`physics.independent_consumer`、`player.math`、`simulation.contracts`、两项 `simulation.legacy_*_baseline`、`simulation.session`。既有测试及输入未改，D12 固定阈值未放宽。另在独立 `game-debug-preflight` 以原始源码完成 D3D12 Debug 配置，验证自定义 SDK 下现有 UCRT 发现可用；未复制主工作区未提交的 app/CMake 修复。

起点生产/测试/构建/资产的文件集与 SHA256、实际命令及前置结果保存在 `out/physics-move/before/results.json`，三文件原字节保存在同目录的 `physics-original/`，仅作本次工作输入，不新增长期原始数据附件。

| 迁移文件 | 原字节 SHA256 |
| --- | --- |
| `src/physics.cpp` | `87c212a5e51c2fc24ee3e2f1d904ea4d0b798d1e3cc9a5fbe9d7506d1f712e5c` |
| `include/symocraft/physics/physics.h` | `4671402c00e4f1e6e3a01c1897378ec838e18975ced82d63e8ac6af00f9091ab` |
| `CMakeLists.txt` | `3cb0a56cf24bda6e09f0f3aef0bb535f7fe0f54792340eb93a518c64e2e4204c` |

## 迁移后 A23 方案 A

全部使用 `out/physics-move/after/` 下独立新目录，保持与迁移前同一编译器、SDK、CMake/Ninja、Python、构建选项、玩法资产及测试输入。核心、公开头和模块 CMake 与上表原字节一致；旧目录不存在，无转发头、双 target 或第二实现。Session、适配层、玩家及全部原生测试源码均未修改，D12 阈值不变。

| 配置 | Debug | Release |
| --- | --- | --- |
| CPU-only，测试开启 | 全部默认目标构建成功；定向 CTest 47/47 | 全部默认目标构建成功；定向 CTest 47/47 |
| 普通游戏，测试开启 | 默认 D3D12 与保留 GL 路径构建成功，游戏最终链接；定向 CTest 49/49 | 同左，定向 CTest 49/49 |
| CPU-only，测试关闭 | 同一生产核心与 simulation 构建成功；0 测试、无测试源码 target | 同左 |
| 普通游戏，测试关闭 | 游戏最终链接成功；0 测试、无测试源码 target | 同左 |

八种配置全部构建成功。四组定向 CTest 共 **192/192 通过，0 失败、0 禁用、0 跳过/未执行**；每组均覆盖迁移前的七项回归，以及公开头、源边界、负向边界和 profiling 检查。CPU-only 实际注册 102 项、游戏配置注册 110 项，只有本表受影响的 47/49 项实际运行，不将未选测试或仅编译的 GPU 探针写成运行通过。迁移前七项结果与迁移后四组逐项对账均为通过。

边界夹具共 **130 项：16 正向接受、114 负向拒绝**，在四个测试配置中均通过。真实负向输入覆盖新根的错误/重复 owner、旧根 source/target 回退、私有头与宽 include 根、app/World 公开头及直接链接、alias/生成器表达式、simulation 非法公开传播，以及核心对 World/ECS、原生库、profiling、document_io/YAML 的直接/传递依赖。profiling 禁止引用夹具确实位于正式新路径；公开头清单包含 `symocraft/physics/physics.h`，其独立消费者真实编译、链接并运行。

独立读取八配置的生产源码/依赖表、编译命令与 CMake File API 后确认：`physics.cpp` 只有 `symocraft_physics` 一个 STATIC owner；直接生产消费边仅为 simulation 的 PRIVATE 链接及其合法 `LINK_ONLY` 闭包，app 编译命令没有 physics include 根。核心依赖仅 foundation；CPU-only 不配置 SDL/platform/原生后端/Tracy target，也不执行原生图形头发现；关闭测试的构建图没有 test 源码 owner。所有 Debug C++ 编译命令含 `_ITERATOR_DEBUG_LEVEL=0`，并保留 Debug CRT/调试信息。

前后全量输入哈希对账只有三文件迁移、游戏/simulation 接线、两份 CMake 检查及两份测试夹具发生变化，玩法资产和其余实现保持。实际命令、定向 JUnit 与构建输出留在 `out/physics-move/{before,after}`，图/输入对账为 `out/physics-move/audit.json`；这些是原工作位置，不复制成长期附件。三个子 agent 的独立审查及最终 `git diff --check` 均通过。

## 保留限制与交接

编译保留已有 ECS 对齐与不可达代码警告，未借迁移修补这些源码。OpenSpec 严格校验仍因两份 delta 顶部 `area/spec` 标签前言产生两条既有警告、退出非零；未删标签或将此结果记成严格校验通过。规划/任务可被 CLI 正常识别，CLI 状态不代替工程验证或用户验收。

当前工作树未复制 NPC 与环境恢复的未提交关联 change，proposal 的两处关联文档链接因此仍缺失；全局验证索引的既有历史 out 日志缺失也未改写。本轮新增或改指的本地链接已核对有效。现役主规格与归档材料维持授权前内容，其中旧路径不表示本工作树仍存在旧实现。

方案 A 没有启动游戏，不覆盖 GPU/人工玩法、性能预算、长时或其他机器；同构建内双会话比较不充当跨版本逐帧轨迹或二进制相同证明。NPC N1.3 的 physics 前置保持待用户独立验收，未据任务完成解除。未执行 Git 提交、主规格同步或归档，不回写 T4 当时 A23 后续不适用的历史结论。

2026-11-16，用户已独立验收本节点并授权主规格同步及当前提交批次，批准身份见全局关键节点。该交付的 physics 前置成立，主工作区真实接入仍需生产代码传播。以上“未授权/待验收/未执行同步提交”描述保留交付时点含义；本次同步两份主规格，变更仍留活动目录，不归档、不推送。当前源码/测试/资产/构建输入与已验收快照逐项哈希一致，因此不重复生产构建或游戏运行；本轮规格与提交检查结果见下方收口记录。

本轮收口检查：当前 365 份源码/测试/资产/构建输入与已验收快照一致，未重新执行生产构建或游戏。主规格校验 11/11 通过、0 失败，保留长文本提示；两份 delta 仅修改三个既有 requirement，原有场景全部保留，其他 requirement 块逐字一致，相对链接已重定位。15 份受影响 Markdown 的 259 条本地链接及标签核对无新增错误，保留本树缺失的两份关联 change 和 18 条历史 out 日志；delta 严格校验仍因原有两条标签前言警告退出 1。提交内容、路径范围和排除项以当前 gitspec 为准；本轮授权的提交在当前托管工作树执行，主工作区仅持续同步文档，不动生产代码、分支或索引。
