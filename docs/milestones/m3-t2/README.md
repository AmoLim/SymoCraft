---
type: 准备记录
status: 准备实验部分完成，P06未批准，生产仍为GLFW
project: Symocraft
module: M3-T2
created: 2026-10-07
tags:
  - area/milestones
---

# M3-T2 SDL3 迁移准备记录

依据 [正式计划](../../spec/M3-T2-SDL3迁移.md) 执行 P01–P05 准备。用户已授权直接继续 T2 准备；树叶剔除与视角突变不属于 T1 spec 或 T2 前置范围，两项继续延期。当前游戏窗口实现、模块链接及安装规则仍使用 GLFW，尚未开始 S1–S5 生产迁移，也未申请 T2 节点通过。

本轮已固定官方 SDL3 源码、保存并重建 GLFW 基线、验证独立 SDL 三模式能力，形成 [平台契约候选](platform-contract-candidate.md)。实验结果不替代真实 SDL 生产平台、鼠标手感、双机玩法或性能验收。

用户随后反馈“一切正常，继续进行开发”。本轮据此继续准备开发，新增持续输入对照、物理按住态重同步和 CMake 3.22 验证；新工具交付后的硬件对照结果及 P06 限制确认单列，不将这次反馈扩写为 SDL 游戏包或双机通过。

## 源码与环境

