---
tags:
  - area/dependencies
---

> 2026-10-11 旧导航引用已失效，已改指现行权威或明确归档入口；历史命令、结果和批准含义保持不变。

# 第三方依赖与通知盘点

本页维护当前活动依赖、本地身份、修改和安装通知事实，安装为 `THIRD-PARTY-INVENTORY.md`。核对日期为 2026-10-11，依据当前源码、CMake 和本地许可材料；未重新下载上游、构建或核验发行包。阶段批准与实际验证只从仓库的 [VERIFICATION](verification/VERIFICATION.md) 判读，不在这里建立批准表。

以下版本声明、已核验来源和来源未解分别记录，不能互相推定。本页不是完整许可文本或法律结论，静态集成不等于没有系统 DLL；盘点完成不证明公开发行或干净机通过。包内阅读不依赖源码链接，末尾链接仅适用于源码 checkout。

2026-10-25 定向更新：已验收 T5 的 EnTT 源码、配置及许可安装声明已按独立授权同步主工作区。以下仅补齐受影响的依赖/ABI 与当前后端范围，不将旧来源缺口或干净机验证改为通过；同步检查见 T5 verification。

## 活动依赖与边界

| 项目 | 当前消费者与公开边界 | 本地身份与关键影响 |
| --- | --- | --- |
| EnTT | ECS 的受控 typed 模板依赖 `symocraft_entt`，仅传播 src 头根；非 ECS 生产代码不直接包含 native API，独立 benchmark 不引入 ECS/EnTT | 官方 v4.0.0、完整 commit `85c6bba014049b5de8fad49d25424df2f1f6a8c1`；本地原样 src 与 MIT，`ENTT_NO_ETO` 保真实空组件对象；DBG01 对全部 Debug C++ 统一 level 0 并保留 Debug CRT，须全新目录重建。准确来源/原始失败与保证边界见 T5 verification |
| SDL3 | platform 私有 `SDL3::SDL3-static`，窗口/输入、GL/Native/Vulkan 桥接；不向游戏公共头或独立 runner 暴露 SDL 类型 | 官方 `release-3.4.18`，commit `829a65d769d935c4852f8159e964312c0957260a`；原样 vendor 及 ZIP/逐文件身份见 T2 来源证据。静态不另带 SDL3.dll；动态 Vulkan 窗口桥接不代表生产 Vulkan renderer 已实现 |
| GLM | foundation 的 `symocraft_math` 公共数学入口，scene/world/simulation/renderer 等传递使用；`glm::glm` 是别名 | `vendor/glm/detail/setup.hpp` 为 0.9.9.8。公共 `glm::*`、宏和顶点布局属于跨模块契约，升级或改对齐须检查布局；不能把传递包含路径当所有模块直接调用 |
| glad | 默认 legacy OpenGL renderer 私有函数加载/调用，部分白盒测试及显式 GL 对照 | glad 0.1.35，于 2022-06-05 生成 GL 4.6 core，记录 `Reproducible: False`；生成器身份不证明精确上游提交或可重复生成。D3D12 候选不自动使默认 GL 路径退役 |
| Khronos 平台头 | glad 配套 `vendor/KHR/khrplatform.h`，不是独立运行库 | 本地头保存 Khronos 2008-2018 版权与完整许可；根 KHR 配套头、SDL 内部 Khronos 头及未来图形 SDK 是不同材料 |
| stb_image | assets 私有 CPU 图片解码，公开返回自有 `Assets::Image` | 本地 2.27；实现宏在 assets/image.cpp，解码内存由模块释放；更换 GPU 后端不替代纹理解码 |
| stb_image_write | assets 私有 PNG 编码，renderer 经 `Assets::WritePng` 保存诊断，D3D12 R1 实验有独立实现 | 本地 1.16；`vendor/stb/LICENSE` 同时覆盖读取与写出组件。截图是额外绘制/读回，不等于整段视觉验收 |
| FastNoiseLite | world 私有三层种子噪声，公共类型不暴露实现 | 本地 1.0.1，OpenSimplex2/FBm。版本/算法变化影响固定种子世界和摘要，不是无行为变化的依赖替换；植被另用标准 mt19937 |
| robin_hood | 仅 legacy renderer 的 `(program, name)` uniform location 缓存 | 本地 3.11.5；world 和当前 platform 输入已不用。未测得与标准容器等价性能，尚未替换 |
| yaml-cpp | world/telemetry 私有文本转换；独立 benchmark_core `PUBLIC symocraft_yaml` 且 runner.h 公开 `YAML::Node` | 本地头/源码子集没有明确版本/提交。真实实现是 vendor/yaml-cpp，不是 lib/yaml-cpp 的历史测试二进制；仅改 PRIVATE 不能隔离 runner 公开类型 |

