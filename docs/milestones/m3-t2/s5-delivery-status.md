---
type: 验证与交付记录
status: T2节点已由用户正式批准通过，S5历史包与遗留验证风险保留
project: Symocraft
module: M3-T2
created: 2026-10-08
updated: 2026-10-09
tags:
  - area/milestones
---

# S5 交付候选与剩余验证

当前需要你核验什么，统一见 [T2 固定人工核查入口](manul-verification.md)；本页维护交付事实和证据，候选更新后不另起一个竞争清单。

**2026-10-09 用户已正式批准 T2 节点通过，并选定 MB01 方案2：working set ≤ 704 MiB、private bytes ≤ 1408 MiB；FB01 方案2的 10 / 12 ms 上限保持。** 当前批准范围与遗留事项见 [正式记录](README.md#t2-正式节点批准)。这是用户节点批准，不把下方 S5 历史失败、未执行项目或冻结身份里的旧批准状态改写为实测通过；不发布新包或覆盖历史材料。

2026-10-08 按授权推进 S5、冻结必要 Release 验证包时未申请 T2 最终批准。硬件范围只限定 [正式计划](../../spec/M3-T2-SDL3迁移.md#当前本机-rtx-5070-ti-验收范围) 中的本机 RTX 5070 Ti；用户随后撤销 S5 核显验证，只删除 S5 专用内容，不干预 T3 的同步修改，原始失败和设置恢复只留最小历史记录。Y9000P/GTX 1650 继续延期，按 [中央规范](../../spec/project-scope.md#m3-本机-rtx-5070-ti-验收与下一玩法阶段性能门槛) 留作下一玩法开发前的中低端整机性能门槛，不标通过或免验。本页以下未关闭项现作为批准时的遗留记录，不继续推定 T2 节点未批准。

## 冻结入口

使用 [冻结包说明](../../../out/m3-t2/s5/frozen/s5-release-001/FREEZE-README.md)、[ZIP](../../../out/m3-t2/s5/frozen/s5-release-001.zip) 和 [冻结身份](evidence/s5-freeze.json)。包内只留游戏、Benchmark runner、输入与三模式桥接工具、必要资产/Release CRT/许可及说明；不打包 Debug runtime、构建缓存、SDK 或 vendor 树。原 S3 冻结包及 GLFW 基线保留，未覆盖。包内状态文档是冻结时的仓库快照，相对文档链接需原工作区；脱源运行按外层 FREEZE-README 的独立命令，不需要这些链接或原构建目录。

普通游戏运行 `package/SymoCraft.exe`；runner 运行 `package/SymoCraftBenchmark.exe`。输入工具不是双击交互入口，使用 `package/run-hardware-check.ps1 -OutputDirectory <全新外部目录>`。F5 Reset、F6 切换 Normal/Hidden/Lock、F7 Capture，Escape 最后测试。工具使用方式仍见 [S3 清单](s3-manual-verification.md)，用户原有填写和日志保持历史包身份，不冒充新包已经再次人工通过。

## S5 核显验证已撤销

2026-10-08 用户要求目前不做核显验证，随后明确本次只删除 S5 核显内容。已移除专用 ZIP、候选/安装副本、解压副本、构建缓存、启动脚本、设备守卫及专用测试；不保留可运行的核显入口。通用 S5、S3、GLFW 三个冻结包的 SHA 保持原值。范围与逐项删除核对见 [撤销记录](evidence/s5-retired-verification.json)。

只保留五次真实运行的最小原始摘要、日志及设置恢复记录，其中三次为用户后续运行。历史事实仍是实际 GL 为 NVIDIA、专用守卫拒绝、临时偏好恢复；既不改判为核显通过，也不据此判定 AMD 不支持 GL。记录不带 exe、源文件复制、SDK 或活动包清单，不构成后续执行要求。

## 已执行范围

| 项目 | 结果与限制 |
| --- | --- |
| 构建/安装与活动依赖 | Debug/Release 各五模式配置、构建、安装通过，活动 GLFW 目标/缓存/产物/安装许可已撤除；61 项真实边界检查通过。历史 vendor/lib/旧源码不删 |
| 主工程 CTest | Release 64/64；Debug 首轮及诊断复测均为 62/63 启用测试通过，`foundation.files` 失败，既有 `world.allocations` 禁用不算通过 |
| 真实生产库 | 两配置各 17/17 三模式/生命周期/失败探针、21/21 输入运行、4/4 分配故障；纯输入回归各 1/1。合成队列/自身消息不等于本机人工硬件验收 |
| 安装交付 | Release 7 项通过，缺 app-local CRT 在本机可被系统 runtime 补足，只标 unavailable；游戏及 runner 的中文空格路径、不同 cwd 已实际执行 |
| runner 正常取消 | 真实 GUI WM_CLOSE → stop token → 子游戏正常退出，runner/game exit 0/4，无强杀；在 warmup 取消、57 帧、正常清理与三类 CSV 保留。不是完整采样，合法取消不要求截图 |
| 三场景协议诊断 | static/walk/edit 各一次预热 0、采样 2 秒，schema/protocol/workload=2、seed 424242、441 区块、1920×1080/4x MSAA/VSync 0、CSV 帧数及 CPU/GPU/present 字段核对通过，完成截图已检查；不计 Q06/Q07 |

完整摘要与原始目录、SHA 见 [S5 验证身份](evidence/s5-validation.json)。首次交付夹具因 PowerShell `$null` 转空字符串而未识别子窗口，随后修正并补足取消不应要求 PNG 的规则；首次失败与正常采样结果原样保留，不反推成生产故障或性能验收。

## 未关闭项

2026-10-08 用户明确暂停 [Fix1](../../spec/M3-T2-Fix1-spec.md#当前暂停)，不继续修复或追踪，先推进 GLFW 预采样及预算。2026-10-09 已确认 FB01/MB01 方案2并另行正式批准 T2；以下发布和协议风险继续保留，Fix1 不恢复。只有完整终态且正常退出的轮次可作测量成绩，故障轮保留，不靠 CSV 存在判成功；用户节点批准不等于 A01/A11 已全部实测通过。正常玩法/渲染故障尚未由该发布问题证明，原失败、诊断改动及冻结包不改。

**文件发布稳定性未验证关闭，现按用户明确 T2 节点批准保留为已知遗留风险。** 此前列为 A01/A11 与最终交付批准的阻断，本次不再以它否定已批准的 T2 节点，也不宣称问题已修复或所有交付测试全绿。旧 `MoveFileExW` 无法替换兼容共享读者持有的目标，已确定性红/绿验证并改为 `ReplaceFileW`；仅缺目标时尝试初次移动，不重试、不吞错。新的完整测试和重复仍出现 Win32 1175，且普通无本地持有读者替换也失败；外部句柄/过滤来源未知，不能归因 SDL、SDK、Defender 或沙箱。实际主工程各 10 次独立重复：Debug files/export 为 9/10、9/10，Release 为 8/10、10/10。替代非 POSIX rename API 也不能满足兼容读者约定，未替换生产策略。详见 [重复证据](../../../out/m3-t2/s5/publication-repeat-003/summary.json) 和 [API 实验](../../../out/m3-t2/s5/rename-api-audit-001/summary.json)。不承诺所有晚期 I/O 失败都保留两个路径。

| 剩余门槛 | 当前状态 |
| --- | --- |
| 本机 RTX 5070 Ti 至少 15 分钟人工游戏 | 未取得本轮记录；有限帧运行及原 S3 输入反馈不替代，结果绑定实际 GPU/源码/包身份 |
| 同源、本机 RTX 5070 Ti 的 GLFW/SDL Q06 与 Q07 | FB01/MB01 均已按方案2确认；首次可操作预算未定、正式同源对照未完成。003 仅为旧 GLFW 预采样，002 已废弃；阈值不从 SDL 诊断反推，不混用其他 GPU/机器成绩 |
| 本机尺寸/硬件条件及安装限制 | Benchmark 真尺寸改变仍 partial/unavailable；原人工条件/数值绑定尚未齐全。本机系统 runtime 会补足 CRT 的限制如实保留并提交用户确认，不新增第二台干净机器必验，不冒称缺依赖行为通过 |
| Y9000P / GTX 1650 整机性能 | 延至下一玩法功能开发前，未执行，不使用本机结果替代 |

T2 已由 2026-10-09 用户明确批准通过，不是由冻结包自动推导。上述缺口继续作为遗留事实，Renderer v1 冻结/R2 授权仍独立核对；另行授权的隔离 D3D12 实验不被撤销。树叶剔除与旧视角突变继续原延期。

## 清理回归与 T3 准备

移除 S5 核显路径后，在全新目录重新构建 Debug/Release 主工程及生产库桥接工具：主工程各 38/38 定向 CTest、工具各 2/2 CPU CTest 通过，普通输入的 Native 路径及人工输入的 GL 路径保持。不是完整主 CTest 全绿或新增人工/GPU验收。启动参数、清理顺序和公开头/模块边界纳入定向回归；旧专用参数在创建窗口前按既有 usage-error 规则拒绝。最终还原参数遍历原写法后已补跑同组回归，详细身份见 [最终清理验证](evidence/s5-cleanup-validation-002.json)；[初始验证](evidence/s5-cleanup-validation.json) 保留为此前源码的点时记录，不用其 exe SHA 指代最后修订。

T3 进入范围按 [正式 spec](../../spec/M3-T3-渲染器重构.md) 和 [R1 记录](../m3-t3/README.md) 为准，不覆盖其历史结果或冻结状态。T2 已由用户单独批准，可继续已授权的 R1 准备；不能把清理回归或 T2 批准视为 v1 已冻结或 R2 正式启动：

| 下一项核对 | 所需事实与边界 |
| --- | --- |
| 生产纹理与采样转接 | 明确纹理层数、格式/布局、方向、色彩空间、mip/采样及三角形同层规则，不以最小夹具资产替代生产契约 |
| 世界网格与相机转接 | 回调借用网格先自有化，核对身份/句柄和成功更新后的版本记账；明确 Y 向上、右手视图、FOV、near/far 与 GL 到 D3D12 转换 |
| SDL 平台与呈现职责 | 使用真实 Window/Native HWND 借用、实际像素、逆序清理及唯一 Present，沿用 [S4 交接](s4-platform-handoff.md) 的接口事实；验收环境以最新中央规范覆盖旧双机安排 |
| Renderer v1 与构建图 | 核对公开消费者独立编译、不传播 SDK 和实际 target 图；保留 RTX R1 实验自身条件，GPU Debug 未完成不能由 CPU Debug 或 Release Debug Layer 替代 |
| 正式冻结与后续实施 | T2 用户节点批准已取得；R1 自身必要核对成立后再冻结 v1，R2仍需独立授权。T2遗留事项不重新作为节点批准前置；Vulkan与OpenGL最终退役不提前混入本次清理 |
