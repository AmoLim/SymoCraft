---
tags:
  - area/spec
---

# Module Boundaries

> 2026-10-25：已按用户选择同步 T5 的五项边界增量，用户随后独立授权将对应生产实现及必要构建/测试配套从 E 同步 F，现已完成源码传播。T5/M3 批准、同步检查与保留风险见 [T5 记录](../../changes/archive/2026-10-25-m3-t5-ecs-memory/verification.md)；A26 中旧中低端采样时点由用户最新明确决定覆盖，改为现代性能分析工具接入后执行，设备/预算/未测状态不变，见 [scope](../../../scope/project-scope.md#已确认的目标)。

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

### Requirement: Narrow typed dependency and test access

ECS 的通用 typed 门面 SHALL 允许模板实现所需的受控 EnTT 头依赖，但消费者只需链接正式 ECS target、包含项目公开头，不得自行拼接全 vendor/src 路径或使用原生 Registry/view/ID 逃生口。EnTT SHALL 固定本地官方版本/配置，相关翻译单元及测试一致；CPU-only 和独立 benchmark 不增加图形 SDK 或不相关依赖。必要极端状态测试 SHALL 经模块私有窄适配，只向指定根测试授权，返回自有事实；不公开 NativeRegistry、任意存储修改器或生产依赖 test/support，不使用改变生产布局/配置的测试专用 ECS 副本。

#### Scenario: M3-T5-A21 public consumers and private adapter
- **WHEN** 编译多翻译单元公开消费者、关闭测试、CPU-only、独立工具或尝试越界包含私有测试适配
- **THEN** 生产与测试链接同一正式实现，配置/类型身份一致，公开头不泄露图形 SDK；只有指定测试取得窄头，非法私有访问被拒绝，关闭测试无测试源码/产物依赖

### Requirement: T4-based migration without business drift

T5 SHALL 以确认后的 T4 交付源码与调用边界替换 ECS，迁移必需的装配、遍历、读取、app 残留操作及测试，不因当前目录代码较旧而恢复旧预算/相机权威或要求并行保留双 ECS。T4 的会话借用、自有结果、逐实体候选成功后提交、前序提交保留和 D12 字段/固定阈值 MUST 保持；业务顺序依赖须显式表达，不把 EnTT 原生顺序当行为保证。app 与 ECS 的独立边界改造不属于本 change。

#### Scenario: M3-T5-A22 session adapter and application consumers
- **WHEN** 迁移 T4 的 Session 工厂、角色/物理/Transform 遍历、私有诊断及 app 场景操作
- **THEN** 组件访问符合新契约，物理仍求解成功再提交当前实体；缺失前提安全拒绝，业务初态、时间/输入/编辑时点及结果所有权不漂移，不为通过测试放宽 D12

### Requirement: Contract and operation-sequence verification

T5 SHALL 用契约用例及有界固定种子操作序列验证同一生产 ECS，以独立简单模型核对实体、组件与结果，不以已知有缺陷的旧存储作正确性 oracle。无序查询比较匹配集合而非偶然次序，有序查询比较键序及平局拒绝。真实分配失败与构造抛错 SHALL 覆盖高风险路径并确认触达；普通 MSVC Debug、专用 ABI 配置、禁用与未执行分别报告，不用崩溃/terminate 或未触发故障的绿灯充当恢复通过。成本核对 SHALL 有界、同条件，不预设速度提升或擅自移用 GPU/整机预算。

#### Scenario: M3-T5-A23 reference model failure and configuration evidence
- **WHEN** 执行跨实例/Clear、增删/复用、查询或真实分配失败序列
- **THEN** 每步按逻辑身份比较相应状态和错误，定位首分歧，故障确实进入目标路径；未覆盖类型/配置和全进程 OOM 限制明确，不通过解引用失效对象验证契约

### Requirement: Directed retirement after dependency review

T5 SHALL 按实际源码、target、测试、工具及部署消费者核对旧字节池、iterator、byte API、私有序列化、分配器、线程池和输入残留；准确删除清单报告并获用户确认后才删除。ECS 不再调用旧 allocator 不证明整个 foundation 可删；仍活动的 GL、Camera 或 legacy 命名测试不因名称退役。正式候选 SHALL 无双 ECS、绕过契约的兼容空壳或无用途的活动适配；确需保留的非活动材料须说明用途，不修改个人文件或冻结证据。

#### Scenario: M3-T5-A24 retirement candidate versus authorization
- **WHEN** 发现旧路径无直接调用，或仍有测试/入口/最终链接依赖
- **THEN** 分别形成可复核候选或保留原因，未获准确清单确认不删除；删除后的构建/运行依据绑定新候选，不继承删除前成绩

### Requirement: Layered integration and explicit completion gates

T5 SHALL 分别形成 ECS 契约、T4 调用方、受影响集成与 M3 收口依据。当前候选的受影响 Debug/Release、公开消费者/负向边界、CPU-only、关闭测试、独立工具、已交付现代后端单/双组合及保留 GL 路径 SHALL 按适用影响复验；未交付/缺环境明确受阻，不造空后端或用另一个 API 补格。M3 整体结论 MUST 仍满足 scope 的双现代要求及各自前序批准；T5 局部成功不豁免 T3/T4 未关闭项。人工复查项目与总时长在执行前单独确认，不自动套用 T4 五分钟或追加历史全套/双机/长时。未受影响旧依据只有核对来源与适用性后才可引用，不能自动继承。

#### Scenario: M3-T5-A25 current candidate integration
- **WHEN** 形成 ECS 局部交付或提交 T5/M3 整体验收
- **THEN** 真实 CPU/SDL/GPU、源码包/资源/设备身份、退出与清理结果分别可辨；缺 Vulkan 组合不冒称双现代完成，局部通过、预算判定和用户批准分开

#### Scenario: M3-T5-A26 retained risks and bounded verification scope
- **WHEN** 复用旧依据、安排人工或性能复验，或 OpenSpec 状态为 planning complete
- **THEN** 保留失败/禁用/未执行及长期风险，具体新增执行范围先确认；不自动恢复 Fix1/正式采样，不修改既有预算/负载或下一玩法前中低端门槛，不把规划完整当实施授权或验收

## Rationale

T0 选真实实现库、窄头视图和实图检查，目的是把职责边界变成可检出的工程约束，而非仅移动目录。scene 与 renderer 接口例外来自值契约/私有后端交接的真实需要；不为凑统一 target 形式建立空实现库。T0 不顺带替换 ECS、引入并行、动态区块或新 GPU 后端；后续已经发生的 renderer target 演进按当前源码表达，不把完成的迁移步骤重新列为待实施任务。

历史 M3-T0-B1（world 无 GPU/平台 SDK）、B2（simulation 不读平台/Application、回调不直写 ECS）、B3（renderer 显式输入、不反查 World/Application）、B4（CPU 数据/资源/统计无图形 SDK）、B5（生产无 test/support 依赖）、B6（依赖可解释且无环）身份仍对应上述所有权及依赖契约。它们不是工程页后来归纳的同号条目，引用历史验收时必须带 M3-T0 前缀，不能把同号不同含义混为一次验证。

M3-T0-A06 的全新 D/R 同生产树、A11 的 seed/网格/文件协议、A12 的真实 GL/受影响玩法和图形先于 context 清理、A13 的无关 cwd/缺坏资源/安装检查，继续由[测试验证边界](../../../architecture/engineering.md#测试及验证边界)、[资源安装](../../../architecture/engineering.md#资源与安装)、[world](../world/spec.md)、[platform](../platform/spec.md)及[Benchmark](../../../architecture/benchmark.md)表达，身份及历史结论留 T0 报告，不重新建立 T0 活动步骤或自动授予节点批准。