SDL 仅在游戏图形配置中创建：video、桌面 GL、动态 Vulkan 桥接启用，无关子系统、shared、自测/示例与 SDL 独立安装关闭；不修改上游源码来减少文件。GL 请求及实际能力仍由真实驱动确认，glad 不提供 OpenGL 实现。

yaml-cpp 的已知本地修改为 T1 的 `vendor/yaml-cpp/src/stream.h`、`stream.cpp`：prefetch 数组改由 `unique_ptr<unsigned char[]>` 接管；修改/失败/复测身份由 T1 证据维护，不能把本地修补说成升级或已确认上游版本。去除 yaml-cpp 须覆盖配置、标量/数字样字符串、序列化、错误和旧文件协议，单换方块解析不足。

## 构建与系统依赖

| 模式 | 实际依赖范围与限制 |
| --- | --- |
| 默认游戏 / game-only | 上述游戏库含 EnTT；关闭 Benchmark 不改变游戏依赖。默认 D3D12 需 Windows/D3D12/DXC 构建能力，Vulkan 显式启用，GL 为保留的过渡路径 |
| 根 runner-only | 显式 `SYMOCRAFT_BUILD_GAME=OFF`、`SYMOCRAFT_BUILD_BENCHMARK=ON`、`SYMOCRAFT_BUILD_CPU_MODULES=OFF`，只用 yaml-cpp 与 Win32 工具库；已有 cache 不随另一开关自动变更 |
| 独立 tools/benchmark | yaml-cpp 与 Win32 工具库，不经过 game 构建或引入 SDL/GLM/glad/游戏 assets；子目录仍引用仓库共享材料，不能单独复制后构建 |
| CPU-only | EnTT、GLM、stb 读写、yaml-cpp、FastNoiseLite；assets 的 PNG 写出也编入 CPU 模块。关闭游戏/runner/原生后端，不配置 SDL 或编译 GL 实现。renderer API/app CPU 数据目标存在不代表 GPU 接入；仍要求 Windows x64/MSVC，不证明跨平台 |

头视图仅暴露选定第三方头，不公开整个 vendor 根目录或 `.cpp`。GLM 是游戏公共数学例外；EnTT 的 typed 模板依赖为 T5 明确授权的另一窄例外，不提供 native Registry/view/ID 逃生口，其余游戏实现依赖不得泄露到公共头。静态库 PRIVATE 依赖仍可能进入最终链接闭包，独立 runner 的公开 YAML 边界单独处理。

系统和外部工具不作为 vendor 库盘点：Windows SDK/Win32 提供文件发布、exe 定位、窗口/进程/Job API；Psapi 提供进程 working set/private bytes；benchmark_core 链接 bcrypt/advapi32/powrprof/user32，GUI 另用 comctl32/comdlg32/shell32/ole32。OpenGL 驱动、可选 NVX 和 GPU 查询按能力判读。导出需要系统 `System32/tar.exe`，缺失时失败且保留目录。Python collector 的可选 `nvidia-smi` 是设备级传感器，不是进程 VRAM，原生 runner 不采温度；旧 PowerShell 文件只启动 Python。

