---
tags:
  - area/architecture
---

# ECS

`symocraft_ecs` 拥有实体及组件对象，不拥有游戏规则、World 或系统调度。公开工程基线见 [module-boundaries](../openspec/specs/module-boundaries/spec.md)。2026-10-25 用户在 T5/M3 验收及归档后独立授权同步生产代码，F 主工作区已取得 `E:/Codex/Worktrees/refactor-ecs-memory/Symocraft` 的已验收 EnTT 门面及必要配套。以下 T5 机制现已适用于 F；旧实现章节仅解释历史依据，不是第二活动 ECS。

T5 已同步 [ecs 主规格](../openspec/specs/ecs/spec.md)并归档；原 [ecs delta](../openspec/changes/archive/2026-10-25-m3-t5-ecs-memory/specs/ecs/spec.md)保留历史来源，实际结果、整体批准及后续 F 同步检查在 [T5 verification](../openspec/changes/archive/2026-10-25-m3-t5-ecs-memory/verification.md)。规格同步、验收、生产传播及 Git 历史分别记账，不将未测风险改为通过。

## T5 实现机制

本节现行源码为 [EntityId](../../game/modules/ecs/include/symocraft/ecs/entity.h)、[Registry](../../game/modules/ecs/include/symocraft/ecs/registry.h)、[typed 实现](../../game/modules/ecs/include/symocraft/ecs/detail/registry.inl)、[配置](../../game/modules/ecs/include/symocraft/ecs/detail/entt_config.h)及[非模板实现](../../game/modules/ecs/src/registry.cpp)。F 已同步这些已验收字节；主工作区新检查与 E 的历史运行成绩分开。

### Owner、对象与类型

Registry 不可复制/移动，`unique_ptr<Storage>` 独占 native Registry、注册元数据、批次/创建高水位和访问状态。析构先进入生命周期修改上下文，再由成员 RAII 清理；无公开 Free、byte、view/iterator、原生 ID/Registry 入口。构造可失败，析构不申请身份批次，不要求跨组件/实体的清理次序。

使用本地固定 EnTT v4.0.0、标准分配和 `ENTT_NO_ETO`，空组件也有真实对象。普通构造可抛，合法类型要求不抛移动构造、移动赋值和析构，生命周期操作不得重入所属 Registry；不是只接受 POD，也不以清零字节替代初始化。无公开 allocator/Reserve 或旧 256 类型限制。

每 Registry 显式注册并拥有首次非空名称；同类型重复注册幂等，但仍检查名称。inline 类型 token 独立辨认真实 C++ 类型，以默认 native key 检查不同类型碰撞；名称不作身份，token 只承诺同一链接域跨 TU，不承诺 DLL/插件或持久化。先准备名称、元数据容量及 native 空池，再无分配发布；准备失败允许留下空容量，不等于半注册。

注册、结构操作、Has/注册状态与 Exclude 仅接受裸 T；Get/TryGet 和必需查询可用 T/const T，共用同一存储。const Registry 只返回只读借用，不继承旧 const 可写路径。Emplace 严格新增，GetOrAdd 仅支持默认构造且已有时返回原对象；Remove 对合法身份/注册下的缺失返回 false，移除最后组件不销毁实体。读取缺失分别为异常/false/nullptr，非法 ID 或未注册不伪装成缺失；NumComponents 只数挂载种类，不是池容量统计。

### 项目身份与 Clear

EntityId 私有保存 uint64 批次和 uint32 native 值；默认空，公开相等/哈希只读自身，不延长实体/Registry 寿命，无排序、整数转换或跨进程承诺。进程内单调批次同时区分 owner 与 Clear 身份，最大值只签发一次，耗尽永久拒绝。入口先比项目批次再查 native valid，不靠地址或索引识别；其他组件内的 ID 不自动级联清理。

Create 有回收槽时走 native 回收，否则以私有 fresh 高水位为 hint，校验真实结果恰好等于请求且有效，成功才前进。合法 fresh index 为 0..1048574；拒绝 0xFFFFF 保留索引，不靠计数、断言或 hint 被忽略后的偶然结果补序。Destroy 在任何组件移除前拒绝版本 4094，允许 4093 正常递增至 4094；非法/重复销毁具名拒绝，不回绕或暗中 Clear。

Clear 先准备新批次，native clear 成功后才发布，注册/首次名称保留。native 实体池保留可回收槽，因此 fresh 高水位也保留，不清零；空/重复 Clear 仍更换批次。批次准备不分配，耗尽拒绝保旧实体/ID/组件；已消耗批次不退回，析构不需要新的批次。一致性不明的实际结构异常转 Failed，不承诺半清理事务或原地恢复。

