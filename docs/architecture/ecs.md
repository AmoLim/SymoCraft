---
tags:
  - area/architecture
---

# ECS

`symocraft_ecs` 拥有旧式 Registry 与组件池，不拥有游戏规则、World 或系统调度。模块公开边界归 [module-boundaries spec](../openspec/specs/module-boundaries/spec.md)；尚未单列 ECS 现役 spec 的存储、借用和失败限制由本文维护。这里描述当前源码，不把版本字段、PImpl 或正常路径测试推定为 T5 的可靠实体/存储实现。

## 所有权与存储机制

Registry 构造一次 `unique_ptr<Storage>`，独占实体槽、空闲槽列表、组件容器和自有 debug 名称；不可复制，用户声明析构且未定义移动，实际也不可移动。系统、Camera、view 和 iterator 只借用 Registry，不能接管或释放其存储。析构调用 `Free`；没有自动调度或后台工作。

每类组件一个稀疏映射加连续字节区，不是 archetype。稀疏池按 8 个实体索引对齐，映射的 `UINT32_MAX` 表示没有组件；查池线性扫描，dense 区初始容量 8，扩容翻倍。新增组件按 `component_size` 清零，typed facade 将字节地址转换为 `T&`，不运行组件构造/析构。因此仅接受 standard-layout 且 trivial 的 POD，不应存入拥有资源或依赖析构的类型；特殊对齐和更一般存储能力未获保证。

组件类型号是进程级 counter 加每个 `T` 的静态缓存，不属于某个 Registry。注册仍要求类型号等于该 Registry 当前容器数，所有 Registry 必须采用相同类型/顺序且不超过 256。2026-10-15局部修补加入 `IsComponentRegistered<T>` 检查存储，缺注册不隐式装配；首次查询仍可能分配该类型号，不是进程类型系统的纯读取。重复/错序注册、无效定义、类型越界及Free后注册在分配前具名抛异常，不再靠断言。`Clear` 不重置进程类型号，不能推导为可任意换一套类型重新注册。

私有ComponentContainer以RAII独占 `pools/entities/data` 三个分配，默认空、禁止复制、noexcept移动转交并清空源；显式Free与析构均可安全面对部分初始化/已释放状态。InitRaw在临时owner中完成三个分配才交换，空返回抛bad_alloc，字节乘法溢出在分配前拒绝。注册先准备容器及自有名称，再reserve两份索引，最后只转交owner发布；前序失败不发布半注册类型，已经注册的值保留。名称使用unique_ptr<string>，避免MSVC Debug中string的noexcept移动仍分配调试proxy；不是更换组件布局或重做类型系统。

依据：[公开头](../../game/modules/ecs/include/symocraft/ecs/registry.h)、[Registry/Iterator](../../game/modules/ecs/src/registry.cpp)、[容器](../../game/modules/ecs/src/component_container.h)、[稀疏池实现](../../game/modules/ecs/src/internal.cpp)、[目标依赖](../../game/modules/ecs/CMakeLists.txt)。

## 身份检查的实际限制

EntityId 的高 32 位是 version，低 32 位是 index；`null_entity` 的 index 为 `UINT32_MAX`。删除先移除组件，再在槽内保存空 index 与递增 version，回收 index，创建时复用该槽的 version。当前局部身份检查与仍保留的边界为：

- `IsEntityValid` 先拒绝Free状态、空/越界索引，再比较槽内完整ID；在未Clear的同一Registry中拒绝已删除及复用前的旧代ID。EntityId仍没有owner字段，同数值跨Registry或跨Clear的旧ID不能靠此识别，version没有耗尽退役策略。
- Has/Get/Add/Remove在Registry门面先检查上述身份，私有映射仍按index；`DestroyEntity` 在移除/索引前具名拒绝非法或重复ID。合法销毁仍依赖未全面修复的Remove，不能推定一般销毁/回滚可靠。
- Iterator解引用与view筛选传递完整64位EntityId，不再截断version；这不改变遍历期间禁止结构修改、或组件借用失效规则。

调用者必须持有所属 Registry 的当前活实体、已注册所需组件，并避免复用/删除路径上的上述风险。不能把现代 Renderer 的 owner/generation 句柄保证套到 ECS。

## 借用、遍历与同步

