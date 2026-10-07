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

## 源码与环境

- SDL3：官方 `release-3.4.18`，完整 commit `829a65d769d935c4852f8159e964312c0957260a`。官方 ZIP SHA-256 为 `9cd42377704398796071b8597cd7e21da254a43bad98c4199739647adc13fa6f`，与 [官方资产](https://github.com/libsdl-org/SDL/releases/expanded_assets/release-3.4.18) 公布值一致。
- 按用户指示使用发行 ZIP 原样替换 `vendor/sdl3`；2183 个文件全部逐字节相同，无额外、缺失或修改文件。原目录及基线中不参与 GLFW 构建的旧 SDL 子目录已按用户指示撤掉，没有保留旧 SDL 目录。详见 [依赖身份](evidence/sdl-provenance.json)。
- GLFW 输入以脏工作区实际文件保存，不用 HEAD 代替。初始保守 manifest 为 4121 项，旧 SDL 副本撤掉的范围及两份安装文档补充分别记于 out 的 exclusions/supplement；这些变化不改动已构建的游戏源码。源码、库、exe 与 ZIP 身份见 [基线身份](evidence/glfw-baseline-identity.json)。M2/T1 原包及冻结数据未覆盖。
- 实际工具链：Windows x64、MSVC 19.38.33145 / toolset 14.38.33130、CLion CMake 4.3.1、Ninja 1.13.2。GLFW API 探针用相同 MSVC 的 Visual Studio 生成器，只导入实际 Release 库，不重新编译生产实现。
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

SDL GL 探针实际 VSync 请求/查询为 1 与 0；真实清屏读取到非零 RGB，8 帧交换成功，context/window/SDL 按顺序清理。SDL Benchmark 探针在本机实际 1920×1080，普通无边框、非置顶/非独占/不可 resize，NonRudeHWND 设置并清除成功；任务栏视觉行为仍需人工核对。

源码及探针身份见 [准备输入](evidence/preparation-inputs.json)、[SDL exe 身份](evidence/sdl-probe-identities.json)。大体量原始日志、安装、源码输入、失败尝试与 ZIP 留在 `out/m3-t2/`，docs 不存二进制包。

## 准备进度

| 步骤 | 当前状态 | 未完成部分 |
| --- | --- | --- |
| P01 | 自动基线已建立，未整项关闭 | 硬件键鼠手感、旧现象边界及完整人工窗口记录 |
| P02 | 固定源码、静态/入口/部署候选已验证 | 完整断网实测、CMake 3.22 实际版本运行；生产接入后的 CPU-only/runner 矩阵与负向 SDL 边界测试属于 S1，当前只做依赖隔离审查 |
| P03 | 三模式真实能力及有限失败路径通过 | 全部创建/分配/桥接失败点、重复清理故障矩阵、完整 DPI/任务栏视觉验证；SDK 缺失及头源例外须在 P06 明确 |
| P04 | GLFW API 基线及 SDL 合成候选已验证 | SDL 真实键鼠、布局、物理按住态重同步、相对模式切换与实测手感，不由合成测试替代 |
| P05 | 已提交候选 | 中立模式、分离私有桥接、主线程/借用期/失败责任待结合 P04 冻结 |
| P06 | 未进入通过状态 | 短采样参数、预算确认安排、剩余准备限制及实施授权 |

## 风险与复测

1. 现有 GLFW 未启用 raw mouse，SDL 相对模式默认不使用系统加速度。不能直接采用默认 xrel/yrel 后宣称手感保留；系统缩放 hint 是待对照候选。灵敏度与相机算法未改。
2. GLFW API 实测表明 Reset 不清掉底层已 held 的键；SDL 纯事件候选会清 held，生产适配必须明确物理状态重同步时点并保留短按锁存，避免跨 Reset 卡键或失去按住输入。
3. 初次故障脚本仍断言旧文本 `Failed to decode texture`，实际 assets 已使用 `Cannot decode image:`。本轮仅修正脚本断言并重跑三个副本通过，未改 assets 或退出逻辑；初次失败日志保留。
4. MSVC 在受限沙箱的编译器检测报 C1902，沙箱外同工具链构建通过；完整断网验证因此未成立。SDL 与项目均未提高最低 CMake 声明，但实际使用 4.3.1，不能将其记为已运行 3.22。
5. GLFW API 初次用合成焦点消息断言系统焦点失败，改为自身真实窗口最小化/恢复后通过。这是 fixture 边界纠正，不是生产焦点缺陷结论。

## 下一步

先补 P04 真实输入对照并审核 [候选契约](platform-contract-candidate.md)，明确 P03/SDK/断网/DPI 的剩余限制在准备阶段解决或进入实施验收的安排，再进行 P06 评审。本轮尚未采样，也未填写 P95/P99、首次可操作时间或内存通过线；建议的 10 秒预热、30 秒采样、3 次重复尚待用户确认，双机使用相同参数，正式九轮仍延期。

开发桌面与 Y9000P 的 SDL 生产玩法、每机至少 15 分钟人工游戏、双机短采样和 D12 预算判定均未完成。P06 审核和实施授权成立后，才切换生产库并推进 S1–S5，不把独立探针记作 T2 完成。