开发依赖为 C++20、MSVC x64、Windows SDK、CMake/Ninja/CTest、英语编译器资源及配置/构建一致的 `VSLANG=1033`；脚本与启用的 Python 测试需 Python 3.10+。Release 部署使用适用 MSVC/UCRT；D3D12 Debug 候选的调试 CRT 仅用于本地诊断，不赋予外部分发权限。静态库、导入表或本机启动不证明无开发环境机器可运行。

## 许可与安装身份

以下是当前 `game/app/CMakeLists.txt` 声明的游戏安装材料，不追溯改写旧冻结包。完整文本来自实际 vendor 文件或 `cmake/licenses/`；来源说明见 `cmake/licenses/SOURCES.txt`。头/源码通知记录是完整文件，不作为开发 SDK 安装。

| 项目 / 通知 | 本地材料与身份 | 游戏安装位置（相对包根） |
| --- | --- | --- |
| EnTT | vendor/entt/LICENSE 原始 MIT，SHA256 `73547f6370ee16ea560665329bb62d4e2d8250f41eaa204b47a6c694ca3286ea`，来源记录含完整官方 commit | licenses/EnTT-LICENSE.txt |
| SDL3 | vendor/sdl3/LICENSE.txt，zlib；官方来源另有 T2 核验 | licenses/SDL3-LICENSE.txt |
| GLM | cmake/licenses/GLM-LICENSE.txt，官方 0.9.9.8 copying.txt 的 MIT 选项；vendor 子集自身无独立完整许可文件 | licenses/GLM-LICENSE.txt |
| glad / Khronos 规范 | cmake/licenses/glad-LICENSE.txt，官方 v0.1.35 的 MIT / Khronos 通知；另存完整 Apache 2.0 文本，不证明 vendored exact commit | licenses/glad-LICENSE.txt、Apache-2.0-LICENSE.txt |
| yaml-cpp | cmake/licenses/yaml-cpp-LICENSE.txt，官方当前 MIT，版权 Jesse Beder 2008-2015；本地精确版本/来源仍未解，不能伪称为对应版核验 | licenses/yaml-cpp-LICENSE.txt |
| stb（读与写） | vendor/stb/LICENSE，MIT / Unlicense 双许可与头部通知 | licenses/stb-LICENSE.txt |
| FastNoiseLite | 完整本地头的 MIT / Jordan Peck 等版权通知 | licenses/FastNoiseLite-license-source.h |
| 根 Khronos 平台头 | 完整本地头的 Khronos 版权/许可 | licenses/Khronos-license-source.h |
| robin_hood | 完整本地头的 Martin Ankerl 2018-2021 MIT / SPDX 通知 | licenses/RobinHood-license-source.h |
| SDL hidapi / yuv2rgb | hidapi 多许可索引及 BSD 选项、yuv2rgb 完整本地许可；不据配置关闭省略已声明材料 | licenses/SDL3-hidapi-LICENSE.txt、SDL3-hidapi-BSD-LICENSE.txt、SDL3-yuv2rgb-LICENSE.txt |
| SDL 内嵌代码 | libm/e_log.c（Sun Microsystems）、SDL_qsort.c（Gareth McCaughan）、SDL_strtokr.c（CC0）、SDL_rotate.c（Andreas Schiffler / SDL_gfx）、内部 stb_image.h、miniz.h（Rich Geldreich）原文件通知 | licenses/SDL3-e_log-license-source.txt、SDL3-SDL_qsort-license-source.txt、SDL3-SDL_strtokr-license-source.txt、SDL3-SDL_rotate-license-source.txt、SDL3-stb_image-license-source.txt、SDL3-miniz-license-source.txt |
| SDL GL / Vulkan 桥接头 | SDL_opengl.h 的 Brian Paul / VMware、SDL_opengl_glext.h 的 Khronos；vk_platform.h、vulkan_win32.h 的本地 Apache-2.0 通知，不证明生产 Vulkan backend | licenses/SDL3-SDL_opengl-license-source.txt、SDL3-SDL_opengl_glext-license-source.txt、SDL3-vk_platform-license-source.txt、SDL3-vulkan_win32-license-source.txt |
| 来源记录 | cmake/licenses/SOURCES.txt（2026-10-09），区分官方许可来源与未解 vendor 身份 | licenses/SOURCES.txt |

