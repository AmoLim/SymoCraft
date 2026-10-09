---
type: 功能
status: 已验证
project: SymoCraft
module: build
created: 2026-10-04
tags:
  - area/build
---

# CMake 模块边界

关联功能：[[Benchmark-执行与导出]]。

> 历史范围：本页记录 2026-10-04 的游戏/执行器分离。M3-T0 已于 2026-10-05 将游戏拆为九模块、统一测试树，本文的 `src/include/tests` 图不再代表当前构建。当前规则见 [构建与测试边界](../architecture/build/构建与测试边界.md)，验证见 [M3-T0 报告](../../milestones/m3-t0/README.md)；下文保留当时设计，不改写历史结论。

## 当前设计

软边界用目录，硬边界用 CMake target。保留现有 src/include，不为目录整洁重排游戏逻辑；新增 `game/CMakeLists.txt` 统一拥有游戏构建规则。

```text
根 CMakeLists.txt
  ├─ Windows x64 / MSVC / CTest / 开关 / Release CRT 部署
  ├─ cmake/ThirdPartyYaml.cmake → symocraft_yaml（已有第三方）
  ├─ SYMOCRAFT_BUILD_GAME=ON → game/CMakeLists.txt
  │    ├─ GLFW / GLAD / GLM / assets / GPU timer
  │    ├─ SymoCraft ← src/ + include/
  │    └─ tests/（游戏与历史脚本回归）
  └─ SYMOCRAFT_BUILD_BENCHMARK=ON → tools/benchmark/CMakeLists.txt
       ├─ benchmark_core ← 本模块 include/ + platform.cpp + runner.cpp
       │    └─ yaml-cpp / Win32 / BCrypt / registry / power API
       ├─ SymoCraftBenchmark ← main.cpp + gui.cpp
       │    └─ benchmark_core / Common Controls / shell dialogs
       └─ tools/benchmark/tests（本模块夹具和执行器测试）

SymoCraftBenchmark --进程参数/版本化文件--> SymoCraft
无游戏到执行器的链接，也无执行器到引擎头文件的包含。
```

### 构建入口

在配置好的 MSVC x64 环境、`VSLANG=1033` 下，现有 windows-debug/windows-release 预设默认构建两者。CLion 可分别选择 `SymoCraft` 或 `SymoCraftBenchmark` 目标。

```cmake
# 作为 CMake 配置选项传入，不是修改源码：
SYMOCRAFT_BUILD_GAME=OFF       # 根项目仅执行器
SYMOCRAFT_BUILD_BENCHMARK=OFF  # 根项目仅游戏
```

执行器也可 `cmake -S tools/benchmark -B out/build/runner-only -G Ninja -DCMAKE_BUILD_TYPE=Release` 独立配置。仍使用仓库内 vendor/yaml-cpp、共享编译环境校验与交付文档，不表示把子目录单独拷走即可构建。该模式不配置 GLFW、不检查 assets、不编译游戏。

### 资源和交付

游戏仍输出至构建目录 bin，使用原有 staging 规则复制六项 assets。执行器使用自己的 RUNTIME_OUTPUT_DIRECTORY，不借用游戏 staging 函数。关闭执行器不影响游戏，关闭游戏不要求游戏资源。

Release 安装通过 CMake `InstallRequiredSystemLibraries` 携带本机工具链对应的可再分发运行库；Debug 不打包。Windows 系统 DLL 和显卡驱动不随包拷贝。该机制不能替代无开发环境电脑实测。

### 验收案例

| 状态 | 场景 | 预期行为 |
| --- | --- | --- |
| [x] | 根工程开启两者，Debug/Release | 游戏和执行器构建，已有回归保留 |
| [x] | 根工程仅执行器 | 成功，无游戏/GLAD/GLFW 构建节点 |
| [x] | 根工程仅游戏 | 成功，无 benchmark_core / 执行器产物 |
| [x] | tools/benchmark 独立入口 | 成功，不配置游戏和 assets |
| [x] | Release 安装至中文空格目录 | exe、assets、CRT 正常部署，真实短测成功 |

证据：2026-10-04，`tools/benchmark/tests/verify-boundaries.cmake` 实际配置/构建三种隔离模式并检查 Ninja 图。完整构建产物在 `out/benchmark-native/boundaries`；仅小型证据清单存入文档。

实现位置：根、game、tools/benchmark 三个 CMakeLists；`cmake/ThirdPartyYaml.cmake`、UTF-8 manifests。共享 yaml-cpp 是第三方依赖，不是新建引擎公共基础层。

## 本次变更

无。隔离构建约定已验证并合并到当前设计；便携交付的跨机器限制见使用说明。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| 更多工具复用依赖 | 集中第三方版本/许可证管理，不把它们放进 game 所有权下 |
| 游戏源码按领域拆目标 | 在后续架构节点实施，不能趁执行器需求扩大重构面 |
