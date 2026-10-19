---
tags:
  - area/spec
---

## Context

目标公开行为仅由 [ecs](specs/ecs/spec.md) 与 [module-boundaries delta](specs/module-boundaries/spec.md)维护，范围见 [proposal](proposal.md)，现状见[当前 ECS](../../../architecture/ecs.md)。这里解释目标机制，不把模板头、版本号或规划完成视为已经实现。

F 当前代码仍是 `Registry::Storage` 加字节组件池、进程类型 counter 和 iterator；T4 在另一个候选中补齐了局部注册/分配安全及真实 Session/physics 适配。已只读核对 T4 验证页所指 E 候选的 `session.cpp`、`physics_adapter.cpp`、`player.cpp`、app 装配及根测试：Session 借专属 Registry，物理取值求解后提交，app 仍注册类型、查玩家、读初态及覆盖场景姿态。源码身份由 [T4 verification](../archive/2026-10-19-m3-t4-application-simulation/verification.md#2026-10-15-完整生产迁移)定位，不能把该候选当成本目录已合入的代码。

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

有序查询收集自有 `(key, EntityId)`，只读 keySelector 按相同声明顺序接收只读组件访问；使用 std::less<> 排序，以双向不小于判等价，在任何业务 visitor 前拒绝平局。键为本次快照，不用业务回调改变后的值重排，也不按 native ID 打破平局。不接入 native 池排序、持久 query cache 或可选组件打包。

### 失败与可观察状态

普通参数/状态拒绝、可以确认逻辑一致性的分配/构造失败、用户值更新或 visitor 异常分别处理。不能用 `catch (...) { Failed; }` 把所有普通失败合并，也不能把未知内部一致性失败包装成“可重试”。原始异常是主要原因，bad_alloc 时不要求完整字符串；最小自有状态摘要仅保存无需再分配或扫描池的已知事实和明确未知，不作为通用 Inspect/统计接口。

失败准备分为发布前和实际结构阶段；前者不发布半成品，后者即使逻辑值保留也结束旧借用。组件 ctor 的输入移动/外部副作用不回滚，ECS 不验证组件的游戏规则；T4 是否进入 Session Failed 仍由会话自身契约判断。Failed Registry 不允许借 Clear 原地恢复，但其已取得资源仍由析构回收。未知结构破坏测试只通过合法受控注入到错误分支，不任意破坏内存或触发未定义行为。

### 消费者与退役组织

迁移基线按[T4 交接](../archive/2026-10-19-m3-t4-application-simulation/design.md#消费者交接)和执行时确认的生产源码核对。当前发现的具体适配如下，不复制成另一份任务清单：

| 消费者 | 必要迁移及不得改变的边界 |
| --- | --- |
| `player.cpp`、Session 工厂 | caller 注册、工厂装配；按真实意图选 Emplace/GetOrAdd，显式保持初态，不承诺共享 Registry 装配事务 |
| character/transform 与 `physics_adapter.cpp` | View/Get 改回调引用；shape 等只读，运动值计算成功再提交当前实体，失败之前的实体/子步仍保留 |
| Session 读取与私有 diagnostics | ID/const/异常适配，自有采集不保组件地址，不扩大 public Inspect |
| app 装配与场景 | 保留当前注册/玩家查找/初态读取/模拟前姿态覆盖职责，仅适配 ECS，不新增另一种会话 owner 或操作接口 |
| 根 ECS/simulation/app 测试 | 迁移旧 Free/Clear/重复注册/无效 Has 等明确改变的预期；保留业务断言及 D12 阈值，不按旧 iterator 次序判无序查询 |

旧 `AddComponent` 实际是获取或新增，不能全仓库机械替换成严格 Emplace。检查跨增删持有的引用、传给构造的组件内部借用及旧 EntityId 数字运算；仅必要修正，不顺便改算法。某真实消费者依赖处理次序时，为它显式声明完整业务排序键；不能用 native ID 补齐后宣称确定性，也不因顺序变化悄悄放宽业务对照。

旧 `component_container.h`、`internal.cpp` 的池逻辑、RegistryViewer/Iterator、byte API 和私有 RawMemory 序列化是定向退役候选；准确文件/符号/依赖清单仍须执行时报告并获确认。`Camera.cpp` 及 legacy 命名基线在 E 候选仍有正式 target 或测试消费者，不能按文件名删除。`AmoBase.cpp` 同时含 allocator 与日志等责任，main、foundation 测试和旧故障探针仍有依赖；不因 ECS 改标准分配就整文件删除。旧线程池/输入残留若已在先前清理中不存在，不恢复一份再宣称删除。

### 验证分层与有限成本核对

第一层是 A01-A20 契约与编译期负例，包含非尾/两元素/稀疏高索引删除、带资源及空类型、跨实例/Clear、末代、嵌套异常和有序平局等实际高风险路径。固定种子简单模型逐步对照逻辑状态，不复刻 EnTT 算法，不让旧删除缺陷成为 oracle。极端合法状态通过窄模块私有适配准备，正常入口验证结果，不自然循环耗尽或解引用失效借用。

真实分配故障用隔离测试可执行程序控制实际 standard new/aligned new 等所需路径，关闭故障后再做可能分配的断言/诊断；旧 AmoMemory probe 不再证明新池失败。第一版覆盖代表性的注册、创建、增长、构造、有序暂存/Clear 准备路径，不要求一开始扫描每个分配序号。普通 MSVC Debug 的 iterator proxy 在 noexcept 路径可能终止，须在实际配置核实，不能用关闭全库 iterator debug 的专用结果冒充默认 Debug 可恢复。生产库布局、EnTT 配置不为测试改成另一份；原始 bad_alloc、资源计数、未发布及失败后再次正常操作都需证据。

第二层复用 T4 同一生产 CPU 双装配对照及小值核心/World 测试，不以旧坏 ECS 作基准；按[T4 D12](../archive/2026-10-19-m3-t4-application-simulation/specs/simulation/spec.md#requirement-same-production-cpu-comparison)逐 Advance/World 提交核对，逻辑角色对应、不比原 ID。第三层按受影响范围验证真实构建/SDK隔离/后端组合/部署/玩法及退出；不把每个历史矩阵机械全部重跑，也不以代码或一个 API 代双现代。

成本核对限代表性的注册/创建/增删、普通/有序遍历、增长与清理，记录源码/配置、场景规模、时间/分配/峰值或缺失项。旧实现仅可作为已知正确子集的定性或局部成本对照，不能为性能保留永久双实现。未设 T5 数值预算，不倒定通过线、不自动沿用 T2/T3 GPU 或内存预算，不新增长采样/多机任务。人工项目/总时长与新增正式性能执行范围在执行前确认。

## Decisions

理由、备选与代价集中于 [decisions](decisions.md)。已有 CT01/TH01/UP01/DF01/TK01/BT01/RS01/TH02/QO01 的编号和未来触发条件保持；本轮仅补齐正式收口选择，不把未来候选预建成接口。

## Risks And Compatibility

1. **候选库不等于契约实现。** S01 在全面替换前完成固定源码和 bounded CPU 核对：末代销毁、Clear 身份与准备、空对象/const、多 TU 类型身份、查询 guard、真实失败及标准分配。若必须改上游或放宽已选语义，停止并报告，不默选其他路线。现阶段未冻结精确 commit/配置，也未完成这些实验。
2. **源码分支尚未交接。** T4/E 与 Vulkan 独立候选和 F 当前目录不同；只读核对不是代码合并。全面生产替换前确认输入身份、必要交接与本轮代码根权限，M3 整体验收仍依赖适用的 T3/T4 节点，不用 T5 规划代批。
3. **普通引用没有运行时全保护。** 项目可检查 Registry 入口及查询深度，不能捕获所有隐藏别名/线程/失效借用；使用方及测试不得依赖越界误用被拦截。线程交接仍需结束所有借用和建立同步。
4. **清理与 OOM 证明有边界。** noexcept 类型声明不证明每个 MSVC/allocator 组合都不会实际分配；只有明确配置/路径的测试结果才有效，不宣称全进程 OOM 恢复。销毁次序无保证，资源 owner 的线程/寿命限制独立成立。
5. **收口还有显式执行前置。** 准确退役清单、T5 人工复查项目/总时长及任何新增正式采样范围须确认；这些不阻止形成规划，但未满足时对应执行/整体验收不能通过。结果只维护同一 change 的 verification.md，当前不预建成绩或附件。
