---
tags:
  - area/spec
---

## MODIFIED Requirements

### Requirement: Production ownership and narrow public headers

生产源码 SHALL 归属 `game/modules/` 的 foundation、scene、assets、world、ecs、physics、simulation、platform、renderer、telemetry、窄 profiling 编译适配或 `game/app/`；旧 `tools/benchmark/` 产品退役，不再是活动构建/安装入口。每个启用的生产实现只能有一个真实编译 owner，测试 SHALL 链接正式生产库，不重编译生产实现，也不成为生产依赖。公开头 SHALL 从所属窄 include 根消费，不暴露私有存储、解析器、SDL、图形 SDK 或 Tracy 类型。按 [foundation-files](../../../../specs/foundation-files/spec.md)退役后，Files::Publish 的源码、公开头消费者与 foundation.files 测试注册 SHALL 从活动图移除；其余 foundation 与独立 YAML 适配 owner 保持，不把历史 Fix1 探针变为活动生产依赖。

physics SHALL 由 `game/modules/physics` 唯一拥有 `symocraft_physics` 及其生产源码，公开头根为该模块的 include；simulation 不取得 physics 私有 src，physics 不取得 simulation include/src。公开头测试与独立核心测试 SHALL 通过既有公开 include 链接同一正式库，旧 `game/modules/simulation/physics` 不再是活动编译 owner。

#### Scenario: M3-T0-A01 A02 A04 A09 ownership and consumers
- **WHEN** 配置启用某生产实现，或独立消费者只包含该模块一个公开头
- **THEN** 实现具有唯一生产 owner，消费者不借全仓库、vendor、聚合头或其他模块 src 补依赖；生产不编译或包含 test/support

#### Scenario: Retired source is not an active fallback
- **WHEN** 历史 .disabled 源、旧 GLFW、已退役 benchmark 实现或未授权私有路径被加入活动生产 target
- **THEN** 边界检查 SHALL 具名拒绝，不以文件仍在仓库或冻结包中作为兼容授权

#### Scenario: PHYS-MOVE-A01 formal owner and public consumer coverage
- **WHEN** 在正式 physics 位置构建公开头与独立消费者，或尝试从旧位置、其他 owner 重编译物理实现及跨模块包含私有头
- **THEN** 正向消费者实际编译并链接唯一正式库，公开头发现集合包含原有 physics 头且不为空；重复/错误 owner 和私有访问被具名拒绝，检查不能因仍扫描旧路径或没有匹配文件而伪通过

### Requirement: Acyclic authorized dependencies

模块及 app SHALL 遵守[允许依赖方向](../../../../../architecture/engineering.md#生产所有权与依赖)，生产 target 图 SHALL 无环，模块不反向依赖 app，已退役独立 benchmark 不得成为活动生产依赖或反向复用游戏实现，不再要求保留该产品。PUBLIC SHALL 只表达真实公开头依赖；PRIVATE/最终链接的 `LINK_ONLY` 不得被当作公开头、SDK 或编译选项传播。renderer 的窄私有 backend contract 与 platform 三桥属于明确授权例外，不是开放其他模块私有目录。

simulation SHALL 是唯一允许直接链接 `symocraft_physics` 的生产模块，使用 PRIVATE 依赖；根测试与独立验证消费者可按公开接口链接该正式库。app/其他生产模块 MUST 不因 physics 纳入模块清单而取得直接链接或包含其公开/私有头的权限，也不得通过 alias 或生成器表达式绕过限制。最终游戏经 simulation 静态链接闭包取得 physics SHALL 保持合法，不传播 physics 为 app 的直接公开头依赖。核心自身允许闭包 SHALL 仍限标准库与 foundation 数学，直接或传递引入 World/ECS、其他业务模块、图形 SDK、profiling/Tracy、document_io/YAML 均拒绝。

#### Scenario: M3-T0-A03 actual boundary violations
- **WHEN** 实际 source/link/include 属性或显式 include 引入跨 owner、重复生产实现、测试反向依赖或依赖环
- **THEN** 配置期检查 SHALL 拒绝并标识违规对象，而不是只根据目录名推定隔离

#### Scenario: Current renderer interfaces are explicit exceptions
- **WHEN** 创建 renderer CPU API、CPU STATIC 门面、窄私有 backend contract 或原生后端
- **THEN** 其依赖 SHALL 按[当前分层](../../../../../architecture/engineering.md#renderer-的工程分层)隔离；只有审核过的消费者可取得私有契约，旧 GL 不混入现代 CPU 门面

#### Scenario: PHYS-MOVE-A02 restricted production consumers
- **WHEN** simulation 私有链接正式 physics 并参与游戏最终链接，或 app/World/其他生产模块通过直接链接、alias、生成器表达式尝试消费该库
- **THEN** 前者成功，后者具名拒绝；simulation 将 physics 使用要求无授权公开传播也被拒绝，正式模块清单不扩大 app 的消费权限

#### Scenario: PHYS-MOVE-A03 neutral closure at the new path
- **WHEN** 在 `game/modules/physics` 检查包含路径、实际 target 依赖闭包及 profiling 禁止引用夹具，或配置 CPU-only/关闭测试
- **THEN** 标准库/foundation 数学依赖可用，宽 simulation 路径、非法直接/传递依赖和核心 profiling 引用均被拒绝；CPU-only 不发现或链接图形/Tracy，关闭测试无 fixture 或生产反向依赖，负向夹具确实验证新位置
