# 构建系统与 CLion 开发指南

> 适用范围：M1 构建指南，保留当时的命令与构建布局，不因目录迁移自动成为最新构建说明。后续目标拆分参见 [CMake 模块边界](CMake-模块边界.md)，自定义目录日志问题参见 [修复记录](CMake-自定义构建目录日志修复.md)。使用前应对照当前构建配置；本轮未重新验证本文全部命令。

## 范围

本指南对应 M1 的 Windows x64、MSVC、C++20 构建入口。项目使用仓库内已有依赖，不要求首次构建下载第三方源码，也不自动安装或升级编译器。

M1 解决“可靠地构建、部署开发资源、运行资源测试”。它不代表玩法、图形资源生命周期、长时间稳定性或最终分发已经通过验收。对应证据见 [M1 报告](../../milestones/m1/README.md)。

## 一次性准备

需要完整的 Visual Studio 2022 C++ 工具、Windows SDK 及 English 编译器语言包，CLion 可用自带的 CMake 和 Ninja。CMake 最低版本为 3.22。Ninja 本身不初始化 MSVC 的头文件、库和 SDK 环境，仅把 `cl.exe` 加入 PATH 并不足够。

语言包可在 Visual Studio Installer 中选择对应 VS2022 的“修改 -> 语言包 -> English”补装，保留中文，不需要修改 CLion 界面语言。见 [微软官方修改安装指南](https://learn.microsoft.com/en-us/visualstudio/install/modify-visual-studio?view=vs-2022)。

仓库不保存本机工具的绝对安装路径。共享 [CMakePresets.json](../../../CMakePresets.json) 记录构建类型、生成器、输出目录和测试开关；本机工具位置由 CLion 工具链、开发者终端或构建辅助脚本提供。

## CLion 配置

根据用户约定，CLion 界面配置由用户执行，代理不继续操作 IDE。以下配置仅用于 Symocraft，不需要改变其他项目的工具链。

1. 打开项目根目录中的 `CMakeLists.txt`，或直接打开 Symocraft 文件夹。保留已有项目时选择新窗口。
2. 进入“设置 -> 构建、执行、部署 -> 工具链”，新增 Visual Studio 工具链，命名为 `Symocraft MSVC`。
3. 选择完整的 VS2022 安装，目标架构选择 amd64/x64；使用 CLion 内置 CMake、Ninja，等待 C/C++ 编译器和 SDK 检测完成。M0 机器上的安装路径为 `E:\Applications\Microsoft VS\2022`，其他机器应选择自己的路径。
4. 在 CMake 设置中启用 `windows-debug`、`windows-release` 两个导入预设，确认关联 `Symocraft MSVC`。界面显示“让 CMake 决定”是正常的，预设已指定 Ninja；受预设管理的字段变灰也不是配置错误。
5. 重新加载 CMake 后，选择 `SymoCraft` 目标构建。若沿用补装英文语言包前的构建目录，先重置本项目 CMake 缓存并重新加载，再对两种配置分别执行清理后重新构建，不能只普通增量构建。运行可不指定特殊工作目录；资源必须来自该 exe 旁的 `assets`，而不是源码树。
6. 测试可以通过 CTest 集成执行，也可以使用下方命令行。CLion 对预设的支持主要是 configure/build，不能仅凭导入成功就假定 test preset 已执行。

CLion 默认把预设与工具链关联；本项目通过 `jetbrains.com/clion` 的 vendor 字段显式指定名称。该字段不影响普通 CMake 命令行。相关官方说明：[CMake presets](https://www.jetbrains.com/help/clion/cmake-presets.html)、[Toolchains](https://www.jetbrains.com/help/clion/how-to-create-toolchain-in-clion.html)。

验收时请记录：CLion 版本、实际编译器路径/版本、两种配置的加载与构建结果。IDE 导入或绿色配置提示不能替代真实构建和测试。

## 普通 PowerShell 入口

[scripts/build.ps1](../../../scripts/build.ps1) 会发现完整的 VS2022 安装，初始化仅本次调用使用的 x64 开发环境，调用同一组 CMake 预设，完成后恢复调用前的进程环境和工作目录。它不修改系统 PATH、执行策略、注册表或 IDE 设置。

本机示例：

```powershell
./scripts/build.ps1 -Configuration Debug -CLionPath 'E:/Applications/JetBrains/CLion'
./scripts/build.ps1 -Configuration Release -Action Install -CLionPath 'E:/Applications/JetBrains/CLion'
```

其他机器把 CLion 路径替换为自己的安装位置。也可在当前 PowerShell 设置 `CLION_HOME` 后省略该参数，或通过 `-CMakePath`、`-NinjaPath` 使用独立安装工具。

| 参数 | 行为 |
| --- | --- |
| `-Configuration Debug/Release` | 选择 `windows-debug` 或 `windows-release` |
| `-Action Configure` | 只配置，不编译 |
| `-Action Build` | 配置并构建默认目标，包括启用的测试程序 |
| `-Action Test` | 配置、构建、执行 CTest；默认动作，0 个测试也视为错误 |
| `-Action Install` | 先配置、构建、测试，全部成功后安装到开发暂存目录 |
| `-Jobs 8` | 构建并行度，默认 8 |
| `-VisualStudioPath` | 显式选择本机 VS 安装，默认发现完整 VS2022 |
| `-ToolsetVersion 14.38` | 可选，选择已安装 MSVC 工具集；不下载该版本 |
| `-BuildDirectory out/verification/debug` | 使用独立验证目录；相对路径基于项目根目录，不删除旧目录 |

选择不同编译器或生成器时应使用新的构建目录，不混用已有缓存。若需保留旧证据，也应另取目录，而不是清除用户已有文件。

## 已初始化开发者终端

在 x64 Visual Studio 开发者终端中，并确保 CMake、Ninja 在 PATH 上：

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug --parallel 8
ctest --preset windows-debug --no-tests=error

cmake --preset windows-release
cmake --build --preset windows-release --parallel 8
ctest --preset windows-release --no-tests=error
cmake --install out/build/windows-release
```

Ninja 预设中的 `architecture.strategy=external` 表示架构环境由 IDE 或开发者终端建立，并不让 CMake 给 Ninja 传入不支持的 `-A`。相关官方解释见 [CMake presets 手册](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html)。

## 输出布局

```text
out/
  build/windows-debug/
    bin/SymoCraft.exe
    bin/asset_paths_tests.exe
    bin/assets/configs/blockFormats.yaml
    bin/assets/shaders/*.glsl
    bin/assets/textures/texture_atlas.png
  build/windows-release/
    bin/...
  install/windows-release/
    SymoCraft.exe
    assets/...
```

安装目标只放游戏和当前必需资产，不放测试程序、PSD、参考图片或未使用音频 DLL。开发构建旁的 PDB/ILK 是编译器产物，不是游戏必需资产。

**安装目录是开发验收用暂存目录，不是最终独立分发包。** Release 仍依赖 MSVC/UCRT 运行库；M6 才验证无开发环境机器和完整通知材料。不要将 Debug 所需调试运行库当作正常发布依赖。

## 构建图与设计理由

```text
SymoCraft
  -> glfw                    Windows 窗口、输入、上下文
  -> symocraft_glad          OpenGL 入口加载
  -> glm::glm                数学头文件
  -> symocraft_yaml          现有 yaml-cpp 源码
  -> symocraft_vendor_headers 其余现有头文件依赖
  -> symocraft_assets        无窗口的资源定位模块

asset_paths_tests -> symocraft_assets
两个可执行目标 -> symocraft_runtime_assets
```

- 顶层使用显式源码清单，新增/删除文件时能在评审中看到构建范围变化；不再用宽泛 GLOB 隐式改变项目源文件。
- 依赖的包含目录、链接关系和语言标准通过 target 表达，不使用全局 include/link 目录影响所有目标。
- GLFW 的示例、自测、文档和安装默认关闭；项目自己的 `BUILD_TESTING` 控制资源测试，两个“测试”概念不能混淆。
- 现有 yaml-cpp 和 glad 独立为静态目标，复用现有源码，不进行版本迁移。GLM 与其余头文件依赖通过接口目标表达。
- 没有有效音频调用，因此取消 irrKlang 的强制链接、复制和公共头引入。原文件保留在仓库，便于追溯；没有新增音频系统。
- C++20 与 `/utf-8` 针对项目目标配置，不使用全局禁用警告来掩盖旧代码问题。第三方头目录标为 SYSTEM，与项目源码诊断分开。
- 资源同步是一个共享构建目标，使用 `copy_if_different`。每次构建检查六个必需文件，但内容未变不重复写入；无需重新链接 exe 才能同步资源。

## 增量构建与中文工具链

Ninja 需要从 MSVC 的 `/showIncludes` 输出记录头文件依赖。本轮早期实验发现中文输出前缀被错误解码，导致编译看似成功但头文件依赖数量为 0。这会让修改头文件后仍运行旧二进制，属于构建正确性问题，不只是日志难看。

共享预设及辅助脚本统一设置 `VSLANG=1033`，保持配置和编译阶段的输出一致。**仅设置变量不足以保证生效**：本机早期复测发现 MSVC 目录只有 `2052/clui.dll`，没有 `1033/clui.dll`，仍回退到中文，依赖问题未解决。因此 Ninja 配置现在检查英文语言资源、环境变量和检测到的前缀，条件不满足就明确失败，不生成看似可用的工程。

安装语言包后仍需全新构建、检查 Ninja 依赖数据库，以及修改头文件后的重编译行为。不通过过滤输出隐藏问题，也不硬编码本机中文前缀修补生成文件。本机已完成此项复验，证据与复现脚本见 [M1 报告](../../milestones/m1/README.md)。

若旧构建目录出现过大量“注意: 包含文件:”输出或乱码前缀，应重新配置全新的构建目录。CLion 中可对 Symocraft 进行重置缓存并重新加载，再清理并重新构建两种配置；不要对其他项目执行清理。仅删除 CMake 缓存可能仍保留旧对象文件，原错误配置的文件不能用一次普通增量构建直接充当干净基线。

## 资源测试的边界

资源位置按 exe 目录确定，完整接口、异常策略和测试说明见 [资源定位模块](../../legacy/architecture/asset-paths.md)。

```powershell
./out/build/windows-debug/bin/SymoCraft.exe --check-assets
```

返回 0 表示六个必需文件存在；缺文件返回 2；不支持的参数返回 64。该检查不创建窗口，也不检查纹理是否能解码、YAML 语义是否有效或 shader 是否能编译。

测试程序使用显式失败返回而非 `assert`，Release 下不会因 `NDEBUG` 而变成空测试。模块测试的 Unicode 支持不等于旧游戏加载器已经支持所有 Unicode 路径；完整游戏目前使用 ASCII 安装路径，包含空格的路径需要单独验证。

## 常见问题

| 现象 | 处理 |
| --- | --- |
| 找不到 cmake/ninja/cl | 先确认实际安装；使用 CLion 工具链或辅助脚本，不仅依赖普通终端 PATH |
| 找到了 VS2026，但安装不完整 | 选择完整 VS2022；辅助脚本默认不会选择该不完整实例 |
| CMake 提示非 MSVC / 非 x64 | 检查是否误选 MinGW 或 x86；本阶段不承诺其他平台 |
| CMake 提示缺少 English compiler language pack | 给选中的 VS2022 补装 English 语言包，使用全新构建目录；不要手改生成的 Ninja 前缀 |
| DLL 缺失 | 区分 Debug 调试运行库和 Release 运行库；不要从任意网站单独下载 DLL |
| `--check-assets` 缺文件 | 重新构建同步资产，或重新安装；移动 exe 时应携带旁边整个 assets 目录 |
| `--check-assets` 通过但游戏失败 | 资源存在性不是运行正确性；保留日志，按 M2 的加载、图形和生命周期用例定位 |
| CTest 返回非零 | 保留失败输出，不因 exe 已经生成就忽略测试结果 |

最终是否通过 M1 以 [里程碑报告](../../milestones/m1/README.md) 中的实测范围为准。
