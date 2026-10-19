---
tags:
  - area/spec
---

# Module Boundaries

## Purpose

维护已实施的生产所有权、构建隔离与公开头边界。机制及当前 B1-B6 不变量只在[工程级契约](../../../architecture/engineering.md#稳定不变量)维护；本页不重建 T0 活动任务。

本基线核对[根配置](../../../../CMakeLists.txt)、[模块组织](../../../../game/CMakeLists.txt)及[边界检查器](../../../../cmake/ModuleBoundaries.cmake)，并消费 [T0 历史验证](../../../verification/VERIFICATION.md#退役材料恢复)和后续 renderer target 的实际演进。原 T0 学习来源已列待授权候选，点时验证仍由上述报告定位。本轮没有执行配置、构建、CTest 或 GPU 验证，旧 target 数和旧测试数量不是当前结果。

## Requirements

### Requirement: Production ownership and narrow public headers

生产源码 SHALL 归属 `game/modules/` 的 foundation、scene、assets、world、ecs、simulation、platform、renderer、telemetry 或 `game/app/`；独立工具 SHALL 归属 `tools/benchmark/`。每个启用的生产实现只能有一个真实编译 owner，测试 SHALL 链接正式生产库，不重编译生产实现，也不成为生产依赖。公开头 SHALL 从所属窄 `include/` 根消费，不暴露私有存储、解析器、SDL 或图形 SDK。

#### Scenario: M3-T0-A01 A02 A04 A09 ownership and consumers

- **WHEN** 配置启用某生产实现，或独立消费者只包含该模块一个公开头
- **THEN** 实现具有唯一生产 owner，消费者不借全仓库、vendor、聚合头或其他模块 `src/` 补依赖；生产不编译或包含 test/support

#### Scenario: Retired source is not an active fallback

- **WHEN** 历史 `.disabled` 源、旧 GLFW 或未授权私有路径被加入活动生产 target
- **THEN** 边界检查 SHALL 具名拒绝，不以文件仍在仓库中作为兼容授权

### Requirement: Acyclic authorized dependencies

模块及 app SHALL 遵守[允许依赖方向](../../../architecture/engineering.md#生产所有权与依赖)，生产 target 图 SHALL 无环，模块不反向依赖 app，game 与独立 benchmark 不互相链接或包含实现。PUBLIC SHALL 只表达真实公开头依赖；PRIVATE/最终链接的 `LINK_ONLY` 不得被当作公开头、SDK 或编译选项传播。renderer 的窄私有 backend contract 与 platform 三桥属于明确授权例外，不是开放其他模块私有目录。

#### Scenario: M3-T0-A03 actual boundary violations

- **WHEN** 实际 source/link/include 属性或显式 include 引入跨 owner、重复生产实现、测试反向依赖或依赖环
- **THEN** 配置期检查 SHALL 拒绝并标识违规对象，而不是只根据目录名推定隔离

#### Scenario: Current renderer interfaces are explicit exceptions

- **WHEN** 创建 renderer CPU API、CPU STATIC 门面、窄私有 backend contract 或原生后端
- **THEN** 其依赖 SHALL 按[当前分层](../../../architecture/engineering.md#renderer-的工程分层)隔离；只有审核过的消费者可取得私有契约，旧 GL 不混入现代 CPU 门面

### Requirement: Product and configuration isolation

CPU-only SHALL 不发现、配置或链接 SDL、GLAD、platform 或原生图形后端；它仍可包含 renderer CPU 门面及 app CPU 转接，不沿用 T0 时“没有 renderer target”的过时描述。独立 benchmark SHALL 不创建游戏模块或加载游戏资产规则。`BUILD_TESTING=OFF` SHALL 不注册测试及 fixture、不安装其产物。模式选项的首次默认值与缓存后独立状态 SHALL 按[配置组合](../../../architecture/engineering.md#配置组合)解释。

#### Scenario: M3-T0-A05 CPU-only build graph

- **WHEN** `SYMOCRAFT_CPU_ONLY=ON`
- **THEN** 当前配置局部强制 GAME/BENCHMARK/D3D12 OFF、CPU_MODULES ON，CPU 图形隔离不被解释成全部 renderer 或必要源资产消失

#### Scenario: M3-T0-A07 A08 separate products

- **WHEN** 关闭测试，或从 `tools/benchmark` 独立配置工具
- **THEN** 产品不依赖测试源码/fixture；独立工具不依赖游戏、窗口或图形运行时

### Requirement: Boundary checks have bounded proof

配置期检查 SHALL 读取实际 target 属性并扫描明确 include，拒绝宏生成 include；其输出 SHALL 绑定本次构建目录。配置成功不得替代真实公开头消费者编译、负向测试、增量验证、安装及运行。检查器不是完整 C++ 解析器或安全沙箱，失败中的部分输出不保证回滚。

#### Scenario: M3-T0-A02 A03 A10 check versus execution

- **WHEN** 静态检查或 CMake 配置成功，但消费者编译、负向用例、增量或运行尚未执行
- **THEN** SHALL 分别报告未执行范围，不从配置成功或历史 T0 成绩推定当前通过

## Rationale

T0 选真实实现库、窄头视图和实图检查，目的是把职责边界变成可检出的工程约束，而非仅移动目录。scene 与 renderer 接口例外来自值契约/私有后端交接的真实需要；不为凑统一 target 形式建立空实现库。T0 不顺带替换 ECS、引入并行、动态区块或新 GPU 后端；后续已经发生的 renderer target 演进按当前源码表达，不把完成的迁移步骤重新列为待实施任务。

历史 M3-T0-B1（world 无 GPU/平台 SDK）、B2（simulation 不读平台/Application、回调不直写 ECS）、B3（renderer 显式输入、不反查 World/Application）、B4（CPU 数据/资源/统计无图形 SDK）、B5（生产无 test/support 依赖）、B6（依赖可解释且无环）身份仍对应上述所有权及依赖契约。它们不是工程页后来归纳的同号条目，引用历史验收时必须带 M3-T0 前缀，不能把同号不同含义混为一次验证。

M3-T0-A06 的全新 D/R 同生产树、A11 的 seed/网格/文件协议、A12 的真实 GL/受影响玩法和图形先于 context 清理、A13 的无关 cwd/缺坏资源/安装检查，继续由[测试验证边界](../../../architecture/engineering.md#测试及验证边界)、[资源安装](../../../architecture/engineering.md#资源与安装)、[world](../world/spec.md)、[platform](../platform/spec.md)及[Benchmark](../../../architecture/benchmark.md)表达，身份及历史结论留 T0 报告，不重新建立 T0 活动步骤或自动授予节点批准。
