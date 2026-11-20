---
tags:
  - area/spec
---

[npc-ecs]: ../../../../../specs/ecs/spec.md

## Purpose

定义 NPC 装配、真实移动、耗时采集和资源计数。实体身份与借用沿用 [ECS][npc-ecs]，采集动作可独立于行为树验证。

## ADDED Requirements

### Requirement: NPC assembly and lifetime

游戏 SHALL 支持显式装配和移除人类、僵尸 NPC。薄 `LivingEntity` 行为门面 SHALL 以 `EntityId` 关联 `Player` 或 `Mob`，共享与专属组件由装配安装。ECS SHALL 唯一持有位姿、运动及玩法状态，门面不得长期保存组件地址。未完整装配的 NPC 不得成为可推进实体。成功移除 SHALL 终结其动作和行为树状态，旧身份不能命中后续实体。

#### Scenario: NPC-MVP-A01 Creation and removal
- **WHEN** 分别装配人类和僵尸，移除一个仍有动作的 NPC，再创建新 NPC
- **THEN** 存活实体的组件、动作与资源独立，已移除实体不再推进或产生资源，旧身份及其在途动作不能作用于新实体

### Requirement: Terrain-only movement to a work position

移动动作 SHALL 在受控、平坦、可直达的场景中用真实角色物理移动至目标附近的合法工作位置，抵达后才允许采集。NPC SHALL 只与地形碰撞，工作位置不得侵入目标方块。移动受阻时 SHALL 停止并给出可辨结果。

#### Scenario: NPC-MVP-A02 Reachable and blocked targets
- **WHEN** 分别为人类 NPC 提供可直达的工作位置和被地形阻断的工作位置
- **THEN** 前者通过真实物理位移抵达后开始采集，后者有界停止并报告受阻，二者均不传送、不穿入目标方块

#### Scenario: NPC-MVP-A03 Characters do not block each other
- **WHEN** NPC 的合法移动路线经过其他 NPC 或玩家的位姿
- **THEN** 角色可以重叠，地形碰撞继续生效

### Requirement: Timed harvest action

人类 NPC SHALL 能对明确指定的有效资源方块发起采集。有效耗时 SHALL 为方块基础秒数除以 NPC 采集效率，效率默认 1.0。参数及有效耗时 MUST 有限且为正。每次动作 SHALL 持有独立进度，并在开始、推进及提交时校验目标。合法抵达后，从下一次 `Advance` 开始按会话校验与裁剪后的有效规则时间累计，独立于该次完成的物理步数。显式暂停 SHALL 冻结进度，恢复首个成功 `Advance` 以零有效规则时间推进。取消、任务切换或中断 SHALL 清零。

#### Scenario: NPC-MVP-A04 Timing and invalid parameters
- **WHEN** 使用默认效率或另一合法效率对有效目标开始采集，或提供零、负值、非有限或导致无效耗时的参数
- **THEN** 合法动作按各自的有效耗时推进，达到门槛前不移除目标或增加资源；非法参数具名拒绝

#### Scenario: NPC-MVP-A05 Pause, cancellation and target replacement
- **WHEN** 采集中暂停再恢复、取消再发起，或原方块被移除或替换
- **THEN** 暂停不计入进度，恢复不补算暂停时间；新动作从零开始；旧目标失效时旧动作失败，不能延续到替换后的方块

#### Scenario: NPC-MVP-A30 Harvest uses effective rule time
- **WHEN** 合法抵达工作位置，随后用已进入采集的动作分别验证正时间且零物理步、超出规则时间上限的长帧，以及暂停后首次成功恢复的推进
- **THEN** 抵达当次不追记移动时间；后续分别只累计会话实际给出的正有效规则时间、裁剪后的有效规则时间和零秒，不以物理步数、原始帧时长或暂停时长代替

### Requirement: Single resource credit for a completed harvest

成功采集 SHALL 通过真实 World 编辑移除仍有效的目标，并且仅向完成该动作的 NPC 的资源计数器增加一次该规则的固定数量。计数 SHALL 按资源 ID 区分。提交请求、Rejected、Unchanged 或目标失效 MUST 不产出资源，重复推进不得重复结算。争抢时各动作独立累计，首个成功移除者取得该目标的产出，其余失效动作失败。

#### Scenario: NPC-MVP-A06 Contest and duplicate completion
- **WHEN** 两名 NPC 采集同一有效方块并先后得到提交结果，随后重复推进已结束的动作
- **THEN** World 只发生一次有效移除，仅成功者增加一次对应资源；另一动作失败，重复推进保持资源计数

#### Scenario: NPC-MVP-A07 Unsuccessful edit
- **WHEN** 采集提交被 World 拒绝、未发生变化或目标已失效
- **THEN** 本动作不给任何 NPC 增加资源，也不报告采集成功

### Requirement: Prepare resource credit before removing a block

采集提交 SHALL 在调用 World 条件移除前确认 NPC 身份及当前动作有效，校验资源数量相加可表示，并准备资源写入与终态发布所需的全部可失败工作，包括可能的分配。准备中的状态 SHALL 对玩法和观测不可见。可恢复准备失败 SHALL 具名结束本动作，保留该动作提交前的方块、World 版本和资源计数。World 未成功改变有效目标时 SHALL 放弃已准备的入账，按 World 结果或故障语义处理。

World 成功改变有效目标后，采集 SHALL 在同一串行提交边界内完成一次资源更新并发布成功终态。该阶段 MUST 无分配、无可失败的业务校验或外部回调；对后续动作、诊断和显示仅发布完成后的状态。若该不变量仍被破坏，SHALL 将会话标为 Failed，停止后续提交与推进，尽可用资源报告首因、故障阶段和已知提交范围，未知事实显式标为未知。已提交状态保留；自动重试、补发和回填方块均禁止。

#### Scenario: NPC-MVP-A26 Credit preparation failure preserves the target
- **WHEN** 采集已就绪，但资源计数将溢出或用于准备入账或终态的分配失败，且存储仍一致
- **THEN** 失败动作未执行 World 移除，方块、World 版本和所有资源计数保持原值；报告准备失败并进入 Idle。若同帧有后续有效 NPC 请求同一目标，仍可按顺序提交

#### Scenario: NPC-MVP-A27 Prepared credit is discarded when World does not change
- **WHEN** 入账和终态准备完成，但 World 条件移除返回 Rejected、Unchanged，或因版本耗尽等准备故障无法提交
- **THEN** 资源计数保持原值，未发布采集成功；World 内容及版本按现行编辑失败保证保持，故障阶段可区分于入账准备失败

#### Scenario: NPC-MVP-A28 Successful removal and credit complete together
- **WHEN** 准备成功且 World 确认移除了原目标，随后读取结果、推进下一动作或重复交付同一动作
- **THEN** 第一次可观察到的完成状态同时包含目标移除、正确数量的一次入账和成功终态；之后不会因重复交付产生额外移除或入账

#### Scenario: NPC-MVP-A29 Post-removal invariant failure stops the session
- **WHEN** 在确认 World 已改变后注入入账或终态发布不变量破坏
- **THEN** 会话进入 Failed 并停止后续提交，报告首因和可确认的 World、资源及动作事实；未能确认的结果保持未知，既有提交保留，不通过重放或恢复方块掩盖故障
