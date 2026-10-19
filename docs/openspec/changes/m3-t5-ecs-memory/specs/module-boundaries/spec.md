---
tags:
  - area/spec
---

# T5 Module Boundaries Delta

沿用[现役 module-boundaries](../../../../specs/module-boundaries/spec.md)全部已有 requirements/scenarios，仅追加本次 typed 依赖、验证与定向退役要求。T4/T3 仍各自维护目标及批准，不在此重写业务或渲染行为。

## ADDED Requirements

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
