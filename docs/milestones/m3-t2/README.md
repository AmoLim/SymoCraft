---
type: 准备记录
status: T2节点已由用户正式批准通过，遗留验证与发布风险保留
project: Symocraft
module: M3-T2
created: 2026-10-07
updated: 2026-10-09
tags:
  - area/milestones
---

# M3-T2 SDL3 迁移准备记录

固定人工核查入口：[manul-verification.md](manul-verification.md)。当前需要你操作、审核报告和批准的项目统一从该表查看；S3 原人工填写和 S5 事实记录保留，不因固定入口建立自动通过或要求全部重验。

## T2 正式节点批准

**2026-10-09 用户明确批准 M3-T2 节点正式通过**，同时选定 **MB01 方案2：working set ≤ 704 MiB、private bytes ≤ 1408 MiB**；此前已确认 FB01 方案2：P95 ≤ 10 ms、P99 ≤ 12 ms。预算定义见 [候选契约](platform-contract-candidate.md#mb01-内存预算确认)，批准来源及范围见 [节点批准记录](evidence/t2-approval-20261009.json) 和 [正式计划](../../spec/M3-T2-SDL3迁移.md#节点批准与遗留事项)。当前节点为 T2 已通过，下一节点为 T3-R1 契约准备与冻结。

本次是用户明确的节点批准，覆盖此前遗留事项阻断 T2 节点批准的安排，不把失败/未执行记录改成实测通过：Win32 1175 与 Fix1 暂停、正式同源 Q06 未完成、首次可操作时间预算未定及部分人工条件/版本绑定材料缺口继续列为遗留事项。002 仍废弃，003 仍是旧 GLFW 单端预采样，不冒称 SDL 同源验收或性能等价；内存限额不代表全过程峰值或无泄漏。T3-R1/v1 冻结、R2 授权及下一玩法前中低端整机性能门槛仍独立成立。本轮只记录批准，不运行新采样/测试，不发布新包或修改冻结材料。下方早期未批准表述是相应日期的历史状态，以本节为当前节点状态。

2026-10-08 最新范围确认：T2/T3/T4 当前功能/兼容验收只限定本机 RTX 5070 Ti，其他 GPU 或第二机器不阻塞节点；指定中低端整机性能验证仍为下一玩法功能开发前门槛。以 [[project-scope#M3 本机 RTX 5070 Ti 验收与下一玩法阶段性能门槛|当前验收范围与后续性能门槛]] 和 [T2 正式计划](../../spec/M3-T2-SDL3迁移.md) 为准。下文双机安排及已做的其他设备包/实测保留为历史事实，不改结果，不作为当前额外必验；S3/S5/预算及 T2 整体验收按本机范围继续核对，不因删减环境自动通过。本次只更新文档，不执行新实验或覆盖包/证据。

依据 [正式计划](../../spec/M3-T2-SDL3迁移.md) 执行准备及后续实施。树叶剔除与视角突变不属于 T1 spec 或 T2 前置范围，两项继续延期。2026-10-07 用户确认 Q04 验证通过可关闭、P06 通过，随后完成 S1 生产依赖隔离及 S2 生产 SDL3 窗口/三模式桥接替换。2026-10-08 按用户“可以实施下个阶段”推进 [S3 输入与窗口行为](#s3-输入与窗口行为)，实现与自动/人工结果分开记录，未申请 T2 节点通过。

同日后续用户要求“暂时不做 Y9000P 的机器验证，冻结可运行验证包，继续进行下个阶段的工作”。已冻结原样 S3 Release 包，保留开发机当前人工反馈与硬件原始会话，并完成 [S4 平台契约交接](#s4-t3-契约交接)。延期不是取消 D09 双机范围、免验或通过；S3 整项、S5、预算与 T2 最终节点仍独立保留。

本轮已固定官方 SDL3 源码、保存并重建 GLFW 基线、验证独立 SDL 三模式能力，形成 [平台契约候选](platform-contract-candidate.md)。实验结果不替代真实 SDL 生产平台、鼠标手感、双机玩法或性能验收。

用户随后反馈“一切正常，继续进行开发”。本轮据此继续准备开发，新增持续输入对照、物理按住态重同步和 CMake 3.22 验证。2026-10-07 用户进一步明确人工手感对照已通过，范围与会话身份见 [候选契约决策表](platform-contract-candidate.md#p06-用户决策表)；不将手感结果扩写为全部硬件输入、SDL 游戏包或双机通过。

同日按用户最终填写同步 Q01/Q02/Q03/Q05/Q06/Q07/Q08 的方案1与 Q04 的方案2，八项选择已完成。Q02 采用保留 GL 六项签名、必要操作具名抛错；SDK 头源的双配置运行门槛已通过，用户随后确认关闭 Q04、批准 P06，Q08 的 S1–S5 顺序实施授权生效。此前未独立取得的安装器实际摘要与 Info/Cube 日志如实保留为历史身份记录限制，不伪造证据、不再阻断 S1。安装步骤与三方案迁移影响分别见候选契约的 [SDK 附录](platform-contract-candidate.md#附录-a-vulkan-sdk-安装与复测) 和 [Q02 附录](platform-contract-candidate.md#附录-b-q02-失败表达与后续图形-api-迁移)。

## 源码与环境

- SDL3：官方 `release-3.4.18`，完整 commit `829a65d769d935c4852f8159e964312c0957260a`。官方 ZIP SHA-256 为 `9cd42377704398796071b8597cd7e21da254a43bad98c4199739647adc13fa6f`，与 [官方资产](https://github.com/libsdl-org/SDL/releases/expanded_assets/release-3.4.18) 公布值一致。
- 按用户指示使用发行 ZIP 原样替换 `vendor/sdl3`；2183 个文件全部逐字节相同，无额外、缺失或修改文件。原目录及基线中不参与 GLFW 构建的旧 SDL 子目录已按用户指示撤掉，没有保留旧 SDL 目录。详见 [依赖身份](evidence/sdl-provenance.json)。
- GLFW 输入以脏工作区实际文件保存，不用 HEAD 代替。初始保守 manifest 为 4121 项，旧 SDL 副本撤掉的范围及两份安装文档补充分别记于 out 的 exclusions/supplement；这些变化不改动已构建的游戏源码。源码、库、exe 与 ZIP 身份见 [基线身份](evidence/glfw-baseline-identity.json)。M2/T1 原包及冻结数据未覆盖。
- 实际工具链：Windows x64、MSVC 19.38.33145 / toolset 14.38.33130、CLion CMake 4.3.1、Ninja 1.13.2。GLFW API 探针用相同 MSVC 的 Visual Studio 生成器，只导入实际 Release 库，不重新编译生产实现。
- 第二轮补用官方 portable CMake 3.22.6，ZIP 与官方 SHA-256 清单一致，不安装到系统目录；GL/Vulkan × Debug/Release 全新配置、构建、纯 CTest 与安装通过。独立探针不依赖冻结 GLFW 库；持续对照工具是显式启用的 Release 目标。见 [工具身份](evidence/input-v2/cmake322-provenance.json)、[四配置审计](evidence/input-v2/cmake322-audit.json)。
- 实际 GL：RTX 5070 Ti，NVIDIA 616.64，GL 4.6 Core、4x MSAA。本次显示器 SDL 报告 display scale 1.25、pixel density 1；这不是 100%/150%/200% DPI 矩阵通过。
- 既有准备时本机未发现完整 Vulkan SDK；旧探针显式使用 SDL 原样附带的 Khronos Vulkan 1.3.282 头，入口来自 SDL 动态 loader，不链接 `vulkan-1.lib`。现在 SDK 位于 `C:/VulkanSDK/1.4.363.0`，核心头/库/Info 文件与机器级环境路径已核对；新 Debug/Release 配置及各 5/5 真实探针通过。两配置实际编译依赖均包含该 SDK 的 14 个头，`VK_HEADER_VERSION=363`，见 [SDK 头源与运行身份](../../../out/m3-t2/archive/preparation/platform/vulkan-sdk-verification-001/verification.json)。该快照记录批准前安装器/Info/Cube 未独立取得的状态；用户随后确认 Q04 关闭，以当前人工批准补充状态，不覆盖快照或旧头源结果。

## 自动结果

| 准备实验 | 实际结果 | 证据 |
| --- | --- | --- |
| GLFW 新 Debug 基线 | 61/61 启用测试通过；`world.allocations` 保持既有禁用状态，不借此宣布分配失败测试通过 | [测试日志](evidence/glfw-debug-test.log) |
| GLFW 新 Release 基线 | 62/62 测试通过，完整安装及基线 ZIP 已保存，ZIP 20 个文件与安装 manifest 逐字节核对一致 | [测试日志](evidence/glfw-release-test.log)、[安装日志](evidence/glfw-release-install.log)、[ZIP 核对](evidence/glfw-zip-verification.json) |
| GLFW 游戏运行 | Debug/Release 各 120 帧，seed 424242、regression；Release 另含 single-block 编辑探针；零 GL diagnostic、正常退出 | [Debug](evidence/glfw-debug-smoke.json)、[Release](evidence/glfw-release-smoke.json) |
| GLFW 真实 platform API | 26 项检查通过：实际 GL4.6/Core/4x MSAA、短按消费、held/Reset、运动符号/浮点滚轮、真实最小化/恢复、Wait 保留 WM_CLOSE、1920×1080/NonRudeHWND/非置顶。导入同源真实 platform/foundation/GLFW/GLAD 库；键鼠消息为合成消息，不是硬件手感验证 | [日志](evidence/glfw-api-probe.stdout.log)、[身份](evidence/glfw-api-probe.json) |
| GLFW 资源故障 | shader、纹理、空 block 配置三个副本均启动失败、exit 3、未 ready、正常清理；原资源未改 | [故障结果](evidence/glfw-faults.json) |
| SDL 构建及候选输入 | 独立 GL Debug、GL Release、Vulkan Debug 均构建成功；每配置 1 个 CTest，合成事件候选 81 项检查通过 | [Debug](evidence/sdl-gl-debug-test.log)、[Release](evidence/sdl-gl-release-test.log)、[Vulkan](evidence/sdl-vulkan-debug-test.log)、[81 项范围](evidence/sdl-input-candidate.log) |
| SDL GL/Native/等待与失败 | Debug 5/5、安装 Release 5/5 探针通过：GL/Native、Benchmark 窗口与 Wait/WM_CLOSE、非法 video driver 具名失败、未编译 Vulkan 具名拒绝 | [Debug](evidence/sdl-gl-debug-probes.json)、[安装 Release](evidence/sdl-release-installed-probes.json) |
| SDL Vulkan | Vulkan Debug 5/5 探针通过；真实取得 `VK_KHR_surface`/`VK_KHR_win32_surface`、创建 instance/surface，先销毁 surface/instance 再退出窗口/SDL。没有 device/swapchain/游戏绘制 | [Vulkan 结果](evidence/sdl-vulkan-debug-probes.json) |
| SDL 静态与安装 | `SDL_STATIC=ON`、`SDL_SHARED=OFF`、`SDL_LIBC=ON`，普通 main；安装只含探针、SDL 许可证及既有 MSVC runtime。未增加 SDL3.dll；GL-only 不含 Vulkan loader 导入，安装版从中文/空格工作目录运行通过 | [安装](evidence/sdl-gl-release-install.log)、[GL 导入](evidence/sdl-gl-release-dependents.log)、[Vulkan 导入](evidence/sdl-vulkan-debug-dependents.log) |
| SDL 按住态候选增量 | 185 项合成检查通过：active Reset 保留 held，失焦/最小化清 held，恢复采新物理状态，重复不制造锁存，短按及 Benchmark Escape 隔离保留；不是硬件输入结果 | [185 项日志](evidence/input-v2/candidate-185.log) |
| 持续输入对照 | GLFW/SDL 共用冻结游戏相机及 0.05 运动灵敏度；三种鼠标模式和 SDL 未缩放候选共 7/7、每项 12 帧真实 GL 渲染通过；非空 readback 已检查。记录快照、有序运动、相机姿态、诊断操作及 exe SHA | [渲染结果](evidence/input-v2/comparison-rendering.json)、[工具说明](../../../test/experimental/input-comparison/README.md) |
| SDL 真窗口输入与生命周期 | 118 项执行检查通过，但整项 **partial / exit 2**：本机布局 11 键合成消息、held/Reset、真实最小化恢复、Wait 首 WM_CLOSE、video 多 owner/重复清理成立；合成鼠标按钮未形成可靠物理 held，未判按钮 held/Reset 通过 | [范围日志](evidence/input-v2/runtime-input.stdout.log)、[部分结果](evidence/input-v2/runtime-input.json) |
| CMake 3.22 无下载路径 | 四全新配置通过，真实 trace、源码内容和生成规则均未发现下载步骤，前后输入 hash 不变。进程代理拒绝/Git 仅 file 是部分约束，用户网络未断开，不称完整断网实测 | [审计](evidence/input-v2/cmake322-audit.json)、[实际纯测试及导入](evidence/input-v2/cmake322-imports.json) |
| CMake 3.22 新包运行 | 新 Release 探针补跑 GL/Native/Benchmark 与两项失败路径，5/5 通过；真实 GL4.6/Core/4x MSAA、1920×1080、VSync 1/0 与清理仍成立 | [运行结果](evidence/input-v2/cmake322-gl-runtime.json) |
| SDK 头源 Debug/Release | 两配置纯 CTest 各 1/1（185 项合成检查），真实探针各 5/5；Windows PowerShell 5.1 记录退出码 `0/0/0/1/0`，真实 Vulkan instance/surface 创建及逆序清理通过；不是生产或 GPU 后端验收 | [Debug](../../../out/m3-t2/archive/preparation/platform/vulkan-sdk-debug-runtime-003/summary.json)、[Release](../../../out/m3-t2/archive/preparation/platform/vulkan-sdk-release-runtime-003/summary.json)、[头源身份](../../../out/m3-t2/archive/preparation/platform/vulkan-sdk-verification-001/verification.json) |

SDL GL 探针实际 VSync 请求/查询为 1 与 0；真实清屏读取到非零 RGB，8 帧交换成功，context/window/SDL 按顺序清理。SDL Benchmark 探针在本机实际 1920×1080，普通无边框、非置顶/非独占/不可 resize，NonRudeHWND 设置并清除成功；任务栏视觉行为仍需人工核对。

首轮源码及探针身份见 [准备输入](evidence/preparation-inputs.json)、[SDL exe 身份](evidence/sdl-probe-identities.json)，作为历史证据不覆盖。第二轮当前输入与 exe 身份见 [增量身份](evidence/input-v2/input-identities.json)，官方 SDL 2183 文件再次全部一致，见 [vendor 复核](evidence/input-v2/vendor-recheck.json)。大体量原始日志、安装、源码输入、失败尝试与 ZIP 留在 `out/m3-t2/`，docs 不存二进制包。

## 准备进度

| 步骤 | 当前状态 | 未完成部分 |
| --- | --- | --- |
| P01 | 自动基线已建立，人工手感已由用户确认通过，未整项关闭 | 旧现象边界及完整人工窗口记录 |
| P02 | 依赖与构建实验完成；固定源码、准备探针入口及静态部署已验证，S1 补齐实际生产接入后的 CPU/runner 矩阵和 SDL 正负边界 | 按 Q05 不声称物理断网实测；生产 SDL 初始化调用及窗口行为仍随 S2 验证 |
| P03 | 既有三模式能力有结果，SDK 头源双配置真实探针已通过，Q04 已获用户确认关闭 | Q03/方案1 将窗口/桥接失败矩阵随 S2、硬件/DPI/任务栏随 S3 补，T2 验收前关闭 |
| P04 | 已补重同步候选、185 项检查、真实 SDL 键盘消息；人工手感已通过 | 未明确覆盖的真实鼠标 held/Reset、切出释放、不同布局及模式切换仍待结果；118 项部分结果不关闭整项 |
| P05 | Q01 末参 `WindowMode` 和 Q02 具名异常已选，三方案与验证安排已同步 | 完整失败矩阵和 P04 剩余硬件用例按 Q03 随 S2/S3 完成 |
| P06 | 八项已选，2026-10-07 用户确认 Q04 关闭、P06 通过，Q08 授权生效 | 剩余验证按 Q03 随 S2/S3 补齐；预算数值仍按 Q07 在 SDL 验收对比采样前确认，不把准备批准当作 T2 验收 |

## S1 依赖隔离

2026-10-07 完成 S1。固定 `vendor/sdl3` 原样源码，`SDL_STATIC=ON`、`SDL_SHARED=OFF`、`SDL_LIBC=ON`，窗口/输入/OpenGL 及动态 Vulkan 窗口桥接启用；无关子系统、SDL 测试、示例和独立安装关闭。platform 私有链接 `SDL3::SDL3-static`、私有定义 `SDL_MAIN_HANDLED`，保留当前 GLFW 实现和链接作为可构建中间状态，不建立双窗口运行方案。S2 在首次 SDL 初始化前加入 `SDL_SetMainReady` 并替换实际窗口/桥接。

汇总与实际源码、exe、日志摘要见 [S1 验证清单](evidence/s1-validation.json)；大日志和包保留在 `out/m3-t2/s1/`，没有覆盖 SDK 探针、GLFW 基线或 M2 数据。

| 验证 | 实际结果 | 证据 |
| --- | --- | --- |
| 主工程 Debug / Release | 全新配置、构建、测试、安装通过；61/61 启用测试、62/62 测试通过；Debug 原有 `world.allocations` 禁用不计通过 | [Debug](../../../out/m3-t2/log/s1/build/main-debug-002-test.log)、[Release](../../../out/m3-t2/log/s1/build/main-release-001-test.log) |
| 构建/安装矩阵 | 两配置各五路径：关闭测试、CPU-only、独立工具、根 runner-only、game-only 全部配置/构建/安装通过；CPU/runner 无 SDL/GLFW/GLAD 目标、缓存或编译产物 | [Debug 矩阵](../../../out/m3-t2/log/s1/build/matrix-debug-001.log)、[Release 矩阵](../../../out/m3-t2/log/s1/build/matrix-release-001.log) |
| 隔离配置 CTest | CPU-only Debug 44/44 启用、Release 45/45；独立工具两配置各 5/5；game-only Debug 56/56 启用、Release 57/57 | 各日志路径及 SHA-256 见验证清单；三处 Debug 分配测试均维持禁用 |
| 工具边界矩阵 | 两配置各四路径：根 runner-only、game-only、独立工具、CPU-only 关闭测试后均构建通过；未生成测试/support 目标 | [Debug](../../../out/m3-t2/log/s1/build/tools-debug-001.log)、[Release](../../../out/m3-t2/log/s1/build/tools-release-001.log) |
| 正负边界与真实头可见性 | 两配置各 3 正向/33 负向边界检查通过；实际 MSVC 编译 platform SDL 头成功，renderer/app/world 各仅因 SDL 头不可见而 C1083 拒绝；生产编译命令无 SDL 宏/头传播 | [Debug 头检查](../../../out/m3-t2/archive/s1/build/header-visibility-debug-002/summary.json)、[Release 头检查](../../../out/m3-t2/archive/s1/build/header-visibility-release-001/summary.json) |
| 静态/CRT/安装 | SDL 与 platform 实际编译分别保持 `/MDd`、`/MD`；六个游戏安装目录均含原样 SDL 许可证、无 SDL3.dll；生产生成图无 Vulkan SDK 路径或 `vulkan-1.lib` | 验证清单及 [Release 导入](../../../out/m3-t2/log/s1/build/main-release-001-dependents.log)；当前无 SDL 调用，导入表不能代替 S2 动态 loader 运行检查 |
| 实际接入无下载路径 | 新根工程 Release 配置 trace 的 55719 条执行命令无网络/依赖拉取命令；Ninja 无网络规则，4758 项输入前后未变、源头覆盖齐全。只证明本次路径，不声称物理断网或网络沙箱 | [审计](../../../out/m3-t2/archive/s1/build/no-download-release-002/summary.json) |
| vendor 原样 | 与已固定官方 ZIP 的 2183/2183 文件一致，修改/缺失/额外均为空 | [逐文件复核](../../../out/m3-t2/archive/s1/evidence/vendor-recheck-001.json) |

S1 初次 Debug 配置在受限环境重现 C1902，同工具链沙箱外新 `002` 通过。新增证据工具首次分别因标准库 target 无 `-I`、CMake trace 的 `:53:EVAL` 合成位置而拒绝；只修正审计解析/覆盖规则，在新目录复测通过，原失败目录未覆盖。这些修正没有改变 SDL、窗口行为或产品通过线。构建中的既有基础/ECS/应用告警未作为本轮修复成果。

上述为 S1 当时的中间状态，未覆盖旧证据。S2 当前进展见下节；无需重复 Q04、P06 或已通过的工具手感，输入/事件失败和剩余硬件/DPI/任务栏随 S3 留证。

## S2 窗口与桥接

2026-10-07 完成 S2 窗口/桥接实现与阶段验证。Window 保留 PImpl，Create 末参为默认 OpenGL 的 `WindowMode`；普通 main 不包含 SDL，platform 私有调用 `SDL_SetMainReady` 并仅持有一次 video 责任。GL context、窗口分别拥有；活窗口存在时 Free 拒绝，重复 Init/Destroy/Free 安全，外部 SDL video 所有者不被释放。GL 六项入口保持签名，必要失败具名抛出拥有诊断文本的异常，合法不可选扩展/过程地址仍返回 false/null。

NativeBridge 仅借出 HWND；VulkanBridge 经 SDL loader 返回入口、复制必要扩展名、真实创建/销毁 surface，instance/surface 归调用方，必须先于窗口与 loader 销毁。生产只用原样 SDL 内部官方 Khronos 头，外部 SDK 只在显式探针中出现，不链接 `vulkan-1.lib`。platform 已撤除 GLFW 链接；旧 `EXCLUDE_FROM_ALL` 配置和安装许可留 S5 收尾，不删休眠材料。

app 清理新增逐段保护：运行主故障不被清理异常覆盖，renderer/GPU、context、窗口和 video 余下释放仍执行；Window 析构 noexcept，正常 checked 清理失败后报告首错误。输入候选私有移植使生产游戏可运行，不跨 test 引用；仍不能用准备期 185 项合成检查代替 S3 生产输入验收。

汇总及原始文件摘要见 [S2 验证清单](evidence/s2-validation.json)，复现入口见 [生产探针 README](../../../test/experimental/platform-sdl3/README.md)。证据均在新 `out/m3-t2/s2/` 路径，没有覆盖准备探针、GLFW 基线或 S1 数据。

S2 开发候选包为 [candidate-release-001 (retired)](../../../out/maintenance/archive-20261008/deleted-packages.json)，不作为最终 T2 发布包。可执行程序与已验证 Release 的 SHA 相同，资产逐项核对一致，附带当前依赖说明及 [包身份 (retired)](../../../out/maintenance/archive-20261008/deleted-packages.json)；人工验收清单随后按 S3/S5 分阶段提供，不要求重做 Q04/P06 或已通过的同一工具手感。

| 项目 | 实际结果与边界 | 证据 |
| --- | --- | --- |
| 主构建/安装/纯 CTest | CMake 3.22.6、MSVC 14.38：Debug 62/62 启用测试、Release 63/63；Debug 原有 `world.allocations` 仍禁用。新增纯清理顺序测试覆盖故障后继续释放与首错误保留 | `main-debug-001` / `main-release-002` 构建、测试、安装日志 |
| 三模式真实生产库 | Debug/Release 各 17/17：真实 GL、Native、Vulkan、Benchmark、生命周期及初始化/loader 失败 8 项，明确标记受限 fixture 9 项；不是把 independent preparation Window 重新编译成生产替身 | [Debug](../../../out/m3-t2/archive/s2/runtime/runtime-probe-debug-003/summary.json)、[Release](../../../out/m3-t2/archive/s2/runtime/runtime-probe-release-001/summary.json) |
| GL/Native/Vulkan 事实 | RTX 5070 Ti / NVIDIA 616.64：实际 GL 4.6 Core、4x MSAA，Debug driver flag=2，VSync 1/0；8 次真实交换与非空 GPU 像素读回。有效 HWND、等待 WM_CLOSE；SDK header=363，真实 instance/surface 和逆序释放 | 双配置 probe JSON；不是 D3D12/Vulkan 游戏后端 |
| 必要失败与清理 | 初始化/GL/Vulkan 库缺失真实失败；window/context/current/VSync/present/扩展查询/必需 GLAD 地址/surface/context 释放后次级失败为受限注入，实际回收计数匹配且 video 退出 | 同一实际 SDL 动态 API table，仅 test exe/fixture 生效；不声称真实驱动销毁失败 |
| 安装包真实游戏 | Debug/Release 各 120 帧、exit 0、无 GL diagnostic、正常 shutdown；每配置 shader/纹理/配置损坏副本各 3/3、exit 3，原资产未改；Release regression single-block 脚本编辑 8 帧正常退出 | `game-*-001`、`resource-faults-*-001`、`game-release-edits-001` |
| 截图/计时/协议诊断 | Release static 预热 0 秒/采样 2 秒、1920×1080、VSync 0、seed 424242、441 区块；237 帧、236 GPU samples，1 个不可用 sample 如实保留；schema/protocol/workload=2，纹理与选择框截图已查看 | [截图](../../../out/m3-t2/archive/s2/benchmark/benchmark-diagnostic-release-001/run/final-frame.png)、`protocol-check.json`；不是 Q06 短采样或 Q07 预算判定 |
| 构建隔离矩阵 | 双配置五模式配置/构建/安装完成；CPU-only 最近复测 Debug 45/45、Release 46/46，独立 runner 各 5/5，game-only 57/57、58/58。首次 CPU-only 文件发布失败及后续 20 次中 1 次复现仍留证，稳定性未关闭 | `matrix-*-001` 与独立 retest 日志，见下方文件发布风险；不宣称整个矩阵稳定全绿 |
| 依赖/头/许可证 | 三私有桥接头的正向/越权边界 45 例通过；最终游戏链接无 GLFW archive、无 SDK/import lib，SDL main/include 不泄漏；probe 各 14 个 SDK 头，生产为原样内部头；2183 个 vendor 文件及安装许可原样 | [生产审计](../../../out/m3-t2/archive/s2/evidence/production-audit-001.json) |

验证期间的修正均保留失败现场：Release `main-release-001` 在受限环境的 CMake 编译器检查触发 C1902，同工具链新 `002` 通过。Debug probe `001` 因 PowerShell 7 将 null 环境设置变成空 DLL 路径，改为真正删除变量；`002` 因把 SDL 为任务栏/最小化保留的 `WS_CAPTION` 误判为实际边框而拒绝，改为 SDL flags 与真实客户区/窗口几何核实。最终 `003` / Release `001` 通过，未改生产窗口策略或放宽 GL 要求。生产实现审查同时补齐实际 driver 属性核实、NonRude 次级清理诊断和输入错误立即停止本帧。

**仍待定位的文件发布风险：** 两配置 CPU-only 的 `performance.export` 首轮遇到 `MoveFileExW` 替换 `status.yaml` 的拒绝访问。后续完整 CTest 通过，但独立新目录复查 Debug 10/10、Release 9/10，仍真实复现一次。该路径不链接 SDL，foundation/telemetry/单测源码本轮无差异；流已显式关闭，具体原因未知，不能推断为 Defender 或沙箱，也不能因一次重跑通过就关闭。原始现场和 20 轮结果见 [复查证据](../../../out/m3-t2/archive/s2/runtime/performance-publish-recheck-001/summary.json)。最终 A01/A11/交付前须定位并复测，不在 S2 顺带重写结果协议或公共文件设施。

S2 窗口/桥接门槛已取得证据；S3–S5、上述发布风险稳定性及 T2 最终节点仍未完成。完整硬件输入/布局、事件分配失败、DPI/跨屏/任务栏可用性、runner 全游戏取消、双机玩法和性能预算不由本节替代。

## S3 输入与窗口行为

2026-10-08 实施 S3。公开 API、0.05 相机灵敏度和已选系统缩放策略不变，不增加生产诊断快捷键，不修改 SDL vendor。三个窄修正：Poll/Wait 排空后按真实 SDL flags 收敛焦点/最小化状态，避免遗漏恢复消息后永久拒绝输入；本窗口 Benchmark Escape 未消费锁存跨同轮失焦保留，仍不采后台全局按键；普通游戏不可绘制（最小化/零 drawable）时，即使有 smoke 帧数限制也暂停/等待，不提交绘制。原普通游戏恢复首帧 dt=0 分支保持，已源码审查，但没有伪造其逐帧动态日志。

Window/Impl 对象分配失败增加具名嵌套原因，生产缓冲预留及 Poll/Wait 扩容故障立即停止发布、后续 Capture 重抛。新增独立测试只导入真实生产 platform/foundation/SDL/GLAD 档案；纯 reducer 测试包含真正的私有 `input_state.h`，不使用准备候选替身。硬件工具使用同一生产库和默认 Raw Input，无合成/全局注入，F5/F6/F7 仅属于测试工具。

最终机器可核对来源见 [S3 证据](evidence/s3-validation.json)。大日志、首次部分/夹具失败与后续结果都留在新 `out/m3-t2/s3/`；没有覆盖准备期、GLFW、S1/S2。S3 开发候选 [candidate-release-002](../../../out/m3-t2/archive/s3/evidence/install/candidate-release-002/SymoCraft.exe) 与已验证 Release 游戏 exe/资产一致，附 [身份](../../../out/m3-t2/archive/s3/evidence/install/candidate-release-002/package-identity.json) 及可复制到 Y9000P 的人工入口。它不是最终 T2 发布包；操作和结果栏见 [S3 人工清单](s3-manual-verification.md)，未执行格均保留 TBD。

| 项目 | 实际结果与限制 | 证据 |
| --- | --- | --- |
| 主生产构建/安装 | CMake 3.22.6 / MSVC 14.38：Debug 62/62 启用、Release 63/63；Debug 原 `world.allocations` 禁用，未算通过 | `main-*-001-final-test.log`，实际链接/导入与源码身份见 S3 JSON |
| 真私有 reducer | 双配置各 263 检查通过，涵盖 11 scancode、短按、held/reset、顺序/符号、遗漏与乱序窗口事实、两种 Escape/失焦次序 | `probe-*-003-pure-checks.log`；是合成值回归，不是物理硬件通过 |
| 真实库输入运行 | 最终双配置各 21/21（7 用例 × 3 次）通过：队列、Win32 当前布局键盘、真实焦点恢复、模式/像素 resize、Benchmark 隔离和 Wait 关闭。之前不获前台的轮次仍 exit 2/partial 保留，不因最终通过删除 | `runtime-input-*-003/summary.json`；是真实生产库 + 合成队列/自身 Win32 消息，不是物理硬件 |
| 分配失败与清理 | 两配置各 4/4，Window 对象、reserve、Poll/Wait growth 的精确单次 test-only C++ 分配失败；真实 GL/window 释放、外部 video owner 保留、Capture 重抛，非空已发布快照保持已观察 | [Debug](../../../out/m3-t2/archive/s3/runtime/runtime-allocation-debug-002/summary.json)、[Release](../../../out/m3-t2/archive/s3/runtime/runtime-allocation-release-002/summary.json)；实际 MSVC growth=96（2304 bytes），不是假定 128 或 OS 内存耗尽 |
| S2 三模式回归 | 双配置各 17/17，真实 GL/Native/Vulkan、生命周期及已标注失败 fixture 仍通过 | `runtime-probe-*-001/summary.json`，绑定本轮生产库；不声称新图形后端 |
| 真实安装游戏 | Debug/Release 各 120 帧、exit 0、无 GL diagnostic、正常 shutdown | `game-*-001/result.json`；不是双机 15 分钟玩法 |
| 完整游戏窗口协议 | 最新 Release 6/7 通过：真实失焦关闭、最小化关闭、allow-unfocused 有效、strict 失焦 invalid、Benchmark 最小化 invalid、WM_CLOSE 取消落盘。结构化解析实际 schema/protocol/workload=2、1920×1080、4x MSAA、VSync 0 | [应用结果](../../../out/m3-t2/archive/s3/platform/application-release-004/summary.json)；3 秒诊断不计 Q06/Q07，未证明 app 已进入某个具体 Wait/dt 分支 |
| 尚未执行的尺寸故障 | 固定 Benchmark 窗口拒绝强制尺寸变更，真实客户区仍 1920×1080；测试分支标 partial/unavailable，退出由 fixture 清理而非成功 resize 验收 | `application-release-004/benchmark-resized/`；保留 `001–003`，不能算生产尺寸回归或通过 |
| 人工工具入口 | 真实 GL 创建/呈现路径、JSONL 写入及自身 WM_CLOSE 清理已单独 smoke；没有键鼠操作，human_acceptance_passed=false | `manual-tool-startup-002/result.json`；用户仍须 H01–H15 |

首次输入运行每配置仅 6/21 已执行，15 轮因前台未获准为 partial；夹具随后显式显示自身 HWND，并记录显示前后和三重焦点事实，重测的成功轮可证明其执行范围，但不能反推首次一定由不可见窗口导致。初次应用夹具读取活日志共享方式不正确；改为显式 FileShare.ReadWrite，按 PID/标题定位自身窗口。跨进程 Win32 查询在 125% DPI 下曾报告虚拟坐标 1536×864，局部测试线程 DPI scope 后实际为 1920×1080；这只修正观察，不改游戏 DPI 策略。固定窗口仍拒绝真实尺寸改变，故明确保持缺口，不修改生产策略迎合测试。

**S3 尚未整项关闭：** 开发桌面 H01–H15 现已由用户填写通过（H14 保留原文 `passs`），不要求重新反馈 H04/H07。实际同包硬件会话已核对 active Reset/Capture、六次模式切换、失焦和正常退出；该会话没有完整记录右键/滚轮、其它布局/DPI及 Benchmark 全矩阵，具体条件/数值材料仍须绑定，不能从工具日志补造全覆盖。Y9000P 按用户要求暂缓，Benchmark 真尺寸变化自动分支仍 partial/unavailable。事件等待的强制空闲 timeout 证明未取得（OS 事件可提前唤醒），只报告有界等待。既有文件发布稳定性仍按 S2 保留，未因 CTest 通过关闭；S4 交接见下节，不自动关闭 S3/S5/T2。

## S4 T3 契约交接

### S3 可运行验证包冻结

冻结入口为 [s3-release-001](../../../out/m3-t2/s3/frozen/s3-release-001/FREEZE-README.md)，便携归档为 [ZIP](../../../out/m3-t2/s3/frozen/s3-release-001.zip)，[SHA-256](../../../out/m3-t2/s3/frozen/s3-release-001.zip.sha256) 与 [冻结身份](evidence/s3-freeze.json) 均已生成。原 `candidate-release-002` 25 文件（24 载荷加原身份）原样复制到 `package/`，不修改工具、资产、许可或包身份；逐项校验与 ZIP 解压后再校验通过，总共 38 个冻结文件。后续开发/新观察写新目录，不覆盖该包或已有结果。

外层另留冻结当时的用户填写清单、S3 自动汇总、同包 `manual-desktop-20261008-133846` 原始记录与三份包标识生产输入。原包内空清单和外层填写快照分别保存，不静默替换。该会话 exit 0、无输入注入、Escape 正常退出、窗口和 SDL 清理成功；H04 的 W/左键 Reset 清空后重新采纳 held、释放后仍空，以及 H07 六次模式切换可由 JSONL 核对。冻结文件摘要与历史实测相同，本次没有把冻结新路径另行 GUI 运行记为已实测。

这是验证包冻结，不是重新构建、最终 T2 发布或完整可重建源码归档；构建缓存、SDK/vendor、旧失败与历史证据不打进包，继续在工作区保留。源码摘要未漂移，历史完整生产来源仍由 S3 证据索引承担。Y9000P 以后可复制整个冻结包使用 `package/run-hardware-check.ps1`，结果写包外新目录；现阶段不要求执行或替第二台机器填写成绩。

### 平台契约与本轮验证

[S4 交接文档](s4-platform-handoff.md) 核对真实 `SymoCraft::Window`、创建模式、主线程/独占/借用期、实际 drawable、当前 GL 呈现与未来 renderer 唯一呈现，以及 Native HWND、Vulkan 同一 loader/自有扩展/surface/清理责任。T3 候选参数改为借用真实 Window，不重命名生产类型；D3D12 使用 Native，Vulkan 使用其专用模式，不能照搬 GLFW no-client-API。同步 app/renderer 文档中过时的当前 GLFW 清理表述，保留历史段落。

本轮只增加 [独立声明消费者](../../../test/experimental/platform-handoff/README.md) 和契约同步，没有修改生产平台/GPU/玩法或 SDL vendor。当前独立 D3D12 前置实验已有自己的台式机局部成绩，仍按其原授权和门槛继续；本轮不新建生产现代后端、冻结 Renderer v1 或进入 R2。

| 检查 | 实际结果 | 证明边界 |
| --- | --- | --- |
| Debug / Release 声明消费者 | 各 4/4：公开 Window、GL、Native、显式 SDK Vulkan；CMake 3.22.6 / MSVC 14.38 | 编译实际头、签名/不可复制移动/输入借用类型与 CPU 默认值，不创建窗口或验证 GPU |
| 无 Vulkan SDK 配置 | 独立 Release 3/3，未传 SDK 路径；公开/GL/Native 编译命令不含 SDL/GLAD/Vulkan 头目录 | Native 仍使用 Windows SDK；不是移除物理系统 loader 后的部署测试 |
| 真实模块边界脚本 | 45/45 正向授权与负向拒绝达到预期，复用生产检查器 | 精确私有桥白名单和 SDL 私有依赖，非未来 T3 子 target 图已实现 |
| 包与源码身份 | 原载荷、三生产输入、原观察包身份一致；38 文件 ZIP roundtrip 一致 | 冻结原样运行版本，不补造双机或预算通过 |

原始日志与摘要见 [S4 验证身份](evidence/s4-validation.json)，新材料留 `out/m3-t2/s4/`。Debug 首轮受限环境的最小编译检查再次 C1902，现场 `contract-debug-sdk-001` 保留；同工具链沙箱外新 `002` 通过，不修改 PDB 选项或以 Release 代替 Debug。

S4 交接完成只关闭本阶段文档/声明核对，不替代 T2 最终验收。真实 bridge 的既有双配置 17/17 仍对应原生产库；没有本轮新跑 OS Vulkan DLL 卸载、device/queue/swapchain 或 GPU 在途生命周期。后端的 acquire/submit/present/rebuild 和致命/可恢复错误分类仍归 T3。

## 风险与复测

1. 现有 GLFW 未启用 raw mouse，SDL 相对模式默认不使用系统加速度。当前工具的系统缩放候选已获用户人工手感通过，灵敏度与相机算法未改；不能将其推广为默认未缩放输入或真实 SDL 生产适配已验收。
2. GLFW API 实测表明 Reset 不清掉底层已 held 的键。第二轮候选已补 drain/Capture 之间及 active Reset 之后的物理重同步，185 项合成与真实 SDL 键盘消息验证该规则；真实鼠标按钮、切出释放与恢复仍须硬件对照，不据此宣布生产适配完成。
3. 初次故障脚本仍断言旧文本 `Failed to decode texture`，实际 assets 已使用 `Cannot decode image:`。本轮仅修正脚本断言并重跑三个副本通过，未改 assets 或退出逻辑；初次失败日志保留。
4. MSVC Debug 在受限沙箱报 C1902，沙箱外相同工具链通过。CMake 3.22 曾因缓存中的 Windows 反斜杠路径失败，已在 build helper 规范为正斜杠并用全新 3.22 Release 持续工具构建复测通过。完整断网仍未实际断开用户网络；无下载路径审计与断网实测分开报告。
5. GLFW API 初次用合成焦点消息断言系统焦点失败，改为自身真实窗口最小化/恢复后通过。这是 fixture 边界纠正，不是生产焦点缺陷结论。
6. SDL 真窗口合成按钮初次在 89 项后断言失败。`PostMessage` 不改变 Windows 物理按钮，SDL 会以 capture/异步物理状态协调释放；修订 fixture 后保留事件观测，明确 partial，不伪造 held 或关闭同步。初次失败和修订后的 118 项部分日志均保留。
7. 评审修正了持续工具的 F1/F2/F3 恢复假边沿和清理失败仍报告成功问题；失焦后诊断键等待释放，SDL 正常退出先释放 GPU 再 checked context/window/video 清理，verifier 拒绝 cleanup 诊断。原成功 Vulkan 探针不因此证明缺失 destroy 入口等全部异常路径成立。
8. SDK 用户复测先因相对结果路径落到 System32 而无法建目录；改用绝对路径后，`002` 五项均因退出码为 `null` 被拒绝，不能仅凭正常 JSON 报告改成通过。验证脚本现按仓库根解析相对结果路径并限制在 out，启动后保留进程 handle，再等待/读取真实退出码；PowerShell 7 和 Windows PowerShell 5.1 的 exit 0/1/超时检查及越界/旧目录拒绝检查通过。新 `003` 双配置 5/5，原 [002 失败记录](../../../out/m3-t2/archive/preparation/platform/vulkan-sdk-debug-runtime-002/summary.json) 未覆盖。[PowerShell 问题与处理依据](https://github.com/PowerShell/PowerShell/issues/5421)

## S5 回归与交付候选

2026-10-08 按用户要求继续 S5，Y9000P 不运行，必要 Release package 单独冻结。当前硬件范围只限定本机 RTX 5070 Ti，既有 S5 结果仍按实际候选版本核对，不推定中低端整机通过。范围、操作入口和剩余门槛见 [S5 交付状态](s5-delivery-status.md)，实际源/产物/日志身份见 [S5 验证](evidence/s5-validation.json)。

活动 GLFW 配置、platform 链接授权及新安装许可已撤除；历史 vendor/lib、休眠材料和旧包不删。两配置各五模式配置/构建/安装与 61 项边界检查通过，Release 主 CTest 64/64；Debug 两次主 CTest 均为 62/63 启用通过、foundation.files 失败，原 world.allocations 禁用。矩阵首次 Debug CPU/runner/game 为 46/46、5/5、58/58，Release 为 47/47、5/5、59/59；Debug CPU 后续诊断复测为 45/46，保留前后结果，不挑绿报告。

两配置真实库三模式/失败各 17/17、输入各 21/21、分配故障各 4/4 通过；Debug 安装游戏 120 帧正常退出。Release 交付新夹具最终 7 通过、1 CRT unavailable，runner 在 warmup 经自身 WM_CLOSE 正常取消，57 帧，exit 0/4、无强杀、落盘/清理/后续轮未启动成立；游戏、runner 与结果路径实际包含中文/空格，cwd 为 System32。三场景两秒协议诊断及非空截图通过，不算 Q06/Q07 或 15 分钟人工。

文件发布窄修复只解决已确定的兼容读者 API 缺口，新的 Win32 1175 仍复现；实际 Debug files/export 各 10 次为 9/10、9/10，Release 8/10、10/10。因此 S5/T2 不关闭，冻结为已知风险候选，不是最终发布。冻结 ZIP 与逐文件身份、解压核验见 [S5 冻结](evidence/s5-freeze.json)；原 S3/GLFW 包摘要复核不变。

2026-10-08 后续撤销：用户明确目前不做核显验证，本次只删除 S5 核显内容。专用包、守卫、启动器、构建/安装副本已删除；通用 S5、S3、GLFW 冻结包保持原 SHA。五次真实失败和设置恢复仅留最小历史记录，不保留执行入口，不改判 AMD 能力，见 [撤销记录](evidence/s5-retired-verification.json) 与 [S5 清理与 T3 准备](s5-delivery-status.md#清理回归与-t3-准备)。本次不改 T3 代码或证据，也不干预另一聊天正在同步的 spec。

## 下一步

2026-10-08 当前优先级：按用户要求暂停 [Fix1 修复与追踪](../../spec/M3-T2-Fix1-spec.md#当前暂停)，先执行 Q06 同条件 GLFW 预采样，以实际 P95/P99、首次可操作时间及 working set/private bytes 提出预算，交用户确认后再做 SDL 验收对比。失败轮不当成绩，保留完整原始证据；不把短采样或暂缓修复写成 T2 批准。当前源的诊断改动与旧冻结包是不同身份，不混用。

旧身份 GLFW 探索预采样 `002` 曾完成 10+30 秒、三场景各三次，9/9 协议有效；**2026-10-09 用户明确将 002 标记为无用数据，现已废弃，不参与任何预算制定或验收对比**。原始数据和采集事实保留，衍生的 MB01 三个旧内存预算候选也停用，不自动改定新值。详细历史与处置见 [Q07 预采样与内存候选](platform-contract-candidate.md#q07-预采样结果与内存预算候选)。

2026-10-09 按用户要求再做一组，`003` 同程序/参数 9/9 有效，三场景 P95 三轮中位为 7.645 / 7.501 / 7.453 ms，P99 为 8.739 / 8.756 / 8.722 ms。本组正式采样全部有焦点，作为当前唯一活动预采样依据；与已废弃 002 的差异不归因优化。详见 [重复采样与条件差异](platform-contract-candidate.md#2026-10-09-p95p99-重复采样)。同日用户已确认 [FB01 方案2](platform-contract-candidate.md#fb01-p95p99-预算候选)：**P95 ≤ 10 ms、P99 ≤ 12 ms**，正式采样全程有焦点的完整轮次适用，各场景三轮逐轮判定，保留失焦轮和原超限；随后明确选定 [MB01 方案2](platform-contract-candidate.md#mb01-内存预算确认)：**704 / 1408 MiB** 并正式批准 T2 节点。首次可操作预算、正式同源 Q06/A11 证据及发布稳定性作为遗留事项保留，Fix1 继续暂停，不把节点批准改写为这些验证已通过。

开发桌面的工具手感对照已由用户确认通过，不要求重复同一手感验证。持续工具 `./test/experimental/input-comparison/run.ps1` 仍可用于剩余用例或生产适配后的复测；人工反馈的具体条件/数值绑定按 S3 保留。它不是完整游戏，当前本机 RTX 5070 Ti 的至少 15 分钟玩法仍须实际完成，后续中低端整机门槛另按中央规范执行。

SDK/Q04/P06 和原手感不要求重做。T2 已由用户正式批准，下一步转入 T3-R1 已授权准备及自身冻结核对；发布稳定性、人工材料绑定、正式同源基线和首次可操作时间预算保留为遗留事项，本次不追加补验或恢复 Fix1。Y9000P 按当前中央规范延期，未覆盖条件继续保留；Q02 三方案选择不重新待选。

Q06 已选每轮预热 10 秒、采样 30 秒、每场景 3 次，仅在本机 RTX 5070 Ti 使用同源、同 adapter 的 GLFW/SDL 条件；Q07 允许 GLFW 预采样/提出预算与实施并行，数值阈值在 SDL 验收对比采样前提交用户确认。FB01 和 MB01 方案2已分别确认 10 / 12 ms 与 704 / 1408 MiB；首次可操作时间通过线仍未定。S2/S5 的两秒单次运行仅为画面/协议诊断，正式同源 Q06 对比采样未完成，正式长时九轮仍延期；这些事实不因本次用户节点批准自动变为实测通过。

当前人工主表保留用户勾选；完整玩法的条件/时长/身份材料和同 adapter、同源 Q06 证据仍有缺口，首次可操作时间预算未定。Y9000P/GTX 1650 后续整机门槛未完成。T2 通过依据是 [2026-10-09 用户明确批准](#t2-正式节点批准)，不是有限帧、独立探针或冻结包自动推导的完成结论。