- SDL3：官方 `release-3.4.18`，完整 commit `829a65d769d935c4852f8159e964312c0957260a`。官方 ZIP SHA-256 为 `9cd42377704398796071b8597cd7e21da254a43bad98c4199739647adc13fa6f`，与 [官方资产](https://github.com/libsdl-org/SDL/releases/expanded_assets/release-3.4.18) 公布值一致。
- 按用户指示使用发行 ZIP 原样替换 `vendor/sdl3`；2183 个文件全部逐字节相同，无额外、缺失或修改文件。原目录及基线中不参与 GLFW 构建的旧 SDL 子目录已按用户指示撤掉，没有保留旧 SDL 目录。详见 [依赖身份](evidence/sdl-provenance.json)。
- GLFW 输入以脏工作区实际文件保存，不用 HEAD 代替。初始保守 manifest 为 4121 项，旧 SDL 副本撤掉的范围及两份安装文档补充分别记于 out 的 exclusions/supplement；这些变化不改动已构建的游戏源码。源码、库、exe 与 ZIP 身份见 [基线身份](evidence/glfw-baseline-identity.json)。M2/T1 原包及冻结数据未覆盖。
- 实际工具链：Windows x64、MSVC 19.38.33145 / toolset 14.38.33130、CLion CMake 4.3.1、Ninja 1.13.2。GLFW API 探针用相同 MSVC 的 Visual Studio 生成器，只导入实际 Release 库，不重新编译生产实现。
- 第二轮补用官方 portable CMake 3.22.6，ZIP 与官方 SHA-256 清单一致，不安装到系统目录；GL/Vulkan × Debug/Release 全新配置、构建、纯 CTest 与安装通过。独立探针不依赖冻结 GLFW 库；持续对照工具是显式启用的 Release 目标。见 [工具身份](evidence/input-v2/cmake322-provenance.json)、[四配置审计](evidence/input-v2/cmake322-audit.json)。
- 实际 GL：RTX 5070 Ti，NVIDIA 616.64，GL 4.6 Core、4x MSAA。本次显示器 SDL 报告 display scale 1.25、pixel density 1；这不是 100%/150%/200% DPI 矩阵通过。
- 本机未发现完整 Vulkan SDK；探针显式使用 SDL 原样附带的 Khronos Vulkan 1.3.282 头，入口来自 SDL 动态 loader，不链接 `vulkan-1.lib`。不能把该头源描述为完整 SDK 安装。

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

SDL GL 探针实际 VSync 请求/查询为 1 与 0；真实清屏读取到非零 RGB，8 帧交换成功，context/window/SDL 按顺序清理。SDL Benchmark 探针在本机实际 1920×1080，普通无边框、非置顶/非独占/不可 resize，NonRudeHWND 设置并清除成功；任务栏视觉行为仍需人工核对。

首轮源码及探针身份见 [准备输入](evidence/preparation-inputs.json)、[SDL exe 身份](evidence/sdl-probe-identities.json)，作为历史证据不覆盖。第二轮当前输入与 exe 身份见 [增量身份](evidence/input-v2/input-identities.json)，官方 SDL 2183 文件再次全部一致，见 [vendor 复核](evidence/input-v2/vendor-recheck.json)。大体量原始日志、安装、源码输入、失败尝试与 ZIP 留在 `out/m3-t2/`，docs 不存二进制包。

## 准备进度

| 步骤 | 当前状态 | 未完成部分 |
| --- | --- | --- |
| P01 | 自动基线已建立，未整项关闭 | 硬件键鼠手感、旧现象边界及完整人工窗口记录 |
| P02 | 固定源码、静态/入口/部署及 CMake 3.22 四配置已验证 | 无下载路径审计不等于物理断网实测，需在 P06 确认范围；生产接入后的 CPU-only/runner 矩阵与负向 SDL 边界测试属于 S1，当前只做依赖隔离审查 |
| P03 | 三模式真实能力及有限失败路径通过 | 全部创建/分配/桥接失败点、重复清理故障矩阵、完整 DPI/任务栏视觉验证；SDK 缺失及头源例外须在 P06 明确 |
| P04 | 已补重同步候选、185 项检查、真实 SDL 键盘消息与持续对照工具 | 真实鼠标按钮、不同布局、模式切换及系统缩放/加速度手感需硬件人工结果；118 项部分结果不关闭整项 |
| P05 | 已评审并收紧候选 | 已明确末参模式、逻辑 SetSize、私有桥接、等待/清理责任；完整失败矩阵和 P04 硬件对照待冻结 |
| P06 | 未进入通过状态 | 短采样参数、预算确认安排、剩余准备限制及实施授权 |

## 风险与复测

1. 现有 GLFW 未启用 raw mouse，SDL 相对模式默认不使用系统加速度。不能直接采用默认 xrel/yrel 后宣称手感保留；系统缩放 hint 是待对照候选。灵敏度与相机算法未改。
2. GLFW API 实测表明 Reset 不清掉底层已 held 的键。第二轮候选已补 drain/Capture 之间及 active Reset 之后的物理重同步，185 项合成与真实 SDL 键盘消息验证该规则；真实鼠标按钮、切出释放与恢复仍须硬件对照，不据此宣布生产适配完成。
3. 初次故障脚本仍断言旧文本 `Failed to decode texture`，实际 assets 已使用 `Cannot decode image:`。本轮仅修正脚本断言并重跑三个副本通过，未改 assets 或退出逻辑；初次失败日志保留。
4. MSVC Debug 在受限沙箱报 C1902，沙箱外相同工具链通过。CMake 3.22 曾因缓存中的 Windows 反斜杠路径失败，已在 build helper 规范为正斜杠并用全新 3.22 Release 持续工具构建复测通过。完整断网仍未实际断开用户网络；无下载路径审计与断网实测分开报告。
5. GLFW API 初次用合成焦点消息断言系统焦点失败，改为自身真实窗口最小化/恢复后通过。这是 fixture 边界纠正，不是生产焦点缺陷结论。
6. SDL 真窗口合成按钮初次在 89 项后断言失败。`PostMessage` 不改变 Windows 物理按钮，SDL 会以 capture/异步物理状态协调释放；修订 fixture 后保留事件观测，明确 partial，不伪造 held 或关闭同步。初次失败和修订后的 118 项部分日志均保留。
7. 评审修正了持续工具的 F1/F2/F3 恢复假边沿和清理失败仍报告成功问题；失焦后诊断键等待释放，SDL 正常退出先释放 GPU 再 checked context/window/video 清理，verifier 拒绝 cleanup 诊断。原成功 Vulkan 探针不因此证明缺失 destroy 入口等全部异常路径成立。

## 下一步

持续工具已可用于开发桌面的 P04 对照。在仓库根目录运行 `./test/experimental/input-comparison/run.ps1`，依次比较 GLFW 与 SDL；F1 切换 Lock/Normal/Hidden，F2 复位相机，F3 清输入后保留真实 held，Escape/关闭结束。重点是鼠标慢/快运动、滚轮、两按钮、按住切出后在外释放、最小化恢复；不同布局按物理位置核对。它不是完整游戏，Y9000P 的 15 分钟游戏仍等生产包。

P06 还需确认 P03 失败矩阵、SDK 头源例外、无下载审计的离线范围及 DPI 项在准备或实施阶段的安排，并审核 [候选契约](platform-contract-candidate.md)。本轮尚未采样，也未填写 P95/P99、首次可操作时间或内存通过线；建议的 10 秒预热、30 秒采样、3 次重复尚待用户确认，双机使用相同参数，正式九轮仍延期。

开发桌面与 Y9000P 的 SDL 生产玩法、每机至少 15 分钟人工游戏、双机短采样和 D12 预算判定均未完成。P06 审核和实施授权成立后，才切换生产库并推进 S1–S5，不把独立探针记作 T2 完成。
