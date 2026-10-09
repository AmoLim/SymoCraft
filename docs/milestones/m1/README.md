---
tags:
  - area/milestones
  - topic/build
---

# M1 可靠构建报告

日期：2026-09-17。状态：**实施和自动验证完成，等待 CLion 内构建确认及用户验收。** M0 已由用户验收通过；本轮没有进入 M2，也没有宣称交付经过玩法验收的游戏版本。

## 本节点范围

建立 Windows x64、MSVC、CMake 和 CLion 的明确构建入口，整理目标级依赖，解除运行资源对工作目录的隐含依赖，增加无窗口的资源测试，并记录构建失败与修复验证。保持现有世界、渲染、ECS、物理和玩法实现，不借此进行全面架构替换。

## 已实现内容

| 部分 | 变更及理由 |
| --- | --- |
| 共享预设 | Ninja + x64 外部工具链环境；Debug/Release 分目录；CLion 关联 `Symocraft MSVC`；统一 `VSLANG=1033` |
| 目标级构建 | 显式游戏和 yaml-cpp 源码；独立 glad/yaml/assets 目标；按目标设置包含目录、C++20、`/utf-8`；关闭 GLFW 附加构建 |
| 依赖收敛 | 取消未实际使用的 irrKlang 链接、复制和公共头依赖；不升级或删除第三方源码 |
| 开发入口 | PowerShell 辅助脚本发现完整 VS2022，初始化 x64 环境并恢复调用前环境；支持独立验证目录，不修改系统 PATH |
| 资源部署 | 构建时用一个共享目标同步六个必需资产；提供只含游戏及资产的开发暂存安装目标 |
| 资源定位 | 从可执行文件目录寻找 `assets`，不依赖或修改 CWD；拒绝绝对路径、父目录穿越、数据流及空路径 |
| 无窗口检查 | `--check-assets`：成功 0、缺资产 2；未知参数 64；不初始化 GLFW 或 OpenGL |
| 测试 | 路径契约、不同 CWD、迁移路径含空格、模块 Unicode 路径、缺文件和真实游戏参数检查 |

完整设计与复现入口：[构建与 CLion 指南](../../project/build/build-and-clion.md)、[资源定位模块](../../legacy/architecture/asset-paths.md)。

## 验证记录

验证环境沿用 M0 的 VS2022、MSVC 19.38.33145.0（14.38.33130）、SDK 10.0.22621.0；CMake 4.3.1，Ninja 1.13.2。用户补装 English 语言包后，两个已安装 MSVC 工具集均已检测到 `1033/clui.dll`；最终干净构建实际仍选择 14.38.33130，未静默升级工具集。

| 检查 | 当前结果 | 证据或说明 |
| --- | --- | --- |
| CLion 工具链设置 | 截图确认正确 | VS2022、amd64、CMake/Ninja/cl 已检测；用户自行操作 IDE |
| CLion 两个启用预设 | 截图确认正确 | 均绑定 `Symocraft MSVC`，Debug/Release 类型、预设参数和目录相符；不等于 IDE 内构建已验收 |
| 初始 Debug 构建与测试 | 编译完成，但存在构建依赖缺陷；沙箱内测试 1/3 通过 | [初始日志](evidence/01-debug-build-test.log)，不能作为通过证据 |
| 设置 VSLANG 后独立目录 Debug | 编译完成，仍回退中文；沙箱内测试 1/3 通过 | [干净目录实验](evidence/02-debug-clean-build-test.log)，仍不是可靠构建基线 |
| 相同 Debug 测试正常权限复验 | 3/3 通过 | [授权宿主复验](evidence/03-debug-tests-host.log)，没有修改 TMP 来放宽测试条件 |
| 真实游戏迁移包无窗口检查 | 通过 | [独立脚本结果](evidence/04-game-package-preflight.log)：含空格目录、无关 CWD、完整资产返回 0、缺纹理返回 2 且具名、错误参数返回 64 |
| 英文语言包后全新 Debug/Release | 两者均通过 | [Debug 78 步构建](evidence/05-debug-english-build.log)、[Release 78 步构建](evidence/06-release-english-build.log)，目录为 `out/m1-validation/english-debug` / `english-release` |
| Ninja 头文件依赖及真实增量重编译 | 通过 | [增量验证](evidence/07-incremental-verification.log)：五个对象各记录 108/413/401/107/110 个依赖，均含 `asset_paths.h`；触碰时间戳后全部重编译；不改文件再次构建不重编译 |
| 最终四项 CTest Debug/Release | 两者各 4/4 通过 | 正常宿主权限下 [Debug](evidence/08-debug-final-tests.log)、[Release](evidence/08-release-final-tests.log)，含新游戏包测试 |
| 安装目录与安装后预检 | 通过 | [Release 安装](evidence/09-release-install.log)、[安装文件校验](evidence/10-installed-package-check.log)：7 个文件均与来源 SHA256 一致，从源码根 CWD 预检通过 |
| 辅助脚本环境恢复 | 修复后通过 | [首次失败](evidence/11-script-environment-check.log)、[修复复验](evidence/12-script-environment-retest.log)：环境变量数量和值、工作目录均与调用前一致 |
| 游戏画面、玩法与长时间稳定性 | 本节点未验收 | 使用 M2 独立用例，不沿用 M0 的启动画面冒充本轮验证 |