### 借用、查询与同步

组件引用只短借：实际进入结构存储修改即结束旧借用，即使失败且旧逻辑值保留也须按 ID 重取；纯预检拒绝或未触碰结构的 no-op 不虚构失效。构造输入须事先真正自有，不能把同 Registry 的组件/成员借用跨入新增。查询引用仅保证当前回调，值修改/同步嵌套本身不是结构修改；普通 T&、隐藏别名或带借用成员的值键不能被运行时全面保护。

普通入口先 Failed，再生命周期/查询上下文，再身份/注册/操作前提，同层多错不保证细序。MutationGuard 覆盖实际结构工作并拒绝生命周期重入；QueryGuard 从筛选准备持续到 visitor 结束，查询深度按栈恢复。所有结构入口包含重复注册、已有 GetOrAdd、缺失 Remove 和空 Clear，都先拒绝活动查询，不能从无变化分支绕过。

Each 同步消费回调，局部 native view 不逃逸；无必需组件走实体存储，包含无组件实体。参数固定为 EntityId 加模板顺序引用，Exclude 不传参；void 继续或显式 VisitControl 控制退出，返回 Completed/Stopped。默认无序，只比较匹配集合；嵌套各层 Stop/异常独立，退出不回滚已经执行的值写入。

EachOrdered 在同一保护内收集自有键/ID，以 `std::less<>` 排序，在任何业务 visitor 前拒绝等价键；不改 native 池顺序，不按原生 ID、地址或遇到顺序破平局。键生成只读，关系须严格弱序；准备/分配/比较失败为零业务回调，已准备的键不会因 visitor 改值而重排。查询后的结构操作由调用方收集自有值并在全部层退出后显式执行，无自动批事务。

同一及不同 Registry 均要求调用方外部串行，结束所有借用并建立同步后可交接线程，不绑定创建线程。guard 不是锁；进程批次及类型机制不提供并行读取/独立实例并发保证。

### 失败分类与诊断

契约错误为具名 RegistryError；原始 bad_alloc、构造或回调异常传播，不经 Logger/断言继续执行。注册准备不发布；Create/Emplace 按固定候选的实际存储结果核对是否可确认一致，区分活对象与允许保留的 in-place tombstone/容量。单次新增失败保已有合法值，不回滚输入移动/外部副作用，也不恢复旧借用或共享装配事务。

Destroy/Remove/Clear 的实际结构异常或 Create/Emplace 一致性无法确认时，记录 Failed 操作/类别及未知事实后停止普通访问；不是所有异常一律 Failed，也不是所有异常都能恢复。普通 visitor/键异常只退出相应查询保护，既有值更新不回滚。GetStatus 返回无需分配、无需扫描 native 的最小自有摘要；Failed 的 IsEntityValid/读取/注册/Clear 均拒绝，只保摘要与析构清理，不提供修复接口。

