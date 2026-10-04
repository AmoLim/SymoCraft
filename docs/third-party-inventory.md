# 第三方依赖盘点

## 范围与证据等级

- 盘点日期：2026-09-17，阶段：M0。
- 本文依据仓库内文件、源码引用、当前顶层 CMake 配置，以及本机已有 `dumpbin` 的只读输出。
- 依赖盘点没有下载、安装或升级依赖；实际配置、构建和启动由独立实验验证，结果见 [M0 主报告](milestones/m0/README.md)。
- 下述版本与来源均为本地文件记载，未与上游发行包或提交进行逐文件比对。“文件记载来源”不等同于已经验证供应链来源。
- “未找到完整许可材料”表示仓库材料待补充，不表示禁止使用或分发。本文不作法律结论。

## M1 构建变更

以下 M0 表格保留历史审计事实，不代表修改后的构建图。M1 未升级或删除第三方源码，但调整了集成方式：GLFW 的示例、测试、文档和安装关闭；游戏显式链接 `glm::glm`；glad 与 yaml-cpp 分别进入静态库目标；取消未使用的 irrKlang 链接、DLL 复制和公共头引入。原二进制目录继续保留，不再纳入游戏构建及开发暂存安装。

这不是新增音频系统，也未补齐上游版本和许可材料。当前目标依赖关系见 [构建指南](project/build/build-and-clion.md)，实际验证状态见 [M1 报告](milestones/m1/README.md)。

## M0 使用的依赖

M2-T3 增量说明：复用已有 `vendor/stb/stb_image_write.h` 保存采样结束后的诊断截图，没有下载或升级 stb；CPU 内存读数使用 Windows SDK 的 Psapi，GPU 设备传感器可选调用本机已有 `nvidia-smi`。后者不是随包部署的依赖，缺失时明确记录。M0 表格及尚未补齐的许可/来源事项仍保留，不因本次增加观测而视为关闭。

表中路径均相对于仓库根目录。

| 依赖 | 本地版本与来源证据 | 当前实际使用 | 本地许可材料 | ABI / 兼容性状态 |
| --- | --- | --- | --- | --- |
| GLFW | `vendor/glfw/CMakeLists.txt:3` 记载 3.3.7；`vendor/glfw/README.md` 记载项目说明与来源 | `CMakeLists.txt` 添加源码子目录并链接 `glfw`；负责窗口、输入与 OpenGL 上下文 | `vendor/glfw/LICENSE.md` 有完整许可文本 | 当前使用源码构建，不依赖 `lib/glfw` 中的二进制；实际 MSVC 编译结果另行记录 |
| GLM | `vendor/glm/detail/setup.hpp:6` 起记载 0.9.9.8；`vendor/glm/glm.hpp:99` 起记载项目网站与 GitHub 地址 | `include/core.h` 引入数学相关头文件；CMake 另建立 `glm` 接口目标，但游戏目标未显式链接该接口目标，当前依靠全局包含目录 | 未在 `vendor/glm` 内找到完整许可文件 | 主要为头文件，无独立预编译库 ABI；C++20 / MSVC 源码兼容性需由构建确认 |
| glad | `vendor/glad/glad.h:3` 起记载由 glad 0.1.35 于 2022-06-05 生成，目标为 OpenGL 4.6 core，并保留生成参数 | `vendor/glad/glad.c` 直接编入游戏，加载 OpenGL 函数 | 未在 `vendor/glad` 内找到 glad 自身的完整许可文本 | 无预编译 ABI；生成记录标注 `Reproducible: False`，当前不能仅凭参数保证重新生成内容相同 |
| Khronos 平台头 | `vendor/KHR/khrplatform.h:30` 指向 Khronos EGL Registry；头部有版权年份 | glad 所需的平台类型定义 | 头部有完整版权与许可文本 | 头文件，无独立预编译 ABI |
| stb_image | `vendor/stb/stb_image.h:1` 记载 2.27，并记载项目地址 | `include/renderer/texture.h:3` 引入；`src/core/application.cpp:5` 定义实现宏；纹理加载使用 `stbi_*` | `vendor/stb/LICENSE` 与头文件末尾含 MIT / Unlicense 双许可文本 | 随应用源码编译；无外部预编译 ABI |
| FastNoiseLite | `vendor/fast_noise_lite/FastNoiseLite.h:47` 记载 1.0.1，下一行记载上游地址 | 区块地形噪声，实际使用 OpenSimplex2 与 FBm | 头部有完整 MIT 文本 | 头文件，无独立预编译 ABI |
| robin_hood | `vendor/robin_hood.h:37` 起记载 3.11.5；`:9` 记载上游地址 | 区块、方块配置、输入映射与着色器变量等容器 | 头部有完整 MIT 文本及 SPDX 标识 | 头文件，无独立预编译 ABI |
| yaml-cpp | 本地目录为头文件与源码子集；未找到明确库版本或上游提交标识 | `CMakeLists.txt:22` 起直接收集源码编入游戏；`src/world/block.cpp:35` 使用 `YAML::LoadFile` 读取方块配置 | 未在 `vendor/yaml-cpp` 内找到完整许可文件 | 当前随应用源码编译，并未链接 `lib/yaml-cpp` 中的文件；`vendor/yaml-cpp/dll.h` 默认不启用 DLL 导入导出宏 |
| irrKlang | `vendor/irrKlang/irrKlang.h:37` 记载头文件版本 1.6.0，并包含厂商项目地址；DLL 文件资源未提供可读取的版本号 | CMake 固定链接并复制 `lib/irrKlang`；`include/core.h:60` 引入头；唯一发现的引擎创建语句在 `src/core/application.cpp:29`，已被注释 | 头部有版权及免责声明；未找到完整发布许可材料 | 导入库和三个 DLL 已确认 x64；导入库为 MSVC 风格符号。头文件与 DLL 版本是否完全匹配、实际加载调用是否正常仍待验证 |

