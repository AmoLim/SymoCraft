---
tags:
  - area/spec
---

# ECS

## Purpose

定义 T5 替换旧 ECS 的目标公开契约，不表示本目录已经实现或通过验证。旧机制见[architecture](../../../../../../architecture/ecs.md)，实施组织见[design](../../design.md)。下述名称表达操作语义，最终 C++ 重载须经适配核对；不得借命名调整改变契约。验收身份均限定 M3-T5。

## ADDED Requirements

### Requirement: Typed facade and exclusive ownership

Registry SHALL 独占实体与组件存储，以项目 typed 接口公开操作，不公开 EnTT registry、原生实体、pool/view/iterator 或字节/类型编号逃生口。Registry MUST 不可复制或移动；RAII 析构不抛异常并释放已取得资源，不要求先调用公开 Free，初版 SHALL 不提供 Free。借用者 MUST 在 Registry 存活且允许访问的期间使用它，不得在活动查询或组件借用中终结 owner。

#### Scenario: M3-T5-A01 ownership and public boundary
- **WHEN** 独立消费者使用公开头、尝试复制/移动 owner，或正常/异常退出部分装配范围
- **THEN** typed 操作可用、非法特殊成员编译期拒绝，已取得资源由唯一 owner 回收；不能取得可绕过检查的底层对象

### Requirement: Non-owning entity identity

EntityId SHALL 是不延长寿命的项目值标识，区分 Registry 实例、Clear 身份批次及实体代数，不依赖对象地址或原生索引单独判断身份。默认值 SHALL 为空标识；提供相等比较与配套哈希，不公开排序、算术、整数/原生位转换或跨进程持久化承诺。失效 ID 仍可作为容器键，相等/哈希不读取 Registry，也不证明实体仍活着；组件内其他 ID 不自动清理或级联销毁。

#### Scenario: M3-T5-A02 stale cross-instance and cross-clear IDs
- **WHEN** 使用空、已销毁、复用前、其他 Registry 或 Clear 前的 ID，包括原生槽值相同的情况
- **THEN** 不能命中当前实体；比较/哈希仍可使用，不延长 owner 或实体寿命，不使旧 ID 复活

### Requirement: Checked creation destruction and exhaustion

创建 SHALL 只发布当前 Registry 中无组件的有效实体，不等于业务装配完成。销毁 SHALL 移除该实体全部组件并使其 ID 失效；非法或重复销毁具名拒绝。任何实体代数、身份或容量耗尽 SHALL 不回绕；销毁若将耗尽代数，MUST 在移除组件前拒绝，原实体及其组件保持可用，不自动 Clear。需要失败准备的工作 SHALL 在不可逆移除前完成，不承诺跨实体批次事务。

#### Scenario: M3-T5-A03 create destroy and slot reuse
- **WHEN** 创建空实体、添加组件后销毁，再创建或重复销毁
- **THEN** 成功销毁恰好终结对应组件对象，新实体不接受旧 ID，重复销毁具名拒绝；正常无组件实体仍有效

#### Scenario: M3-T5-A04 bounded exhaustion rejection
- **WHEN** 创建或销毁触及实际容量/身份边界，或通过窄测试适配准备合法的末代状态
- **THEN** 正常入口拒绝耗尽且不回绕；销毁预检拒绝保留实体与组件，不用海量自然循环或 Release 断言代替安全检查

### Requirement: Reusable clear with preserved registration

健康 Registry 的 Clear SHALL 清除全部活实体及组件对象，保留已注册类型和首次调试名称，使 Registry 可继续使用；重复或空 Clear 也合法，但所有旧 ID MUST 跨成功 Clear 永久失效。Clear MUST 在破坏旧状态前准备新的不复用身份批次；准备失败保留旧 ID、实体、组件及注册。Clear 不承诺释放全部容量，不充当 Failed 的修复入口；析构释放资源不依赖申请新身份。

#### Scenario: M3-T5-A05 successful clear and reuse
- **WHEN** 对健康 Registry Clear 后重新创建实体，或再次 Clear
- **THEN** 注册/名称保留、新实体可正常使用，原 ID 全部失效，不要求重新注册也不承诺缩容