独立 `tools/benchmark` 及根 runner-only 当前只安装 Benchmark exe、适用 CRT、本页和操作 README；没有单独安装 yaml-cpp 许可。不扩大安装范围来让盘点看似完整，本页不能替代通知原文。公开发行仍需按实际产物核对来源/本地修改、全部通知、运行库与真实干净机；“未找到”是证据缺口而不是禁止使用结论，“头中保存”也不证明实际包已经附带。

## 非活动材料与后续约束

| 材料 | 已知身份 / 使用状态 | 保留与变更边界 |
| --- | --- | --- |
| vendor/glfw、lib/glfw 与休眠旧代码 | vendor 3.3.7 及完整 vendor/glfw/LICENSE.md；生产配置/链接/授权/新包许可安装已退出；显式 GLFW 基线仍引用冻结静态库 | platform D11 要求保留供 T5 审计，不能物理删除。M0 二进制记录为 x64、DLL linker 2.30、msvcrt/LNK4078，均是历史检查，不证明当前 SDK/ABI/加载 |
| vendor/irrKlang、lib/irrKlang | 头声明 1.6.0，DLL 对应版本和完整发布许可待核对；活动源码/CMake不用，没有现役音频模块 | M0 导入库/3 DLL 为 x64、MSVC 符号，DLL linker 14.12；静态导入不证明实际加载。保留原身份，未来音频另选范围，不能据闲置推断永久不用 |
| lib/yaml-cpp/libgtest*.a、libgmock*.a | 四份历史 Google Test/Mock x64 二进制，来源/工具链未解，gtest 见 _ZN 符号；不是实际 yaml-cpp，也未被当前自有测试/CTest链接 | 不直接作为 MSVC 测试依赖接入，未来框架须明确来源并同工具链构建；材料处置另授权 |
| vendor/stb-master | 非活动；原审计只确认其中 stb_image.h 与 active 副本 hash 相同 | 不能断言整目录逐文件重复；清理前核对独有文件、工具及许可，保留 active vendor/stb |

依赖退出与仓库材料删除是两件事；清理闲置材料不再次减少当前 exe 依赖或证明 FPS 提升。robin_hood 标准容器替换、GL/glad 最终退役、YAML 公开边界隔离/格式更换都只是有前提的后续候选，不从本盘点取得实施授权，不为减少目录牺牲缓存语义、固定世界、截图或旧协议。依赖变更须覆盖实际消费者和必要生成/布局/解码/GL计时/YAML/runner回归，候选理由按 scope 维护，不建立第二任务清单。

## 仓库核对入口

当前接入见 [ThirdPartyGame](../cmake/ThirdPartyGame.cmake)、[ThirdPartyYaml](../cmake/ThirdPartyYaml.cmake)、[游戏安装声明](../game/app/CMakeLists.txt)、[独立工具安装声明](../tools/benchmark/CMakeLists.txt) 和 [通知来源](../cmake/licenses/SOURCES.txt)。SDL 的既有 [来源证据](verification/archive/milestone-evidence-20261011.7z "member: milestone-evidence-20261011/m3-t2/evidence/sdl-provenance.json") 与各阶段原始验证从 [VERIFICATION](verification/VERIFICATION.md) 进入；历史二进制审计暂保留于 [旧盘点](verification/archive/docs-navigation-20261011.7z "member: docs-navigation-20261011/historical-extracts.json")，不是当前操作或计划权威。本轮未重跑这些历史检查或新建批准。