DBG01 的统一 Debug ABI 和窄诊断测试授权见 [工程约束](engineering.md#t5-typed-依赖与统一-abi)。关闭 STL iterator debug 接受检查损失，不证明任意 noexcept、组件违约、持续 OOM 或内存破坏可恢复；代表性故障命中、零分配路径与未覆盖边界分别在 verification 记录。

### 不变量与消费者

保留本域 I1-I4 身份，以下明确是 T5 对原条件的替换，不反写旧实现或历史测试：I1 仍为 Registry 唯一 owner 且覆盖借用；I2 从旧 POD/全局顺序注册改为真实对象约束、每实例注册与独立碰撞检查；I3 明确实际结构阶段及当前回调的借用边界；I4 从旧 Free/Clear 约定改为分层失败、不可原地修复的 Failed 和 RAII。

simulation 的 PhysicsStepOrder 是装配方业务键，不是 ECS ID；完整键准备、单体提交与 Session 两层失败诊断见 [simulation](simulation.md#t5-调用适配)。app 不取得 native 存储。仅指定根测试取得私有合法极端状态适配，同一正式 ECS 库不为测试换布局。已获准退役的旧五文件随本轮同步在 F 移除，不在正式 ECS 编译或安装；foundation 分配/日志、Camera 兼容及活动 GL 仍有消费者，不据此删库或宣布全局内存审计。

## F 主工作区旧实现

标题及下列旧锚点保留供 T4 历史引用使用。以下仅描述 T4 时期的局部修补及限制，已被上面的 T5 机制替换，不再适用于 F 当前代码，也不继续维护为活动兼容入口。旧源码引用限定仍保留的 T4 worktree；点时成绩见 [T4 最小修补](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/verification.md#2026-10-15-最小-ecs-前提修补)，不是当前 T5 测试结果。

### 所有权与存储机制

Registry 构造一次 `unique_ptr<Storage>`，独占实体槽、空闲槽列表、组件容器和自有 debug 名称；不可复制，用户声明析构且未定义移动，实际也不可移动。系统、Camera、view 和 iterator 只借用 Registry，不能接管或释放其存储。析构调用 `Free`；没有自动调度或后台工作。

每类组件一个稀疏映射加连续字节区，不是 archetype。稀疏池按 8 个实体索引对齐，映射的 `UINT32_MAX` 表示没有组件；查池线性扫描，dense 区初始容量 8，扩容翻倍。新增组件按 `component_size` 清零，typed facade 将字节地址转换为 `T&`，不运行组件构造/析构。因此仅接受 standard-layout 且 trivial 的 POD，不应存入拥有资源或依赖析构的类型；特殊对齐和更一般存储能力未获保证。

组件类型号是进程级 counter 加每个 `T` 的静态缓存，不属于某个 Registry。注册仍要求类型号等于该 Registry 当前容器数，所有 Registry 必须采用相同类型/顺序且不超过 256。2026-10-15局部修补加入 `IsComponentRegistered<T>` 检查存储，缺注册不隐式装配；首次查询仍可能分配该类型号，不是进程类型系统的纯读取。重复/错序注册、无效定义、类型越界及Free后注册在分配前具名抛异常，不再靠断言。`Clear` 不重置进程类型号，不能推导为可任意换一套类型重新注册。

私有ComponentContainer以RAII独占 `pools/entities/data` 三个分配，默认空、禁止复制、noexcept移动转交并清空源；显式Free与析构均可安全面对部分初始化/已释放状态。InitRaw在临时owner中完成三个分配才交换，空返回抛bad_alloc，字节乘法溢出在分配前拒绝。注册先准备容器及自有名称，再reserve两份索引，最后只转交owner发布；前序失败不发布半注册类型，已经注册的值保留。名称使用unique_ptr<string>，避免MSVC Debug中string的noexcept移动仍分配调试proxy；不是更换组件布局或重做类型系统。

旧依据：[公开头](E:/Codex/Worktrees/refactor-application-simulation/Symocraft/game/modules/ecs/include/symocraft/ecs/registry.h)、[Registry/Iterator](E:/Codex/Worktrees/refactor-application-simulation/Symocraft/game/modules/ecs/src/registry.cpp)、[容器](E:/Codex/Worktrees/refactor-application-simulation/Symocraft/game/modules/ecs/src/component_container.h)、[稀疏池实现](E:/Codex/Worktrees/refactor-application-simulation/Symocraft/game/modules/ecs/src/internal.cpp)、[目标依赖](E:/Codex/Worktrees/refactor-application-simulation/Symocraft/game/modules/ecs/CMakeLists.txt)。不把当前 F 的新头/测试链接当旧实现依据。

### 身份检查的实际限制

EntityId 的高 32 位是 version，低 32 位是 index；`null_entity` 的 index 为 `UINT32_MAX`。删除先移除组件，再在槽内保存空 index 与递增 version，回收 index，创建时复用该槽的 version。当前局部身份检查与仍保留的边界为：

- `IsEntityValid` 先拒绝Free状态、空/越界索引，再比较槽内完整ID；在未Clear的同一Registry中拒绝已删除及复用前的旧代ID。EntityId仍没有owner字段，同数值跨Registry或跨Clear的旧ID不能靠此识别，version没有耗尽退役策略。
- Has/Get/Add/Remove在Registry门面先检查上述身份，私有映射仍按index；`DestroyEntity` 在移除/索引前具名拒绝非法或重复ID。合法销毁仍依赖未全面修复的Remove，不能推定一般销毁/回滚可靠。
- Iterator解引用与view筛选传递完整64位EntityId，不再截断version；这不改变遍历期间禁止结构修改、或组件借用失效规则。

调用者必须持有所属 Registry 的当前活实体、已注册所需组件，并避免复用/删除路径上的上述风险。不能把现代 Renderer 的 owner/generation 句柄保证套到 ECS。

### 借用、遍历与同步

`GetComponent<T>(...) const` 和 byte Get 实际返回可写引用/指针；`const Registry` 不提供组件只读性。byte Get缺实体/类型/组件返回nullptr，私有Get对dense索引做运行时范围检查；typed Add/Get在解引用前检查空值，以包含操作名的runtime_error传播，不再用断言处理缺组件。实际容器分配失败以bad_alloc中断AddOrGet，不再打印后继续Get；依赖 [foundation限定修补](foundation.md#旧分配与诊断边界) 的真实失败返回，不扩大为任意旧内存调用均安全。

组件地址借用 Registry 的 dense 区，池扩容、移除重排、Clear/Free 或 Registry 销毁会使地址或其所指实体语义失效。即使地址未变，也不能在结构修改后把旧引用当成同一实体组件。系统只在当前同步查询内使用引用。

view 自有 256 位筛选条件、借用 Registry，不生成实体快照。构造带类型筛选时先核对注册，未注册/越界类型具名拒绝，不作为空查询；begin/increment顺序扫描实体槽并检查完整ID/组件，end是当时槽数量。iterator/view的复制或移动构造仍是借用别名，引用成员阻止赋值。iterator比较仅比index，不检查Registry/filter，不能比较不同查询的迭代器，也不能跨结构变化保存end。

Registry、全局类型 counter、容器、view 均无并发/重入保护。禁止遍历中增删实体/组件、Clear/Free 或并发调用；底层分配器的锁不保护这里的容器、类型注册和返回引用。

### 失败与清理边界

`Free` 标记存储不可访问，再逐容器释放并置空三个旧指针，清容量/计数，但保留实体/注册向量；已取得和部分取得的组件分配可重复清理，不再分配内存，之后Create/Register/typed Add/Get具名拒绝。`Clear`先清实体、Free，再清容器/名称/空闲槽并恢复可注册状态；不是重置进程类型号。Clear前必须结束旧借用，不依赖IsEntityValid跨Clear防止旧ID被误认；不承诺所有标准库构造或全进程OOM均可恢复。

以下 T4 点时风险不能浓缩成“稀疏集可可靠删除/扩容”，不表示当前 T5 仍使用这些路径：

- dense容量仍翻倍，但data/entities改为先分配并复制两份临时缓冲，均成功才替换旧指针/容量并发布新映射。任一失败清理临时缓冲，既有payload/映射保留，新组件不存在；代价是扩容期暂存双缓冲，未作性能保证。
- sparse池扩容沿已修补的公共ReAlloc，失败保留旧池。其成功后若dense增长失败，可以留下额外空池/容量，既有组件值仍有效；不是存储字节级强事务，也不承诺跨操作装配回滚。调用方可整体丢弃专属Registry，不以通用Remove回滚半装配。
- Remove 用 sparse `entity_index >= num_components` 判断存在；非尾交换只在组件数大于 2 时执行，且 `data[dense_index] = data[last_index]` 只复制一字节，没有按组件大小搬移。稀疏高 index、非尾/少量删除及多字节 payload 不具通用正确性保证。
- Destroy 逐组件移除后再更新实体槽/空闲列表；中途失败不回滚先前变更。不能因正常两实体路径通过而关闭这些 T5 缺口。

私有 `Serialize/Deserialize` 与 `legacy/RawMemory.h` 是保留的非活动遗留代码，不是产品存档协议。RawMemory 没有自动释放，危险写入分支尚未实现实际扩容/写入，反序列化也无完整格式/索引校验；`MemoryHelper` 的打包指针、对齐/边界检查不提供新持久化能力。

### 保留的不变量与核对边界

原 ECS 编号保持本域身份，不与其他模块 I 编号混用：

| 编号 | T4 点时条件 |
| --- | --- |
| I1 | Storage 从构造完成至析构由 Registry 唯一拥有；借用者寿命被它覆盖。 |
| I2 | 注册顺序/类型号一致、最多 256、组件满足 POD；这是调用者前提而非通用错误恢复。 |
| I3 | 组件借用只在池未扩容、删除、释放且结构稳定时有效；view 不提供持久身份。 |
| I4 | Free可重复且置空旧池，初始化/注册转交/增长失败的已取得组件资源可清理；门面拒绝继续装配，显式Clear后按原顺序重建。不是通用删除/全进程OOM保证。 |

[旧 registry_tests](E:/Codex/Worktrees/refactor-application-simulation/Symocraft/test/unit/ecs/registry_tests.cpp)检查正常门面、注册/缺组件拒绝、同Registry旧代/非法/重复销毁、64位过滤遍历与Free/Clear；[registry_allocation_tests](E:/Codex/Worktrees/refactor-application-simulation/Symocraft/test/unit/ecs/registry_allocation_tests.cpp)在同一生产库注入三个初始化点、实际string/vector分配及sparse/dense增长失败，核对未发布、旧值、重试或整体清理。普通/跟踪模式分别执行，清理还在后续分配继续失败时验证。不覆盖任意Remove搬移、跨Registry/跨Clear身份、版本耗尽、特殊对齐或操作系统整体OOM。实际结果见 [T4 verification](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/verification.md#2026-10-15-最小-ecs-前提修补)，不能据此通过T5或完整T4。