#### Scenario: M3-T5-A06 clear preparation failure
- **WHEN** 新身份准备因耗尽或分配失败而未完成
- **THEN** 尚未移除旧实体/组件，旧 ID 仍有效；不得先原生 clear 再补做可能失败的身份准备

### Requirement: Explicit registration and owned diagnostic names

组件 SHALL 按 Registry 显式注册，不受跨实例注册顺序约束；同一 C++ 类型重复注册幂等并保留首次名称。名称 MUST 非空，指针形式不得为空指针；重复注册仍检查参数合法性。Registry SHALL 拥有名称副本，不同类型可同名，名称只用于诊断，不参与类型身份/匹配或存档。注册失败不得发布半注册类型或破坏已有值。注册状态查询 SHALL 是不隐式注册的纯读取；其他 typed 操作和查询对未注册类型具名拒绝，不能把它当有效空池。

#### Scenario: M3-T5-A07 registration independence and name lifetime
- **WHEN** 两 Registry 以不同顺序注册相同类型、重复注册或调用方销毁原名称字符串
- **THEN** 注册相互独立，首次名称继续有效；非法名称或注册准备失败不发布半成品，已有实体/组件不变

#### Scenario: M3-T5-A08 unregistered access and type collision
- **WHEN** 查询注册状态、对未注册类型操作，或不同真实 C++ 类型占用相同底层类型键
- **THEN** 状态查询返回未注册而不创建池，普通操作具名拒绝；碰撞在错误池访问/发布前拒绝，依据独立真实类型身份而非同一哈希或调试名

### Requirement: Object components and type qualifiers

组件 SHALL 按正常 C++ 对象构造、移动和析构，包括空类型，不采用无对象的 tag 特例或整块清零替代构造。支持类型 MUST 满足不抛移动构造、移动赋值及析构，允许普通构造抛异常；构造/移动/析构不得重入所属 Registry。注册与结构修改只接受裸 T；Get/TryGet 和查询必需组件允许 T 或 const T，二者查同一类型，const 仅表达访问权限。注册状态、Has 与 Exclude 只接受裸 T；引用及 volatile 等不支持形式 MUST 在编译期拒绝，不静默去修饰。类型别名不形成新类型。

#### Scenario: M3-T5-A09 ordinary empty and throwing component types
- **WHEN** 注册含资源的合法类型、空类型、不可默认构造类型或不满足移动/析构约束的类型
- **THEN** 合法类型运行真实生命周期并支持相应引用/指针；不可默认构造类型可显式参数新增，非法类型编译期拒绝，不承诺所有数值字段自动为零

#### Scenario: M3-T5-A10 const identity and compile-time misuse
- **WHEN** 请求 const T 读取、向 const Registry 索取组件，或用非法修饰/重复底层类型声明查询
- **THEN** 不产生 const 专属池，const Registry 不返回可写访问；非法模板形式及 T/const T 重复在编译期拒绝

### Requirement: Distinct insertion removal and value update

严格新增 Emplace SHALL 用显式参数构造缺失组件，已有组件具名拒绝而不覆盖。GetOrAdd SHALL 仅对可默认构造类型提供，存在时返回原对象、不重新构造，缺失时按类型正常初始化。Remove SHALL 在合法实体/已注册类型下返回是否实际移除，缺失为 false，最后一个组件移除后实体仍有效。初版不公开单实体 RemoveAllComponent。已有值可通过 typed 引用修改，但 SHALL 不提供通用 Replace/Patch、自动通知或值事务。

#### Scenario: M3-T5-A11 duplicate insertion and missing removal
- **WHEN** 对已有组件严格新增、获取或新增，或对有效实体移除存在/缺失组件
- **THEN** 分别拒绝重复新增、保留原对象或返回实际移除结果；不隐式重置旧值，不因组件数为零销毁实体

### Requirement: Checked reads and per-entity count

