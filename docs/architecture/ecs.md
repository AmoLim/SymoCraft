---
tags:
  - area/architecture
---

# ECS

`symocraft_ecs` 拥有旧式 Registry 与组件池，不拥有游戏规则、World 或系统调度。模块公开边界归 [module-boundaries spec](../openspec/specs/module-boundaries/spec.md)；尚未单列 ECS 现役 spec 的存储、借用和失败限制由本文维护。这里描述当前源码，不把版本字段、PImpl 或正常路径测试推定为 T5 的可靠实体/存储实现。

## 所有权与存储机制

Registry 构造一次 `unique_ptr<Storage>`，独占实体槽、空闲槽列表、组件容器和自有 debug 名称；不可复制，用户声明析构且未定义移动，实际也不可移动。系统、Camera、view 和 iterator 只借用 Registry，不能接管或释放其存储。析构调用 `Free`；没有自动调度或后台工作。

每类组件一个稀疏映射加连续字节区，不是 archetype。稀疏池按 8 个实体索引对齐，映射的 `UINT32_MAX` 表示没有组件；查池线性扫描，dense 区初始容量 8，扩容翻倍。新增组件按 `component_size` 清零，typed facade 将字节地址转换为 `T&`，不运行组件构造/析构。因此仅接受 standard-layout 且 trivial 的 POD，不应存入拥有资源或依赖析构的类型；特殊对齐和更一般存储能力未获保证。

组件类型号是进程级 counter 加每个 `T` 的静态缓存，不属于某个 Registry。注册要求类型号等于该 Registry 当前容器数，所有 Registry 必须采用相同类型/顺序且不超过 256；查询未注册类型也可能首次分配该类型号。注册诊断依赖断言，不是可恢复错误协议。`Clear` 不重置进程类型号，不能推导为可任意换一套类型重新注册。

私有 ComponentContainer 持有 `pools/entities/data` 三个裸分配，没有 RAII 析构且保留浅复制。注册先在局部零初始化容器中分配，再按值放入 Storage；唯一释放责任仍在 Registry 的清理循环，不能在局部副本再 Free，也不能把容器复制当成独立所有者。注册中分配、容器插入或名称插入失败没有完整回滚。

依据：[公开头](../../game/modules/ecs/include/symocraft/ecs/registry.h)、[Registry/Iterator](../../game/modules/ecs/src/registry.cpp)、[容器](../../game/modules/ecs/src/component_container.h)、[稀疏池实现](../../game/modules/ecs/src/internal.cpp)、[目标依赖](../../game/modules/ecs/CMakeLists.txt)。

## 身份检查的实际限制

EntityId 的高 32 位是 version，低 32 位是 index；`null_entity` 的 index 为 `UINT32_MAX`。删除意图是先移除组件，再在槽内保存空 index 与递增 version，回收 index，创建时复用该槽的 version。这是编码和回收机制，不等于可靠的代数验证：

- `IsEntityValid` 只检查传入 ID 的低位在槽范围内且不是空 index，不比较当前槽 ID/version，也不确认槽仍存活。stale ID、已删除 ID 或其他 Registry 中同 index 的 ID 可能被当作有效；EntityId 没有 owner 字段。
- Has/Get/Add/Remove 的组件映射也按 index，不验证代数。`DestroyEntity` 在 RemoveAll 后仍直接索引实体数组；非法、重复或跨 Registry 删除不具安全拒绝或回滚保证，version 也没有耗尽退役策略。
- Iterator 解引用声明返回 `EntityIndex`，实际从 64 位槽 ID 返回，截断 version；view 不能恢复这一身份信息。当前正常单玩家调用不证明可靠多代或跨实例查询。

调用者必须持有所属 Registry 的当前活实体、已注册所需组件，并避免复用/删除路径上的上述风险。不能把现代 Renderer 的 owner/generation 句柄保证套到 ECS。

## 借用、遍历与同步

`GetComponent<T>(...) const` 和 byte Get 实际返回可写引用/指针；`const Registry` 不提供组件只读性。typed Get 先断言 Has，typed Add/Get 最后会解引用 byte 结果；byte 层的日志或 `nullptr` 不是 typed 层的通用错误返回，不能依赖 Release 中断言提供完整保护。

组件地址借用 Registry 的 dense 区，池扩容、移除重排、Clear/Free 或 Registry 销毁会使地址或其所指实体语义失效。即使地址未变，也不能在结构修改后把旧引用当成同一实体组件。系统只在当前同步查询内使用引用。

view 自有 256 位筛选条件、借用 Registry，不生成实体快照。begin/increment 顺序扫描实体槽并检查组件；end 是当时槽数量。iterator/view 的复制或移动构造仍是借用别名，引用成员阻止赋值。iterator 比较仅比 index，不检查 Registry/filter，不能比较不同查询的迭代器，也不能跨结构变化保存 end。未注册/越界类型筛选不构成安全空查询。

Registry、全局类型 counter、容器、view 均无并发/重入保护。禁止遍历中增删实体/组件、Clear/Free 或并发调用；底层分配器的锁不保护这里的容器、类型注册和返回引用。

## 失败与清理边界

`Free` 逐容器释放并置空三个旧指针，清容量/计数，但保留 Registry 的实体/注册向量；重复清理安全，之后只继续清理或销毁，不继续 Add/Get。`Clear` 先清实体、Free，再清容器/名称/空闲槽；它不是重置整个进程类型注册的 API。

以下现存实现风险不能浓缩成“稀疏集可可靠删除/扩容”：

- `InitRaw` 对分配结果未完整判空；扩容 data 与 entities 连续执行两次 realloc，失败分支释放返回的新地址，而旧成员可能已因成功 realloc 失效，不能承诺 OOM 后对象仍可用、仍可安全释放或没有泄漏。
- `AddOrGet` 在 Add 失败后仍 Get；新增池、各 vector 和映射更新不是事务，缺少强失败保证。
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
| I4 | Free 可重复且置空旧池；Free 后不再继续 Add，只清理/销毁。 |

[registry_tests](../../test/unit/ecs/registry_tests.cpp) 覆盖正常注册、清零、读写、双组件 view、两次 Free 和第二 Registry 同序注册；未覆盖 stale/cross-ID、Remove 搬移、非法 Destroy、类型耗尽、对齐或 OOM。本文经源码/公开头/CMake 与测试静态核对，本轮未运行这些测试。历史依据从 [VERIFICATION](../verification/VERIFICATION.md) 进入；后续 T5 范围见 [scope](../scope/project-scope.md)，不因文档迁移获得实施授权。