## 预编译文件与重复材料

| 路径 | 已验证事实 | 当前构建关系 | 注意事项 |
| --- | --- | --- | --- |
| `lib/irrKlang/irrKlang.lib` | `dumpbin /headers` 显示 x64；导出 MSVC 风格 `?createIrrKlangDevice@irrklang@@...` 等导入符号，指向 `irrKlang.dll` | 顶层显式链接 | 没有发现 x86 / x64 架构混用，不能将其列为已证实的位数冲突 |
| `lib/irrKlang/irrKlang.dll` | x64；linker version 14.12；静态依赖表列出 `KERNEL32.dll`、`USER32.dll`、`ole32.dll`、`WINMM.dll` | 顶层构建后复制 | 静态依赖表没有显式 VC runtime DLL，不代表已经完成运行时验证或动态依赖验证 |
| `lib/irrKlang/ikpMP3.dll`、`ikpFlac.dll` | 均为 x64；linker version 14.12；静态依赖表仅列 `KERNEL32.dll` | 随整个目录复制 | 是否需要分发插件应由实际音频范围决定 |
| `lib/glfw/glfw3.dll`、`libglfw3.a`、`libglfw3dll.a` | 均显示 x64；DLL linker version 为 2.30，依赖表包含 `msvcrt.dll`；检查 DLL 时工具报告 LNK4078 节属性警告 | 当前顶层不引用，使用 `vendor/glfw` 源码目标 | 不应仅依据扩展名或位数认定 MSVC 兼容；这些文件也不是当前已证实的构建阻塞项 |
| `lib/yaml-cpp/libgtest.a`、`libgtest_main.a`、`libgmock.a`、`libgmock_main.a` | 实际是 Google Test / Mock 命名的四份库，均显示 x64；`libgtest.a` 符号表包含 `_ZN...` 风格 C++ 符号 | 当前顶层不引用；它们不是正在链接的 yaml-cpp 库 | 不应直接作为 MSVC 测试依赖接入；如后续引入测试框架，应使用与主工程一致的工具链从明确来源构建 |
| `vendor/stb-master` | 项目源码未发现引用；其中 `stb_image.h` 与 `vendor/stb/stb_image.h` 的 SHA256 相同 | 当前实际使用 `vendor/stb` | 仅确认这一个头文件相同，不代表整个目录逐文件一致；M0 不删除文件 |

## 构建边界观察