健康 Registry 的 IsEntityValid SHALL 对当前活实体返回 true，对普通失效 ID 返回 false；Failed 时具名拒绝检查。Get/Has/TryGet MUST 先满足合法实体与已注册类型前提，缺失组件分别为具名异常/false/空指针；非法 ID 不得混为缺失。NumComponents SHALL 返回有效实体挂载的组件种数，含空对象组件、无组件为零，不计 Registry 元数据或未挂载注册类型；非法 ID 具名拒绝，结果不得静默溢出。不扩展实体总数、逐池容量或反射枚举接口，不承诺常数时间。

#### Scenario: M3-T5-A12 missing versus invalid reads
- **WHEN** 有效实体缺组件、没有组件，或失效 ID 用于读取/计数
- **THEN** 正常缺失按入口区分，空实体计数为零且有效；非法 ID 的 Has/TryGet/Get/计数拒绝，专用 IsEntityValid 返回 false

### Requirement: Short-lived borrowing and constructor inputs

组件引用/指针 SHALL 仅短期借用：一旦操作实际进入结构存储修改，即使该操作失败且保留旧逻辑值，也不保证此前借用有效；纯预检拒绝或未触碰结构的无变化路径不终结借用。普通读取、同步嵌套查询及值修改本身不构成结构修改。查询组件借用 SHALL 只保证当前回调，不跨回调保存。调用方 MUST 在结构操作前准备真正自有的构造输入，不依赖同 Registry 组件/成员的借用；输入移动和构造外部副作用不纳入 ECS 回滚。普通 T& 的所有误用不承诺被自动捕获。

#### Scenario: M3-T5-A13 structural failure and reacquisition
- **WHEN** 取得组件引用后发生结构增长、增删、Clear，或进入结构阶段后失败
- **THEN** 调用方以 ID 重新读取，不解引用旧借用来测试失效；无状态改变的纯预检拒绝不虚构组件逻辑变化

### Requirement: Synchronous typed query and filtering

查询 SHALL 同步消费回调，不公开可逃逸 view、iterator、缓存查询或 builder。必需类型写在模板参数中，排除类型通过项目 Exclude 参数表达；匹配为全部必需且不含任一排除类型。必需/排除类型均须已注册；底层类型重复或同时必需/排除编译期拒绝。无必需类型时 SHALL 可遍历全部活实体，包括无组件实体；可选组件通过 TryGet 读取，不初建 OR 或运行时筛选框架。默认顺序未指定，不保证底层重排后的次序。

#### Scenario: M3-T5-A14 required excluded and empty query
- **WHEN** 使用必需/排除组合、无必需类型或没有匹配实体
- **THEN** 每个匹配活实体恰好访问一次，无漏项/重复，无匹配正常完成；未注册不伪装为空查询

### Requirement: Fixed callback protocol and independent termination

查询 SHALL 固定向 visitor 传 EntityId，再按模板声明顺序传组件引用，const T 为只读，不随驱动池变化；排除类型不传参数，无必需组件时只传 ID，不自动适配省略 ID 的形式。回调返回 void 表示继续，或返回显式 VisitControl::Continue/Stop，bool 等其他返回形式编译期拒绝。查询 SHALL 返回 Completed/Stopped；空查询为 Completed，末实体明确 Stop 仍为 Stopped。同步嵌套允许，各层退出独立；异常传播并恢复对应查询保护，不回滚已执行回调的值修改。

#### Scenario: M3-T5-A15 stop nesting and callback exception
- **WHEN** 内层 Stop、内层异常被外层捕获、外层 Stop 或 visitor 抛异常
- **THEN** 只退出相应层；仍有外层时保护持续，全部退出后释放保护，先前值修改不因退出而回滚

### Requirement: Structural entry guards and failure precedence

任意查询仍活动时，注册、创建、销毁、增删组件及 Clear 入口 MUST 在触碰存储前具名拒绝，即使重复注册、GetOrAdd 已有、Remove 缺失或空 Clear 预计无变化。读取/计数/注册状态和同步嵌套查询允许。受控入口 SHALL 先检查 Failed，再检查当前调用上下文，再检查身份/注册/参数/操作前提；同层多错不承诺细粒度顺序。检查不涵盖进入函数前的参数求值，也不是完整并发或隐藏借用误用检测。查询后结构修改 SHALL 由调用方收集自有值/非拥有 ID，全部层退出后显式执行，无自动队列或批事务。

