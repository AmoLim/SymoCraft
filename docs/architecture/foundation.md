---
tags:
  - area/architecture
---

# Foundation

## 边界与权威

`symocraft_foundation` 提供公共整数别名、统一 GLM 配置、自有元数据值、文件发布和旧内存/日志兼容入口；不管理 World、Registry、窗口、GPU 或会话。
公开依赖只有数学 target；Windows、分配记录和日志状态留在私有实现。target 规则见 [module-boundaries](../openspec/specs/module-boundaries/spec.md)，发布的公开保证与错误特定状态以 [foundation-files](../openspec/specs/foundation-files/spec.md) 为准。
实现入口：[CMake](../../game/modules/foundation/CMakeLists.txt)、[公开头](../../game/modules/foundation/include/symocraft/foundation)、[Value](../../game/modules/foundation/src/document.cpp)、[发布](../../game/modules/foundation/src/files.cpp)、[兼容实现](../../game/modules/foundation/src/legacy/AmoBase.cpp)。

## 自有元数据

`Data::Value` 是递归自有 `variant`：undefined、bool、有符号/无符号 64 位整数、float、double、string、sequence、mapping。float 与 double 分开保存，避免元数据序列化时无意改变原数值表达；mapping 是插入顺序的 `vector<pair<string,Value>>`，不是共享解析节点或哈希表。YAML 解析/序列化属于 telemetry 的私有适配，不是 foundation 的公共依赖。

- 默认值 undefined。复制递归拥有字符串/子集合；移动后源只具有标准库的有效但未指定状态，不承诺为空。flow-style 是值的输出提示，不是引用或解析器状态。
- mutable key 访问可把 undefined 转为空 mapping，再查找/插入；`push_back` 可把 undefined 转为空 sequence。已有其他类型不能被这两种操作静默覆盖。后续分配失败可能已经留下空集合，不是整个操作的强事务保证。
- const key 访问不插入；不匹配/缺失返回进程期静态 undefined。sequence 索引不扩容，类型错误/越界分别沿 `variant`/`at` 异常传播。
- `operator bool` 判断是否定义，**不是**所存 bool 的真值；`Value(false)` 仍为 defined。`size` 只统计 mapping/sequence 项数，字符串/其他标量为零。
- `as<T>` 返回自有值；匹配类型复制，数值之间仅 `static_cast`，不提供范围、溢出或业务校验。无效转换抛异常；调用方负责合法数值域。直接构造 Mapping 不去重，C 字符串输入须非空且正确终止。
- key/index 返回子值引用，`storage()` 返回内部只读借用；容器增长、替换、赋值及 owner 销毁均可能使借用失效。复制 span/引用不延寿，不能跨结构修改缓存子节点地址。

元数据构造、深复制、线性 key 查找和序列化属于启动/导出冷路径，不应放入逐格 meshing 或每 draw 热循环。`Value` 无锁；独立深副本消除共享所有权，不证明同一对象的并发读写安全。

## 文件发布机制

调用方先完整写入并关闭同 Windows 卷临时文件。当前实现直接调用 `ReplaceFileW`（无 backup，flags=0）；仅 `ERROR_FILE_NOT_FOUND` 才调用一次不覆盖并发新目标的 `MoveFileExW`（flags=0）。不先 `exists`、不 truncate 目标、不 sleep/retry，不将该错误解释为“只有目标缺失”。成功的新读者取得完整发布结果，兼容 delete-sharing 的旧读者仍持旧文件身份。

API 后立即保存 `GetLastError`，再诊断/包装 `system_error`，避免时间/输出操作改写原错误。`SYMOCRAFT_PUBLISH_DIAGNOSTICS` 在首次检查时缓存，仅值 `1` 启用 API/flags/error/进程/线程/UTC/序列日志；诊断不改变发布选择，但会影响时序，日志不是无干扰复现。
失败后的文件名/属性须按实际 Windows 错误判断，不能从“原子发布”推定所有 I/O 故障均回滚。1175、1176、1177 的不同后状态及 telemetry 晚期发布失败语义只在上述现役 spec 维护。
Win32 1175 根因/正式同源 Q06 仍未关闭，[Fix1](../openspec/changes/m3-t2-publication-fix1/proposal.md) 暂停；T2 批准不使它们自动修复。

## 旧分配与诊断边界

`AmoBase` 兼容入口保留 `malloc/realloc/free` 和字节操作。tracking 模式在分配前后增加哨兵，以私有 vector 记录地址、大小、来源和活动计数；记录拥有元数据而不拥有调用方 payload。`file_allocator` 借用来源字符串，宏的 `__FILE__` 有静态寿命；自行调用须保证该字符串覆盖后续诊断。`references` 是旧记账，不是通用共享引用计数或稳定 allocation handle。

调用方不得混用 tracking/非 tracking 指针，不得在活动分配期间改变模式或 buffer 单位；原始字节函数要求合法足量内存，CopyMem 沿 `memcpy` 的非重叠前提。这里没有 RAII payload owner、异常安全容器或完整错误返回保证。
当前 tracking 的 OOM 指针偏移、realloc 失败/记账、分配大小加法及查找/迭代器锁范围均有旧风险，不把“存在 mutex”写成线程安全或失败保旧保证。初始化/模式、日志级别也没有完整同步；T5 审计未实施，本轮不修算法或声称故障已覆盖。

Error/Warning 是输出，不自动抛异常；Assert 受日志级别控制，启用且失败时断点并 `exit(-1)`，不能靠 catch 得到正常清理，也不能当作 Release 下始终有效的参数检查。日志输出锁只保护该局部输出过程，不赋予 ECS/分配器/整个应用并发保证。

## 稳定不变量与验证边界

| 原身份 | 继续成立的边界 |
| --- | --- |
| Foundation I1 | Value 的拥有式副本不共享另一文档/解析节点的子树。 |
| Foundation I2 | 不兼容 mapping/sequence 修改明确失败，不静默替换已有类型；不扩大为 OOM 事务保证。 |
| Foundation I3 | Files 发布须关闭临时文件、同卷且不依赖窗口；成功完整可见及失败状态以现役发布契约限定。 |

[document_tests](../../test/unit/foundation/document_tests.cpp) 核对值复制、类型/精度和 telemetry YAML 适配；[files_tests](../../test/unit/foundation/files_tests.cpp) 核对首次/替换、旧读者身份与具名失败，不证明所有文件系统并发及 1175 根因。旧 allocator 不由这两组测试获得安全背书。
历史结果/批准统一从 [VERIFICATION](../verification/VERIFICATION.md) 进入；本次只有源码及文档核对，未重新运行 C++ 测试、发布复现或性能测量。
