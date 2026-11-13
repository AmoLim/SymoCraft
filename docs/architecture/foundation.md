---
tags:
  - area/architecture
---

# Foundation

> 已验收候选及保留限制见 [change 验证记录](../openspec/changes/integrate-renderdoc-tracy/verification.md)。构建和运行命令以仓库根目录为准。

## 边界与权威

`symocraft_foundation` 提供公共整数别名、统一 GLM 配置、自有元数据值和旧内存/日志兼容入口；不管理 World、Registry、窗口、GPU 或会话。
公开依赖只有数学 target；Windows、分配记录和日志状态留在私有实现。target 规则见 [module-boundaries](../openspec/specs/module-boundaries/spec.md)，Files::Publish 已随旧性能链退役，目标边界及候选验证见 [当前 change](../openspec/changes/integrate-renderdoc-tracy/verification.md#2026-11-03-publish-追加退役范围与准确清单)。
实现入口：[CMake](../../game/modules/foundation/CMakeLists.txt)、[公开头](../../game/modules/foundation/include/symocraft/foundation)、[Value](../../game/modules/foundation/src/document.cpp)、[兼容实现](../../game/modules/foundation/src/legacy/AmoBase.cpp)。

## 自有元数据

`Data::Value` 是递归自有 `variant`：undefined、bool、有符号/无符号 64 位整数、float、double、string、sequence、mapping。float 与 double 分开保存，避免元数据序列化时无意改变原数值表达；mapping 是插入顺序的 `vector<pair<string,Value>>`，不是共享解析节点或哈希表。YAML 解析/序列化由下述独立适配负责，不是基础 `symocraft_foundation` 的依赖。

- 默认值 undefined。复制递归拥有字符串/子集合；移动后源只具有标准库的有效但未指定状态，不承诺为空。flow-style 是值的输出提示，不是引用或解析器状态。
- mutable key 访问可把 undefined 转为空 mapping，再查找/插入；`push_back` 可把 undefined 转为空 sequence。已有其他类型不能被这两种操作静默覆盖。后续分配失败可能已经留下空集合，不是整个操作的强事务保证。
- const key 访问不插入；不匹配/缺失返回进程期静态 undefined。sequence 索引不扩容，类型错误/越界分别沿 `variant`/`at` 异常传播。
- `operator bool` 判断是否定义，**不是**所存 bool 的真值；`Value(false)` 仍为 defined。`size` 只统计 mapping/sequence 项数，字符串/其他标量为零。
- `as<T>` 返回自有值；匹配类型复制，数值之间仅 `static_cast`，不提供范围、溢出或业务校验。无效转换抛异常；调用方负责合法数值域。直接构造 Mapping 不去重，C 字符串输入须非空且正确终止。
- key/index 返回子值引用，`storage()` 返回内部只读借用；容器增长、替换、赋值及 owner 销毁均可能使借用失效。复制 span/引用不延寿，不能跨结构修改缓存子节点地址。

元数据构造、深复制、线性 key 查找和序列化属于启动/导出冷路径，不应放入逐格 meshing 或每 draw 热循环。`Value` 无锁；独立深副本消除共享所有权，不证明同一对象的并发读写安全。

## 独立 YAML 适配

N1 将原 telemetry 适配原义迁入 foundation 所属的 `symocraft_document_io`，唯一编译 [document_io.cpp](../../game/modules/foundation/src/document_io.cpp)。消费者包含 [symocraft/foundation/document_io.h](../../game/modules/foundation/include/symocraft/foundation/document_io.h) 并链接该 target；它公开依赖 foundation 的中立 `Data::Value`，私有消费 yaml-cpp，基础 foundation 和独立 physics 不反向链接它。旧性能链已退役，world-summary、文档往返和有效故障探针继续消费此适配。

Dump 构造独立 YAML node/string，不修改输入；Mapping/Sequence 保持值结构及 flow style，能被解释为 bool/number 的 string 显式标为字符串。Load 同步复制 text，再返回自有值；显式字符串不重解释，其他 scalar 按 bool → int64 → uint64 → double → string 尝试，map key 转 string，null 为 monostate。LoadYamlFile 直接使用传入 path，不经 Assets resolver；文件/语法/转换异常传播，不返回半个成功文档。它不是业务 schema 校验器，也没有对任意文档分配量提供界限。此适配从未依赖 Files::Publish，发布 API 退役不改变其行为。

## 文件发布退役

2026-11-03 用户确认移除无生产消费者的 Files::Publish 公开头、实现与专属测试/注册，不提供替代发布服务或兼容壳。YAML、world-summary 与图像输出各自的有效行为保留；外部性能工具不替代其正确性检查。

旧同卷关闭流、ReplaceFileW/MoveFileExW、兼容读者快照与错误特定后状态只作历史依据，详见 [Fix1 设计](../openspec/changes/archive/2026-11-03-m3-t2-publication-fix1/design.md)。Fix1 已因能力退役取消计划归档，14项未完成任务保持，未合并其目标规格。1175 根因、缺码和超时不改判已修复；旧诊断现场保留，不再作为活动产品编译或验收入口。正式同源 Q06 等历史未测范围也不由退役转为通过。

## 旧分配与诊断边界

T5 的 [ECS](ecs.md#t5-实现机制) 已改用标准分配并于 2026-10-25 按用户独立授权同步 F，不再消费旧字节池或 allocator probe。AmoBase 仍同时承载实际日志、app 初始化/诊断及 foundation 分配测试，故保留本节机制与消费者，不因 ECS 停用而整文件/整库删除。新 ECS 的 OOM 依据来自实际 new/aligned-new 注入，不是旧 tracked 开关；本次不是全局内存、特殊对齐或全进程 OOM 审计完成声明。

`AmoBase` 兼容入口保留 `malloc/realloc/free` 和字节操作。tracking 模式在分配前后增加哨兵，以私有 vector 记录地址、大小、来源和活动计数；记录拥有元数据而不拥有调用方 payload。`file_allocator` 借用来源字符串，宏的 `__FILE__` 有静态寿命；自行调用须保证该字符串覆盖后续诊断。`references` 是旧记账，不是通用共享引用计数或稳定 allocation handle。

调用方不得混用 tracking/非 tracking 指针，不得在活动分配期间改变模式或 buffer 单位；Free仍要求有效、非空、未释放的库分配，非法/重复释放处理没有在本轮重构。原始字节函数要求合法足量内存，CopyMem沿 `memcpy` 的非重叠前提。没有通用RAII payload owner；具体调用方仍须在取得分配后接管所有权。

2026-10-15为T4装配清理做限定修补：Allocate/ReAlloc先检查零请求和哨兵padding加法溢出，失败返回真实nullptr而不偏移空地址；零请求不隐式释放旧块，`ReAlloc(nullptr, positive)`沿Allocate。tracked Allocate以局部owner保管raw malloc，元数据插入bad_alloc/length_error或故障拒绝时释放raw块，不发布记录。tracked ReAlloc在同一锁范围内查找活动记录、复制旧尾哨兵并调用realloc；尾缓冲或realloc失败保留旧块、payload及活动记账。成功后只更新/复用已有记录，不在旧块可能移动后再扩容元数据；尾哨兵保留原字节，不抹掉已有越界证据。底层仍为malloc/realloc/free，未改变buffer单位、对齐和分配策略。

私有 [allocation_probe](E:/Codex/Worktrees/refactor-application-simulation/Symocraft/game/modules/foundation/src/allocation_diagnostics/allocation_probe.h) 提供线程局部故障点及加锁的活动块/字节/哨兵快照；探针须非重入且不抛异常，只由指定分配回归测试取得窄include根，不成为公共分配器替换接口或生产对测试的依赖。关闭测试时同一正式库仍成立。此锁范围修补不赋予调用方并发安全；初始化/模式、日志级别仍没有完整同步，特殊对齐、非法Free、全进程OOM恢复及T5完整审计未覆盖。

Error/Warning 是输出，不自动抛异常；Assert 受日志级别控制，启用且失败时断点并 `exit(-1)`，不能靠 catch 得到正常清理，也不能当作 Release 下始终有效的参数检查。日志输出锁只保护该局部输出过程，不赋予 ECS/分配器/整个应用并发保证。

## 稳定不变量与验证边界

| 原身份 | 继续成立的边界 |
| --- | --- |
| Foundation I1 | Value 的拥有式副本不共享另一文档/解析节点的子树。 |
| Foundation I2 | 不兼容 mapping/sequence 修改明确失败，不静默替换已有类型；不扩大为 OOM 事务保证。 |
| Foundation I3（退役） | 原 Files 发布契约仅保留历史适用身份，当前不提供该 API；未证明的可靠性保证不因退役成立。 |

[document_tests](../../test/unit/foundation/document_tests.cpp)核对值复制、类型/精度和独立 YAML 适配；旧 files_tests 随 API 退役，先前失败见 [退役记录](../openspec/changes/integrate-renderdoc-tracy/verification.md#2026-11-03-publish-追加退役范围与准确清单)，不改判1175根因已修复。[memory_tests](E:/Codex/Worktrees/refactor-application-simulation/Symocraft/test/unit/foundation/memory_tests.cpp)直接链接正式foundation，覆盖普通/跟踪分配失败、真实记录vector抛错、旧块保留、增缩容及哨兵；不是操作系统整体耗尽实验。实际CPU结果及限制见 [T4 verification](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/verification.md#2026-10-15-最小-ecs-前提修补)，历史结果/批准仍从 [VERIFICATION](../verification/VERIFICATION.md) 进入；未做新发布复现或性能测量。