#### Scenario: M3-T5-A16 forbidden no-op entry and multi-error call
- **WHEN** 活动查询中调用可能无变化的结构入口，或同次调用同时含非法 ID/未注册类型
- **THEN** 查询上下文拒绝优先且无结构触碰；Failed 状态优先于普通调用错误，不因恰好无变化绕过保护

### Requirement: Explicit complete-key ordered query

显式有序查询 SHALL 在查询保护内收集匹配 ID 及自有完整键，以 std::less<> 排序，在业务 visitor 开始前检查排序等价键；双向比较都不成立即平局，平局具名拒绝，不以原生 ID、地址或遇到顺序兜底。keySelector 只读，不借出长期键；键关系 MUST 满足严格弱序，NaN 等破坏前提的键不可直接使用，不承诺检测任意自定义键的全部错误。键生成/分配/比较抛异常时 SHALL 不开始业务遍历并恢复保护。顺序是本次键快照，visitor 改值不重排；不公开自定义比较器，不修改底层池顺序，不承诺跨运行确定性。

#### Scenario: M3-T5-A17 ordering ties and preparation failure
- **WHEN** 完整键唯一、存在平局，或键生成/排序准备失败
- **THEN** 唯一键按声明关系访问；平局或准备失败时业务 visitor 执行零次，异常传播且保护恢复

### Requirement: Recoverable failures and failed storage state

ECS SHALL 以项目具名异常表达契约错误，不靠 Release 断言、日志后继续或 exit 处理；bad_alloc 和用户构造/回调异常允许保留原始原因，OOM 不承诺完整文字。可恢复准备失败 SHALL 不发布半实体/半注册/半组件；单次新增失败不改变已有合法组件逻辑值，不等于容量、地址、输入副作用或多步业务装配回滚。若实际存储一致性无法确认，Registry MUST 进入 Failed 并停止普通访问，不自动 Clear 修复；仅保留无需遍历存储的最小自有状态摘要及析构清理。摘要区分已知操作/类别与未知，不持有借用字符串或异常对象，不覆盖原异常。业务字段非法或普通 visitor 抛错不自动表示 Registry 损坏；组件业务校验归上层，无自动校验钩子。

#### Scenario: M3-T5-A18 allocation constructor and value failure
- **WHEN** 注册/创建/新增准备分配失败、组件构造抛异常，或已有值更新/visitor 抛异常
- **THEN** 按对应阶段保留未发布和旧逻辑状态，值修改不获自动回滚；可确认一致性时仍可继续，无法确认时 Failed，不能把所有异常一律视为可恢复或存储损坏

#### Scenario: M3-T5-A19 failed access and safe diagnostics
- **WHEN** Registry 已 Failed 后调用普通读取、IsEntityValid、注册、Clear 或读取状态摘要
- **THEN** 普通入口具名拒绝，摘要仅返回已知自有事实而不扫描不可靠池，未知不补零，析构仍清理已取得资源

### Requirement: Serial access allocation and cleanup scope

同一及不同 Registry 的 ECS 访问 SHALL 由调用方外部串行协调，包括涉及共享类型/身份的生命周期；结束全部查询及借用并建立同步后可交接线程，不绑定创建线程，不承诺并发读取。初版 SHALL 不内置锁、线程池或调度器。存储使用默认标准分配、自动增长，不保留旧 256 类型上限或提供公开 Reserve/批量增删/自定义 allocator；实际容量耗尽和分配失败须明确。不保证跨实体/跨组件销毁次序，必要业务清理由上层提前组织。

#### Scenario: M3-T5-A20 handoff and unspecified cleanup order
- **WHEN** 调用方结束借用后同步交接线程，或移除/Clear/析构多个资源组件
- **THEN** 串行访问可继续、组件各自生命周期正确，但不保证跨类型/实体的先后；不由查询 guard 或独立实例推定并行安全