1. 顶层 CMake 没有统一依赖清单、版本锁定记录或校验值清单。源码随仓库存放能固定当前文件内容，但尚不能明确追溯每项的原始发行包和后续修改。
2. GLFW 自带构建默认开启示例、测试、文档选项，顶层没有主动关闭。它们扩大默认构建范围，但不能未经构建就认定为失败原因。
3. 当前主要依赖从源码或头文件参与构建。irrKlang 是游戏目标显式链接的外部预编译依赖，应与未使用的 `.a` 文件分开评估。
4. 音频初始化目前被注释。第一阶段是恢复现有玩法，不应把更换音频库或新增音频功能自动纳入范围。

M0 后续实际构建补充：Debug / Release 的游戏目标已使用 MSVC 19.38 x64 编译链接成功；两个最终可执行文件的静态导入表未列出 irrKlang.dll。Release 依赖 MSVC/UCRT 运行库，Debug 依赖调试版运行库，不能把 Debug 产物当作独立发布版本。见 [最终二进制依赖记录](milestones/m0/evidence/08-executable-dependencies.log)。这个结果不替代启用音频后的实际加载与调用测试。

## 本机复核方法

本次使用 VS2022 已安装工具，只读检查，不执行编译或链接。以下命令在仓库根目录的 PowerShell 中运行；工具路径是本次机器记录，不是项目必须硬编码的路径。

```powershell
$dumpbin = 'E:\Applications\Microsoft VS\2022\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\dumpbin.exe'

# 查看二进制架构与链接器版本。
& $dumpbin /headers lib/irrKlang/irrKlang.lib lib/irrKlang/irrKlang.dll lib/irrKlang/ikpMP3.dll lib/irrKlang/ikpFlac.dll

# 查看静态导入依赖，不等同于实际加载验证。
& $dumpbin /dependents lib/irrKlang/irrKlang.dll lib/irrKlang/ikpMP3.dll lib/irrKlang/ikpFlac.dll

# 查看导入库符号。
& $dumpbin /linkermember:1 lib/irrKlang/irrKlang.lib

# 检查当前未使用的预编译库。
& $dumpbin /headers lib/glfw/glfw3.dll lib/glfw/libglfw3.a lib/glfw/libglfw3dll.a
& $dumpbin /headers lib/yaml-cpp/libgtest.a lib/yaml-cpp/libgtest_main.a lib/yaml-cpp/libgmock.a lib/yaml-cpp/libgmock_main.a
& $dumpbin /linkermember:1 lib/yaml-cpp/libgtest.a

# 复核两份实际相关头文件的内容是否相同。
Get-FileHash vendor/stb/stb_image.h,vendor/stb-master/stb_image.h -Algorithm SHA256 | Format-List Path,Hash
```

本次 `dumpbin` 报告版本为 14.44.35222.0。这里记录的是审计工具版本，不是主工程最终选定的编译器版本；不能用它推断 CMake 实际使用了 MSVC 14.44。

## 后续建议与验收

| 阶段 | 建议动作 | 验收证据 |
| --- | --- | --- |
| M1 构建恢复 | 明确 MSVC x64 工具链；保留当前依赖基线，先处理实际编译与链接错误，不同时全量升级依赖 | 干净构建目录中的配置、Debug / Release 构建记录及实际编译器版本 |
| M1 运行恢复 | 明确现有玩法是否需要 irrKlang；若继续启用，则验证头文件、DLL 版本与运行调用；若不需要，评估独立可选化 | 依赖决策记录、加载结果和现有玩法冒烟检查 |
| 工程整理 | 为每项依赖记录来源、版本或提交、文件校验值、本地修改说明；区分运行依赖与第三方开发材料 | 依赖清单可追溯，主工程依赖通过目标级配置表达 |
| 发布准备 | 补齐缺少的许可与通知材料，并依据确定的发布范围核对分发要求 | 发布目录中的第三方通知与依赖清单；不以本次本地材料盘点替代授权核对 |
| 后续测试建设 | 使用同一 MSVC 工具链构建选定测试框架，不直接复用来源不明的 `.a` | 测试依赖来源、构建配置与测试执行记录 |

动态区块加载属于后续功能与性能阶段，不影响本次依赖证据的范围，也不是 M0 的实现内容。
