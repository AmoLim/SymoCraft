---
tags:
  - area/spec
---

## Purpose

定义策划行为树的编辑、运行、动作绑定和诊断边界。

## ADDED Requirements

### Requirement: Offline authoring and startup validation

项目 SHALL 采用 BehaviorTree.CPP 执行策划树，导出自定义节点及参数描述供 Groot2 FREE 离线编辑。树资产 SHALL 只在启动时加载，修改后重启生效。启动装载 SHALL 识别未知节点、非法参数及缺失子树并报告具体错误，不静默以 Idle 代替错误资产。

#### Scenario: NPC-MVP-A08 Edit, export and load
- **WHEN** 策划用导出的节点描述编辑并保存合法树，再启动游戏，或在运行中修改该树
- **THEN** 合法资产可加载并绑定动作，运行中已加载的树保持原版本，下次启动采用修改后的合法资产；整个流程可离线完成

#### Scenario: NPC-MVP-A09 Invalid tree assets
- **WHEN** 启动时树含未知节点、非法参数或无法解析的子树引用
- **THEN** 装载失败并报告资产及可定位的节点、参数或子树，游戏不以 Idle 掩盖错误

### Requirement: Isolated behavior and controlled action binding

每个 NPC SHALL 独立持有树运行状态、黑板及动作进度。行为树 SHALL 选择并组合底层动作，由动作层统一校验、推进、取消和报告结果。内建表达式 SHALL 限于算术、比较、逻辑及黑板赋值，不得绕过动作校验直接修改 World 或 ECS，黑板不持有第二份权威资源账。新能力 SHALL 可通过注册节点及参数进入编辑、运行和诊断流程，职业行为组合可通过资产表达。

#### Scenario: NPC-MVP-A10 Shared definition and independent state
- **WHEN** 两名 NPC 实例化同一树资产，并使用各自的输入、黑板和动作
- **THEN** 一个实例改变参数值、动作进度或结束状态时，另一实例保持自身状态；各自资源仍由玩法状态持有

#### Scenario: NPC-MVP-A11 Action extension
- **WHEN** 实施方添加一项底层能力、注册对应树节点及参数，并导出、编辑、加载和运行使用该节点的树
- **THEN** 编辑器能识别其节点描述，运行按底层动作规则处理参数和结果，诊断可定位该节点及其动作

### Requirement: Bounded first behavior and read-only diagnosis

人类 NPC SHALL 从场景配置或树输入取得指定方块，驱动移动及采集，成功或失败后进入 Idle，不自动搜索或重试。僵尸 SHALL 使用独立实体定义与 Idle 树。运行 SHALL 通过状态变化日志和按需自有只读快照关联 NPC、当前节点、目标、动作、进度、资源及失败原因。观测不得推进模拟或要求连接编辑器。

#### Scenario: NPC-MVP-A12 Terminal behavior and observation
- **WHEN** 一名人类成功采集、另一名人类采集失败，且仍继续 tick 这两棵树和僵尸的 Idle 树
- **THEN** 三者均保持 Idle，旧目标不自动触发新动作；状态变化可追踪，重复观察不重复输出状态变化、不推进动作或改写旧快照