`GetComponent<T>(...) const` 和 byte Get 实际返回可写引用/指针；`const Registry` 不提供组件只读性。byte Get缺实体/类型/组件返回nullptr，私有Get对dense索引做运行时范围检查；typed Add/Get在解引用前检查空值，以包含操作名的runtime_error传播，不再用断言处理缺组件。实际容器分配失败以bad_alloc中断AddOrGet，不再打印后继续Get；依赖 [foundation限定修补](foundation.md#旧分配与诊断边界) 的真实失败返回，不扩大为任意旧内存调用均安全。

组件地址借用 Registry 的 dense 区，池扩容、移除重排、Clear/Free 或 Registry 销毁会使地址或其所指实体语义失效。即使地址未变，也不能在结构修改后把旧引用当成同一实体组件。系统只在当前同步查询内使用引用。

view 自有 256 位筛选条件、借用 Registry，不生成实体快照。构造带类型筛选时先核对注册，未注册/越界类型具名拒绝，不作为空查询；begin/increment顺序扫描实体槽并检查完整ID/组件，end是当时槽数量。iterator/view的复制或移动构造仍是借用别名，引用成员阻止赋值。iterator比较仅比index，不检查Registry/filter，不能比较不同查询的迭代器，也不能跨结构变化保存end。

Registry、全局类型 counter、容器、view 均无并发/重入保护。禁止遍历中增删实体/组件、Clear/Free 或并发调用；底层分配器的锁不保护这里的容器、类型注册和返回引用。

## 失败与清理边界

`Free` 标记存储不可访问，再逐容器释放并置空三个旧指针，清容量/计数，但保留实体/注册向量；已取得和部分取得的组件分配可重复清理，不再分配内存，之后Create/Register/typed Add/Get具名拒绝。`Clear`先清实体、Free，再清容器/名称/空闲槽并恢复可注册状态；不是重置进程类型号。Clear前必须结束旧借用，不依赖IsEntityValid跨Clear防止旧ID被误认；不承诺所有标准库构造或全进程OOM均可恢复。

以下现存实现风险不能浓缩成“稀疏集可可靠删除/扩容”：

- dense容量仍翻倍，但data/entities改为先分配并复制两份临时缓冲，均成功才替换旧指针/容量并发布新映射。任一失败清理临时缓冲，既有payload/映射保留，新组件不存在；代价是扩容期暂存双缓冲，未作性能保证。
- sparse池扩容沿已修补的公共ReAlloc，失败保留旧池。其成功后若dense增长失败，可以留下额外空池/容量，既有组件值仍有效；不是存储字节级强事务，也不承诺跨操作装配回滚。调用方可整体丢弃专属Registry，不以通用Remove回滚半装配。
- Remove 用 sparse `entity_index >= num_components` 判断存在；非尾交换只在组件数大于 2 时执行，且 `data[dense_index] = data[last_index]` 只复制一字节，没有按组件大小搬移。稀疏高 index、非尾/少量删除及多字节 payload 不具通用正确性保证。
- Destroy 逐组件移除后再更新实体槽/空闲列表；中途失败不回滚先前变更。不能因正常两实体路径通过而关闭这些 T5 缺口。

私有 `Serialize/Deserialize` 与 `legacy/RawMemory.h` 是保留的非活动遗留代码，不是产品存档协议。RawMemory 没有自动释放，危险写入分支尚未实现实际扩容/写入，反序列化也无完整格式/索引校验；`MemoryHelper` 的打包指针、对齐/边界检查不提供新持久化能力。

## 保留的不变量与核对边界

原 ECS 编号保持本域身份，不与其他模块 I 编号混用：

| 编号 | 当前条件 |
| --- | --- |
| I1 | Storage 从构造完成至析构由 Registry 唯一拥有；借用者寿命被它覆盖。 |
| I2 | 注册顺序/类型号一致、最多 256、组件满足 POD；这是调用者前提而非通用错误恢复。 |
| I3 | 组件借用只在池未扩容、删除、释放且结构稳定时有效；view 不提供持久身份。 |
| I4 | Free可重复且置空旧池，初始化/注册转交/增长失败的已取得组件资源可清理；门面拒绝继续装配，显式Clear后按原顺序重建。不是通用删除/全进程OOM保证。 |

[registry_tests](../../test/unit/ecs/registry_tests.cpp)检查正常门面、注册/缺组件拒绝、同Registry旧代/非法/重复销毁、64位过滤遍历与Free/Clear；[registry_allocation_tests](E:/Codex/Worktrees/refactor-application-simulation/Symocraft/test/unit/ecs/registry_allocation_tests.cpp)在同一生产库注入三个初始化点、实际string/vector分配及sparse/dense增长失败，核对未发布、旧值、重试或整体清理。普通/跟踪模式分别执行，清理还在后续分配继续失败时验证。不覆盖任意Remove搬移、跨Registry/跨Clear身份、版本耗尽、特殊对齐或操作系统整体OOM。实际结果见 [T4 verification](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/verification.md#2026-10-15-最小-ecs-前提修补)，不能据此通过T5或完整T4。
