---
tags:
  - area/spec
---

## ADDED Requirements

### Requirement: Headless NPC advance and action handoff

SimulationSession SHALL 在现有串行会话内推进 NPC 的决策、动作和真实角色物理。NPC 刚体 SHALL 遵守显式物理步进顺序，共用会话的固定步预算。动作请求、结果和可选显示交接 SHALL 为自有中立值，不包含组件借用或窗口、渲染器和 GPU 资源。CPU 调用方 SHALL 可使用正式会话、物理、World 和 ECS 完成移动、采集与结果交接。

用于动作表现和演示反馈的交接 SHALL 可关联本轮、实体及动作，提供实际位姿/位移、真实动作阶段、有效已耗时及总耗时、已提交结果与资源计数。交接 SHALL 在安全的推进及提交边界之外复制，保留就绪与已结算成功的区别；缺失事实不伪造为成功或零值。省略、重复读取或绘制这些值不改变模拟和资源结算。

#### Scenario: NPC-MVP-A15 Headless action loop
- **WHEN** 无窗口、无渲染器的调用方以真实 World 和正式会话推进人类 NPC 采集，并省略或重复读取显示数据
- **THEN** 世界、资源、动作结果与等价的游戏模拟输入及提交顺序一致，显示读取不会推进模拟、改变已交接值或消费另一份物理预算

### Requirement: Ordered harvest commit after player edits

普通帧和等价 CPU 序列 SHALL 在本次 Advance 结束后，先处理适用的玩家编辑，再串行处理本次已就绪的 NPC 采集。场景 SHALL 为可参与采集的 NPC 明确分配会话内唯一且在其存活期间稳定的顺序键，重复键在发布对应 NPC 前拒绝。就绪动作 SHALL 按键升序处理，每个动作在自己的提交位置重新核对当前实体、动作及 World 原目标，并完成该动作的提交与结果后再处理下一项。裁决 MUST 只依据该显式键；底层 ECS 遍历、原生身份、创建次序和请求收集次序均不得影响结果。可恢复的动作失败 SHALL 给出终态并继续处理后续就绪动作；不可恢复的提交故障 SHALL 停止后续提交及会话推进。现有场景编辑仍在 Advance 前提交。

#### Scenario: NPC-MVP-A24 Stable order for simultaneous contenders
- **WHEN** 两个 NPC 在同一次 Advance 达到同一原目标的采集耗时，玩家没有改变该目标，并以相同顺序键反转 NPC 装配次序和请求收集次序重复验证
- **THEN** 两次均由键较小的有效动作先提交并结算一次，键较大的动作报告目标失效；World 只发生一次有效移除

#### Scenario: NPC-MVP-A25 Rejected player edit and invalid ordering key
- **WHEN** 玩家编辑被拒绝且原目标仍有效，或装配试图为另一个可采集 NPC 使用重复顺序键
- **THEN** 前者继续按显式顺序处理 NPC 采集；后者具名拒绝重复键且对应 NPC 不发布为可推进实体
