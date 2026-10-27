---
tags:
  - area/spec
---

## Context

目标公开行为仅由 [ecs](specs/ecs/spec.md) 与 [module-boundaries delta](specs/module-boundaries/spec.md)维护，范围见 [proposal](proposal.md)，现状见[当前 ECS](../../../../architecture/ecs.md)。这里解释目标机制，不把模板头、版本号或规划完成视为已经实现。

2026-10-19 当前输入：T4 已获本机范围整体验收，规格同步并归档；生产源码及必要构建/测试配套随后按用户明确授权同步至 F 的 feature-refactoring，依据为 [T4 完成批准](../2026-10-19-m3-t4-application-simulation/verification.md#2026-10-19-t4整体完成批准)、[scope](../../../../scope/project-scope.md#m3-t4-完成应用平台与模拟模块接口)和[当前工程状态](../../../../architecture/engineering.md)。归档中早期“仅 E 候选/F 仅 T3”的文字保留其历史时点，不再代表当前输入；源码同步不是 Git 历史合并。

F 当前 ECS 仍是含 T4 限定注册/分配安全修补的旧字节池、进程类型 counter 和 iterator，尚非 EnTT Registry。本轮只读核对 F 的 `session.cpp`、`physics_adapter.cpp` 和 app target：真实 SimulationSession 借专属 Registry，物理小值求解成功后提交当前实体；应用 owner 为 `symocraft_application`，`symocraft_app_runtime` 仅为别名，内部 `symocraft_physics` 位于 simulation/physics。现役行为由 [simulation](../../../specs/simulation/spec.md)、[physics-core](../../../specs/physics-core/spec.md)、[application-lifecycle](../../../specs/application-lifecycle/spec.md)及[telemetry-lifecycle](../../../specs/telemetry-lifecycle/spec.md)维护，不将旧归档的 Purpose 当当前未实现证明。

2026-10-20 用户明确授权后，原 E 实验树已定向取得上游 `6fea31e4631e267f132aa9b1608dc6538b562e13` 的代码/构建/测试输入，4362 项逐字节核对一致；原实验及旧结果保留，Git HEAD 未移动。E 是 T5 正式实施代码根，F 继续持有唯一规划正文，不将后续 T5 改动自动回写主工作区生产代码。输入原配置与计划采用的 DBG01 分开，新配置必须重新构建，具体基线与源身份见 [verification](verification.md#2026-10-20-上游输入同步与正式入口)。

2026-10-18：S01.2/S01.3 的独立前置核对已完成；下方 S01.4 只是在用户确认范围内保存待审候选，不是正式冻结或生产入口。当前实验与生产 Registry 分开，实际依据及未覆盖项见 [verification](verification.md)。

## Technical Approach

### 依赖与 typed 门面

保留 `symocraft_ecs` 的真实生产 owner，Registry 为稳定、不可移动对象。非模板的 owner 身份、状态、查询深度和公共预检尽量放入模块实现；typed 存储访问和查询展开放入项目受控 detail 头。EnTT 只作为实现依赖，不提供 `NativeRegistry()`、原生 ID 或 view 返回值。通用 typed API 使部分第三方模板定义必须对消费者可编译可见；这不是完全隐藏第三方头或承诺稳定二进制 ABI。

采用本地固定官方源码，候选 `v4.0.0`。不 FetchContent 跟踪远端 main、不修改上游源码、不增加 EnTT 自带测试/工具为游戏依赖。使用窄 INTERFACE 依赖 target 传播确实需要的头、C++20 与统一配置，不传播全 vendor 或兄弟模块 src。上游 [CMake](https://github.com/skypjack/entt/blob/v4.0.0/CMakeLists.txt)要求 CMake 3.28；项目可为原始 header-only 源建立窄 target，而不是为了执行上游顶层配置自动提高全工程最低版本。最终具体接入在 S01 核对。

空类型按普通对象处理，优先采用[官方配置](https://github.com/skypjack/entt/blob/v4.0.0/src/entt/config/config.h)关闭 ETO，并保证所有相关翻译单元一致。配置由项目受控入口/target 管理，不允许测试独用另一种空类型存储或宏组合。普通对象和代表性对齐类型的真实构造、搬移、移除及指针访问需 CPU 实验验证；库能编译不等于项目契约成立。

### 身份与生命周期

项目 EntityId 在内部组合不复用的 owner/incarnation token 与 EnTT entity。每次 Registry 构造取得新批次，成功 Clear 换新批次；先比批次再调用 native valid，不按对象地址区分实例。相等/哈希直接比较项目值，不读 owner；不新增一套公开整数句柄或持久 ID 系统。位宽及封装布局不在公开契约冻结。

全局批次分配只有串行使用承诺，耗尽拒绝，不回绕也不为测试倒退以复活旧 ID。若分配准备失败，构造不发布 owner，Clear 尚不改变旧存储。创建、销毁和查询回调产生的 native ID 只通过内部装配转换；组件中的 ID 仍为非拥有值，上层负责引用失效。

销毁先验证身份、原生代数与所有可能失败的回收准备，再移除对象及失效实体。到达不允许递增的末代时拒绝本次销毁，保留仍活实体，而不是绕回或偷偷 Clear。需要核实选定 native entity traits、保留值与 `destroy`/回收路径是否会分配；不能只用一个大整数相加代替边界核对。

Clear 的准备顺序是校验状态/查询上下文、准备新批次，再原生清理，成功后提交新批次；注册表和名称不清空。EnTT 的 [clear 实现](https://github.com/skypjack/entt/blob/v4.0.0/src/entt/entity/registry.hpp)逐池清理后移除实体，不替项目保证跨 Clear 身份或全部准备边界。中途若出现无法确认的一致性故障，按 Failed 停止，不能承诺任意半清理事务。析构仅释放已取得 owner，无须为终结对象再次申请身份。资源对象必须满足不抛析构和禁止重入，不保证跨池/跨实体析构次序。

### 类型注册与正常对象

注册表属于每个 Registry，保存真实 C++ 类型身份、底层默认键、首次自有诊断名和必要 typed 操作元数据。元数据不对外提供反射。名称、注册索引及必要存储先准备，全部成功才发布注册标记；底层留有不可见的空容量不等于已注册。纯注册状态查询不走会隐式 assure 新组件池的路径。

沿用默认类型键，在访问/创建池前检查它是否已被不同真实类型占用。EnTT [type_info](https://github.com/skypjack/entt/blob/v4.0.0/src/entt/core/type_info.hpp)的相等不能作为独立碰撞证明；真实类型身份机制在 S01 确认，不在本计划静默选定 RTTI 或以调试名代替。默认键、独立类型身份和项目 EntityId 哈希是不同概念。

在门面处理合法模板形式及 const 归一化，非法引用/volatile、重复底层查询类型和必需/排除重叠在实例化期拒绝。注册/结构操作仅用裸 T，读取和必需查询中的 const T 只影响引用权限；const Registry 的路径不得经可写访问泄漏 T&。底层正常对象存储运行构造/移动/析构，严格新增与默认获取或新增使用不同前提，不复制旧清零池行为。

默认标准分配和自动增长，不接 AmoMemory，不新增 allocator 参数、公开 Reserve 或内存预算框架。组件内部容器的分配独立存在。新增准备失败保留已有合法逻辑值，但即使地址偶然没变，也不恢复此前跨结构阶段的借用；构造输入的自有化由调用方负责。

### 统一入口与查询保护

所有受控普通入口先 Failed、再调用上下文、最后身份/注册/操作前提，同层多错无固定顺序。读取类入口与结构入口分别复用公共检查，禁止通过 GetOrAdd 已存在等分支绕开活动查询检查。IsEntityValid 和注册状态查询是有定义的正常否定出口，不把 Failed 伪装为 false。

查询以 RAII 深度 guard 维护同步嵌套；从筛选准备到最后 visitor 结束都在保护期。有序查询从收集键之前即取得 guard，避免准备阶段被重入结构修改。外层存活时内层 Stop/异常不能释放外层保护；组件构造/移动/析构重入不属于允许的同步嵌套查询。可控入口强制检查不等于能拦截任意旧 T& 写入、隐藏别名或并发误用。

普通查询用已验证注册的 typed 池或局部 native view 驱动，直接调用项目 visitor，参数始终为项目 EntityId 加模板顺序引用；不把 native each 对空类型、最小池或回调签名的选择传播出去。无必需类型时走实体存储并按排除条件过滤，不能因此遗漏无组件实体。Stop/Completed 由当前查询栈帧决定，异常不改写此前值修改。

表达方向如下，不冻结最终 overload、include 布局或辅助类型名称：

```cpp
registry.Each<Transform, const Velocity>(visitor);
registry.Each<Transform, const Velocity>(Exclude<Disabled>{}, visitor);
registry.EachOrdered<Transform, const Velocity>(
    Exclude<Disabled>{}, keySelector, visitor);
```

本段仍解释查询语义；本次提交审阅的重载范围见下方 S01.4 候选，尚未因保存草案而冻结。

有序查询收集自有 `(key, EntityId)`，只读 keySelector 按相同声明顺序接收只读组件访问；使用 std::less<> 排序，以双向不小于判等价，在任何业务 visitor 前拒绝平局。键为本次快照，不用业务回调改变后的值重排，也不按 native ID 打破平局。不接入 native 池排序、持久 query cache 或可选组件打包。

### 失败与可观察状态

普通参数/状态拒绝、可以确认逻辑一致性的分配/构造失败、用户值更新或 visitor 异常分别处理。不能用 `catch (...) { Failed; }` 把所有普通失败合并，也不能把未知内部一致性失败包装成“可重试”。原始异常是主要原因，bad_alloc 时不要求完整字符串；最小自有状态摘要仅保存无需再分配或扫描池的已知事实和明确未知，不作为通用 Inspect/统计接口。

失败准备分为发布前和实际结构阶段；前者不发布半成品，后者即使逻辑值保留也结束旧借用。组件 ctor 的输入移动/外部副作用不回滚，ECS 不验证组件的游戏规则；T4 是否进入 Session Failed 仍由会话自身契约判断。Failed Registry 不允许借 Clear 原地恢复，但其已取得资源仍由析构回收。未知结构破坏测试只通过合法受控注入到错误分支，不任意破坏内存或触发未定义行为。

### 消费者与退役组织

迁移基线为 F 已同步的 T4 生产源码，沿用[T4 交接](../2026-10-19-m3-t4-application-simulation/design.md#消费者交接)，并在 S01.1 绑定 T5 实际采用输入。下表保留既有消费者核对所得的必要适配方向，正式迁移仍须逐处复核，不复制成另一份任务清单：

| 消费者 | 必要迁移及不得改变的边界 |
| --- | --- |
| `player.cpp`、Session 工厂 | caller 注册、工厂装配；按真实意图选 Emplace/GetOrAdd，显式保持初态，不承诺共享 Registry 装配事务 |
| character/transform 与 `physics_adapter.cpp` | View/Get 改回调引用；shape 等只读，运动值计算成功再提交当前实体，失败之前的实体/子步仍保留 |
| Session 读取与私有 diagnostics | ID/const/异常适配，自有采集不保组件地址，不扩大 public Inspect |
| app 装配与场景 | 保留当前注册/玩家查找/初态读取/模拟前姿态覆盖职责，仅适配 ECS，不新增另一种会话 owner 或操作接口 |
| 根 ECS/simulation/app 测试 | 迁移旧 Free/Clear/重复注册/无效 Has 等明确改变的预期；保留业务断言及 D12 阈值，不按旧 iterator 次序判无序查询 |

旧 `AddComponent` 实际是获取或新增，不能全仓库机械替换成严格 Emplace。检查跨增删持有的引用、传给构造的组件内部借用及旧 EntityId 数字运算；仅必要修正，不顺便改算法。某真实消费者依赖处理次序时，为它显式声明完整业务排序键；不能用 native ID 补齐后宣称确定性，也不因顺序变化悄悄放宽业务对照。

#### 物理步进顺序

2026-10-20 调用方复核确认 Session 的既有部分提交夹具依赖玩家先于后续坏实体求解，不能直接继承 EnTT 的池次序。用户明确同意显式业务顺序键并要求先更新设计。本项细化既有 A17/A22 的完整键/调用方迁移，沿用[现役部分推进失败与 D12](../../../specs/simulation/spec.md#requirement-rejection-and-partial-advance-failure)、[核心 D04 边界](../../../specs/physics-core/spec.md#requirement-borrowed-query-and-per-body-result)，不修改 T4 归档或现役规格，不扩大物理核心职责。

simulation 的普通值组件 `PhysicsStepOrder` 保存装配方显式提供的 uint64 顺序键；它不是 EntityId、存档身份或 Registry 自动签发序号，不按原生实体、地址、默认池顺序或当前运动值补序。专属 Session 玩家装配显式使用键 0，额外刚体由其装配方使用其他唯一键；调用方负责注册该组件与本步完整刚体集合中的键唯一性。键可由业务确定，不为此新增全局分配器/持久排序表，不能靠默认构造多个键 0 当合法装配。

`physics_adapter` 的匹配集合仍是 Transform/RigidBody/HitBox，不把顺序组件加到筛选中而静默遗漏缺键刚体。用 EachOrdered 的只读键生成读取每个匹配体的 PhysicsStepOrder，在任何本步物理 visitor 前完成键准备、排序和平局检查；缺注册/缺键、平局、键暂存分配或比较失败均停止本步，物理实体提交数为零。此前已执行角色规则/预算或前序子步不因此回滚，Session 仍按实际阶段报告 AdvanceFailed，不伪装为时间/输入预检 Rejected。

按键逐体取小值、求解成功后才提交本体，不提前校验所有运动/形状来抹去合法的前序部分提交；排序只改变显式顺序依据，不改变求解算法、D12 字段/固定阈值或跨实体回滚边界。每子步键是自有快照；装配方不得借回调改键来重排本轮。配套验证覆盖反向装配/原生池重排后仍按业务键提交、缺键/平局零本步提交、后续坏体保前序提交及同一生产 CPU 双装配对照；有序准备的有界成本纳入既有 S04.5，不由此授权正式性能采样。

旧 `component_container.h`、`internal.cpp` 的池逻辑、RegistryViewer/Iterator、byte API 和私有 RawMemory 序列化是定向退役候选；准确文件/符号/依赖清单仍须执行时报告并获确认。此前在 T4 候选核到 `Camera.cpp` 及 legacy 命名基线的正式 target 或测试消费者，S05.1 须在 F 当前源码复核，不能按文件名删除。`AmoBase.cpp` 同时含 allocator 与日志等责任，main、foundation 测试和旧故障探针仍有依赖；不因 ECS 改标准分配就整文件删除。旧线程池/输入残留若已在先前清理中不存在，不恢复一份再宣称删除。

### 验证分层与有限成本核对

第一层是 A01-A20 契约与编译期负例，包含非尾/两元素/稀疏高索引删除、带资源及空类型、跨实例/Clear、末代、嵌套异常和有序平局等实际高风险路径。固定种子简单模型逐步对照逻辑状态，不复刻 EnTT 算法，不让旧删除缺陷成为 oracle。极端合法状态通过窄模块私有适配准备，正常入口验证结果，不自然循环耗尽或解引用失效借用。

真实分配故障用隔离测试可执行程序控制实际 standard new/aligned new 等所需路径，关闭故障后再做可能分配的断言/诊断；旧 AmoMemory probe 不再证明新池失败。第一版覆盖代表性的注册、创建、增长、构造、有序暂存/Clear 准备路径，不要求一开始扫描每个分配序号。普通 MSVC Debug 的 iterator proxy 在 noexcept 路径可能终止，须在实际配置核实，不能用关闭全库 iterator debug 的专用结果冒充默认 Debug 可恢复。生产库布局、EnTT 配置不为测试改成另一份；原始 bad_alloc、资源计数、未发布及失败后再次正常操作都需证据。

第二层复用 T4 同一生产 CPU 双装配对照及小值核心/World 测试，不以旧坏 ECS 作基准；按已同步的[现役 D12 契约](../../../specs/simulation/spec.md#requirement-same-production-cpu-comparison)逐 Advance/World 提交核对，逻辑角色对应、不比原 ID。第三层按受影响范围验证真实构建/SDK隔离/后端组合/部署/玩法及退出；不把每个历史矩阵机械全部重跑，也不以代码或一个 API 代双现代。

成本核对限代表性的注册/创建/增删、普通/有序遍历、增长与清理，记录源码/配置、场景规模、时间/分配/峰值或缺失项。旧实现仅可作为已知正确子集的定性或局部成本对照，不能为性能保留永久双实现。未设 T5 数值预算，不倒定通过线、不自动沿用 T2/T3 GPU 或内存预算，不新增长采样/多机任务。人工项目/总时长与新增正式性能执行范围在执行前确认。

### S01.4 待审冻结候选

本节标题保留原待审锚点供历史链接使用。2026-10-20 用户明确确认“确认冻结，按此进入正式替换”，冻结现有依赖/配置与本设计，正式入口另按同日明确授权登记；权威节点见 [VERIFICATION](../../../../verification/VERIFICATION.md#关键节点)。目标语义仍以两份 delta spec 为准，不重选 D01、CT01/TH01/UP01/DF01/TK01/BT01/RS01/TH02/QO01。冻结不等于生产实现或契约验收，后续机制冲突/范围扩大仍先报告。

#### 依赖与构建身份

| 项目 | 提交审阅的准确候选及边界 |
| --- | --- |
| EnTT | 官方 v4.0.0，完整 commit `85c6bba014049b5de8fad49d25424df2f1f6a8c1`；tree/源码归档、本地 src 与原始 LICENSE 摘要唯一见 [依赖核对](verification.md#依赖身份与接入条件)。不以本机换行相关摘要冒充官方压缩包摘要 |
| 本地接入 | 候选落点 `vendor/entt`，保留官方 `src/entt` 结构及原始许可；不引用 `out/experiments` 作为生产输入，不 FetchContent 或执行上游顶层工程，不自动提高项目 CMake 3.22 门槛。实际实验使用 CMake 4.3.5，不声称已用 3.22 二进制验证 |
| 窄依赖 | 建单独 EnTT INTERFACE target，仅暴露其 `src` 头根、C++20 与配置，由正式 ECS 的真实模板需求传递；消费者只链接 ECS。现有 `symocraft_header_view` 会排除 `/src/` 路径，不直接套它导致 EnTT 头缺失，也不为此全局解除原过滤。ECS 的第三方 target 白名单须定向更新，不开放全 vendor/兄弟私有目录 |
| EnTT 配置 | 定义 `ENTT_NO_ETO`；默认 uint32 entity/type key、4096/1024 sparse/packed page、标准 allocator、异常开启及默认 mixin。不定义 `ENTT_NO_EXCEPTION`、`ENTT_STANDARD_CPP`、`ENTT_USE_ATOMIC`、`ENTT_NO_MIXIN` 或自定义 STL，不接受未审核的 `entt/ext/config.h` 覆盖；不接 AmoMemory |
| MSVC | 已验证工具集目录 `14.51.36231`、实际编译器 `19.51.36260.0`、Windows x64/C++20、SDK `10.0.22621.0`，与已交付 T4 及 F 当前 CLI 基线一致。这不是最低兼容版本承诺或 SDK/DXC 等连带升级授权，也不证明 DBG01 已传播；最终交付身份及统一 ABI 仍须与 S01.1 采用输入相符 |
| Debug ABI | 沿用已批准 [DBG01](decisions.md#m3-t5-dbg01-统一-debug-abi-配置)：level 0，保留 `/MDd /Zi /Od /RTC1`；Release `/MD /O2 /DNDEBUG`、level 0。相关 C++ 库、公开消费者与测试统一配置并重建，不只给 ECS/EnTT 下游局部设宏，也不在头内抢先改写已生效 STL 配置 |
| 配置拒绝与通知 | 受控入口核对版本/ETO/关键 traits 与 ABI，不能只依赖上游 detect_mismatch（其未覆盖 ETO）。安装按现有 `licenses/` 规则增加原始 MIT 版权/许可及来源；不从实验 clone 干净推定正式 vendor、包或许可部署已完成 |

#### 身份、创建和提交机制

推荐用一个进程内、跨 Registry 外部串行的非零 uint64 单调批次，同时区分 owner 及每次成功 Clear；签发最大值后进入永久耗尽状态，避免自增回绕。EntityId 私有保存批次及 native 实体值，默认批次 0 为空；相等/哈希不读 Registry，只公开相等与 `std::hash<EntityId>`，不公开字段、整数转换或排序，也不冻结 padding/二进制布局。身份源在正式 ECS 实现中唯一拥有，不复制到每个 TU；不承诺 DLL/插件或跨进程身份。

按值保存及准备批次，不为这一个整数必设堆 owner。S01.3 的 `unique_ptr<Batch>` 只是验证准备先于清理的实验路径，不是每次 Clear 分配 8 字节的设计依据。按值组合的耗尽拒绝、准备/提交及构造展开已有[独立实测](verification.md#2026-10-19-按值批次与创建高水位组合核对)，生产实现仍须按自身实际路径测试；不把原堆准备的 OOM 成绩转移过来。确无分配的准备点记为不发生分配，不模拟恢复通过，A06 保持。

每 Registry 保留一个内部 fresh-index 高水位，而非第二套槽映射。有回收槽时只走 native 回收；否则先拒绝 `next_fresh >= 0xFFFFF`，以明确 hint 创建下一新槽，仅成功后推进高水位；不依赖原生 `next()` 的断言、占用实体数量或故障后继续扫描来维护容量边界。合法新索引为 0..1048574；保留原生版本 4095，销毁前拒绝版本 4094，允许 4093 正常递增至 4094。异常后不能确认实际存储一致性时进入 Failed，不猜测高水位回滚或继续签发。

**Clear 与高水位必须一起处理。** 固定候选的 `registry.clear()` 清组件后对实体存储调用 erase，保留已取得实体槽以供回收，而非清空整个实体存储。成功 Clear 更换项目批次、保留注册/名称及 fresh-index 高水位；不能将高水位置 0，与保留槽形成 hint 冲突。新身份先准备、原生清理成功后才提交；一致性不可确认则 Failed，已消耗批次不回收，析构不申请新身份。2026-10-19 的独立组合及故意清零反例已取证，不能据此宣称尚未实现的生产 Registry 通过。

类型检查推荐复用实验的独立 inline 类型 token 地址，裸 T 归一化仅发生于允许 const 的入口；以真实 token 查重复，以默认 native key 查不同类型碰撞。限当前同一链接域的跨 TU 使用，不依赖 native type_info 的哈希相等或诊断名，配置变化/动态模块不自动沿用证据。名称副本、元数据容量和必要 native 空池先准备，最后通过已准备容量及不抛转移发布注册标记；留下空容量/空池不等于注册成功。真实对象存储及类型的移动/析构约束不变。

#### 必要公开重载候选

保留语义合适的旧名称以减少无意义迁移，但不留旧行为别名或两套门面。下表只确定需审核的入口族和返回/const 方向，不预写生产声明或逐函数学习说明：

| 入口族 | 候选表面 |
| --- | --- |
| owner 与实体 | 不可复制/移动的 Registry，构造可失败、析构 noexcept；保留 `CreateEntity()`、`DestroyEntity(EntityId)`、`Clear()`、`IsEntityValid(EntityId) const`，计数 `NumComponents(EntityId) const` 返回 `std::size_t` |
| 注册 | `RegisterComponent<T>(std::string_view)` 加 `const char*` 重载，返回 void；纯读取 `IsComponentRegistered<T>() const` 返回 bool，仅接受裸 T。指针重载先做统一状态/查询检查，再检查 null、构造 view，不在预检前对空指针求长度；重复注册同样检查名称 |
| 组件结构 | `Emplace<T>(EntityId, Args&&...)` 与仅默认可构造的 `GetOrAdd<T>(EntityId)` 返回 T&；保留 `RemoveComponent<T>(EntityId)` 名称、改为 bool 表达是否移除。三者只接受裸 T，不保留语义含混的 `AddComponent` 别名 |
| 读取 | `GetComponent<T>`、新增 `TryGetComponent<T>` 有 mutable/const Registry 重载，分别给引用/指针；mutable 的 const T 只读，const Registry 的 T 或 const T 均只读。保留 `HasComponent<T>(EntityId) const`、仅裸 T；非法实体仍拒绝，正常缺失按入口区分 |
| 同步查询 | `Each<Required...>(visitor)` / `(Exclude<Excluded...>, visitor)`；`EachOrdered<Required...>(keySelector, visitor)` / `(Exclude<Excluded...>, keySelector, visitor)`，各有 const Registry 版本。统一返回 `QueryResult::Completed/Stopped`，visitor 只接受 void 或 `VisitControl::Continue/Stop`；const Registry 强制只读，keySelector 始终只读 |
| 最小状态 | `GetStatus() const noexcept` 返回最小自有 RegistryStatus：健康/Failed、已知操作/类别及必要事实的明确已知/未知标记，无存储扫描、动态字符串或异常对象借用。不开放一般 Inspect、名称查找/反射或统计接口；契约拒绝用项目 RegistryError 类别，bad_alloc/用户异常保留原原因 |

EntityId 的默认空值即可用于初态；是否保留既有 `null_entity` 的 constexpr 值名称属于兼容表面核对，不保留可写全局身份权威。查询模板非法修饰、底层重复、必需/排除重叠和错误 visitor 返回在实例化期拒绝，不用删除有效 const 用法来简化重载。`GetComponent`、`TryGetComponent`、`GetStatus` 等名字及完整签名仍是候选，未由草稿存在推定已可编译或生产消费者已迁移。

统一入口仍先 Failed、再调用上下文、最后身份/注册/参数；所有结构入口含无变化路径先查 guard。查询保护覆盖筛选/键生成/暂存/排序/平局检查至本层 visitor 完成；不捕获 visitor 异常后假装回滚值，不把任意 bad_alloc 一律标 Failed 或一律可恢复。受控一致性错误分支只验证状态策略，不声称可以恢复任意内存损坏。

#### 冻结依据与实施前置

- 按值批次/高水位组合已取得有界实测及错误清零反例，见[组合核对](verification.md#2026-10-19-按值批次与创建高水位组合核对)；生产身份、边界与失败保证仍按正式实现复验。
- const/名称/查询重载已有正例实例化、运行及每配置 36 个预期编译拒绝，见[接口核对](verification.md#2026-10-19-公开重载候选定向核对)。冻结此接口方向不把原型当生产门面，其他完整对象/模板契约仍需生产测试。
- S01.1 已绑定上游源码、实际工具链与 E 代码根，本轮正式工作授权及设计冻结分别记录。T3/T4 不重复验收；替换后的 CPU/消费者/图形/部署等结果不继承旧成绩，删除、额外采样和人工执行范围仍各自确认。

按同日明确授权开始 S02 起的正式实现，不包含其他工作树合并、未确认的性能/人工矩阵或删除。已批准 DBG01 不重开，发现新冲突仍先报告；验证正文只维护同一 verification.md，不新增冻结包、API 清单文档或第二活动计划。

## Decisions

理由、备选与代价集中于 [decisions](decisions.md)。已有 CT01/TH01/UP01/DF01/TK01/BT01/RS01/TH02/QO01 的编号和未来触发条件保持；本轮仅补齐正式收口选择，不把未来候选预建成接口。

## Risks And Compatibility

1. **冻结不等于契约实现。** S01 已完成输入、独立核对和明确冻结/入口，尚未交付新 Registry。必须改上游或放宽已选语义时停止报告，不默选其他路线。
2. **同步不继承验证成绩。** T3/T4 已批准归档，E 已定向取得 F 上游代码，但并非 Git HEAD 已切换，精确身份以同步清单为准。后续 DBG01 和 Registry 变更均使用新构建目录/结果，不复用旧 ABI 缓存，不用输入基线代替替换后验证。
3. **普通引用没有运行时全保护。** 项目可检查 Registry 入口及查询深度，不能捕获所有隐藏别名/线程/失效借用；使用方及测试不得依赖越界误用被拦截。线程交接仍需结束所有借用和建立同步。
4. **清理与 OOM 证明有边界。** noexcept 类型声明不证明每个 MSVC/allocator 组合都不会实际分配；只有明确配置/路径的测试结果才有效，不宣称全进程 OOM 恢复。销毁次序无保证，资源 owner 的线程/寿命限制独立成立。
5. **收口还有显式执行前置。** 准确退役清单、T5 人工复查项目/总时长及任何新增正式采样范围须确认；这些不阻止形成规划，但未满足时对应执行/整体验收不能通过。结果只维护同一 change 的 verification.md，当前不预建成绩或附件。
