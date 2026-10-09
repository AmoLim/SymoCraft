---
type: 功能
status: FB01与MB01均选方案2，T2节点已由用户正式批准通过，遗留风险保留
project: Symocraft
module: M3-T2-Platform-SDL3
created: 2026-10-07
updated: 2026-10-09
tags:
  - area/milestones
---

# M3-T2 平台契约与失败责任候选

**当前状态（2026-10-09）：用户已选定 MB01 方案2：working set ≤ 704 MiB、private bytes ≤ 1408 MiB，并明确批准 T2 节点正式通过。** FB01 方案2的 10 / 12 ms 上限保持。批准依据与遗留事项见 [正式节点批准](README.md#t2-正式节点批准) 和 [正式计划](../../spec/M3-T2-SDL3迁移.md#节点批准与遗留事项)。下方准备期“不通过”表述保留为历史，不覆盖本次明确批准；已知失败、002 废弃及 Fix1 暂停事实不改。

本文是 [T2 正式计划](../../spec/M3-T2-SDL3迁移.md) 的 P05 准备产物及已选契约，不是 T2 节点验收通过记录。用户已批准 P06；S1 依赖隔离及 S2 生产 SDL3 窗口/桥接已完成，见 [实施记录](README.md#s2-窗口与桥接)。GL 六项入口保留签名并 checked，Native/Vulkan 仅从授权私有桥接消费。2026-10-08 已进入 [S3 实施与验证](README.md#s3-输入与窗口行为)，增加真实生产库输入/分配故障回归与 [硬件操作清单](s3-manual-verification.md)，未执行人工格保持未验证，不重新要求 Q04/P06 或原工具手感批准。下方八项三方案决策继续保留，不重新待选。树叶剔除与视角突变继续延期，留基线记录，不作为准备工作的前置阻断。

2026-10-08 后续授权：用户要求暂不执行 Y9000P 验证，冻结可运行 S3 包并推进 S4。开发桌面清单的通过填写与实际硬件日志分别保留，见 [冻结与交接记录](README.md#s4-t3-契约交接) 和 [平台交接契约](s4-platform-handoff.md)。Y9000P 是延期，不是免验、通过或 D09 范围变更；本次授权不关闭 T2、预算或 R1/v1 冻结门槛。

S5 交付事实见 [候选与遗留验证](s5-delivery-status.md)：活动 GLFW 已退役，部署/取消及真实库回归留证，通用 Release package 已冻结；Win32 1175 发布稳定性仍开放，按本次用户节点批准列为遗留风险，不改写成已修复。硬件范围以最新 [[M3-T2-SDL3迁移#当前本机 RTX 5070 Ti 验收范围|T2 正式范围]] 为准，只限定本机 RTX 5070 Ti。用户已明确撤销 S5 核显验证并删除专用包及代码，只留 [最小历史记录](evidence/s5-retired-verification.json)：此前请求后仍取 NVIDIA，偏好恢复，不改判 AMD 能力；不再提供核显执行入口或要求补验。Y9000P/GTX 1650 整机性能验证延至下一玩法开发前，不复写历史安排或八项三方案选择。FB01/MB01 已确认，T2 已正式通过；正式同源 Q06 和首次可操作预算仍作为遗留事项保留，不伪造实测通过。

本次沿用已有 `Window::Impl`、应用主循环和静态 Init/Free 调用路径，只补充换库、三种窗口用途及失败释放所需契约。不建立完整 Runtime/SimulationSession 管理器，不在此实施 Renderer PImpl 或应用对象重构；这些工作分别留 T3/T4。原草稿附录不修改。

权威起点：[Window 公共头](../../../game/modules/platform/include/symocraft/platform/window.h)、[当前窗口实现](../../../game/modules/platform/src/window.cpp)、[私有图形桥接](../../../game/modules/platform/src/graphics_bridge/graphics_bridge.h)、[应用生命周期](../../../game/app/src/application.cpp)、[Window 设计](../../project/architecture/platform/Window-类设计.md)。

## P06 用户决策表

只补充尚待冻结的接口与实施进入条件，不重新选择正式计划的 D01–D12。Q01–Q02 为技术方案确认，Q03–Q08 为验证安排和实施授权；每项保留三个方案，以“最终敲定方案”为准，推荐不代替用户选择。

2026-10-07 按用户最终填写同步：**Q01/Q02/Q03/Q05/Q06/Q07/Q08 已选方案1，Q04 已选方案2。** Q02 先要求补充细节，随后最终栏已填写方案1；其迁移影响与选择依据见 [附录 B](#附录-b-q02-失败表达与后续图形-api-迁移)，三个方案继续留作对照。完整 Vulkan SDK 的安装步骤见 [附录 A](#附录-a-vulkan-sdk-安装与复测)。同日用户明确确认“Q04可关闭，验证通过，P06通过，进行下一阶段的工作”，据此关闭 Q04、记录 P06 通过，Q08 的 S1–S5 顺序实施授权生效。剩余准备缺口按 Q03 随 S2/S3 补齐；T2 节点仍须另行验收。

### 已确认项目

2026-10-07，用户在本聊天明确确认“人工手感对照目前已通过”，此项不再待选。现有两次人工对照会话均请求 `SystemScale=1`，最近一次见 [SDL 会话身份](../../../out/m3-t2/archive/preparation/platform/input-comparison/manual-20261007-134058-29d1c4af/SDL3-identity.json)及 [GLFW 会话身份](../../../out/m3-t2/archive/preparation/platform/input-comparison/manual-20261007-134058-29d1c4af/GLFW-identity.json)。继续以已试的系统缩放策略和 0.05 灵敏度作为生产适配候选，不另选 raw mouse 或重新校准。

该人工结论不自动覆盖未明确反馈的鼠标按住/切出释放、不同键盘布局、完整 DPI/任务栏矩阵、Y9000P 或真实 SDL 生产玩法；这些剩余项按 Q03 安排，不因手感通过而整项勾选 P04/T2。

### Q01 窗口创建入口

三种用途及默认 OpenGL 已确定；只选择中立接口形态，所有方案均不暴露 SDK 类型、不新增完整 Runtime 管理器。

| 最终敲定方案 | 候选方案1（推荐）                                          | 候选方案2                                             | 候选方案3                                   |
| ------ | -------------------------------------------------- | ------------------------------------------------- | --------------------------------------- |
| 1      | 现有 `Create` 最后增加 `WindowMode`，默认 OpenGL；旧调用不改，增量最小 | 小型 `CreateOptions` 表达模式/创建参数，保留旧入口转调；扩展方便但多一个配置类型 | 新增模式为首参的显式重载，旧入口继续创建 GL；模式醒目，但要维护两个调用入口 |

### Q02 GL 桥接失败表达

当前化、VSync、呈现失败必须具名报告并停止无效提交；合法“不支持”、查询失败和无当前 context 不能混淆。只决定如何传递失败，不提供吞错继续方案。

| 最终敲定方案 | 候选方案1（推荐）                                | 候选方案2                                     | 候选方案3                                            |
| ------ | ---------------------------------------- | ----------------------------------------- | ------------------------------------------------ |
| 1      | 保持现有六项签名，必要操作失败具名抛错，进入 app 统一退出清理；调用改动最少 | 必要操作返回 `bool` 并提供项目自有诊断出参，调用方显式检查；增加窄调用适配 | 返回项目中立 `BridgeResult`，携带状态、操作名和自有诊断；检查集中，但新增结果类型 |

三个方案都保留 `GetProcedure` 可返回空供 GLAD 判断可选函数；正常退出先释放 GPU，析构 best-effort 兜底，不以异常阻断其余资源释放。

本项已按最终栏选定方案1：保留六项签名，必要操作失败具名抛错；这不代表生产失败检查与清理回归已完成。[附录 B](#附录-b-q02-失败表达与后续图形-api-迁移) 保留三个方案的调用成本、状态表达、诊断寿命及与 T3 的衔接对照。

### Q03 剩余准备项安排

决定何时补证据，不免除失败矩阵、硬件输入、DPI 或任务栏必验，也不把当前部分结果改写为通过。

| 最终敲定方案 | 候选方案1（推荐）                                                          | 候选方案2                                     | 候选方案3                                                       |
| ------ | ------------------------------------------------------------------ | ----------------------------------------- | ----------------------------------------------------------- |
| 1      | 明确接受已记录的准备缺口：窗口/桥接失败随 S2 补，输入/事件失败及剩余硬件输入/DPI/任务栏随 S3 补；T2 验收前全部关闭 | 先补齐能由独立探针完成的准备项，再开始 S1；真实生产回归仍随实现完成，准备期较长 | 先补鼠标 held/Reset、切出释放、不同布局和 DPI/任务栏高风险项，再开始 S1；其它失败测试随真实实现完成 |

### Q04 Vulkan 探针头源

既有探针用官方 SDL 原样附带的 Khronos Vulkan 1.3.282 头，并已真实创建 instance/surface，但不是安装了完整 SDK。这是历史证据，不能直接满足已选方案2；须先安装固定的完整 SDK，再以其 `Include` 为探针头源新建配置复测。该选择不让 GL-only/Native/CPU/工具强制依赖 Vulkan。

| 最终敲定方案               | 候选方案1（推荐）                                                 | 候选方案2                                      | 候选方案3                                          |
| -------------------- | --------------------------------------------------------- | ------------------------------------------ | ---------------------------------------------- |
| 2，在该文档附录内给出SDK环境安装步骤 | 接受 T2 探针使用 SDL 原样附带的固定头源，记录身份/许可；不新增完整 SDK 依赖，同步正式计划的头源例外 | 先安装并固定完整 Vulkan SDK，用该头源复测后再通过此准备项；环境要求更完整 | 准备阶段沿用当前固定头源，S2 改用固定完整 SDK 再复测；现在可推进，但增加后续环境切换 |

执行 [附录 A](#附录-a-vulkan-sdk-安装与复测)。SDK 头源 Debug/Release 真实探针各 5/5；2026-10-07 用户确认验证通过并允许关闭 Q04，此项已关闭，不再要求重复安装或探针。此前自动身份记录未取得的安装器实际摘要及 Info/Cube 独立日志仍如实保留为历史记录限制，不伪造文件或摘要，也不再作为 S1 进入阻断。

### Q05 离线证据范围

当前已有真实无下载路径审计及进程网络约束，未物理断开网络；选择证据门槛，不改变固定源码和不自动下载依赖的要求。

| 最终敲定方案 | 候选方案1（推荐）                               | 候选方案2                                   | 候选方案3                                                |
| ------ | --------------------------------------- | --------------------------------------- | ---------------------------------------------------- |
| 1      | 接受现有无下载路径审计作为准备证据，S1 复核真实接入；明确不声称物理断网实测 | 由用户控制断网，在新目录完整构建/安装后再通过此项；证据直接，但需临时中断网络 | 使用网络阻断的隔离机器/虚拟机，以预装工具链和固定 vendor 全新构建；不影响主机网络，但需额外环境 |

### Q06 短采样参数

当前 Q06 只在本机 RTX 5070 Ti 进行同 adapter、GLFW/SDL 同条件的 static/walk/edit，每场景按已选方案1重复；原双机环境不再作为当前前置，候选表保留决策时原文。正式长时九轮仍延期；“3/5 次”是短采样重复数，不是重新采用正式九轮协议。

| 最终敲定方案 | 候选方案1（推荐）                                    | 候选方案2                                        | 候选方案3                                           |
| ------ | -------------------------------------------- | -------------------------------------------- | ----------------------------------------------- |
| 1      | 每轮预热 10 秒、采样 30 秒，每场景 3 次；每版每机约 6 分钟，不含启动/切换 | 每轮预热 20 秒、采样 60 秒，每场景 3 次；每版每机约 12 分钟，单轮观察更长 | 每轮预热 10 秒、采样 30 秒，每场景 5 次；每版每机约 10 分钟，重复更多但单轮仍短 |

共同复测规则：最小化/尺寸变化等运行条件导致的无效轮保留日志并最多补跑一次；再次无效则停止该场景排查。有效的慢帧/失焦帧不删除，成绩超出已确认预算时仅追加同参数一组复测并保留全部结果，不能挑轮次报通过。程序故障、输入丢失、泄漏、协议不符或画面降规格仍阻断验收，不用性能复测掩盖；数值预算与最终判定规则仍按 Q07 在 SDL 验收对比采样前确认。

### Q07 预算确认时点

只细化既定 D12/A 的时间安排。当前 P95/P99、首次可操作时间、working set/private bytes 的阈值须先由本机 RTX 5070 Ti 同条件 GLFW 预采样评估波动后提出；不要求另一 GPU 或第二机器预采样，现在不填无数据依据的百分比，也不根据 SDL 成绩倒推通过线。下表保留原候选及已选方案1，不把未选候选中的双机安排恢复为活动门槛。

| 最终敲定方案 | 候选方案1（推荐）                                          | 候选方案2                                       | 候选方案3                                   |
| ------ | -------------------------------------------------- | ------------------------------------------- | --------------------------------------- |
| 1      | 预采样/提出预算与已授权实施并行；在 SDL 验收对比采样前由用户确认阈值及复测规则，未确认不判通过 | S1 可先做，但双机 GLFW 预采样与预算确认完成后才进入 S2；增加较早的性能门槛 | 先完成双机 GLFW 预采样与预算确认，再开始 S1；前置最严格，准备周期最长 |

#### Q07 预采样结果与内存预算候选

**002 状态：无用数据 / 已废弃。** 2026-10-09 用户明确要求“将002标记为无用数据”。本组约 99.49% 正式采样时间失焦，不再用于 P95/P99、内存、首次可操作时间的预算制定或验收对比；下方数字和 MB01 旧推导只保留为历史记录，不作为当前通过线来源。用户随后明确确认的 MB01 绝对限额另记于 [MB01 内存预算确认](#mb01-内存预算确认)，不恢复 002 的使用资格。原始文件不删除、不改写，既有 9/9 协议有效事实也不改成产品失败；数据使用资格与采集有效性分开记录。当前活动预采样依据仅为 [003](#2026-10-09-p95p99-重复采样)。

2026-10-08 用户暂停 Fix1，要求先采 P95/P99、制定内存预算，并确认默认频率、平衡模式。已原样使用冻结 GLFW Release 游戏 `30e7aee1…`，static/walk/edit 各三次独立启动，10 秒预热、30 秒采样、allow-unfocused，无自动重试、无 status 轮询。九轮正常退出且 `finished`、完整帧数据、协议/workload=2、seed=424242、441 区块、1920×1080、4x MSAA 和实际 RTX 5070 Ti 均核对成立；三场景末帧截图已查看。

实际环境：Ryzen 7 9700X、Windows 11 build 26200、约 63,124 MiB 物理内存、GL 4.6 / NVIDIA 616.64。电源方案 GUID 为平衡；无超频/降压是用户声明，不把单次 CIM 频率读数当锁频证据。CPU/GPU 温度未采集；驱动强制 VSync 等设置未独立确认。大多数帧为失焦，按既定策略保留，不宣称前台独占或消除了后台干扰。

本次是**旧身份单端探索预采样**：旧 GLFW 与当前 SDL 除窗口适配外还有 application/foundation/telemetry 等差异，不能把两者当正式同源 A/B；不关闭 Q06/A11 或 T2，不据此批准性能等价。现有 runner quick 实际是 10+10 各一次，未拿它替代这次 10+30 各三次。游戏未重新编译；小型校验器只导入现有 runner core/YAML 库，沿用真实退出码与终态校验，不绕过发布故障。

每格帧时为“三轮最小 / 三轮中位 / 三轮最大”，单位 ms；不是合并全部帧后的百分位。

| 场景 | P95 | P99 | 三轮最大 working set / private bytes（MiB） |
| --- | --- | --- | --- |
| static | 10.213 / 11.919 / 13.313 | 28.294 / 37.918 / 38.276 | 603.96 / 1197.31 |
| walk | 10.556 / 11.228 / 11.867 | 16.665 / 38.247 / 39.850 | 527.86 / 1122.91 |
| edit | 8.063 / 10.343 / 10.646 | 8.896 / 23.752 / 32.194 | 533.21 / 1128.47 |

共 32,050 个正式采样帧，GPU 查询缺失 4 个（约 0.0125%），不填零；CPU/GPU/present 明细保留于逐轮规范摘要与 metrics。按帧间隔估计约 99.49% 正式采样时间失焦，不能称受控前台预算。P99 轮间波动明显，仅三轮不能证明稳定尾部或长期性能，因此帧时/首次可操作预算暂不敲定。九轮首次帧交换返回为 971.28–1194.15 ms，是近似首次可见时间，不是输入延迟。

内存仅统计 `10 <= elapsed_s < 40` 的每秒观察：各轮 30 点、无缺失，最大点间隔约 1.068 秒；全组观测最大值为 working set **603.96 MiB**、private bytes **1197.31 MiB**。不覆盖加载、截图/导出或瞬时峰值，不是 VRAM；static 第一次该窗口首末增长约 74.70 / 74.18 MiB，不能由这次短测认定或排除泄漏，不另开修复工作。

以下 **MB01 旧推导已随 002 废弃而停用**，三个当时未批准的旧候选只供追溯，不修改 Q07 时间安排已选的方案1。当时以本组最坏观察值加 10% / 15% / 25% 策略余量，再向上取整到 64 MiB；这不是统计置信区间。方案1与2工作集上限相同是当时取整所致。旧推荐不作为当前依据；用户随后明确选择相同的方案2绝对数值，当前效力见 [新确认记录](#mb01-内存预算确认)，不伪称按 003 自动重算。

| 最终敲定方案 | 方案1：紧约束 | 方案2：平衡（推荐） | 方案3：宽余量 |
| --- | --- | --- | --- |
| MB01：002旧推导停用；当前决定见下方 | working set ≤ 704 MiB；private bytes ≤ 1344 MiB。更敏感地检出提交内存增长，余量较小 | working set ≤ 704 MiB；private bytes ≤ 1408 MiB。原推导约 15% 加取整余量，现仅作历史 | working set ≤ 768 MiB；private bytes ≤ 1536 MiB。空间更大，但较迟才发现增长 |

历史判定草案（不激活上述停用数值）：固定身份/画面/时长/策略，逐轮同时检查两个上限，不用平均掩盖最坏轮，不将工作集与 private bytes 相加。条件导致的无效轮按 Q06 最多补一次；程序/发布故障直接停，保留而不反复重跑。超过已确认上限时仅按 Q06 追加一组同参数复测，原超限不删除；仍有超限则记“未通过 / 待明确处置”，不继续挑绿。内存选择不自动批准帧时、首次可操作时间、同源 Q06 或整个 Q07。

证据：[逐轮与三方案小型汇总](evidence/q07-presample-002.json)、[原始运行摘要](../../../out/m3-t2/q07-glfw-presample-002/summary.json)、[程序/资源/工具与机器身份](../../../out/m3-t2/q07-glfw-presample-002/manifest.json)、[原 GLFW 身份](evidence/glfw-baseline-identity.json)。`001` 只有机器信息准备失败，启动游戏为零；改用只读电源查询后新 `002` 完成九轮，不是补跑或覆盖产品失败。原 CSV/现场保留在 `out`，不写入冻结包。所有 Fix1 原失败仍保持原样。

#### 2026-10-09 P95/P99 重复采样

按用户“再做一次采样”要求，在全新 `q07-glfw-presample-003` 目录重复三场景各三次 10+30 秒；与 `002` 的游戏、资源、校验程序/库、采集脚本 SHA 一致，参数和 allow-unfocused 策略未改。9/9 正常退出并达到 `finished`、帧数/百分位/场景/画面/设备等校验成立，原始 CSV 独立复算与小型汇总一致。未重建游戏、继续 Fix1、自动补跑或覆盖 `002`。

本组实际 GL 仍为 RTX 5070 Ti、GL 4.6 / NVIDIA 616.64，电源方案仍为平衡；默认频率沿用 2026-10-08 人工声明，不冒充当天重新确认或持续锁频证明。**本组所有正式采样帧均 focused=1，失焦为零；上组约 99.49% 正式采样时间失焦。** 程序/参数相同不等于全部运行条件相同：只能展示两组观察差异，不能将下降宣称为优化、迁移收益、Fix1 修复，或单独证明由焦点造成。

每格仍为三轮最小 / 中位 / 最大，单位 ms，未混池或删慢帧。

| 场景 | 本组 P95 | 本组 P99 |
| --- | --- | --- |
| static | 7.605 / 7.645 / 8.550 | 8.671 / 8.739 / 9.832 |
| walk | 6.796 / 7.501 / 7.525 | 7.335 / 8.756 / 9.703 |
| edit | 6.933 / 7.453 / 7.563 | 7.616 / 8.722 / 8.840 |

本组共 44,371 个正式采样帧，GPU 查询缺失 7 个（约 0.0158%），缺失不填零；每轮内存窗口均 30 点。working set / private bytes 全组最大周期观察为 558.51 / 1151.08 MiB，仍不是全过程峰值或无泄漏证明。2026-10-09 用户明确废弃 002，**003 是当前预算唯一活动预采样依据**；002 原记录保留，不混入统计或验收对比。这不代表两组性能提升已被证明。FB01/MB01 均已按方案2确认，T2 节点另由用户正式批准；首次可操作预算未定、正式同源 Q06/A11 证据和发布稳定性仍作遗留记录，Fix1 继续暂停。

证据：[本组小型汇总](evidence/q07-presample-003.json)、[本组原始运行摘要](../../../out/m3-t2/q07-glfw-presample-003/summary.json)、[本组身份与条件](../../../out/m3-t2/q07-glfw-presample-003/manifest.json)。分析生成时只输出测量数据（`MetricsOnly`），显式记录 2026-10-09，预算候选为空、批准状态为 false；这是用户预算/节点批准前的测量快照，原文件不改写，后续决定另记于下方及 [节点批准记录](evidence/t2-approval-20261009.json)。

#### MB01 内存预算确认

**2026-10-09 用户明确选定 MB01 方案2：working set ≤ 704 MiB、private bytes ≤ 1408 MiB。** 本次确认沿用这两个绝对限额，不重新启用 002 或其旧百分比推导，也不声称数值由 003 的 15% 取整得到。003 九轮已有周期观察均在该上限内；这不是当前 SDL 正式同源验收通过。

| 最终敲定方案 | 候选方案1：紧约束 | 候选方案2：平衡（已选） | 候选方案3：宽余量 |
| --- | --- | --- | --- |
| MB01：**方案2，2026-10-09 已确认**；[正式决策附录](../../spec/M3-T2-SDL3迁移.md#内存预算决策附录) | working set ≤ 704 MiB；private bytes ≤ 1344 MiB。较早检出私有提交增长 | working set ≤ **704 MiB**；private bytes ≤ **1408 MiB**。采用用户明确确认值 | working set ≤ 768 MiB；private bytes ≤ 1536 MiB。空间更大，但较迟发现增长 |

适用范围为 FB01 同一本机 RTX 5070 Ti、Release、固定画面/工作负载与完整前台短采样轮次；每轮只取 `10 <= elapsed_s < 40`、约每秒采集的两个指标各自**观察最大值**，逐轮同时检查，不相加、不用平均或百分位掩盖最大值。工作集是进程驻留工作集，private bytes 是私有提交字节，不是 VRAM；不覆盖加载、截图/导出或瞬时/全过程峰值，不认定无泄漏。超限仅按 Q06 追加一组同参数复测并保留原超限；产品/发布故障保留，不当成绩、不反复补跑。

方案1/3 原数值仅保留对照，不再待选，不改变生产实现或画质。首次可操作时间预算仍未定；本次 MB01 选择及 T2 节点批准分别记录，不将其升级为所有自动验收已通过。

#### FB01 P95/P99 预算候选

2026-10-09 用户明确确认“行，就按方案2来，记录下”，**FB01 最终采用方案2：P95 ≤ 10 ms、P99 ≤ 12 ms**，下述前台适用条件和逐轮判定规则同步冻结。不替代 Q07 时间安排已选的方案1，不混淆两个决策编号。依据 003 三场景九轮的最坏逐轮 P95 **8.5504 ms**、P99 **9.8316 ms**，不用中位数掩盖较慢轮次。三个方案继续保留对照，均采用三场景统一上限；余量是工程策略，不是统计置信区间或已测出的噪声上界。

| 最终敲定方案 | 候选方案1：紧约束 | 候选方案2：平衡（推荐） | 候选方案3：宽余量 |
| --- | --- | --- | --- |
| FB01：**方案2，2026-10-09 已确认**；[详细对比](#附录-c-fb01-帧时预算对比) | P95 ≤ **9 ms**；P99 ≤ **11 ms**。相对观察最坏值约留 5.3% / 11.9% 余量，细小回退较易触线 | P95 ≤ **10 ms**；P99 ≤ **12 ms**。约留 17.0% / 22.1% 余量，兼顾迁移回退检查与短测波动 | P95 ≤ **12 ms**；P99 ≤ **16.7 ms**。约留 40.3% / 69.9% 余量，较少触线，但可能放过明显回退 |

**已选方案2，采用平衡余量。** 当前改动目标是窗口库迁移，不是重做性能优化；10 / 12 ms 比沿用中端机器目标更能发现本机回退，也避免仅凭一组短测就要求几乎贴着当前成绩运行。预算只约束此条件下的帧时分位数，不承诺全部帧达标、无卡顿、固定 FPS 或性能等价；003 最长采样帧仍为 18.1671 ms。

已确认的共同适用条件：本机 RTX 5070 Ti、Release、1920×1080、4x MSAA、VSync 请求关闭、seed 424242、441 区块及固定工作负载，10 秒预热 + 30 秒采样、static/walk/edit 各三次独立启动。**预算比较对象是正式采样全程有焦点的完整轮次**。仍沿用 allow-unfocused：失焦轮在协议上可以有效，整轮与慢帧均保留，但单列为“不满足本次前台预算比较条件”，不能删掉失焦帧后重算通过，也不自动改为 strict 无效或反复补跑。额外重采此类条件不符数据须另行确认；本次不制定后台预算，也不改变 M2 等历史失焦基准的结论。

已确认的判定规则：每场景三轮均逐轮同时满足 P95 和 P99 上限才报告该场景数值通过；三场景分别报告，不混池、不取平均或只报最佳轮。frame_ms 沿用相邻交换缓冲返回之间的间隔，不是物理显示延迟，CPU/GPU/present 分位数不相加。成绩超限仅按 Q06 追加一组同参数复测，保留原超限；若追加组仍有超限则未通过，若全部满足也必须同时报告初次超限及复测通过，不能写首次全部通过。产品/发布故障不当成绩，不靠追加采样掩盖。

FB01 当次用户确认只冻结上述阈值及比较规则，已同步 [正式计划](../../spec/M3-T2-SDL3迁移.md#短采样与预算确认)；同日后续 MB01 与 T2 节点批准分别按 [内存确认](#mb01-内存预算确认) 和 [正式节点批准](README.md#t2-正式节点批准) 记录。正式同源 GLFW/SDL 对照仍未完成，不把旧 GLFW 003 直接与当前 SDL 当同源 A/B。当前不执行新采样、不恢复 Fix1，不伪造首次可操作预算或 Q06/A11 实测通过。

### Q08 生产实施授权

只在其余选择已落实、前置补项已完成或按 Q03 明确确认安排后生效；填写本表本身不表示 T2 节点通过。

| 最终敲定方案 | 候选方案1（推荐）                                     | 候选方案2                                              | 候选方案3                                 |
| ------ | --------------------------------------------- | -------------------------------------------------- | ------------------------------------- |
| 1      | 批准候选增量并授权按 S1–S5 顺序实施；保留每步验证门槛，最终 T2 节点另行申请批准 | 先授权 S1–S3，实际 SDL platform 回归后再评审 S4/S5；交接与双机交付暂不启动 | 仅继续准备，暂不授权生产迁移；当前游戏继续 GLFW，补证据后再做 P06 |

## 公共接口候选

公共头仍不包含 SDL3、Windows、GLAD、D3D12 或 Vulkan 头，不发布原生对象、SDK 枚举或公开 `void*`。窗口、输入和平台操作继续由应用主线程同步调用。

| 入口 | 候选增量或保留行为 | 冻结前必须确定 |
| --- | --- | --- |
| Init/Free | Init 成功后记录本模块取得的 SDL video 初始化责任；重复 Init 不增加本模块持有次数。Free 只释放本模块取得的责任，重复调用安全 | P02 验证普通 main 的私有入口准备；Free 的前提是所有窗口及所借用图形资源已释放，不替调用者销毁 GPU 对象 |
| Create | 增加项目中立的 OpenGL/Native/Vulkan 用途。现有默认创建仍为 OpenGL；窗口和需要的 GL context 全部成功后才发布拥有的 Window | Q01 末参模式已写入生产头；标题借用仅持续到调用结束；当前转换与失败保证以 [Window 实现契约](../../project/architecture/platform/Window-类设计.md) 为准，硬件 DPI 仍须 S3 验证 |
| Destroy/析构 | Destroy 幂等，销毁本 Window 所有的 GL context 和系统窗口；析构只兜底 Destroy，不结束全局 video 生命周期 | renderer 和借用 HWND/surface 的调用方已结束使用；失败窗口不对外发布；不允许一半窗口仍可继续绘制 |
| PollInt | SDL 事件先转换为中立事实；过滤窗口 ID，同时处理全局退出。发布这一帧有序指针事件和失焦标记，不直接修改 ECS/相机或提交图形命令 | 等待取得的首事件如何暂存；事件缓冲失败如何保留具名诊断并停止本帧，不能伪装成成功空输入 |
| CaptureInput/Input | 11 个项目 Key 映射 scancode，每键按现行规则消费一次；短按/同帧按放锁存到一次采样。Input 仍为同步借用，不跨下一次变更/销毁保存 | 最终按住态与未消费短按态分开；repeat 不制造新短按；Benchmark 仅消费 Escape，不采纳玩家键鼠 |
| ResetInput | 清当前快照、未消费锁存、待发布指针事件和鼠标恢复基准，保持失焦/暂停恢复首帧 dt=0 | 失焦按住及恢复时何时重新采纳仍按住的键；相对模式切换和事件消费顺序；不能把旧积压运动带入恢复帧 |
| WaitEvents | 秒到等待单位的转换有边界；取得的事件经相同适配路径消费恰好一次，不吞关闭/焦点/按键 | 小于一等待单位、零值和上限的规则；等待只累积事实，何时由 PollInt 发布 |
| Time | 单调高精度时间，公开单位仍为秒 | 起点及 SDL 初始化/退出前后的合法调用期；不改 Benchmark 的 CPU/GPU/present 统计定义 |
| SetCursorMode | Lock/Hidden/Normal 可以切换和恢复，保持向右/向上为正、现有灵敏度与焦点行为 | 对照实测决定系统加速度/相对输入策略，不默认把迁移变成手感调整 |
| width/height、SetSize/GetAspectRatio | width/height 继续表示实际绘制像素；`SetSize` 保持现行逻辑窗口尺寸入参，不改成像素；零像素代表暂不可绘制 | resize 后查询实际像素，不把请求值直接赋给 width/height；Benchmark 要求实际 1920×1080，不能把逻辑尺寸相等当作通过 |

Q01 已选小型项目枚举 `enum class WindowMode : std::uint8_t { OpenGL, Native, Vulkan }`，已作为真实 `SymoCraft::Window::Create` 的最后一个参数，默认 OpenGL，保留当前调用点；不再同时新增 `CreateOptions` 或模式首参重载。窗口私有实现记录不可变用途：OpenGL/Vulkan 使用各自专用标志，Native 两者都不设置；无 GL 用途不能调用 GL 当前化/呈现。不新增拥有 SDL 运行期的公开类；现行签名以 [Window 头](../../../game/modules/platform/include/symocraft/platform/window.h) 和 S4 声明检查为准。

私有桥接候选分为现有 GL 头、原生 HWND 头、按需 Vulkan 头。Vulkan 所需扩展以项目拥有的字符串副本返回，过程入口统一来自 SDL loader；surface 由探针或后续 renderer 拥有，销毁在 instance/window 之前。通用 GL 头不包含 Vk 头，不要求 GL-only 使用者安装 Vulkan SDK。

## 所有权与私有桥接

| 对象或事实 | 所有者 | 借用方与期限 | 释放责任 |
| --- | --- | --- | --- |
| SDL video 初始化责任 | platform 的现行静态生命周期路径 | app 主线程；从成功 Init 到 Free | Window/所有图形对象结束后释放本模块取得的责任，失败初始化不记为成功 |
| SDL 系统窗口 | 对应 Window::Impl | app 通过项目 Window；私有桥接借用 | Window::Destroy/析构唯一销毁，不允许 app 或探针直接销毁 |
| SDL GL context | OpenGL 用途的 Window::Impl | renderer 的当前化、加载、绘制及呈现 | renderer 先释放图形资源，再由 Window 释放 context，最后销毁窗口 |
| Native HWND | 系统窗口，不另有拥有者 | 授权私有桥接及原生探针，至窗口销毁前 | 不单独 DestroyWindow，不缓存到窗口销毁后；验证有效 HWND 不等于 D3D12 交换链验收 |
| Vulkan loader 入口及窗口关联加载责任 | 按所选 SDL Vulkan 路径固定一个来源 | 探针/后续后端，仅在对应加载责任有效期间 | SDL 窗口关联加载责任随窗口结束；不得额外加载另一来源来掩盖入口或寿命问题 |
| Vulkan instance/surface | 准备阶段为探针；后续 T3 为后端 | platform 只按私有桥接协助创建 surface，不接管实例或 GPU 资源 | 探针先销毁 surface，再销毁 instance，再销毁窗口；T3 实际设备另需结束在途工作并清理其资源 |
| Vulkan 必需扩展名 | `GetRequiredExtensions` 返回的 `vector<string>` | 调用方拥有名称副本，但不因此延长窗口/loader 寿命 | 当前实现查询后立即复制，不缓存 SDL 借用字符串；实例/surface 创建仍须匹配活窗口 |
| InputSnapshot/PointerEvent 缓冲 | Window::Impl | app 当前同步消费期 | 下一次发布/重置/销毁可使引用失效，调用者不跨该边界留指针或引用 |

GL 桥接保留现有六项职责：当前化、当前 context 查询、GLAD 函数地址、扩展查询、VSync、呈现。Native/Vulkan 的 SDK 类型仅存在于授权私有桥接或探针；游戏调用方只选择项目窗口用途，不读取 SDL 原生句柄。需要新增私有头路径时同步边界授权和负向测试，不扩大整个 platform/src 的可见范围。

GL 的当前化、VSync 和呈现若返回失败，候选实现必须具名报告并阻止无效继续提交；不能因为旧桥接返回 void 就继续吞失败。能力查询的合法“不支持”与无当前 context/查询失败应能区分。Vulkan 探针要求真实扩展、instance 和 surface，不创建或声称完成游戏 GPU 后端。

准备期输入候选增加 `PhysicalInputState` / `SynchronizePhysicalState`，当时的 185 项合成检查保留为历史。当前生产 drain 后、Capture 前采新 SDL 状态；active Reset 清短按、公开快照和运动，保留 known held，并在 Capture 前再次采新状态。失焦/最小化清 held，恢复后重新采纳真实按住或释放态；repeat 不制造新短按；不采后台全局键鼠，Benchmark 保留本窗口 Escape 事件。S3 当前纯适配器各 263 检查、真实库输入各 21/21 和两配置分配故障各 4/4 见 [S3 记录](README.md#s3-输入与窗口行为)；用户实际硬件观察与其它布局/DPI条件不由合成结果替代。

SDL 相对运动默认不采用系统加速度，而当前 GLFW 未启用 raw mouse。连续对照工具默认请求 `SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE=1`，另有显式未缩放诊断选项；两端共用冻结游戏的 `ApplyPointerInput`/`Camera::Scroll`。本次用户确认的人工手感结果及范围见上方“已确认项目”；设置 hint 和自动渲染本身仍不证明手感等价，生产适配后须按真实 platform 再回归。

Q02 已选保持 GL 六项签名，在当前化、VSync、呈现失败时具名抛错；理由及另外两个方案见 [附录 B](#附录-b-q02-失败表达与后续图形-api-迁移)。`GetProcedure` 可返回空供 GLAD 判断可选函数，扩展查询先要求有效当前 context。Native 已实现为 `NativeBridge::GetHandle(Window&) -> HWND`；Vulkan 已实现 `GetInstanceProcedureAddress`、自有 `vector<string>` 的 `GetRequiredExtensions` 与借用 instance 的 `CreateSurface`/`DestroySurface`，分配回调须成对一致。上述 SDK 只进入授权私有头，不写入生产公开头；S4 交接不承诺非空 surface 的重复销毁安全或完整 GPU 后端。

等待候选规则为拒绝负数/非有限值、零值不阻塞、正数向上取整为毫秒并限制 `Sint32` 上限；等待前清本次 SDL error，false 时区分超时与新错误。首事件只累积、下次 Poll 发布一次。Time 限定 Init 至 Free，以本次初始化起点返回单调秒值。正常退出先释放 GPU，再 checked context/window/video 清理；析构仅作 best-effort 兜底，不因清理异常中断后续释放。

## 失败责任与清理矩阵

| 失败点 | 失败时可能取得的资源 | 处理责任与对外保证 | 准备证据 |
| --- | --- | --- | --- |
| video 初始化或入口准备 | 无成功初始化责任，或局部入口状态 | platform 记录操作名与库诊断，保持未初始化；app 正常错误退出，不继续 Create | 实际失败或受限测试注入；重复 Free |
| Window/输入缓冲分配 | 局部 C++ 对象 | 不发布 Window；局部 RAII 回收；不跨 C 回调边界抛异常 | 分配失败探针；无 public 故障开关 |
| 系统窗口创建 | 局部 Window，无有效系统窗口 | 不建立借用句柄；app 仍可调用幂等清理 | 创建失败与诊断记录 |
| GL context 创建/当前化 | 有系统窗口，可能有 GL context | 先释放已创建 context，再释放窗口；不发布“可绘制”状态 | context 创建、当前化失败及重试/退出 |
| GLAD 加载/GL 属性检查 | 已发布 Window/context，renderer 部分状态 | app 先结束 renderer 的部分资源，再释放 Window/context；不降 GL/MSAA 规格继续跑 | 真实驱动诊断、部分初始化清理 |
| HWND 或 Benchmark 任务栏策略 | 有系统窗口，无完整启动 | Win32 失败具名报告；移除已设置的本窗口属性后销毁；不误改其它 HWND | HWND、NonRudeHWND 与隐藏后显示探针 |
| Vulkan 扩展/loader/instance | 有 Vulkan 窗口，可能有 instance | 探针清理已取得对象；缺环境清楚失败，不影响未选 Vulkan 的 GL/Native 运行 | 启用/关闭探针的配置差异、真实环境结果 |
| Vulkan surface 创建 | 有窗口和 instance，可能有 surface | 探针释放已成功 surface，再 instance，最后 Window；所有入口在 loader 有效期内使用 | surface 失败、逆序退出和重复清理 |
| 事件转换或缓冲分配 | 有 Window/context、上一已发布输入 | 停止本帧正常模拟/绘制，交给 app 具名退出与统一清理；不生成误导的成功空快照 | 真 platform 故障探针及退出记录 |
| WM_CLOSE/全局退出/用户取消 | 正常运行期对象 | 转关闭请求，走既有 app 清理及结果落盘；不在事件处理内直接销毁图形对象 | runner 在强制结束前正常完成；关闭事件经过等待也不丢失 |

故障注入只放根级 test 的受限 fixture/测试私有访问中，不向游戏公共 API 增加 SDK 类型或开关。不以 mock 成功替代真实窗口/context/surface 能力；不能可靠制造的真实失败须记录限制与替代验证范围，不能勾成已通过。

## P05 评审检查

- [ ] P02 的固定源码/入口/静态选项与 Init/Free 责任一致。
- [ ] P03 三种用途各有真实成功、失败及逆序清理记录；GL 不要求 Vulkan SDK/运行环境。
- [ ] P04 已明确 scancode、短按锁存、失焦恢复、相对运动与等待首事件规则。
- [ ] 中立创建模式与私有 HWND/Vulkan 接口的签名、借用期、失败保证有实验依据。
- [ ] GLAD/Vulkan 函数入口来源、实际图形属性和 loader 寿命可追溯。
- [ ] 真实 platform 回归覆盖上表；现有纯 KeySampling 单测不作替代。
- [ ] 新私有桥接路径、SDL 头/target 授权及 CPU/工具防泄漏规则有正向和负向用例。
- [x] P06 落实八项已选安排和完整 SDK 头源复测；2026-10-07 用户确认 Q04 关闭、P06 通过，Q08 授权生效。

上述 P05 的完整失败覆盖尚未完成，手感子项按用户反馈通过。八项决策及 SDK 头源双配置结果已同步，用户已批准 P06；Q03 明确将剩余窗口/桥接失败随 S2、输入/事件失败及硬件/DPI/任务栏随 S3 补齐，T2 验收前全部关闭。准备通过不能把 T2、三后端玩法、两项延期问题或后续 T3/T4/T5 标记完成。

## 附录 A Vulkan SDK 安装与复测

本附录执行 **Q04/方案2**。需要安装的是 Vulkan 开发 SDK，不是已供 MSVC 使用的 Windows SDK，也不是仅含运行组件的 Vulkan Runtime。SDK 安装属于开发环境准备，不把 SDK、SDK 附带 SDL 或 GPU 驱动复制进 `vendor`，不替换已固定的 `vendor/sdl3`。

### 当前复测结果

2026-10-07，SDK 已位于 `C:/VulkanSDK/1.4.363.0`，核心头/库/Info 文件及机器级环境路径已核对。SDK 头源 Debug/Release 纯 CTest 各 1/1，真实窗口探针各 **5/5**；两配置实际编译依赖均来自 SDK 的 14 个头，头版本 363，继续使用 SDL loader，真实 instance/surface 创建和逆序清理正常。见 [Debug 运行](../../../out/m3-t2/archive/preparation/platform/vulkan-sdk-debug-runtime-003/summary.json)、[Release 运行](../../../out/m3-t2/archive/preparation/platform/vulkan-sdk-release-runtime-003/summary.json)、[编译头源与运行身份](../../../out/m3-t2/archive/preparation/platform/vulkan-sdk-verification-001/verification.json)。

此前 `002` 结果因验证脚本丢失 OS 退出码全部被拒绝，原记录保留。已按现有工具模式缓存进程 handle，并将相对结果目录按仓库根解析、限制在 out；Windows PowerShell 5.1 从 System32 启动的新 `003` 已正确记录 `0/0/0/1/0`，不再以 `null` 误判。

**2026-10-07 用户确认 Q04 验证通过并关闭、P06 通过。** 本文以该人工确认记录当前批准状态；此前 [自动身份快照](../../../out/m3-t2/archive/preparation/platform/vulkan-sdk-verification-001/verification.json) 中安装器摘要、Info/Cube 日志未独立取得及 `q04_overall_complete=false` 是批准前的历史状态，不覆盖原快照、不补造摘要。下方安装步骤保留为复现说明，不要求重装或重复双配置探针；此批准不表示生产 SDL 游戏、完整失败矩阵或 T3 GPU 后端通过。

### 用户安装步骤

1. 打开 [LunarG 官方下载页](https://vulkan.lunarg.com/sdk/home/)，选择 **Windows → x64/x86 → SDK Installer**。本次固定安装基准为 **1.4.363.0**，文件名 `vulkansdk-windows-X64-1.4.363.0.exe`；该版于 2026-09-29 发布。不要下载 ARM64、Runtime Installer 或 Runtime zip 代替 SDK；后续网页出现新版也不自动改本次固定版本。[官方版本说明](https://vulkan.lunarg.com/doc/view/latest/windows/release_notes.html)
2. 安装前核对下载文件 SHA-256。官方该文件公布值为 `94a82d378f7a5e3e54c9db7d2fb7016af136e14ac0a18dbf0f2f67a36352d141`，这是官方期望值，不是本机已下载核验结果。以下示例假设下载到当前用户的 Downloads，路径不同时只改 `$Installer`；不一致就停止，不运行安装器。[官方文件与摘要](https://vulkan.lunarg.com/sdk/home/)

```powershell
$Installer = Join-Path $HOME 'Downloads/vulkansdk-windows-X64-1.4.363.0.exe'
$Expected = '94a82d378f7a5e3e54c9db7d2fb7016af136e14ac0a18dbf0f2f67a36352d141'
$Actual = (Get-FileHash -LiteralPath $Installer -Algorithm SHA256).Hash.ToLowerInvariant()
if ($Actual -ne $Expected) { throw 'SDK installer SHA-256 mismatch.' }
$Actual
```

3. 双击安装器，按界面确认许可和所需权限；建议目录 `C:\VulkanSDK\1.4.363.0`。保留核心开发组件、工具和验证层，不为 T2 额外勾选 SDL/GLM/Volk/VMA、shader Debug 库或 ARM64 交叉组件；本工程继续使用自己的固定依赖。不要手动把 DLL 放入工程或系统目录，不设置全局强制验证层。[官方安装说明](https://vulkan.lunarg.com/doc/view/latest/windows/getting_started.html)
4. 安装结束后重开 PowerShell、CLion 及用于后续构建的 Codex 窗口，使新环境生效。应有 `VULKAN_SDK` 和 `VK_SDK_PATH` 指向所装目录，`PATH` 包含其 `Bin`。随后在新开的 PowerShell 执行下面的检查；自定义目录时同步修改 `$Sdk`，不要仅凭命令在 PATH 中能运行就认为版本正确。[环境变量与重启说明](https://vulkan.lunarg.com/doc/view/latest/windows/getting_started.html)

```powershell
$Sdk = 'C:/VulkanSDK/1.4.363.0'
$env:VULKAN_SDK
$env:VK_SDK_PATH
Test-Path -LiteralPath "$Sdk/Include/vulkan/vulkan_core.h"
Test-Path -LiteralPath "$Sdk/Lib/vulkan-1.lib"
Test-Path -LiteralPath "$Sdk/Bin/vulkaninfoSDK.exe"
& "$Sdk/Bin/vulkaninfoSDK.exe" --summary
if ($LASTEXITCODE -ne 0) { throw 'SDK Vulkan Info failed.' }
& "$Sdk/Bin/vkcube.exe"
if ($LASTEXITCODE -ne 0) { throw 'SDK Vulkan Cube failed.' }
```

三个 `Test-Path` 应为 `True`，环境路径应与实际固定目录一致；Info 应列出真实 GPU/驱动和 API 能力，Cube 应出现正常运动画面，观察后关闭窗口。Windows SDK 中的 Info 文件名是 `vulkaninfoSDK.exe`，这里指定 SDK 内绝对路径，避免误用驱动包附带的同名工具。[Vulkan Info 用法](https://vulkan.lunarg.com/doc/view/latest/windows/vulkaninfo.html)、[官方安装验证](https://vulkan.lunarg.com/doc/view/latest/windows/getting_started.html)

SDK 不安装 GPU 驱动，安装新版 SDK 也不证明设备支持相同版本的 API；以实际设备报告为准。本机旧探针已能加载 Vulkan，不能因此跳过上述 SDK 核对，也不因安装 SDK 默认要求更新显卡驱动。[SDK 与驱动的区别](https://vulkan.lunarg.com/doc/view/latest/windows/getting_started.html)

后续新环境完成安装后核对**实际 SDK 目录、版本，以及 Info/Cube 是否正常**；本机 Q04 已获用户确认关闭，无需再次提交相同验证。同一鼠标手感无需重复确认。若新环境工具失败，保留错误文本，先排查环境，不把失败复测记为通过。

### 仓库复测与证据

以下为可复现步骤，实际已执行结果单独记录在上方，不以命令文本代替结果。使用现有 [构建脚本](../../../test/experimental/sdl3/build.ps1) 和 [真实窗口验证脚本](../../../test/experimental/sdl3/verify-probes.ps1)，继续固定 MSVC 14.38 与 portable CMake 3.22.6。显式使用绝对路径，Debug/Release 各用一个全新目录；示例 `001` 已存在时改用新编号，不覆盖旧证据。

```powershell
$Root = 'F:/GameDevelop/OpenGLProject/Symocraft'
$Sdk = 'C:/VulkanSDK/1.4.363.0'
$CMake = "$Root/out/m3-t2/tools/cmake-3.22.6/portable/cmake-3.22.6-windows-x86_64/bin/cmake.exe"
foreach ($Configuration in @('Debug', 'Release')) {
    $Name = $Configuration.ToLowerInvariant()
    $Build = "$Root/out/m3-t2/probe/vulkan-sdk-$Name-001"
    $Evidence = "$Root/out/m3-t2/probe/vulkan-sdk-$Name-runtime-001"
    & "$Root/test/experimental/sdl3/build.ps1" -Configuration $Configuration -Action Test `
        -Vulkan -HeadersRoot "$Sdk/Include" -CMakePath $CMake -BuildDirectory $Build
    & "$Root/test/experimental/sdl3/verify-probes.ps1" `
        -Executable "$Build/SymoCraftSdl3Probe.exe" -OutputDirectory $Evidence -Vulkan
}
```

复测后核对并留证：

| 核对项 | 必须保留的证据与边界 |
| --- | --- |
| SDK 身份 | 官方版本/下载来源、安装器实际 SHA-256、实际安装目录与组件；外部头文件清单及摘要，至少标明 `vulkan_core.h` 的 SHA-256 和 `VK_HEADER_VERSION` |
| 真实头源 | 新配置的 `SYMOCRAFT_VULKAN_HEADERS_ROOT` 及探针编译依赖指向固定 SDK `Include`，不回落到旧探针头源；SDL 自身原样源码中的内部头不在本次替换范围 |
| 工具与运行环境 | MSVC/CMake/配置、exe SHA-256、SDK Info 工具路径/身份、原始 GPU/驱动/loader/API 输出；不把 SDK 版本等同于驱动版本 |
| Debug/Release 运行 | 各配置纯 CTest 成功，再各自取得真实 5/5 探针结果，含 GL/Native/Benchmark、初始化失败和 Vulkan instance/surface；纯输入 CTest 不替代真实 Vulkan 窗口运行 |
| 所有权与来源 | 真实扩展、instance/surface 创建和逆序清理，无 cleanup 诊断；入口继续来自 SDL loader，不因 SDK 有 `vulkan-1.lib` 就新增另一入口来源 |
| 准备范围 | 新结果只关闭对应 SDK 环境/头源准备项，不自动关闭完整失败矩阵、生产 SDL 游戏、双机验收或 T3 GPU 后端 |

现有 [无下载路径审计脚本](../../../test/experimental/sdl3/verify-cmake-offline.ps1) 仍将 `vulkan_headers_are_installed_sdk` 固定写为 `false`，输入 manifest 也未纳入外部 SDK 头；不能直接把它传入 SDK 路径后的旧式摘要当作 SDK 身份证明。复测需新增独立 SDK 身份清单；若后续扩展此审计脚本，再按实际头源生成标记和外部输入清单，历史证据不改写。安装器下载与环境安装是一次性准备，不改变 Q05/S1 的工程无自动下载依赖要求。

返回 [Q04](#q04-vulkan-探针头源)。

## 附录 B Q02 失败表达与后续图形 API 迁移

### 先区分责任

Q02 只决定 **platform 私有 GL 桥接如何报告必要操作失败**，不决定 T3 的设备、交换链、命令提交、GPU 同步或完整 RHI。未来引入 D3D12/Vulkan 不要求把这些 SDK 的全部返回码提前塞进 T2。现有 [六项签名](../../../game/modules/platform/src/graphics_bridge/graphics_bridge.h) 的合法结果与已选失败保证如下；S2 已实现 SDL3 checked 转调和阶段失败测试，完整 T2 验收仍以实施记录为准，不以签名或异常捕获本身替代矩阵结果。

| 当前入口 | 成功或合法结果 | 必须识别的故障 |
| --- | --- | --- |
| `MakeCurrent(Window&)` | GL 窗口/context 成为当前 context | 非 GL 用途、已销毁窗口或当前化失败；不得继续 GLAD/绘制 |
| `HasCurrentContext()` | `true` 有 context，`false` 合法无 context | 合法 `false` 不等于运行故障；需要 context 的操作另外校验 |
| `GetProcedure(name)` | 函数地址或空地址 | 空地址交 GLAD 判断是否必需，不能把缺少每个可选函数都变成致命故障 |
| `ExtensionSupported(name)` | 有效当前 context 下，`false` 表示不支持 | 无当前 context、无效查询或查询故障不能伪装成“不支持” |
| `SetVsync(bool)` | 请求明确设置成功 | 失败不能静默沿用旧设置并继续生成误导的 Benchmark 成绩 |
| `Present(Window&)` | 有效窗口/context 下交换缓冲成功 | 失败后停止正常帧提交，不仍宣称本帧呈现成功 |

### 三方案详细对比

| 比较项 | 方案1：保留签名，具名异常（推荐） | 方案2：`bool` 加自有诊断出参 | 方案3：项目中立 `BridgeResult` |
| --- | --- | --- | --- |
| 基本表达 | 必要操作失败抛异常；正常返回表示成功 | 成功/失败显式返回，错误对象通过出参交给调用方 | 返回自有状态、操作名和诊断，由调用方统一检查 |
| 当前调用改动 | 最少；renderer 已使用 `runtime_error`，app 已捕获运行错误 | 每个必要操作及 renderer 转发都需检查和适配 | 新增结果类型、检查入口及调用适配；不是只改一个返回值 |
| 能力查询 | `false` 表示合法不支持，查询无效另行抛错 | 必须分开“查询成功”与“支持值”，例如成功位加 `supported` 出参 | 结果也需区分调用状态和查询载荷，不能只设一个失败位 |
| 漏检风险 | 错误沿调用栈进入处理，不依赖每处显式检查 | 每处可能漏检；需 `[[nodiscard]]` 及调用覆盖 | 可 `[[nodiscard]]` 并集中检查，但仍须防止显式忽略 |
| 诊断寿命 | 异常拥有文字副本 | 诊断对象拥有文字，每次调用初始化，避免上次错误残留 | 结果拥有文字，状态、操作和诊断一致 |
| 与 T3 衔接 | 符合现有 `RenderError` 设计方向，不代替 `FrameResult` | 可在私有后端使用；若门面最终仍抛错，可能多一次重复适配 | 适合确需结构化检查的窄桥接，不自动成为通用 GPU 结果 |
| 主要代价 | 补齐 RAII、异常退出及清理隔离；不能在 C 回调或析构中抛错 | 调用点噪声、出参规则和漏检测试较多 | 要冻结状态集合与载荷，容易提前设计未实验的现代后端框架 |
| 更适合的目标 | T2 聚焦换窗口库，T3 单独完善现代渲染状态 | 明确偏好逐点显式错误处理，愿意修改全部窄调用点 | 明确希望平台桥接结构化返回，并接受新增类型与检查规范 |

三者都能适配后续 API；这里比较工程成本和风险，**没有测出异常或结果类型谁更快**。RAII 和清理责任不随错误表达改变，不能把 `bool`/结果类型当作自动解决资源释放的机制。

调用形状仅作对照，不是新增生产 API；以下 `Diagnostic`、`CheckBridge` 为说明性名字，方案2/3 未采用，不冻结或实现其类型与签名：

```cpp
// Option 1: failure propagates to the existing app error boundary.
GraphicsBridge::MakeCurrent(window);

// Option 2: the caller must handle the result and its owned diagnostic.
Diagnostic error;
if (!GraphicsBridge::MakeCurrent(window, error)) {
    throw std::runtime_error(error.message);
}

// Option 3: a shared checker applies the chosen failure policy.
CheckBridge(GraphicsBridge::MakeCurrent(window));
```

方案2 的示例特意展示当前 app 异常退出路径下的二次转译；选择它不等于必须重新建设整个 app 错误系统。方案3 可用小型项目自有类型表达状态/操作/文字诊断，不要求引入 C++23 `std::expected`，也不先建立全局 `GraphicsResult`。

### 现代图形 API 的状态由谁处理

Vulkan 的 `VK_SUBOPTIMAL_KHR` 仍可呈现，但需适时按新表面信息重建；`VK_ERROR_OUT_OF_DATE_KHR` 则须重建后才能继续呈现。后者虽是错误码，也不等于任何场景都必须立刻终止整个游戏，是否可恢复由后端的重建协议决定。[Khronos 交换链规范](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_swapchain.html)

DXGI 也区分状态与故障；遮挡状态不等于设备丢失。不过 flip-model 交换链不会返回 `DXGI_STATUS_OCCLUDED`，不能将其写成未来 D3D12 的通用必经行为。设备移除/重置与正常状态仍需后端分别处理。[Microsoft DXGI 状态](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-status)、[Microsoft Present](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present)

因此，现代 API 的 acquire/submit/fence/present 与恢复策略归**拥有交换链、设备和 GPU 在途资源的 renderer 后端**，不归仅拥有系统窗口的 platform。[T3 接口设计](../../spec/M3-T3-渲染器重构.md#公开数据与接口契约) 已拟定 `FrameResult` 表达 Presented/Skipped 及原因、`RenderError` 表达不可恢复故障；后端在帧边界处理 resize、零像素暂停和重建，应用以后不再额外调用 GL 桥接 `Present`。这是后续设计方向，不是已经实现的 GPU 能力。

选方案1仍可由后端将致命 `HRESULT`/`VkResult` 转为拥有原生码文字的 `RenderError`，将正常或约定可恢复状态转为 `FrameResult`/后端内部状态。选方案3也仍需上述分类和恢复设计，不能仅把原生码包进 `BridgeResult` 就认为支持了现代 API 迁移。

### 三方案共同约束与验证

1. 公开 API 不泄漏 SDL、HWND、HRESULT、VkResult 或公开 `void*`；SDK 类型仍仅进入授权私有桥接。诊断立即复制为项目自有文字，不缓存 `SDL_GetError()` 的借用指针，也不在清理后才取可能被覆盖的主错误。
2. 创建失败不发布半初始化 Window；回收已取得的 context 再销毁窗口。运行退出仍先在有效 context 下释放 renderer 图形资源，再释放 context/window/video。
3. 主故障保留为主错误，清理中的次级故障另行记录，不能覆盖主错误或阻断余下释放；析构 `noexcept`、best-effort 且有界，不替代正常 checked 清理。S2 已将 [app 清理路径](../../../game/app/src/application.cpp) 改为逐段捕获并继续 renderer/context/window/video 释放，保留原运行主错误；正常退出遇清理故障转 exit 3。纯清理顺序测试及真实资源失败退出已通过；context 释放后次级错误使用明确标记的受限 fixture，不声称制造了真实驱动销毁失败，见 [S2 证据](README.md#s2-窗口与桥接)。
4. 不跨 SDL/Win32/GL 的 C 回调边界抛异常；事件故障先保存，再回到受控 C++ 调用边界报告。方案2/3 同样不能把错误变成成功空输入。
5. HWND 借用、Vulkan loader/instance/surface 寿命不因方案改变；surface 仍由探针或后端拥有，不由 Window 接管 GPU 清理。
6. 分别测合法“不支持”、合法无当前 context、查询无效和必要操作失败；验证失败后没有无效绘制/Present，部分初始化和重复清理按矩阵留证，无法真实制造的点明确受限注入范围。

### 已选方案及依据

**Q02 最终采用方案1。** 该选择与评审推荐一致：当前 renderer/app 已采用异常运行错误路径，T3 又拟定了 `FrameResult` 加 `RenderError`；保留 T2 六项 GL 签名不阻碍后续退役 OpenGL 或引入 D3D12/Vulkan，能避免为了未来 API 提前扩大本次换库范围。实现时仍须补齐 checked failure 与清理隔离，不把现有异常捕获当作全部失败矩阵已通过。

方案3适合明确要求平台桥接结构化显式检查的方向，方案2适合偏好 `bool`/出参且接受逐点检查成本的方向；本次都不采用，保留对照不等于要求再次选择，也不建立尚未实验的完整 RHI。

**本表八项选择已完成，Q04 已关闭，P06 已获用户批准，Q08 的 S1–S5 授权已生效。** 2026-10-09 FB01/MB01 均已确认方案2，T2 节点由用户另行正式批准；首次可操作时间预算仍未定。剩余失败和验证缺口按 [遗留事项](../../spec/M3-T2-SDL3迁移.md#节点批准与遗留事项) 保留，不凭空补造阈值或通过证据。

返回 [Q02](#q02-gl-桥接失败表达) 或 [P06 决策表](#p06-用户决策表)。

## 附录 C FB01 帧时预算对比

| 比较项 | 方案1：9 / 11 ms | 方案2：10 / 12 ms（推荐） | 方案3：12 / 16.7 ms |
| --- | --- | --- | --- |
| 适用目标 | 优先检出小幅回退，接受较敏感门槛 | 当前窗口库迁移的回退警戒线 | 优先较低复测成本，接受较宽回退空间 |
| 功能及架构影响 | 不新增采样器或改画质 | 同左 | 同左；不代表可降低画质换通过 |
| 实施与迁移成本 | 相同校验规则；触线后的排查需求可能较高 | 相同校验规则；预期折中，尚未实测误报率 | 相同校验规则；可能较少触线，但不能据此保证省时 |
| 运行开销 | 阈值本身不增加采集步骤；未测量成本差异 | 同左 | 同左 |
| 风险 | 单组短测没有证明余量足以覆盖正常波动 | 仍可能漏掉小幅回退；不是长期稳定性证明 | P99 余量较大，可能放过本机明显退化 |
| 验证方法 | 固定同源 GLFW/SDL、前台可比条件，各三场景三轮逐轮判定 | 同左 | 同左 |

三个方案都不能单凭达标证明 SDL 与 GLFW 等价，也不取消发布稳定性和正确性门槛。**2026-10-09 用户已敲定方案2**；方案1/3 保留为决策历史，不再待选。选用理由与适用范围见 [FB01](#fb01-p95p99-预算候选)。

返回 [FB01](#fb01-p95p99-预算候选)。