## 失败与复验

### 头文件依赖未被记录

Ninja 通过 MSVC `/showIncludes` 输出识别头文件。早期生成的 `msvc_deps_prefix` 出现乱码，`ninja -t deps` 显示游戏对象文件依赖数为 0。因此，编译成功仍可能导致编辑头文件后不重编译，属于 M1 阻断项。

仅设置 `VSLANG=1033` 的早期复验没有解决问题：当时本机两个 MSVC 工具集只有中文资源目录 `2052`，缺少英文 `1033/clui.dll`。用户已自行补装 VS2022 English 语言包。工程增加明确配置检查，缺语言资源、环境变量不匹配或前缀异常时直接停止，不掩盖缺陷。

补装后已在新目录中重新检测、构建并验证依赖数据库。生成前缀变为 `Note: including file:`；五个引用资源头的对象均在头文件时间戳变化后重编译，无变化再构建只有资源同步目标执行。验证没有改变头文件内容，最后恢复了其原时间戳。此项在本轮工具链下已解决；保留旧错误目录与日志，未删除用户的 IDE 构建目录。

可复现脚本：[Verify-Incremental.ps1](Verify-Incremental.ps1)。它针对本轮 Debug 验证目录执行，先检查依赖，再验证实际重编译和无修改构建，不用 `assert` 或人工目测替代结果。

### 沙箱中的迁移子进程失败

资源模块测试把自己的 exe 复制到系统临时目录，再用不同 CWD 启动。受限环境下子进程返回 1，原诊断未保留退出码。本轮补充退出码后，经用户授权在正常宿主权限下重跑相同 CTest，三项全部通过。

该对照表明早期失败与运行限制有关，不据此修改生产资源解析逻辑。底层究竟哪个文件访问被限制尚未取得精确子进程诊断；不要把它写成已经证明的某一种 DLL 故障。自动测试必须记录运行权限，不能静默跳过失败用例。

### 辅助脚本留下空环境变量

首次逐项比较调用前后环境时，发现当前 PowerShell/.NET 组合下通过 `SetEnvironmentVariable(..., $null, ...)` 清理新增项，留下了值为空的变量，而非真正删除。改用 PowerShell 环境提供程序移除本次新增项后，正常配置调用的变量数量和值及工作目录完全恢复，复验已通过。没有修改系统环境变量或注册表。

## 产物与复现

- 默认 IDE 构建目录仍是 `out/build/windows-debug` 和 `out/build/windows-release`；本轮使用独立 `out/m1-validation/english-*`，避免与用户 IDE 并发写同一目录。
- Release 开发暂存目录：`out/install/windows-release`，包含 `SymoCraft.exe` 和六个必需资产；没有测试 exe、irrKlang DLL 或开发源文件。
- 普通 PowerShell 可使用 `scripts/build.ps1`；完整参数与预设说明见构建指南。涉及迁移子程序的自动化运行环境必须允许从其系统临时目录启动子进程。
- 后续重置 IDE 旧缓存后，还要清理并重新构建两种配置，避免沿用旧的零依赖对象文件。没有代替用户操作 CLion。

## 待验收条件

前三项自动条件已满足：全新 Debug/Release 构建及测试通过、头文件依赖与增量编译正确、资源同步及开发暂存安装通过。剩余人工确认：

1. 用户在 CLion 重置本项目旧 CMake 缓存并重新加载，对 Debug/Release 清理后重新构建 `SymoCraft`，确认 IDE 实际调用正确工具链。
2. 确认以下已知边界，由用户决定是否通过 M1；通过后才进入 M2 的首个可玩版本验收。

## 已知边界

- `--check-assets` 检查必需普通文件是否存在，不验证 YAML 语义、图片解码、shader 编译、显卡能力或玩法。
- 路径模块的 Unicode 测试不代表旧游戏加载器的 `.string()` 调用已支持任意 Unicode 安装路径；完整游戏暂使用 ASCII 路径。
- 旧初始化失败传播、OpenGL 对象销毁顺序、网格容量、固定步长积累等问题未在 M1 修复，仍进入首个可玩版本的正确性验收。
- 安装目标尚未收集 MSVC 运行库、完整第三方通知，也未在无开发环境机器验证；不能作为 M6 最终分发包。
- 未进行性能测量或参考笔记本验证；没有实现动态区块加载、存档或新玩法。
